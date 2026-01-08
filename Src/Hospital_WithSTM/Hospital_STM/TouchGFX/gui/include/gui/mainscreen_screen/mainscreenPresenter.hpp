#ifndef MAINSCREENPRESENTER_HPP
#define MAINSCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

extern "C" {
#include "common_data.h"
}

using namespace touchgfx;

class mainscreenView; // 대문자 Main -> 소문자 main 수정

// 클래스 이름 수정: MainScreenPresenter -> mainscreenPresenter
class mainscreenPresenter : public Presenter, public ModelListener
{
public:
    mainscreenPresenter(mainscreenView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~mainscreenPresenter() {};

    // 로봇 데이터 업데이트 함수
    virtual void updateRobotData(RobotData_t data);
    void userClickedStart();


private:
    mainscreenPresenter();

    mainscreenView& view;
};

#endif // MAINSCREENPRESENTER_HPP
