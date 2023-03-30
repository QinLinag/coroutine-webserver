#include "sylar/address.h"
#include "sylar/log.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test() {
    std::vector<sylar::Address::ptr> addrs;

    bool v = sylar::Address::Lookup(addrs, "www.baidu.com");
    if(!v) {
        SYLAR_LOG_ERROR(g_logger) << "lookup fail";
        return;
    } 

    for(size_t i = 0; i < addrs.size(); ++i) {
        SYLAR_LOG_INFO(g_logger) << i << "-" << addrs[i]->toString();
    }
}


void iface() {
    std::multimap<std::string, std::pair<sylar::Address::ptr, uint32_t> > results;
    bool v = sylar::Address::GetInterfaceAddresses(results);
    if(!v) {
        SYLAR_LOG_ERROR(g_logger) << "GetINterfaceAddresses fail";
        return;
    }
}


int main(int argc, char** argv) {

    test();

    return 0;
}