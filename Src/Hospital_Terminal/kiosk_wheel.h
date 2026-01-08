#ifndef KIOSK_WHEEL_H
#define KIOSK_WHEEL_H

#include <QWidget>
#include <QString>
#include <QTimer>

namespace Ui {
class kiosk_wheel;
}

class kiosk_wheel : public QWidget
{
    Q_OBJECT

public:
    explicit kiosk_wheel(QWidget *parent = nullptr);
    ~kiosk_wheel();

    void setPatientInfo(QString name, QString id);
signals:
    void wheelConfirmed();
    void goBack();

private:
    Ui::kiosk_wheel *ui;
    QString selectedDestination;
    QString m_patientID;
    QString m_patientName;

    //버튼 깜빡임 효과를 위한 타이머
    QTimer * blinkTimer;
    //현재 빨간색인지 아닌지 체크용
    bool isRed;
};

#endif // KIOSK_WHEEL_H
