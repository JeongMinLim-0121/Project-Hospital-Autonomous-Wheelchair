#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QMap>

//환자 정보 구조체
struct PatientFullInfo {
    QString id;
    QString name;
    QString diseaseCode; // 병명 코드 (OS001 등)
    QString diseaseNameKR; // 한글 병명 (조회용)
    QString diseaseNameEN; // 영문 병명 (조회용)
    QString type;       // "IN" or "OUT"
    bool isEmergency;

    // 입원 환자용 상세 정보
    QString ward;
    QString room;       // 호실 (UI에 없다면 내부적으로만 처리하거나 고정값)
    int bed;
    QDateTime admissionDate;
};

//휠체어 대기열 정보 구조체
struct CallQueueItem {
    int call_id;
    QString call_time;
    QString caller_name;
    QString start_loc;
    QString dest_loc;
    int is_dispatched; // 0: 대기, 1: 배차완료
    QString eta;
};



class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager& instance();

    bool connectToDb();
    bool isOpen();
    void closeDb();

    //로봇 제어
    QString getUserRole(const QString &id, const QString &pw);
    bool addRobot(const QString &robot_name, const QString &ip, double x, double y, double theta);
    QSqlQuery getRobotStatusQuery();
    bool deleteRobot(int id);
    bool updateRobotOrder(int id, unsigned char orderVal, QString &who);
    bool updateRobotGoal(int id, double x, double y, unsigned char orderVal, QString &who);

    //USER ID/PW 조회, 수정, 삭제
    bool addUser(const QString &id, const QString &pw, const QString &role, const QString &comment);
    bool updateUser(const QString &id, const QString &pw, const QString &role, const QString &comment);
    bool deleteUser(const QString &id);
    QList<QStringList> getAllUsers(); // 모든 사용자 정보 반환 (테이블 출력용)

    //환자 관리용 함수들
    // 1. 환자 검색 (이름으로 검색, 빈칸이면 전체)
    QList<PatientFullInfo> searchPatients(const QString &name);
    // [추가] 환자번호(patient_id)로 정확 조회 (키오스크용)
    PatientFullInfo getPatientById(const QString &patientId, bool *ok = nullptr);

    // 2. 환자 추가 (트랜잭션)
    bool addPatient(const PatientFullInfo &info);

    // 3. 환자 수정 (트랜잭션)
    bool updatePatient(const PatientFullInfo &info);

    // 4. 환자 삭제
    bool deletePatient(const QString &id);

    // 5. 질병 목록 가져오기 (코드 -> {한글명, 영문명})
    // QMap<코드, QPair<한글, 영문>> 형태로 반환
    QMap<QString, QPair<QString, QString>> getDiseaseList();

    // 6. 병실 목록 가져오기 코드
    QStringList getWardList();
    QStringList getBedList(const QString &ward);


    //휠체어 대기열 관련 함수
    // 1. 환자 이름 목록 가져오기 (콤보박스용)
    QStringList getPatientNameList();

    // 2. 휠체어 호출 추가 (INSERT)
    bool addCallToQueue(const QString &name, const QString &start, const QString &dest);

    // 3. 대기열 목록 가져오기 (SELECT)
    QList<CallQueueItem> getCallQueue();

    // 4. 호출 취소 (DELETE)
    bool deleteCall(int call_id);

    //장소 이름으로 좌표 가져오기
    QPair<double, double> getLocation(const QString &locName);

    // 특정 테이블의 전체 데이터를 HTML 테이블 문자열로 반환
    QString getTableDataAsHtml(const QString &tableName);

    bool executeRawSql(const QString &sqlString, QString &errorMsg);
    QString getQueryDataAsHtml(const QString &queryString);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();
    QSqlDatabase db;
};

#endif // DATABASE_MANAGER_H
