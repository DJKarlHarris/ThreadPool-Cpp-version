#ifndef _TASKQUEUE_H_
#define _TASKQUEUE_H_

#include <queue>
#include <pthread.h>

using callback = void (*)(void*);

//任务类
template<typename T>
struct Task {

    Task<T>() {
        m_function = nullptr;
        m_arg = nullptr;
    }

    Task<T>(callback function, void* arg) {
        m_function = function;
        m_arg = (T*)arg;
    }

    callback m_function;
    T* m_arg;
};

//任务队列类
template<typename T>
class TaskQueue {

public:
    TaskQueue();
    ~TaskQueue();

    //取出任务
    Task<T> getTask();
    //加入任务
    void addTask(Task<T> task);
    //重载addTask
    void addTask(callback function, void* arg);
    //获取任务总个数
    inline size_t taskNumber() {
        return m_queue.size();
    }

private:
    pthread_mutex_t m_mutex;
    std::queue<Task<T>> m_queue;
};

#endif