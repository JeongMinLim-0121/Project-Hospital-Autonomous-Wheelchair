#ifndef LIVE_MONITOR_DIALOG_H
#define LIVE_MONITOR_DIALOG_H

#include <QDialog>
#include <QTableView>
#include <QSqlQueryModel>
#include <QTimer>
#include <QVBoxLayout>
#include <QHeaderView>

class LiveMonitorDialog : public QDialog
{
    Q_OBJECT

public:
    // 생성자에서 보고 싶은 테이블 이름을 받습니다.
    explicit LiveMonitorDialog(const QString &tableName, QWidget *parent = nullptr)
        : QDialog(parent), m_tableName(tableName)
    {
        // 1. UI 설정
        setWindowTitle("Live Monitor: " + m_tableName);
        resize(600, 400);

        // 메모리 자동 해제 (창 닫으면 객체 삭제)
        setAttribute(Qt::WA_DeleteOnClose);

        // 2. 모델 & 뷰 설정 (Qt의 SQL 전용 모델 사용)
        model = new QSqlQueryModel(this);
        view = new QTableView(this);
        view->setModel(model);
        view->setAlternatingRowColors(true); // 줄마다 색깔 다르게 보기 좋게
        view->horizontalHeader()->setStretchLastSection(true);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(view);

        // 3. 타이머 설정 (1초 = 1000ms)
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &LiveMonitorDialog::refreshData);
        timer->start(500); // 0.2초마다 실행

        // 처음에 한 번 로딩
        refreshData();
    }

private slots:
    void refreshData() {
        // 해당 테이블의 모든 데이터를 다시 긁어옵니다.
        // setQuery를 호출하면 모델이 알아서 뷰를 갱신합니다.
        model->setQuery("SELECT * FROM " + m_tableName);
    }

private:
    QString m_tableName;
    QTableView *view;
    QSqlQueryModel *model;
    QTimer *timer;
};

#endif // LIVE_MONITOR_DIALOG_H
