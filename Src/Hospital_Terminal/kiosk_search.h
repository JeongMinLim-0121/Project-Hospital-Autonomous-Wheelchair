#ifndef KIOSK_SEARCH_H
#define KIOSK_SEARCH_H

#include <QWidget>

namespace Ui {
class kiosk_search;
}

class kiosk_search : public QWidget
{
    Q_OBJECT

public:
    explicit kiosk_search(QWidget *parent = nullptr);
    ~kiosk_search();

signals:
    void searchAccepted(QString name, QString id);
    void goBack();

protected:
    void showEvent(QShowEvent *event) override;

private:
    Ui::kiosk_search *ui;
};

#endif // KIOSK_SEARCH_H
