#include "wheelchair_map.h"
#include "ui_wheelchair_map.h"
#include "database_manager.h" // DB 매니저 포함
#include "live_monitor_dialog.h"
#include <QDateTime>
#include <QScrollBar>


wheelchair_map::wheelchair_map(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::wheelchair_map)
{
    ui->setupUi(this);

    // 터미널 초기 메시지
    ui->pbTerm->setHtml("<h3>=== Hospital Terminal System v1.0 ===</h3>"
                        "Type <b>'help'</b> to see available commands.<br>");

    // 입력창에 포커스
    ui->pbLeInput->setFocus();
}

wheelchair_map::~wheelchair_map()
{
    delete ui;
}

// [입력창 엔터]
void wheelchair_map::on_pbLeInput_returnPressed()
{
    on_pbEnter_clicked();
}

// [엔터 버튼 클릭]
void wheelchair_map::on_pbEnter_clicked()
{
    QString cmd = ui->pbLeInput->text().trimmed(); // 공백 제거
    if (cmd.isEmpty()) return;

    // 1. 사용자 입력 출력 (예: > ls robots)
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->pbTerm->append(QString("<font color='blue'>[%1] user$ %2</font>").arg(timestamp, cmd));

    // 2. 명령어 처리
    processCommand(cmd);

    // 3. 입력창 비우기
    ui->pbLeInput->clear();
    ui->pbLeInput->setFocus();
}

// [명령어 처리 로직]
void wheelchair_map::processCommand(QString cmd)
{
    // 소문자로 변환하여 비교 (대소문자 무시용)
    QString lowerCmd = cmd.toLower();
    // 공백 제거한 원본 (파싱용)
    QString cleanCmd = cmd.trimmed();

    // DB 인스턴스 가져오기
    DatabaseManager& db = DatabaseManager::instance();

    // =========================================================
    // [기능 1] 도움말 및 Clear
    // =========================================================
    if (lowerCmd == "help") {
        printToTerminal("<b>[Available Commands]</b><br>"
                        "- <b>ls [robots|patients|queue|users|maps]</b> : Show tables<br>"
                        "- <b>view [table_name]</b> : Open Live Monitor Popup (1s refresh)<br>"
                        "- <b>sql [query]</b> : Execute Raw SQL (Insert/Update/Select)<br>"
                        "- <b>clear</b> : Clear terminal screen<br>", "green");
    }
    else if (lowerCmd == "clear") {
        ui->pbTerm->clear();
        ui->pbTerm->setHtml("<h3>=== Hospital Terminal System v1.0 ===</h3>");
    }

    // =========================================================
    // [기능 2] 단순 조회 (ls 시리즈) - 기존 유지
    // =========================================================
    else if (lowerCmd == "ls robots") {
        printToTerminal("Querying Robot Status...", "gray");
        ui->pbTerm->insertHtml(db.getTableDataAsHtml("robot_status"));
    }
    else if (lowerCmd == "ls patients") {
        printToTerminal("Querying Patient Info...", "gray");
        ui->pbTerm->insertHtml(db.getTableDataAsHtml("patient_info"));
    }
    else if (lowerCmd == "ls queue") {
        printToTerminal("Querying Call Queue...", "gray");
        ui->pbTerm->insertHtml(db.getTableDataAsHtml("call_queue"));
    }
    else if (lowerCmd == "ls users") {
        printToTerminal("Querying Users...", "gray");
        ui->pbTerm->insertHtml(db.getTableDataAsHtml("users"));
    }
    else if (lowerCmd == "ls maps") {
        printToTerminal("Querying Map Locations...", "gray");
        ui->pbTerm->insertHtml(db.getTableDataAsHtml("map_location"));
    }

    // =========================================================
    // [기능 3] 실시간 팝업 모니터링 (view 명령어)
    // - 별명(robots)을 입력해도 실제 테이블(robot_status)로 연결해줌
    // =========================================================
    else if (lowerCmd.startsWith("view ")) {
        QStringList args = cleanCmd.split(" ", Qt::SkipEmptyParts);

        if (args.size() < 2) {
            printToTerminal("Error: Specify table name (e.g., view robots)", "red");
        } else {
            QString inputName = args[1]; // 사용자가 입력한 이름
            QString tableName = inputName; // 기본값은 입력한 그대로 사용

            // [핵심] 별명 매핑: 입력한 별명을 실제 DB 테이블 이름으로 변환
            if (inputName == "robots")        tableName = "robot_status";
            else if (inputName == "patients") tableName = "patient_info";
            else if (inputName == "queue")    tableName = "call_queue";
            else if (inputName == "users")    tableName = "users"; // users는 같음
            else if (inputName == "maps")     tableName = "map_location";

            // 팝업창 생성 (1초마다 갱신됨)
            // LiveMonitorDialog 헤더가 include 되어 있어야 함
            LiveMonitorDialog *monitor = new LiveMonitorDialog(tableName, this);
            monitor->show();

            printToTerminal("Launched Live Monitor for: " + tableName, "blue");
        }
    }

    // =========================================================
    // [기능 4] 만능 SQL 실행 (sql 명령어)
    // - SELECT 문이면 결과를 표로 출력
    // - INSERT/UPDATE/DELETE 문이면 성공/실패 여부만 출력
    // =========================================================
    else if (lowerCmd.startsWith("sql ")) {
        // "sql " (4글자) 뒤의 모든 문장을 쿼리로 인식
        QString sqlQuery = cleanCmd.mid(4);

        // 1. SELECT(조회) 쿼리인 경우
        if (lowerCmd.contains("select")) {
            QString resultHtml = db.getQueryDataAsHtml(sqlQuery);
            ui->pbTerm->insertHtml(resultHtml);
            printToTerminal("Query Executed.", "green");
        }
        // 2. 그 외(수정/삭제/삽입) 쿼리인 경우
        else {
            QString errorMsg;
            if (db.executeRawSql(sqlQuery, errorMsg)) {
                printToTerminal("SQL Executed Successfully.", "green");
                printToTerminal("Query: " + sqlQuery, "gray");
            } else {
                printToTerminal("SQL Failed: " + errorMsg, "red");
            }
        }
    }
    else if (lowerCmd == "show tables" || lowerCmd == "show tables;") {
        printToTerminal("Listing Tables...", "gray");
        // SHOW TABLES 명령어도 SELECT처럼 결과가 반환되므로 그대로 사용 가능
        QString html = db.getQueryDataAsHtml("SHOW TABLES");
        ui->pbTerm->insertHtml(html);
    }

    // =========================================================
    // [기타] 알 수 없는 명령어 처리
    // =========================================================
    else {
        printToTerminal("Unknown command: " + cmd, "red");
        printToTerminal("Type 'help' for usage.", "red");
    }

    // 터미널 스크롤을 항상 맨 아래로 이동
    ui->pbTerm->verticalScrollBar()->setValue(ui->pbTerm->verticalScrollBar()->maximum());
}

void wheelchair_map::printToTerminal(const QString &text, const QString &color)
{
    ui->pbTerm->append(QString("<font color='%1'>%2</font>").arg(color, text));
}
