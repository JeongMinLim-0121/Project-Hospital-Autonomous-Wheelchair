#include "kiosk_wheel.h"
#include "ui_kiosk_wheel.h"
#include "database_manager.h"

#include <QMessageBox>
#include <QDebug>

kiosk_wheel::kiosk_wheel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::kiosk_wheel)
{
    ui->setupUi(this);

    // ---------------------------------
    // 1. 깜빡임 타이머 설정
    // ---------------------------------
    blinkTimer = new QTimer(this);
    isRed = true;

    // 0.5초(500ms)마다 호출 버튼 색상 변경
    connect(blinkTimer, &QTimer::timeout, this, [=]() {
        if (isRed) {
            ui->btn_call->setStyleSheet(
                "background-color: #FF9E9E; color: black; "
                "border-radius: 10px; font-weight: bold; font-size: 24px;"
                );
        } else {
            ui->btn_call->setStyleSheet(
                "background-color: #FF6B6B; color: white; "
                "border-radius: 10px; font-weight: bold; font-size: 24px;"
                );
        }
        isRed = !isRed;
    });

    // ---------------------------------
    // 2. 기본 버튼 스타일 정의 (선택 안됐을 때)
    // ---------------------------------
    QString defaultStyle =
        "background-color: #FFC0CB; color: black; "
        "border-radius: 10px; font-size: 18px;";

    QString selectedStyle =
        "background-color: #FF1493; color: white; "
        "border: 3px solid red; border-radius: 10px; "
        "font-size: 18px; font-weight: bold;";

    ui->btn_dest_food->setStyleSheet(defaultStyle);
    ui->btn_dest_jung->setStyleSheet(defaultStyle);
    ui->btn_dest_room->setStyleSheet(defaultStyle);
    ui->btn_dest_jae->setStyleSheet(defaultStyle);

    // ---------------------------------
    // 3. 목적지 선택 공통 처리 함수
    // ---------------------------------
    auto selectDest = [=](const QString &dest, QPushButton* btn) {

        // (1) 선택된 목적지 저장
        selectedDestination = dest;

        // (2) 모든 버튼 초기화
        ui->btn_dest_food->setStyleSheet(defaultStyle);
        ui->btn_dest_jung->setStyleSheet(defaultStyle);
        ui->btn_dest_room->setStyleSheet(defaultStyle);
        ui->btn_dest_jae->setStyleSheet(defaultStyle);

        // (3) 선택된 버튼 강조
        if (btn) {
            btn->setStyleSheet(selectedStyle);
        }

        // (4) 호출 버튼 깜빡임 시작
        if (!blinkTimer->isActive()) {
            blinkTimer->start(500);
        }
    };

    // ---------------------------------
    // 4. 목적지 버튼 연결
    // ---------------------------------
    connect(ui->btn_dest_food, &QPushButton::clicked,
            this, [=]() { selectDest("식당", ui->btn_dest_food); });

    connect(ui->btn_dest_jung, &QPushButton::clicked,
            this, [=]() { selectDest("정형외과", ui->btn_dest_jung); });

    connect(ui->btn_dest_room, &QPushButton::clicked,
            this, [=]() { selectDest("입원실", ui->btn_dest_room); });

    connect(ui->btn_dest_jae, &QPushButton::clicked,
            this, [=]() { selectDest("재활의학과", ui->btn_dest_jae); });

    // ---------------------------------
    // 뒤로가기
    // ---------------------------------
    connect(ui->btn_back, &QPushButton::clicked, this, [=]() {
        blinkTimer->stop();
        emit goBack();
    });

    // ---------------------------------
    // 휠체어 호출 버튼
    // ---------------------------------
    connect(ui->btn_call, &QPushButton::clicked, this, [=]() {

        if (selectedDestination.isEmpty()) {
            QMessageBox::information(this, "알림", "목적지를 선택해주세요.");
            return;
        }

        blinkTimer->stop();
        ui->btn_call->setStyleSheet(
            "background-color: #FF6B6B; color: white; "
            "border-radius: 10px; font-weight: bold; font-size: 24px;"
            );

        // ---------------------------------------------------------
        // DB 로직
        // ---------------------------------------------------------
        QString finalName = m_patientName;

        QPair<double, double> loc =
            DatabaseManager::instance().getLocation(selectedDestination);

        if (loc.first == -1.0 && loc.second == -1.0) {
            QMessageBox::critical(this, "오류", "좌표 데이터 없음");
            return;
        }

        bool queueOk =
            DatabaseManager::instance().addCallToQueue(
                finalName, "키오스크", selectedDestination
                );

        if (!queueOk) {
            QMessageBox::warning(this, "오류", "대기열 추가 실패");
            return;
        }

        bool robotOk =
            DatabaseManager::instance().updateRobotGoal(
                1, loc.first, loc.second, 2, finalName
                );

        if (robotOk) {
            emit wheelConfirmed();
        } else {
            QMessageBox::critical(this, "오류", "로봇 통신 실패");
        }
    });
}

kiosk_wheel::~kiosk_wheel()
{
    delete ui;
}

void kiosk_wheel::setPatientInfo(QString name, QString id)
{
    m_patientName = name;
    m_patientID = id;

    // =================================================
    // [추가] 선택된 환자 정보 표시
    // =================================================
    if (m_patientID.isEmpty()) {
        // 외래환자
        ui->label_patient->setText(
            QString("%1 님").arg(m_patientName)
            );
    } else {
        // 일반 환자
        ui->label_patient->setText(
            QString("%1 (%2) 님").arg(m_patientName, m_patientID)
            );
    }
    // ---------------------------------
    // 화면 진입 시 상태 초기화
    // ---------------------------------
    selectedDestination.clear();

    if (blinkTimer->isActive())
        blinkTimer->stop();

    QString defaultStyle =
        "background-color: #FFC0CB; color: black; "
        "border-radius: 10px; font-size: 18px;";

    ui->btn_dest_food->setStyleSheet(defaultStyle);
    ui->btn_dest_jung->setStyleSheet(defaultStyle);
    ui->btn_dest_room->setStyleSheet(defaultStyle);
    ui->btn_dest_jae->setStyleSheet(defaultStyle);

    ui->btn_call->setStyleSheet(
        "background-color: #FF6B6B; color: white; "
        "border-radius: 10px; font-weight: bold; font-size: 24px;"
        );

    qDebug() << "Kiosk_Wheel Init -> Name:" << name << ", ID:" << id;
}
