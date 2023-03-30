#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include "scheduler.h"
#include <memory>
#include "thread.h"
#include "timer.h"
#include <functional>
#include <atomic>
#include <vector>


namespace sylar {

class IOManager :public Scheduler, public TimerManager {
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event{
        NONE    = 0x0,
        READ    = 0x1,  //EPOLLIN
        WRITE   = 0x4   //EPOLLOUT
    };

private:


    struct FdContext {
        typedef Mutex MutexType;
        struct EventContext {
            Scheduler* scheduler = nullptr; //事件执行的scheduler
            Fiber::ptr fiber;               //事件协程
            std::function<void()> cb;       //事件的回调函数，
        };

        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);

        EventContext read;    //读事件，
        EventContext write;   //写事件,
        int fd = 0;               //事件关联的句柄
        Event events = NONE; //已经注册的事件，
        MutexType mutex;
    };


public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string name ="");
    ~IOManager();
    
    //0 success, -1 error
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

    //删除事件fd对应的fdcontext中的event-EventContext事件，不会触发event-EventContext事件，
    bool delEvent(int fd, Event event);
    //删除事件fd对应的fdcontext中的event-EventContext事件，会触发event-EventContext事件，
    bool cancelEvent(int fd, Event event);
    bool cancelAll(int fd);

    bool hasIdleThreads() { return m_idLeThreadCount == 0; }

    static IOManager* GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    bool stopping(uint64_t& timeout);
    void contextResize(size_t size, size_t old_size)
    void onTimeInsertedAtFront() override;
private:
    int m_epfd = 0;
    int m_tickleFds[2];


    std::atomic<size_t> m_pendingEventCount = {0};
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;
};  



}


#endif