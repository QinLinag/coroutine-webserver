#include "sylar/socket.h"
#include "sylar/sylar.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_socket() {
    sylar::IPAddress::ptr addr = sylar::Address::LookupAnyIPAddress("www.baidu.com");
    if(addr) {
        SYLAR_LOG_INFO(g_logger) << "get address:" << addr->toString();
    } else {
        SYLAR_LOG_ERROR(g_logger) << "get address fail";
        return;
    }

    sylar::Socket::ptr sock = sylar::Socket::CreateTCP(addr);
    addr->setPort(80);
    if(!sock->connect(addr)) {
        SYLAR_LOG_ERROR(g_logger) << "connect" << addr->toString() << "fail";
        return;
    } else {
        SYLAR_LOG_INFO(g_logger) << "connect" << addr->toString() << " connected";
    }

    const char buffer[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buffer, sizeof(buffer));
    if(rt <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "send fail rt = " << rt;
        return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if(rt <= 0) {
        SYLAR_LOG_ERROR(g_logger) << "recv fail rt = " << rt;
        return;
    }

    buffs.resize(rt);
    SYLAR_LOG_INFO(g_logger) << buffs;
}

int main(int argc, char** argv) {
    sylar::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}