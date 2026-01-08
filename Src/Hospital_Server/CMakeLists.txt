cmake_minimum_required(VERSION 3.10)
project(Hospital_Server C)

# 1. C 표준 설정 (C99)
set(CMAKE_C_STANDARD 99)

# 2. 헤더 파일 경로 설정 (헤더 파일들이 include 폴더나 현재 폴더에 있어야 함)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}) # 편의상 현재 루트도 포함

# 3. 소스 파일 목록 지정
# (주의: 실제 파일이 src 폴더 안에 없다면 'src/'를 지워주세요)
set(SOURCES
    src/main.c
    src/server_db.c
    src/robot_manager.c
)

# 4. 실행 파일 생성
add_executable(hospital_server ${SOURCES})

# ------------------------------------------------------------------
# [5. DB 라이브러리 설정 (MariaDB/MySQL)]
# ------------------------------------------------------------------
find_package(PkgConfig REQUIRED)

# Ubuntu/Debian 계열에서 라이브러리 찾기
pkg_check_modules(MARIADB QUIET libmariadb)
if(NOT MARIADB_FOUND)
    pkg_check_modules(MARIADB REQUIRED mysqlclient)
endif()

# 찾은 헤더 경로와 라이브러리 경로 연결
target_include_directories(hospital_server PRIVATE ${MARIADB_INCLUDE_DIRS})
target_link_directories(hospital_server PRIVATE ${MARIADB_LIBRARY_DIRS})

# ------------------------------------------------------------------
# [6. 라이브러리 링크 (핵심 수정 사항)]
# m       : 수학 라이브러리 (math.h의 sqrt, pow 사용 시 필수) -> gcc -lm 효과
# pthread : DB 클라이언트 스레드 사용 시 필수
# ------------------------------------------------------------------
target_link_libraries(hospital_server PRIVATE ${MARIADB_LIBRARIES} pthread m)

# ------------------------------------------------------------------
# [7. Python 스크립트 자동 복사]
# 빌드 후 실행 파일(hospital_server)이 있는 곳으로 .py 파일을 복사합니다.
# C 코드가 "./tcp_bridge.py"를 실행하므로 같은 폴더에 있어야 합니다.
# ------------------------------------------------------------------

# (1) tcp_bridge.py가 소스 루트(CMakeLists.txt 옆)에 있는 경우:
#file(COPY ${CMAKE_SOURCE_DIR}/tcp_bridge.py DESTINATION ${CMAKE_BINARY_DIR})

# (2) 만약 src 폴더 안에 있다면 위를 주석하고 아래를 푸세요:
file(COPY ${CMAKE_SOURCE_DIR}/src/tcp_bridge.py DESTINATION ${CMAKE_BINARY_DIR})

message(STATUS "Build Setup Complete. Math library (-lm) linked.")