#include <gui/mainscreen_screen/MainScreenView.hpp>
#include <touchgfx/Color.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <cstring> // strlen 사용

mainscreenView::mainscreenView()
{
}

void mainscreenView::setupScreen()
{
    mainscreenViewBase::setupScreen();
}

void mainscreenView::tearDownScreen()
{
    mainscreenViewBase::tearDownScreen();
}

void mainscreenView::updateScreen(RobotData_t data)
{
    // =========================================================
    // 1. 속도 및 배터리 (공통 정보)
    // =========================================================
    gauge_speed.updateValue(data.speed_kmh, 30); // 게이지 애니메이션
    Unicode::snprintf(txt_speedBuffer, TXT_SPEED_SIZE, "%d.%d", data.speed_kmh / 10, data.speed_kmh % 10);
    txt_speed.invalidate();

    progress_battery.setValue(data.battery_percent);
    Unicode::snprintf(txt_batteryBuffer, TXT_BATTERY_SIZE, "%d", data.battery_percent);
    txt_battery.invalidate();

    // =========================================================
    // 2. 텍스트 정보 (환자 이름 및 위치)
    // =========================================================

    // 2-1. 환자 이름 표시
    Unicode::fromUTF8((const uint8_t*)data.caller_name, txt_patientBuffer, TXT_PATIENT_SIZE);
    txt_patient.invalidate();

    // 2-2. 상태에 따른 위치 텍스트 선택
    const char* location_text = (const char*)data.dest_loc; // 기본값: 목적지

    if (data.current_state == STATE_HEADING)
    {
        // 픽업 중일 때는 출발지(환자위치)가 있다면 우선 표시
        if (strlen((char*)data.start_loc) > 0) {
            location_text = (const char*)data.start_loc;
        } else {
            location_text = (const char*)data.dest_loc;
        }
    }
    else
    {
        // 그 외에는 목적지 표시
        location_text = (const char*)data.dest_loc;
    }

    Unicode::fromUTF8((const uint8_t*)location_text, txt_destinationBuffer, TXT_DESTINATION_SIZE);
    txt_destination.invalidate();


    // =========================================================
    // 3. 상태별 UI 메시지 및 버튼 제어
    // =========================================================
    const char* status_msg = "";
    bool show_btn = false;
    TEXTS btn_text_id = T_T_BTN_EMERGENCY; // 기본값

    switch(data.current_state)
    {
        case STATE_WAITING: // 0
            status_msg = "대기중";
            show_btn = false;
            // 대기 중 텍스트 클리어
            Unicode::snprintf(txt_patientBuffer, TXT_PATIENT_SIZE, "");
            Unicode::snprintf(txt_destinationBuffer, TXT_DESTINATION_SIZE, "");
            txt_patient.invalidate();
            txt_destination.invalidate();
            break;

        case STATE_HEADING: // 1
            status_msg = "픽업 이동중";
            show_btn = true;
            btn_text_id = T_T_BTN_EMERGENCY;
            break;

        case STATE_BOARDING: // 2
            status_msg = "환자 탑승 대기";
            show_btn = true;
            btn_text_id = T_T_BTN_BOARDING;
            break;

        case STATE_RUNNING: // 3
            status_msg = "목적지 이동중";
            show_btn = true;
            btn_text_id = T_T_BTN_EMERGENCY;
            break;

        case STATE_STOP: // 4
            status_msg = "!! 비상 정지 !!";
            show_btn = true;
            btn_text_id = T_T_BTN_RESUME;
            break;

        case STATE_ARRIVED: // 5
            status_msg = "목적지 도착";
            show_btn = true;
            // [수정] 에러 해결을 위해 일단 BOARDING(탑승완료) 텍스트를 재사용합니다.
            // 추후 TouchGFX Designer에서 '하차 완료' 텍스트 리소스를 추가하면 교체하세요.
            btn_text_id = T_T_BTN_BOARDING;
            break;

        case STATE_EXITING: // 6
            status_msg = "안녕히 가세요";
            show_btn = false;
            break;

        case STATE_CHARGING: // 7
            status_msg = "충전중";
            show_btn = false;
            break;

        default:
            status_msg = "연결 대기중...";
            show_btn = false;
            break;
    }

    // 상태 메시지 업데이트
    Unicode::fromUTF8((const uint8_t*)status_msg, txt_StatusBuffer, TXT_STATUS_SIZE);
    txt_Status.invalidate();

    // 버튼 업데이트
    if (show_btn)
    {
        btn.setLabelText(touchgfx::TypedText(btn_text_id));
        btn.setVisible(true);
    }
    else
    {
        btn.setVisible(false);
    }
    btn.invalidate();
}

void mainscreenView::push_btn()
{
    // [수정] 에러 해결: 인자 제거 ()
    presenter->userClickedStart();
}
