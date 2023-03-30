#include "config.h"
#include "log.h"
#include <yaml-cpp/yaml.h>

//约定大于配置，
sylar::ConfigVar<int>::ptr g_int_value_config=
    sylar::Config::Lookup("system.port", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_float_value_config = 
    sylar::Config::Lookup("system.value", (float)10.2f, "system value");

sylar::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config = 
    sylar::Config::Lookup("system.int_vec", std::vector<int>{1,2}, "system int vector");

sylar::ConfigVar<std::list<int> >:: ptr g_int_list_value_config = 
    sylar::Config::Lookup("system.int_list", std::list<int>{1,2}, "system int list");

sylar::ConfigVar<std::set<int> >::ptr g_int_set_value_config = 
    sylar::Config::Lookup("system.int_set", std::set<int>{1,2,3,2}, "system int set");

sylar::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config = 
    sylar::Config::Lookup("system.int_set", std::unordered_set<int>{1,2,3,2}, "system int uset");

sylar::ConfigVar<std::map<std::string, int> >::ptr g_str_int_value_map_config =
    sylar::Config::Lookup("system.str_int_map" ,std::map<std::string ,int>{{"k",2}},"system str int map");

sylar::ConfigVar<std::unordered_map<std::string, int> >::ptr g_str_int_value_umap_config =
    sylar::Config::Lookup("system.str_int_umap" ,std::unordered_map<std::string ,int>{{"k",2}},"system str int umap");


void test_stl(){
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) <<g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) <<g_float_value_config->toString();

#define XX(g_var, name, prefix) \
    { \
        auto& v=g_var->getValue(); \
        for(auto& i : v){ \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix "" #name ": " <<i; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name "yaml: " <<g_var->toString(); \
    }

#define XX_M(g_var, name ,prefix) \
    { \
        auto& v =g_var->getValue(); \
        for(auto& i : v){ \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name ": {" \
                << i.first <<"-" <<i.second <<"}"; \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix " " #name "yaml: " <<g_var->toString(); \
    }

    XX(g_int_vec_value_config, int_vec, before);
    XX(g_int_list_value_config, int_list, before);
    XX(g_int_set_value_config, int_set, before);
    XX(g_int_uset_value_config, int_uset, before);
    XX_M(g_str_int_value_map_config, int_map, before);
    XX_M(g_str_int_value_umap_config,int_umap, before);
    

    YAML::Node root=YAML::LoadFile("/home/sylar/log.yml");
    sylar::Config::LoadFromYaml(root);


    XX(g_int_vec_value_config, int_vec, after);
    XX(g_int_list_value_config, int_list, after);
    XX(g_int_set_value_config, int_set, after);
    XX(g_int_uset_value_config, int_uset, after);
    XX_M(g_str_int_value_map_config, int_map, after);
    XX_M(g_str_int_value_umap_config,int_umap, after);

}






class Person{
public:
    Person(){}
    std::string m_name;
    int m_age;
    bool m_gender;

    std::string toString() const{
        std::stringstream ss;
        ss << "[Person name=" <<m_name
            <<" age=" << m_age
            <<" gender= " <<m_gender
            <<"]";
    return ss.str();
    }

    bool operator==(const Person& oth) const{
        return m_name == oth.m_name
            && m_age == oth.m_age
            && m_gender == oth.m_gender;
    }

};


namespace sylar{

template<>
class LexicalCast<std::string, Person> {
public:
    Person operator()(std::string& v){
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_gender = node["gender"].as<bool>();
        return p;
    }
};

template<>
class LexicalCast<Person, std::string>{
public:
    std::string operator()(const Person& p){
        YAML::Node node;
        std::stringstream ss;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["gender"] = p.m_gender;
        ss << node;
        return ss.str();
    }
};

}

sylar::ConfigVar<Person>::ptr g_person =
    sylar::Config::Lookup("class.peron", Person(), "system person");

sylar::ConfigVar<std::map<std::string,Person> >::ptr g_map_person =
    sylar::Config::Lookup("class.peron", std::map<std::string, Person>(), "system person");

sylar::ConfigVar<std::vector<std::map<std::string, Person> > >::ptr g_vec_map_person =
    sylar::Config::Lookup("class.peron", std::vector<std::map<std::string, Person> >(), "system person");



void test_class(){

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " <<g_person->getValue().toString() << "-" << g_person->toString();

#define XX_CLASS(g_val, prefix) \
    { \
        auto m = g_val->getValue(); \
        for(auto& i : m){ \
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix << ":" << i.first <<"-" <<i.second.toString(); \
        } \
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix << ": size = " <<m.size(); \
    }

    // 添加一个变更响应函数，lamda    g_person最高智能指针的m_val一旦改变，就会调用最高变更函数，
    g_person->addListener(10,[](const Person& oldValue, const Person& newValue){
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) <<"old_value= " <<oldValue.toString()
            << "new_value= " <<newValue.toString();
    });

    XX_CLASS(g_map_person, before);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_vec_map_person->toString();


    YAML::Node root=YAML::LoadFile("/home/sylar/log.yml");
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " <<g_person->getValue().toString() << "-" << g_person->toString();
    g_person->delListener(10);   //删除变更响应函数，

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_vec_map_person->toString();

    XX_CLASS(g_map_person, after);
}

void test_log(){
    static sylar::Logger::ptr system_log = SYLAR_LOG_NAME("system");
    
}




int main(int argc,char** argv){

    test_stl();

    test_class();
    

    return 0;
}

