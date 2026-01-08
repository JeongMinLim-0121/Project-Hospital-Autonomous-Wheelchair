#include "database_manager.h"
#include <QSqlRecord>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    closeDb();
}

bool DatabaseManager::deleteRobot(int id)
{
    QSqlQuery query;
    // SQL: robot_status 테이블에서 해당 id를 가진 행 삭제
    query.prepare("DELETE FROM robot_status WHERE robot_id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Delete robot failed:" << query.lastError();
        return false;
    }
    return true;
}

bool DatabaseManager::connectToDb()
{
    if(db.isOpen()) return true;

    db = QSqlDatabase::addDatabase("QMYSQL");
    // 라즈베리파이 IP (환경에 맞게 수정)
    db.setHostName("10.10.14.31");
    db.setPort(3306);
    db.setDatabaseName("hospital_db");
    
    // DB 접속 계정
    db.setUserName("root");
    db.setPassword("1234");

    if (!db.open()) {
        qDebug() << "DB Connect Error:" << db.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::isOpen()
{
    return db.isOpen();
}

void DatabaseManager::closeDb()
{
    if (db.isOpen()) db.close();
}

// [핵심 변경] 헤더와 똑같이 함수 이름과 반환형을 맞춰줍니다.
QString DatabaseManager::getUserRole(const QString &id, const QString &pw)
{
    if (!db.isOpen()) {
        if (!connectToDb()) return "";
    }

    QSqlQuery query;
    // role 컬럼도 같이 가져오도록 쿼리 수정
    query.prepare("SELECT role FROM users WHERE id = :id AND pw = :pw");
    query.bindValue(":id", id);
    query.bindValue(":pw", pw);

    if (query.exec()) {
        if (query.next()) {
            // 조회된 역할(role)을 문자열로 반환 (예: "admin", "medical")
            return query.value(0).toString();
        }
    }
    
    return ""; // 로그인 실패 시 빈 문자열 반환
}

bool DatabaseManager::addRobot(const QString &robot_name, const QString &ip, double x, double y, double theta)
{
    if (!db.isOpen()) {
        if (!connectToDb()) return false;
    }

    QSqlQuery query;

    // [중요 수정] 'order' (X) -> `order` (O)
    // 작은따옴표가 아니라 키보드 숫자 1 왼쪽에 있는 ` (백틱) 입니다.
    query.prepare("INSERT INTO robot_status "
                      "(name, ip_address, current_x, current_y, current_theta, op_status) "
                      "VALUES "
                      "(:name, :ip, :x, :y, :theta, 'STOP')");

    // 값 바인딩
    //query.bindValue(":id", id);
    query.bindValue(":name", robot_name);
    query.bindValue(":ip", ip);
    query.bindValue(":x", x);
    query.bindValue(":y", y);
    query.bindValue(":theta", theta);

    if (!query.exec()) {
        // 에러 확인용 로그
        qDebug() << "Insert Robot Error:" << query.lastError().text();
        return false;
    }
    return true;
}

QSqlQuery DatabaseManager::getRobotStatusQuery()
{
    // DB가 닫혀있으면 연결 시도
    if (!db.isOpen()) {
        if(!connectToDb()) {
            qDebug() << "DB is not open!";
            return QSqlQuery(); // 빈 쿼리 반환
        }
    }

    // [수정] 쿼리 객체 생성 시 db를 인자로 전달 (가장 안전함)
    QSqlQuery query(db);

    // 쿼리 실행
    if (!query.exec("SELECT * FROM robot_status")) {
        // 에러가 났다면 로그 출력
        qDebug() << "Select Query Error:" << query.lastError().text();
    } else {
        qDebug() << "Select Query Executed. Active:" << query.isActive();
    }

    return query;
}

bool DatabaseManager::updateRobotOrder(int id, unsigned char orderVal, QString &who)
{
    if (!db.isOpen()) {
        if (!connectToDb()) return false;
    }

    QSqlQuery query;
    // `order`는 SQL 예약어이므로 백틱(`)으로 감싸야 함
    query.prepare("UPDATE robot_status SET `order` = :val, who_called = :whois WHERE robot_id = :id");
    query.bindValue(":val", (int)orderVal);
    query.bindValue(":whois", who);
    query.bindValue(":id", id);


    if (!query.exec()) {
        qDebug() << "Update Order Failed:" << query.lastError().text();
        return false;
    }
    return true;
}

// 목적지 좌표와 order를 함께 업데이트 (직접 호출)
bool DatabaseManager::updateRobotGoal(int id, double x, double y, unsigned char orderVal, QString &who)
{
    if (!db.isOpen()) {
        if (!connectToDb()) return false;
    }

    QSqlQuery query;
    // goal_x, goal_y, order를 모두 업데이트
    query.prepare("UPDATE robot_status SET goal_x = :x, goal_y = :y, `order` = :val, who_called = :whois WHERE robot_id = :id");
    query.bindValue(":x", x);
    query.bindValue(":y", y);
    query.bindValue(":val", (int)orderVal);
    query.bindValue(":whois", who);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Update Goal Failed:" << query.lastError().text();
        return false;
    }
    return true;
}

//사용자 추가
bool DatabaseManager::addUser(const QString &id, const QString &pw, const QString &role, const QString &comment)
{
    QSqlQuery query;
    query.prepare("INSERT INTO users (id, pw, role, comment) VALUES (:id, :pw, :role, :comment)");
    query.bindValue(":id", id);
    query.bindValue(":pw", pw);
    query.bindValue(":role", role);
    query.bindValue(":comment", comment);

    if (!query.exec()) {
        qDebug() << "Insert Error:" << query.lastError().text();
        return false;
    }
    return true;
}

//사용자 수정
bool DatabaseManager::updateUser(const QString &id, const QString &pw, const QString &role, const QString &comment)
{
    QSqlQuery query;
    // ID를 조건으로 비밀번호, 권한, 비고 수정
    query.prepare("UPDATE users SET pw = :pw, role = :role, comment = :comment WHERE id = :id");
    query.bindValue(":pw", pw);
    query.bindValue(":role", role);
    query.bindValue(":comment", comment);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Update Error:" << query.lastError().text();
        return false;
    }
    return true;
}

//사용자 삭제
bool DatabaseManager::deleteUser(const QString &id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM users WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Delete Error:" << query.lastError().text();
        return false;
    }
    return true;
}

//사용자 목록 조회
QList<QStringList> DatabaseManager::getAllUsers()
{
    QList<QStringList> list;
    QSqlQuery query("SELECT id, pw, role, comment FROM users");

    while (query.next()) {
        QStringList rowData;
        rowData << query.value(0).toString(); // id
        rowData << query.value(1).toString(); // pw
        rowData << query.value(2).toString(); // role
        rowData << query.value(3).toString(); // comment
        list.append(rowData);
    }
    return list;
}

//질병 목록 가져오기
QMap<QString, QPair<QString, QString>> DatabaseManager::getDiseaseList()
{
    QMap<QString, QPair<QString, QString>> list;
    if (!db.isOpen()) connectToDb();

    // 병명 코드, 한글명, 영문명을 모두 가져옵니다.
    QSqlQuery query("SELECT disease_code, name_kr, name_en FROM disease_types");

    while (query.next()) {
        QString code = query.value(0).toString();
        QString nameKr = query.value(1).toString();
        QString nameEn = query.value(2).toString();

        // 코드(Key) -> {한글명, 영문명}(Value) 형태로 저장
        list.insert(code, qMakePair(nameKr, nameEn));
    }
    return list;
}

//환자 검색
QList<PatientFullInfo> DatabaseManager::searchPatients(const QString &searchName)
{
    QList<PatientFullInfo> list;
    QSqlQuery query;

    // [중요 수정] p.patient_type -> p.type 으로 변경했습니다.
    // DB 테이블 컬럼명이 'type'이기 때문입니다.
    QString sql = "SELECT p.patient_id, p.name, d.name_kr, p.is_emergency, "
                  "       p.type, p.ward, p.bed, p.admission_date "
                  "FROM patient_info p "
                  "LEFT JOIN disease_types d ON p.disease_code = d.disease_code ";

    // 검색어가 있으면 WHERE절 추가
    if (!searchName.isEmpty()) {
        sql += "WHERE p.name LIKE :search ";
    }

    // 정렬 (최근 입원순, 이름순 등 원하는대로)
    sql += "ORDER BY p.patient_id ASC";

    query.prepare(sql);

    if (!searchName.isEmpty()) {
        query.bindValue(":search", "%" + searchName + "%");
    }

    if (query.exec()) {
        while (query.next()) {
            PatientFullInfo info;
            // SELECT 순서대로 인덱싱 (0부터 시작)
            info.id = query.value(0).toString();          // p.patient_id
            info.name = query.value(1).toString();        // p.name
            info.diseaseNameKR = query.value(2).toString(); // d.name_kr (조인된 값)
            info.isEmergency = query.value(3).toBool();   // p.is_emergency

            // [확인] 여기가 4번째 인덱스입니다.
            info.type = query.value(4).toString();        // p.type (수정됨)

            info.ward = query.value(5).toString();        // p.ward
            info.bed = query.value(6).toInt();            // p.bed

            // 날짜 변환 (문자열 -> DateTime)
            QString dateStr = query.value(7).toString();
            if(!dateStr.isEmpty()) {
                // DB에 저장된 포맷에 맞춰 파싱 (예: "yyyy-MM-dd HH:mm:ss")
                // Qt 기본 SQL 변환으로 DateTime으로 바로 올 수도 있음
                info.admissionDate = query.value(7).toDateTime();
            }

            list.append(info);
        }
    } else {
        qDebug() << "Search Error:" << query.lastError().text();
    }

    return list;
}

//키오스크용 환자 검색
PatientFullInfo DatabaseManager::getPatientById(const QString &patientId, bool *ok)
{
    PatientFullInfo info;
    if (ok) *ok = false;

    if (!db.isOpen()) {
        if (!connectToDb()) return info;
    }

    QSqlQuery query;
    query.prepare(
        "SELECT p.patient_id, p.name, p.disease_code, d.name_kr, d.name_en, "
        "       p.is_emergency, p.type, p.ward, p.bed, p.admission_date "
        "FROM patient_info p "
        "LEFT JOIN disease_types d ON p.disease_code = d.disease_code "
        "WHERE p.patient_id = :pid"
        );
    query.bindValue(":pid", patientId);

    if (!query.exec()) {
        qDebug() << "getPatientById exec error:" << query.lastError().text();
        return info;
    }

    if (!query.next()) {
        // 조회 결과 없음
        return info;
    }

    info.id = query.value(0).toString();
    info.name = query.value(1).toString();
    info.diseaseCode = query.value(2).toString();
    info.diseaseNameKR = query.value(3).toString();
    info.diseaseNameEN = query.value(4).toString();
    info.isEmergency = query.value(5).toBool();
    info.type = query.value(6).toString();
    info.ward = query.value(7).toString();
    info.bed = query.value(8).toInt();
    info.admissionDate = query.value(9).toDateTime();

    if (ok) *ok = true;
    return info;
}


//환자 추가 (트랜잭션 중요)
bool DatabaseManager::addPatient(const PatientFullInfo &info)
{
    QSqlQuery query;

    // [핵심 수정 1] 테이블 이름이 'TB_PATIENT'가 아니라 'patient_info'여야 함
    // [핵심 수정 2] 컬럼 이름이 'patient_type'이 아니라 'type'이어야 함
    query.prepare("INSERT INTO patient_info "
                  "(patient_id, name, disease_code, is_emergency, type, ward, bed, admission_date) "
                  "VALUES "
                  "(:id, :name, :code, :emergency, :type, :ward, :bed, :date)");

    // 값 매핑
    query.bindValue(":id", info.id);
    query.bindValue(":name", info.name);
    query.bindValue(":code", info.diseaseCode);
    query.bindValue(":emergency", info.isEmergency ? 1 : 0);
    query.bindValue(":type", info.type); // "IN" 또는 "OUT"

    // 입원 환자("IN")인 경우에만 병동/침대/날짜 저장
    if (info.type == "IN") {
        query.bindValue(":ward", info.ward);
        query.bindValue(":bed", info.bed);
        // 날짜 저장 (DATETIME 형식에 맞춤)
        query.bindValue(":date", info.admissionDate.toString("yyyy-MM-dd HH:mm:ss"));
    } else {
        // 외래 환자("OUT")라면 NULL 처리
        query.bindValue(":ward", QVariant(QVariant::String)); // NULL
        query.bindValue(":bed", QVariant(QVariant::Int));     // NULL
        query.bindValue(":date", QVariant(QVariant::String)); // NULL
    }

    // 쿼리 실행 및 결과 확인
    if (query.exec()) {
        return true; // 성공
    } else {
        // [디버깅] 여기서 콘솔에 정확한 에러가 출력됩니다.
        qDebug() << "환자 추가 에러:" << query.lastError().text();
        return false; // 실패
    }
}

//환자 수정
bool DatabaseManager::updatePatient(const PatientFullInfo &info)
{
    QSqlQuery query;

    // [중요 수정사항]
    // 1. 테이블 이름: TB_PATIENT -> patient_info
    // 2. 컬럼 이름: patient_type (혹은 type) 확인 -> type
    // 3. WHERE 절: patient_id 기준
    QString sql = "UPDATE patient_info SET "
                  "name = :name, "
                  "disease_code = :code, "
                  "is_emergency = :emergency, "
                  "type = :type, "
                  "ward = :ward, "
                  "bed = :bed, "
                  "admission_date = :date "
                  "WHERE patient_id = :id";

    query.prepare(sql);

    // 값 바인딩 (순서는 상관없음, 이름표(:name 등)가 중요)
    query.bindValue(":name", info.name);
    query.bindValue(":code", info.diseaseCode);
    query.bindValue(":emergency", info.isEmergency ? 1 : 0);
    query.bindValue(":type", info.type); // "IN" or "OUT"
    query.bindValue(":id", info.id);     // WHERE 조건용

    // 입원 환자("IN")인 경우에만 병동/침대/날짜 저장
    if (info.type == "IN") {
        query.bindValue(":ward", info.ward);
        query.bindValue(":bed", info.bed);
        // 날짜를 문자열로 변환 (DB가 DATETIME이므로 포맷 지정)
        query.bindValue(":date", info.admissionDate.toString("yyyy-MM-dd HH:mm:ss"));
    } else {
        // 외래 환자("OUT")라면 NULL 처리
        query.bindValue(":ward", QVariant(QVariant::String)); // NULL
        query.bindValue(":bed", QVariant(QVariant::Int));     // NULL
        query.bindValue(":date", QVariant(QVariant::String)); // NULL
    }

    // 쿼리 실행
    if (query.exec()) {
        return true;
    } else {
        // [디버깅] 에러 원인을 콘솔에 출력 (이걸 봐야 정확한 이유를 알 수 있습니다)
        qDebug() << "Update Error:" << query.lastError().text();
        return false;
    }
}

//환자 삭제 (Cascade 설정되어 있으면 patient_info만 지워도 됨)
bool DatabaseManager::deletePatient(const QString &id)
{
    if (!db.isOpen()) connectToDb();

    // ON DELETE CASCADE가 DB에 설정되어 있다고 가정하고 부모만 삭제
    // 만약 안 되어 있다면 inpatient_details 먼저 삭제해야 함
    QSqlQuery query;
    query.prepare("DELETE FROM patient_info WHERE patient_id = :id");
    query.bindValue(":id", id);

    return query.exec();
}

QStringList DatabaseManager::getWardList()
{
    QStringList list;
    // 이미 만들어진 DB에서 긁어오기만 함
    QSqlQuery query("SELECT ward_name FROM TB_WARD ORDER BY ward_name ASC");

    while (query.next()) {
        list << query.value(0).toString();
    }
    return list;
}

// =============================================================
// [유지] 침대 목록 조회 (UI에서 긁어갈 때 사용)
// =============================================================
QStringList DatabaseManager::getBedList(const QString &ward)
{
    QStringList list;
    QSqlQuery query;

    // 이미 만들어진 DB에서 긁어오기만 함
    query.prepare("SELECT bed_num FROM TB_BED WHERE ward_name = :ward ORDER BY bed_num ASC");
    query.bindValue(":ward", ward);

    if (query.exec()) {
        while (query.next()) {
            list << query.value(0).toString();
        }
    }
    return list;
}

// 환자 이름 목록 가져오기
// =============================================================
QStringList DatabaseManager::getPatientNameList()
{
    QStringList list;
    if (!db.isOpen()) connectToDb();

    // 이름순으로 정렬해서 가져옴
    QSqlQuery query("SELECT name FROM patient_info ORDER BY name ASC");
    while (query.next()) {
        list << query.value(0).toString();
    }
    return list;
}

// =============================================================
// 휠체어 호출 대기열에 추가 (INSERT)
// =============================================================
bool DatabaseManager::addCallToQueue(const QString &name, const QString &start, const QString &dest)
{
    QSqlQuery query;
    // call_time은 DEFAULT CURRENT_TIMESTAMP이므로 생략 가능
    // is_dispatched(0), eta('-') 등은 기본값 활용
    query.prepare("INSERT INTO call_queue (caller_name, start_loc, dest_loc, is_dispatched, eta) "
                  "VALUES (:name, :start, :dest, 0, '-')");

    query.bindValue(":name", name);
    query.bindValue(":start", start);
    query.bindValue(":dest", dest);

    if (!query.exec()) {
        qDebug() << "Add Call Queue Error:" << query.lastError().text();
        return false;
    }
    return true;
}

// =============================================================
// 대기열 전체 목록 조회 (SELECT)
// =============================================================
QList<CallQueueItem> DatabaseManager::getCallQueue()
{
    QList<CallQueueItem> list;
    // 최근 호출한 순서(내림차순) 혹은 오래된 순서(오름차순) 중 선택. 여기선 최신이 위로 오게 함.
    QSqlQuery query("SELECT call_id, call_time, caller_name, start_loc, dest_loc, is_dispatched, eta "
                    "FROM call_queue ORDER BY call_time DESC");

    while (query.next()) {
        CallQueueItem item;
        item.call_id = query.value(0).toInt();

        // 시간 포맷 예쁘게 (YYYY-MM-DD hh:mm:ss)
        QDateTime dt = query.value(1).toDateTime();
        item.call_time = dt.toString("yyyy-MM-dd HH:mm:ss");

        item.caller_name = query.value(2).toString();
        item.start_loc = query.value(3).toString();
        item.dest_loc = query.value(4).toString();
        item.is_dispatched = query.value(5).toInt();
        item.eta = query.value(6).toString();

        list.append(item);
    }
    return list;
}

// =============================================================
// 호출 취소 (DELETE)
// =============================================================
bool DatabaseManager::deleteCall(int call_id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM call_queue WHERE call_id = :id");
    query.bindValue(":id", call_id);

    if (!query.exec()) {
        qDebug() << "Delete Call Error:" << query.lastError().text();
        return false;
    }
    return true;
}

//장소 이름으로 좌표 조회 (없으면 -1.0, -1.0 반환)
QPair<double, double> DatabaseManager::getLocation(const QString &locName)
{
    QPair<double, double> coord = qMakePair(-1.0, -1.0);

    if (!db.isOpen()) connectToDb();

    QSqlQuery query;
    query.prepare("SELECT x, y FROM map_location WHERE location_name = :name");
    query.bindValue(":name", locName);

    if (query.exec()) {
        if (query.next()) {
            coord.first = query.value(0).toDouble();  // x
            coord.second = query.value(1).toDouble(); // y
        }
    } else {
        qDebug() << "Get Location Error:" << query.lastError().text();
    }
    return coord;
}

// 테이블 이름을 받아서 HTML 표 형태로 반환하는 함수
QString DatabaseManager::getTableDataAsHtml(const QString &tableName)
{
    if (!db.isOpen()) connectToDb();

    // SQL Injection 방지를 위해 테이블 이름은 화이트리스트로 관리하거나 주의 필요
    // 여기서는 관리자 전용 툴이라 가정하고 진행
    QSqlQuery query(QString("SELECT * FROM %1").arg(tableName));

    if (!query.exec()) {
        return QString("<font color='red'>Error: %1</font>").arg(query.lastError().text());
    }

    // HTML 테이블 시작
    QString html = "<table border='1' cellspacing='0' cellpadding='5' style='border-collapse:collapse; width:100%;'>";

    // 1. 헤더 만들기
    QSqlRecord record = query.record();
    html += "<tr style='background-color: #444; color: white;'>";
    for(int i=0; i<record.count(); ++i) {
        html += QString("<th>%1</th>").arg(record.fieldName(i));
    }
    html += "</tr>";

    // 2. 데이터 행 만들기
    while(query.next()) {
        html += "<tr>";
        for(int i=0; i<record.count(); ++i) {
            QString val = query.value(i).toString();
            // NULL 값 처리
            if(val.isEmpty()) val = "-";
            html += QString("<td style='text-align:center;'>%1</td>").arg(val);
        }
        html += "</tr>";
    }
    html += "</table><br>";

    return html;
}

bool DatabaseManager::executeRawSql(const QString &sqlString, QString &errorMsg)
{
    if (!db.isOpen()) {
        if (!connectToDb()) {
            errorMsg = "DB Connection Failed";
            return false;
        }
    }

    QSqlQuery query;
    // 사용자가 입력한 쿼리 그대로 실행 (주의: 관리자용이므로 SQL Injection 신경 안 씀)
    if (!query.exec(sqlString)) {
        errorMsg = query.lastError().text();
        return false;
    }
    return true;
}

QString DatabaseManager::getQueryDataAsHtml(const QString &queryString)
{
    if (!db.isOpen()) connectToDb();

    QSqlQuery query;
    // 사용자가 입력한 쿼리 그대로 실행
    if (!query.exec(queryString)) {
        return QString("<font color='red'>SQL Error: %1</font>").arg(query.lastError().text());
    }

    // 결과가 Select가 아니거나 비어있을 수 있으므로 체크
    if (!query.isSelect()) {
        return QString("<font color='green'>Query Executed (Rows affected: %1)</font>").arg(query.numRowsAffected());
    }

    // HTML 테이블 시작
    QString html = "<table border='1' cellspacing='0' cellpadding='5' style='border-collapse:collapse; width:100%;'>";

    // 1. 헤더 만들기
    QSqlRecord record = query.record();
    html += "<tr style='background-color: #444; color: white;'>";
    for(int i=0; i<record.count(); ++i) {
        html += QString("<th>%1</th>").arg(record.fieldName(i));
    }
    html += "</tr>";

    // 2. 데이터 행 만들기
    int rowCount = 0;
    while(query.next()) {
        rowCount++;
        html += "<tr>";
        for(int i=0; i<record.count(); ++i) {
            QString val = query.value(i).toString();
            if(val.isEmpty()) val = "-";
            html += QString("<td style='text-align:center;'>%1</td>").arg(val);
        }
        html += "</tr>";
    }
    html += "</table><br>";

    if (rowCount == 0) return "No data found.";
    return html;
}
