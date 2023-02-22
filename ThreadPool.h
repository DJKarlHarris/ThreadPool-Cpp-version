#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_
 
#include "TaskQueue.h"
#include <vector>

template<typename T>
class ThreadPool {

public:
    // 创建线程池并初始化
    ThreadPool(int min, int max);

    // 销毁线程池
    ~ThreadPool();

    // 给线程池添加任务
    void addTask(Task<T> task);
    //overload
    void addTask(callback funciton, void* arg);

    // 获取线程池中工作的线程的个数
    int getBusyNum();

    // 获取线程池中活着的线程的个数
    int getAliveNum();

private:
    // 工作的线程(消费者线程)任务函数
    static void* worker(void* arg);

    //管理者线程
    static void* manager(void* arg);

    // 单个线程退出
    void threadExit();

private:
    // 任务队列
    TaskQueue<T> *m_taskqueue;

    pthread_t m_managerID;    // 管理者线程ID
    pthread_t* m_workerID;  // 工作的线程ID
    int m_minNum;             // 最小线程数量
    int m_maxNum;             // 最大线程数量
    int m_busyNum;            // 忙的线程的个数
    int m_liveNum;            // 存活的线程的个数
    int m_exitNum;            // 要销毁的线程个数
    pthread_mutex_t m_mutex;  // 锁整个的线程池
    pthread_cond_t m_notEmpty;    // 任务队列是不是空了
    static const int KILLNUMBER = 2;
    static const int CREATENUMBER = 2;

    bool shutdown;           // 是不是要销毁线程池, 销毁为1, 不销毁为0

};

#endif