#ifndef TAB_MEDICAL_H
#define TAB_MEDICAL_H

#include <QWidget>

namespace Ui {
class tab_medical;
}

class tab_medical : public QWidget
{
    Q_OBJECT

public:
    explicit tab_medical(QWidget *parent = nullptr);
    ~tab_medical();

private:
    Ui::tab_medical *ui;
};

#endif // TAB_MEDICAL_H
