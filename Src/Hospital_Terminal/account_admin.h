#ifndef ACCOUNT_ADMIN_H
#define ACCOUNT_ADMIN_H

#include <QWidget>

namespace Ui {
class account_admin;
}

class account_admin : public QWidget
{
    Q_OBJECT

public:
    explicit account_admin(QWidget *parent = nullptr);
    ~account_admin();

private slots:
    void on_pbAddUser_clicked();
    void on_pbUpdateUser_clicked();
    void on_pbDeleteUser_clicked();
    void on_pbClearForm_clicked();
    void on_twUserList_cellClicked(int row, int column);

private:
    Ui::account_admin *ui;

    void initUserTable();   // 테이블 기본 설정
    void loadUserList();    // db에서 데이터 불러오기
};

#endif // ACCOUNT_ADMIN_H
