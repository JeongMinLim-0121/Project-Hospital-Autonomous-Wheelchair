#ifndef SEARCH_MEDICAL_H
#define SEARCH_MEDICAL_H

#include <QWidget>

class QTableWidgetItem; // 전방 선언

namespace Ui {
class search_medical;
}

class search_medical : public QWidget
{
    Q_OBJECT

public:
    explicit search_medical(QWidget *parent = nullptr);
    ~search_medical();

private slots:
    // [수정] clicked -> toggled (체크박스 상태 변화 감지)
    void on_btnIN_toggled(bool checked);

    // [추가] 병동 콤보박스 값이 바뀌면 침대 목록을 갱신하는 슬롯
    void on_cbWard_currentTextChanged(const QString &arg1);

    // 기존 슬롯들
    void on_pbSearchPatient_clicked();
    void on_twPatientList_cellClicked(int row, int column);
    void on_pbAddPatient_clicked();
    void on_pbUpdatePatient_clicked();
    void on_pbDeletePatient_clicked();

private:
    Ui::search_medical *ui;

    void initUi();          // UI 초기화
    void loadDiseaseList(); // 병명 로드
    void loadWardList();    // [추가] 병동 목록 로드 (DB에서 '5동' 가져옴)
    void clearForm();       // 폼 초기화
};

#endif // SEARCH_MEDICAL_H
