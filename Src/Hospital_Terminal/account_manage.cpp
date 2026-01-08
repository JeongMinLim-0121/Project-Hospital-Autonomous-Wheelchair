#include "account_manage.h"
#include "ui_account_manage.h"

account_manage::account_manage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::account_manage)
{
    ui->setupUi(this);
}

account_manage::~account_manage()
{
    delete ui;
}
