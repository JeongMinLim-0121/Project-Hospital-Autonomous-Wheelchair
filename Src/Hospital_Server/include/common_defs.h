/**
 * ============================================================================
 * @file    common_defs.h
 * @brief   프로젝트 공통 정의 헤더 (서버-클라이언트 공용)
 * @author  Team Hospital
 * @date    2025-12-04
 * @details
 * - 이 파일은 서버(C)와 클라이언트(Qt/ROS)가 반드시 *동일하게* 보유해야 함.
 * - 통신 프로토콜(패킷 구조), 에러 코드, 상수 등을 정의.
 * ============================================================================
 */

#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#include <stdint.h> // uint8_t, uint16_t 등 고정 크기 정수형 사용

/* ============================================================================
 * [시스템 상수 정의]
 * ============================================================================ */
#define SERVER_PORT     8080    // 접속 포트
#define MAX_BUFFER_SIZE 1024    // 송수신 버퍼 크기
#define MAGIC_NUMBER    0xAB    // 패킷 유효성 검사 키 (Header Start)

/* ============================================================================
 * [에러 코드 정의]
 * - 함수 리턴값 표준화
 * ============================================================================ */
typedef enum {
    RET_SUCCESS         =  0,   // 성공
    ERR_COMMON          = -1,   // 일반 에러
    ERR_SOCKET_CREATE   = -10,  // 소켓 생성 실패
    ERR_SOCKET_BIND     = -11,  // 바인딩 실패
    ERR_SOCKET_LISTEN   = -12,  // 리슨 실패
    ERR_SOCKET_ACCEPT   = -13,  // 수락 실패
    ERR_DISCONNECTED    = -14,  // 연결 끊김
    ERR_SIGNAL_REG      = -20   // 시그널 핸들러 등록 실패
} ErrorCode;

/* ============================================================================
 * [통신 프로토콜 정의]
 * ============================================================================ */

/**
 * @brief 기기 식별자 (Who are you?)
 * - 접속 시 자신이 누구인지 알리기 위함
 */
typedef enum {
    DEVICE_UNKNOWN    = 0x00,
    DEVICE_ADMIN_QT   = 0x01,  // 관리자/의료진 터미널
    DEVICE_ROBOT_ROS  = 0x02,  // 휠체어/터틀봇
    DEVICE_MONITOR    = 0x03   // 단순 모니터링
} DeviceType;

/**
 * @brief 메시지 타입 (What do you want?)
 * - 패킷의 목적을 구분
 */
typedef enum {
    MSG_NONE          = 0x00,
    
    // --- [접속 관련] ---
    MSG_LOGIN_REQ     = 0x01,  // "저 접속해요" (ID 전송)
    MSG_LOGIN_RES     = 0x02,  // "접속 허가/거부"

    // --- [이동 명령 관련 (Qt -> Server -> ROS)] ---
    MSG_MOVE_FORWARD  = 0x10,  // 전진
    MSG_MOVE_BACKWARD = 0x11,  // 후진
    MSG_TURN_LEFT     = 0x12,  // 좌회전
    MSG_TURN_RIGHT    = 0x13,  // 우회전
    MSG_STOP          = 0x14,  // 정지
    MSG_MOVE_TO_GOAL  = 0x15,  // 특정 좌표로 이동 (Payload에 좌표 포함)

    // --- [상태 정보 관련 (ROS -> Server -> Qt)] ---
    MSG_ROBOT_STATE   = 0x20,  // 배터리, 현재 위치 등 상태 보고
    MSG_ASSIGN_GOAL   = 0x30,  // 로봇에게 목표 명령
    MSG_ERROR_REPORT  = 0x99   // 로봇 에러 발생 알림
} MsgType;

/* ============================================================================
 * [패킷 구조체 정의]
 * - __attribute__((packed))를 사용하여 패딩(공백) 없이 바이트를 딱 붙임
 * - 네트워크 전송 시 구조체 크기를 일정하게 유지하기 위함
 * ============================================================================ */

/**
 * @brief 기본 패킷 헤더 (고정 길이 4바이트)
 */
typedef struct __attribute__((packed)) {
    uint8_t  magic;         // 0xAB (패킷 시작 식별)
    uint8_t  device_type;   // DeviceType (보내는 놈)
    uint8_t  msg_type;      // MsgType (메시지 종류)
    uint8_t  payload_len;   // 뒤에 따라올 데이터의 길이 (0이면 데이터 없음)
} PacketHeader;

typedef struct __attribute__((packed)) {
    int   order;     // 명령 종류 
    float start_x;   // 출발지 X (환자 위치)
    float start_y;   // 출발지 Y
    float goal_x;    // 목적지 X (병실 위치)
    float goal_y;    // 목적지 Y
    char caller_name[64]; // 탑승자 이름
} GoalAssignData;

/**
 * @brief 좌표 이동용 데이터 구조체 (예시)
 * - MSG_MOVE_TO_GOAL 일 때 payload에 실어서 보냄
 */
typedef struct __attribute__((packed)) {
    float x;
    float y;
    float theta;
} WaypointData;

/**
 * @brief 로봇 상태 데이터 구조체 (예시)
 * - MSG_ROBOT_STATE 일 때 payload에 실어서 보냄
 */
typedef struct __attribute__((packed)) {
    int   battery_level; // 0~100
    float current_x;
    float current_y;
    float theta;         // 로봇이 바라보는 방향 (라디안)
    uint8_t is_moving;   // 1:이동중, 0:정지
    int ultra_distance_cm;
    uint8_t seat_detected;
} RobotStateData;

#endif // COMMON_DEFS_H