#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include <gpiod.h>  // gpiod 라이브러리 추가
#include "controller.h"

#define DEVICE "/dev/i2c-1"
#define ADXL345_ADDR 0x53

#define POWER_CTL 0x2D
#define DATAX0    0x32
#define SCREEN_ROWS 42
#define SCREEN_COLS 90
#define THRESHOLD 70  // 노이즈 방지용 임계값 원래 100

#define SWITCH_PIN1 4   // GPIO 4번을 첫 번째 버튼으로 사용
#define SWITCH_PIN2 14  // GPIO 14번을 두 번째 버튼으로 사용

// 전역 변수
int switch1_state = 1, switch2_state = 1;  // 스위치 상태 초기화

// ADXL345 초기화 함수
int init_adxl345() {
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        //perror("Failed to open I2C bus");
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE, ADXL345_ADDR) < 0) {
        perror("Failed to connect to sensor");
        close(fd);
        return -1;
    }

    char config[2] = {POWER_CTL, 0x08};  // Power control register 설정 (측정 모드로 설정)
    if (write(fd, config, 2) != 2) {
        perror("Failed to write POWER_CTL");
        close(fd);
        return -1;
    }

    return fd;
}

// IMU와 스위치 데이터를 읽고 처리하는 함수
void* imu_thread_func(void* arg) {
    int fd = *(int*)arg;
    struct gpiod_chip *chip;
    struct gpiod_line *line1, *line2;
    
    // GPIO 칩 열기 (보통 gpiochip0을 사용)
    chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        perror("Failed to open the GPIO chip");
        return NULL;
    }

    // GPIO 핀 설정
    line1 = gpiod_chip_get_line(chip, SWITCH_PIN1);
    line2 = gpiod_chip_get_line(chip, SWITCH_PIN2);
    
    if (!line1 || !line2) {
        perror("Failed to get lines");
        gpiod_chip_close(chip);
        return NULL;
    }

    // GPIO를 입력으로 설정
    if (gpiod_line_request_input(line1, "gpio_test") < 0 || gpiod_line_request_input(line2, "gpio_test") < 0) {
        perror("Failed to request input lines");
        gpiod_chip_close(chip);
        return NULL;
    }

    while (1) {
        // IMU 센서로부터 데이터 읽기
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

        // 방향 값을 뮤텍스로 보호하며 저장
        pthread_mutex_lock(&imu_lock);
        imu_direction[0] = dir;  // IMU 방향 정보 업데이트
        pthread_mutex_unlock(&imu_lock);

        // 스위치 상태 확인
        int value1 = gpiod_line_get_value(line1);
        int value2 = gpiod_line_get_value(line2);

        pthread_mutex_lock(&switch_lock); // 공유 자원 보호
        switch1_state = value1;
        switch2_state = value2;

        // 스위치 정보를 IMU 방향에 추가
        if (switch1_state == 0) imu_direction[1] = '1';  // 스위치 1 눌렸을 때 '1'
	else imu_direction[1] = '0';
        if (switch2_state == 0) imu_direction[2] = '1';  // 스위치 2 눌렸을 때 '2'
        else imu_direction[2] = '0';
        pthread_mutex_unlock(&switch_lock); // 공유 자원 보호 종료

        usleep(100000);  // 0.1초 대기
    }

    // 리소스 해제
    gpiod_chip_close(chip);
    return NULL;
}
