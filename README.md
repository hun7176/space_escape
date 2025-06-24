
# 🚀 라즈베리파이 IMU 게임 컨트롤러  
**I2C, GPIO, 쓰레드, 소켓 프로그래밍 기반의 실시간 멀티플레이 '우주선 피하기' 게임**

![Language](https://img.shields.io/badge/C-100%25-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Raspberry%20Pi%203-green.svg)
![Sensor](https://img.shields.io/badge/Sensor-ADXL345-ff69b4.svg)

---

## 📌 프로젝트 개요

이 프로젝트는 ADXL345 IMU 센서와 GPIO 버튼을 이용한 **모션 기반 게임 컨트롤러**를 설계하고, 두 라즈베리파이 간 **TCP 소켓 통신**을 통해 실시간 경쟁 게임을 구현한 시스템입니다.  
커널 드라이버, 디바이스 파일 제어, 쓰레드, ncurses 그래픽 등 **시스템 프로그래밍의 핵심 요소**를 종합적으로 활용하였습니다.

## 🎯 목표 및 동기

- 실제 **디바이스 드라이버 및 시스템 콜 기반 프로그래밍**을 경험하고자 함
- 쓰레드와 소켓, I/O 등의 시스템 자원을 직접 활용하는 **임베디드 게임 시스템**을 제작
- "재미있고 직관적인 리얼타임 시스템" 구현

---

## 🕹️ 게임 설명: Space Escape

플레이어는 IMU 센서를 통해 우주선을 좌우로 조작하며 위에서 떨어지는 장애물(운석)을 피해야 합니다.  
또한 2인 멀티플레이 모드에서는 서로 점수를 겨루거나 협동하여 생존할 수 있습니다.

- **좌우 이동**: IMU 기울기
- **버튼 입력**: 일시정지, 아이템 사용 등  
- **게임 모드**:
  - 1인 경쟁 모드 (화면 분할)
  - 2인 협동 모드 (공동 화면)

---

## 🧩 시스템 구조

```plaintext
[ IMU / Button ] --> [ Device Driver (/dev/imu_dev) ]
                              |
                              v
                    [ User App - Controller Thread ]
                              |
                              v
       [ Game Engine ] <--> [ Socket Communication ] <--> [ Opponent ]
                              |
                              v
                        [ ncurses UI ]
```

- **IMU 입력**
  - 센서: ADXL345 (3축 가속도, I2C)
  - 드라이버: 커널 모듈 작성 → `/dev/imu_dev`로 사용자 접근 가능
- **버튼 입력**
  - GPIO 기반 주기적 상태 확인 (`polling` 방식)
- **멀티스레드 구조**
  - IMU/버튼 입력을 별도 스레드에서 비동기 처리
  - 메인 게임 로직은 입력 큐로부터 이벤트 수신
- **소켓 통신**
  - TCP 기반 클라이언트-서버 구조 (스레드마다 소켓 핸들)
  - 서버는 클라이언트 접속마다 스레드 생성

---

## ⚙️ 구현 기술 요약

| 기술 요소         | 설명 |
|------------------|------|
| **센서 제어**       | I2C 통신 / ADXL345 / `ioctl()` |
| **디바이스 드라이버** | `/dev/imu_dev` 생성 / read 기반 사용자 접근 |
| **버튼 이벤트**     | GPIO + `poll()` |
| **소켓 통신**       | `socket()`, `bind()`, `connect()`, `recv()`, `send()` |
| **스레드 처리**     | `pthread_create()`, `pthread_join()` 등 |
| **화면 출력**       | ncurses 기반 텍스트 UI / flicker 최소화 |
| **시스템콜 활용**   | 총 19종 시스템 콜 적용 |

---

## 📂 프로젝트 구조

```
space_escape/
├── include/              # 헤더 파일
├── src/
│   ├── server.c          # 서버 로직
│   ├── client.c          # 클라이언트 로직
│   ├── controller.c      # IMU + 버튼 처리
│   └── game.c            # 게임 로직
├── driver/
│   └── imu_driver.c      # 커널 드라이버
├── Makefile
└── README.md
```

---

## 🗓️ 버전 히스토리

| 버전 | 주요 변경사항 |
|------|----------------|
| v0.1.0 | 기본 게임 구현 및 UI 구성 시작 |
| v0.2.0 | 컨트롤러 테스트 / 난이도 조정 |
| v0.3.0 | 코드 리팩터링 및 구조 정리 |
| v0.4.0 | 소켓 통신 및 네트워크 구조 설계 |
| v0.5.0 | 서버 메시지 시스템 재설계 / 2인 대기방 구현 |
| v0.6.0 | UI 깜빡임 개선, 예외처리 강화 |
| v0.7.0 | 동기화 개선, 서버 이탈 처리 |
| v0.8.0 | 게임 시작 시 seed 공유로 상태 일관성 보장 |

---

## 🎬 시연 영상

### 🔹 1인 경쟁 모드  
[![1인 경쟁 모드](https://img.youtube.com/vi/-aqC0NLWS3E/0.jpg)](https://youtu.be/-aqC0NLWS3E)  
🔗 https://youtu.be/-aqC0NLWS3E

&nbsp;

### 🔹 2인 협동 모드  
[![2인 협동 모드](https://img.youtube.com/vi/F4oVLLpWjE0/0.jpg)](https://youtu.be/F4oVLLpWjE0)  
🔗 https://youtu.be/F4oVLLpWjE0


---

## 🖼️ 게임 소개 이미지

게임 개요 요약 (발표자료 중 슬라이드 1):

![게임 소개 슬라이드](./presentation_note/page-0010.jpg)


---

## 🙌 참여 및 문의

본 프로젝트는 **경북대학교 시스템 소프트웨어 강의**의 팀 과제로 수행되었습니다.  
기술 문의 및 협업 제안은 GitHub Issue 또는 Pull Request로 환영합니다.

---

© 2025 Team 5 - KNU ELEC462 System Programming
