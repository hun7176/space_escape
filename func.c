#include <stdio.h>
#include "func.h"
#include "pre_defined.h"

void init_colors() {
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);   // 기본 텍스트
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // 로고/제목
    init_pair(3, COLOR_CYAN, COLOR_BLACK);    // 설명 텍스트
    init_pair(4, COLOR_BLACK, COLOR_YELLOW);  // 선택된 메뉴 강조
}


void show_main_menu() {
    const char *menu[] = {
        ">  1인 모드 시작",
        ">  2인 모드 (미지원)",
        ">  만든이",
        ">  종료"
    };
    int choice = 0;
    int menu_size = sizeof(menu) / sizeof(menu[0]);

    while (1) {
        clear();

        // 전체 그리드 테두리 출력 (90x42)
        for (int x = 0; x < SCREEN_COLS; x++) {
            mvaddch(0, x, '#');
            mvaddch(SCREEN_ROWS - 1, x, '#');
        }
        for (int y = 0; y < SCREEN_ROWS; y++) {
            mvaddch(y, 0, '#');
            mvaddch(y, SCREEN_COLS - 1, '#');
        }

        // 가운데 기준 좌표 계산
        int center_x = SCREEN_COLS / 2;
        int base_y = SCREEN_ROWS / 2 - 8;

        attron(COLOR_PAIR(2));
        mvprintw(base_y - 2, center_x - 16, "+-----------------------------+");
        mvprintw(base_y - 1, center_x - 16, "|       SPACE ESCAPE          |");
        mvprintw(base_y + 0, center_x - 16, "|      with controller        |");
        mvprintw(base_y + 1, center_x - 16, "+-----------------------------+");
        attroff(COLOR_PAIR(2));

        for (int i = 0; i < menu_size; i++) {
            if (i == choice) {
                attron(COLOR_PAIR(4));
                mvprintw(base_y + 3 + i, center_x - 10, "%s", menu[i]);
                attroff(COLOR_PAIR(4));
            } else {
                attron(COLOR_PAIR(1));
                mvprintw(base_y + 3 + i, center_x - 10, "%s", menu[i]);
                attroff(COLOR_PAIR(1));
            }
        }

        attron(COLOR_PAIR(3));
        mvprintw(base_y + 9, center_x - 18, "↑↓ 방향키로 이동, Enter로 선택");
        mvprintw(base_y + 11, center_x - 18, "게임 시작 전 좌우 키로 캐릭터 위치 조정 가능");
        mvprintw(base_y + 12, center_x - 18, "→ 준비되었으면 Enter 키를 눌러 시작!");
        attroff(COLOR_PAIR(3));

        int ch = getch();
        switch (ch) {
            case KEY_UP:
                choice = (choice - 1 + menu_size) % menu_size;
                break;
            case KEY_DOWN:
                choice = (choice + 1) % menu_size;
                break;
            case KEY_LEFT:
                if (choice == 0 && player_x > 1) player_x--;
                break;
            case KEY_RIGHT:
                if (choice == 0 && player_x < WIDTH - 2) player_x++;
                break;
            case '\n':
            case ' ':
                if (choice == 0) return;  // 1인 모드 시작
                else if (choice == 1) {
                    mvprintw(base_y + 14, center_x - 18, "2인 모드는 준비 중입니다.");
                    refresh(); usleep(1000000);
                } else if (choice == 2) {
                    mvprintw(base_y + 14, center_x - 18, "제작자: 전준형 (경북대 컴퓨터학부)");
                    mvprintw(base_y + 15, center_x - 18, "제작자: 최승헌 (경북대 전자공학부)");
                    refresh(); usleep(3000000);
                } else if (choice == 3) {
                    endwin();
                    printf("게임을 종료합니다.\n");
                    exit(0);
                }
                break;
        }
        refresh();
    }
}