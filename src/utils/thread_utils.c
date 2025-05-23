#include "thread_utils.h"
#include <pthread.h>
#include <stdio.h>

void create_thread(pthread_t* tid, void* (*func)(void*), void* arg) {
    if (pthread_create(tid, NULL, func, arg)) {
        perror("Thread creation failed");
    }
}

void join_thread(pthread_t tid) {
    pthread_join(tid, NULL);
}
