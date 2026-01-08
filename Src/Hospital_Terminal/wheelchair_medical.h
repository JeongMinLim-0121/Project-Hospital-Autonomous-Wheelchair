#ifndef WHEELCHAIR_MEDICAL_H
#define WHEELCHAIR_MEDICAL_H

#include <QWidget>
#include <QTimer> // 타이머 추가

namespace Ui {
class wheelchair_medical;
}

class wheelchair_medical : public QWidget
{
    Q_OBJECT

public:
    explicit wheelchair_medical(QWidget *parent = nullptr);
    ~wheelchair_medical();

private slots:
    // [기능] 휠체어 호출 버튼 클릭
    void on_pbSendWheelchair_clicked();

    // [기능] 호출 취소 버튼 클릭
    void on_pbCancel_clicked(); // 버튼 이름을 pbCancel로 가정

    // [기능] 타이머에 의해 주기적으로 실행될 슬롯
    void updateQueueTable();

    // [기능] 환자 명단이 바뀌었는지 확인하고 갱신하는 함수
    void updatePatientList();

private:
    Ui::wheelchair_medical *ui;
    QTimer *updateTimer; // 실시간 업데이트용 타이머

    void initUi();       // UI 초기 설정 (콤보박스 채우기 등)
    void initTable();    // 테이블 헤더 설정
};

#endif // WHEELCHAIR_MEDICAL_H
