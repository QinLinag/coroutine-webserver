#include <unistd.h>
#include "log.h"

#include "thread.h"

sylar::Logger::ptr g_logger= SYLAR_LOG_ROOT();

int count=0;
sylar::RWMutex s_mutex;      //全局的读写互斥量

sylar::Mutex ss_mutex;      //互斥量,

void fun1(){
    SYLAR_LOG_INFO(g_logger) <<"name: " <<sylar::Thread::GetName()
                             <<"this.name: " <<sylar::Thread::GetThis()
                             <<"id: " << sylar::GetThreadId()
                             <<"this.id: " << sylar::Thread::GetThis()->getId();

    for (int  i = 0; i < 50000; i++){
        sylar::RWMutex::WriteLock lock(s_mutex);
        ++count;
    }
    

}



int main(int argc,char** argv){
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    std::vector<sylar::Thread::ptr> thrs;
    for(int i=0;i<5;i++){
        sylar::Thread::ptr thr(new sylar::Thread(&fun1,"name_"+std::to_string(i)));
        thrs.push_back(thr);
    }

    for(int i=0;i<5;i++){
        thrs[i]->join();
    }
    SYLAR_LOG_INFO(g_logger) <<"threaad test end";
    SYLAR_LOG_INFO(g_logger) <<"count =" <<count;
    

    return 0;
}