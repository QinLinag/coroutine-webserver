#include "http_servlet.h"
#include <fnmatch.h>

namespace sylar {
namespace http {

FunctionServlet::FunctionServlet(callback cb) 
    :Servlet("FunctionServelt")
    ,m_cb(cb){
}

NotFoundServlet::NotFoundServlet() 
    :Servlet("NotFoundServlet"){
}

int32_t NotFoundServlet::handle(sylar::http::HttpRequest::ptr request
                        , sylar::http::HttpResponse::ptr response
                        , sylar::http::HttpSession::ptr session) {
    static const std::string& RSP_BODY = "<html......";  //随便找一个404的response
    response->setStatus(sylar::http::HttpStatus::NOT_FOUND);
    response->setHeader("Servlet", "sylar/1.0.0");
    response->setHeader("Content-Type", "text/html");
    response->setBody(RSP_BODY);
    return 0;
}


int32_t FunctionServlet::handle(sylar::http::HttpRequest::ptr request
                        , sylar::http::HttpResponse::ptr response
                        , sylar::http::HttpSession::ptr session) {
    return m_cb(request, response, session);
}

ServletDispatch::ServletDispatch() 
    :Servlet("ServletDispatch"){
    m_default.reset(new NotFoundServlet());
}


int32_t ServletDispatch::handle(sylar::http::HttpRequest::ptr request
                            , sylar::http::HttpResponse::ptr response
                            , sylar::http::HttpSession::ptr session) {
    auto slt = getMatchedServelt(request->getPath());
    if(slt) {
        slt->handle(request, response, session);
    }
    return 0;
}

void ServletDispatch::addServelt(const std::string& uri
                    , Servlet::ptr slt) {
    RWMutexType::WriteLock lock(m_mutex);
    m_datas[uri] = slt;
}
void ServletDispatch::addServelt(const std::string& uri
                    , FunctionServlet::callback cb) {
    RWMutexType::WriteLock lock(m_mutex);
    m_datas[uri].reset(new FunctionServlet(cb));
}
void ServletDispatch::addGlobServlet(const std::string& uri
                    , Servlet::ptr slt) {
    RWMutexType::WriteLock lock(m_mutex);
    for(auto it = m_globs.begin();
            it != m_globs.end(); ++it) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back(std::make_pair(uri, slt));
}
void ServletDispatch::addGlobServlet(const std::string& uri
                    , FunctionServlet::callback cb) {
    return addGlobServlet(uri, FunctionServlet::ptr(new FunctionServlet(cb)));
}

void ServletDispatch::delServlet(const std::string& uri) {
    RWMutexTypeType::WriteLock lock(m_mutex);
    m_datas.erase(uri);
}

void ServletDispatch::delGlobServlet(const std::string& uri) {
    RWMutexType::WriteLock lock(m_mutex);
    for(auto it = m_globs.begin();
            it != m_globs.end(); ++it) {
        if(it->first == uri) {
            m_globs.erase(it);
            break;
        }
    }
}   

Servlet::ptr ServletDispatch::getServlet(const std::string& uri) {
    RWMutexType::WriteLock lock(m_mutex);
    auto it = m_datas.find(uri);
    return it == m_datas.end() ? nullptr : it->second;
}

Servlet::ptr ServletDispatch::getGlobServlet(const std::string& uri) {
    RWMutexType::WriteLock lock(m_mutex);
    for(auto it = m_globs.begin();
            it != m_globs.end(); ++it) {
        if(!fnmatch(it->first.c_str(), uri.c_str(), 0)) {
            return it->second;
        }
    }
    return nullptr;
}





Servlet::ptr ServletDispatch::getMatchedServelt(const std::string& uri) {
    RWMutexType::WriteLock lock(m_mutex);
    auto mit = m_datas.find(uri);
    if(mit != m_datas.end()) {
        return mit->second;
    }
    for(auto it = m_globs.begin();
            it != m_globs.end(); ++it) {
        if(!fnmatch(it->first ,uri.c_str(), 0)) {
            return it->second;
        }
    }
    return m_default;
}

}   
}