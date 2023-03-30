#include "iomanager.h"
#include "macro.h"
#include "sys/epoll.h"
#include "unistd.h"
#include "log.h"
#include "fcntl.h"


namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

enum EpollCtlOp{
};

static std::ostream& operator<< (std::ostream& os, const EpollCtlOp& op) {
    switch((int)op) {
#define XX(ctl) \
        case ctl: \
            return os << #ctl;
        XX(EPOLL_CTL_ADD);
        XX(EPOLL_CTL_MOD);
        XX(EPOLL_CTL_DEL);
        default:
            return os << (int)op;        
    }
#undef XX
}

static std::ostream& operator<< (std::ostream& os, EPOLL_EVENTS events) {
    if(!events) {
        return os << "0";
    }
    bool first = true;
#define XX(E) \
    if(events & E) { \
        if(!first) { \
            os << "|"; \
        } \
        os << #E; \
        first = false; \
    }
    XX(EPOLLIN);
    XX(EPOLLPRI);
    XX(EPOLLOUT);
    XX(EPOLLRDNORM);
    XX(EPOLLRDBAND);
    XX(EPOLLWRNORM);
    XX(EPOLLWRBAND);
    XX(EPOLLMSG);
    XX(EPOLLERR);
    XX(EPOLLHUP);
    XX(EPOLLRDHUP);
    XX(EPOLLONESHOT);
    XX(EPOLLET);
}


IOManager::FdContext::EventContext& IOManager::FdContext::getContext(Event event){
    switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SYLAR_ASSERT2(false, "getContext");
    }
    throw std::invalid_argument("getContext invalid event");
}

void IOManager::FdContext::resetContext(EventContext& ctx){
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(Event event){
    SYLAR_ASSERT(events & event);
    events = (Event)(events & ~event);     //triggerEvent函数执行了event对应的事件， 那么event对应的事件就没了， 所以events & ～event返回就是剩下的事件类型了，
    EventContext& ctx = getContext(event);
    if(ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    ctx.cb = nullptr;
    ctx.fiber = nullptr;
    return;
}


IOManager::IOManager(size_t threads , bool use_caller , const std::string name)
    : Scheduler(threads, use_caller, name) {
    m_epfd = epoll_create(5000);
    SYLAR_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    SYLAR_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    SYLAR_ASSERT(!rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    SYLAR_ASSERT(!rt);

    contextResize(32, 0);
    //m_fdContexts.resize(64);
    start();  //开启协程调度器，
}

IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size, size_t old_size) {
    m_fdContexts.resize(size);
    for(size_t i = old_size; i < m_fdContexts.size(); ++i){
        if(!m_fdContexts[i]){
            m_fdContexts[i] = new FdContext();
            m_fdContexts[i]->fd = i;
        }
    }
}


//1 success, 0 retry, -1 error
int IOManager::addEvent(int fd, Event event, std::function<void()> cb){     //添加 fd的event类型的事件， 回调事件时cb，
    FdContext* fd_ctx = nullptr;
    RWMutexType::Readlock lock(m_mutex);
    if(m_fdContexts.size() > (size_t)fd){   
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(m_fdContexts.size() * 1.5, m_fdContexts.size());
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock3(fd_ctx->mutex);
    if(fd_ctx->events & event) { //默认fd_ctx是没有event类型的事件，因为addEent是要添加event的事件给fd，   如果events & event = true，那么原来就有event的事件
        SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd = " << fd
                    << "event=" <<event
                    <<" fd_ctx.event=" <<fd_ctx->events;
        SYLAR_ASSERT(!(fd_ctx->events & event)); 
    }

    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;    //确定到底是修改红黑树上的节点还是添加，
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;   //将EPOLLIN 和 EPOLLOUT或给events，
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " <<fd << ", " << epevent.events << "):"
            << rt <<" (" << errno << ")(" << strerror(errno) << ")";
            return -1;
    }

    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);

    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);   //新给fd添加的eventcontext，类型是event，
    
    SYLAR_ASSERT(!event_ctx.scheduler
                && !event_ctx.fiber
                && !event_ctx.cb);
    
    event_ctx.scheduler = Scheduler::GetThis();
    if(cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        SYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC
                        , "state=" << event_ctx.fiber->getState());
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event){
    RWMutexType::Readlock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events & event) {   //很明显，events中没有event类型的事件，
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);   //new_events等于 fd_ctx->events去除event类型，
    int op = new_events ? EPOLL_CTL_MOD :EPOLL_CTL_DEL;  //如果原来fd_ctx->events不止有event类型，   那么epoll_ctl就是修改，
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " <<fd << ", " << epevent.events << "):"
            << rt <<" (" << errno << ")(" << strerror(errno) << ")";
            return false;
    }

    --m_pendingEventCount;
    fd_ctx->events = new_events;  
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;

}

bool IOManager::cancelEvent(int fd, Event event){
    RWMutexType::Readlock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events & event) {
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event); 
    int op = new_events ? EPOLL_CTL_MOD :EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " <<fd << ", " << epevent.events << "):"
            << rt <<" (" << errno << ")(" << strerror(errno) << ")";
            return false;
    }

    fd_ctx->triggerEvent(event);   //cancleEvent函数最后会触发event-EventContex，
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd){
    RWMutexType::Readlock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd){
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << ", " <<fd << ", " << epevent.events << "):"
            << rt <<" (" << errno << ")(" << strerror(errno) << ")";
            return false;
    }

    if(fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    return true;
}



IOManager* IOManager::GetThis(){
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}



void IOManager::tickle() {
    if(!hasIdleThreads()){
        return ;
    }
    int rt = write(m_tickleFds[1], "T", 1);   //写入一个字符，监听这个m_tickleFds[0]的事件就会被触发，
    SYLAR_ASSERT(rt == 1);
}

bool IOManager::stopping(uint64_t& timeout) {
    timeout = getNextTimer();
    return timeout == ~0ull
        && m_pendingEventCount == 0
        && Scheduler::stopping();
}   


bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}


void IOManager::idle() {
    epoll_event* events = new epoll_event[64]();
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){  //不需要用到智能指针，主要是自动释放，
        delete[] ptr;
    });

    uint64_t next_timeout = 0;
    while(true) {
        next_timeout = getNextTimer();
        if(stopping(next_timeout)) {
            SYLAR_LOG_INFO(g_logger) <<"name =" << getName() << " idle stopping exit";
            break;
        }
    }

   

    int rt = 0;  //有多少个epoll事件被监听到了，
    do {
        static const int MAX_TIMEOUT = 5000;   //单位是毫秒
        if(next_timeout != ~0ull) {
            next_timeout = (int)next_timeout > MAX_TIMEOUT
                                    ? MAX_TIMEOUT : next_timeout;
        }else {
            next_timeout = MAX_TIMEOUT;
        }
        rt = epoll_wait(m_epfd, events, 64, (int)next_timeout);       //循环监听是否有事件被触发，
        if(rt < 0 && errno ==EINTR) {
        } else {
            break;
        }
    } while (true);

    std::vector<std::function<void()> > cbs;
    listExpiredCb(cbs);
    if(!cbs.empty()) {
        schedule(cbs.begin(), cbs.end());
        cbs.clear();
    }
    
    for(int i = 0;i < rt; ++i) {
        epoll_event& event = events[i];
        if(event.data.fd == m_tickleFds[0]){
            uint8_t dummy;
            while(read(m_tickleFds[0], &dummy, 1) == 1);
            continue;
        }

        FdContext* fd_ctx = (FdContext*)event.data.ptr;   //在addevent时， fdcontext被保存到了event_epoll.data.ptr里面了，
        FdContext::MutexType::Lock lock(fd_ctx->mutex);
        if(event.events & (EPOLLERR | EPOLLHUP)) {
            event.events |= EPOLLIN |EPOLLOUT;
        }
        int real_events = NONE;
        if(event.events & EPOLLIN) {
            real_events |= READ;
        }
        if(event.events & EPOLLOUT) {
            real_events |= WRITE;
        }

        if((fd_ctx->events & real_events) == NONE) {  //再次判断是否fd_ctx有对于的事件需要处理，
            continue;
        }

        int left_events = (fd_ctx->events & ~real_events);   //left_events保存了fd_ctx没有监听到的事件，也就是不需要处理的事件
        int op =left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        event.events = EPOLLET | left_events;     //需要修改epoll红黑树上的fd需要监听的事件，

        int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
        if(rt2) {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << op << ", " <<fd_ctx->fd << ", " << event.events << "):"
                     << rt <<" (" << errno << ")(" << strerror(errno) << ")";
            continue;
        }

        if(real_events & READ) {
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
        }
        if(real_events & WRITE) {
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }
    }
    Fiber::ptr cur = Fiber::GetThis();
    //下面两行代码是为了释放内存，
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();

}


void IOManager::onTimeInsertedAtFront() {
    tickle();
}





};