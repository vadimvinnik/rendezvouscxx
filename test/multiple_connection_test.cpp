#include  "rendezvouscxx.hpp"
#include  "i_server.hpp"

#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <mutex>
#include <thread>

namespace
{

class i_test_server_t : public rendezvouscxx::i_server
{
public:
    virtual void func() = 0;
};

class test_server_t : public i_test_server_t
{
public:
    test_server_t() :
        m_call_count { 0 }
    {
        m_mx_before_call.lock();
        m_mx_after_call.lock();
    }

    void unlock_call()
    {
        m_mx_before_call.unlock();
    }

    void expect_call()
    {
        m_mx_after_call.lock();
    }

    int call_count() const { return m_call_count; }

private:
    void on_client_connected() override {}
    void on_client_disconnecting() override {}

    void func() override
    {
        m_mx_before_call.lock();
        ++m_call_count;
        m_mx_after_call.unlock();
    }

    std::mutex m_mx_before_call;
    std::mutex m_mx_after_call;
    int m_call_count;
};

using test_gate_t = rendezvouscxx::gate_t<i_test_server_t>;

} // namespace

TEST_CASE("A single client connecting to a single server thread", "[RendezVousCxx]")
{
    test_gate_t gate;

    test_server_t server;

    auto const client_func = [&gate]()
    {
        std::cout << "Client waits for a server\n";
        auto const guard = gate.connect_client();
        std::cout << "Client makes a call\n";
        guard->server().func();
        std::cout << "Client finishes\n";
    };

    auto const server_func = [&server, &gate]()
    {
        std::cout << "Server waits for the  client 1\n";
        gate.connect_server(server);
        std::cout << "Server finished\n";
    };

    std::thread server_thread { server_func };

    std::thread client_thread { client_func };

    server.unlock_call();
    server.expect_call();

    client_thread.join();

    server_thread.join();

    CHECK(server.call_count() == 1);
}

TEST_CASE("Multiple clients connecting to a single server thread", "[RendezVousCxx]")
{
    test_gate_t gate;

    test_server_t server;

    auto const client_func = [&gate]()
    {
        std::cout << "Client waits for a server\n";
        auto const guard = gate.connect_client();
        std::cout << "Client makes a call\n";
        guard->server().func();
        std::cout << "Client finishes\n";
    };

    auto const server_func = [&server, &gate]()
    {
        std::cout << "Server waits for the  client 1\n";
        gate.connect_server(server);
        std::cout << "Server waits for the client 2\n";
        gate.connect_server(server);
        std::cout << "Server waits for the client 3\n";
        gate.connect_server(server);
        std::cout << "Server finished\n";
    };

    std::thread server_thread { server_func };

    std::thread client_thread_1 { client_func };
    std::thread client_thread_2 { client_func };
    std::thread client_thread_3 { client_func };

    server.unlock_call();
    server.expect_call();
    server.unlock_call();
    server.expect_call();
    server.unlock_call();
    server.expect_call();

    client_thread_1.join();
    client_thread_2.join();
    client_thread_3.join();

    server_thread.join();

    CHECK(server.call_count() == 3);
}

TEST_CASE("Multiple clients connecting to multiple server threads", "[RendezVousCxx]")
{
    test_gate_t gate;

    test_server_t server;

    auto const client_func = [&gate]()
    {
        std::cout << "Client waits for a server\n";
        auto const guard = gate.connect_client();
        std::cout << "Client makes a call\n";
        guard->server().func();
        std::cout << "Client finishes\n";
    };

    auto const server_func = [&server, &gate]()
    {
        std::cout << "Server waits for a client\n";
        gate.connect_server(server);
        std::cout << "Server finished\n";
    };

    std::thread server_thread_1 { server_func };
    std::thread server_thread_2 { server_func };
    std::thread server_thread_3 { server_func };

    std::thread client_thread_1 { client_func };
    std::thread client_thread_2 { client_func };
    std::thread client_thread_3 { client_func };

    server.unlock_call();
    server.expect_call();
    server.unlock_call();
    server.expect_call();
    server.unlock_call();
    server.expect_call();

    client_thread_1.join();
    client_thread_2.join();
    client_thread_3.join();

    server_thread_1.join();
    server_thread_2.join();
    server_thread_3.join();

    CHECK(server.call_count() == 3);
}

