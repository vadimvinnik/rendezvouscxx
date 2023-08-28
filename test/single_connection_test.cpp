#include  "rendezvouscxx.hpp"
#include  "i_server.hpp"

#include <catch2/catch_test_macros.hpp>

#include <mutex>
#include <thread>

namespace
{

class i_test_server_t : public rendezvouscxx::i_server
{
public:
    virtual void set_x(int x) = 0;
    virtual void set_y(int y) = 0;
    virtual int sum() const = 0;
};

class initially_locked_mutex : public std::mutex
{
public:
    initially_locked_mutex()
    {
        lock();
    }
};

class test_server_t : public i_test_server_t
{
public:
    test_server_t() :
        m_x { 0 },
        m_y { 0 }
    {}

    void expect_client_connected() { m_mx_client_connected.lock(); }
    void expect_client_disconnecting() { m_mx_client_disconnecting.lock(); }
    int get_x() const { return m_x; }
    int get_y() const { return m_y; }

private:
    void on_client_connected() override { m_mx_client_connected.unlock(); }
    void on_client_disconnecting() override { m_mx_client_disconnecting.unlock(); }
    void set_x(int x) override { m_x = x; }
    void set_y(int y) override { m_y = y; }
    int sum() const override { return m_x + m_y; }

    initially_locked_mutex m_mx_client_connected;
    initially_locked_mutex m_mx_client_disconnecting;
    int m_x;
    int m_y;
};

using test_gate_t = rendezvouscxx::gate_t<i_test_server_t>;

class client_thread_t
{
public:
    explicit client_thread_t(test_gate_t& gate) :
        m_sum(0),
        m_gate(gate),
        m_thread([this]() { run_impl(); })
    {}

    void go_connect() { m_mx_connect.unlock(); }
    void go_set_x  () { m_mx_set_x  .unlock(); }
    void go_set_y  () { m_mx_set_y  .unlock(); }
    void go_sum    () { m_mx_sum    .unlock(); }
    void go_finish () { m_mx_finish .unlock(); }

    int sum() { m_mx_sum_ready.lock(); return m_sum; }

    void expect_stop() { m_thread.join(); }

private:
    void run_impl()
    {
        m_mx_connect.lock();
        auto guard = m_gate.connect_client();

        m_mx_set_x.lock();
        guard->server().set_x(1000);

        m_mx_set_y.lock();
        guard->server().set_y(1);

        m_mx_sum.lock();
        m_sum = guard->server().sum();
        m_mx_sum_ready.unlock();

        m_mx_finish.lock();
    }

    int m_sum;
    test_gate_t& m_gate;
    initially_locked_mutex m_mx_connect;
    initially_locked_mutex m_mx_set_x;
    initially_locked_mutex m_mx_set_y;
    initially_locked_mutex m_mx_sum;
    initially_locked_mutex m_mx_sum_ready;
    initially_locked_mutex m_mx_finish;
    std::thread m_thread;
};

class server_thread_t
{
public:
    explicit server_thread_t(test_gate_t& gate) :
        m_gate(gate),
        m_thread([this]() { run_impl(); })
    {}

    void go_connect() { m_mx_connect.unlock(); }
    void go_finish () { m_mx_finish .unlock(); }

    void expect_stop() { m_thread.join(); }

    test_server_t& server() { return m_server; }

private:
    void run_impl()
    {
        m_mx_connect.lock();
        m_gate.connect_server(m_server);
        m_mx_finish.lock();
    }

    test_server_t m_server;
    test_gate_t& m_gate;
    initially_locked_mutex m_mx_connect;
    initially_locked_mutex m_mx_finish;
    std::thread m_thread;
};

} // namespace

TEST_CASE("No ordering", "[RendezVousCxx]")
{
    rendezvouscxx::gate_t<i_test_server_t> test_gate;

    client_thread_t client_thread { test_gate };
    server_thread_t server_thread { test_gate };

    client_thread.go_set_x();
    client_thread.go_set_y();
    client_thread.go_sum();
    client_thread.go_finish();
    server_thread.go_finish();
    client_thread.go_connect();
    server_thread.go_connect();
    client_thread.expect_stop();
    server_thread.expect_stop();

    CHECK(server_thread.server().get_x() == 1000);
    CHECK(server_thread.server().get_y() == 1);
    CHECK(client_thread.sum() == 1001);
}

TEST_CASE("Client connects first and waits for a server", "[RendezVousCxx]")
{
    rendezvouscxx::gate_t<i_test_server_t> test_gate;

    client_thread_t client_thread { test_gate };
    server_thread_t server_thread { test_gate };

    client_thread.go_connect();
    server_thread.go_connect();
    server_thread.server().expect_client_connected();
    client_thread.go_set_x();
    client_thread.go_set_y();
    client_thread.go_sum();
    client_thread.go_finish();
    server_thread.go_finish();
    server_thread.server().expect_client_disconnecting();
    client_thread.expect_stop();
    server_thread.expect_stop();

    CHECK(server_thread.server().get_x() == 1000);
    CHECK(server_thread.server().get_y() == 1);
    CHECK(client_thread.sum() == 1001);
}

TEST_CASE("Server connects first and waits for a client", "[RendezVousCxx]")
{
    rendezvouscxx::gate_t<i_test_server_t> test_gate;

    client_thread_t client_thread { test_gate };
    server_thread_t server_thread { test_gate };

    server_thread.go_connect();
    client_thread.go_connect();
    server_thread.server().expect_client_connected();
    client_thread.go_set_x();
    client_thread.go_set_y();
    client_thread.go_sum();
    client_thread.go_finish();
    server_thread.go_finish();
    server_thread.server().expect_client_disconnecting();
    client_thread.expect_stop();
    server_thread.expect_stop();

    CHECK(server_thread.server().get_x() == 1000);
    CHECK(server_thread.server().get_y() == 1);
    CHECK(client_thread.sum() == 1001);
}

TEST_CASE("Client connects first, server connects and tries to finish immediately", "[RendezVousCxx]")
{
    rendezvouscxx::gate_t<i_test_server_t> test_gate;

    client_thread_t client_thread { test_gate };
    server_thread_t server_thread { test_gate };

    client_thread.go_connect();
    server_thread.go_connect();
    server_thread.go_finish();
    server_thread.server().expect_client_connected();
    client_thread.go_set_x();
    client_thread.go_set_y();
    client_thread.go_sum();
    client_thread.go_finish();
    server_thread.server().expect_client_disconnecting();
    client_thread.expect_stop();
    server_thread.expect_stop();

    CHECK(server_thread.server().get_x() == 1000);
    CHECK(server_thread.server().get_y() == 1);
    CHECK(client_thread.sum() == 1001);
}

TEST_CASE("Server connects first, waits for a client and tries to finish immediately", "[RendezVousCxx]")
{
    rendezvouscxx::gate_t<i_test_server_t> test_gate;

    client_thread_t client_thread { test_gate };
    server_thread_t server_thread { test_gate };

    server_thread.go_connect();
    client_thread.go_connect();
    server_thread.go_finish();
    server_thread.server().expect_client_connected();
    client_thread.go_set_x();
    client_thread.go_set_y();
    client_thread.go_sum();
    client_thread.go_finish();
    server_thread.server().expect_client_disconnecting();
    client_thread.expect_stop();
    server_thread.expect_stop();

    CHECK(server_thread.server().get_x() == 1000);
    CHECK(server_thread.server().get_y() == 1);
    CHECK(client_thread.sum() == 1001);
}

