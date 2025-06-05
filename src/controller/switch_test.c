#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <gpiod.h>  // gpiod 라이브러리 추가

#define DEVICE "/dev/i2c-1"
#define ADXL345_ADDR 0x53

#define POWER_CTL 0x2D
#define DATAX0    0x32
#define THRESHOLD 100  // 노이즈 방지용 임계값

#define SWITCH_PIN1 4   // GPIO 4번을 첫 번째 버튼으로 사용
#define SWITCH_PIN2 14  // GPIO 14번을 두 번째 버튼으로 사용

// ADXL345 초기화 함수
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

    char config[2] = {POWER_CTL, 0x08};  // Power control register 설정 (측정 모드로 설정)
    if (write(fd, config, 2) != 2) {
        perror("Failed to write POWER_CTL");
        close(fd);
        return -1;
    }

    return fd;
}

int main() {
    // gpiod 라이브러리 초기화
    struct gpiod_chip *chip;
    struct gpiod_line *line1, *line2;
    
    // GPIO 칩 열기 (보통 gpiochip0을 사용)
    chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        perror("Failed to open the GPIO chip");
        return -1;
    }

    // GPIO 핀 설정
    line1 = gpiod_chip_get_line(chip, SWITCH_PIN1);
    line2 = gpiod_chip_get_line(chip, SWITCH_PIN2);
    
    if (!line1 || !line2) {
        perror("Failed to get lines");
        gpiod_chip_close(chip);
        return -1;
    }

    // GPIO를 입력으로 설정
    if (gpiod_line_request_input(line1, "gpio_test") < 0 || gpiod_line_request_input(line2, "gpio_test") < 0) {
        perror("Failed to request input lines");
        gpiod_chip_close(chip);
        return -1;
    }

    // ADXL345 초기화
    int fd = init_adxl345();
    if (fd < 0) {
        gpiod_chip_close(chip);
        return -1;
    }

    while (1) {
        // IMU 센서로부터 데이터 읽기
        char reg = DATAX0;
        if (write(fd, &reg, 1) != 1) continue;

        char data[6];
        if (read(fd, data, 6) != 6) continue;

        int16_t x = (data[1] << 8) | data[0];
        int16_t y = (data[3] << 8) | data[2];

        printf("IMU Data - X: %d, Y: %d\n", x, y);

        // 스위치 상태 확인
        int value1 = gpiod_line_get_value(line1);
        int value2 = gpiod_line_get_value(line2);

        if (value1 == 0) {  // 첫 번째 스위치가 눌린 경우
            printf("Switch 1 pressed!\n");
        }

        if (value2 == 0) {  // 두 번째 스위치가 눌린 경우
            printf("Switch 2 pressed!\n");
        }

        usleep(100000);  // 0.1초 대기
    }

    // 리소스 해제
    gpiod_chip_close(chip);
    close(fd);
    return 0;
}
