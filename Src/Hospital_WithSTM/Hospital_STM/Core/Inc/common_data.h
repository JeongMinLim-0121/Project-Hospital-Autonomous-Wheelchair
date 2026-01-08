/* Core/Inc/common_data.h */
#ifndef COMMON_DATA_H
#define COMMON_DATA_H

#include <stdint.h>
#include <stdbool.h>

// 1. 로봇 상태
typedef enum {
    STATE_WAITING  = 0,
    STATE_HEADING  = 1,
    STATE_BOARDING = 2,
    STATE_RUNNING  = 3,
    STATE_STOP     = 4,
    STATE_ARRIVED  = 5,
    STATE_EXITING  = 6,
    STATE_CHARGING = 7
} RobotState_t;

// 2. 수신 데이터 구조체
typedef struct {
    RobotState_t current_state;
    int speed_kmh;
    int battery_percent;
    char caller_name[64];
    char start_loc[64];
    char dest_loc[64];
    bool is_updated;
} RobotData_t;

// 3. 버튼 명령 (이 부분이 없어서 에러가 났을 수 있습니다)
typedef enum {
    BTN_CMD_NONE = 0,
    BTN_CMD_BOARDING_COMPLETE = 2,
    BTN_CMD_EMERGENCY_STOP = 4,
    BTN_CMD_RESUME = 3,
    BTN_CMD_EXIT_COMPLETE = 5
} ButtonCommand_t;

// 전역 변수 공유 선언
extern volatile RobotData_t g_robot_data;
extern volatile ButtonCommand_t g_btn_cmd_queue; // <-- 이 줄 필수!

#endif