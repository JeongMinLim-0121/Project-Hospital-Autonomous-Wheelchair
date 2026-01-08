/* TouchGFX/gui/include/gui/model/ModelListener.hpp */
#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
// C헤더를 C++에서 쓰기 위해 extern "C" 감싸기 (혹은 common_data.h 내부에 되어있으면 생략 가능하지만 안전하게)
extern "C" {
#include "common_data.h"
}

class ModelListener
{
public:
    ModelListener() : model(0) {}
    
    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }

    // [추가] Presenter가 구현해야 할 함수 (데이터 갱신 알림)
    virtual void updateRobotData(RobotData_t data) {}

protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
