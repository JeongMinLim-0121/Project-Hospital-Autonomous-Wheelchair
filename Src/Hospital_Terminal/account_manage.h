#ifndef ACCOUNT_MANAGE_H
#define ACCOUNT_MANAGE_H

#include <QWidget>

namespace Ui {
class account_manage;
}

class account_manage : public QWidget
{
    Q_OBJECT

public:
    explicit account_manage(QWidget *parent = nullptr);
    ~account_manage();

private:
    Ui::account_manage *ui;
};

#endif // ACCOUNT_MANAGE_H
