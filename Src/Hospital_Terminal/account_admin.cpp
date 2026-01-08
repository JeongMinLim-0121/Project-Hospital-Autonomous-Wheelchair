#include "account_admin.h"
#include "ui_account_admin.h"
#include "database_manager.h"

#include <QTableWidgetItem>
#include <QHeaderView>
#include <QMessageBox>

account_admin::account_admin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::account_admin)
{
    ui->setupUi(this);
    initUserTable();

    //콤보박스 아이템 설정(DB에 맞춤)
    ui->cbRole->clear();
    ui->cbRole->addItems({"admin", "medical", "kiosk"});

    //테이블 초기화 및 데이터 로드
    initUserTable();
    loadUserList();

}

account_admin::~account_admin()
{
    delete ui;
}

void account_admin::initUserTable()
{
    auto *table = ui->twUserList;
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);

    QStringList headers;
    headers << "사용자 ID" << "비밀번호" << "권한" << "비고";

    table->setColumnCount(4);
    table->setHorizontalHeaderLabels(headers);
    table->setColumnWidth(0, 100);
    table->setColumnWidth(1, 100);
    table->setColumnWidth(2, 80);
    table->horizontalHeader()->setStretchLastSection(true);
}

// DB 매니저에서 데이터를 받아와 테이블에 표시
void account_admin::loadUserList()
{
    ui->twUserList->setRowCount(0);

    // DatabaseManager를 통해 데이터 가져오기
    QList<QStringList> users = DatabaseManager::instance().getAllUsers();

    for (const QStringList &user : users) {
        int row = ui->twUserList->rowCount();
        ui->twUserList->insertRow(row);

        // id, pw, role, comment 순서
        ui->twUserList->setItem(row, 0, new QTableWidgetItem(user[0]));
        ui->twUserList->setItem(row, 1, new QTableWidgetItem(user[1]));
        ui->twUserList->setItem(row, 2, new QTableWidgetItem(user[2]));
        ui->twUserList->setItem(row, 3, new QTableWidgetItem(user[3]));
    }
}

void account_admin::on_pbAddUser_clicked()
{
    QString id = ui->leUserId->text();
    QString pw = ui->leUserName->text(); // *중요: UI 객체 leUserName이 비밀번호 역할
    QString role = ui->cbRole->currentText();
    QString comment = ui->leNote->text();

    if (id.isEmpty() || pw.isEmpty()) {
        QMessageBox::warning(this, "경고", "ID와 비밀번호를 입력하세요.");
        return;
    }

    // DB 매니저에게 추가 요청
    if (DatabaseManager::instance().addUser(id, pw, role, comment)) {
        QMessageBox::information(this, "성공", "사용자가 추가되었습니다.");
        loadUserList();
        on_pbClearForm_clicked();
    } else {
        QMessageBox::critical(this, "실패", "사용자 추가 실패 (ID 중복 등)");
    }
}

void account_admin::on_pbUpdateUser_clicked()
{
    QString id = ui->leUserId->text();
    QString pw = ui->leUserName->text();
    QString role = ui->cbRole->currentText();
    QString comment = ui->leNote->text();

    if (id.isEmpty()) {
        QMessageBox::warning(this, "경고", "수정할 사용자를 선택하세요.");
        return;
    }

    // DB 매니저에게 수정 요청
    if (DatabaseManager::instance().updateUser(id, pw, role, comment)) {
        QMessageBox::information(this, "성공", "정보가 수정되었습니다.");
        loadUserList();
        on_pbClearForm_clicked();
    } else {
        QMessageBox::critical(this, "실패", "사용자 수정 실패");
    }
}

void account_admin::on_pbDeleteUser_clicked()
{
    QString id = ui->leUserId->text();

    if (id.isEmpty()) {
        QMessageBox::warning(this, "경고", "삭제할 사용자를 선택하세요.");
        return;
    }

    if (QMessageBox::question(this, "삭제", id + " 계정을 삭제하시겠습니까?",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
        return;
    }

    // DB 매니저에게 삭제 요청
    if (DatabaseManager::instance().deleteUser(id)) {
        QMessageBox::information(this, "성공", "사용자가 삭제되었습니다.");
        loadUserList();
        on_pbClearForm_clicked();
    } else {
        QMessageBox::critical(this, "실패", "사용자 삭제 실패");
    }
}

void account_admin::on_pbClearForm_clicked()
{
    ui->leUserId->clear();
    ui->leUserId->setEnabled(true); // ID 입력 활성화
    ui->leUserName->clear();        // PW 초기화
    ui->leNote->clear();
    ui->cbRole->setCurrentIndex(0);
}

void account_admin::on_twUserList_cellClicked(int row, int column)
{
    Q_UNUSED(column);

    // 테이블에서 텍스트 가져오기
    QString id = ui->twUserList->item(row, 0)->text();
    QString pw = ui->twUserList->item(row, 1)->text();
    QString role = ui->twUserList->item(row, 2)->text();
    QString comment = "";
    if (ui->twUserList->item(row, 3)) comment = ui->twUserList->item(row, 3)->text();

    // 입력폼 채우기
    ui->leUserId->setText(id);
    ui->leUserId->setEnabled(false); // 수정 모드: ID 변경 불가
    ui->leUserName->setText(pw);     // 비밀번호 칸 채우기
    ui->cbRole->setCurrentText(role);
    ui->leNote->setText(comment);
}
