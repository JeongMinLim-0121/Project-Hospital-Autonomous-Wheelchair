#include <gui/mainscreen_screen/MainScreenView.hpp>
#include <gui/mainscreen_screen/MainScreenPresenter.hpp>

// 생성자 이름 수정
mainscreenPresenter::mainscreenPresenter(mainscreenView& v)
    : view(v)
{
}

void mainscreenPresenter::activate()
{
}

void mainscreenPresenter::deactivate()
{
}

// 함수 이름 수정
void mainscreenPresenter::updateRobotData(RobotData_t data)
{
    view.updateScreen(data);
}

void mainscreenPresenter::userClickedStart()
{
    // Model에게 전달
    model->sendButtonCommand(1); // 1: 출발 명령이라 가정
}
