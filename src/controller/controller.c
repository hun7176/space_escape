#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <math.h>

#define DEVICE "/dev/i2c-1"
#define ADXL345_ADDR 0x53

#define POWER_CTL 0x2D
#define DATAX0    0x32

#define THRESHOLD 100  // 노이즈 방지용 임계값

int main() {
    int fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C bus");
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, ADXL345_ADDR) < 0) {
        perror("Failed to connect to sensor");
        close(fd);
        return 1;
    }

    // 센서 측정 모드 활성화
    char config[2] = {POWER_CTL, 0x08};
    if (write(fd, config, 2) != 2) {
        perror("Failed to write POWER_CTL");
        close(fd);
        return 1;
    }

    while (1) {
        char reg = DATAX0;
        if (write(fd, &reg, 1) != 1) {
            perror("Failed to set data address");
            break;
        }

        char data[6];
        if (read(fd, data, 6) != 6) {
            perror("Failed to read data");
            break;
        }

        int16_t x = (data[1] << 8) | data[0];
        int16_t y = (data[3] << 8) | data[2];

        printf("X: %6d, Y: %6d -> ", x, y);

        // 방향 결정 (좌우 반전)
        if (abs(x) < THRESHOLD && abs(y) < THRESHOLD) {
            printf("5 (정지)\n");
        } else {
            if (y > THRESHOLD) {
                if (x < -THRESHOLD)
                    printf("9 (우상)\n");
                else if (x > THRESHOLD)
                    printf("7 (좌상)\n");
                else
                    printf("8 (상)\n");
            } else if (y < -THRESHOLD) {
                if (x < -THRESHOLD)
                    printf("3 (우하)\n");
                else if (x > THRESHOLD)
                    printf("1 (좌하)\n");
                else
                    printf("2 (하)\n");
            } else {
                if (x < -THRESHOLD)
                    printf("6 (우)\n");
                else if (x > THRESHOLD)
                    printf("4 (좌)\n");
            }
        }

        usleep(500000);
    }

    close(fd);
    return 0;
}

