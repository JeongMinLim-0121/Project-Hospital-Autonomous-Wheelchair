#ifndef KIOSK_MAIN_H
#define KIOSK_MAIN_H

#include <QWidget>

namespace Ui {
class kiosk_main;
}

class kiosk_main : public QWidget
{
    Q_OBJECT

public:
    explicit kiosk_main(QWidget *parent = nullptr);
    ~kiosk_main();

signals:
    void goWheel();
    void goSearch();

private:
    Ui::kiosk_main *ui;
};

#endif // KIOSK_MAIN_H
