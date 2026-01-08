#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include "keyboard.h" 
#include "login.h"

#include "tab_admin.h"
#include "tab_medical.h"
#include "kiosk_main.h"
#include "kiosk_search.h"
#include "kiosk_login.h"
#include "kiosk_wheel.h"
#include "kiosk_back.h"
#include "kiosk_container.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    // 키오스크모드 로그아웃버튼 숨김
    void setLogoutKioskMode(bool on);

    ~MainWindow();

protected:
    // 이벤트 필터: 사용자가 화면을 터치할 때마다 감시
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onLoginSuccess(QString role);
    void onLogoutClicked();

private:
    Ui::MainWindow *ui;
    // 가상 키보드
    Keyboard *virtualKeyboard = nullptr;
    
    // 로그인 페이지
    Login *loginPage;

    // Kiosk pages (stackedWidget용)
    // -----------------------------
    kiosk_main   *kioskMain   = nullptr;
    kiosk_search *kioskSearch = nullptr;
 //   kiosk_login  *kioskLogin  = nullptr;
    kiosk_wheel  *kioskWheel  = nullptr;
    kiosk_back   *kioskBack   = nullptr;

    // kiosk_wheel에서 뒤로가기 분기용 (search에서 왔는지 / login에서 왔는지)
    QString kioskPrevPage;

    // [핵심] 물리 키보드가 꽂혀있는지 확인하는 함수
    bool isPhysicalKeyboardConnected();
};
#endif // MAINWINDOW_H
