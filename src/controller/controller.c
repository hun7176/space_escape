#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include "controller.h"
#define DEVICE "/dev/i2c-1"
#define ADXL345_ADDR 0x53

#define POWER_CTL 0x2D
#define DATAX0    0x32
#define SCREEN_ROWS 42
#define SCREEN_COLS 90
#define THRESHOLD 100  // 노이즈 방지용 임계값

int init_adxl345() {
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C bus");
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE, ADXL345_ADDR) < 0) {
        perror("Failed to connect to sensor");
        close(fd);
        return -1;
    }

    char config[2] = {POWER_CTL, 0x08};
    if (write(fd, config, 2) != 2) {
        perror("Failed to write POWER_CTL");
        close(fd);
        return -1;
    }

    return fd;
}

void* imu_thread_func(void* arg) {
    int fd = *(int*)arg;
    char config[2] = {POWER_CTL, 0x08};
    write(fd, config, 2);

    while (1) {
        char reg = DATAX0;
        if (write(fd, &reg, 1) != 1) continue;

        char data[6];
        if (read(fd, data, 6) != 6) continue;

        int16_t x = (data[1] << 8) | data[0];
        int16_t y = (data[3] << 8) | data[2];

        char dir = '5';  // 기본은 정지

        if (abs(x) < THRESHOLD && abs(y) < THRESHOLD) {
            dir = '5';
        } else {
            if (y > THRESHOLD) {
                if (x < -THRESHOLD) dir = '9';
                else if (x > THRESHOLD) dir = '7';
                else dir = '8';
            } else if (y < -THRESHOLD) {
                if (x < -THRESHOLD) dir = '3';
                else if (x > THRESHOLD) dir = '1';
                else dir = '2';
            } else {
                if (x < -THRESHOLD) dir = '6';
                else if (x > THRESHOLD) dir = '4';
            }
        }
        printf("%c\n",dir);
        // 방향 값을 뮤텍스로 보호하며 저장
        pthread_mutex_lock(&imu_lock);
        imu_direction = dir;
        pthread_mutex_unlock(&imu_lock);

        usleep(100000);  // 0.1초
    }

    return NULL;
}
