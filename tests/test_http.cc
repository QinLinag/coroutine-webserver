#include "sylar/http/http.h"
#include "sylar/log.h"


void test_request() {
    sylar::http::HttpRequest::ptr req(new sylar::http::HttpRequest());
    req->setHeader("host", "www.sylar.top");
    req->setBody("hello world");

    req->dump(std::cout) << std::endl;
}

void test_response() {
    sylar::http::HttpResponse::ptr rsp(new sylar::http::HttpResponse());
    rsp->setHeader("x-x", "sylar");
    rsp->setBody("hello sylar");
    rsp->setStatus((sylar::http::HttpStatus)400);
    rsp->setClose(false);
    rsp->dump(std::cout) << std::endl;   
}

int main(int argc, char** argv) {


    test_request();
    return 0;
}