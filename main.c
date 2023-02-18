#include "threadpool.h"
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

void taskFunc(void* arg) {
    int num = *(int*)arg;
    printf("threadID %ld is working,Now number = %d \n",pthread_self(),num);
    usleep(1);
}

int main() {
    ThreadPool* pool = threadPoolCreate(3, 10, 100);
    for(int i = 0; i <100; i++) {
        int* num = (int*)malloc(sizeof(int));
        *num = 100 + i;
        //printf("%d \n",*num);
        threadPoolAddTask(pool, taskFunc, num);
    }


    //等待工作线程处理完毕
    sleep(3);

    threadPoolDestory(pool);
    return 0;
}