#include "ThreadPool.h"
#include "TaskQueue.h"
#include "TaskQueue.cpp"
#include <string.h>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <mutex>

template<typename T>
ThreadPool<T>::ThreadPool(int min, int max) {

    do {
        //创建工作线程数组
        m_workerID = new std::thread[max];

        m_minNum = min;
        m_maxNum = max;
        m_busyNum = 0;
        m_liveNum = min;    // 和最小个数相等
        m_exitNum = 0;

        m_taskqueue = new TaskQueue<T>();

        //静态成员函数，无需创建对象即可使用
        //如果声明为静态成员,则需要将函数声明友元，这样的话有点麻烦
        // 创建线程,传递this指针：静态方法只能访问静态变量，要想访问非静态变量，就要传入对象指针
        m_managerID = std::thread(manager, this);
        for (int i = 0; i < min; ++i) {
            m_workerID[i] = std::thread(worker, this);
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
    m_managerID.join();

    // 唤醒阻塞的消费者线程
    for (int i = 0; i < m_liveNum; ++i) {
        m_notEmpty.notify_one();
    }

    for(int i = 0; i < m_maxNum ; i++) {
        if(m_workerID[i].joinable()) {
            m_workerID[i].join();
        }
    }

    // 释放堆内存
    delete m_taskqueue;
    delete[] m_workerID;

}

//添加任务api
template<typename T>
void ThreadPool<T>::addTask(Task<T> task) {

    if (shutdown) {
        return;
    }
    
    // 添加任务
    m_taskqueue->addTask(task);

    m_notEmpty.notify_one();
}

//添加任务api
template<typename T>
void ThreadPool<T>::addTask(callback function, void* arg) {

    if (shutdown) {
        return;
    }
    
    // 添加任务
    m_taskqueue->addTask(function, arg);

    m_notEmpty.notify_one();
}

template<typename T>
int ThreadPool<T>::getBusyNum() {
    std::lock_guard<std::mutex> lockGuard(m_mutex);
    return m_busyNum;
}

template<typename T>
int ThreadPool<T>::getAliveNum() {
    std::lock_guard<std::mutex> lockGuard(m_mutex);
    return m_liveNum;
}

template<typename T>
void ThreadPool<T>::worker(void* arg) {

    ThreadPool* pool = static_cast<ThreadPool*>(arg);

    while (true) {
        std::unique_lock<std::mutex> uniqueLock(pool->m_mutex);
        // 当前任务队列是否为空
        while (pool->m_taskqueue->taskNumber() == 0 && !pool->shutdown) {

            // 阻塞工作线程
            pool->m_notEmpty.wait(uniqueLock);

            // 判断是不是要销毁线程
            if (pool->m_exitNum > 0) {
                pool->m_exitNum--;
                if (pool->m_liveNum > pool->m_minNum) {
                    pool->m_liveNum--;
                    uniqueLock.unlock();
                    std::cout << "threadID:" << std::this_thread::get_id() << " is exiting\n";
                    return;
                }
            }
        }

        // 判断线程池是否被关闭了
        if (pool->shutdown) {
            uniqueLock.unlock();
            std::cout << "threadID:" << std::this_thread::get_id() << " is exiting\n";
            return;
        }

        // 从任务队列中取出一个任务
        Task<T> task = pool->m_taskqueue->getTask();
        pool->m_busyNum++;

        //解锁
        uniqueLock.unlock();

        std::cout <<  "threadID: " << std::this_thread::get_id() << " start working...\n" ;

        task.m_function(task.m_arg);
        delete task.m_arg;
        task.m_arg = NULL;

        std::cout << "threadID: " << std::this_thread::get_id() << " end working...\n" ;

        uniqueLock.lock();
        pool->m_busyNum--;
        uniqueLock.unlock();
    }
     
}

template<typename T>
void ThreadPool<T>::manager(void* arg) {
    
        ThreadPool* pool = static_cast<ThreadPool*>(arg);
    while (!pool->shutdown) {
        // 每隔3s检测一次
        sleep(3);
        int taskNumber, liveNum, busyNum;

        // 取出线程池中任务的数量和当前线程的数量
        {
            std::lock_guard<std::mutex> lockGuard(pool->m_mutex);
            taskNumber = pool->m_taskqueue->taskNumber();
            liveNum = pool->m_liveNum;
            busyNum = pool->m_busyNum;
        }
       
        // 添加线程
        // 任务的个数>存活的线程个数 && 存活的线程数<最大线程数
        if (taskNumber > liveNum && liveNum < pool->m_maxNum) {
            std::lock_guard<std::mutex> lockGuard(pool->m_mutex);
            int counter = 0;
            for (int i = 0; i < pool->m_maxNum && 
            counter < CREATENUMBER &&
            pool->m_liveNum < pool->m_maxNum; ++i) {
                if (!pool->m_workerID[i].joinable()) {
                    pool->m_workerID[i] = std::thread(worker, pool);
                    counter++;
                    pool->m_liveNum++;
                }
            }
        }

        // 销毁线程
        // 忙的线程*2 < 存活的线程数 && 存活的线程>最小线程数
        if (busyNum * 2 < liveNum && liveNum > pool->m_minNum) {
            {
                std::lock_guard<std::mutex> lockGuard(pool->m_mutex);
                pool->m_exitNum = KILLNUMBER;
            }
            // 让工作的线程自杀
            for (int i = 0; i < KILLNUMBER; ++i) {
                pool->m_notEmpty.notify_one();
            }
        }
    }
    return;

}