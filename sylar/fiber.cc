#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include <atomic>
#include "util.h"



namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};
static std::atomic<uint64_t> s_fiber_count {0};

static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;

//协程栈的大小
static sylar::ConfigVar<uint32_t>::ptr g_fiber_stack_size = 
    sylar::Config::Lookup<uint32_t>("fiber.stack_size",1024*1024,"fiber stack size");

class MallocStackAllocator {
public:
    static void* Alloc(size_t size){
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size){
        return free(vp);
    }

};
using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId(){
    if(t_fiber){
        return t_fiber->getId();
    }
    return 0;
}



//默认构造函数，是主协程， 第一个被创建的协程就是主协程
Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    if(getcontext(&m_ctx)){
        SYLAR_ASSERT2(false,"getcontext");
    }
    ++s_fiber_count;

    SYLAR_LOG_INFO(g_logger) <<"Fiber::Fiber()";
}


Fiber::Fiber(std::function<void()> cb, size_t stacksize = 0,bool use_caller)
    :m_id(++s_fiber_id)
    ,m_cb(cb){
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    m_stack = StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx)){      //getcontext()函数的作用：初始化ucp结构体，将当前上下文保存在ucp中，
        SYLAR_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr; //这个uc_link，意思是，当前上下文执行结束后，需要切换到的另一个上下文，如果是nullptr，执行完该协程后，线程就退出，
    m_ctx.uc_stack.ss_sp = m_stack;    //该上下文使用的栈，
    m_ctx.uc_stack.ss_size = m_stacksize;

    if(!use_caller){
        makecontext(&m_ctx, &Fiber::MainFunc, 0); //makecontext函数执行之前，一定要执行一次getcontext，makecontext()函数的作用：将ucp对应的上下文的指令地址指向func函数(协程)地址，argc是func的参数，
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0); 
    }

    SYLAR_LOG_INFO(g_logger) <<"Fiber::Fiber id=" << m_id;

}

Fiber::~Fiber(){
    --s_fiber_count;
    if(m_stack){
        SYLAR_ASSERT(m_state == TERM || m_state == INIT || m_state ==EXCEPT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state == EXCEPT);

        Fiber* cur = t_fiber;
        if(cur == this){
            SetThis(nullptr);
        }
    }
    SYLAR_LOG_INFO(g_logger) <<"Fiber::Fiber id=" << m_id;

}

//重置协程函数，并且重置状态，
//INIT， 
void Fiber::reset(std::function<void()> cb){
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);
    m_cb = cb;
    if(getcontext(&m_ctx)){
        SYLAR_ASSERT2(false ,"getcontxt");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

void Fiber::call(){
    m_state = EXEC;
    if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)){
        SYLAR_ASSERT2(false,"swapcontext");
    }
}

void Fiber::back(){
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)){
        SYLAR_ASSERT2(false, "swapcontext");
    }
}

//切换到当前协程执行
void Fiber::swapIn(){
    SetThis(this);
    SYLAR_ASSERT(m_state != EXEC);
    m_state = EXEC;
    if(swapcontext(&Scheduler::GetMainFiber()->m_ctx,&m_ctx)){ 
        SYLAR_ASSERT2(false,"swapcontext");
    }   
    
}
//切换到后台执行
void Fiber::swapOut(){
    SetThis(Scheduler::GetMainFiber());
    if(swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)){
        SYLAR_ASSERT2(false,"swapcontext");
    }
}


//设置当前协程
void Fiber::SetThis(Fiber* f){
    t_fiber = f;
}
//返回当前协程
Fiber::ptr Fiber::GetThis(){
    if(t_fiber){
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber());     //实例化一个主协程，
    SYLAR_ASSERT((t_fiber == main_fiber.get()));
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

//协程切换到后台，并且设置为Ready状态，
void Fiber::YieldToReady(){
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}
//协程切换到后台，并且设置为Hold状态，
void Fiber::YieldToHold(){
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;  //切换为主协助程
    cur->swapOut();
}
//协程总数，
uint64_t Fiber::TotalFibers(){
    return s_fiber_count;
}

//让协程执行其任务，
void Fiber::MainFunc(){
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try{
        cur->m_cb();
        cur->m_cb = nullptr;   //只执行一次，
        cur-> m_state = TERM;
    } catch(std::exception& ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << "fiber_if= " << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    } catch (...) {
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: "
            << "fiber_if= " << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }

    auto raw_ptr = cur.get(); 
    cur.reset();   //计数减1，
    raw_ptr->swapOut();

    SYLAR_ASSERT2(false, "fiber should swapout never reach fiber_id= " + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc(){
    Fiber::ptr cur = GetThis();
    SYLAR_ASSERT(cur);
    try{
        cur->m_cb();
        cur->m_cb = nullptr;
        cur-> m_state = TERM;
    } catch(std::exception& ex) {
        cur->m_state = EXCEPT;
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
            << "fiber_if= " << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    } catch (...) {
        SYLAR_LOG_ERROR(g_logger) << "Fiber Except: "
            << "fiber_if= " << cur->getId()
            << std::endl
            << sylar::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();

    SYLAR_ASSERT2(false, "never reach fiber_id= " + std::to_string(raw_ptr->getId()));
}

};