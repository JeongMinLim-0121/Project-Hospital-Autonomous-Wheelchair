#include "kiosk_back.h"
#include "ui_kiosk_back.h"

#include <QPixmap>

kiosk_back::kiosk_back(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::kiosk_back)
{
    ui->setupUi(this);

    // =================================================
    // 1. 완료 화면 마스코트 이미지 설정
    // =================================================
    QPixmap pm(":/icons/mascotbye.png");
    ui->label_image->setPixmap(pm);
    ui->label_image->setScaledContents(true);
    ui->label_image->setAlignment(Qt::AlignCenter);

    connect(ui->btn_main, &QPushButton::clicked,
            this, &kiosk_back::goMain);
}

kiosk_back::~kiosk_back()
{
    delete ui;
}
