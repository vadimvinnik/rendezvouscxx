#pragma once

namespace rendezvouscxx
{

class i_server
{
public:
    virtual ~i_server() = default;
    virtual void on_client_connected() = 0;
    virtual void on_client_disconnecting() = 0;
};

} // namespace rendezvouscxx

