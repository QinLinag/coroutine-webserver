#ifndef __SYLAR_SOCKET_STREAM_H__
#define __SYLAR_SOCKET_STREAM_H__

#include "sylar/stream.h"
#include "socket.h"

namespace sylar{

class SocketStream : public Stream{
public:
    typedef std::shared_ptr<SocketStream> ptr;
    SocketStream(Socket::ptr sock, bool owner = true);
    ~SocketStream();

    virtual int read(void* buffer, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;

    virtual int write(const void* buffer, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override;

    bool isConnected() const;
    Socket::ptr getSocket() { return m_socket;}

    virtual void close() override;

protected:
    Socket::ptr m_socket;
    bool m_owner;

};

}


#endif