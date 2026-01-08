#ifndef KIOSK_BACK_H
#define KIOSK_BACK_H

#include <QWidget>

namespace Ui {
class kiosk_back;
}

class kiosk_back : public QWidget
{
    Q_OBJECT

public:
    explicit kiosk_back(QWidget *parent = nullptr);
    ~kiosk_back();

signals:
    void goMain();

private:
    Ui::kiosk_back *ui;
};

#endif // KIOSK_BACK_H
