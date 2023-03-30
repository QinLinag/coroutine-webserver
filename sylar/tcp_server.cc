#include "tcp_server.h"
#include "config.h"
#include "log.h"


namespace sylar {

static sylar::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout = 
    sylar::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2), 
        "tcp server read timeout");

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

TcpServer::TcpServer(sylar::IOManager* woker
                        , sylar::IOManager* accept_worker) 

        :m_worker(woker)
        ,m_acceptWorker(accept_worker)
        ,m_recvTimeout(g_tcp_server_read_timeout->getValue())
        ,m_name("sylar/1.0.0")
        ,m_isStop(true){
}

TcpServer::~TcpServer() {
    for(auto& i : m_socks) {
        i->close();
    }
    m_socks.clear();
}


bool TcpServer::bind(sylar::Address::ptr addr) {
    std::vector<sylar::Address::ptr> addrs;
    std::vector<sylar::Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

bool TcpServer::bind(const std::vector<sylar::Address::ptr>& addrs
                    , std::vector<sylar::Address::ptr>& fails) {
    for(auto& addr : addrs) {
        sylar::Socket::ptr sock = Socket::CreateTCP(addr);
        if(!sock->bind(addr)) {
            SYLAR_LOG_ERROR(g_logger) << "bind fail errno="
                        << errno << " errstr=" << strerror(errno)
                        << " addr =[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        if(!sock->listen()) {
            SYLAR_LOG_ERROR(g_logger) << "listen fail errno = "
                    << errno << "errstr= " << strerror(errno)
                    << " addr=[" << addr->toString() << "]";
            continue;
        }
        m_socks.push_back(sock);
    }
    if(!fails.empty()) {
        for(auto& i : m_socks) {
        i->close();
        }
        m_socks.clear();
        return false;
    }
    for(auto& i : m_socks) {
        SYLAR_LOG_INFO(g_logger) << "server bind success:" << *i;
    }
    return true;
}

bool TcpServer::start() {
    if(!m_isStop) {
        return true;
    }
    m_isStop = false;
    for(auto& sock : m_socks) {
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept,
            shared_from_this(), sock));
    }
    return true;
}

void TcpServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self](){
        for(auto& sock : m_socks) {
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}

void TcpServer::startAccept(Socket::ptr sock) {   //服务器开始接受请求，
    while(!m_isStop) {  //这里循环，就可以不停的监听来请求的客户端，
        Socket::ptr client = sock->accept();
        if(client) {   //如果这里服务端接受了这个客户端，那么就通过携程去处理客户端的请求，
            m_worker->schedule(std::bind(&TcpServer::handleClient, 
                        shared_from_this(), client));
        } else {
            SYLAR_LOG_ERROR(g_logger) << "accept errno = " << errno
                    << " errstr= " << strerror(errno);
        }
    }
}


void TcpServer::handleClient(Socket::ptr client) {  //这个函数放到具体继承这个类的类实现，   handleClient函数主要用于处理服务器和客户端到底有什么交互，
    SYLAR_LOG_INFO(g_logger) << "handleClient:" << *client;
}




}