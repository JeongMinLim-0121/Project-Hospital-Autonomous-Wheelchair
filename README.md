

# 🏥 Hospital Autonomous Wheelchair Robot  

> **요약**  
> TurtleBot3 Burger 플랫폼에 3D 프린팅 휠체어 구조물을 장착하고,  
> **LiDAR SLAM 기반 자율주행(ROS2/Nav2)** + **중앙 서버/DB 배차 시스템** + **Qt 터치 키오스크(외래 호출)** + **STM32U5(초음파/압력 + Touch-GFX UI)** 를 결합해 병원 환경에서 **환자 호출 → 탑승 확인 → 목적지 이동 → 도착 알림 → 하차/대기/충전** 흐름을 구현하는 프로젝트입니다.
---
## 🎬 자율주행 휠체어 시연 영상
>  https://github.com/user-attachments/assets/8a61449f-51c5-4cb4-b58b-52134d1384b4

---

## 📌 1. 프로젝트 목표

- 병원에서 거동이 불편한 환자가 휠체어를 쉽게 호출하고 이동할 수 있도록 지원
- **외래 환자**는 **터치 키오스크(Qt)** 로 호출
- **입원 환자**는 간호사가 **간호사용(QT)** 로 호출
- 자율주행은 **LiDAR SLAM + Nav2** 로 수행
- **STM32U5G9J-DK2 보드**로:
  - LiDAR가 감지하기 어려운 **낮은 높이 장애물**을 초음파로 감지
  - **압력 센서**로 환자 탑승/하차를 감지하여 서버/DB에 반영
  - 보드 내장 **Touch-GFX**에서 로봇 상태/토픽값을 실시간 UI로 표시



---


## 🔧 2. 하드웨어 구성 

### ✅ Robot Side
<img width="787" height="224" alt="image" src="https://github.com/user-attachments/assets/54c18447-0883-4ffe-8b27-023a83595357" />


### ✅ STM32 Side
<img width="808" height="270" alt="image" src="https://github.com/user-attachments/assets/e0f09e85-82c0-49e6-930a-bf4ff67fae4e" />

---

## ✨ 3. 핵심 기능

### ✅ 자율주행
- SLAM으로 맵 생성
- AMCL로 위치 추정
- Nav2로 경로 계획 및 목표 지점 이동

### ✅ 외래 호출: Qt 터치 키오스크
- 외래 환자가 직접 터치 키오스크에서 “휠체어 호출”
- 출발/목적지 선택(또는 스테이션 기준 호출)
- 호출 정보는 서버/DB에 저장되고, 배차 로직이 로봇을 할당

### ✅ 입원 환자 호출/이동
  - 간호사가 필요에 따라 QT에서 환자에게 휠체어 배정 
  - 병동/병실 등 미리 등록된 위치로 휠체어 로봇 배차 및 이동
    
### ✅ 서버/DB 기반 배차
- DB의 호출 큐(call queue)를 기반으로
  - 가용 로봇 탐색
  - 우선순위 배정
  - 로봇에 start/goal 좌표 전달
  - 로봇 상태를 DB에 지속 기록

### ✅ STM32U5 보드 + Touch-GFX UI
- 초음파(IUM-100): **하단 장착** → 낮은 장애물 감지용
- 압력(FSR): **seat_detected** → 탑승 감지용
- Touch-GFX UI에 ROS 토픽 기반 상태 표시

---


## 🗺️ 4. 동작 시나리오 

### 4.1 외래 환자 호출(키오스크)
1. 외래 환자가 **Qt 키오스크**에서 휠체어 호출
2. 키오스크가 서버/DB에 호출 등록(call queue insert)
3. 서버 Dispatch가 가용 로봇을 선택하여 배차
4. 로봇은 출발지 → 목적지로 Nav2 goal 이동
5. 도착 시 서버/관리자 UI에 알림

### 4.2 입원 환자 호출(병동/병실 기반)
1. 간호사/관리자가 호출(병실/병동 선택)
2. 서버가 DB의 위치(map_location) 좌표를 참조
3. 로봇 배차 후 해당 위치로 이동

### 4.3 탑승/하차 판단(압력 센서)
- STM32U5의 FSR 압력 센서로 `seat_detected = 1/0` 판별
- Raspberry Pi4가 이를 수신하고 서버에 전달하여 DB에 기록

### 4.4 하단 장애물 안전(초음파)
- LiDAR는 보통 바닥 가까운 낮은 장애물을 놓칠 수 있음
- STM32U5 초음파를 하단에 장착하여 `/ultra_distance_cm` publish
- 임계 거리 이하면:
    -Nav2 goal cancel/재계획(설계에 따라)

----


## 🧠 5. 시스템 아키텍처 

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

## 6. 🧱 Database (MariaDB/MySQL)


### ✅ 주요 테이블 요약

#### 1) `call_queue` — 호출 대기열 (키오스크/시스템 호출 저장)

| 컬럼명 | 설명 | 예시 |
|---|---|---|
| `call_id`  | 호출 ID | 101 |
| `call_time`  | 호출 생성 시간 | 2025-12-31 12:34:56 |
| `caller_name`   | 환자/호출자 이름 | "홍길동" |
| `start_loc`   | 출발지(장소명 문자열) | "키오스크" |
| `dest_loc`   | 목적지(장소명 문자열) | "정형외과" |
| `is_dispatched`  | 배차 여부 | 배차 여부(0/1)|
| `eta`  | 상태/예상 정보 | "이동중" |


#### 2) `map_location` — 장소명 → 좌표 매핑 테이블

| 컬럼명  | 설명 | 예시 |
|---|---|---|
| `location_id`  | 장소 ID | 12 |
| `location_name` | 장소명(문자열) | "진료실 2" |
| `x`   | 맵 좌표 X | 1.25 |
| `y`   | 맵 좌표 Y | -3.40 |

> 서버 배차 시 `call_queue.start_loc`, `call_queue.dest_loc`을 `map_location`에서 찾아 (x, y)로 변환합니다.


#### 3) `robot_status` — 로봇 상태 + 명령 채널(핵심 테이블)

| 구분 | 컬럼명  | 설명 |
|---|---|---|
| 식별 | `robot_id` | 로봇 고유 ID |
| 식별 | `name`  | 로봇 이름(서버가 로봇 구분에 사용) |
| 네트워크 | `ip_address`   | 로봇 IP |
| 상태 | `op_status`  | `WAITING/HEADING/BOARDING/RUNNING/STOP/ARRIVED/EXITING/CHARGING/ERROR` |
| 전원 | `battery_percent`   | 배터리 % |
| 전원 | `is_charging`   | 충전 중 여부(0/1) |
| 현재 위치 | `current_x`   | 현재 X |
| 현재 위치 | `current_y`   | 현재 Y |
| 현재 방향 | `current_theta`   | 현재 θ(yaw) |
| 시작 좌표 | `start_x`  | 배차된 출발지 X |
| 시작 좌표 | `start_y`   | 배차된 출발지 Y |
| 목표 좌표 | `goal_x`   | 배차된 목적지 X |
| 목표 좌표 | `goal_y`   | 배차된 목적지 Y |
| 센서/기타 | `sensor`   | 센서 상태/메모 |
| **명령** | **`order`**   | **명령 존재 여부(서버: `order>0`이면 로봇에게 전송)** |
| 호출자 | `who_called`   | 호출자 이름 저장 |

> `order`는 서버 → 로봇 명령 전달을 위한 “DB 기반 명령 채널” 역할입니다.



#### 4) `patient_info`, `disease_types` — 우선순위 배차(응급/질병/호출시간)

##### `patient_info` (환자 정보)

| 컬럼(예시) | 설명 |
|---|---|
| `name` | 환자 이름 |
| `disease_code` | 질병 코드 |
| `is_emergency` | 응급 여부(1이면 최우선) |
| `type` | 환자 타입(OUT/IN) |

##### `disease_types` (질병 우선순위 기준)

| 컬럼(예시) | 설명 |
|---|---|
| `base_priority` | 우선순위 점수(클수록 우선) |

✅ **배차 우선순위 정렬**

| 우선순위 | 기준 | 정렬 방향 |
|---|---|---|
| 1 | `is_emergency` | DESC (응급 먼저) |
| 2 | `base_priority` | DESC (점수 높은 질병 먼저) |
| 3 | `call_time` | ASC (먼저 온 호출 먼저) |




---
## 7. 지도
   
<img width="802" height="584" alt="image" src="https://github.com/user-attachments/assets/fb9f6af5-0ed8-4555-b5ec-a6c7ca96ab85" />


---
 










