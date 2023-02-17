#include <stdio.h>
#include "threadpool.h"

#define KILLNUMBER 2
#define CREATENUMBER 2

//创建线程池并初始化，返回线程池地址
ThreadPool* threadPoolCreate(int min, int max,int Size) {

    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    do {
        //创建线程池实例
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
        pool->buzyNum = 0;
        pool->liveNum = min; 
        pool->exitNum = 0;

        //创建任务数组
        pool->taskQueue = (Task*)malloc(sizeof(Task) * pool->queueSize);
        pool->queueCapacity = Size;
        pool->queueSize = 0; 
        pool->queueFront = 0;
        pool->queueRear = 0;

        //线程池为就绪状态
        pool->shutdown = 0;

        //初始化互斥锁和条件变量
        if(pthread_mutex_init(&pool->mutexPool, NULL) != 0 || 
        pthread_mutex_init(&pool->mutexBuzyNum, NULL) != 0 ||
        pthread_cond_init(&pool->notFull, NULL) != 0 ||
        pthread_cond_init(&pool->notEmpty, NULL) != 0) {
            printf("mutex or condition creation fail...\n");
            break;
        } 

        //创建线程
        pthread_create(&pool->managerID, NULL, manager, pool);
        for(int i = 0; i < min; i++) {
            pthread_create(&pool->workerID, NULL, worker, pool);
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

//工作函数
void* worker(void* arg) {
    ThreadPool* pool = (ThreadPool*) arg;
    
    //循环访问任务队列
    while(1) {
        pthread_mutex_lock(&pool->mutexPool);
        //while的原因：若阻塞9个线程，但只有一个资源，那么唤醒一个线程，继续阻塞8个线程
        //阻塞条件：任务队列没有任务，并且pool还存在
        while(pool->queueSize == 0 && !pool->shutdown) {
            //阻塞工作线程
            pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
            //工作线程自杀
            if(pool->exitNum) {
                pool->exitNum--;
                pthread_mutex_unlock(&pool->mutexPool);
                pthread_exit(NULL);
            }
        }

        if(pool->shutdown) {
            pthread_mutex_unlock(&pool->mutexPool);
            pthread_exit(NULL);
        }

        //取出任务
        Task task;
        task.function = pool->taskQueue[pool->queueFront].function;
        task.arg = pool->taskQueue[pool->queueFront].arg;
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->queueSize--;

        pthread_mutex_unlock(&pool->mutexPool);

        //忙线程++
        printf("threadID xxx start working");
        pthread_mutex_lock(&pool->mutexBuzyNum);
        pool->buzyNum++;
        pthread_mutex_unlock(&pool->mutexBuzyNum);

        //调用任务,释放存在堆内存中的参数
        task.function(task.arg);
        free(task.arg);
        task.arg = NULL;

        printf("threadID xxx stop working");
        //忙线程--
        pthread_mutex_lock(&pool->mutexBuzyNum);
        pool->buzyNum++;
        pthread_mutex_unlock(&pool->mutexBuzyNum);
    }
    
}

//管理者线程
void* manager(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    while(pool->shutdown == 0) {
        //每隔三秒检测一次
        sleep(3);

        //获取任务个数、存活线程数
        pthread_mutex_lock(&pool->mutexPool);
        int queueSize = pool->queueSize;
        int liveNum = pool->liveNum;
        pthread_mutex_unlock(&pool->mutexPool);

        pthread_mutex_lock(&pool->mutexBuzyNum);
        int buzyNum = pool->buzyNum;
        pthread_mutex_unlock(&pool->mutexBuzyNum);

        //添加线程：每次添加CREATNUMBER个，
        //任务个数>存活线程个数 && 存活线程数 < 最大线程数
        int count = 0;
        if(queueSize > pool->liveNum && liveNum < pool->maxNum) {
            pthread_mutex_lock(&pool->mutexPool);
            //for循环中有pool->liveNum是互斥量，所以锁要上到for循环外部
            for(int i = 0; i < pool->maxNum && pool->liveNum < pool->maxNum && count < CREATENUMBER; i++) {
                if(pool->workerID[i] == 0) {
                    pthread_create(&pool->workerID[i], NULL, worker, pool);
                    count++;
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }

        //这里可能出现问题，如果同时存在添加线程和销毁线程的条件
        //销毁线程
        //忙线程*2<存活线程 && 存活线程 > 最小线程
        //每次删除KILLNUMBER个
        if(buzyNum * 2 < liveNum && liveNum > pool->minNum) {
            //工作线程根据KILLNUMBER来判定自己醒来时需不需要自杀
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum = KILLNUMBER;
            pthread_mutex_unlock(&pool->mutexPool);
            //每次唤醒KILLNUMBER个
            for(int i = 0; i < KILLNUMBER; i++) {
                pthread_cond_signal(&pool->notEmpty);
            }
        }
        //管理线程不是主动销毁线程，而是让工作线程去自杀

    }
    return NULL;
}