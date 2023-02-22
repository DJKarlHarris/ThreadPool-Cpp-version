#include "TaskQueue.h"
#include <pthread.h>

template<typename T>
TaskQueue<T>::TaskQueue() {
    pthread_mutex_init(&m_mutex, NULL);
}

template<typename T>
TaskQueue<T>::~TaskQueue() {
    pthread_mutex_destroy(&m_mutex);
}

//取出任务
template<typename T>
Task<T> TaskQueue<T>::getTask() {
    Task<T> task;
    if(!m_queue.empty()) {
        pthread_mutex_lock(&m_mutex);
        task = m_queue.front();
        m_queue.pop();
        pthread_mutex_unlock(&m_mutex);
    }
    return task;
}

//加入任务
template<typename T>
void TaskQueue<T>::addTask(Task<T> task) {
    pthread_mutex_lock(&m_mutex);
    m_queue.push(task);
    pthread_mutex_unlock(&m_mutex);
}

template<typename T>
void TaskQueue<T>::addTask(callback function, void* arg) {
    pthread_mutex_lock(&m_mutex);
    m_queue.push(Task<T>(function, arg));
    pthread_mutex_unlock(&m_mutex);
}
