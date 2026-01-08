#ifndef WHEELCHAIR_MAP_H
#define WHEELCHAIR_MAP_H

#include <QWidget>

namespace Ui {
class wheelchair_map;
}

class wheelchair_map : public QWidget
{
    Q_OBJECT

public:
    explicit wheelchair_map(QWidget *parent = nullptr);
    ~wheelchair_map();

private slots:
    // 엔터 버튼 클릭 시
    void on_pbEnter_clicked();

    // 입력창에서 엔터 키 눌렀을 때
    void on_pbLeInput_returnPressed();

private:
    Ui::wheelchair_map *ui;

    // 명령어 처리 함수
    void processCommand(QString cmd);

    // 터미널에 메시지 출력 헬퍼
    void printToTerminal(const QString &text, const QString &color = "black");
};

#endif // WHEELCHAIR_MAP_H
