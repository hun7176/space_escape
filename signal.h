#ifndef SIGNAL_H
#define SIGNAL_H
#include <stdio.h>
#include "pre_defined.h"
extern int score;

void func_pause(int sig) {
    signal(SIGTSTP, SIG_IGN);
    int mid_x = WIDTH / 2;
    int mid_y = HEIGHT / 2;
    mvprintw(mid_y - 1, mid_x - 7, "== PAUSED ==");
    mvprintw(mid_y, mid_x - 15, "Press 'p' or Ctrl+Z to resume");
    refresh();

    timeout(-1);
    while (1) {
        int ch = getch();
        if (ch == 'p' || ch == 'P' || ch == 26) break;
    }
    timeout(50);
    signal(SIGTSTP, func_pause);
}

void end_by_signal(int sig) {
    int mid_x = WIDTH / 2;
    int mid_y = HEIGHT / 2;
    mvprintw(mid_y, mid_x - 18, "Are you sure you want to quit? (y/n)");
    refresh();

    timeout(-1);
    int ch = getch();
    timeout(50);

    if (ch == 'y' || ch == 'Y') {
        endwin();
        printf("Terminated by user. Final Score: %d\n", score);
        exit(0);
    }
}

#endif
