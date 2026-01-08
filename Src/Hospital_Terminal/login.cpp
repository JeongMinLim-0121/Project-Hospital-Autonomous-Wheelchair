#include "login.h"
#include "ui_login.h"
#include "database_manager.h"
#include <QMessageBox>
#include <QShowEvent>

Login::Login(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);
}

Login::~Login()
{
    delete ui;
}

void Login::on_pushButton_clicked()
{
    // UI의 입력창에서 텍스트 가져오기 (Qt Designer의 objectName과 일치해야 함)
    // 기존에 제공해주신 ui_login.h에 따르면 lineEdit, lineEdit_2로 되어있을 수 있습니다.
    // 만약 에러가 난다면 ui->lineEdit_id, ui->lineEdit_pw 등으로 이름을 확인하세요.
    QString id = ui->lineEdit->text();
    QString pw = ui->lineEdit_2->text();

    // DB 매니저를 통해 역할(Role) 조회
    // "admin", "medical", "patient" 또는 실패 시 ""(빈 문자열) 반환
    QString role = DatabaseManager::instance().getUserRole(id, pw);

    if (!role.isEmpty()) {
        // [핵심 변경] 
        // 직접 MainWindow를 띄우지 않고, 부모(MainWindow)에게 "성공 신호"만 보냅니다.
        emit loginSuccess(role);
    } else {
        // 로그인 실패 시 경고창 (인자 3개 필수: 부모, 제목, 내용)
        QMessageBox::warning(this, "Login Failed", "Wrong ID or Password.");
    }
}

void Login::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // 로그인 입력창 초기화
    ui->lineEdit->clear();
    ui->lineEdit_2->clear();

    // 포커스도 같이 정리 (선택)
    ui->lineEdit->clearFocus();
    ui->lineEdit_2->clearFocus();
}
