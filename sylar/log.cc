#include "log.h"
#include<map>
#include<functional>
#include <time.h>
#include <string.h>

namespace sylar{


const char* LogLevel::toString(LogLevel::Level level){
    switch (level){
#define XX(name) \
    case LogLevel::name:\
        return #name; \
        break;
    
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKNOW";
    }
    return "UNKNOW";
}

LogLevel::Level LogLevel::fromString(const std::string str){

#define XX(level, v) \
    if(str == #v){ \
        return LogLevel::level; \   
    }

    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);

    return LogLevel::UNKNOW;
#undef XX

}


LogEventWrap::LogEventWrap(LogEvent::ptr event)
    :m_event(event){

}

LogEventWrap::~LogEventWrap(){
    m_event->getLogger()->log(m_event->getLevel(),m_event);
}

LogEvent::ptr LogEventWrap::getEvent(){
    return m_event;
}

std::stringstream& LogEventWrap::getSS(){
    return m_event->getSS();
}


 void LogEvent::format(const char* fmt, ...){
    va_list al;
    va_start(al,fmt);
    format(fmt,al);
    va_end(al);
 }

void LogEvent::format(const char* fmt, va_list al){
    char* buf = nullptr;
    int len = vasprintf(&buf,fmt,al);
    if(len!=-1){
        m_ss<<std::string(buf,len);
        free(buf);
    }
}

LogEvent::LogEvent(std::shared_ptr<Logger> logger,LogLevel::Level level,const char* file,int32_t line,uint32_t elapse,uint32_t threadId,uint32_t fiberId,uint64_t time,const std::string& thread_name)
    :m_line(line)
    ,m_file(file)
    ,m_elapse(elapse)
    ,m_threadId(threadId)
    ,m_fiberId(fiberId)
    ,m_time(time)
    ,m_logger(logger)
    ,m_level(level)
    ,m_threadName(thread_name){
}

class MessageFormatItem: public LogFormatter::FormatItem{
public:
    MessageFormatItem(const std::string& str=""){}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override{
        os << event->getContent();
    }
};


class LevelFormatItem: public LogFormatter::FormatItem{
public:
    LevelFormatItem(const std::string& str){}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override{
        os << LogLevel::toString(level);
    }
};

class ElapseFormatItem: public LogFormatter::FormatItem{
public:
    ElapseFormatItem(const std::string& str){}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override{
        os << event->getElapse();
    }
};


class NameFormatItem: public LogFormatter::FormatItem{
public:
    NameFormatItem(const std::string& str){}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override{
        os << event->getLogger()->getName();
    }
};


class ThreadNameFormatItem: public LogFormatter::FormatItem{
public:
    ThreadNameFormatItem(const std::string& str){}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override{
        os << event->getThreadName();
    }
};

class ThreadIdFormatItem: public LogFormatter::FormatItem{
public:
    ThreadIdFormatItem(const std::string& str){}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override{
        os << event->getThread();
    }
};

class FiberIdFormatItem: public LogFormatter::FormatItem{
public:
    FiberIdFormatItem(const std::string& str){}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override{
        os << event->getFiberId();
    }
};


class DateTimeFormatItem: public LogFormatter::FormatItem{
public:
    DateTimeFormatItem(const std::string& format="%Y-%m-%d %H:%M:%s")
        :m_format(format){
        if(m_format.empty()){
            m_format= "%Y-%m-%d %H:%M:%s";
        }
    }

    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override{
        struct tm tm;
        time_t time =event->getTIme();
        localtime_r(&time,&tm);
        char buf[64];
        strftime(buf,sizeof(buf),m_format.c_str(),&tm);
        os << buf;
    }

private:
    std::string m_format;
};

class FilenameFormatItem: public LogFormatter::FormatItem{
public:
    FilenameFormatItem(const std::string& str){}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override{
        os << event->getFile();
    }
};


class LineFormatItem: public LogFormatter::FormatItem{
public:
    LineFormatItem(const std::string& str){}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override{
        os << event->getLine();
    }
};


class NewLineFormatItem: public LogFormatter::FormatItem{
public:
    NewLineFormatItem(const std::string& str){}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override{
        os << std::endl;
    }
};

class StringFormatItem: public LogFormatter::FormatItem{
public:
    StringFormatItem(const std::string& str)
        :m_string(str){

        }

    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override{
        os << m_string;
    }
private:
    std::string m_string;
};

class TabFormatItem: public LogFormatter::FormatItem{
public:
    TabFormatItem(const std::string& str="") {}

    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override{
        os << "\t";
    }
};




//初始化logger时，默认给一个formatter
Logger::Logger(const std::string& name) 
    :m_name(name)
    ,m_level(LogLevel::DEBUG) {
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));

    if(name == "root"){
        this->addAppender(LogAppender::ptr(new StdoutLogAppender));
    }

}

void Logger::setFormatter(LogFormatter::ptr val){
    sylar::Mutex::Lock lock(m_mutex);
    m_formatter=val;
    for(auto& i : m_appenders){
        sylar::Mutex::Lock ll(m_mutex);
        if(!i->m_hasFormatter){
            i->m_formatter=m_formatter;
        }
    }
}

void Logger::setFormatter(const std::string& val){
    LogFormatter::ptr new_val(new LogFormatter(val));
    if(new_val->isError()){
        std::cout << "Logger setFormatter name=" <<m_name
                  << "formatter pattern value = " << val <<" invalid formatter"
                  << std::endl;
       return; 
    }
    sylar::Mutex::Lock lock(m_mutex);
    m_formatter = new_val;
    for(auto& i : m_appenders){
        sylar::Mutex::Lock ll(m_mutex);
        if(!i->m_hasFormatter){
            i->m_formatter=m_formatter;
        }
    }
}

std::string Logger::toYamlString(){
    Mutex::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    if(m_level != LogLevel::UNKNOW){
        node["level"] = LogLevel::toString(m_level);
    }
    if(m_formatter){
        node["formatter"] = m_formatter->getPattern();
    }

    for(auto& i : m_appenders){
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}


LogFormatter::ptr Logger::getFormatter(){
    Mutex::Lock lock(m_mutex);
    return m_formatter;
}



void Logger::log(LogLevel::Level level,LogEvent::ptr event){
    if(level>=m_level){
        auto self=shared_from_this();  //将自己封装为智能指针，
        Mutex::Lock lock(m_mutex);
        if(!m_appenders.empty()){
            for(auto &i:m_appenders){
                i->log(self,level,event);
            }
        }else if(m_root){
            m_root->log(level,event);
        }
        
    }
}

//这里很关键，在添加appender到logger里面时，如果appender没有设置formatter，就将logger的给appender
void Logger::addAppender(LogAppender::ptr appender){
    Mutex::Lock lock(m_mutex);
    if(!appender->getFormatter()){  
        Mutex::Lock ll(appender->m_mutex);
        appender->m_formatter = m_formatter;
    }
    m_appenders.push_back(appender);
}

void Logger::deleteAppender(LogAppender::ptr appender){
    Mutex::Lock lock(m_mutex);
     for(auto it =m_appenders.begin();it!=m_appenders.end();it++){
        if (*it==appender){
            m_appenders.erase(it);
            break;
        }
        
    }
}

void Logger::clearAppenders(){
    Mutex::Lock lock(m_mutex);
    m_appenders.clear();
}


void Logger::debug(LogEvent::ptr event){
    log(LogLevel::DEBUG, event);
}
void Logger::info(LogEvent::ptr event){
    log(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event){
    log(LogLevel::WARN, event);
}
void Logger::error(LogEvent::ptr event){
    log(LogLevel::ERROR, event);
}
void Logger::fatal(LogEvent::ptr event){
    log(LogLevel::FATAL, event);
}

void LogAppender::setFormatter(LogFormatter::ptr val){
    sylar::Mutex::Lock lock(m_mutex);      //加锁，
    m_formatter =val;
    if(m_formatter){
        m_hasFormatter=true;
    }else{
        m_hasFormatter = false;
    }
}
LogFormatter::ptr LogAppender::getFormatter(){
    sylar::Mutex::Lock lock(m_mutex);             //加锁
    return m_formatter;
}



FileLogAppender::FileLogAppender(const std::string filename)
                    :m_filename(filename){

}



//输出到文件中，
void FileLogAppender::log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event){
    if (m_level<=level){
        uint64_t now = time(0);
        if(now != m_lastTime){          //每秒钟打开一次文件，以防把日志文件已经被删了， 没有指定的文件，
            reopen();
            m_lastTime = now;
        }

        Mutex::Lock lock(m_mutex);
        m_filestream << m_formatter->format(logger,level,event);         
    }
}

std::string FileLogAppender::toYamlString(){
    Mutex::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if(m_level != LogLevel::UNKNOW){
        node["level"] = LogLevel::toString(m_level);
    }
    if(m_hasFormatter && m_formatter){
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

bool FileLogAppender::reopen(){
    Mutex::Lock lock(m_mutex);
    if (m_filestream){
        m_filestream.close();
    }
    m_filestream.open(m_filename);
    return !!m_filestream;
}

//输出到控制台
void StdoutLogAppender::log(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event){
    if(level>=m_level){
        Mutex::Lock lock(m_mutex);
        std::cout<<m_formatter->format(logger,level,event);         
    }
}

std::string StdoutLogAppender::toYamlString(){
    Mutex::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if(m_level != LogLevel::UNKNOW){
        node["level"] = LogLevel::toString(m_level);
    }
    if(m_hasFormatter && m_formatter){
        node["formatter"] = m_formatter->getPattern();
    }

    std::stringstream ss;
    ss << node;
    return ss.str();
}



LogFormatter::LogFormatter(const std::string& pattern)
                :m_pattern(pattern){
                init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event){
    std::stringstream ss;    //所有日志数据都保存这里面了，
    for(auto& i:m_items){    //有不同的formattItem，比如时间的，线程的，协程的，那个文件的等，这些数据都会按照制定的formatter输入到ss流中，
        i->format(ss,logger,level,event);
    }
    return ss.str();
}

//%xxx %xxx{xxx} %%               init函数是最关键的东西，用来解析formatter格式的
void LogFormatter::init(){
    //str,format,type
    std::vector<std::tuple<std::string,std::string,int> > vec;  //tuple随意什么类型，随意多少个
    std::string nstr;
    for (size_t i = 0; i < m_pattern.size(); i++){
        if(m_pattern[i]!='%'){
            nstr.append(1,m_pattern[i]);
            continue;
        }

        if((i + 1) < m_pattern.size()){
            if(m_pattern[i + 1]=='%') {   //连续两个%符号，
                nstr.append(1,'%');
                continue;
            }
        }

        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while(n < m_pattern.size()){
            if(!isalpha(m_pattern[n]) && m_pattern[n]!='{' && m_pattern[n]!='}'){
                break;
            }
            if(fmt_status == 0) {
                if(m_pattern[n]=='{'){
                    str = m_pattern.substr(i+1,n-i-1); //从i+1的位置开始截取n-i-1个字符
                    fmt_status =1;
                    fmt_begin =n;
                    ++n;
                    continue;
                }
            }
            if(fmt_status==1){
                if(m_pattern[n]=='}'){
                    fmt =m_pattern.substr(fmt_begin + 1,n-fmt_begin-1);
                    fmt_status=2;
                    break;
                }
            }
            ++n;
        }
    
        if(fmt_status==0){
            if(!nstr.empty()){
                vec.push_back(std::make_tuple(nstr,std::string{},0));
                nstr.clear();
            }
            str=m_pattern.substr(i+1,n-i-1);
            vec.push_back(std::make_tuple(str,fmt,1));
            i=n -1;
        }else if(fmt_status ==1){
            std::cout<<"pattern parse error:"<<m_pattern<<" - "<<m_pattern.substr(i)<<std::endl;
            vec.push_back(std::make_tuple("<<pattern_error>>",fmt,0));
            m_error = true;
        }else if(fmt_status==2){
            if(!nstr.empty()){
                vec.push_back(std::make_tuple(nstr,"",0));
            }
            vec.push_back(std::make_tuple(str,fmt,1));
            i=n -1 ;
        }
    }

    if(!nstr.empty()){
        vec.push_back(std::make_tuple(nstr,"",0));
    }


//定义一个map，里面通过string作为key，value存放函数指针
    static std::map<std::string,std::function<LogFormatter::FormatItem::ptr(const std::string& str)> > s_format_items = {
#define XX(str,C) \
        {#str, [](const std::string& fmt) {return LogFormatter::FormatItem::ptr(new C(fmt));}}   //lamda 

        XX(m,MessageFormatItem), //m:消息
        XX(p,LevelFormatItem),   //p:日志级别
        XX(r,ElapseFormatItem),  //r:积累毫秒数
        XX(c,NameFormatItem),   //c：日志名称
        XX(t,ThreadIdFormatItem),   //t：线程id
        XX(n,NewLineFormatItem),    //n：换行
        XX(d,DateTimeFormatItem),   //d:时间
        XX(f,FilenameFormatItem),   //f：文件名
        XX(l,LineFormatItem),       //l:行号
        XX(T,TabFormatItem),        //t：tab分割符
        XX(F,FiberIdFormatItem),    //F：协程号
        XX(N,ThreadNameFormatItem)  //N:线程名称
#undef XX
    };

    for(auto& i: vec){
        if(std::get<2>(i)==0){
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        }else{
            auto it=s_format_items.find(std::get<0>(i));  
            if(it==s_format_items.end()){
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %"+std::get<0>(i)+">>")));
                m_error = true;
            }else{
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
        //测试代码，
        std::cout<<"(" <<std::get<0>(i) <<")" - "(" <<std::get<1>(i) <<")" - "(" <<std::get<2>(i) << ")" <<std::endl;
    }
}



LoggerManager::LoggerManager(){
    m_root.reset(new Logger());
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender()));  //默认的loggeroot是stdout输出到屏幕的，
    init();
}



Logger::ptr LoggerManager::getLogger(const std::string& name){
    Mutex::Lock lock(m_mutex);
    auto it =m_loggers.find(name);
    if(it != m_loggers.end()){
        return it->second;
    }
    //如果没有这个logger，创建一个
    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_loggers[name] = logger;
    return logger;
}

struct LogAppenderDefine {
    int type = 0; //1-file  2-stdout
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formattter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const{
        return type == oth.type
            && level == oth.level
            && formattter == oth.formattter
            && file == oth.file;
    }
};

struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name
            && level == oth.level
            && formatter == oth.formatter
            && appenders == oth.appenders;
    }

    bool operator<(const LogDefine& oth) const {   //set集合需要，
        return name < oth.name;
    }
};


//set<LogDefine>的lexical的片特化，
template<>
class LexicalCast<std::string, std::set<LogDefine> > {
public:
    std::set<LogDefine> operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        std::set<LogDefine> vec;
        for(size_t i = 0;i < node.size(); i++){
            auto n = node[i];
            if(!n["name"].IsDefined()){
                std::cout << "log config error : name is null" << n <<std::endl;
                continue;
            }

            LogDefine ld;
            ld.name = n["name"].as<std::string>();
            ld.level = LogLevel::fromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
            if(n["formatter"].IsDefined()){
                ld.formatter = n["formatter"].as<std::string>();
            }

            if(n["appender"].IsDefined()){
                for(size_t x = 0; x < n["appender"].size(); x++){
                    auto a = n["appender"][x];
                    if(!a["type"].IsDefined()){
                        std::cout << "log config error: appender type is null" <<a <<std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();
                    LogAppenderDefine lad;
                    if(type == "FileLogAppender"){
                        lad.type = 1;
                        if(!a["file"].IsDefined()){
                            std::cout << "log config error : fileappender file path is null" << a <<std::endl;
                            continue;
                        }
                        lad.file = a["file"].as<std::string>();
                        if(a["formatter"].IsDefined()){
                            lad.formattter = a["formatter"].as<std::string>();
                        }
                    }else if(type == "StdoutLogAppender"){
                            lad.type = 2;
                    }else {
                        std::cout << "log config error : appender type is invalid " << a << std::endl;
                        continue;
                    }
                    ld.appenders.push_back(lad);
                }
            }
            vec.insert(ld);
        }
        return vec;
    }
};


template<>
class LexicalCast<std::set<LogDefine>, std::string> {
public:
    std::string operator()(const std::set<LogDefine>& v){
        YAML::Node node;
        for(auto& i : v){
            YAML::Node n;
            n["name"] = i.name;
            n["level"] = LogLevel::toString(i.level);
            if(!i.formatter.empty()){
                n["formatter"] = i.formatter;
            }

            for(auto& a : i.appenders){
                YAML::Node na;
                if(a.type == 1){
                    na["type"] = "FileLogAppender";
                    na["file"] = a.file;
                }else if(a.type == 2) {
                    na["type"] = "StdoutLogAppender";
                }
                if(a.level != LogLevel::UNKNOW){
                    na["level"] = LogLevel::toString(a.level);
                }

                if(!a.formattter.empty()){
                    na["formatter"] = a.formattter;
                }

                n["appenders"].push_back(na);
            }

            node.push_back(n);
        }
    }

};





sylar::ConfigVar<std::set<LogDefine> >::ptr g_log_defines =
    sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");


struct LogIniter {
    LogIniter() {
        g_log_defines->addListener([](const std::set<LogDefine>& old_value,
                        const std::set<LogDefine>& new_value){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_logger_conf_changed";
            for(auto& i : new_value){
                Logger::ptr logger;
                auto it = old_value.find(i);
                if(it == old_value.end()){
                    //新增logger
                    logger= SYLAR_LOG_NAME(i.name); //没有找到就会创建一个放到logmanager中
                } else {
                    if(!(i == *it)){
                        //修改logger
                        logger = SYLAR_LOG_NAME(i.name);
                    }else {
                        continue;
                    }
                }
                logger->setLevel(i.level);
                if(!i.formatter.empty()){
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppenders();
                for(auto& a : i.appenders){
                    LogAppender::ptr ap;
                    if(a.type == 1){
                          ap.reset(new FileLogAppender(a.file));
                     }else if(a.type == 2){
                        ap.reset(new StdoutLogAppender());
                     }
                    ap->setLevel(a.level);
                    if(!a.formattter.empty()){
                        LogFormatter::ptr fmt(new LogFormatter(a.formattter));
                        if(!fmt->isError()){
                            ap->setFormatter(fmt);
                        } else {
                            std::cout<< "appender name= " << a.type
                                     << " formatter = "   << a.formattter << "is error" <<std::endl;
                        }
                    }
                     logger->addAppender(ap);
                    }
            }

            for(auto& i : old_value){
                auto it = new_value.find(i);
                if(it == new_value.end()){
                    //删除logger          软删除
                    auto logger = SYLAR_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)100);
                    logger->clearAppenders();
                }
            }

        } );
    }
};


//这里很关键，  静态全局变量先初始化，农民LogIniter的构造函数就会执行了，就会添加一个监听函数，
//当从配置文件读取到log配置修改configar，就会触发这个监听函数从而生成logger放到loggermanager里面，
static LogIniter __log_init;

std::string LoggerManager::toYamlString(){
    Mutex::Lock lock(m_mutex);
    YAML::Node node;
    for(auto& i : m_loggers){
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void LoggerManager::init(){
    
}




};





















