#ifndef TAB_ADMIN_H
#define TAB_ADMIN_H

#include <QWidget>

namespace Ui {
class tab_admin;
}

class tab_admin : public QWidget
{
    Q_OBJECT

public:
    explicit tab_admin(QWidget *parent = nullptr);
    ~tab_admin();

private:
    Ui::tab_admin *ui;
};

#endif // TAB_ADMIN_H
