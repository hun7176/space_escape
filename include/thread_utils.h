#ifndef THREAD_UTILS_H
#define THREAD_UTILS_H

#include <pthread.h>

void create_thread(pthread_t* thread, void* (*func)(void*), void* arg);
void join_thread(pthread_t thread);



#endif // THREAD_UTILS_H
