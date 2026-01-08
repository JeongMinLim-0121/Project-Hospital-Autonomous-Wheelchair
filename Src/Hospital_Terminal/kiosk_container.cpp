#include "kiosk_container.h"
#include "ui_kiosk_container.h"

#include "kiosk_main.h"
#include "kiosk_search.h"
//#include "kiosk_login.h"
#include "kiosk_wheel.h"
#include "kiosk_back.h"
#include "keyboard.h"
#include "mainwindow.h"

kiosk_container::kiosk_container(QMainWindow *mw, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::kiosk_container)
    , mainWindow(mw)
{
    ui->setupUi(this);

    //키오스크모드 로그아웃버튼 숨김
    if (auto *mw = qobject_cast<MainWindow*>(mainWindow)) {
        mw->setLogoutKioskMode(true);
    }

    // ---------------------------------
    // 1. 페이지 생성
    // ---------------------------------
    kiosk_main   *pageMain   = new kiosk_main(this);
    kiosk_search *pageSearch = new kiosk_search(this);
//   kiosk_login  *pageLogin  = new kiosk_login(this);
    kiosk_wheel  *pageWheel  = new kiosk_wheel(this);
    kiosk_back   *pageBack   = new kiosk_back(this);


    // ---------------------------------
    // 1-1. (핵심) 키오스크 내부 모든 QLineEdit을 MainWindow eventFilter에 등록
    // ---------------------------------
    if (mainWindow) {
        // container 전체에서 찾아도 되고, 필요한 페이지만 찾아도 됨
        // (너는 kiosk_login, kiosk_search만 필요하다고 했으니 아래처럼)
//       const QList<QLineEdit*> edits1 = pageLogin->findChildren<QLineEdit*>();
//      for (QLineEdit *e : edits1) e->installEventFilter(mainWindow);

        const QList<QLineEdit*> edits2 = pageSearch->findChildren<QLineEdit*>();
        for (QLineEdit *e : edits2) e->installEventFilter(mainWindow);
    }

    // ---------------------------------
    // 2. stackedWidget 교체
    // ---------------------------------
    ui->stackedWidget->removeWidget(ui->stackedWidget->widget(0));
    ui->stackedWidget->insertWidget(0, pageMain);

    ui->stackedWidget->removeWidget(ui->stackedWidget->widget(1));
    ui->stackedWidget->insertWidget(1, pageSearch);

 //   ui->stackedWidget->removeWidget(ui->stackedWidget->widget(2));
 //   ui->stackedWidget->insertWidget(2, pageLogin);

    ui->stackedWidget->removeWidget(ui->stackedWidget->widget(3));
    ui->stackedWidget->insertWidget(3, pageWheel);

    ui->stackedWidget->removeWidget(ui->stackedWidget->widget(4));
    ui->stackedWidget->insertWidget(4, pageBack);

    ui->stackedWidget->setCurrentWidget(pageMain);

    // ---------------------------------
    // kiosk_main
    // ---------------------------------
    connect(pageMain, &kiosk_main::goSearch, this, [=]() {
        ui->stackedWidget->setCurrentWidget(pageSearch);
    });

    connect(pageMain, &kiosk_main::goWheel, this, [=]() {
        prevPage = pageMain;                      // back 기준을 main으로
        pageWheel->setPatientInfo("외래환자", ""); // main에서 바로 들어간 경우 표시용
        ui->stackedWidget->setCurrentWidget(pageWheel);
    });


    // ---------------------------------
    // kiosk_search
    // ---------------------------------
    connect(pageSearch, &kiosk_search::searchAccepted, this, [=](QString name, QString id) {
        prevPage = pageSearch;
        pageWheel->setPatientInfo(name, id);
        ui->stackedWidget->setCurrentWidget(pageWheel);
    });

    connect(pageSearch, &kiosk_search::goBack, this, [=]() {
        ui->stackedWidget->setCurrentWidget(pageMain);
    });
/*
    // ---------------------------------
    // kiosk_login
    // ---------------------------------
    connect(pageLogin, &kiosk_login::loginAccepted, this, [=](QString name, QString id) {
        prevPage = pageLogin;
        pageWheel->setPatientInfo(name, id);
        ui->stackedWidget->setCurrentWidget(pageWheel);
    });

    connect(pageLogin, &kiosk_login::goBack, this, [=]() {
        ui->stackedWidget->setCurrentWidget(pageMain);
    });
*/
    // ---------------------------------
    // kiosk_wheel
    // ---------------------------------
    connect(pageWheel, &kiosk_wheel::wheelConfirmed, this, [=]() {
        ui->stackedWidget->setCurrentWidget(pageBack);
    });

    connect(pageWheel, &kiosk_wheel::goBack, this, [=]() {
        ui->stackedWidget->setCurrentWidget(prevPage ? prevPage : pageMain);
    });

    // ---------------------------------
    // kiosk_back
    // ---------------------------------
    connect(pageBack, &kiosk_back::goMain, this, [=]() {
        ui->stackedWidget->setCurrentWidget(pageMain);
    });

    connect(ui->stackedWidget, &QStackedWidget::currentChanged,
        this, [=](int){
            // MainWindow에 있는 키보드 숨김
            QMainWindow *mw = qobject_cast<QMainWindow*>(window());
            if (!mw) return;

            Keyboard *kbd = mw->findChild<Keyboard*>();
            if (kbd) {
                kbd->hide();
            }
        });
}

kiosk_container::~kiosk_container()
{
    //키오스크모드 로그아웃버튼 숨김
    if (auto *mw = qobject_cast<MainWindow*>(mainWindow)) {
        mw->setLogoutKioskMode(false);
    }

    delete ui;
}
