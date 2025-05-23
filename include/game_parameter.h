#ifndef GAME_PARAMETER_H
#define GAME_PARAMETER_H

#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define WIDTH 60
#define HEIGHT 40
#define MAX_BULLETS 10
#define MAX_OBSTACLES 20

#define SCREEN_ROWS 42
#define SCREEN_COLS 90

extern int player_x;
extern int player_y;

typedef struct {
    int x, y;
    int active;
} Bullet;

typedef struct {
    int x, y;
    int active;
} Obstacle;

#endif
