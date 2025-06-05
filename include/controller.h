#ifndef CONTROLLER_H
#define CONTROLLER_H

void* imu_thread_func(void* arg);
int init_adxl345();
extern char imu_direction[3];
extern pthread_mutex_t imu_lock;
extern pthread_mutex_t switch_lock;

#endif
