#ifndef __SYLAR_HTTP_CONNECTION_H__
#define __SYLAR_HTTP_CONNECTION_H__

#include "sylar/socket.h"
#include "http.h"
#include "socket_stream.h"
#include "uri.h"
#include "sylar/thread.h"
#include <list>
#include <atomic>


namespace sylar{
namespace http {



//httpResult这个结构体是用来封装DoGet/DoPost/DoRequest的返回结果的，  里面有一个HttpResponse对象，就是用来携带浏览器返回的结果的，  Error就是定义类很多的错误类型，在DoRequest/DoGet/DoPost这些函数执行过程中，
struct HttpResult {   
    typedef std::shared_ptr<HttpResult> ptr;
    enum class Error{
        OK = 0,
        INVALID_URI = 1,
        INVALID_HOST = 2,
        CONNECT_FAIL = 3,
        SEND_CLOSE_BY_PEER = 4,
        SEND_SOCKET_ERROR = 5,
        TIMEOUT = 6,
        POOL_GET_CONNECTION = 7,
        POOL_INVALID_CONNECTION = 8,
    };

    HttpResult(int _result
                ,HttpResponse::ptr _response
                ,const std::string& _error)
        :result(_result)
        ,response(_response)
        ,error(_error) {}
    int result;
    HttpResponse::ptr response;
    std::string error;
    std::string toString() const;
};



class HttpConnectionPool;
class HttpConnection : public SocketStream {
    
friend class HttpConnectionPool;
public:
    typedef std::shared_ptr<HttpConnection> ptr;


    static HttpResult::ptr DoGet(const std::string& url
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    static HttpResult::ptr DoGet(Uri::ptr uri
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    static HttpResult::ptr DoPost(const std::string& url
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    static HttpResult::ptr DoPost(Uri::ptr uri
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpMethod method
                                , const std::string& url
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpMethod method
                                , Uri::ptr uri
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    static HttpResult::ptr DoRequest(HttpRequest::ptr req     //最后所有的Do方法，都会落到这一个方法来处理，
                                , Uri::ptr uri
                                , uint64_t timeout_ms);


    

    HttpConnection(Socket::ptr sock, bool owner = true);
    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr req);

private:
    uint64_t m_createTime = 0;
    uint64_t m_request = 0;
};





//httpConnectionPool,是存放连接host:port服务端的connection的连接池，里面有多个connection，需要的时候就取一个出来因为如果需要再创建会浪费时间，
class HttpConnectionPool {
public:
    typedef std::shared_ptr<HttpConnectionPool> ptr;
    typedef Mutex MutexType;

    HttpConnectionPool(const std::string& host
                        ,const std::string& vhost
                        ,uint32_t port
                        ,uint32_t max_size
                        ,uint32_t max_alive_time
                        ,uint32_t max_request);

    HttpConnection::ptr getConnection();

    HttpResult::ptr doGet(const std::string& url
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    HttpResult::ptr doGet(Uri::ptr uri
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    HttpResult::ptr doPost(const std::string& url
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    HttpResult::ptr doPost(Uri::ptr uri
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    HttpResult::ptr doRequest(HttpMethod method
                                , const std::string& url
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    HttpResult::ptr doRequest(HttpMethod method
                                , Uri::ptr uri
                                , uint64_t timeout_ms
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    HttpResult::ptr doRequest(HttpRequest::ptr req
                                , uint64_t timeout_ms);
private:
    static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);

private:
    std::string m_host;
    std::string m_vhost;
    uint32_t m_port;
    uint32_t m_maxSize;
    uint32_t m_maxAliveTime;
    uint32_t m_maxRequest;

    MutexType m_mutex;
    std::list<HttpConnection*> m_conns;
    std::atomic<int32_t> m_total = {0};
};



}
}

#endif