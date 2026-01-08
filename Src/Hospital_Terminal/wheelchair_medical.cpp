#include "wheelchair_medical.h"
#include "ui_wheelchair_medical.h"
#include "database_manager.h"

#include <QMessageBox>
#include <QDebug>
#include <QTableWidgetItem>

wheelchair_medical::wheelchair_medical(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::wheelchair_medical)
{
    ui->setupUi(this);

    // 1. 테이블 초기화
    initTable();

    // 2. 콤보박스 데이터 채우기 (환자, 장소)
    initUi();

    // 3. 타이머 설정 (1초마다 DB 조회하여 테이블 갱신)
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &wheelchair_medical::updateQueueTable);
    updateTimer->start(1000); // 1000ms = 1초

    // 시작하자마자 한 번 갱신
    updateQueueTable();
}

wheelchair_medical::~wheelchair_medical()
{
    if(updateTimer->isActive()) updateTimer->stop();
    delete ui;
}

// =========================================================
// [초기화] 테이블 컬럼 설정
// =========================================================
void wheelchair_medical::initTable()
{
    // 컬럼: 호출시간, 환자이름, 호출위치, 목적지, 배차여부, ETA (6개)
    // call_id는 숨겨서 관리할 예정
    ui->twReservation->setColumnCount(6);
    QStringList headers;
    headers << "호출 시간" << "환자 이름" << "호출 위치" << "목적지" << "배차 여부" << "ETA";
    ui->twReservation->setHorizontalHeaderLabels(headers);

    ui->twReservation->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->twReservation->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->twReservation->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->twReservation->verticalHeader()->setVisible(false);
    ui->twReservation->horizontalHeader()->setStretchLastSection(true);

    ui->twReservation->setColumnWidth(0, 160); // 호출 시간 (조금 넓게)
    ui->twReservation->setColumnWidth(1, 100); // 환자 이름
    ui->twReservation->setColumnWidth(2, 120); // 호출 위치
    ui->twReservation->setColumnWidth(3, 120); // 목적지
    ui->twReservation->setColumnWidth(4, 100); // 배차 여부
    ui->twReservation->setColumnWidth(5, 100); // ETA

    // 만약 전체 너비를 꽉 채우고 싶다면 헤더의 ResizeMode를 설정할 수도 있습니다.
    // 예: 모든 컬럼을 균등하게 배분
    // ui->twReservation->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 또는 특정 컬럼만 늘어나게 하기 (예: ETA만 늘리기)
    ui->twReservation->horizontalHeader()->setStretchLastSection(true);
}

// =========================================================
// [초기화] 콤보박스 데이터 설정 (하드코딩 + DB연동)
// =========================================================
void wheelchair_medical::initUi()
{
    // 1. 출발지/도착지 하드코딩
    QStringList locations = {"키오스크", "식당", "정형외과", "재활의학과", "입원실"};

    ui->cbSelectStart->clear();
    ui->cbSelectStart->addItems(locations);

    ui->cbSelectEnd->clear();
    ui->cbSelectEnd->addItems(locations);
    // 출발지와 목적지가 겹치지 않게 기본값 설정 (예: 출발=입원실, 도착=재활)
    if(locations.size() > 3) ui->cbSelectEnd->setCurrentIndex(3);

    // 2. 환자 이름 콤보박스 (DB에서 가져오기)
    ui->cbSelectPatient->clear();
    QStringList patients = DatabaseManager::instance().getPatientNameList();
    if (patients.isEmpty()) {
        ui->cbSelectPatient->addItem("환자 없음");
    } else {
        ui->cbSelectPatient->addItems(patients);
    }
}

// =========================================================
// [기능] 휠체어 호출 (DB INSERT)
// =========================================================
void wheelchair_medical::on_pbSendWheelchair_clicked()
{
    // 입력값 가져오기
    QString name = ui->cbSelectPatient->currentText();
    QString start = ui->cbSelectStart->currentText();
    QString dest = ui->cbSelectEnd->currentText();

    // 유효성 검사
    if (name.isEmpty() || name == "환자 없음") {
        QMessageBox::warning(this, "알림", "환자를 선택해주세요.");
        return;
    }
    if (start == dest) {
        QMessageBox::warning(this, "알림", "출발지와 목적지가 같습니다.");
        return;
    }

    // DB에 추가
    if (DatabaseManager::instance().addCallToQueue(name, start, dest)) {
        QMessageBox::information(this, "성공", "휠체어 호출이 등록되었습니다.");
        updateQueueTable(); // 즉시 갱신
    } else {
        QMessageBox::critical(this, "오류", "호출 등록 실패.");
    }
}

// =========================================================
// [기능] 호출 취소 (DB DELETE)
// =========================================================
// 버튼 이름을 pbCancel 로 가정 (Qt Designer에서 확인 필요)
void wheelchair_medical::on_pbCancel_clicked()
{
    int currentRow = ui->twReservation->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "알림", "취소할 호출을 선택해주세요.");
        return;
    }

    // 선택된 행의 첫 번째 아이템에서 call_id 추출 (UserRole에 저장해둠)
    QTableWidgetItem *item = ui->twReservation->item(currentRow, 0);
    int callId = item->data(Qt::UserRole).toInt();

    // 삭제 확인
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "취소 확인", "선택한 호출을 취소하시겠습니까?",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (DatabaseManager::instance().deleteCall(callId)) {
            // 성공 시 테이블 갱신
            updateQueueTable();
        } else {
            QMessageBox::critical(this, "오류", "삭제 실패.");
        }
    }
}

// =========================================================
// [기능] 테이블 실시간 업데이트 (SELECT & Refresh)
// =========================================================
void wheelchair_medical::updateQueueTable()
{

    updatePatientList();
    // 1. 현재 선택된 행의 call_id 기억하기
    int selectedCallId = -1;
    int currentRow = ui->twReservation->currentRow();

    if (currentRow >= 0) {
        // 선택된 행이 있다면 call_id를 가져옴 (UserRole에 숨겨둔 값)
        QTableWidgetItem *item = ui->twReservation->item(currentRow, 0);
        if (item) {
            selectedCallId = item->data(Qt::UserRole).toInt();
        }
    }

    // 2. DB에서 새로운 목록 가져오기
    QList<CallQueueItem> list = DatabaseManager::instance().getCallQueue();

    // 3. 테이블 내용 비우고 새로 채우기
    ui->twReservation->setRowCount(0); // 여기서 기존 선택이 다 날아감

    for (const auto &item : list) {
        int row = ui->twReservation->rowCount();
        ui->twReservation->insertRow(row);

        // 0: 호출 시간 & call_id 저장
        QTableWidgetItem *timeItem = new QTableWidgetItem(item.call_time);
        timeItem->setData(Qt::UserRole, item.call_id);
        ui->twReservation->setItem(row, 0, timeItem);

        // 1: 환자 이름
        ui->twReservation->setItem(row, 1, new QTableWidgetItem(item.caller_name));

        // 2: 호출 위치
        ui->twReservation->setItem(row, 2, new QTableWidgetItem(item.start_loc));

        // 3: 목적지
        ui->twReservation->setItem(row, 3, new QTableWidgetItem(item.dest_loc));

        // 4: 배차 여부
        QString status = (item.is_dispatched == 1) ? "배차완료" : "대기중";
        QTableWidgetItem *statusItem = new QTableWidgetItem(status);
        if (item.is_dispatched == 1) {
            statusItem->setForeground(Qt::blue);
        } else {
            statusItem->setForeground(Qt::red);
        }
        statusItem->setTextAlignment(Qt::AlignCenter);
        ui->twReservation->setItem(row, 4, statusItem);

        // 5: ETA
        ui->twReservation->setItem(row, 5, new QTableWidgetItem(item.eta));

        // [핵심] 아까 기억해둔 ID와 현재 행의 ID가 같으면 다시 선택
        if (selectedCallId != -1 && item.call_id == selectedCallId) {
            ui->twReservation->selectRow(row);
        }
    }
}

// =========================================================
// [기능] 환자 명단 실시간 동기화 (변동사항 있을 때만 갱신)
// =========================================================
void wheelchair_medical::updatePatientList()
{
    // 1. DB에서 최신 환자 명단 가져오기
    QStringList dbList = DatabaseManager::instance().getPatientNameList();
    if (dbList.isEmpty()) dbList << "환자 없음";

    // 2. 현재 콤보박스에 있는 명단 가져오기
    QStringList currentList;
    for(int i=0; i < ui->cbSelectPatient->count(); ++i) {
        currentList << ui->cbSelectPatient->itemText(i);
    }

    // 3. [핵심] 두 리스트가 다를 때만 갱신 (그래야 깜빡임 방지)
    if (dbList != currentList) {
        // 현재 선택된 환자 이름 기억해두기 (갱신 후 복구용)
        QString selectedName = ui->cbSelectPatient->currentText();

        ui->cbSelectPatient->blockSignals(true); // 불필요한 시그널 차단
        ui->cbSelectPatient->clear();
        ui->cbSelectPatient->addItems(dbList);

        // 아까 선택했던 이름이 여전히 목록에 있다면 다시 선택
        int index = ui->cbSelectPatient->findText(selectedName);
        if (index != -1) {
            ui->cbSelectPatient->setCurrentIndex(index);
        }
        ui->cbSelectPatient->blockSignals(false);
    }
}
