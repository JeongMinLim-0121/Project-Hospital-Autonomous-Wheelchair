#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QLineEdit> // QLineEdit 캐스팅을 위해 필요
#include <QTabBar>
#include <QTimer>
#include <QDateTime>
#include <QPixmap>
#include <QProcess>
#include <QStringList>
#include <QGraphicsOpacityEffect>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , virtualKeyboard(nullptr)
{
    ui->setupUi(this);
    //-----------네트워크 로고(초기)----------//
    QPixmap pmWifi(":/icons/wifi.png");
    QPixmap pmLan(":/icons/Lan.png");
    QPixmap pmCaution(":/icons/caution.png");

    ui->label_wifi->setScaledContents(true);
    ui->label_wifi->setAlignment(Qt::AlignCenter);

    // 초기값은 연결 확인 전이므로 경고 아이콘
    ui->label_wifi->setPixmap(pmCaution);

    //-----------네트워크 상태 감시 타이머----------//
    QTimer *netTimer = new QTimer(this);

    connect(netTimer, &QTimer::timeout, this, [=]() {

        // 1) nmcli로 연결 상태 확인 (NetworkManager 기준)
        QProcess proc;
        proc.start("nmcli", {"-t", "-f", "TYPE,STATE", "device"});
        if (!proc.waitForFinished(1000)) {
            // 명령 실패/타임아웃 → 경고
            ui->label_wifi->setPixmap(pmCaution);
            return;
        }

        QString out = QString::fromUtf8(proc.readAllStandardOutput());

        // 2) 우선순위: ethernet > wifi > none
        // (원하면 wifi를 우선으로 바꿔도 됨)
        if (out.contains("ethernet:connected")) {
            ui->label_wifi->setPixmap(pmLan);
        }
        else if (out.contains("wifi:connected")) {
            ui->label_wifi->setPixmap(pmWifi);
        }
        else {
            ui->label_wifi->setPixmap(pmCaution);
        }
    });

    // 처음 실행 시 즉시 반영 + 이후 주기 실행
    netTimer->start(3000);

    //-----------년일월시----------//
    QTimer *timeTimer = new QTimer(this);

    connect(timeTimer, &QTimer::timeout, this, [=]() {
        ui->label_time->setText(
            QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
            );
    });

    timeTimer->start(1000);

    // ---------------------------------------------------------
    // 1. 가상 키보드 설정 (조건부 생성)
    // ---------------------------------------------------------
    
    // 디버깅용: 현재 키보드 상태 출력
    bool hasKeyboard = isPhysicalKeyboardConnected();
    qDebug() << "Initial Keyboard Check:" << hasKeyboard;

    if (!hasKeyboard) {
        virtualKeyboard = new Keyboard(this);
        
        // 메인 레이아웃의 맨 아래에 추가
        ui->verticalLayout->addWidget(virtualKeyboard);
        
        // 높이 고정 및 사이즈 정책 설정
        virtualKeyboard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        virtualKeyboard->setFixedHeight(300);
        
        // 초기 상태는 숨김
        virtualKeyboard->hide();
        virtualKeyboard->setVisible(false);
    }
    // 물리 키보드가 있다면 virtualKeyboard는 nullptr 상태로 유지

    // ---------------------------------------------------------
    // 2. 로그인 페이지 조립
    // ---------------------------------------------------------
    loginPage = new Login(this);
    
    ui->stackedWidget->addWidget(loginPage);
    ui->stackedWidget->setCurrentWidget(loginPage);

    // 신호 연결
    connect(loginPage, &Login::loginSuccess, this, &MainWindow::onLoginSuccess);

    // 로그인 페이지의 입력창(LineEdit)들도 이벤트 필터에 등록
    QList<QLineEdit *> lineEdits = loginPage->findChildren<QLineEdit *>();
    for (QLineEdit *edit : lineEdits) {
        edit->installEventFilter(this);
    }

    // ---------------------------------------------------------
    // 2-1. Kiosk 페이지 생성
    // ---------------------------------------------------------
    kioskMain   = new kiosk_main(this);
    kioskSearch = new kiosk_search(this);
//   kioskLogin  = new kiosk_login(this);
    kioskWheel  = new kiosk_wheel(this);
    kioskBack   = new kiosk_back(this);

    // ---------------------------------------------------------
    // 2-2. stackedWidget에 Kiosk 페이지 등록
    // (상단바는 MainWindow에 있으므로 내용만 교체됨)
    // ---------------------------------------------------------
    ui->stackedWidget->addWidget(kioskMain);
    ui->stackedWidget->addWidget(kioskSearch);
//    ui->stackedWidget->addWidget(kioskLogin);
    ui->stackedWidget->addWidget(kioskWheel);
    ui->stackedWidget->addWidget(kioskBack);

    // ---------------------------------------------------------
    // 3. 로그아웃 버튼 연결 및 초기 상태
    // ---------------------------------------------------------
    if (ui->btn_logout) {
        connect(ui->btn_logout, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    }
    
    if (ui->label_status) {
        ui->label_status->setText("Hospital");
    }

    if (ui->centralwidget->layout()) {
        ui->centralwidget->layout()->setSizeConstraint(QLayout::SetNoConstraint);
    }

    // 2. 창 크기 정책을 '무조건 늘어날 수 있음'으로 설정합니다.
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 3. 최소 크기를 작게 잡고, 기본 크기를 재설정합니다.
    this->setMinimumSize(QSize(100, 100)); // 최소 크기 제한 해제
    this->setMaximumSize(QSize(16777215, 16777215)); // 최대 크기 제한 해제
    this->resize(800, 600); // 원하는 초기 크기 강제 적용
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ---------------------------------------------------------
// [수정됨] 기능 1: 물리 키보드 연결 확인 (정밀 검사)
// ---------------------------------------------------------
bool MainWindow::isPhysicalKeyboardConnected()
{
    QFile file("/proc/bus/input/devices");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open input devices file";
        return false; 
    }

    // 파일 전체 읽기 (0바이트 문제 해결)
    QByteArray data = file.readAll();
    file.close();

    QString content = QString::fromUtf8(data);
    
    // 각 장치 블록은 빈 줄로 구분됩니다.
    // 하지만 간단하게 줄 단위로 읽으면서 'Context'를 파악하는 방식으로 합니다.
    
    QStringList lines = content.split('\n');
    
    bool hasKbdHandler = false;
    bool isUsb = false;
    bool isKeyName = false;

    // 파일을 순차적으로 읽으면서 하나의 장치 블록을 분석합니다.
    // I: Bus=... 로 시작하는 부분이 새 장치의 시작입니다.
    
    for (const QString &line : lines) {
        if (line.trimmed().isEmpty()) {
            // 빈 줄을 만나면 이전 장치 분석 결과를 확인
            if (hasKbdHandler && (isUsb || isKeyName)) {
                qDebug() << "Real Physical Keyboard Detected!";
                return true;
            }
            
            // 다음 장치 분석을 위해 초기화
            hasKbdHandler = false;
            isUsb = false;
            isKeyName = false;
            continue;
        }

        // 1. 핸들러 확인 (kbd가 있는지)
        if (line.contains("Handlers=") && line.contains("kbd")) {
            hasKbdHandler = true;
        }

        // 2. 물리적 연결 방식 확인 (USB인지)
        if (line.contains("Phys=usb")) {
            isUsb = true;
        }

        // 3. 이름 확인 (Keyboard라는 단어가 들어가는지) - 대소문자 무시
        if (line.contains("Name=") && line.contains("keyboard", Qt::CaseInsensitive)) {
            isKeyName = true;
        }
    }
    
    // 마지막 장치 확인 (파일 끝이라 빈 줄이 없을 수 있음)
    if (hasKbdHandler && (isUsb || isKeyName)) {
        qDebug() << "Real Physical Keyboard Detected (Last device)!";
        return true;
    }

    qDebug() << "No physical keyboard found.";
    return false;
}

// ---------------------------------------------------------
// 기능 2: 이벤트 필터 (터치 감지 -> 가상 키보드 띄우기)
// ---------------------------------------------------------
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    //  마우스로 눌렀을 때 기준 처리
    if (event->type() == QEvent::MouseButtonPress) {

        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(obj);

        if (lineEdit && virtualKeyboard) {
            //  LineEdit 클릭 → 키보드 표시
            virtualKeyboard->setLineEdit(lineEdit);
            virtualKeyboard->show();
        }
        else if (virtualKeyboard) {
            //  LineEdit 아닌 곳 클릭 → 키보드 숨김
            virtualKeyboard->hide();
        }
    }

    //  포커스 인 (키보드 포커스 이동용 보조)
    if (event->type() == QEvent::FocusIn) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit*>(obj);
        if (lineEdit && virtualKeyboard) {
            virtualKeyboard->setLineEdit(lineEdit);
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

// ---------------------------------------------------------
// 기능 3: 로그인 성공 처리
// ---------------------------------------------------------
void MainWindow::onLoginSuccess(QString role)
{
    if (ui->page_dashboard) {
        ui->stackedWidget->setCurrentWidget(ui->page_dashboard);
    }

    if (virtualKeyboard != nullptr) {
        virtualKeyboard->hide();
    }

    if (ui->mainTabWidget) {
        ui->mainTabWidget->clear();

        // 🔹 탭 바 숨기기 (시스템 관리 / 진료 업무 글자 제거)
        ui->mainTabWidget->tabBar()->hide();

        if (role == "admin") {
            if (ui->label_status)
                ui->label_status->setText("관리자 모드");

            ui->mainTabWidget->addTab(new tab_admin(this), "시스템 관리");
            ui->mainTabWidget->setCurrentIndex(0);
        }
        else if (role == "medical") {
            if (ui->label_status)
                ui->label_status->setText("의료진 모드");

            ui->mainTabWidget->addTab(new tab_medical(this), "진료 업무");
            ui->mainTabWidget->setCurrentIndex(0);
        }
        else if (role == "kiosk") {
            if (ui->label_status)
                ui->label_status->setText("키오스크 모드");
            ui->mainTabWidget->clear();
            ui->mainTabWidget->addTab(new kiosk_container(this, this), "키오스크");
            ui->mainTabWidget->setCurrentIndex(0);
        }
    }

}

// ---------------------------------------------------------
// 기능 4: 로그아웃 처리
// ---------------------------------------------------------
void MainWindow::onLogoutClicked()
{
    if (ui->mainTabWidget) {
        while (ui->mainTabWidget->count() > 0) {
            QWidget* widget = ui->mainTabWidget->widget(0);
            ui->mainTabWidget->removeTab(0);
            delete widget;
        }
    }

    ui->stackedWidget->setCurrentWidget(loginPage);
    
    if (ui->label_status) ui->label_status->setText("로그아웃 되었습니다.");
        ui->label_status->setText("Hospital");
    if (virtualKeyboard != nullptr) {
        virtualKeyboard->hide();
    }
}

// ---------------------------------------------------------
// 기능 5: 키오스크모드 로그아웃버튼 숨김
// ---------------------------------------------------------
void MainWindow::setLogoutKioskMode(bool on)
{
    if (on) {
        auto *e = new QGraphicsOpacityEffect(ui->btn_logout);
        e->setOpacity(0.0);                 // 안 보이게
        ui->btn_logout->setGraphicsEffect(e);
        ui->btn_logout->setEnabled(true);   // 클릭은 유지
    } else {
        ui->btn_logout->setGraphicsEffect(nullptr); // 원복
    }
}
