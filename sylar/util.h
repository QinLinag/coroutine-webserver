#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <sys/time.h>

namespace sylar{

pid_t GetThreadId();
uint32_t GetFiberId();


//栈空间层的获取，
void Backtrace(std::vector<std::string>& bt,int size = 64, int skip);
std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "    ");

//时间，
uint64_t GetCurrentMS();
uint64_t GetCurrentUS();

}


#endif