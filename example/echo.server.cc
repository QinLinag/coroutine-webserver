#include "sylar/tcp_server.h"
#include "sylar/log.h"
#include "sylar/iomanager.h"
#include "sylar/bytearry.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

class EchoServer : public sylar::TcpServer {
public:
    EchoServer(int type);
    void handleClient(sylar::Socket::ptr client);
private:
    int m_type = 0;
};

EchoServer::EchoServer(int type) 
    :m_type(type){
}


void EchoServer::handleClient(sylar::Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "handleClient" << *client;
    sylar::ByteArray::ptr ba(new sylar::ByteArray());
    while(true) {
        ba->clear();    //每次循环处理client一次就会及那个bytearry清空，
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);  //将bytearray中的缓存区初始化，并且赋值给iovs，
        int rt = client->recv(&iovs[0], iovs.size());
        if(rt = 0) {  //关闭，就退出while循环读client，
            SYLAR_LOG_INFO(g_logger) << "client close" << *client;
            break;
        } else if(rt < 0) {
            SYLAR_LOG_INFO(g_logger) << "client error rt=" << rt
                << " errno = " << errno << " strerr= " << strerror(errno);
            break;           
        }
        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);
        if(m_type == 1) {
            std::cout << ba->toString(); // << std::endll;
        } else {
            std::cout << ba->toHexString(); // << std::endll
        }
        std::cout.flush();
    }
}


int type = 1;

void run () {
    SYLAR_LOG_INFO(g_logger) << "server type =" << type;
    EchoServer::ptr es(new EchoServer(type));
    auto addr = sylar::Address::LookupAny("0.0.0.0:8020");
    while(!es->bind(addr)) {
        sleep(2);
    }
    es->start();
}

int main(int argc, char** argv){
    if(argc < 2) {
        SYLAR_LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
        return 0;
    }
    if(!strcmp(argv[1], "-b")) {
        type = 2;
    }

    sylar::IOManager iom(2);
    iom.schedule(run);


    return 0;
}










