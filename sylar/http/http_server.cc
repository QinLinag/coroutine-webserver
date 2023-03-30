#include "http_server.h"
#include "sylar/log.h"

namespace sylar {
namespace http {


static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive 
                        ,sylar::IOManager* worker 
                        ,sylar::IOManager* accept_worker) 
    :TcpServer(worker, accept_worker)
    ,m_isKeepalive(keepalive)
    ,m_dispatch.reset(new ServletDispatch()){
}
    
void HttpServer::handleClient(Socket::ptr client) {
    HttpSession::ptr session(new HttpSession(client));   //如果server连接到了一个浏览器请求， 就要为这个连接创建一个httpSession，
    do {
        auto req = session->recvRequest();
        if(!req) {
            SYLAR_LOG_WARN(g_logger) << "recv http request fail, errno= "
                << errno << "strerror = " << strerror(errno)
                << "client:" << *client;
            break;
        }
        HttpResponse::ptr rsp(new HttpResponse(req->getVersion()
                    , req->isClose() || !m_isKeepalive));
        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);   //在这里，当m_dispatch->handle函数处理完成后，就通过sendResponse函数将response写会给浏览器，
    } while(true);
    session->close();
}

}

}