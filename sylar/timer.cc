#include "timer.h"

namespace sylar {

bool Timer::Comparator::operator() (const Timer::ptr& lhs, const Timer::ptr& rhs) const {
    if(!lhs && !rhs) {
        return false;
    }

    if(!lhs) {
        return true;
    }
    if(!rhs) {
        return false;
    }

    if(lhs->m_next < rhs->m_next) {
        return true;
    }

    if(lhs->m_next > rhs->m_next) {
        return false;
    }
    return lhs.get() <= rhs.get();
}


Timer::Timer(uint64_t ms, std::function<void()> cb,
            bool recurring, TimerManager* manager) 
    :m_ms(ms)
    ,m_cb(cb)
    ,m_recurring(recurring)
    ,m_manager(manager){
    m_next = sylar::GetCurrentMS() + m_ms;
}

Timer::Timer(uint64_t next) 
    :m_next(next){
}

bool Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }

    m_manager->m_timers.erase(it);       //先删除，从timers中，
    m_next = sylar::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this()); //这里不是直接修改timers里面的timer的时间，而是先将其移除再插入，这样不会影响set数据结构，
    return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
    if(ms == m_ms && !from_now) {     //时间周期相同，并且从现在开始的，那么就不需要reset，
        return true;
    }

    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }

    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if(from_now) {      //从现在开始，
        start = sylar::GetCurrentMS();
    } else {
        start = m_next - m_ms;     //上一次执行的时间，
    }
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(),lock);
    return true;
}

TimerManager::TimerManager() {
    m_previousTimer = sylar::GetCurrentMS();
}

TimerManager::~TimerManager() {
}

void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock) {
    auto it = m_timers.insert(val).first; //set集合的insert方法的返回值是一个pair对象，first是一个iterator，指向插入的element，
                                               //第second是一个bool值，表示是否插入成功， 
    
    bool at_front = (it == m_timers.begin()) && !m_tickled;  //如果新插入的timer，排在m_timers的最前面，那么说明，新插入的timer即将执行最快，
    if(at_front) {
        m_tickled = true;
    }
    lock.unlock();
    if(at_front) {
        onTimeInsertedAtFront();
    }
}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb,
                        bool recurring = false) {
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    
    addTimer(timer, lock);

    return timer;
}

bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    if(now_ms < m_previousTimer
        && now_ms < (m_previousTimer - 60*60*1000)) {
            rollover = true;
        }
    m_previousTimer = now_ms;
    return rollover;
 }



static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    std::shared_ptr<void> tmp = weak_cond.lock();
    if(tmp) {   //只有条件满足的情况下，定时器时间到了才能触发cb函数，
        cb();
    }
}
        //条件定时器，需要条件满足才能触发，
Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb
                                ,std::weak_ptr<void> weak_cond
                                ,bool recurring) {
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb) , recurring);
}


uint64_t TimerManager::getNextTimer() {
    RWMutexType::Readlock lock(m_mutex);
    m_tickled = false;
    if(m_timers.empty()) {
        return ~0ull;   // unsigned long long类型的0 ，～将所有位取反，   得到一个很大的数值，
    }

    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = sylar::GetCurrentMS();
    if(now_ms >= next->m_next) {
        return 0;
    } else {
        return next->m_next - now_ms;
    }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs) {
    uint64_t now_ms = sylar::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    
    {
        RWMutexType::Readlock lock(m_mutex);
        if(m_timers.empty()) {
            return;
        }
    }
    RWMutexType::WriteLock lock1(m_mutex);

    bool rollover = detectClockRollover(now_ms);
    if(!rollover && ((*m_timers.begin())->m_next > now_ms)) {
        return ;
    }

    Timer::ptr now_timer(new Timer(now_ms));
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    while(it != m_timers.end() && (*it)->m_next == now_ms) {
        ++it;
    }
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(expired.size());


    for(auto& timer : expired) {
        cbs.push_back(timer->m_cb);
        if(timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {
            timer->m_cb = nullptr;
        }
    }


}

bool TimerManager::hasTimer() {
    RWMutexType::Readlock lock(m_mutex);
    return m_timers.empty();
}




};