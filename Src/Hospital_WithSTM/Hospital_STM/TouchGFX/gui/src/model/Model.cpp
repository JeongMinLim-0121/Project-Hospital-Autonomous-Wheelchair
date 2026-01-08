#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <cstring>

extern "C" {
#include "common_data.h"
#include "main.h"
extern volatile ButtonCommand_t g_btn_cmd_queue;
extern volatile RobotData_t g_robot_data;
}

Model::Model() : modelListener(0)
{
}

void Model::tick()
{
    // 데이터가 갱신되었는지 확인
    if (g_robot_data.is_updated)
    {
        g_robot_data.is_updated = false;

        // 2. 수정된 부분: volatile 제거를 위해 memcpy 사용
        RobotData_t currentData;
        // (void*) 형변환을 통해 volatile 성질을 잠시 무시하고 복사합니다.
        memcpy(&currentData, (void*)&g_robot_data, sizeof(RobotData_t));

        if (modelListener != 0)
        {
            modelListener->updateRobotData(currentData);
        }
    }
}

void Model::sendButtonCommand(int cmd)
{
    // 드디어 C 변수에 값을 넣습니다!
    g_btn_cmd_queue = (ButtonCommand_t)cmd;

    int actual_cmd = 0;
    switch(g_robot_data.current_state)
        {
            case STATE_BOARDING: // (2) 환자 탑승 대기 중일 때 -> 버튼 누르면 "탑승 완료(1)"
                actual_cmd = 1;
                break;

            case STATE_RUNNING:  // (3) 이동 중일 때 -> 버튼 누르면 "비상 정지(4)"
            case STATE_HEADING:  // (1) 픽업 중일 때 -> 버튼 누르면 "비상 정지(4)"
                actual_cmd = 4;
                break;

            case STATE_STOP:     // (4) 비상 정지 상태일 때 -> 버튼 누르면 "주행 재개(3)"
                actual_cmd = 3;
                break;

            case STATE_ARRIVED:  // (5) 목적지 도착 상태일 때 -> 버튼 누르면 "하차 완료(5)"
                actual_cmd = 5;
                break;

            default:
                // 그 외 상태(대기중 등)에서는 버튼 눌러도 반응 없음 (혹은 0)
                actual_cmd = 0;
                break;
        }

        if (actual_cmd != 0)
        {
            g_btn_cmd_queue = (ButtonCommand_t)actual_cmd;
        }

}
