#ifndef __SYLAR_HTTP_SERVLET_H__
#define __SYLAR_HTTP_SERVLET_H__

#include <memory>
#include <functional>
#include <string>
#include "http.h"
#include "http_session.h"
#include <unordered_map>
#include <vector>
#include "sylar/thread.h"

namespace sylar {
namespace http {


class Servlet {
public:
    typedef std::shared_ptr<Servlet> ptr;
    ~Servlet() {}
    Servlet(std::string name) 
        :m_name(name){
    }
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                            , sylar::http::HttpResponse::ptr response
                            , sylar::http::HttpSession::ptr session) = 0;
    const std::string& GetName() { return m_name;}
private:
    std::string m_name;

};

class NotFoundServlet : public Servlet {
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;
    NotFoundServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                            , sylar::http::HttpResponse::ptr response
                            , sylar::http::HttpSession::ptr session) override;

};


class FunctionServlet : public Servlet {
public:
    typedef std::shared_ptr<FunctionServlet> ptr;
    typedef std::function<int32_t (sylar::http::HttpRequest::ptr request
                            , sylar::http::HttpResponse::ptr response
                            , sylar::http::HttpSession session)> callback;

    FunctionServlet(callback cb);
    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                            , sylar::http::HttpResponse::ptr response
                            , sylar::http::HttpSession::ptr session) override;

private:
    callback m_cb;
};


class ServletDispatch : public Servlet {
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef RWMutex RWMutexType;

    ServletDispatch();

    virtual int32_t handle(sylar::http::HttpRequest::ptr request
                            , sylar::http::HttpResponse::ptr response
                            , sylar::http::HttpSession::ptr session) override;

    void addServelt(const std::string& uri, Servlet::ptr slt);
    void addServelt(const std::string& uri, FunctionServlet::callback cb);
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    Servlet::ptr getServlet(const std::string& uri);
    Servlet::ptr getGlobServlet(const std::string& uri);


    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    Servlet::ptr getDefault() const { return m_default;}
    void setDefault(Servlet::ptr v) { m_default = v;}

    Servlet::ptr getMatchedServelt(const std::string& uri);

private:
    
    RWMutexType m_mutex;
    //uri(sylar/xxx) ->servlet     精准查询
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    //uri(sylar/*) -> servlet 模糊查询
    std::vector<std::pair<std::string, Servlet::ptr> > m_globs;
    //默认servlet， 没有匹配到任何路径时使用，
    Servlet::ptr m_default;

};








}
}





#endif