#include "kiosk_search.h"
#include "ui_kiosk_search.h"
#include "database_manager.h" // [필수] DB 매니저 헤더 포함
#include <QShowEvent>
#include <QMessageBox>
#include <QTableWidgetItem>

kiosk_search::kiosk_search(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::kiosk_search)
{
    ui->setupUi(this);

    // =================================================
    // 1. 마스코트 이미지 설정
    // =================================================
    QPixmap pm(":/icons/HJK_2.png");
    ui->label->setPixmap(pm);
    ui->label->setScaledContents(true);
    ui->label->setAlignment(Qt::AlignCenter);

    // ---------------------------------
    // 테이블 초기 상태 설정
    // ---------------------------------
    ui->table_basic->clearSelection();
    ui->table_basic->setSelectionBehavior(QAbstractItemView::SelectRows);   // 행 단위 선택
    ui->table_basic->setSelectionMode(QAbstractItemView::SingleSelection);  // 하나만 선택 가능
    ui->table_basic->setEditTriggers(QAbstractItemView::NoEditTriggers);    // 수정 불가능하게

    // [변경] 더미 데이터 삭제 -> 초기엔 빈 테이블로 시작
    ui->table_basic->setRowCount(0);

    // 헤더 설정 (이름, ID, 병동, 병명)
    QStringList headers;
    headers << "이름" << "환자번호" << "병동/위치" << "병명";
    ui->table_basic->setColumnCount(4);
    ui->table_basic->setHorizontalHeaderLabels(headers);
    // 컬럼 너비 조정 (보기 좋게)
    ui->table_basic->horizontalHeader()->setStretchLastSection(true);


    // ---------------------------------
    // 뒤로가기 → 메인
    // ---------------------------------
    connect(ui->btn_back, &QPushButton::clicked,
            this, &kiosk_search::goBack);

    // ---------------------------------
    // [핵심] 검색 버튼 클릭 시 DB 조회
    // ---------------------------------
    connect(ui->btn_search, &QPushButton::clicked, this, [=]() {

        QString patientId = ui->edit_search->text().trimmed();

        if (patientId.isEmpty()) {
            QMessageBox::warning(this, "알림", "환자번호를 입력해주세요.");
            return;
        }

        bool ok = false;
        PatientFullInfo info =
            DatabaseManager::instance().getPatientById(patientId, &ok);

        if (!ok) {
            QMessageBox::information(this, "알림", "조회된 환자가 없습니다.");
            ui->table_basic->setRowCount(0);
            return;
        }

        // 결과 1명 표시 (선택은 사용자가)
        ui->table_basic->setRowCount(1);

        ui->table_basic->setItem(0, 0, new QTableWidgetItem(info.name));
        ui->table_basic->setItem(0, 1, new QTableWidgetItem(info.id));

        QString location = (info.type == "IN") ? info.ward : "외래";
        ui->table_basic->setItem(0, 2, new QTableWidgetItem(location));

        ui->table_basic->setItem(0, 3, new QTableWidgetItem(info.diseaseNameKR));

        for (int c = 0; c < 4; c++) {
            if (ui->table_basic->item(0, c))
                ui->table_basic->item(0, c)->setTextAlignment(Qt::AlignCenter);
        }
    });


    // ---------------------------------
    // [핵심] 선택 환자 → 휠체어 호출 (다음 화면으로 데이터 전달)
    // ---------------------------------
    connect(ui->btn_call, &QPushButton::clicked, this, [=]() {
        // 1. 선택 확인
        if (!ui->table_basic->selectionModel()->hasSelection()) {
            QMessageBox::warning(this, "알림", "호출할 환자를 목록에서 선택해주세요.");
            return;
        }

        // 2. 선택된 행(Row) 정보 가져오기
        int currentRow = ui->table_basic->currentRow();

        // 이름 (0번 컬럼), ID (1번 컬럼) 가져오기
        QString name = ui->table_basic->item(currentRow, 0)->text();
        QString id   = ui->table_basic->item(currentRow, 1)->text();

        // 3. 다음 화면으로 신호 전송 (인자 포함)
        emit searchAccepted(name, id);
    });
}

void kiosk_search::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    ui->edit_search->clear();

    ui->table_basic->setRowCount(0);
}

kiosk_search::~kiosk_search()
{
    delete ui;
}
