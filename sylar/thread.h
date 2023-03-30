#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <pthread.h>
#include <thread>
#include <functional>
#include <memory>
#include <semaphore.h>
#include <atomic>
#include <noncopyable.h>

namespace sylar{


class Semaphore : public Noncopyable{
public:
    Semaphore(uint32_t count=0);  //信号量数量
    ~Semaphore();

    void wait();
    void notify();
 
private:
    sem_t m_semaphore;
};


//抽取出统一加锁和解锁api，
template<class T>
struct ScopedLockImpl {
public:
    ScopedLockImpl(T& mutex)        //在构造函数中加索
        :m_mutex(mutex){
        m_mutex.lock();
        m_locked =true;
    }

    ~ScopedLockImpl(){             //析构函数释放索
        m_mutex.unlock();
    }

    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }


    void lock(){
        if(!m_locked){
            m_mutex.lock();
            m_locked = true;
        }
    }

private:
    T& m_mutex;
    bool m_locked;

};


template<class T>
struct ReadScopedLockImpl {
public:
    ReadScopedLockImpl(T& mutex)        //在构造函数中加索
        :m_mutex(mutex){
        m_mutex.rdlock();
        m_locked =true;
    }

    ~ScopedLockImpl() {             //析构函数释放索
        m_mutex.unlock();
    }

    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }


    void lock(){
        if(!m_locked){
            m_mutex.rdlock();
            m_locked = true;
        }
    }

private:
    T& m_mutex;
    bool m_locked;

};

template<class T>
struct WriteScopedLockImpl {
public:
    WriteScopedLockImpl(T& mutex)        //在构造函数中加索
        :m_mutex(mutex){
        m_mutex.wrlock();
        m_locked =true;
    }

   ~ScopedLockImpl(){             //析构函数释放索
        m_mutex.unlock();
    }

    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }


    void lock(){
        if(!m_locked){
            m_mutex.wrlock();
            m_locked = true;
        }
    }

private:
    T& m_mutex;
    bool m_locked;

};

class Mutex : public Noncopyable{
public:
    typedef ScopedLockImpl<Mutex> Lock;   //将加锁和解锁的api类，整合到了Mutex中，

    Mutex(){
        pthread_mutex_init(&m_mutex,nullptr);
    }

    ~Mutex(){
        pthread_mutex_destroy(&m_mutex);
    }

    void lock(){
        pthread_mutex_lock(&m_mutex);
    }

    void unlock(){
        pthread_mutex_unlock(&m_mutex);
    }

    
private:
    pthread_mutex_t m_mutex;
};

class RWMutex{
public:
    typedef ReadScopedLockImpl<RWMutex> Readlock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex(){
        pthread_rwlock_init(&m_lock,nullptr);   //初始化读写锁，
    }

    ~RWMutex(){
        pthread_rwlock_destroy(&m_lock);   //销毁读写锁
    }

    void rdlock(){
        pthread_rwlock_rdlock(&m_lock);    //读上锁
    }

    void wrlock(){
        pthread_rwlock_wrlock(&m_lock);   //写上锁
    }

    void unlock(){
        pthread_rwlock_unlock(&m_lock);    //解锁,
    }

private:
    pthread_rwlock_t m_lock;
};


//自选锁，更快，但是比较消耗cpu
class Spinlock : public Noncopyable{
public:
    typedef ScopedLockImpl<Spinlock> Lock;

    Spinlock(){
        pthread_spin_init(&m_mutex,0);
    }

    ~Spinlock(){
        pthread_spin_destroy(&m_mutex);
    }

    void lock(){
        pthread_spin_lock(&m_mutex);
    }

    void unlock(){
        pthread_spin_unlock(&m_mutex);
    }

private:
    pthread_spinlock_t m_mutex;
};

class CASLock : public Noncopyable{
public:
    typedef ScopedLockImpl<CASLock> Lock;
    CASLock() {
        m_mutex.clear();
    }

    ~CASLock() {

    }

    void lock() {
        while(std::atomic_flag_test_and_set_explicit(&m_mutex
        ,std::memory_order_acquire));
    }

    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex
        ,std::memory_order_release);
    }

private:
    volatile std::atomic_flag m_mutex;
};


//通过pthread_create实现了一个简单的线程类
class Thread{
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void ()> cb,const std::string& name);
    ~Thread();

    pid_t getId() {return m_id;};
    const std::string& getName() const {return m_name;};

    void join();

    static Thread* GetThis();
    static const std::string& GetName();
    static void SetName(const std::string& name);

private:
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) =delete;

    static void* run(void* arg);
private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;
    std::function<void ()> m_cb;
    std::string m_name;

    Semaphore m_semaphore;
};



}



#endif