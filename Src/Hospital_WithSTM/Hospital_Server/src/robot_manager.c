/**
 * @file    robot_manager.c
 * @brief   로봇 프로세스(Python Bridge) 자동 실행 관리자 (Pure C)
 */

#include "robot_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h> // C99 bool 타입 지원

#define MAX_ROBOTS 100

// 내부 관리용 구조체
typedef struct {
    char name[64];
    pid_t pid;
    bool alive; // DB와 비교를 위한 체크 플래그
} RobotProc;

// 전역 변수로 관리 (이 프로세스 내에서만 유효)
static RobotProc process_list[MAX_ROBOTS];
static int proc_count = 0;

// [내부 함수] 프로세스 리스트에서 이름으로 인덱스 찾기
static int find_proc_index(const char* name) {
    for (int i = 0; i < proc_count; i++) {
        if (strncmp(process_list[i].name, name, 64) == 0) {
            return i;
        }
    }
    return -1;
}

// [내부 함수] 파이썬 브리지 실행 (Fork & Exec)
static pid_t spawn_python_bridge(const char* robot_name) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("[RobotManager] Fork failed");
        return -1;
    }

    if (pid == 0) {
        // [자식 프로세스]
        // 실행 명령: python3 ./tcp_bridge.py [robot_name]
        // 주의: tcp_bridge.py 경로가 정확해야 함
        execlp("python3", "python3", "./tcp_bridge.py", robot_name, "map_graph.json", (char *)NULL);
        
        // 실패 시
        perror("[RobotManager] execlp failed");
        exit(1);
    }
    
    return pid; // 부모에게 PID 반환
}

// [핵심 로직] DB와 프로세스 목록 동기화
void sync_processes_with_db(DBContext *ctx) {
    // 1. 모든 프로세스를 '사망 예정(false)'으로 초기화
    for (int i = 0; i < proc_count; i++) {
        process_list[i].alive = false;
    }

    // 2. DB 조회 (robot_status 테이블에 있는 로봇들은 실행되어야 함)
    if (mysql_query(ctx->conn, "SELECT name FROM robot_status")) {
        fprintf(stderr, "[RobotManager] DB Query Fail: %s\n", mysql_error(ctx->conn));
        return;
    }
    
    MYSQL_RES *res = mysql_store_result(ctx->conn);
    if (!res) return;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        if (!row[0]) continue;
        char *db_name = row[0];
        
        int idx = find_proc_index(db_name);

        if (idx != -1) {
            // [CASE A] 리스트에 이미 존재하는 경우
            // -> 실제로 프로세스가 살아있는지 OS에 확인 (kill(pid, 0))
            
            if (kill(process_list[idx].pid, 0) == 0) {
                // (1) 살아있음 -> 생존 마킹하고 유지
                process_list[idx].alive = true;
            } 
            else {
                // (2) 리스트엔 있는데 죽어있음 (좀비/자폭함) -> "부활(Respawn)" 시도
                printf("[RobotManager] ⚠️ Found dead process for '%s' (PID %d). Respawning...\n", 
                       db_name, process_list[idx].pid);
                
                // 기존 PID는 이미 죽었으므로 waitpid로 정리될 것임.
                // 즉시 새로운 프로세스 생성
                pid_t new_pid = spawn_python_bridge(db_name);
                
                if (new_pid > 0) {
                    process_list[idx].pid = new_pid; // PID 갱신
                    process_list[idx].alive = true;  // 생존 마킹
                    printf("[RobotManager] ♻️ Respawned '%s' (New PID: %d)\n", db_name, new_pid);
                } else {
                    fprintf(stderr, "[RobotManager] Failed to respawn '%s'\n", db_name);
                    // alive=false로 남아서 아래 3번 단계에서 리스트에서 삭제됨 (다음 루프 때 신규 생성 시도)
                }
            }
        } 
        else {
            // [CASE B] 리스트에 없음 (신규 로봇) -> 새로 실행 (Spawn)
            if (proc_count < MAX_ROBOTS) {
                printf("[RobotManager] ✨ New Robot found in DB: '%s'. Spawning...\n", db_name);
                pid_t new_pid = spawn_python_bridge(db_name);
                
                if (new_pid > 0) {
                    strncpy(process_list[proc_count].name, db_name, 63);
                    process_list[proc_count].pid = new_pid;
                    process_list[proc_count].alive = true;
                    proc_count++;
                }
            } else {
                fprintf(stderr, "[RobotManager] Max robots limit reached.\n");
            }
        }
    }
    mysql_free_result(res);

    // 3. DB에 없는(alive=false) 프로세스 종료 (Kill)
    //    (DB에서 삭제되었거나, 위에서 부활 실패한 경우 포함)
    int remaining_count = 0;
    RobotProc temp_list[MAX_ROBOTS]; // 살아남은 애들을 옮겨담을 임시 배열

    for (int i = 0; i < proc_count; i++) {
        if (!process_list[i].alive) {
            // DB에 없어서 종료해야 하는 경우
            // (이미 죽어있는 경우 kill은 -1을 반환하겠지만 안전함)
            printf("[RobotManager] 🚫 Robot '%s' removed from DB. Killing PID %d...\n", 
                   process_list[i].name, process_list[i].pid);
            
            kill(process_list[i].pid, SIGTERM);
            // waitpid는 메인 루프에서 처리
        } else {
            // 살아남은 프로세스 유지
            temp_list[remaining_count++] = process_list[i];
        }
    }

    // 리스트 갱신 (빈 공간 제거)
    memcpy(process_list, temp_list, sizeof(RobotProc) * remaining_count);
    proc_count = remaining_count;
}

// [공개 함수] 메인 루프
void run_robot_manager_loop() {
    DBContext db_ctx;
    
    if (db_open(&db_ctx) != 0) {
        fprintf(stderr, "[RobotManager] DB Connect Failed.\n");
        exit(1);
    }

    //프로세스 시작 전 최신 맵 파일 생성
    db_export_map_json(&db_ctx, "map_graph.json");

    printf("[RobotManager] Service Started (PID: %d)\n", getpid());

    while (1) {
        // [순서 변경] 1. 좀비 프로세스 먼저 정리 (청소 먼저!)
        while (waitpid(-1, NULL, WNOHANG) > 0);

        // [순서 변경] 2. 그 다음 DB 동기화 및 부활 확인
        sync_processes_with_db(&db_ctx);
        
        // 3. 주기 대기 (2초)
        sleep(2);
    }
    
    db_close(&db_ctx);
}