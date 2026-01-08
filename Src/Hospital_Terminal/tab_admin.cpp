#include "tab_admin.h"
#include "ui_tab_admin.h"

#include "account_admin.h"
#include "wheelchair_admin.h"
#include "wheelchair_map.h"
#include "search_medical.h"
#include "wheelchair_medical.h"

#include <QVBoxLayout>

tab_admin::tab_admin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::tab_admin)
{
    ui->setupUi(this);

    // 탭1: 계정 관리
    QWidget *tab1 = ui->tabWidget->widget(0);
    auto *layout1 = new QVBoxLayout(tab1);
    layout1->setContentsMargins(0, 0, 0, 0);
    layout1->addWidget(new account_admin(tab1));

    // 탭2: 휠체어 모니터링
    QWidget *tab2 = ui->tabWidget->widget(1);
    auto *layout2 = new QVBoxLayout(tab2);
    layout2->setContentsMargins(0, 0, 0, 0);
    layout2->addWidget(new wheelchair_admin(tab2));

    // 탭3: 환자 정보
    QWidget *tab3 = ui->tabWidget->widget(2);
    auto *layout3 = new QVBoxLayout(tab3);
    layout3->setContentsMargins(0, 0, 0, 0);
    layout3->addWidget(new search_medical(tab3));

    // 탭4: 휠체어 정보
    QWidget *tab4 = ui->tabWidget->widget(3);
    auto *layout4 = new QVBoxLayout(tab4);
    layout4->setContentsMargins(0, 0, 0, 0);
    layout4->addWidget(new wheelchair_medical(tab4));

    // 탭5: 터미널
    QWidget *tab5 = ui->tabWidget->widget(4);
    auto *layout5 = new QVBoxLayout(tab5);
    layout5->setContentsMargins(0, 0, 0, 0);
    layout5->addWidget(new wheelchair_map(tab5));
}

tab_admin::~tab_admin()
{
    delete ui;
}
