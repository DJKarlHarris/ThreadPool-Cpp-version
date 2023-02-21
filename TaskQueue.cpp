#include "TaskQueue.h"
#include <pthread.h>

TaskQueue::TaskQueue(Task task) {
    pthread_mutex_init(&m_mutex, NULL);
}

TaskQueue::~TaskQueue() {
    pthread_mutex_destroy(&m_mutex);
}

//取出任务
Task TaskQueue::getTask() {
    Task task;
    if(!m_queue.empty()) {
        pthread_mutex_lock(&m_mutex);
        task = m_queue.front();
        m_queue.pop();
        pthread_mutex_unlock(&m_mutex);
    }
    return task;
}

//加入任务
void TaskQueue::addTask(Task task) {
    pthread_mutex_lock(&m_mutex);
    m_queue.push(task);
    pthread_mutex_unlock(&m_mutex);
}

void TaskQueue::addTask(callback function, void* arg) {
    pthread_mutex_lock(&m_mutex);
    m_queue.push(Task(function, arg));
    pthread_mutex_unlock(&m_mutex);
}
