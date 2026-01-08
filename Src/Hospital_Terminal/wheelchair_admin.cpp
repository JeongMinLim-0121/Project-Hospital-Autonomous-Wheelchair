#include "wheelchair_admin.h"
#include "ui_wheelchair_admin.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDebug>
#include <QVBoxLayout>
#include <QGridLayout> // 그리드 레이아웃 사용

#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <cmath>
#include <QPoint>

#include "add_robot_dialog.h"
#include "database_manager.h"

static constexpr double MAP_RESOLUTION = 0.05; // meters per pixel
static constexpr double MAP_ORIGIN_X   = -0.4; // meters
static constexpr double MAP_ORIGIN_Y   = -1.0;// meters

wheelchair_admin::wheelchair_admin(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::wheelchair_admin)
{
    ui->setupUi(this);

    initRobotTable();
    initCallLogTable(); // 호출 이력 테이블 초기화

    //맵 이미지 로드
    QPixmap mapImg(":/icons/Hospital.pgm");

    if (!mapImg.isNull()) {
        ui->lbMapPlaceholder->setPixmap(mapImg);

        // 라벨 크기에 맞춰 이미지 꽉 채우기 (비율 깨질 수 있음)
        //ui->lbMapPlaceholder->setScaledContents(true);
        // 또는 비율 유지하며 꽉 채우기
        //ui->lbMapPlaceholder->setPixmap(mapImg.scaled(ui->lbMapPlaceholder->size(), Qt::KeepAspectRatio, Qt::FastTransformation));

        m_mapOriginal = QPixmap(":/icons/Hospital.pgm");

        if (!m_mapOriginal.isNull()) {
            ui->lbMapPlaceholder->setAlignment(Qt::AlignCenter);
            ui->lbMapPlaceholder->setScaledContents(false); // 직접 scaled해서 넣을거라 false

            updateMapPixmapToLabel(); // ★ 추가: 라벨 크기에 맞게 맵 스케일 + offset 계산

            //qDebug() << "Map Loaded Successfully!";
        } else {
            //qDebug() << "Failed to load map image! Check 'icons/Hospital.pgm' path.";
            ui->lbMapPlaceholder->setStyleSheet("border: 2px solid red;");
            ui->lbMapPlaceholder->setText("Map Not Found");
        }

        //qDebug() << "Map Loaded Successfully!";
     } else {
        qDebug() << "Failed to load map image! Check 'icons/Hospital.pgm' path.";
        // 이미지가 없으면 빨간 테두리로 표시 (디버깅용)
        ui->lbMapPlaceholder->setStyleSheet("border: 2px solid red;");
        ui->lbMapPlaceholder->setText("Map Not Found");
     }

    // 로봇 선택 콤보박스 이벤트
    connect(ui->cbSelectedRobot, QOverload<int>::of(&QComboBox::activated), this, [this](int index){
        QString text = ui->cbSelectedRobot->itemText(index);
        if (!text.isEmpty()) {
            selectRobot(text.toInt());
        }
    });

    // 타이머 설정 (로봇 상태 + 호출 이력 동시 갱신)
//    updateTimer = new QTimer(this);
//    connect(updateTimer, &QTimer::timeout, this, &wheelchair_admin::refreshAll);
//    updateTimer->start(800);

    QTimer *fastTimer = new QTimer(this);
    connect(fastTimer, &QTimer::timeout, this, &wheelchair_admin::refreshRobotTable);
    fastTimer->start(1000);

    // 2. 느린 타이머 (호출 이력용): 2000ms (2초)
    // 호출 이력은 텍스트가 많아서 자주 갱신하면 렉 유발의 주범이 됩니다.
    QTimer *slowTimer = new QTimer(this);
    connect(slowTimer, &QTimer::timeout, this, &wheelchair_admin::refreshCallLog);
    slowTimer->start(2000);
}

wheelchair_admin::~wheelchair_admin()
{
    delete ui;
}

// 통합 갱신 함수
void wheelchair_admin::refreshAll()
{
    refreshRobotTable(); // 로봇 상태 갱신
    refreshCallLog();    // 호출 이력 갱신 (요구사항 1)
}

void wheelchair_admin::initRobotTable()
{
    auto *t = ui->twRobotStatus;
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->verticalHeader()->setVisible(false);

    t->setColumnCount(8);

    QStringList headers;
    headers << "ID" << "배터리" << "상태" << "현재 위치" << "출발 위치" << "도착 위치" << "초음파" << "탑승";
    t->setHorizontalHeaderLabels(headers);

    // =========================================================
    // [수정] 컬럼 너비 디자인 개선 (3, 4, 5번 균등 분배)
    // =========================================================
    QHeaderView *header = t->horizontalHeader();

    header->setSectionResizeMode(0, QHeaderView::Interactive); // ID
        t->setColumnWidth(0, 80);

        header->setSectionResizeMode(1, QHeaderView::Interactive); // 배터리
        t->setColumnWidth(1, 80);

        header->setSectionResizeMode(2, QHeaderView::Interactive); // 상태
        t->setColumnWidth(2, 100);

        header->setSectionResizeMode(6, QHeaderView::Interactive); // 초음파
        t->setColumnWidth(6, 80);

        header->setSectionResizeMode(7, QHeaderView::Interactive); // 탑승
        t->setColumnWidth(7, 60);

        // 2. 긴 정보들(좌표): 남은 공간을 1:1:1로 나눠 갖기 (Stretch)
        header->setSectionResizeMode(3, QHeaderView::Stretch); // 현재 위치
        header->setSectionResizeMode(4, QHeaderView::Stretch); // 출발 위치
        header->setSectionResizeMode(5, QHeaderView::Stretch); // 도착 위치

        t->horizontalHeader()->setStretchLastSection(false);

    t->setContextMenuPolicy(Qt::CustomContextMenu);
    disconnect(t, &QTableWidget::customContextMenuRequested, this, &wheelchair_admin::onCustomContextMenuRequested);
    connect(t, &QTableWidget::customContextMenuRequested, this, &wheelchair_admin::onCustomContextMenuRequested);
}

// [요구사항 1] 휠체어/예약 현황과 동일하게 보이도록 설정
void wheelchair_admin::initCallLogTable()
{
    auto *table = ui->twCallLog; // Qt Designer에서 이름이 twCallLog인지 확인

    table->setColumnCount(6); // 컬럼 6개
    QStringList headers;
    headers << "호출 시간" << "환자 이름" << "호출 위치" << "목적지" << "배차 여부" << "ETA";
    table->setHorizontalHeaderLabels(headers);

    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);

    // 컬럼 너비 조정
    table->setColumnWidth(0, 160); // 시간
    table->setColumnWidth(1, 100); // 이름
    table->setColumnWidth(2, 120); // 출발
    table->setColumnWidth(3, 120); // 도착
    table->setColumnWidth(4, 100); // 상태
    table->horizontalHeader()->setStretchLastSection(true); // ETA
}

// [요구사항 1] 호출 이력 데이터 실시간 갱신
void wheelchair_admin::refreshCallLog()
{
    // DB에서 Call Queue 가져오기 (기존 함수 재사용)
    QList<CallQueueItem> list = DatabaseManager::instance().getCallQueue();
    auto *table = ui->twCallLog;

    // 행 개수 맞추기
    if (table->rowCount() != list.size()) {
        table->setRowCount(list.size());
    }

    // 데이터 채우기
    for (int i = 0; i < list.size(); ++i) {
        const auto &item = list[i];

        // 0: 시간
        if(!table->item(i, 0)) table->setItem(i, 0, new QTableWidgetItem());
        table->item(i, 0)->setText(item.call_time);
        table->item(i, 0)->setTextAlignment(Qt::AlignCenter);

        // 1: 환자 이름
        if(!table->item(i, 1)) table->setItem(i, 1, new QTableWidgetItem());
        table->item(i, 1)->setText(item.caller_name);
        table->item(i, 1)->setTextAlignment(Qt::AlignCenter);

        // 2: 출발지
        if(!table->item(i, 2)) table->setItem(i, 2, new QTableWidgetItem());
        table->item(i, 2)->setText(item.start_loc);
        table->item(i, 2)->setTextAlignment(Qt::AlignCenter);

        // 3: 목적지
        if(!table->item(i, 3)) table->setItem(i, 3, new QTableWidgetItem());
        table->item(i, 3)->setText(item.dest_loc);
        table->item(i, 3)->setTextAlignment(Qt::AlignCenter);

        // 4: 배차 여부
        QString status = (item.is_dispatched == 1) ? "배차완료" : "대기중";
        if(!table->item(i, 4)) table->setItem(i, 4, new QTableWidgetItem());
        table->item(i, 4)->setText(status);
        table->item(i, 4)->setTextAlignment(Qt::AlignCenter);

        if (item.is_dispatched == 1) table->item(i, 4)->setForeground(Qt::blue);
        else table->item(i, 4)->setForeground(Qt::red);

        // 5: ETA
        if(!table->item(i, 5)) table->setItem(i, 5, new QTableWidgetItem());
        table->item(i, 5)->setText(item.eta);
        table->item(i, 5)->setTextAlignment(Qt::AlignCenter);
    }
}


void wheelchair_admin::on_pbDirectCall_clicked()
{
    // 1. 로봇 선택 확인
    if (m_selectedRobotId == -1) {
        QMessageBox::warning(this, "알림", "제어할 로봇을 먼저 선택해주세요.");
        return;
    }

    // 2. 다이얼로그 생성
    QDialog dialog(this);
    dialog.setWindowTitle("목적지 선택 및 좌표 입력");
    dialog.resize(400, 350); // 높이를 좀 더 키움

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

    // --- [섹션 1] 기존 장소 버튼 ---
    QLabel *label = new QLabel("저장된 장소로 이동:", &dialog);
    label->setAlignment(Qt::AlignCenter);
    QFont font = label->font();
    font.setBold(true);
    font.setPointSize(12);
    label->setFont(font);
    mainLayout->addWidget(label);

    QGridLayout *gridLayout = new QGridLayout();
    mainLayout->addLayout(gridLayout);

    QStringList locations = {"정형외과", "재활의학과", "식당", "입원실", "키오스크"};
    QString btnStyle = "QPushButton { padding: 15px; font-size: 14px; font-weight: bold; background-color: #E0E0E0; border-radius: 5px; }"
                       "QPushButton:hover { background-color: #D0D0D0; }"
                       "QPushButton:pressed { background-color: #B0B0B0; }";

    bool commandSent = false;

    // 버튼 생성 및 연결
    for(int i=0; i<locations.size(); ++i) {
        QString locName = locations[i];
        QPushButton *btn = new QPushButton(locName, &dialog);
        btn->setStyleSheet(btnStyle);
        gridLayout->addWidget(btn, i / 2, i % 2);

        connect(btn, &QPushButton::clicked, [&, locName]() {
            QPair<double, double> coord = DatabaseManager::instance().getLocation(locName);
            if (coord.first < 0 && coord.second < 0) {
                QMessageBox::critical(&dialog, "오류", QString("'%1' 좌표 없음").arg(locName));
                return;
            }

            QString who = "admin";
            if (DatabaseManager::instance().updateRobotGoal(m_selectedRobotId, coord.first, coord.second, 1, who)) {
                QMessageBox::information(&dialog, "명령 전송", QString("'%1'(%2, %3)로 이동").arg(locName).arg(coord.first).arg(coord.second));
                commandSent = true;
                dialog.accept();
            } else {
                QMessageBox::critical(&dialog, "오류", "DB 업데이트 실패");
            }
        });
    }

    // --- 구분선 추가 ---
    QFrame *line = new QFrame(&dialog);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(line);
    mainLayout->addSpacing(10);

    // --- [섹션 2] 수동 좌표 입력 ---
    QLabel *manualLabel = new QLabel("좌표 직접 입력:", &dialog);
    manualLabel->setFont(font);
    manualLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(manualLabel);

    QHBoxLayout *inputLayout = new QHBoxLayout();

    // X 좌표 입력
    QLabel *lblX = new QLabel("X:", &dialog);
    QDoubleSpinBox *sbX = new QDoubleSpinBox(&dialog);
    sbX->setRange(-100.0, 100.0); // 맵 크기에 맞게 범위 설정
    sbX->setDecimals(2);
    sbX->setSingleStep(0.1);

    // Y 좌표 입력
    QLabel *lblY = new QLabel("Y:", &dialog);
    QDoubleSpinBox *sbY = new QDoubleSpinBox(&dialog);
    sbY->setRange(-100.0, 100.0);
    sbY->setDecimals(2);
    sbY->setSingleStep(0.1);

    // 이동 버튼
    QPushButton *btnManualGo = new QPushButton("좌표 이동", &dialog);
    btnManualGo->setStyleSheet("QPushButton { padding: 5px 15px; background-color: #4CAF50; color: white; font-weight: bold; border-radius: 4px; }");

    inputLayout->addWidget(lblX);
    inputLayout->addWidget(sbX);
    inputLayout->addSpacing(10);
    inputLayout->addWidget(lblY);
    inputLayout->addWidget(sbY);
    inputLayout->addWidget(btnManualGo);

    mainLayout->addLayout(inputLayout);

    // 수동 이동 버튼 이벤트 연결
    connect(btnManualGo, &QPushButton::clicked, [&]() {
        double x = sbX->value();
        double y = sbY->value();
        QString who = "admin";

        if (DatabaseManager::instance().updateRobotGoal(m_selectedRobotId, x, y, 1, who)) {
            QMessageBox::information(&dialog, "명령 전송", QString("좌표 (%1, %2)로 이동합니다.").arg(x).arg(y));
            commandSent = true;
            dialog.accept();
        } else {
            QMessageBox::critical(&dialog, "오류", "DB 업데이트 실패");
        }
    });

    // --- [하단] 취소 버튼 ---
    mainLayout->addSpacing(10);
    QPushButton *btnCancel = new QPushButton("취소", &dialog);
    connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    mainLayout->addWidget(btnCancel);

    dialog.exec();

    if (commandSent) {
        refreshAll();
    }
}

// ... (Stop, Resume, Wait, Charge 등 나머지 버튼 기능은 기존 코드 유지) ...
// 기존 코드에서 selectRobot, refreshRobotTable, updateMapMarkers 등도 그대로 유지
// 단, refreshRobotTable 안에서 refreshCallLog()를 호출하도록 refreshAll()로 구조 변경함에 주의

void wheelchair_admin::on_pbStop_clicked() {
    if (m_selectedRobotId == -1) { QMessageBox::warning(this, "알림", "선택 필요"); return; }
    if (QMessageBox::Yes == QMessageBox::question(this, "정지", "즉시 정지하시겠습니까?")) {
        QString who = "admin";
        DatabaseManager::instance().updateRobotOrder(m_selectedRobotId, 2, who); // 2: STOP
        refreshAll();
    }
}

void wheelchair_admin::on_pbResume_clicked() {
    if (m_selectedRobotId == -1) { QMessageBox::warning(this, "알림", "선택 필요"); return; }
    if (QMessageBox::Yes == QMessageBox::question(this, "재개", "동작을 재개하시겠습니까?")) {
        QString who = "admin";
        DatabaseManager::instance().updateRobotOrder(m_selectedRobotId, 3, who); // 3: RESUME
        refreshAll();
    }
}

void wheelchair_admin::on_pbGoWait_clicked() {
    // 1. 로봇 선택 확인
    if (m_selectedRobotId == -1) {
        QMessageBox::warning(this, "알림", "선택 필요");
        return;
    }

    // 2. 확인 메시지
    if (QMessageBox::Yes == QMessageBox::question(this, "복귀", "대기 장소(키오스크)로 복귀하시겠습니까?")) {

        // 3. DB에서 '키오스크' 좌표 조회
        QString targetName = "키오스크";
        QPair<double, double> coord = DatabaseManager::instance().getLocation(targetName);

        // 4. DB 업데이트 (이동 명령 전송: order=1)
        QString who = "admin";
        bool success = DatabaseManager::instance().updateRobotGoal(
            m_selectedRobotId,
            coord.first,
            coord.second,
            4, // order 1: MOVE/CALL
            who
        );

        if (success) {
            // 성공 시 테이블 갱신
            refreshAll();
            QMessageBox::information(this, "명령 전송", QString("로봇이 '%1'로 이동합니다.").arg(targetName));
        } else {
            QMessageBox::critical(this, "오류", "DB 업데이트 실패");
        }
    }
}

void wheelchair_admin::on_pbGoCharge_clicked() {
    // 1. 로봇 선택 확인
    if (m_selectedRobotId == -1) {
        QMessageBox::warning(this, "알림", "선택 필요");
        return;
    }

    // 2. 확인 메시지
    if (QMessageBox::Yes == QMessageBox::question(this, "충전", "충전소(충전스테이션)로 이동하시겠습니까?")) {

        // 3. DB에서 '충전스테이션' 좌표 조회
        QString targetName = "충전스테이션";
        QPair<double, double> coord = DatabaseManager::instance().getLocation(targetName);


        // 4. DB 업데이트 (이동 명령 전송: order=1)
        QString who = "admin";
        bool success = DatabaseManager::instance().updateRobotGoal(
            m_selectedRobotId,
            coord.first,
            coord.second,
            5, // order 1: MOVE/CALL
            who
        );

        if (success) {
            refreshAll();
            QMessageBox::information(this, "명령 전송", QString("로봇이 '%1'로 이동합니다.").arg(targetName));
        } else {
            QMessageBox::critical(this, "오류", "DB 업데이트 실패");
        }
    }
}

void wheelchair_admin::on_pbAddWheel_clicked() {
    AddRobotDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        DatabaseManager::instance().addRobot(dlg.getName(), dlg.getIp(), dlg.getX(), dlg.getY(), dlg.getTheta());
        refreshAll();
    }
}

void wheelchair_admin::updateMapMarkers(const QList<RobotInfo> &robotList)
{
    QLabel *mapLabel = ui->lbMapPlaceholder;
    if (m_mapOriginal.isNull()) return;
    if (mapLabel->pixmap(Qt::ReturnByValue).isNull()) return;

    // 원본 맵(좌표계 기준) 크기
    const int mapW = m_mapOriginal.width();
    const int mapH = m_mapOriginal.height();

    // 라벨 크기, 실제 맵이 그려진 크기(KeepAspectRatio), 그리고 여백(레터박스) 오프셋
    const QSize labelSize = mapLabel->size();
    const QSize drawSize  = m_mapDrawSize;
    const QPoint offset   = m_mapOffset;

    // 원본 픽셀 -> 화면(그려진 맵 영역) 픽셀로 스케일
    const double scaleX = (double)drawSize.width()  / (double)mapW;
    const double scaleY = (double)drawSize.height() / (double)mapH;

    QSet<int> currentIds;

    for (const RobotInfo &info : robotList) {
        currentIds.insert(info.id);

        QPushButton *btn = nullptr;
        if (m_robotButtons.contains(info.id)) {
            btn = m_robotButtons[info.id];
        } else {
            btn = new QPushButton(mapLabel);
            btn->setText(info.name);
            btn->resize(24, 24);
            btn->show();
            connect(btn, &QPushButton::clicked, this, [this, info]() { selectRobot(info.id); });
            m_robotButtons.insert(info.id, btn);
        }

        // 선택된 로봇 강조
        if (info.id == m_selectedRobotId)
            btn->setStyleSheet("background-color: red; color: white; border-radius: 12px; border: 2px solid yellow;");
        else
            btn->setStyleSheet("background-color: #FF69B4; color: black; border-radius: 12px; border: 1px solid gray;");

        // =========================
        // (1) map 좌표(m) -> 원본 맵 픽셀(px)
        // x_px = (x - origin_x) / resolution
        // y_px = (H-1) - ((y - origin_y) / resolution)  (y축 반전)
        // =========================

        double px_raw = (info.current_x - MAP_ORIGIN_X) / MAP_RESOLUTION;
        const double px = px_raw + 5.0;

        double py_raw = (mapH - 1) - ((info.current_y - MAP_ORIGIN_Y) / MAP_RESOLUTION);
        const double py = py_raw - 21.0; // <-- 여기 숫자(15.0)를 조절하세요!

        // =========================
        // (2) 원본 픽셀 -> 라벨 좌표(그려진 맵 영역) : scale + offset
        // =========================
        const int xOnLabel = offset.x() + (int)std::round(px * scaleX);
        const int yOnLabel = offset.y() + (int)std::round(py * scaleY);

        // 버튼 중심 정렬
        int bx = xOnLabel - btn->width()  / 2;
        int by = yOnLabel - btn->height() / 2;

        // =========================
        // (3) 버튼이 "맵이 그려진 영역" 밖으로 나가지 않도록 clamp
        // =========================
        const int minX = offset.x();
        const int minY = offset.y();
        const int maxX = offset.x() + drawSize.width()  - btn->width();
        const int maxY = offset.y() + drawSize.height() - btn->height();

        bx = qBound(minX, bx, maxX);
        by = qBound(minY, by, maxY);

        btn->move(bx, by);
    }

    // 사라진 로봇 버튼 제거
    QList<int> existingIds = m_robotButtons.keys();
    for (int id : existingIds) {
        if (!currentIds.contains(id)) {
            if (m_robotButtons[id]) {
                m_robotButtons[id]->hide();
                m_robotButtons[id]->deleteLater();
            }
            m_robotButtons.remove(id);
        }
    }
}

void wheelchair_admin::refreshRobotTable() {
    QSqlQuery query = DatabaseManager::instance().getRobotStatusQuery();

    // 쿼리 에러 체크 (필수)
    if (query.lastError().isValid()) {
        return;
    }

    QList<RobotInfo> robotList;

    // [1] DB 데이터 읽기
    while(query.next()) {
        RobotInfo info;
        info.id = query.value("robot_id").toInt();
        info.name = query.value("name").toString();
        info.ip = query.value("ip_address").toString();
        info.status = query.value("op_status").toString();
        info.battery = query.value("battery_percent").toInt();

        // 좌표 읽기 (소수점)
        info.current_x = query.value("current_x").toDouble();
        info.current_y = query.value("current_y").toDouble();
        info.start_x = query.value("start_x").toDouble();
        info.start_y = query.value("start_y").toDouble();
        info.goal_x = query.value("goal_x").toDouble();
        info.goal_y = query.value("goal_y").toDouble();

        // [추가] 센서 데이터 읽기
        info.ultra_distance = query.value("ultra_distance_cm").toInt();
        info.seat_detected = query.value("seat_detected").toInt();

        robotList.append(info);
        m_robotCache.insert(info.id, info);
    }

    // 콤보박스 갱신 로직 (목록이 변경되었을 때만 갱신하여 깜빡임 방지)
    if (ui->cbSelectedRobot->count() != robotList.size()) {
        QString currentText = ui->cbSelectedRobot->currentText();
        ui->cbSelectedRobot->blockSignals(true);
        ui->cbSelectedRobot->clear();
        for(const auto &info : robotList) {
            ui->cbSelectedRobot->addItem(QString::number(info.id));
        }
        int idx = ui->cbSelectedRobot->findText(currentText);
        if(idx != -1) ui->cbSelectedRobot->setCurrentIndex(idx);
        ui->cbSelectedRobot->blockSignals(false);
    }

    // [2] 테이블 갱신
    auto *t = ui->twRobotStatus;
    t->blockSignals(true); // 갱신 중 시그널 차단
    t->setRowCount(robotList.size());

    for(int i=0; i<robotList.size(); ++i) {
        // Col 0: ID
        t->setItem(i, 0, new QTableWidgetItem(QString::number(robotList[i].id)));

        // Col 1: 배터리
        t->setItem(i, 1, new QTableWidgetItem(QString("%1%").arg(robotList[i].battery)));

        // Col 2: 상태
        t->setItem(i, 2, new QTableWidgetItem(robotList[i].status));

        // Col 3: 현재 위치 (소수점 3자리)
        t->setItem(i, 3, new QTableWidgetItem(
            QString("(%1, %2)")
            .arg(robotList[i].current_x, 0, 'f', 3)
            .arg(robotList[i].current_y, 0, 'f', 3)));

        // Col 4: 출발 위치
        t->setItem(i, 4, new QTableWidgetItem(
            QString("(%1, %2)")
            .arg(robotList[i].start_x, 0, 'f', 3)
            .arg(robotList[i].start_y, 0, 'f', 3)));

        // Col 5: 도착 위치
        t->setItem(i, 5, new QTableWidgetItem(
            QString("(%1, %2)")
            .arg(robotList[i].goal_x, 0, 'f', 3)
            .arg(robotList[i].goal_y, 0, 'f', 3)));

        // =========================================================
        // [추가] Col 6: 초음파 (거리 표시)
        // =========================================================
        QString distStr = QString::number(robotList[i].ultra_distance) + " cm"; // <--- cm 추가
        QTableWidgetItem *ultraItem = new QTableWidgetItem(distStr);
        ultraItem->setTextAlignment(Qt::AlignCenter);
        t->setItem(i, 6, ultraItem);

        // =========================================================
        // [추가] Col 7: 탑승 (0 -> X, 1 -> O + 빨간색)
        // =========================================================
        QTableWidgetItem *seatItem = new QTableWidgetItem();
        if (robotList[i].seat_detected == 1) {
            seatItem->setText("O");
            seatItem->setForeground(QBrush(Qt::red)); // 빨간색 설정

            // (옵션) 더 잘 보이게 굵게 처리
            QFont font = seatItem->font();
            font.setBold(true);
            seatItem->setFont(font);
        } else {
            seatItem->setText("X");
            seatItem->setForeground(QBrush(Qt::black)); // 기본 검정
        }
        t->setItem(i, 7, seatItem);

        // 전체 컬럼 가운데 정렬 (0 ~ 7번 컬럼)
        for(int col=0; col<8; col++) {
            if(t->item(i, col)) {
                t->item(i, col)->setTextAlignment(Qt::AlignCenter);
            }
        }
    }
    t->blockSignals(false);

    // 맵 마커 업데이트
    updateMapMarkers(robotList);

    // 테이블이 갱신되었으므로 선택 상태 복구
    if(m_selectedRobotId != -1){
        updateSelectionUI();
    }
}

void wheelchair_admin::selectRobot(int robotId) {
    m_selectedRobotId = robotId;
    if (m_selectedRobotId == robotId) return;

    m_selectedRobotId = robotId;

    // [핵심] 변수 변경 후 UI 강제 동기화 호출
    updateSelectionUI();

    qDebug() << "Robot Selected:" << robotId;
}

void wheelchair_admin::on_twRobotStatus_cellClicked(int row, int column) {
    QTableWidgetItem *item = ui->twRobotStatus->item(row, 0);
    if(item) selectRobot(item->text().toInt());
}

void wheelchair_admin::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateMapPixmapToLabel(); //창 크기 변화시 맵 재스케일+offset 재계산
    //refreshRobotTable();
}

void wheelchair_admin::onCustomContextMenuRequested(const QPoint &pos) {
    QTableWidgetItem *item = ui->twRobotStatus->itemAt(pos);
    if (!item) return;

    int row = item->row();
    // 0번 컬럼에 로봇 ID가 있다고 가정
    int robotId = ui->twRobotStatus->item(row, 0)->text().toInt();

    QMenu menu(this);

    // [수정] 변수명을 delAction으로 명확하게 통일했습니다.
    QAction *delAction = menu.addAction("삭제하기");

    // [신규] SSH 연결 액션 추가
    QAction *sshAction = menu.addAction("SSH 연결");

    menu.addSeparator(); // 구분선

    // 메뉴를 실행하고 선택된 액션을 받습니다.
    QAction *selectedItem = menu.exec(ui->twRobotStatus->mapToGlobal(pos));

    // 선택된 액션이 무엇인지 확인하고 처리
    if (selectedItem == delAction) {
        deleteRobotProcess(robotId);
    }
    else if (selectedItem == sshAction) {
        openSshDialog(robotId); // SSH 다이얼로그 호출
    }
}

void wheelchair_admin::deleteRobotProcess(int robotId) {
    if(QMessageBox::Yes == QMessageBox::question(this, "삭제", "삭제하시겠습니까?")) {
        DatabaseManager::instance().deleteRobot(robotId);
        if(m_selectedRobotId == robotId) m_selectedRobotId = -1;
        refreshAll();
    }
}

QString wheelchair_admin::selectedRobotId() const { return ui->cbSelectedRobot->currentText(); }

// [신규 기능] SSH 접속용 팝업창
void wheelchair_admin::openSshDialog(int robotId)
{
    // 1. 로봇 정보 조회 (캐시에서)
    if (!m_robotCache.contains(robotId)) {
        QMessageBox::warning(this, "오류", "로봇 정보를 찾을 수 없습니다.");
        return;
    }
    RobotInfo info = m_robotCache[robotId];

    // 2. 다이얼로그 생성
    QDialog dialog(this);
    dialog.setWindowTitle("SSH 연결");
    dialog.resize(300, 150);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 안내 라벨
    QLabel *label = new QLabel("접속할 주소 (User@IP):", &dialog);
    layout->addWidget(label);

    // 텍스트 입력창 (자동완성: name@ip)
    QLineEdit *lineEdit = new QLineEdit(&dialog);
    // 예: wc1@10.10.14.31 (만약 IP가 없으면 name만 뜸)
    QString defaultStr = QString("%1@%2").arg(info.name).arg(info.ip);
    lineEdit->setText(defaultStr);
    layout->addWidget(lineEdit);

    // 버튼 박스 (연결 / 취소)
    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    btnBox->button(QDialogButtonBox::Ok)->setText("연결");
    btnBox->button(QDialogButtonBox::Cancel)->setText("취소");
    layout->addWidget(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // 3. 다이얼로그 실행 및 결과 처리
    if (dialog.exec() == QDialog::Accepted) {
        QString sshTarget = lineEdit->text().trimmed();
        if (!sshTarget.isEmpty()) {
            launchSshTerminal(sshTarget);
        }
    }
}

// [신규 기능] 새 터미널 창을 열어서 SSH 실행
void wheelchair_admin::launchSshTerminal(const QString &sshTarget)
{
    // Ubuntu 기본 터미널인 gnome-terminal을 사용합니다.
    // 명령 형태: gnome-terminal -- ssh user@ip

    QString program = "gnome-terminal";
    QStringList arguments;

    // "--" 옵션은 이후 나오는 인자들을 터미널 내부에서 실행되는 명령어로 처리하라는 뜻입니다.
    arguments << "--" << "ssh" << sshTarget;

    // startDetached를 사용해야 현재 프로그램(Admin)과 별개로 터미널 창이 독립적으로 뜹니다.
    bool success = QProcess::startDetached(program, arguments);

    if (!success) {
        // gnome-terminal이 없는 경우 xterm 시도 (혹시 모르니)
        arguments.clear();
        arguments << "-e" << "ssh" << sshTarget;
        success = QProcess::startDetached("xterm", arguments);

        if (!success) {
            QMessageBox::critical(this, "실패", "터미널을 실행할 수 없습니다.\n(gnome-terminal 또는 xterm 필요)");
        }
    }
}

void wheelchair_admin::updateMapPixmapToLabel()
{
    if (m_mapOriginal.isNull()) return;

    const QSize labelSize = ui->lbMapPlaceholder->size();

    // KeepAspectRatio로 스케일된 픽스맵을 라벨에 표시
    QPixmap scaled = m_mapOriginal.scaled(labelSize, Qt::KeepAspectRatio, Qt::FastTransformation);
    ui->lbMapPlaceholder->setPixmap(scaled);

    // 실제 표시된 맵 크기, 레터박스 오프셋 계산 (중앙정렬 기준)
    m_mapDrawSize = scaled.size();
    m_mapOffset = QPoint((labelSize.width()  - scaled.width())  / 2,
                         (labelSize.height() - scaled.height()) / 2);
}

// ID에 맞춰 테이블, 콤보박스, 버튼의 상태를 즉시 동기화
void wheelchair_admin::updateSelectionUI() {
    if (m_selectedRobotId == -1) return;

    QString idStr = QString::number(m_selectedRobotId);

    // 1. 테이블 선택 동기화
    auto *t = ui->twRobotStatus;
    bool rowFound = false;
    t->blockSignals(true); // 무한 루프 방지
    for (int i = 0; i < t->rowCount(); ++i) {
        if (t->item(i, 0) && t->item(i, 0)->text() == idStr) {
            t->selectRow(i);
            rowFound = true;
            break;
        }
    }
    if (!rowFound) t->clearSelection();
    t->blockSignals(false);

    // 2. 콤보박스 선택 동기화
    auto *cb = ui->cbSelectedRobot;
    cb->blockSignals(true);
    int cbIdx = cb->findText(idStr);
    if (cbIdx != -1) cb->setCurrentIndex(cbIdx);
    cb->blockSignals(false);

    // 3. 맵 버튼 스타일 동기화 (즉시 반영)
    QList<int> keys = m_robotButtons.keys();
    for (int id : keys) {
        QPushButton *btn = m_robotButtons.value(id);
        if (!btn) continue;

        if (id == m_selectedRobotId) {
            btn->setStyleSheet("background-color: red; color: white; border-radius: 12px; border: 2px solid yellow;");
            btn->raise(); // 맨 위로 올리기
        } else {
            btn->setStyleSheet("background-color: #FF69B4; color: black; border-radius: 12px; border: 1px solid gray;");
        }
    }
}
