

# 🏥 Hospital Autonomous Wheelchair Robot  
> Qt 기반 호출 UI, ROS2 자율주행, STM32 센서 제어, 서버·DB 배차 로직을 통합한  
> **병원용 자율주행 휠체어 운영 시스템**
---

## 📌 1. 프로젝트 목표

- 병원에서 거동이 불편한 환자가 휠체어를 쉽게 호출하고 이동할 수 있도록 지원
- **외래 환자**는 **터치 키오스크(Qt)** 로 호출
- **입원 환자**는 간호사가 **간호사용(QT) 또는 터치 키오스크(Qt)** 로 호출
- 자율주행은 **LiDAR SLAM + Nav2** 로 수행
 
---
## 🎬 2. 시연 영상
*Nav2 기반 경로 이동, 호출 배차부터 도착까지 전체 흐름을 보여주는 데모 영상입니다.*
>  https://github.com/user-attachments/assets/8a61449f-51c5-4cb4-b58b-52134d1384b4

---

## 🧠 3. 시스템 아키텍처 

### 1) 전체 시스템 아키텍처 
<img width="787" height="539" alt="image" src="https://github.com/user-attachments/assets/56bccc3c-7ea7-4e55-9197-c0469b62460f" />



- **Application (Qt)**  
  - 관리자용 Qt / 외래환자용 Qt  / 간호사용 Qt 
- **Database (MariaDB/SQL)**  
  - `robot_status`, `call_queue`, `map_location` 등을 통해 상태 저장 및 배차 데이터 관리
- **Central Server (TCP, C)**  
  - 로봇 접속 관리, 상태 수집, 명령 전달, 배차 로직 수행
- **Platform (ROS2)**  
  - 로봇의 자율주행, 센서 토픽 처리, 서버-로봇 브리지 연동
- **Hardware**  
  - TurtleBot3 Burger + STM32U5 모듈(센서/디스플레이)


---


### 2) 다중 로봇 확장형 배차 구조 (robot_status 기반 최대 N대 운영)
<img width="656" height="551" alt="image" src="https://github.com/user-attachments/assets/cc0663b3-6a31-4001-9924-6ead65dee5dd" />





본 시스템은 **DB의 `robot_status` 테이블**을 중심으로 로봇을 관리합니다.

- 로봇 하드웨어가 추가 되다면
  - 서버는 `robot_status`의 `robot_id`(또는 name)를 기준으로 **로봇 호스트를 등록**
  - 로봇 상태를 주기적으로 갱신하고,
  - 호출 큐(`call_queue`)와 매칭해 **배차/명령 할당**을 수행합니다.
- 즉, **로봇이 늘어나도 서버/DB 구조는 동일**하며,
  - `robot_status` 레코드 수만 증가하는 형태로 **최대 100대 까지 확장 가능한 구조**를 목표로 설계했습니다.


---


### 3) STM32U5 + TouchGFX  연동 구조 (ROS 토픽 표시 + 센서 토픽 생성)

<img width="870" height="562" alt="image" src="https://github.com/user-attachments/assets/0a79c6d4-a78e-4ef9-8138-e94d0924d4d7" />


STM32U5는 단순 센서 보드가 아니라, **로봇 상태 표시(TouchGFX) + 센서 모듈** 역할을 수행합니다.

#### ✅ A. ROS → STM32U5(TouchGFX) 
Raspberry Pi 4(ROS2)에서 수신/발행 중인 주요 토픽을 STM32U5로 전달하여 TouchGFX에 표시합니다.

| Topic | 의미 | UI 표시 예시 |
|------|------|-------------|
| `/hostname/amcl_pose` | 맵 기준 현재 위치 | x, y, yaw |
| `/hostname/odom` | odom/속도 | v, w, 누적 |
| `/hostname/battery_state` | 배터리 상태 | %, charging |
| `/hostname/goal_pose` | 목표 좌표 | goal x, y |
| `/hostname/scan` | LiDAR 스캔 | min range |

#### ✅ B. STM32U5 센서 → ROS2 토픽 생성 → STM32U5(TouchGFX) 
STM32U5에 연결된 센서를 통해 ROS2로 전송하여 **추가 토픽을 생성**하고, 로봇 호스트명을 붙여 재발행 하고 STM32U5로 전달하여 TouchGFX에 표시합니다.

- STM32U5 센서 입력:
- | Topic | 의미 | UI 표시 예시 |
  |------|------|-------------|
  | `/hostname/ultra_distance_cm` | 초음파 거리(cm) | min 거리 |
  | `/hostname/seat_detected` | 탑승 감지(0/1) | Seated/Empty |

- 운영 관점:
  - 다중 로봇 환경에서 토픽 충돌을 막기 위해  
    `/<hostname>/ultra_distance_cm`, `/<hostname>/seat_detected` 처럼 **호스트명 구조로 재발행** 

 
---

## ✨ 4. 핵심 기능


### 🖥 UI (운영)

- Qt 기반 터치 키오스크 UI 구성
  - 외래 환자 직접 휠체어 호출
  - 출발지 / 목적지 선택 또는 스테이션 기준 호출
  - 호출 정보 서버 → DB(call_queue) 저장

- 병동 / 병실 기반 호출 UI
  - 간호사·관리자용 QT 화면 구성
  - 사전 등록된 병동 / 병실 위치 선택 후 배차 요청

- 운영 모니터링 UI
  - 로봇 상태 표시
    - WAITING / HEADING / RUNNING / ARRIVED / CHARGING / ERROR
  - 현재 위치, 배터리 잔량, 충전 여부 확인
  - 도착 / 이상 상태 관리자 알림 연계

### 🤖 Robot (자율주행)
- ROS2 기반 자율주행 파이프라인 구성
  - SLAM : 병원 맵 생성
  - AMCL : 실시간 위치 추정
  - Nav2 : 경로 계획 및 목표 지점 이동

- 센서 기반 위치 추정
  - LiDAR + IMU + Odometry 데이터 처리
  - 주행 안정성 확보

- Gazebo 시뮬레이션 환경 구성
  - 병원 구조 반영
  - 호출 → 배차 → 이동 → 도착 시나리오 검증

- 하단 장애물 안전 보완
  - STM32 초음파 센서 하단 장착
  - ROS 토픽 `/ultra_distance_cm` publish
  - 임계 거리 이하 시
    - Nav2 goal cancel
    - 또는 경로 재계획

### 🗃 Data (기록 / 배차)
- MariaDB 기반 서버 · DB 구성

- call_queue (호출 대기열)
  - 키오스크 / 시스템 호출 저장
  - call_time, start_loc, dest_loc, is_dispatched, eta 관리
  - 호출 등록 → 배차 → 이동 상태 갱신

- map_location (장소 → 좌표 매핑)
  - 병실 / 진료실 / 키오스크 명칭을 (x, y) 좌표로 관리
  - 서버 배차 시 Nav2 목표 좌표로 변환

- robot_status (핵심 테이블)
  - 로봇 상태 / 위치 / 배터리 지속 기록
  - start_x/y, goal_x/y 관리
  - order 컬럼을 DB 기반 명령 채널로 사용
    - order > 0 → 로봇 이동 명령 전달

- 우선순위 배차 로직
  - 1순위 : 응급 여부 (is_emergency)
  - 2순위 : 질병 우선순위 (base_priority)
  - 3순위 : 호출 시간 (call_time)

- 센서 이벤트 기록
  - STM32 FSR 압력 센서 → seat_detected 판단
  - Raspberry Pi 수신 → 서버 전달 → DB 기록
  - 운영 UI에서 탑승 / 하차 상태 확인
 
---
## 5. 지도
   
<img width="802" height="584" alt="image" src="https://github.com/user-attachments/assets/fb9f6af5-0ed8-4555-b5ec-a6c7ca96ab85" />

---

## 🛠 6. 구현 상세

- STM32U5를 ROS2 상위 제어와 분리된 하위 제어 계층으로 구현
- 초음파·FSR 센서를 MCU에서 직접 처리하여 이벤트 상태를 생성
- UART 기반 커스텀 메시지 포맷으로 STM32–ROS 간 상태 및 센서 데이터 동기화
- ROS2 노드는 센서 결과 토픽을 수신하여 주행 로직에서 활용
- 미션 매니저 노드에서 Nav2 goal cancel 등 제어 명령을 사용
- 다중 로봇 환경을 고려해 hostname 기반 토픽 및 제어 구조 적용
---
 ## 🧩 7. 기술 요약
- UI        : Qt
- Robot     : ROS2, SLAM, AMCL, Nav2
- Database  : MariaDB
- Simulation: Gazebo











