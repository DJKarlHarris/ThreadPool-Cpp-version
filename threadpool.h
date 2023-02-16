#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <stdio.h>


typedef struct ThreadPool ThreadPool;

//创建线程池并初始化，返回线程池地址
ThreadPool* threadPoolCreate(int min, int max,int Size) {

    {
        //创建线程池实例
        ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
        if(pool == NULL) {
            printf("create pool fail...\n");
            break;
        }

        //创建工作线程数组
        pool->workerID = malloc(max * sizeof(pthread_t));
        if(pool->workerID == NULL) {
            printf("create workerID fail...\n");
            break;
        }
        memset(pool->workerID, 0, sizeof(pthread_t) * max);
        pool->minNum = min;
        pool->maxNum = max;
        pool->busyNum = 0;
        pool->liveNum = min; 
        pool->exit = 0;

        //创建任务数组
        pool->taskQueue = (Task*)malloc(sizeof(Task) * queueSize);
        pool->queueCapacity = Size;
        pool->queueSize = 0; 
        pool->queueFront = 0;
        pool->queueRear = 0;

        //线程池为就绪状态
        pool->shutdown = 0;

        //初始化互斥锁和条件变量
        if(pthread_mutex_init(&mutexPool, NULL) != 0 || 
        pthread_mutex_init(&mutexBuzyNum, NULL) != 0 ||
        pthread_cond_init(&notFull, NULL) != 0 ||
        pthread_cond_init(&notEmpty, NULL) != 0) {
            printf("mutex or condition creation fail...\n");
            break;
        } 

        //创建线程
        pthread_create(&pool->managerID, NULL, manager, NULL);
        for(int i = 0; i < min; i++) {
            pthread_create(&pool->workerID, NULL, worker, NULL);
        }

        return pool;

    } while(0);
    
    if(pool && pool->taskQueue) {
        free(pool->taskQueue);
    }

    if(pool && pool->workerID) {
        free(pool->workerID);
    }

    if(pool) {
        free(pool);
    }
    //malloc - free
    return NULL;
}
//销毁线程池

//添加任务

//获取当前线程池多少忙线程

//获取当前线程池多少活着的线程

#endif