#pragma once

#include <i_server.hpp>

#include <type_traits>

namespace rendezvouscxx
{

template <typename ISrv>
class simple_server_base : public ISrv
{
    static_assert(std::is_base_of_v<i_server, ISrv>, "Must implement the rendezvouscxx::i_server interface");

public:
    void on_client_connected() override {}
    void on_client_disconnecting() override {}
};

} // namespace rendezvouscxx

