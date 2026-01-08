#ifndef KIOSK_CONTAINER_H
#define KIOSK_CONTAINER_H
#include <QMainWindow>
#include <QLineEdit>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class kiosk_container; }
QT_END_NAMESPACE

class kiosk_container : public QWidget
{
    Q_OBJECT

public:
    explicit kiosk_container(QMainWindow *mw, QWidget *parent = nullptr);
    ~kiosk_container();

private:
    Ui::kiosk_container *ui;
    QWidget *prevPage = nullptr;
    QMainWindow *mainWindow = nullptr;
};

#endif // KIOSK_CONTAINER_H
