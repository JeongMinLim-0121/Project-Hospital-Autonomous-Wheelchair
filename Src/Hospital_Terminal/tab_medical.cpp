#include "tab_medical.h"
#include "ui_tab_medical.h"

#include "search_medical.h"
#include "wheelchair_medical.h"
#include "QVBoxLayout"

tab_medical::tab_medical(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::tab_medical)
{
    ui->setupUi(this);

    QWidget *tab1 = ui->tabWidget->widget(0);
    QWidget *tab2 = ui->tabWidget->widget(1);

    // 탭1: 환자 조회 페이지
    auto *layout1 = new QVBoxLayout(tab1);
    layout1->setContentsMargins(0, 0, 0, 0);
    search_medical *searchPage = new search_medical(tab1);
    layout1->addWidget(searchPage);

    // 탭2: 휠체어 페이지
    auto *layout2 = new QVBoxLayout(tab2);
    layout2->setContentsMargins(0, 0, 0, 0);
    wheelchair_medical *wheelchairPage = new wheelchair_medical(tab2);
    layout2->addWidget(wheelchairPage);
}

tab_medical::~tab_medical()
{
    delete ui;
}
