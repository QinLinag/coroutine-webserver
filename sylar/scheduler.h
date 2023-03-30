#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <string>
#include <list>
#include <vector>
#include "thread.h"
#include "fiber.h"
#include "hook.h"

namespace sylar {



class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const { return m_name;}

    static Scheduler* GetThis();
    static Fiber* GetMainFiber();

    void start();
    void stop();

    //schedule这个函数的作用就是加入一个任务到 m_threads中，
    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1){
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        if(need_tickle){
            tickle();  //这个tickle函数，如果m_threads中有任务，就会通知任务去执行，
        }
    }


    //调度协程
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end){
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutes);
            while(begin != end){
                need_tickle = scheduleNoLock(&begin, -1) || need_tickle;
                ++begin;
            }
        }
        if(need_tickle){
            tickle();
        }
    }

protected:
    //通知协程调度器，有任务了，
    virtual void tickle();
    //协程调度函数
    void run();
    virtual bool stopping();
    void setThis();
    virtual void idle();
private:
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread){
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if(ft.fiber || ft.cb){
            m_fibers.push_back(ft);    //加入一个任务，
        }
        return need_tickle;
    }

private:
    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        FiberAndThread(Fiber::ptr f, int thr)
                :fiber(f), thread(thr){ 
        }

        FiberAndThread(Fiber::ptr* f, int thr)
                :thread(thr) {
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr)
                :cb(f), thread(thr){
        }

        FiberAndThread(std::function<void()>* f, int thr)
                :thread(thr) {
            cb.swap(*f);
        }

        FiberAndThread()
                :thread(-1){
        }

        void reset(){
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }

    };



private:
    MutexType m_mutex;
    std::vector<sylar::Thread::ptr> m_threads;
    std::list<FiberAndThread> m_fibers;
    std::string m_name;
    Fiber::ptr m_rootFiber;
protected:
    std::vector<int> m_threadIds;
    size_t m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount = {0};
    std::atomic<size_t> m_idLeThreadCount = {0};
    bool m_stopping = true;
    bool m_autoStop = false;
    int m_rootThread = 0;

};

};


#endif