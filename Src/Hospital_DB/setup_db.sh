#!/bin/bash

# ==============================================================================
# [1. 설정 정보]
# ==============================================================================
DB_NAME="hospital_db"
DB_USER="root"
DB_PASS="1234"

FILE_BACKUP="hospital_backup.sql"
FILE_SCHEMA="hospital_schema.sql"
SELECTED_FILE=""

# ==============================================================================
# [2. 파일 선택 메뉴]
# ==============================================================================
echo "========================================================"
echo " 🏥 Hospital DB 설치 및 초기화 스크립트 (Fix Version)"
echo "========================================================"
echo "현재 디렉토리에서 SQL 파일을 확인했습니다."
echo ""
echo "  1) $FILE_BACKUP  (데이터 포함 복구)"
echo "  2) $FILE_SCHEMA  (테이블 구조만 초기화)"
echo ""
echo "========================================================"

while true; do
    read -p "진행할 번호를 선택하세요 (1 또는 2): " choice
    case $choice in
        1)
            if [ -f "$FILE_BACKUP" ]; then
                SELECTED_FILE="$FILE_BACKUP"
                break
            fi
            echo "❌ 오류: $FILE_BACKUP 파일이 없습니다."
            ;;
        2)
            if [ -f "$FILE_SCHEMA" ]; then
                SELECTED_FILE="$FILE_SCHEMA"
                break
            fi
            echo "❌ 오류: $FILE_SCHEMA 파일이 없습니다."
            ;;
        *) echo "잘못된 입력입니다." ;;
    esac
done

# ==============================================================================
# [3. MariaDB 설치 및 호환성 패치]
# ==============================================================================
if ! command -v mariadb &> /dev/null; then
    echo "📦 MariaDB 설치 중..."
    sudo apt update
    sudo apt install -y mariadb-server
    sudo systemctl start mariadb
    sudo systemctl enable mariadb
    
    echo "⏳ 서비스 안정화를 위해 5초 대기..."
    sleep 5
    echo "✅ MariaDB 설치 완료!"
else
    echo "✅ MariaDB가 이미 설치되어 있습니다."
fi

# ★ 핵심 수정: 최신 버전의 Collation(uca1400)을 구버전(general_ci)으로 강제 치환
echo "🔧 SQL 파일 버전 호환성 패치 중..."
sed -i 's/utf8mb4_uca1400_ai_ci/utf8mb4_general_ci/g' "$SELECTED_FILE"
echo "✅ 패치 완료 (uca1400 -> general_ci)"

# ==============================================================================
# [4. Root 계정 설정 및 DB 생성]
# ==============================================================================
echo "⚙️ DB 및 Root 계정 설정 중..."

# 1. DB 생성 (sudo 사용)
sudo mariadb -e "DROP DATABASE IF EXISTS $DB_NAME;"
sudo mariadb -e "CREATE DATABASE $DB_NAME DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;"

# 2. Root 비밀번호 및 권한 설정
# (이미 비번이 설정된 경우를 대비해 실패 시 무시하고 진행하도록 처리)
sudo mariadb -e "ALTER USER 'root'@'localhost' IDENTIFIED BY '$DB_PASS';" 2>/dev/null || true
sudo mariadb -e "GRANT ALL PRIVILEGES ON *.* TO 'root'@'%' IDENTIFIED BY '$DB_PASS' WITH GRANT OPTION;"
sudo mariadb -e "FLUSH PRIVILEGES;"

echo "✅ DB($DB_NAME) 생성 및 Root 권한 설정 완료!"

# ==============================================================================
# [5. 외부 접속 허용]
# ==============================================================================
CONFIG_FILE="/etc/mysql/mariadb.conf.d/50-server.cnf"
if grep -q "bind-address            = 127.0.0.1" "$CONFIG_FILE"; then
    echo "🌍 외부 접속 설정 변경 (127.0.0.1 -> 0.0.0.0)"
    sudo sed -i 's/bind-address            = 127.0.0.1/bind-address            = 0.0.0.0/' "$CONFIG_FILE"
    sudo systemctl restart mariadb
    echo "⏳ 재시작 대기 (3초)..."
    sleep 3
fi

# ==============================================================================
# [6. SQL 파일 적용]
# ==============================================================================
echo "📥 데이터 적용 시작 ($SELECTED_FILE)..."

# 비밀번호 명시하여 접속
mariadb -u root -p"$DB_PASS" $DB_NAME < "$SELECTED_FILE"

if [ $? -eq 0 ]; then
    echo ""
    echo "🎉 [성공] 모든 작업이 완료되었습니다!"
    echo "-----------------------------------------------------"
    echo " 🔹 DB명: $DB_NAME"
    echo " 🔹 ID  : root"
    echo " 🔹 PW  : $DB_PASS"
    echo "-----------------------------------------------------"
else
    echo "❌ [실패] 여전히 오류가 발생했습니다. SQL 파일을 확인해주세요."
fi