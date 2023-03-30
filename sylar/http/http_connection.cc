#include "http_parser.h"
#include "http_connection.h"
#include "sylar/util.h"
#include "log.h"

namespace sylar {
namespace http {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

std::string HttpResult::toString() const {
    std::stringstream ss;
    ss << "[HttpResult result= " << result
       << " error=" << error
       << " response= " << (response ? response->toString() : "nullptr")
       << "]";
    return ss.str();
}



HttpConnection::HttpConnection(Socket::ptr sock, bool owner) 
    :SocketStream(sock, owner){
}

HttpResponse::ptr HttpConnection::recvResponse() {
    HttpResponseParser::ptr parser(new HttpResponseParser());
    uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize();
    std::shared_ptr<char> buffer(
            new char[buff_size + 1], [](char* ptr){
                delete[] ptr;
            });    

    char* data = buffer.get();
    int offset = 0;
    do {
        int len = read(data + offset, buff_size - offset);
        if(len <= 0) {
            close();
            return nullptr;
        }
        len += offset;
        data[len] = '\0';
        size_t nparse = parser->execute(data,len, false); //处理了多少，
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        offset = len - nparse;    //剩下还没有处理的
        if(offset == (int)buff_size) {
            close();
            return nullptr;
        }
        if(parser->isFinished()) {
            break;
        }
    } while(true);
    auto& client_parser = parser->getParser();
    std::string body;
    if(client_parser.chunked) {
        int len = offset;       //处理完response，body也在上面循环读了一些进入了data，大小是offset，
        do {
            do {
                int rt = read(data + len, buff_size - len);
                if(rt <= 0) {
                    return nullptr;
                }
                len += rt;
                data[len] = '\0';
                size_t nparse = parser->execute(data, len, true);
                if(parser->hasError()) {
                    return nullptr;
                }
                len -= nparse;
                if(len == (int)buff_size) {
                    return nullptr;
                }
            } while(!parser->isFinished());
            if(client_parser.content_len + 2 <= len) {
                body.append(data, client_parser.content_len);
                memmove(data, data + client_parser.content_len
                            , len - client_parser.content_len);   //dest src count三个参数，memmove函数是将src的前count个字符拷贝到dest中，
                len -= client_parser.content_len;
            }else {
                body.append(data, len);
                int left = client_parser.content_len - len;   
                while(left > 0) {
                    int rt = read(data, left > (int)buff_size ? (int)buff_size : left);
                    if(rt <= 0) {
                        return nullptr;
                    }
                    body.append(data, rt);
                    left -= rt;
                }
                len = 0;
            }
        } while(!client_parser.chunks_done);
        parser->getData()->setBody(body);
    } else {
        int64_t length = parser->getContentLength();
        if(length > 0) {
            std::string body;
            body.resize(length);

            int len = 0;
            if(length >= offset) {
                memcpy(&body[0], data, offset);
                len = offset;
            } else {
                memcpy(&body[0], data, length);
                len = length;
            }
            length -= offset;
            if(length > 0) {
                if(readFixSize(&body[len], length <= 0)) {
                    close();
                    return nullptr;
                }
            }
            parser->getData()->setBody(body);
        }
    }
    return parser->getData();
}

int HttpConnection::sendRequest(HttpRequest::ptr req) {
    std::stringstream ss;
    ss << *req;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}

HttpResult::ptr HttpConnection::DoGet(const std::string& url
                            , uint64_t timeout_ms
                            , const std::map<std::string, std::string>& headers 
                            , const std::string& body ) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URI
                , nullptr, "invalid url:" + url);
    }
    return DoGet(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoGet(Uri::ptr uri
                        , uint64_t timeout_ms
                        , const std::map<std::string, std::string>& headers 
                        , const std::string& body ) {
    DoRequest(HttpMethod::GET, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(const std::string& url
                        , uint64_t timeout_ms
                        , const std::map<std::string, std::string>& headers 
                        , const std::string& body ) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URI
                , nullptr, "invalid url:" + url);
    }
    return DoPost(uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(Uri::ptr uri
                        , uint64_t timeout_ms
                        , const std::map<std::string, std::string>& headers 
                        , const std::string& body ) {
    DoRequest(HttpMethod::POST, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                        , const std::string& url
                        , uint64_t timeout_ms
                        , const std::map<std::string, std::string>& headers 
                        , const std::string& body ) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URI
                , nullptr, "invalid url:" + url);
    }
    return DoRequest(method, uri, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                        , Uri::ptr uri
                        , uint64_t timeout_ms
                        , const std::map<std::string, std::string>& headers 
                        , const std::string& body ) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setMethod(method);
    bool has_host = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);   
            }
            continue;
        }
        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }
        req->setHeader(i.first, i.second);
    }
    if(!has_host) {
        req->setHeader("Host", uri->getHost());
    }
    req->setBody(body);
    return DoRequest(req, uri, timeout_ms);
}

HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req
                        , Uri::ptr uri
                        , uint64_t timeout_ms) {
    Address::ptr addr = uri->createAddress();
    if(!addr) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST
                    , nullptr, "invalid host:" + uri->getHost());
    }
    Socket::ptr sock = Socket::CreateTCP(addr);   //通过服务端的addr类型，创建一个客户端的sock，
    if(!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL
                    , nullptr, "connect fail:", addr->toString());
    }

    if(!sock->connect(addr)) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL
                , nullptr, "connect fail: " + addr->toString());
    }

    sock->setRevTimeout(timeout_ms);
    HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
    int rt = conn->sendRequest(req);
    if(rt == 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                    , nullptr, "send request closed by peer:" + addr->toString());
    }
    if(rt < 0) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                    , nullptr, "send request socket error errno=:" + std::to_string(errno)
                    + "errstr=" + std::string(strerror(errno)));
    }
    auto rsp = conn->recvResponse();
    if(!rsp) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                    , nullptr, "recv response timeout: " + addr->toString()
                    + "timeout_ms:" + std::to_string(timeout_ms));
    }
    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}



HttpConnectionPool::HttpConnectionPool(const std::string& host
                        ,const std::string& vhost
                        ,uint32_t port
                        ,uint32_t max_size
                        ,uint32_t max_alive_time
                        ,uint32_t max_request) 
            :m_host(host)
            ,m_vhost(vhost)
            ,m_port(port)
            ,m_maxSize(max_size)
            ,m_maxAliveTime(max_alive_time)
            ,m_maxRequest(max_request){
}

HttpConnection::ptr HttpConnectionPool::getConnection() {
    uint64_t now_ms = sylar::GetCurrentMS();
    std::vector<HttpConnection*> invalid_conns;  //用于存放超时/关闭了的connection*,
    HttpConnection* ptr = nullptr;
    MutexType::Lock lock(m_mutex);
    while(!m_conns.empty()) {
        auto conn = *m_conns.begin();
        m_conns.pop_front();
        if(!conn->isConnected()) {
            invalid_conns.push_back(conn);
            continue;
        }
        if((conn->m_createTime + m_maxAliveTime) < now_ms) {
            invalid_conns.push_back(conn);
            continue;
        }
        ptr = conn;
        break;
    }
    lock.unlock();
    for(auto i : invalid_conns) {
        delete i;
    }
    m_total -= invalid_conns.size();

    if(!ptr) {   //如果connectionPool中没有connection了（可能是被其他协程取走了/或着超时了），并且没有超过max_size，就创建新的connection
        IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
        if(!addr) {
            SYLAR_LOG_ERROR(g_logger) << "get addr fail: " << m_host;
            return nullptr;
        }
        addr->setPort(m_port);
        Socket::ptr sock = Socket::CreateTCP(addr);
        if(!sock) {
            SYLAR_LOG_ERROR(g_logger) << "create sock fail: " << *addr;
            return nullptr;
        }
        if(!sock->connect(addr)) {
            SYLAR_LOG_ERROR(g_logger) << "sock connection fail: " << *addr;
            return nullptr;
        }
        ptr = new HttpConnection(sock);
        ++m_total;
    }
    return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::ReleasePtr
                                    , std::placeholders::_1, this));
}

void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool) {   
    ++ptr->m_request;
    if(!ptr->isConnected() 
            || ((ptr->m_createTime + pool->m_maxAliveTime) <= sylar::GetCurrentMS())
            || (ptr->m_request >= pool->m_maxRequest)
            || pool->m_total >= pool->m_maxSize) {
        delete ptr;
        --pool->m_total;
        return;
    }
    MutexType::Lock lock(pool->m_mutex);
    pool->m_conns.push_back(ptr);        //协程用完从pool中取出来的connection后，如果connection没有问题，就重新放回pool中，
}




HttpResult::ptr HttpConnectionPool::doGet(const std::string& url
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers
                                , const std::string& body) {
    return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doGet(Uri::ptr uri
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers
                                , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       <<(uri->getFragment().empty()  ? "" : "#")
       <<uri->getFragment();
    return doGet(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(const std::string& url
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers
                                , const std::string& body) {
    return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(Uri::ptr uri
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers
                                , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       <<(uri->getFragment().empty()  ? "" : "#")
       <<uri->getFragment();
    return doPost(ss.str(), timeout_ms, headers, body);                                
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
                                , const std::string& url
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers
                                , const std::string& body) {
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(url);
    req->setMethod(method);
    req->setClose(false);   //长连接，
    bool has_host = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            if(strcasecmp(i.second.c_str(), "keep-alive") == 0) {
                req->setClose(false);
            }
            continue;
        }
        if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }
        req->setHeader(i.first, i.second);
    }
    if(!has_host) {
        if(m_vhost.empty()){
            req->setHeader("Host", m_host);
        } else {
            req->setHeader("Host", m_vhost);
        }
    }
    req->setBody(body);
    return doRequest(req, timeout_ms);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
                                , Uri::ptr uri
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers
                                , const std::string& body) {
    std::stringstream ss;
    ss << uri->getPath()
       << (uri->getQuery().empty() ? "" : "?")
       << uri->getQuery()
       <<(uri->getFragment().empty()  ? "" : "#")
       <<uri->getFragment();
    return doRequest(method, ss.str(), timeout_ms, headers, body);                                
}


HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req
                                , uint64_t timeout_ms) {
    auto conn = getConnection();   //从connectionPool中拿一个连接出来用，
    if(!conn) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECTION
                    , nullptr, "pool host: " + m_host + " port: " + std::to_string(m_port));
    }
    Socket::ptr sock = conn->getSocket();

    if(!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALID_CONNECTION
                    , nullptr, "pool host: " + m_host + " port: " + std::to_string(m_port));

        sock->setRevTimeout(timeout_ms);
        int rt = conn->sendRequest(req);
        if(rt == 0) {
            return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                        , nullptr, "send request closed by peer:" + sock->getRemoteAddress()->toString());
        }
        if(rt < 0) {
            return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                        , nullptr, "send request socket error errno=:" + std::to_string(errno)
                        + "errstr=" + std::string(strerror(errno)));
        }
        auto rsp = conn->recvResponse();
        if(!rsp) {
            return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                        , nullptr, "recv response timeout: " + addr->toString()
                        + "timeout_ms:" + std::to_string(timeout_ms));
        }
        return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
    }
}




}
}