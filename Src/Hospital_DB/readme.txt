--sql
sudo apt update
sudo apt upgrade -y
--maria DB
sudo apt install -y mariadb-server
-- 버전 확인
mysql -V
//MariaDB 보안 설정
sudo mariadb-secure-installation   --아래 참고 이미지 보기  

-- 디비 접속 비번 : 1234
sudo mariadb -u root -p    

--디비 생성
CREATE DATABASE hospital_db;
--사용자 생성
CREATE USER 'admin'@'%' IDENTIFIED BY '1234';
--권한
GRANT ALL PRIVILEGES ON hospital_db.* TO 'admin'@'%';
--즉시반영
FLUSH PRIVILEGES;

--테이블 만들기

-- 디비 선택
USE hospital_db;

-- 맵테이블
CREATE TABLE map_location (
    location_id  INT AUTO_INCREMENT PRIMARY KEY,
    name         VARCHAR(100) NOT NULL,
    floor        INT NOT NULL,
    x            DOUBLE NOT NULL,
    y            DOUBLE NOT NULL,
    theta        DOUBLE NOT NULL
);

-- 로봇정보
CREATE TABLE robot_status (
    robot_id           INT AUTO_INCREMENT PRIMARY KEY,
    name               VARCHAR(50) NOT NULL,

    op_status          ENUM('RUNNING','STOP','ERROR') NOT NULL, 
    battery_percent    TINYINT UNSIGNED NOT NULL,              
    is_charging        BOOLEAN NOT NULL DEFAULT 0,              

    current_x DOUBLE NOT NULL DEFAULT 0,
    current_y DOUBLE NOT NULL DEFAULT 0,                                    
    
    goal_x DOUBLE NULL DEFAULT NULL,
    goal_y DOUBLE NULL DEFAULT NULL,

    senser INT(11) NULL DEFAULT NULL,
    `order` TINYINT(3) UNSIGNED NOT NULL DEFAULT 0,
    `ip_address` VARCHAR(15) NULL DEFAULT NULL
);

 -- 로봇 이동/호출 데이터
CREATE TABLE robot_call_log (
    log_seq           INT AUTO_INCREMENT PRIMARY KEY,

    robot_id          INT NOT NULL,         
    user_name         VARCHAR(50) NOT NULL,  
    request_time      DATETIME NOT NULL,     

    start_location_id INT NOT NULL,            
    target_location_id INT NOT NULL,          

    CONSTRAINT fk_log_robot
      FOREIGN KEY (robot_id) REFERENCES robot_status(robot_id),
    CONSTRAINT fk_log_start_loc
      FOREIGN KEY (start_location_id) REFERENCES map_location(location_id),
    CONSTRAINT fk_log_target_loc
      FOREIGN KEY (target_location_id) REFERENCES map_location(location_id)
);

 -- 환자 테이블
CREATE TABLE patient (
    patient_id           INT AUTO_INCREMENT PRIMARY KEY,
    name                 VARCHAR(50) NOT NULL,
    disease              VARCHAR(50) NOT NULL,

    patient_type         ENUM('INPATIENT','OUTPATIENT') NOT NULL,  -- 입원/외래
    booking_date         DATETIME,                                -- 예약일

    room_location_id     INT,                                     -- 침실 위치 FK
    enter_date           DATETIME,
    leave_date           DATETIME,

    CONSTRAINT fk_patient_room
      FOREIGN KEY (room_location_id) REFERENCES map_location(location_id)
);

// 테이블 확인 
SHOW TABLES;

// 마리아 디비 나가기
exit;