#if 0
#include "kiosk_login.h"
#include "ui_kiosk_login.h"
#include "database_manager.h"   // [필수] 환자번호로 DB 조회를 위해 필요

#include <QShowEvent>
#include <QMessageBox>

kiosk_login::kiosk_login(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::kiosk_login)
{
    ui->setupUi(this);

    // =================================================
    // 1. 마스코트 이미지 설정
    // =================================================
    QPixmap pm(":/icons/HJK_2.png");
    ui->label->setPixmap(pm);
    ui->label->setScaledContents(true);
    ui->label->setAlignment(Qt::AlignCenter);

    // ---------------------------------
    // 뒤로가기
    // ---------------------------------
    connect(ui->btn_back, &QPushButton::clicked,
            this, &kiosk_login::goBack);

    // ---------------------------------
    // [핵심] 환자번호 로그인 → DB 조회 → 바로 wheel로
    // ---------------------------------
    connect(ui->btn_login, &QPushButton::clicked,
            this, [=]() {

                QString patientId = ui->edit_name->text().trimmed();

                if (patientId.isEmpty()) {
                    QMessageBox::information(this, "오류", "환자번호를 입력해주세요.");
                    return;
                }

                // DB에서 환자 조회
                bool ok = false;
                PatientFullInfo info =
                    DatabaseManager::instance().getPatientById(patientId, &ok);

                if (!ok) {
                    QMessageBox::information(this, "오류", "해당 환자번호가 존재하지 않습니다.");
                    return;
                }

                // ---------------------------------
                // 로그인 성공 → 선택된 환자 상태로 wheel 이동
                // ---------------------------------
                emit loginAccepted(info.name, info.id);
            });
}

void kiosk_login::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // ---------------------------------
    // 화면 진입 시 입력 초기화
    // ---------------------------------
    ui->edit_name->clear();
}

kiosk_login::~kiosk_login()
{
    delete ui;
}
#endif
