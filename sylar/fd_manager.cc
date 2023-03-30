#include "fd_manager.h"
#include "hook.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>



namespace sylar {


FdCtx::FdCtx(int fd) 
    :m_isInit(false)
    ,m_isSocket(false)
    ,m_sysNonblock(false)
    ,m_userNonblock(false)
    ,m_isClosed(false)
    ,m_fd(fd)
    ,m_recvTimeout(-1)
    ,m_sendTimeout(-1){
        init();  //调用init函数里面判断fd是不是socket，
}

FdCtx::~FdCtx(){

}

bool FdCtx::init() {
    if(m_isInit) {
        return true;
    }

    m_recvTimeout = -1;
    m_sendTimeout = -1;

    struct stat fd_stat;
    if(-1 == fstat(m_fd, &fd_stat)) {
        m_isInit = false;
        return false;
    } else {
        m_isInit = true;
        m_isSocket = S_ISSOCK(fd_stat.st_mode);   //判断是不是socket，
    }

    if(m_isSocket) {       //将socket类型的fd设置为nonblock，
        int flags = fcntl_f(m_fd, F_GETFL, 0);
        if(!(flags & O_NONBLOCK)) {
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        }
        m_sysNonblock = true;
    } else {
        m_sysNonblock = false;
    }

    m_userNonblock = false;
    m_isClosed = false;

    return m_isInit;
}


bool FdCtx::close() {

}


void FdCtx::setTimeout(int type, uint64_t v) {
    if(type == SO_RCVTIMEO) {
        m_recvTimeout = v;
    } else {
        m_sendTimeout = v;
    }
}

uint64_t FdCtx::getTimeout(int type) {
    if(type == SO_RCVTIMEO) {
        return m_recvTimeout;
    } else {
        return m_sendTimeout;
    }
}


FdManager::FdManager() {
    m_datas.resize(64);
}

FdCtx::ptr FdManager::get(int fd, bool auto_create){
    if(fd == -1) {
        return nullptr;
    }
    RWMutexType::Readlock lock(m_mutex);
    if((int)m_datas.size() < fd) {
        if(auto_create == false) {   //本来fd就已经比m_datas的空间大了，所以肯定次fd没有被FdMgr管理，并且此时还不自动创建，那么就返回nullptr
            return nullptr;
        }
    } else {
        if(m_datas[fd] || !auto_create) {   //此fd已经被FdMgr管理了，
            return m_datas[fd];
        }
    }
    lock.unlock();

    RWMutexType::WriteLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    m_datas[fd] = ctx;
    return ctx;
}

void FdManager::del(int fd) {
    if(fd == -1) {
        return;
    }
    RWMutexType::WriteLock lock(m_mutex);
    if((int)m_datas.size() <= fd) {
        return;
    }
    m_datas[fd].reset();
}

}