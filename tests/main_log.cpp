#include<iostream>
#include "log.h"
#include "util.h"

int main(int argc, char const **argv){
    sylar::Logger::ptr logger(new sylar::Logger());

    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender()));  //这里的appender没有主动设置formatter
                                                                                //addAppender函数里面会将loggerformatter给appender，
    sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("./log.txt"));
    sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(sylar::LogLevel::ERROR);
    logger->addAppender(file_appender); 


    //sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__,__LINE__,0,sylar::GetThreadId(),sylar::GetFiberId(),time(0)));
    //event->getSS()<<"hello sylar log";
    //logger->log(sylar::LogLevel::DEBUG,event);

    std::cout<<"hello sylar log"<<std::endl;

    SYLAR_LOG_INFO(logger) << "test macro";
    SYLAR_LOG_ERROR(logger) << "test macro error";


    SYLAR_LOG_FMT_ERROR(logger,"test macro fmt error %s","aa");


                         //singleton 
    auto l=sylar::LoggerMgr::GetInstance()->getLogger("xx");
    SYLAR_LOG_INFO(l) << "xxx";


    return 0;
}
