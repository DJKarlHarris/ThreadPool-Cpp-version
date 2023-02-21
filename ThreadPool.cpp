
#include "ThreadPool.h"
#include <string.h>
#include <iostream>

ThreadPool::ThreadPool(int min, int max) {

    do 
    {
        //创建工作线程数组
        workerID = new pthread_t[max];

        memset(workerID, 0, sizeof(pthread_t) * max);
        minNum = min;
        maxNum = max;
        busyNum = 0;
        liveNum = min;    // 和最小个数相等
        exitNum = 0;

        taskqueue = new TaskQueue();

        //创建互斥锁
        if (pthread_mutex_init(&mutexPool, NULL) != 0 ||
            pthread_cond_init(&notEmpty, NULL) != 0)
        {
            std::cout << "mutex or condition init fail...\n" << std::endl;
            break;
        }

        // 创建线程
        pthread_create(managerID, NULL, manager, pool);
        for (int i = 0; i < min; ++i)
        {
            pthread_create(&workerID[i], NULL, worker, pool);
        }

        return;
    } while (0);

    // 释放资源
    if (workerID) {
        delete[] workerID;
    } 

    if(taskqueue) {
        delete taskqueue;
    }

    return;
}

ThreadPool::~ThreadPool() {

    // 关闭线程池
    shutdown = true;

    // 阻塞回收管理者线程
    pthread_join(managerID, NULL);

    // 唤醒阻塞的消费者线程
    for (int i = 0; i < liveNum; ++i) {
        pthread_cond_signal(&notEmpty);
    }

    for(int i = 0; i < maxNum; i++) {
        if(workerID[i] != 0) {
            pthread_join(workerID[i],NULL);
        }
    }

    // 释放堆内存
    if (taskqueue) {
        delete taskqueue;
    }

    if(workerID) {
        delete[] workerID;
    }

    //销毁锁和条件变量
    pthread_mutex_destroy(&mutexPool);
    pthread_cond_destroy(&notEmpty);

}

void ThreadPool::addTask(Task task)
{
    
}

int ThreadPool::getBusyNum()
{
    
}

int ThreadPool::getAliveNum()
{
    
}

void* ThreadPool::worker(void* arg)
{
    
}

void* ThreadPool::manager(void* arg)
{
    
}

void ThreadPool::threadExit()
{
    
}
