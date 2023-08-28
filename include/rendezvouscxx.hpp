#pragma once

#include <i_server.hpp>

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

#ifdef RENDEZVOUSCXX_ENABLE_TRACE
#include <iostream>
#include <sstream>
#include <thread>
#define RENDEZVOUSCXX_TRACE(message) \
    do \
    { \
        std::ostringstream stream; \
        stream << "[RendesVousCxx] thread " << std::this_thread::get_id() << " " << message << std::endl; \
        std::cout << stream.str(); \
    } while (false)
#else
#define RENDEZVOUSCXX_TRACE(message) do {} while (false)
#endif

namespace rendezvouscxx
{

template <typename ISrv>
class gate_t
{
    static_assert(std::is_base_of_v<i_server, ISrv>, "Must implement the rendezvouscxx::i_server interface");

public:
    class client_guard_t
    {
    public:
        client_guard_t(gate_t<ISrv>& gate, std::unique_lock<std::mutex> lock) :
            m_gate(gate),
            m_lock(std::move(lock))
        {
            RENDEZVOUSCXX_TRACE("client_guard_t ctor");
        }

        ~client_guard_t()
        {
            RENDEZVOUSCXX_TRACE("client_guard_t dtor");
            m_gate.on_client_disconnecting();
        }

        client_guard_t(client_guard_t const&) = delete;
        client_guard_t& operator=(client_guard_t const&) = delete;

        ISrv& server()
        {
            RENDEZVOUSCXX_TRACE("client_guard_t::server()");
            return *m_gate.m_server;
        }

    private:
        gate_t<ISrv>& m_gate;
        std::unique_lock<std::mutex> m_lock;
    };

    gate_t() :
        m_is_client_connected { false },
        m_server { nullptr }
    {}

    std::unique_ptr<client_guard_t> connect_client()
    {
        RENDEZVOUSCXX_TRACE("connect_client() waits for the client mutex");
        std::unique_lock client_lock(m_mx_client);

        RENDEZVOUSCXX_TRACE("connect_client() waits for the common mutex");
        std::unique_lock common_lock(m_mx_common);

        RENDEZVOUSCXX_TRACE("connect_client() notifies about getting connected");
        m_is_client_connected = true;
        m_cv_client_chaned.notify_all();

        RENDEZVOUSCXX_TRACE("connect_client() waits for the server");
        m_cv_server_chaned.wait(
            common_lock,
            [this]() { return m_server != nullptr; });
        m_is_mutual_connection = true;

        RENDEZVOUSCXX_TRACE("connect_client() has established a mutual connection");
        static_cast<i_server*>(m_server)->on_client_connected();
        return std::make_unique<client_guard_t>(*this, std::move(client_lock));
    }

    void connect_server(ISrv& server)
    {
        RENDEZVOUSCXX_TRACE("connect_server() waits for the server mutex");
        std::unique_lock lock(m_mx_server);

        RENDEZVOUSCXX_TRACE("connect_server() waits for the common mutex");
        std::unique_lock common_lock(m_mx_common);

        RENDEZVOUSCXX_TRACE("connect_server() waits the client to connect");
        m_cv_client_chaned.wait(
            common_lock,
            [this]() { return m_is_client_connected; });

        RENDEZVOUSCXX_TRACE("connect_server() notifies about getting connected");
        m_server = &server;
        m_cv_server_chaned.notify_all();

        RENDEZVOUSCXX_TRACE("connect_server() has got a client and waits it to get disconnected");
        m_cv_client_chaned.wait(
            common_lock,
            [this]() { return !m_is_client_connected; });

        RENDEZVOUSCXX_TRACE("connect_server() breaks the mutual connection");
        m_is_mutual_connection = false;
        m_cv_disconnected.notify_all();
    }

private:
    void on_client_disconnecting()
    {
        RENDEZVOUSCXX_TRACE("on_client_disconnecting() waits for the common mutex");
        std::unique_lock lock(m_mx_common);

        RENDEZVOUSCXX_TRACE("on_client_disconnecting() notifies about getting disconnected");
        static_cast<i_server*>(m_server)->on_client_disconnecting();
        m_is_client_connected = false;
        m_server = nullptr;
        m_cv_client_chaned.notify_all();

        RENDEZVOUSCXX_TRACE("on_client_disconnecting() waits for disconnection to be confirmed");
        m_cv_disconnected.wait(
            lock,
            [this]() { return !m_is_mutual_connection; });
    }

    std::mutex m_mx_client;
    std::mutex m_mx_server;
    std::mutex m_mx_common;

    std::condition_variable m_cv_client_chaned;
    std::condition_variable m_cv_server_chaned;
    std::condition_variable m_cv_disconnected;

    bool m_is_client_connected;
    ISrv* m_server;
    bool m_is_mutual_connection;
};

} // namespace rendezvouscxx

