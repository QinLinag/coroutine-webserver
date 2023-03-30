#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__


#include<memory>
#include<string>
#include<sstream>
#include<boost/lexical_cast.hpp>
#include<map>
#include<yaml-cpp/yaml.h>
#include<vector>
#include<list>
#include<map>
#include<unordered_map>
#include<set>
#include<unordered_set>
#include<functional>

#include "log.h"
#include "thread.h"




namespace sylar{
    
//F from_type, T to_type
template<class F, class T>
class LexicalCast{
public:
    T operator() (const F& v){
        return boost::lexical_cast<T>(v);        //运算符()  重载
    }
};

template<class T>               //模板偏例化
class LexicalCast<std::string, std::vector<T> >{
public:
    std::vector<T> operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for(size_t i =0;i < node.size(); i++){
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>() (ss.str()));   //递归调用，如果T类型还是复杂类型的化，
        }
        return vec;
    }
};


template<class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator() (const std::vector<T>& v){
        YAML::Node node;
        for(auto& i : v){
            node.push_back(YAML::Load(LexicalCast<T, std::string>() (i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};



template<class T>               //模板偏例化
class LexicalCast<std::string, std::list<T> >{
public:
    std::list<T> operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for(size_t i =0;i < node.size(); i++){
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>() (ss.str()));   //递归调用，如果T类型还是复杂类型的化，
        }
        return vec;
    }
};


template<class T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator() (const std::list<T>& v){
        YAML::Node node;
        for(auto& i : v){
            node.push_back(YAML::Load(LexicalCast<T, std::string>() (i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


template<class T>               //模板偏例化
class LexicalCast<std::string, std::set<T> >{
public:
    std::set<T> operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for(size_t i =0;i < node.size(); i++){
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>() (ss.str()));   //递归调用，如果T类型还是复杂类型的化，
        }
        return vec;
    }
};


template<class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator() (const std::set<T>& v){
        YAML::Node node;
        for(auto& i : v){
            node.push_back(YAML::Load(LexicalCast<T, std::string>() (i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


template<class T>               //模板偏例化
class LexicalCast<std::string, std::unordered_set<T> >{
public:
    std::unordered_set<T> operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for(size_t i =0;i < node.size(); i++){
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>() (ss.str()));   //递归调用，如果T类型还是复杂类型的化，
        }
        return vec;
    }
};


template<class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator() (const std::unordered_set<T>& v){
        YAML::Node node;
        for(auto& i : v){
            node.push_back(YAML::Load(LexicalCast<T, std::string>() (i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


template<class T>               //模板偏例化
class LexicalCast<std::string, std::map<std::string, T> >{
public:
    std::map<std::string ,T> operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string ,T> vec;
        std::stringstream ss;
        for(auto it =node.begin();it!=node.end();it++){
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),LexicalCast<std::string,T>() (ss.str())));
        }
        return vec;
    }
};


template<class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator() (const std::map<std::string ,T>& v){
        YAML::Node node;
        for(auto& i : v){
            node[i.first] = YAML::Load(LexicalCast<T, std::string>() (i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>               //模板偏例化
class LexicalCast<std::string, std::unordered_map<std::string, T> >{
public:
    std::unordered_map<std::string ,T> operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string ,T> vec;
        std::stringstream ss;
        for(auto it =node.begin();it!=node.end();it++){
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(),LexicalCast<std::string,T>() (ss.str())));
        }
        return vec;
    }
};


template<class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator() (const std::unordered_map<std::string ,T>& v){
        YAML::Node node;
        for(auto& i : v){
            node[i.first] = YAML::Load(LexicalCast<T, std::string>() (i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


class ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name,const std::string& description="")
        :m_name(name)
        ,m_description(description){
        //转为小写
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }

    virtual ~ConfigVarBase() {}

    const std::string getName() const{return m_name;}
    const std::string getDescription() const{return m_description;}

    virtual std::string toString() = 0;
    virtual bool fromString(const std::string& val) = 0;
    virtual std::string getTypeName() const =0;
private:
    std::string m_name;
    std::string m_description;
};

//FromStr T operator() (const std::string&)
//ToStr std::string operator() (const T&)
template<class T, class FromStr = LexicalCast<std::string,T>
                , class ToStr = LexicalCast<T, std::string> >             //特例化
class ConfigVar: public ConfigVarBase{
public:
    typedef RWMutex RWMutexType;
    typedef std::shared_ptr<ConfigVar<T>> ptr;
    typedef std::function<void (const T& old_value, const T& new_value)> on_change_cb;

    ConfigVar(const std::string& name
            ,const T& default_value     //读取到的值，
            ,const std::string& description="")
            :ConfigVarBase(name,description)
            ,m_val(default_value){
        
    }

    std::string toString() override{
        try{
            //return boost::lexical_cast<std::string>(m_val);
            RWMutexType::Readlock lock(m_mutex);
            return ToStr() (m_val);
        }catch(std::exception e){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) <<"ConfigVar::toString exception"
                << e.what() <<"convert: " <<typeid(m_val).name() << "to string";
        }
        return "";
    }
    bool fromString(const std::string& val) override{
        try{
            //m_val=boost::lexical_cast<T>(val);
            SetValue(FromStr() (val));
        }catch(std::exception e){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) <<"ConfigVar::toString exception " \
                <<e.what() <<"convert: string to "  <<typeid(T).name();
        }
        return false;
    }

    const T getValue()  {
        RWMutexType::Readlock lock(m_mutex);
        return m_val;
    }

    void setValue(const T& v) {
        {
            RWMutexType::Readlock lock(m_mutex);
            if(v == m_val){
                return ;
            }
            for(auto& fun : m_cbs){
                fun.second(m_val,v);
            }
        }
        RWMutex::WriteLock lock(m_mutex);
        m_val = v;
    }


    std::string getTypeName()const {return typeid(T).name();}  //临时添加的派生类特有的方法，如果基类指针指向派生类想要访问派生类特有方法，基类必须要虚实现

    uint64_t addListener(const on_change_cb cb){
        static uint64_t s_fun_id = 0;
        RWMutexType::WriteLock lock(m_mutex);
        ++s_fun_id;
        m_cbs[key] = cb;
        return s_fun_id;
    }
     
    void delListener(const uint64_t key){
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }

    on_change_cb getListener(uint64_t key){
        RWMutexType::Readlock lock(m_mutex);
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it;
    }

    void clearListener(){
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
    }

private:
    RWMutexType m_mutex;
    T m_val;
    //变更回调函数组，，  uint64_t,要求唯一，一般可以用hash
    std::map<uint64_t, on_change_cb> m_cbs;
};


class Config{

public:
    typedef std::map<std::string,ConfigVarBase::ptr> ConfigVarMap;
    typedef RWMutex RWMutexType;
    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name
            ,const T& default_value
            ,const std::string& description= ""){
        RWMutexType::Readlock lock(GetMutex());
        auto it =GetDatas().find(name);
        if(it != GetDatas().end()){    //说明存在，
            auto temp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second()); //转换为智能指针，
            if(temp){          //转换成功，说明，原来的该name对于的configar类型，和现在的T类型相同，
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) <<"Lookup nname=" <<name << "exists";
                return temp;
            }else{             //不同，
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) <<"Lookup name=" <<name <<"exists but type is error,you can change the name"
                    << typeid(T).name() <<" real_type=" << it->second->getTypeNamer()
                        << " " << it->second->toString();
 
            }
        }

        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._012345678")
                ! = std::string::npos){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) <<"Lookup name invalid " <<name;
            throw std::invalid_argument(name);
        }              


        //如果原来没有在个配置，就新创建一个
        typename ConfigVar<T>::ptr v(new ConfigVar(name,default_value,description));
        GetDatas().insert(v);
        return v;
    }

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name){
        RWMutexType::Readlock lock(GetMutex());
        auto it=GetDatas().find(name);
        if(it == GetDatas().end()){
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T> >(it->second());
    }

    static void LoadFromYaml(const YAML::Node& root);

    static ConfigVarBase::ptr LookupBase(const std::string& name);

private:

                                      //Config是一个全局变量，
    static ConfigVarMap& GetDatas(){  //这里很关键，为什么表示直接定义静态类变量，而是通过静态函数方式创建静态类变量
        static ConfigVarMap s_datas;  //原因是因为，Config的全局变量的初始化是没有严格的顺序，
        return s_datas;              //如果我们在使用最高静态类变量的时候还没有初始化好，就会出现内存错误，
    }

    static RWMutexType& GetMutex(){
        static RWMutex s_mutex;
        return s_mutex;
    }

};


};






#endif