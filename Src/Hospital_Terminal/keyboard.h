#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QObject> // [필수 추가] QMetaObject 사용을 위해 필요

namespace Ui {
class Keyboard;
}

class Keyboard : public QWidget
{
    Q_OBJECT
    
public:
    explicit Keyboard(QWidget *parent = nullptr);
    ~Keyboard();
    
    // 타겟이 될 입력창을 지정하는 함수
    void setLineEdit(QLineEdit *line);
    
private slots:
    void keyboardHandler();         // 키 입력 처리
    void on_shift_clicked();        // Shift 키
    void on_char_2_toggled(bool checked); // 특수문자 키
    void on_enterButton_clicked();  // Enter 키
    void on_char_hangul_toggled(bool checked); // 한/영
    void on_backspaceButton_clicked();         // ⌫

private:
    Ui::Keyboard *ui;
    QLineEdit *outputLineEdit; // 입력할 대상 위젯
    bool shift;
    bool hangulMode;   // 한/영 상태
};

#endif // KEYBOARD_H