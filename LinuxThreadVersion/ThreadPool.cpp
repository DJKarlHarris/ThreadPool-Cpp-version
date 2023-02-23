#include "ThreadPool.h"
#include "TaskQueue.h"
#include "TaskQueue.cpp"
#include <string.h>
#include <iostream>
#include <unistd.h>

template<typename T>
ThreadPool<T>::ThreadPool(int min, int max) {

    do {
        //创建工作线程数组
        m_workerID = new pthread_t[max];

        memset(m_workerID, 0, sizeof(pthread_t) * max);
        m_minNum = min;
        m_maxNum = max;
        m_busyNum = 0;
        m_liveNum = min;    // 和最小个数相等
        m_exitNum = 0;

        m_taskqueue = new TaskQueue<T>();

        //创建互斥锁
        if (pthread_mutex_init(&m_mutex, NULL) != 0 ||
            pthread_cond_init(&m_notEmpty, NULL) != 0) {
            std::cout << "mutex or condition init fail...\n" << std::endl;
            break;
        }

        // 创建线程,传递this指针：静态方法只能访问静态变量，要想访问非静态变量，就要传入对象指针
        pthread_create(&m_managerID, NULL, manager, this);
        for (int i = 0; i < min; ++i) {
            pthread_create(&m_workerID[i], NULL, worker, this);
        }

        return;
    } while (0);

    // 释放资源
    if (m_workerID) {
        delete[] m_workerID;
    } 

    if(m_taskqueue) {
        delete m_taskqueue;
    }

    return;
}


template<typename T>
ThreadPool<T>::~ThreadPool() {

    // 关闭线程池
    shutdown = true;

    // 阻塞回收管理者线程
    pthread_join(m_managerID, NULL);

    // 唤醒阻塞的消费者线程
    for (int i = 0; i < m_liveNum; ++i) {
        pthread_cond_signal(&m_notEmpty);
    }

    //避免线程池在线程之前销毁，所以要join来回收所有线程资源
    for(int i = 0; i < m_maxNum; i++) {
        if(m_workerID[i] != 0) {
            pthread_join(m_workerID[i],NULL);
        }
    }

    // 释放堆内存
    delete m_taskqueue;
    delete[] m_workerID;

    //销毁锁和条件变量
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_notEmpty);

}

//添加任务api
template<typename T>
void ThreadPool<T>::addTask(Task<T> task) {

    if (shutdown) {
        pthread_mutex_unlock(&m_mutex);
        return;
    }
    
    // 添加任务
    m_taskqueue->addTask(task);

    pthread_cond_signal(&m_notEmpty);
}

//添加任务api
template<typename T>
void ThreadPool<T>::addTask(callback function, void* arg) {

    if (shutdown) {
        pthread_mutex_unlock(&m_mutex);
        return;
    }
    
    // 添加任务
    m_taskqueue->addTask(function, arg);

    pthread_cond_signal(&m_notEmpty);
}

template<typename T>
int ThreadPool<T>::getBusyNum() {
    pthread_mutex_lock(&m_mutex);
    int buzyNum = m_busyNum; 
    pthread_mutex_unlock(&m_mutex);
    return buzyNum;
}

template<typename T>
int ThreadPool<T>::getAliveNum() {
    pthread_mutex_lock(&m_mutex);
    int liveNum = m_liveNum; 
    pthread_mutex_unlock(&m_mutex);
    return liveNum;
}

template<typename T>
void* ThreadPool<T>::worker(void* arg) {

    ThreadPool* pool = static_cast<ThreadPool*>(arg);

    while (true) {
        pthread_mutex_lock(&pool->m_mutex);
        // 当前任务队列是否为空
        while (pool->m_taskqueue->taskNumber() == 0 && !pool->shutdown) {

            // 阻塞工作线程
            pthread_cond_wait(&pool->m_notEmpty, &pool->m_mutex);

            // 判断是不是要销毁线程
            if (pool->m_exitNum > 0) {
                pool->m_exitNum--;
                if (pool->m_liveNum > pool->m_minNum) {
                    pool->m_liveNum--;
                    pthread_mutex_unlock(&pool->m_mutex);
                    pool->threadExit();
                }
            }
        }

        // 判断线程池是否被关闭了
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->m_mutex);
            pool->threadExit();
        }

        // 从任务队列中取出一个任务
        Task<T> task = pool->m_taskqueue->getTask();
        pool->m_busyNum++;

        //解锁
        pthread_mutex_unlock(&pool->m_mutex);

        std::cout <<  "threadID: " << std::to_string(pthread_self()) << " start working...\n" ;

        task.m_function(task.m_arg);
        delete task.m_arg;
        task.m_arg = NULL;

        std::cout << "threadID: " << std::to_string(pthread_self()) << " end working...\n" ;

        pthread_mutex_lock(&pool->m_mutex);
        pool->m_busyNum--;
        pthread_mutex_unlock(&pool->m_mutex);
    }
}

template<typename T>
void* ThreadPool<T>::manager(void* arg) {
    
        ThreadPool* pool = static_cast<ThreadPool*>(arg);
    while (!pool->shutdown) {
        // 每隔3s检测一次
        sleep(3);

        // 取出线程池中任务的数量和当前线程的数量
        pthread_mutex_lock(&pool->m_mutex);
        int taskNumber = pool->m_taskqueue->taskNumber();
        int liveNum = pool->m_liveNum;
        int busyNum = pool->m_busyNum;
        pthread_mutex_unlock(&pool->m_mutex);
        
        // 添加线程
        // 任务的个数>存活的线程个数 && 存活的线程数<最大线程数
        if (taskNumber > liveNum && liveNum < pool->m_maxNum) {
            pthread_mutex_lock(&pool->m_mutex);
            int counter = 0;
            for (int i = 0; i < pool->m_maxNum && 
            counter < CREATENUMBER &&
            pool->m_liveNum < pool->m_maxNum; ++i) {
                if (pool->m_workerID[i] == 0) {
                    pthread_create(&pool->m_workerID[i], NULL, worker, pool);
                    counter++;
                    pool->m_liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->m_mutex);
        }

        // 销毁线程
        // 忙的线程*2 < 存活的线程数 && 存活的线程>最小线程数
        if (busyNum * 2 < liveNum && liveNum > pool->m_minNum) {
            pthread_mutex_lock(&pool->m_mutex);
            pool->m_exitNum = KILLNUMBER;
            pthread_mutex_unlock(&pool->m_mutex);
            // 让工作的线程自杀
            for (int i = 0; i < KILLNUMBER; ++i) {
                pthread_cond_signal(&pool->m_notEmpty);
            }
        }
    }
    return NULL;

}

//线程退出函数
template<typename T>
void ThreadPool<T>::threadExit() {
     pthread_t tid = pthread_self();
    for (int i = 0; i < m_maxNum; ++i) {
        if (m_workerID[i] == tid) {
            std::cout << "threadExit() -> threadID " << std::to_string(tid) << " exiting...\n";
            m_workerID[i] = 0;
            break;
        }
    }
    pthread_exit(NULL);
}
