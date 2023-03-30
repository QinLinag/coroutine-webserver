#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <memory>
#include <ucontext.h>
#include <functional>
#include "thread.h"
#include "scheduler.h"

//每个线程有一个主协程，主协程能够创建子协程，子协程执行完以后，要回到主协程
namespace sylar {


class Scheduler;
class Fiber : public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };

private:
    Fiber();

public:
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();

    //重置协程函数，并且重置状态，
    //INIT， TEMM
    void reset(std::function<void()> cb);
    //切换到当前协程执行， 主协程切换为子协程执行，
    void swapIn();
    //切换到后台执行， 当前正在执行的子协程切换为主协程
    void swapOut();

    uint64_t getId() { return m_id;}
    State getState() const { return m_state;}

    void call();
    void back();


public:
    //设置当前协程
    static void SetThis(Fiber* f);
    //返回当前协程
    static Fiber::ptr GetThis();
    //协程切换到后台，并且设置为Ready状态，
    static void YieldToReady();
    //协程切换到后台，并且设置为Hold状态，
    static void YieldToHold();
    //协程总数，
    static uint64_t TotalFibers();

    static  void MainFunc();
    static  void CallerMainFunc();

    static uint64_t GetFiberId();



private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    void* m_stack = nullptr;

    std::function<void()> m_cb;
};



}

#endif