#ifndef _TASKQUEUE_H_
#define _TASKQUEUE_H_

#include <queue>
#include <pthread.h>

using callback = void (*)(void*);

//任务类
struct Task {

    Task() {
        m_function = nullptr;
        m_arg = nullptr;
    }

    Task(callback function, void* arg) {
        m_function = function;
        m_arg = arg;
    }

    callback m_function;
    void* m_arg;
};

//任务队列类
class TaskQueue {

    TaskQueue(Task task);
    ~TaskQueue();

    //取出任务
    Task getTask();
    //加入任务
    void addTask(Task task);
    //重载addTask
    void addTask(callback function, void* arg);
    //获取任务总个数
    inline int taskNumber() {
        return m_queue.size();
    }

private:
    pthread_mutex_t m_mutex;
    std::queue<Task> m_queue;
};

#endif