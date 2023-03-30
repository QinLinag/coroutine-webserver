#ifndef __SYLAR_LOG_H
#define __SYLAR_LOG_H

#include<string>
#include<stdint.h>
#include<memory>
#include<list>
#include<sstream>
#include<fstream>
#include<iostream>
#include<vector>
#include "util.h"
#include<stdarg.h>
#include<map>
#include "singleton.h"
#include "thread.h"
#include "config.h"


#define SYLAR_LOG_LEVEL(logger,level) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger,level,__FILE__,__LINE__,0,sylar::GetThreadId(), \
                    sylar::GetFiberId(),time(0),sylar::Thread::GetName()))).getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger,sylar::LogLevel::FATAL)



#define SYLAR_LOG_FMT_LEVEL(logger,level,fmt,...) \
    if(logger->getLevel()<=level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger,level,__FILE__,__LINE__,0, \
            sylar::GetThreadId(),sylar::GetFiberId(),time(0), sylar::Thread::GetName()))).getEvent()->format(fmt,__VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger,fmt,...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::DEBUG,fmt,__VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger,fmt,...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::INFO,fmt,__VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger,fmt,...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::WARN,fmt,__VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger,fmt,...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::ERROR,fmt,__VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger,fmt,...) SYLAR_LOG_FMT_LEVEL(logger,sylar::LogLevel::FATAL,fmt,__VA_ARGS__)


#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name);

namespace sylar{

class Logger;
class LoggerManager;

class LogLevel{
public:
    enum Level{
        UNKNOW=0,
        DEBUG=1,
        INFO=2,
        WARN=3,
        ERROR=4,
        FATAL=5
    };
    static const char* toString(LogLevel::Level level);
    static LogLevel::Level fromString(const std::string str);
};

class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger
            ,LogLevel::Level level
            ,const char* file
            ,int32_t line
            ,uint32_t elapse
            ,uint32_t threadId
            ,uint32_t fiberId,uint64_t time
            ,const std::string& thread_name);

    const char* getFile(){return m_file;}
    int32_t getLine(){return m_line;}
    uint32_t getThread(){return m_threadId;}
    uint32_t getElapse(){return m_elapse;}
    uint32_t getFiberId(){return m_elapse;}
    uint32_t getTIme(){return m_time;}
    std::string getContent(){return m_ss.str();}
    const std::string getThreadName() {return m_threadName;}
    std::shared_ptr<Logger> getLogger(){return m_logger;}
    LogLevel::Level getLevel(){return m_level;}

    std::stringstream& getSS() {return m_ss;}
    
    void format(const char* fmt, ...);
    void format(const char* fmt, va_list al);
    

 private:
    const char* m_file = nullptr;
    int32_t m_line= 0;
    uint32_t m_threadId= 0;
    uint32_t m_elapse= 0;
    uint32_t m_fiberId =0;
    uint64_t m_time;
    std::string m_threadName;
    std::stringstream m_ss;

    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
};

class LogEventWrap {
public:
    LogEventWrap(LogEvent::ptr event);
    //析构的时候调用了 event对象里面的logger的log函数进行日至
    ~LogEventWrap();
    std::stringstream& getSS();
    LogEvent::ptr getEvent();
private:
    LogEvent::ptr m_event;
};

class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);

    std::string format(std::shared_ptr<Logger> Logger,LogLevel::Level level,LogEvent::ptr event);

    std::string getPattern(){ return m_pattern;}
public:
    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem() {}
        FormatItem(const std::string& fmt=""){};
        virtual void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event)=0;
    };

    bool isError() {return m_error;}

    void init();
private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;
    bool m_error;
};



class Logger: public std::enable_shared_from_this<Logger>{
friend class LoggerManager;   
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger(const std::string& name="root");
    void log(LogLevel::Level level,LogEvent::ptr event);


    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void error(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void deleteAppender(LogAppender::ptr appender);
    void clearAppenders();
    LogLevel::Level getLevel() const{return this->m_level;}
    void setLevel(LogLevel::Level val){this->m_level=val;}

    const std::string& getName() const {return m_name;}

    void setFormatter(LogFormatter::ptr val);
    void setFormatter(const std::string& val);
    LogFormatter::ptr getFormatter();

    std::string toYamlString();

private:
    std::string m_name;
    LogLevel::Level m_level;
    sylar::Mutex m_mutex;
    std::list<LogAppender::ptr> m_appenders;
    LogFormatter::ptr m_formatter;
    Logger::ptr m_root;
};

class LogAppender{
friend class Logger;
public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender();

    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();                   //复杂的类型需要加锁，

    virtual void log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event);

    LogLevel::Level getLevel()const {return m_level;}          //基本类型，就不需要加锁，
    void setLevel(LogLevel::Level level){m_level=level;}
    virtual std::string toYamlString();
protected:
    LogLevel::Level m_level =LogLevel::DEBUG;
    bool m_hasFormatter = false;
    LogFormatter::ptr m_formatter;
    sylar::Mutex m_mutex;
};

class StdoutLogAppender:public LogAppender{
friend class Logger;
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override;
    std::string toYamlString() override;
};

class FileLogAppender:public LogAppender{
friend class Logger;
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string filename);
    void log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override;
    bool reopen();
    std::string toYamlString() override;
private:
    std::string m_filename;     //日志输出到的文件名
    std::ofstream m_filestream;
    uint64_t m_lastTime = 0;
};




//logger管理器
class LoggerManager{
public:
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);

    void init();
    Logger::ptr getRoot(){return m_root;}
    std::string toYamlString();
private:
    Mutex m_mutex;
    std::map<std::string,Logger::ptr> m_loggers;
    Logger::ptr m_root;
};


typedef sylar::Singleton<LoggerManager> LoggerMgr;



};




#endif