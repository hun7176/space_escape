# 컴파일러 및 플래그
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -pthread
LDFLAGS = -lncursesw  # -lwiringPi 필요시 여기에 추가

# 서버 소스 및 오브젝트
SERVER_SRC = server/server.c
SERVER_OBJ = build/server/server.o
SERVER_TARGET = server_app

# 실행 파일 이름
TARGET = space_escape



# 소스 파일 수동 명시 (src/ 하위 디렉토리까지)
SRC = \
	src/main.c \
	src/controller/controller.c \
	src/controller/filter.c \
	src/game/game.c \
	src/game/intro_ui.c \
	src/network/network.c \
	src/utils/thread_utils.c

# 오브젝트 파일 경로 매핑
OBJ = $(patsubst %.c, build/%.o, $(SRC))

# 기본 타겟
all: $(TARGET)

# 실행 파일 생성
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 서버 실행 파일 생성
$(SERVER_TARGET): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -pthread

# 오브젝트 빌드 규칙 (하위 디렉토리 포함)
build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 정리
clean:
	rm -rf build $(TARGET) $(SERVER_TARGET)

.PHONY: all clean