#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include <sys/epoll.h>
#include <cassert>
#include <fcntl.h>
#include <vector>
#include <map>
#include <cstdarg>
#include <cstring>
#include "DataDef.h"
#include "Locker.h"
#include "HttpTask.h"

template <typename T>
class CThreadPool
{
public:
    CThreadPool(void *arg);
    ~CThreadPool();
    bool append(T *request);
    pthread_t *get_threads() { return m_Threads; }
    static void *WorkerThreadProc(void *arg);   // 下载文件工作线程
    static void *WorkerWriteLogFileThread(void *arg);   // 从日志链表读取日志信息，写错误日志线程
    void Log(const char *format, ...);  // 打印日志信息并推送至日志链表
    void RunWriteLogFileThread();   // 写错误日志文件
    void SetThreadExited(); // 设置并通知所有工作线程退出
    unsigned int GetThreadNum() { return m_nThreadNum; }    // 获取线程池线程数
    void WaitThreadsExit(); // 等待所有工作线程退出

private:
    unsigned int m_nThreadNum;  // 线程池中线程数
    unsigned int m_nMaxTaskNum; // 线程池处理最大任务数
    unsigned int m_nThreadStackSize;    // 线程池单个线程堆栈大小
    unsigned int m_nCurTaskNum; // 当前任务数
    pthread_t *m_Threads;   // 指向线程数组指针
    pthread_t m_LogThr; // 日志线程号
    std::list<T *> m_TaskQueue; // 任务存放链表
    std::list<char *> m_LogQueue;   // 日志存放链表
    locker m_TaskQueuelocker;   // 任务链表线程锁
    locker m_Loglocker; // 日志链表线程锁
    sem m_TaskQueueSem; // 通知工作线程获取任务信号量
    sem m_LogSem;   // 通知日志线程获取日志信息信号量
    bool m_stop;    // 工作线程退出标志
    void run(); // 执行任务
    int z_conf_check(threadpool_conf_t *conf);  // 初始化线程池配置
};

template <typename T>
CThreadPool<T>::CThreadPool(void *arg) : m_stop(false), m_Threads(NULL)
{
    threadpool_conf_t *conf = (threadpool_conf_t *)arg;

    if (z_conf_check(conf) == -1)
    {
        Log("[error] thread pool configure error");
        throw std::exception();
    }

    m_nThreadNum = conf->thread_num;
    m_nMaxTaskNum = conf->max_tasknum;
    m_nThreadStackSize = conf->thread_stack_size;

    if (pthread_create(&m_LogThr, NULL, WorkerWriteLogFileThread, this) != 0)
    {
        Log("[error] create log thread failed");
        throw std::exception();
    }
    if (pthread_detach(m_LogThr))
    {
        Log("[error] detach log thread failed");
        throw std::exception();
    }

    pthread_attr_t attr;    // 设置线程属性
    if (pthread_attr_init(&attr) != 0)
    {
        throw std::exception();
    }

    if (conf->thread_stack_size != 0)
    {
        if (pthread_attr_setstacksize(&attr, conf->thread_stack_size) != 0)
        {
            pthread_attr_destroy(&attr);
            throw std::exception();
        }
    }

    m_Threads = new pthread_t[conf->thread_num];
    if (!m_Threads)
    {
        pthread_attr_destroy(&attr);
        throw std::exception();
    }

    for (unsigned int i = 0; i < conf->thread_num; ++i)
    {
        if (pthread_create(m_Threads + i, &attr, WorkerThreadProc, this) != 0)
        {
            Log("[error] create work thread failed");
            delete[] m_Threads;
            pthread_attr_destroy(&attr);
            throw std::exception();
        }

        printf("[notice] create the %dth work thread\n", i);
    }
    pthread_attr_destroy(&attr);
}

template <typename T>
CThreadPool<T>::~CThreadPool()
{
    delete[] m_Threads;
    m_stop = true;
}

template <typename T>
bool CThreadPool<T>::append(T *task)
{
    m_TaskQueuelocker.lock();
    if (m_TaskQueue.size() > m_nMaxTaskNum)
    {
        m_TaskQueuelocker.unlock();
        return false;
    }
    m_TaskQueue.push_back(task);
    m_TaskQueuelocker.unlock();
    m_TaskQueueSem.post();
    return true;
}

template <typename T>
int CThreadPool<T>::z_conf_check(threadpool_conf_t *conf)
{
    if (conf == NULL)
    {
        return -1;
    }

    if (conf->thread_num < 1)
    {
        conf->thread_num = THREAD_NUMBER;
    }

    if (conf->max_tasknum < 1)
    {
        conf->max_tasknum = MAX_TASK_SIZE;
    }

    return 0;
}

template <typename T>
void CThreadPool<T>::SetThreadExited()
{
    m_stop = true;
    m_TaskQueueSem.post();
}

template <typename T>
void CThreadPool<T>::WaitThreadsExit()
{
    for (unsigned int i = 0; i < m_nThreadNum; ++i)
    {
        pthread_join(m_Threads[i], NULL);
    }
}

template <typename T>
void *CThreadPool<T>::WorkerWriteLogFileThread(void *arg)
{
    CThreadPool *pool = (CThreadPool *)arg;
    pool->RunWriteLogFileThread();
    return pool;
}

template <typename T>
void CThreadPool<T>::RunWriteLogFileThread()
{
    while (1)
    {
        m_LogSem.wait();
        m_Loglocker.lock();
        if (m_LogQueue.empty())
        {
            m_Loglocker.unlock();
            continue;
        }
        char *mLog = m_LogQueue.front();
        m_LogQueue.pop_front();
        m_Loglocker.unlock();
        if (!mLog)
        {
            continue;
        }

    REWRITE:
        FILE *pFile = fopen("error.log", "a+");
        if (pFile == NULL)
            return;
        int nInfoLen = strlen(mLog);
        int nRetCode = fwrite(mLog, sizeof(char), nInfoLen, pFile);
        if (nRetCode != nInfoLen)
            goto REWRITE;
        else
        {
            fflush(pFile);
        }
    }
}

template <typename T>
void CThreadPool<T>::Log(const char *format, ...)
{
    char wzLog[1024] = {0};
    char szBuffer[1024] = {0};
    va_list args;
    va_start(args, format);
    vsprintf(wzLog, format, args);
    va_end(args);

    time_t now;
    time(&now);
    struct tm *local;
    local = localtime(&now);
    sprintf(szBuffer, "%04d-%02d-%02d %02d:%02d:%02d (%s %d) %s\n", local->tm_year + 1900, local->tm_mon, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec, __FILE__, __LINE__, wzLog);
    printf("%s", szBuffer);

    m_Loglocker.lock();
    m_LogQueue.push_back(szBuffer);
    m_Loglocker.unlock();
    m_LogSem.post();

    return;
}

template <typename T>
void *CThreadPool<T>::WorkerThreadProc(void *arg)
{
    CThreadPool *pool = (CThreadPool *)arg;
    /*
    int err = pthread_detach(pthread_self());
    if( err != 0 )
    {
        delete [] (pool->get_threads());
        throw std::exception();
    }
    */

    pool->run();

    return pool;
}

template <typename T>
void CThreadPool<T>::run()
{
    bool bRet;
    while (!m_stop)
    {
        T* task = nullptr;

        bRet = m_TaskQueueSem.wait();
        if (!bRet)
        {
            // Log("");
            continue;
        }
        //printf("Task Queue have member!");
        m_TaskQueuelocker.lock();
        if (m_TaskQueue.empty())
        {
            m_TaskQueuelocker.unlock();
            continue;
        }

        task = m_TaskQueue.front();
        m_TaskQueue.pop_front();
        m_TaskQueuelocker.unlock();

        if (!task)
        {
            continue;
        }

        bRet = task->Run();
        if (bRet)
        {
            m_stop = true;
        }
    }
}

#endif
