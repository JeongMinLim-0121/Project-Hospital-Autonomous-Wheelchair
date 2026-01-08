#include "search_medical.h"
#include "ui_search_medical.h"
#include "database_manager.h"

#include <QMessageBox>
#include <QDebug>
#include <QDate>

search_medical::search_medical(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::search_medical)
{
    ui->setupUi(this);

    // 1. UI 및 테이블 초기 설정
    initUi();

    // 2. DB에서 데이터 로드 (병명, 병동)
    loadDiseaseList();
    loadWardList(); // [중요] 프로그램 시작 시 DB에서 병동('5동')을 읽어옴

    // 3. 초기 상태: 입원 체크박스 상태에 따라 활성/비활성 동기화
    on_btnIN_toggled(ui->btnIN->isChecked());
}

search_medical::~search_medical()
{
    delete ui;
}

// =========================================================
// [초기화] 테이블 설정 및 시그널 연결
// =========================================================
void search_medical::initUi()
{
    // 1. 테이블 컬럼 및 헤더 설정
    ui->twPatientList->setColumnCount(7);
    QStringList headers;
    headers << "환자 ID" << "환자 이름" << "병명" << "응급여부" << "입원병동" << "침대번호" << "입원일";
    ui->twPatientList->setHorizontalHeaderLabels(headers);

    // 2. 테이블 스타일 설정
    ui->twPatientList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->twPatientList->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->twPatientList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->twPatientList->verticalHeader()->setVisible(false);
    ui->twPatientList->horizontalHeader()->setStretchLastSection(true);

    // 3. 병명 콤보박스 동기화 (한글 <-> 영문)
    connect(ui->cbDisease_KR, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index){
        if (index < 0) return;
        QString code = ui->cbDisease_KR->itemData(index).toString();
        int enIndex = ui->cbDisease_EN->findData(code);
        if (enIndex != -1 && enIndex != ui->cbDisease_EN->currentIndex()) {
            ui->cbDisease_EN->blockSignals(true);
            ui->cbDisease_EN->setCurrentIndex(enIndex);
            ui->cbDisease_EN->blockSignals(false);
        }
    });

    connect(ui->cbDisease_EN, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index){
        if (index < 0) return;
        QString code = ui->cbDisease_EN->itemData(index).toString();
        int krIndex = ui->cbDisease_KR->findData(code);
        if (krIndex != -1 && krIndex != ui->cbDisease_KR->currentIndex()) {
            ui->cbDisease_KR->blockSignals(true);
            ui->cbDisease_KR->setCurrentIndex(krIndex);
            ui->cbDisease_KR->blockSignals(false);
        }
    });

    // [추가] 병동 콤보박스 변경 시 -> 침대 번호 목록 갱신 연결
    connect(ui->cbWard, &QComboBox::currentTextChanged,
            this, &search_medical::on_cbWard_currentTextChanged);

    connect(ui->leSearchName, &QLineEdit::returnPressed,
                this, &search_medical::on_pbSearchPatient_clicked);
}

// =========================================================
// [데이터 로드] 병명 리스트
// =========================================================
void search_medical::loadDiseaseList()
{
    ui->cbDisease_KR->clear();
    ui->cbDisease_EN->clear();

    auto diseases = DatabaseManager::instance().getDiseaseList();

    for(auto it = diseases.begin(); it != diseases.end(); ++it) {
        QString code = it.key();
        QString nameKr = it.value().first;
        QString nameEn = it.value().second;

        ui->cbDisease_KR->addItem(nameKr, code);
        ui->cbDisease_EN->addItem(nameEn, code);
    }
}

// =========================================================
// [추가] 병동 리스트 로드 (DB의 '5동'을 가져옴)
// =========================================================
void search_medical::loadWardList()
{
    ui->cbWard->clear();
    // DatabaseManager::getWardList()는 SELECT * FROM TB_WARD 결과를 반환
    QStringList wards = DatabaseManager::instance().getWardList();

    if(wards.isEmpty()) {
        ui->cbWard->addItem("없음");
    } else {
        ui->cbWard->addItems(wards);
    }
    // 병동이 추가되면 currentTextChanged 시그널이 발생하여 침대 목록도 자동으로 로드됨
}

// =========================================================
// [추가] 병동 선택 시 침대 목록 갱신 (1~6번 로드)
// =========================================================
void search_medical::on_cbWard_currentTextChanged(const QString &ward)
{
    ui->cbBedNum->clear();
    if(ward.isEmpty() || ward == "없음") return;

    // DatabaseManager::getBedList()는 SELECT bed_num ... 결과를 반환
    QStringList beds = DatabaseManager::instance().getBedList(ward);
    ui->cbBedNum->addItems(beds);
}

// =========================================================
// [UI 이벤트] 입원 체크박스 토글 (비활성화 처리)
// =========================================================
void search_medical::on_btnIN_toggled(bool checked)
{
    // [수정] setVisible 대신 setEnabled 사용
    // 체크 해제 시: 화면에는 보이지만 회색 처리되어 클릭 불가

    if(ui->label_6) ui->label_6->setEnabled(checked);   // 라벨: 입원병동
    if(ui->cbWard)  ui->cbWard->setEnabled(checked);    // 콤보: 병동선택

    if(ui->label_7) ui->label_7->setEnabled(checked);   // 라벨: 침대번호
    if(ui->cbBedNum) ui->cbBedNum->setEnabled(checked); // 콤보: 침대번호

    if(ui->label_8) ui->label_8->setEnabled(checked);   // 라벨: 입원일
    if(ui->dtDate)  ui->dtDate->setEnabled(checked);    // 날짜 에디트
}

// =========================================================
// [기능] 환자 조회 버튼 클릭
// =========================================================
void search_medical::on_pbSearchPatient_clicked()
{
    QString searchName = ui->leSearchName->text();

    QList<PatientFullInfo> list = DatabaseManager::instance().searchPatients(searchName);

    ui->twPatientList->setRowCount(0);

    for(const auto &info : list) {
        int row = ui->twPatientList->rowCount();
        ui->twPatientList->insertRow(row);

        ui->twPatientList->setItem(row, 0, new QTableWidgetItem(info.id));
        ui->twPatientList->setItem(row, 1, new QTableWidgetItem(info.name));
        ui->twPatientList->setItem(row, 2, new QTableWidgetItem(info.diseaseNameKR));

        QString emergStr = info.isEmergency ? "Yes" : "No";
        QTableWidgetItem *emItem = new QTableWidgetItem(emergStr);
        emItem->setTextAlignment(Qt::AlignCenter);
        if(info.isEmergency) {
            emItem->setForeground(Qt::red);
            emItem->setFont(QFont("Ubuntu", 11, QFont::Bold));
        }
        ui->twPatientList->setItem(row, 3, emItem);

        ui->twPatientList->setItem(row, 4, new QTableWidgetItem(info.ward));
        QString bedStr = (info.bed > 0) ? QString::number(info.bed) : "";
        ui->twPatientList->setItem(row, 5, new QTableWidgetItem(bedStr));
        QString dateStr = info.admissionDate.isValid() ? info.admissionDate.toString("yyyy-MM-dd") : "";
        ui->twPatientList->setItem(row, 6, new QTableWidgetItem(dateStr));
    }
}

// =========================================================
// [기능] 테이블 클릭 시 -> 하단 폼에 정보 채우기
// =========================================================
void search_medical::on_twPatientList_cellClicked(int row, int column)
{
    Q_UNUSED(column);
    if (row < 0) return;

    QString id = ui->twPatientList->item(row, 0)->text();
    QString name = ui->twPatientList->item(row, 1)->text();
    QString diseaseName = ui->twPatientList->item(row, 2)->text();
    QString emStr = ui->twPatientList->item(row, 3)->text();
    QString ward = ui->twPatientList->item(row, 4)->text();
    QString bed = ui->twPatientList->item(row, 5)->text();
    QString dateStr = ui->twPatientList->item(row, 6)->text();

    ui->leID->setText(id);
    ui->leName_2->setText(name);

    int dIndex = ui->cbDisease_KR->findText(diseaseName);
    if (dIndex != -1) {
        ui->cbDisease_KR->setCurrentIndex(dIndex);
    } else {
        if(ui->cbDisease_KR->count() > 0)
            ui->cbDisease_KR->setCurrentIndex(0);
    }

    ui->btnEM->setChecked(emStr == "Yes");

    // 입원 여부 판단
    bool isInpatient = !ward.isEmpty();
    ui->btnIN->setChecked(isInpatient); // on_btnIN_toggled 호출됨 -> 활성/비활성 처리

    if (isInpatient) {
        // [중요] 병동을 먼저 세팅해야 침대 목록이 로드됨
        ui->cbWard->setCurrentText(ward);

        // 그 다음 침대 번호를 세팅
        ui->cbBedNum->setCurrentText(bed);

        QDate dt = QDate::fromString(dateStr, "yyyy-MM-dd");
        if(dt.isValid()) ui->dtDate->setDate(dt);
    }
}

// =========================================================
// [기능] 환자 추가 (DB INSERT)
// =========================================================
void search_medical::on_pbAddPatient_clicked()
{
    if(ui->leID->text().isEmpty() || ui->leName_2->text().isEmpty()) {
        QMessageBox::warning(this, "알림", "ID와 이름은 필수 입력사항입니다.");
        return;
    }

    PatientFullInfo info;
    info.id = ui->leID->text();
    info.name = ui->leName_2->text();
    info.diseaseCode = ui->cbDisease_KR->currentData().toString();
    info.isEmergency = ui->btnEM->isChecked();
    info.type = ui->btnIN->isChecked() ? "IN" : "OUT";

    if(info.type == "IN") {
        info.ward = ui->cbWard->currentText();
        info.bed = ui->cbBedNum->currentText().toInt();
        info.admissionDate = ui->dtDate->dateTime();
    }

    if(DatabaseManager::instance().addPatient(info)) {
        QMessageBox::information(this, "성공", "환자가 등록되었습니다.");
        on_pbSearchPatient_clicked();
        clearForm();
    } else {
        QMessageBox::critical(this, "오류", "등록 실패 (ID 중복 등을 확인하세요).");
    }
}

// =========================================================
// [기능] 환자 정보 수정 (DB UPDATE)
// =========================================================
void search_medical::on_pbUpdatePatient_clicked()
{
    if(ui->leID->text().isEmpty()) {
        QMessageBox::warning(this, "알림", "수정할 환자를 선택해주세요.");
        return;
    }

    PatientFullInfo info;
    info.id = ui->leID->text();
    info.name = ui->leName_2->text();
    info.diseaseCode = ui->cbDisease_KR->currentData().toString();
    info.isEmergency = ui->btnEM->isChecked();
    info.type = ui->btnIN->isChecked() ? "IN" : "OUT";

    if(info.type == "IN") {
        info.ward = ui->cbWard->currentText();
        info.bed = ui->cbBedNum->currentText().toInt();
        info.admissionDate = ui->dtDate->dateTime();
    }

    if(DatabaseManager::instance().updatePatient(info)) {
        QMessageBox::information(this, "성공", "정보가 수정되었습니다.");
        on_pbSearchPatient_clicked();
    } else {
        QMessageBox::critical(this, "오류", "수정 실패.");
    }
}

// =========================================================
// [기능] 환자 삭제 (DB DELETE)
// =========================================================
void search_medical::on_pbDeletePatient_clicked()
{
    QString id = ui->leID->text();
    if(id.isEmpty()) {
        QMessageBox::warning(this, "알림", "삭제할 환자를 선택해주세요.");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "삭제 확인",
                                  "정말 [" + ui->leName_2->text() + "] 환자 정보를 삭제하시겠습니까?",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if(DatabaseManager::instance().deletePatient(id)) {
            QMessageBox::information(this, "성공", "삭제되었습니다.");
            on_pbSearchPatient_clicked();
            clearForm();
        } else {
            QMessageBox::critical(this, "오류", "삭제 실패.");
        }
    }
}

// =========================================================
// [헬퍼] 입력 폼 초기화
// =========================================================
void search_medical::clearForm()
{
    ui->leID->clear();
    ui->leName_2->clear();
    ui->btnEM->setChecked(false);
    ui->btnIN->setChecked(false); // setEnabled(false) 처리됨

    if(ui->cbDisease_KR->count() > 0)
        ui->cbDisease_KR->setCurrentIndex(0);

    ui->dtDate->setDateTime(QDateTime::currentDateTime());
}
