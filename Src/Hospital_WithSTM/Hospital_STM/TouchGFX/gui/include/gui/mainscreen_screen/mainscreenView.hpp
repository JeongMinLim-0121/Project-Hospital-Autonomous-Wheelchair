#ifndef MAINSCREENVIEW_HPP
#define MAINSCREENVIEW_HPP

#include <gui_generated/mainscreen_screen/MainScreenViewBase.hpp>
#include <gui/mainscreen_screen/MainScreenPresenter.hpp>

extern "C" {
#include "common_data.h"
}

// 클래스 이름 수정: MainScreenView -> mainscreenView
class mainscreenView : public mainscreenViewBase
{
public:
    mainscreenView();
    virtual ~mainscreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    // 화면 갱신 함수
    virtual void updateScreen(RobotData_t data);
    virtual void push_btn();
protected:
};

#endif // MAINSCREENVIEW_HPP
