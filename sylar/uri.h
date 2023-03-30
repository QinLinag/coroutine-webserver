#ifndef __SYLAR_URI_H__
#define __SYLAR_URI_H__

#include <memory>
#include <string>
#include <stdint.h>
#include "address.h"


namespace sylar {

/*
     foo://user@sylar.com:8042/over/there?name=ferret#nose
       \_/   \______________/\_________/ \_________/ \__/
        |           |            |            |        |
     scheme     authority       path        query   fragment
*/

class Uri {
public:
    typedef std::shared_ptr<Uri> ptr;

    static Uri::ptr Create(const std::string uri);    
    Uri();

    const std::string getScheme() const { return m_scheme;}
    const std::string getUserinfo() const { return m_userinfo;}
    const std::string getHost() const { return m_host;}
    const std::string getPath() const { return m_path;}
    const std::string getQuery() const { return m_query;}
    const std::string getFragment() const { return m_fragment;}
    int32_t getPort() const { return m_port;}

    std::string setScheme(const std::string v) { m_scheme = v;}
    std::string setUserinfo(const std::string v) { m_userinfo = v;}
    std::string setHost(const std::string v) { m_host = v;}
    std::string setPath(const std::string v) { m_path = v;}
    std::string setQuery(const std::string v) { m_query = v;}
    std::string setFragment(const std::string v) { m_fragment = v;}
    int32_t setPort(int32_t v) { m_port = v;}

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;
    Address::ptr createAddress() const;

protected:
    bool isDefaultPort() const;

private:
    std::string m_scheme;
    std::string m_userinfo;
    std::string m_host;
    std::string m_path;
    std::string m_query;
    std::string m_fragment;
    int32_t m_port;
};


}


#endif