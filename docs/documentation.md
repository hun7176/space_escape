gcc 컴파일용
gcc -I/home/junhyung/project/space_escape/include -pthread \
    /home/junhyung/project/space_escape/src/main.c \
    /home/junhyung/project/space_escape/src/utils/thread_utils.c \
    /home/junhyung/project/space_escape/src/game/game.c \
    /home/junhyung/project/space_escape/src/game/intro_ui.c \
    -lncursesw -o space_escape