#ifndef ADD_ROBOT_DIALOG_H
#define ADD_ROBOT_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>

class AddRobotDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddRobotDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("휠체어 등록");

        QFormLayout *layout = new QFormLayout(this);

        leIp = new QLineEdit(this);
        leName = new QLineEdit(this);
        sbX = new QDoubleSpinBox(this); sbX->setRange(-999, 999);
        sbY = new QDoubleSpinBox(this); sbY->setRange(-999, 999);
        sbTheta = new QDoubleSpinBox(this); sbTheta->setRange(-360, 360);

        layout->addRow("휠체어 이름:", leName);
        layout->addRow("IP 주소:", leIp);
        layout->addRow("X 좌표:", sbX);
        layout->addRow("Y 좌표:", sbY);
        layout->addRow("Theta:", sbTheta);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        layout->addWidget(buttonBox);

        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    QString getIp() const { return leIp->text(); }
    QString getName() const {return leName->text();}
    double getX() const { return sbX->value(); }
    double getY() const { return sbY->value(); }
    double getTheta() const { return sbTheta->value(); }

private:
    QLineEdit *leIp;
    QLineEdit *leName;
    QDoubleSpinBox *sbX;
    QDoubleSpinBox *sbY;
    QDoubleSpinBox *sbTheta;
};

#endif // ADD_ROBOT_DIALOG_H
