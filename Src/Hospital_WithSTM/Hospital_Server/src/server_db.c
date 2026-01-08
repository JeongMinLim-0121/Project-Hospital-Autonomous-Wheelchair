/**
 * ============================================================================
 * @file    server_db.c
 * @brief   MariaDB/MySQL 데이터베이스 연동 모듈 (Final Version)
 * @details
 * - is_new_order 컬럼 제거 버전 (order > 0 조건 사용)
 * - 배차 시 출발지(Start)와 목적지(Goal) 좌표를 모두 계산하여 할당
 * - 상세한 한글 주석 포함
 * ============================================================================
 */

#include "server_db.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * [데이터베이스 접속 정보]
 * ============================================================================ */
#define DB_HOST "10.10.14.31"
#define DB_PORT 3306
#define DB_USER "admin"
#define DB_PASS "1234"
#define DB_NAME "hospital_db"

/* ============================================================================
 * [UPSERT 쿼리]
 * 로봇 상태 보고용 쿼리입니다.
 * 중복된 키(name)가 있으면 UPDATE, 없으면 INSERT를 수행합니다.
 * ============================================================================ */
static const char *SQL_UPSERT =
    "INSERT INTO robot_status "
    "(name, op_status, battery_percent, is_charging, current_x, current_y, current_theta, ultra_distance_cm, seat_detected) " // <-- 추가
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) " // <-- 물음표 2개 추가
    "ON DUPLICATE KEY UPDATE "
    "op_status=VALUES(op_status), "
    "battery_percent=VALUES(battery_percent), "
    "is_charging=VALUES(is_charging), "
    "current_x=VALUES(current_x), "
    "current_y=VALUES(current_y), "
    "current_theta=VALUES(current_theta), "
    "ultra_distance_cm=VALUES(ultra_distance_cm), " // <-- 추가
    "seat_detected=VALUES(seat_detected)";

/* ============================================================================
 * [함수: DB 연결 초기화]
 * ============================================================================ */
int db_open(DBContext *ctx)
{
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(*ctx)); // 구조체 초기화

    // 1. MySQL 핸들 생성
    ctx->conn = mysql_init(NULL);
    if (!ctx->conn) {
        fprintf(stderr, "[DB] mysql_init failed\n");
        return -1;
    }

    // 2. 재연결 옵션 설정 (네트워크 끊김 대비)
    my_bool reconnect = 1;
    mysql_options(ctx->conn, MYSQL_OPT_RECONNECT, &reconnect);

    // 3. DB 서버 연결
    if (!mysql_real_connect(ctx->conn, DB_HOST, DB_USER, DB_PASS, 
                            DB_NAME, DB_PORT, NULL, 0)) {
        fprintf(stderr, "[DB] connect error: %s\n", mysql_error(ctx->conn));
        mysql_close(ctx->conn);
        ctx->conn = NULL;
        return -1;
    }

    // 4. 문자셋 설정 (한글 지원)
    mysql_set_character_set(ctx->conn, "utf8mb4");

    // 5. UPSERT 쿼리 미리 준비 (Prepared Statement)
    ctx->upsert_stmt = mysql_stmt_init(ctx->conn);
    if (!ctx->upsert_stmt) {
        db_close(ctx);
        return -1;
    }

    if (mysql_stmt_prepare(ctx->upsert_stmt, SQL_UPSERT, strlen(SQL_UPSERT)) != 0) {
        fprintf(stderr, "[DB] stmt_prepare error: %s\n", mysql_stmt_error(ctx->upsert_stmt));
        db_close(ctx);
        return -1;
    }

    ctx->connected = 1;
    return 0;
}

/* ============================================================================
 * [함수: DB 연결 종료]
 * ============================================================================ */
void db_close(DBContext *ctx)
{
    if (!ctx) return;

    // Statement 해제
    if (ctx->upsert_stmt) {
        mysql_stmt_close(ctx->upsert_stmt);
        ctx->upsert_stmt = NULL;
    }
    
    // 연결 해제
    if (ctx->conn) {
        mysql_close(ctx->conn);
        ctx->conn = NULL;
    }
    ctx->connected = 0;
}

/* ============================================================================
 * [함수: 로봇 상태 저장 (UPSERT)]
 * 로봇이 보내온 하트비트 정보를 DB에 씁니다.
 * ============================================================================ */
int db_upsert_robot_status(DBContext *ctx, const char *name, const char *op_status,
                           uint32_t battery_percent, uint8_t is_charging,
                           double current_x, double current_y, double current_theta,
                           int ultra_dist, uint8_t seat_detected)
{
    if (!ctx || !ctx->connected || !ctx->upsert_stmt) return -1;

    // 파라미터 바인딩 구조체 준비 (7개)
    MYSQL_BIND b[9];
    memset(b, 0, sizeof(b));

    unsigned long name_len = (unsigned long)strlen(name);
    unsigned long st_len   = (unsigned long)strlen(op_status);

    // 1. Name
    b[0].buffer_type = MYSQL_TYPE_STRING; 
    b[0].buffer = (char*)name; 
    b[0].length = &name_len;
    
    // 2. Status
    b[1].buffer_type = MYSQL_TYPE_STRING; 
    b[1].buffer = (char*)op_status; 
    b[1].length = &st_len;
    
    // 3. Battery
    b[2].buffer_type = MYSQL_TYPE_LONG;   
    b[2].buffer = &battery_percent;
    
    // 4. Charging
    b[3].buffer_type = MYSQL_TYPE_TINY;   
    b[3].buffer = &is_charging;
    
    // 5. X
    b[4].buffer_type = MYSQL_TYPE_DOUBLE; 
    b[4].buffer = &current_x;
    
    // 6. Y
    b[5].buffer_type = MYSQL_TYPE_DOUBLE; 
    b[5].buffer = &current_y;
    
    // 7. Theta
    b[6].buffer_type = MYSQL_TYPE_DOUBLE; 
    b[6].buffer = &current_theta;

    // 8. Ultra Distance
    b[7].buffer_type = MYSQL_TYPE_LONG;
    b[7].buffer = &ultra_dist;

    // 9. Seat Detected
    b[8].buffer_type = MYSQL_TYPE_TINY;
    b[8].buffer = &seat_detected;

    if (mysql_stmt_bind_param(ctx->upsert_stmt, b) != 0) return -1;
    
    // 쿼리 실행
    if (mysql_stmt_execute(ctx->upsert_stmt) != 0) return -1;
    
    // 다음 실행을 위해 리셋
    mysql_stmt_reset(ctx->upsert_stmt);
    return 0;
}

/* ============================================================================
 * [함수: 새 명령 확인]
 * 로봇 핸들러(handle_client)가 호출합니다.
 * order 값이 0보다 크면 "명령이 있다"고 판단하여 모든 좌표를 반환합니다.
 * ============================================================================ */
int db_check_new_order(DBContext *ctx, const char *robot_name, 
                       int *order_out, 
                       double *sx, double *sy, 
                       double *gx, double *gy,
                       char *caller_out)
{
    if (!ctx || !ctx->connected) return -1;

    // [핵심] order > 0 조건을 사용하여 새 명령 감지
    const char *query = "SELECT `order`, start_x, start_y, goal_x, goal_y, who_called "
                        "FROM robot_status WHERE name = ? AND `order` > 0";

    MYSQL_STMT *stmt = mysql_stmt_init(ctx->conn);
    if (!stmt) return -1;

    if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0) {
        mysql_stmt_close(stmt);
        return -1;
    }

    // 파라미터 바인딩 (WHERE name = ?)
    MYSQL_BIND bind_param[1];
    memset(bind_param, 0, sizeof(bind_param));
    unsigned long name_len = strlen(robot_name);
    bind_param[0].buffer_type = MYSQL_TYPE_STRING;
    bind_param[0].buffer = (char*)robot_name;
    bind_param[0].length = &name_len;
    mysql_stmt_bind_param(stmt, bind_param);

    // 결과 바인딩 (6개 컬럼)
    MYSQL_BIND bind_res[6];
    memset(bind_res, 0, sizeof(bind_res));

    unsigned long caller_len = 0;
    
    bind_res[0].buffer_type = MYSQL_TYPE_LONG;   bind_res[0].buffer = order_out;
    bind_res[1].buffer_type = MYSQL_TYPE_DOUBLE; bind_res[1].buffer = sx;
    bind_res[2].buffer_type = MYSQL_TYPE_DOUBLE; bind_res[2].buffer = sy;
    bind_res[3].buffer_type = MYSQL_TYPE_DOUBLE; bind_res[3].buffer = gx;
    bind_res[4].buffer_type = MYSQL_TYPE_DOUBLE; bind_res[4].buffer = gy;

    bind_res[5].buffer_type = MYSQL_TYPE_STRING;
    bind_res[5].buffer = caller_out;
    bind_res[5].buffer_length = 64;
    bind_res[5].length = &caller_len;

    mysql_stmt_bind_result(stmt, bind_res);

    mysql_stmt_execute(stmt);
    mysql_stmt_store_result(stmt);

    int has_order = 0;
    // 결과가 있으면(fetch 성공) order > 0 이므로 명령 존재
    if (mysql_stmt_fetch(stmt) == 0) {
        has_order = 1;
    }

    mysql_stmt_close(stmt);
    return has_order;
}

/* ============================================================================
 * [함수: 명령 초기화]
 * 로봇이 명령을 수신한 후 호출하여 order를 0으로 되돌립니다.
 * ============================================================================ */
int db_reset_order(DBContext *ctx, const char *name) {
    if (!ctx || !ctx->connected) return -1;
    
    // order를 0으로 설정하여 명령 처리 완료 표시
    const char *query = "UPDATE robot_status SET `order` = 0 WHERE name = ?";
    
    MYSQL_STMT *stmt = mysql_stmt_init(ctx->conn);
    if (!stmt) return -1;
    mysql_stmt_prepare(stmt, query, strlen(query));

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    unsigned long name_len = strlen(name);
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)name;
    bind[0].length = &name_len;
    mysql_stmt_bind_param(stmt, bind);

    int ret = mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);
    return ret;
}

/* ============================================================================
 * [함수: 우선순위 호출 조회]
 * 대기 중인 호출 중 가장 급한 건을 가져옵니다.
 * ============================================================================ */
int db_get_priority_call(DBContext *ctx, int *call_id, char *start_loc, char *dest_loc, char *caller_name) {
    // 응급환자 > 질병점수 > 호출시간 순으로 정렬
    const char *sql = 
        "SELECT c.call_id, c.start_loc, c.dest_loc, c.caller_name "
        "FROM call_queue c "
        "LEFT JOIN patient_info p ON c.caller_name = p.name "
        "LEFT JOIN disease_types d ON p.disease_code = d.disease_code "
        "WHERE c.is_dispatched = 0 "
        "ORDER BY COALESCE(p.is_emergency, 0) DESC, COALESCE(d.base_priority, 0) DESC, c.call_time ASC "
        "LIMIT 1";

    if (mysql_query(ctx->conn, sql)) return -1;
    MYSQL_RES *res = mysql_store_result(ctx->conn);
    if (!res) return -1;

    MYSQL_ROW row = mysql_fetch_row(res);
    int found = 0;
    if (row) {
        *call_id = atoi(row[0]);
        // 안전한 문자열 복사
        if(row[1]) strncpy(start_loc, row[1], 63); else start_loc[0] = '\0';
        if(row[2]) strncpy(dest_loc, row[2], 63);  else dest_loc[0] = '\0';
        if(row[3]) strncpy(caller_name, row[3], 63); else caller_name[0] = '\0';
        found = 1;
    }
    mysql_free_result(res);
    return found;
}

/* ============================================================================
 * [함수: 가용 로봇 조회]
 * 쉬고 있고(WAITING), 배터리 충분하며(>20%), 현재 명령이 없는(order=0) 로봇 조회
 * ============================================================================ */
int db_get_available_robot(DBContext *ctx, char *robot_name) {
    const char *query = "SELECT name FROM robot_status "
                        "WHERE op_status = 'WAITING' "
                        "AND battery_percent > 20 "
                        "AND COALESCE(`order`, 0) = 0 " // 방금 명령받은 로봇은 제외
                        "LIMIT 1";

    if (mysql_query(ctx->conn, query)) return -1;
    MYSQL_RES *res = mysql_store_result(ctx->conn);
    if (!res) return -1;

    MYSQL_ROW row = mysql_fetch_row(res);
    int found = 0;
    if (row) {
        strncpy(robot_name, row[0], 63);
        robot_name[63] = '\0';
        found = 1;
    }
    mysql_free_result(res);
    return found;
}

/* ============================================================================
 * [함수: 장소 -> 좌표 변환]
 * map_location 테이블에서 (x, y) 좌표를 조회합니다.
 * ============================================================================ */
int db_get_location_coords(DBContext *ctx, const char *loc_name, double *x, double *y) {
    char query[512];
    snprintf(query, sizeof(query), "SELECT x, y FROM map_location WHERE location_name = '%s'", loc_name);

    if (mysql_query(ctx->conn, query)) return -1;
    MYSQL_RES *res = mysql_store_result(ctx->conn);
    if (!res) return -1;

    MYSQL_ROW row = mysql_fetch_row(res);
    int found = 0;
    if (row) {
        *x = atof(row[0]);
        *y = atof(row[1]);
        found = 1;
    }
    mysql_free_result(res);
    return found;
}

/* ============================================================================
 * [함수: 배차 실행 (DB 업데이트)]
 * 로봇 테이블에 명령(Order 6)과 좌표를 넣고, 호출 대기열을 '배차됨'으로 변경
 * ============================================================================ */
int db_assign_job_to_robot(DBContext *ctx, const char *robot_name, int call_id, 
                           double start_x, double start_y, 
                           double goal_x, double goal_y, 
                           const char *caller) 
{
    char query[1024];

    // 1) 로봇 상태 업데이트 (Order 6 부여)
    snprintf(query, sizeof(query),
             "UPDATE robot_status SET "
             "`order` = 6, "
             "start_x = %f, start_y = %f, "
             "goal_x = %f, goal_y = %f, "
             "op_status = 'HEADING', who_called = '%s' "
             "WHERE name = '%s'",
             start_x, start_y, goal_x, goal_y, caller, robot_name);
    
    if (mysql_query(ctx->conn, query)) {
        fprintf(stderr, "[Dispatch] Update Robot Failed: %s\n", mysql_error(ctx->conn));
        return -1;
    }

    // 2) 호출 대기열 업데이트 (배차 완료 처리)
    snprintf(query, sizeof(query),
             "UPDATE call_queue SET is_dispatched = 1, eta = '이동중' WHERE call_id = %d",
             call_id);
             
    if (mysql_query(ctx->conn, query)) return -1;

    return 0; // 성공
}

/* ============================================================================
 * [함수: 배차 사이클 메인]
 * 이 함수가 배차 관리자(Dispatch Manager)에 의해 주기적으로 호출됩니다.
 * ============================================================================ */
void db_process_dispatch_cycle(DBContext *ctx) {
    int call_id;
    char start_loc[64], dest_loc[64], caller[64];
    char robot_name[64];
    double start_x, start_y;
    double goal_x, goal_y;

    // 1. 가장 급한 호출 확인 (출발지, 목적지 정보 포함)
    if (db_get_priority_call(ctx, &call_id, start_loc, dest_loc, caller) == 1) {
        
        // 2. 일할 수 있는 로봇 찾기
        if (db_get_available_robot(ctx, robot_name) == 1) {
            
            // 3. 출발지와 목적지의 (x,y) 좌표 조회
            int s_ok = db_get_location_coords(ctx, start_loc, &start_x, &start_y);
            int d_ok = db_get_location_coords(ctx, dest_loc, &goal_x, &goal_y);

            // 두 좌표 모두 찾았을 때만 배차 진행
            if (s_ok == 1 && d_ok == 1) {
                printf("[Dispatch] Matched! Call #%d (%s -> %s) assigned to '%s'\n",
                       call_id, start_loc, dest_loc, robot_name);

                // 4. 배차 확정 및 DB 업데이트
                db_assign_job_to_robot(ctx, robot_name, call_id, start_x, start_y, goal_x, goal_y, caller);
                
            } else {
                printf("[Dispatch] Error: Unknown location '%s' or '%s' (Check map_location table)\n", start_loc, dest_loc);
            }
        } 
        // 로봇이 없으면 다음 사이클 대기
    }
}

/* ============================================================================
 * [함수: 로봇 존재 여부 확인]
 * DB에 해당 이름의 로봇이 있는지 확인 (1: 있음, 0: 없음)
 * ============================================================================ */
int db_check_robot_exists(DBContext *ctx, const char *name) {
    if (!ctx || !ctx->connected) return -1;

    const char *query = "SELECT 1 FROM robot_status WHERE name = ?";
    
    MYSQL_STMT *stmt = mysql_stmt_init(ctx->conn);
    if (!stmt) return -1;

    if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0) {
        mysql_stmt_close(stmt);
        return -1;
    }

    // 파라미터 바인딩
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    unsigned long name_len = strlen(name);
    
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (char*)name;
    bind[0].length = &name_len;

    if (mysql_stmt_bind_param(stmt, bind) != 0) {
        mysql_stmt_close(stmt);
        return -1;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        return -1;
    }

    mysql_stmt_store_result(stmt);
    
    // 행(Row)이 있으면 존재하는 것
    int exists = (mysql_stmt_num_rows(stmt) > 0) ? 1 : 0;

    mysql_stmt_close(stmt);
    return exists;
}

int db_export_map_json(DBContext *ctx, const char *filepath) {
    if (!ctx || !ctx->connected) return -1;

    FILE *fp = fopen(filepath, "w");
    if (!fp) {
        fprintf(stderr, "[DB] File open error: %s\n", filepath);
        return -1;
    }

    // ---------------------------------------------------------
    // 1. 노드(Nodes) 저장: "id": [x, y]
    // ---------------------------------------------------------
    fprintf(fp, "{ \"nodes\": {");
    
    if (mysql_query(ctx->conn, "SELECT node_id, x, y FROM tb_waypoints")) {
        fclose(fp); return -1;
    }
    
    MYSQL_RES *res = mysql_store_result(ctx->conn);
    MYSQL_ROW row;
    int first = 1;
    
    while ((row = mysql_fetch_row(res))) {
        if (!first) fprintf(fp, ",");
        fprintf(fp, "\"%s\": [%s, %s]", row[0], row[1], row[2]);
        first = 0;
    }
    mysql_free_result(res);

    // ---------------------------------------------------------
    // 2. 엣지(Edges) 저장: [node1, node2, distance]
    // ---------------------------------------------------------
    fprintf(fp, "}, \"edges\": [");

    const char *join_query = 
        "SELECT e.node1_id, e.node2_id, w1.x, w1.y, w2.x, w2.y "
        "FROM tb_edges e "
        "JOIN tb_waypoints w1 ON e.node1_id = w1.node_id "
        "JOIN tb_waypoints w2 ON e.node2_id = w2.node_id";

    if (mysql_query(ctx->conn, join_query)) {
        fclose(fp); return -1;
    }

    res = mysql_store_result(ctx->conn);
    first = 1;
    while ((row = mysql_fetch_row(res))) {
        double x1 = atof(row[2]); double y1 = atof(row[3]);
        double x2 = atof(row[4]); double y2 = atof(row[5]);
        double dist = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));

        if (!first) fprintf(fp, ",");
        fprintf(fp, "[%s, %s, %.4f]", row[0], row[1], dist);
        first = 0;
    }
    mysql_free_result(res);
    
    // ---------------------------------------------------------
    // [NEW] 3. 장소(Locations) 저장: "장소명": [x, y]
    // ---------------------------------------------------------
    fprintf(fp, "], \"locations\": {");

    if (mysql_query(ctx->conn, "SELECT location_name, x, y FROM map_location")) {
        fclose(fp); return -1;
    }

    res = mysql_store_result(ctx->conn);
    first = 1;
    while ((row = mysql_fetch_row(res))) {
        if (!first) fprintf(fp, ",");
        // JSON 문자열 처리를 위해 장소명에 따옴표 추가
        fprintf(fp, "\"%s\": [%s, %s]", row[0], row[1], row[2]);
        first = 0;
    }
    mysql_free_result(res);

    fprintf(fp, "} }");
    fclose(fp);
    printf("[DB] Map (Nodes+Edges+Locations) exported to '%s'\n", filepath);
    return 0;
}