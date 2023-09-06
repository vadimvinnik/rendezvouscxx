#define RENDEZVOUSCXX_ENABLE_WAIT_COUNTER
#include  <rendezvouscxx.hpp>

#include  <rendezvouscxx/i_server.hpp>

#include <catch2/catch_test_macros.hpp>

#include <mutex>
#include <thread>

namespace
{

class i_test_server_t : public rendezvouscxx::i_server
{};

class test_server_t : public i_test_server_t
{
private:
    void on_client_connected() override {}
    void on_client_disconnecting() override {}
};

using test_gate_t = rendezvouscxx::gate_t<i_test_server_t>;
using test_client_gate_t = typename test_gate_t::client_t;
using test_server_gate_t = typename test_gate_t::server_t;

void gate_closer(uint32_t old_counter, test_gate_t &gate)
{
    do {} while (gate.wait_count() == old_counter);
    gate.close();
}

void close_gate_when_waiting(test_gate_t &gate)
{
    std::thread closer_thread(gate_closer, gate.wait_count(), std::ref(gate));
    closer_thread.detach();
}

}

TEST_CASE("The gate closes, then one client tries to connect", "[RendezVousCxx]")
{
    test_gate_t gate;
    auto client_gate = gate.client();

    gate.close();

    auto const guard = client_gate.connect();
    CHECK(!guard);
}

TEST_CASE("The gate closes, then one server tries to connect", "[RendezVousCxx]")
{
    test_gate_t gate;
    auto server_gate = gate.server();

    gate.close();

    test_server_t server;
    auto const is_connected = server_gate.connect(server);
    CHECK(!is_connected);
}

TEST_CASE("One client tries to connect, then the gate closes", "[RendezVousCxx]")
{
    test_gate_t gate;
    auto client_gate = gate.client();

    close_gate_when_waiting(gate);

    auto const guard = client_gate.connect();
    CHECK(!guard);
}

TEST_CASE("One server tries to connect, then the gate closes", "[RendezVousCxx]")
{
    test_gate_t gate;
    auto server_gate = gate.server();

    close_gate_when_waiting(gate);

    test_server_t server;
    auto const is_connected = server_gate.connect(server);
    CHECK(!is_connected);
}

