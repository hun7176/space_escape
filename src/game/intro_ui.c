#include <stdio.h>
#include "intro_ui.h"
#include "game_parameter.h"
#include "game.h"
#include "network.h"
#include "common.h"


LobbyRoom lobby = {0};


void init_colors() {
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);   // 기본 텍스트
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // 로고/제목
    init_pair(3, COLOR_CYAN, COLOR_BLACK);    // 설명 텍스트
    init_pair(4, COLOR_BLACK, COLOR_YELLOW);  // 선택된 메뉴 강조
}

NetworkLobbyRoom get_lobby_status() {
    NetworkLobbyRoom lobby_copy;
    pthread_mutex_lock(&network_mutex);
    lobby_copy = network_lobby;
    pthread_mutex_unlock(&network_mutex);
    return lobby_copy;
}

int is_game_ready_to_start() {
    pthread_mutex_lock(&network_mutex);
    int ready = network_lobby.game_started;
    pthread_mutex_unlock(&network_mutex);
    return ready;
}


int show_main_menu() {
    const char *menu[] = {
        ">  1인 모드 시작",
        ">  2인 모드 시작",
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
                if (choice == 0){
                    current_game_mode = GAME_MODE_SINGLE;
                    return GAME_MODE_SINGLE;
                } // 1인 모드 시작
                else if (choice == 1) { //멀티모드 선택시 2인 로비로 진입
                    current_game_mode = GAME_MODE_MULTI;

                    return GAME_MODE_MULTI;
                    // if(show_multiplayer_lobby() == 1){
                    // }
                    // mvprintw(base_y + 14, center_x - 18, "2인 모드는 준비 중입니다.");
                    // refresh(); usleep(1000000);

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
//2인 모드 로비 화면
int show_multiplayer_lobby() {
    //서버에 로비 참가 요청
    if (send_lobby_join_request() != 0) {
        mvprintw(SCREEN_ROWS/2, SCREEN_COLS/2 - 15, "서버 연결 실패. 메인 메뉴로 돌아갑니다.");
        refresh();
        usleep(2000000);
        return 0;
    }
    
    //더블 버퍼링을 위한 백그라운드 윈도우 생성
    WINDOW *back_buffer = newwin(SCREEN_ROWS, SCREEN_COLS, 0, 0);
    if (back_buffer == NULL) {
        mvprintw(SCREEN_ROWS/2, SCREEN_COLS/2 - 10, "윈도우 생성 실패");
        refresh();
        usleep(1000000);
        return 0;
    }
    
    int my_ready = 0;  //내 준비 상태
    
    timeout(100);
    
    while (1) {
        //백그라운드 버퍼 초기화
        werase(back_buffer);
        
        //네트워크에서 현재 로비 상태 가져오기
        NetworkLobbyRoom current_lobby = get_lobby_status();
        
        //게임 시작 확인
        if (is_game_ready_to_start()) {
            werase(back_buffer);
            
            for (int x = 0; x < SCREEN_COLS; x++) {
                mvwaddch(back_buffer, 0, x, '#');
                mvwaddch(back_buffer, SCREEN_ROWS - 1, x, '#');
            }
            for (int y = 0; y < SCREEN_ROWS; y++) {
                mvwaddch(back_buffer, y, 0, '#');
                mvwaddch(back_buffer, y, SCREEN_COLS - 1, '#');
            }
            
            int center_x = SCREEN_COLS / 2;
            int center_y = SCREEN_ROWS / 2;
            
            wattron(back_buffer, COLOR_PAIR(2));
            mvwprintw(back_buffer, center_y, center_x - 10, "게임 시작!");
            wattroff(back_buffer, COLOR_PAIR(2));
            
            overwrite(back_buffer, stdscr);
            refresh();
            
            delwin(back_buffer);
            usleep(1000000);
            return 1;  //게임 시작
        }
        
        
        for (int x = 0; x < SCREEN_COLS; x++) {
            mvwaddch(back_buffer, 0, x, '#');
            mvwaddch(back_buffer, SCREEN_ROWS - 1, x, '#');
        }
        for (int y = 0; y < SCREEN_ROWS; y++) {
            mvwaddch(back_buffer, y, 0, '#');
            mvwaddch(back_buffer, y, SCREEN_COLS - 1, '#');
        }
        
        int center_x = SCREEN_COLS / 2;
        int base_y = SCREEN_ROWS / 2 - 10;
        
        wattron(back_buffer, COLOR_PAIR(2));
        mvwprintw(back_buffer, base_y, center_x - 10, "=== 2인 모드 로비 ===");
        wattroff(back_buffer, COLOR_PAIR(2));
        
        mvwprintw(back_buffer, base_y + 3, center_x - 15, "대기 중인 플레이어: %d/2", current_lobby.player_count);
        
        for (int i = 0; i < 2; i++) {
            if (i < current_lobby.player_count && current_lobby.players[i].connected) {
                if (current_lobby.players[i].ready) {
                    wattron(back_buffer, COLOR_PAIR(2));
                    mvwprintw(back_buffer, base_y + 5 + i, center_x - 15, "Player %d: %s [준비완료]", 
                            i + 1, current_lobby.players[i].name);
                    wattroff(back_buffer, COLOR_PAIR(2));
                } else {
                    wattron(back_buffer, COLOR_PAIR(3));
                    mvwprintw(back_buffer, base_y + 5 + i, center_x - 15, "Player %d: %s [대기중]", 
                            i + 1, current_lobby.players[i].name);
                    wattroff(back_buffer, COLOR_PAIR(3));
                }
            } else {
                wattron(back_buffer, COLOR_PAIR(3));
                mvwprintw(back_buffer, base_y + 5 + i, center_x - 15, "Player %d: 빈 슬롯", i + 1);
                wattroff(back_buffer, COLOR_PAIR(3));
            }
        }
        
        mvwprintw(back_buffer, base_y + 9, center_x - 15, "[R] 준비/준비해제");
        mvwprintw(back_buffer, base_y + 10, center_x - 15, "[ESC] 메인 메뉴로 돌아가기");
        
        //내 준비 상태 표시
        if (my_ready) {
            wattron(back_buffer, COLOR_PAIR(2));
            mvwprintw(back_buffer, base_y + 12, center_x - 10, "준비 완료!");
            wattroff(back_buffer, COLOR_PAIR(2));
        } else {
            wattron(back_buffer, COLOR_PAIR(3));
            mvwprintw(back_buffer, base_y + 12, center_x - 10, "준비 대기중...");
            wattroff(back_buffer, COLOR_PAIR(3));
        }
        
        overwrite(back_buffer, stdscr);
        refresh();
        
        int ch = getch();
        
        switch (ch) {
            case 'r':
            case 'R':
                my_ready = !my_ready;
                send_ready_status(my_ready);
                break;
            case 27:  //ESC
                send_lobby_leave_request();
                delwin(back_buffer);  //윈도우 정리
                return 0;  //메인 메뉴로
            case ERR:

                update_lobby_status();
                break;
            default:
                //다른 키 입력 시에도 상태 업데이트
                update_lobby_status();
                break;
        }
    }

    delwin(back_buffer);
    return 0;
}