#include "kiosk_main.h"
#include "ui_kiosk_main.h"

#include <QPixmap>

kiosk_main::kiosk_main(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::kiosk_main)
{
    ui->setupUi(this);

    // =================================================
    // 1. 메인 화면 마스코트 이미지 설정
    // =================================================
    QPixmap pm(":/icons/mascot.png");
    ui->label->setPixmap(pm);
    ui->label->setScaledContents(true);
    ui->label->setAlignment(Qt::AlignCenter);

    // =================================================
    // 2. 메인 화면 버튼 연결
    // =================================================

    // ---------------------------------
    // 휠체어 호출 버튼
    //  → 로그인 화면으로 이동
    // ---------------------------------
    connect(ui->pbkioskwheel, &QPushButton::clicked,
            this, &kiosk_main::goWheel);

    // ---------------------------------
    // 환자 조회 버튼
    //  → 환자 조회 화면으로 이동
    // ---------------------------------
    connect(ui->pbkioskpatient, &QPushButton::clicked,
            this, &kiosk_main::goSearch);
}

kiosk_main::~kiosk_main()
{
    delete ui;
}
