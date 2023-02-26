#include "ThreadPool.h"
#include "ThreadPool.cpp"
#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <string>

void taskFunc(void* arg) {
    int num = *(int*)arg;
    std::cout << "threadID " << std::to_string(pthread_self()) << " is working , Now number is " << num <<std::endl;
    usleep(10);
}

int main() {
    ThreadPool<int> pool = ThreadPool<int>(3, 10);
    for(int i = 0; i < 1000; i++) {
        int* num = new int(i);
        //printf("%d \n",*num);
        Task<int> task = Task<int>(taskFunc, num);
        pool.addTask(task);
        //pool.addTask(taskFunc, num);
    }

    //等待工作线程处理完毕
    sleep(3);
    
    return 0;
}