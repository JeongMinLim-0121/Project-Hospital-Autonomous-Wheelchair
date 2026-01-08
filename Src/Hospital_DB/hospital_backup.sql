/*M!999999\- enable the sandbox mode */ 
-- MariaDB dump 10.19-11.8.3-MariaDB, for debian-linux-gnu (aarch64)
--
-- Host: localhost    Database: hospital_db
-- ------------------------------------------------------
-- Server version	11.8.3-MariaDB-0+deb13u1 from Debian

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*M!100616 SET @OLD_NOTE_VERBOSITY=@@NOTE_VERBOSITY, NOTE_VERBOSITY=0 */;

--
-- Table structure for table `TB_BED`
--

DROP TABLE IF EXISTS `TB_BED`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `TB_BED` (
  `ward_name` varchar(50) NOT NULL,
  `bed_num` int(11) NOT NULL,
  PRIMARY KEY (`ward_name`,`bed_num`),
  CONSTRAINT `TB_BED_ibfk_1` FOREIGN KEY (`ward_name`) REFERENCES `TB_WARD` (`ward_name`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `TB_BED`
--

LOCK TABLES `TB_BED` WRITE;
/*!40000 ALTER TABLE `TB_BED` DISABLE KEYS */;
set autocommit=0;
INSERT INTO `TB_BED` VALUES
('5동',1),
('5동',2),
('5동',3),
('5동',4),
('5동',5),
('5동',6);
/*!40000 ALTER TABLE `TB_BED` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `TB_WARD`
--

DROP TABLE IF EXISTS `TB_WARD`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `TB_WARD` (
  `ward_name` varchar(50) NOT NULL,
  PRIMARY KEY (`ward_name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `TB_WARD`
--

LOCK TABLES `TB_WARD` WRITE;
/*!40000 ALTER TABLE `TB_WARD` DISABLE KEYS */;
set autocommit=0;
INSERT INTO `TB_WARD` VALUES
('5동');
/*!40000 ALTER TABLE `TB_WARD` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `call_queue`
--

DROP TABLE IF EXISTS `call_queue`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `call_queue` (
  `call_id` int(11) NOT NULL AUTO_INCREMENT,
  `call_time` datetime DEFAULT current_timestamp(),
  `caller_name` varchar(50) DEFAULT NULL,
  `start_loc` varchar(50) DEFAULT NULL,
  `dest_loc` varchar(50) DEFAULT NULL,
  `is_dispatched` int(11) DEFAULT 0,
  `eta` varchar(50) DEFAULT NULL,
  PRIMARY KEY (`call_id`)
) ENGINE=InnoDB AUTO_INCREMENT=328 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `call_queue`
--

LOCK TABLES `call_queue` WRITE;
/*!40000 ALTER TABLE `call_queue` DISABLE KEYS */;
set autocommit=0;
INSERT INTO `call_queue` VALUES
(205,'2025-12-31 12:34:28','민성쿤','정형외과','재활의학과',1,'이동중'),
(206,'2025-12-31 12:48:55','민성쿤','정형외과','재활의학과',1,'이동중'),
(207,'2025-12-31 12:50:17','민성쿤','정형외과','재활의학과',1,'이동중'),
(208,'2025-12-31 12:54:01','민성쿤','키오스크','재활의학과',1,'이동중'),
(209,'2025-12-31 12:55:04','안창회','입원실','정형외과',1,'이동중'),
(210,'2025-12-31 13:22:33','안창회','재활의학과','키오스크',1,'이동중'),
(211,'2025-12-31 13:22:39','서민솔','정형외과','입원실',1,'이동중'),
(212,'2025-12-31 13:22:46','서민솔','키오스크','식당',1,'이동중'),
(213,'2025-12-31 13:49:16','리두현','입원실','정형외과',1,'이동중'),
(214,'2025-12-31 13:51:30','리두현','입원실','정형외과',1,'이동중'),
(215,'2025-12-31 13:55:17','나지훈','식당','정형외과',1,'이동중'),
(216,'2025-12-31 13:59:23','나지훈','재활의학과','입원실',1,'이동중'),
(217,'2025-12-31 14:09:34','나지훈','재활의학과','입원실',1,'이동중'),
(218,'2025-12-31 14:14:33','나지훈','재활의학과','입원실',1,'이동중'),
(219,'2025-12-31 14:33:58','나지훈','재활의학과','입원실',1,'이동중'),
(220,'2025-12-31 14:40:57','리두현','정형외과','입원실',1,'이동중'),
(221,'2025-12-31 14:43:39','민성쿤','키오스크','입원실',1,'이동중'),
(222,'2025-12-31 14:48:14','민성쿤','키오스크','입원실',1,'이동중'),
(223,'2025-12-31 14:48:54','민성쿤','키오스크','입원실',1,'이동중'),
(224,'2025-12-31 14:54:10','민성쿤','키오스크','입원실',1,'이동중'),
(225,'2025-12-31 14:55:40','서민솔','정형외과','키오스크',1,'이동중'),
(226,'2025-12-31 15:03:41','외래환자','키오스크','정형외과',1,'이동중'),
(227,'2025-12-31 15:04:42','외래환자','키오스크','식당',1,'이동중'),
(228,'2025-12-31 15:07:05','외래환자','키오스크','입원실',1,'이동중'),
(229,'2025-12-31 15:15:00','강송구','키오스크','재활의학과',1,'이동중'),
(230,'2025-12-31 15:28:03','서민솔','식당','재활의학과',1,'이동중'),
(231,'2025-12-31 15:32:43','서민솔','식당','재활의학과',1,'이동중'),
(232,'2025-12-31 15:47:08','외래환자','키오스크','재활의학과',1,'이동중'),
(233,'2025-12-31 16:04:30','서민솔','식당','재활의학과',1,'이동중'),
(234,'2025-12-31 16:14:30','서민솔','재활의학과','식당',1,'이동중'),
(235,'2025-12-31 16:15:22','안창회','재활의학과','입원실',1,'이동중'),
(236,'2025-12-31 16:18:15','안창회','정형외과','키오스크',1,'이동중'),
(237,'2025-12-31 16:18:23','외래환자','키오스크','재활의학과',1,'이동중'),
(238,'2025-12-31 18:02:32','안창회','정형외과','키오스크',1,'이동중'),
(239,'2025-12-31 18:03:49','외래환자','키오스크','재활의학과',1,'이동중'),
(240,'2025-12-31 18:04:05','외래환자','키오스크','식당',1,'이동중'),
(241,'2025-12-31 18:04:22','리두현','식당','재활의학과',1,'이동중'),
(243,'2025-12-31 18:07:47','외래환자','키오스크','정형외과',1,'이동중'),
(245,'2025-12-31 18:15:05','리두현','정형외과','재활의학과',1,'이동중'),
(276,'2025-12-31 20:47:07','강송구','재활의학과','키오스크',1,'이동중'),
(277,'2025-12-31 20:51:07','강송구','식당','키오스크',1,'이동중'),
(278,'2025-12-31 20:51:20','문두르','재활의학과','입원실',1,'이동중'),
(279,'2025-12-31 20:59:15','외래환자','키오스크','입원실',1,'이동중'),
(280,'2025-12-31 21:00:18','문두르','식당','정형외과',1,'이동중'),
(281,'2025-12-31 21:01:43','문두르','재활의학과','키오스크',1,'이동중'),
(282,'2025-12-31 21:02:53','문두르','입원실','정형외과',1,'이동중'),
(283,'2025-12-31 21:03:05','리두현','재활의학과','입원실',1,'이동중'),
(285,'2026-01-02 12:20:59','강송구','키오스크','재활의학과',1,'이동중'),
(286,'2026-01-02 12:27:57','강송구','정형외과','재활의학과',1,'이동중'),
(287,'2026-01-02 12:29:11','강송구','키오스크','입원실',1,'이동중'),
(288,'2026-01-02 12:30:29','외래환자','키오스크','입원실',1,'이동중'),
(289,'2026-01-02 12:32:18','외래환자','키오스크','재활의학과',1,'이동중'),
(290,'2026-01-02 13:01:19','외래환자','키오스크','재활의학과',1,'이동중'),
(291,'2026-01-02 13:14:01','서채건','키오스크','입원실',1,'이동중'),
(292,'2026-01-02 13:15:05','강송구','키오스크','입원실',1,'이동중'),
(293,'2026-01-02 13:16:30','외래환자','키오스크','입원실',1,'이동중'),
(294,'2026-01-02 13:17:07','강송구','재활의학과','입원실',1,'이동중'),
(295,'2026-01-02 13:18:55','외래환자','키오스크','식당',1,'이동중'),
(296,'2026-01-02 18:52:04','나지훈','키오스크','재활의학과',1,'이동중'),
(297,'2026-01-02 21:19:46','강송구','키오스크','재활의학과',1,'이동중'),
(298,'2026-01-02 21:51:47','강송구','키오스크','재활의학과',1,'이동중'),
(299,'2026-01-02 22:35:43','강송구','정형외과','입원실',1,'이동중'),
(302,'2026-01-03 15:59:27','나지훈','키오스크','입원실',1,'이동중'),
(303,'2026-01-03 16:00:00','외래환자','키오스크','재활의학과',1,'이동중'),
(305,'2026-01-03 16:17:51','유종민','정형외과','식당',1,'이동중'),
(308,'2026-01-03 17:34:21','나지훈','식당','키오스크',1,'이동중'),
(309,'2026-01-03 17:39:44','나지훈','키오스크','정형외과',1,'이동중'),
(310,'2026-01-03 19:05:26','강송구','키오스크','재활의학과',1,'이동중'),
(311,'2026-01-03 19:53:00','강송구','키오스크','재활의학과',1,'이동중'),
(312,'2026-01-03 20:14:36','강송구','정형외과','식당',1,'이동중'),
(313,'2026-01-03 20:59:32','강송구','정형외과','식당',1,'이동중'),
(314,'2026-01-03 21:02:05','강송구','정형외과','입원실',1,'이동중'),
(315,'2026-01-03 21:52:33','강송구','재활의학과','식당',1,'이동중'),
(316,'2026-01-03 21:55:06','강송구','정형외과','입원실',1,'이동중'),
(317,'2026-01-03 22:14:00','강송구','재활의학과','정형외과',1,'이동중'),
(318,'2026-01-03 22:20:15','강송구','식당','정형외과',1,'이동중'),
(319,'2026-01-03 22:24:14','나지훈','키오스크','입원실',1,'이동중'),
(320,'2026-01-04 11:35:12','강송구','키오스크','재활의학과',1,'이동중'),
(321,'2026-01-04 11:37:13','강송구','키오스크','재활의학과',1,'이동중'),
(322,'2026-01-04 11:54:22','강송구','키오스크','재활의학과',1,'이동중'),
(323,'2026-01-04 11:56:05','강송구','키오스크','재활의학과',1,'이동중'),
(324,'2026-01-04 12:07:22','강송구','키오스크','재활의학과',1,'이동중'),
(325,'2026-01-04 13:43:19','강송구','키오스크','재활의학과',1,'이동중'),
(326,'2026-01-04 14:21:13','강송구','키오스크','재활의학과',1,'이동중'),
(327,'2026-01-04 14:32:23','강송구','키오스크','재활의학과',1,'이동중');
/*!40000 ALTER TABLE `call_queue` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `disease_types`
--

DROP TABLE IF EXISTS `disease_types`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `disease_types` (
  `disease_code` varchar(10) NOT NULL,
  `name_kr` varchar(50) NOT NULL,
  `name_en` varchar(50) DEFAULT NULL,
  `base_priority` int(11) DEFAULT 1,
  PRIMARY KEY (`disease_code`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `disease_types`
--

LOCK TABLES `disease_types` WRITE;
/*!40000 ALTER TABLE `disease_types` DISABLE KEYS */;
set autocommit=0;
INSERT INTO `disease_types` VALUES
('GEN01','일반 물리치료','Physical Therapy',1),
('OS001','대퇴골/고관절 골절','Hip Fracture',10),
('OS002','하지 골절(종아리/발목)','Leg Fracture',8),
('OS003','허리 디스크(중증)','Herniated Disc',6),
('OS004','십자인대 파열','ACL Rupture',6),
('OS005','퇴행성 관절염','Arthritis',4),
('OS006','단순 타박상/염좌','Contusion/Sprain',2),
('RM001','뇌졸중/편마비','Stroke/Hemiplegia',10),
('RM002','척수 손상','Spinal Cord Injury',9),
('RM003','수술 후 재활','Post-op Rehab',7);
/*!40000 ALTER TABLE `disease_types` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `inpatient_details`
--

DROP TABLE IF EXISTS `inpatient_details`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `inpatient_details` (
  `patient_id` varchar(20) NOT NULL,
  `ward_number` varchar(10) DEFAULT NULL,
  `room_number` varchar(10) DEFAULT NULL,
  `bed_number` int(11) DEFAULT NULL,
  `admission_date` datetime DEFAULT current_timestamp(),
  PRIMARY KEY (`patient_id`),
  CONSTRAINT `inpatient_details_ibfk_1` FOREIGN KEY (`patient_id`) REFERENCES `patient_info` (`patient_id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `inpatient_details`
--

LOCK TABLES `inpatient_details` WRITE;
/*!40000 ALTER TABLE `inpatient_details` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `inpatient_details` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Temporary table structure for view `map_location`
--

DROP TABLE IF EXISTS `map_location`;
/*!50001 DROP VIEW IF EXISTS `map_location`*/;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8mb4;
/*!50001 CREATE VIEW `map_location` AS SELECT
 1 AS `location_id`,
  1 AS `location_name`,
  1 AS `x`,
  1 AS `y` */;
SET character_set_client = @saved_cs_client;

--
-- Table structure for table `patient_info`
--

DROP TABLE IF EXISTS `patient_info`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `patient_info` (
  `patient_id` varchar(20) NOT NULL,
  `name` varchar(50) NOT NULL,
  `disease_code` varchar(20) DEFAULT NULL,
  `is_emergency` int(11) DEFAULT 0,
  `type` varchar(10) DEFAULT 'OUT',
  `ward` varchar(50) DEFAULT NULL,
  `bed` int(11) DEFAULT NULL,
  `admission_date` datetime DEFAULT NULL,
  PRIMARY KEY (`patient_id`),
  KEY `disease_code` (`disease_code`),
  CONSTRAINT `patient_info_ibfk_1` FOREIGN KEY (`disease_code`) REFERENCES `disease_types` (`disease_code`) ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `patient_info`
--

LOCK TABLES `patient_info` WRITE;
/*!40000 ALTER TABLE `patient_info` DISABLE KEYS */;
set autocommit=0;
INSERT INTO `patient_info` VALUES
('P1001','서채건','RM002',1,'OUT',NULL,NULL,NULL),
('P1002','리두현','RM001',0,'IN','5동',1,'2025-12-22 11:04:51'),
('P1003','홍진경','OS001',0,'IN','5동',2,'2025-12-18 21:35:34'),
('P1004','민성쿤','RM003',0,'OUT',NULL,NULL,NULL),
('P1005','나지훈','RM001',1,'IN','5동',3,'2025-12-19 00:00:00'),
('P1006','문두르','OS001',0,'OUT',NULL,NULL,NULL),
('P1007','강송구','OS004',0,'OUT',NULL,NULL,NULL),
('P1008','유종민','OS002',0,'IN','5동',4,'2025-12-19 21:34:58'),
('P1009','김선곤','OS003',0,'IN','5동',5,'2025-12-19 21:34:58'),
('P1010','안창회','OS003',0,'OUT',NULL,NULL,NULL),
('P1011','서민솔','RM002',1,'IN','5동',6,'2025-12-19 21:37:02');
/*!40000 ALTER TABLE `patient_info` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `robot_call_log`
--

DROP TABLE IF EXISTS `robot_call_log`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `robot_call_log` (
  `log_seq` int(11) NOT NULL AUTO_INCREMENT,
  `robot_id` int(11) NOT NULL,
  `user_name` varchar(50) NOT NULL,
  `request_time` datetime NOT NULL,
  `start_location_id` int(11) NOT NULL,
  `target_location_id` int(11) NOT NULL,
  PRIMARY KEY (`log_seq`),
  KEY `fk_log_robot` (`robot_id`),
  KEY `fk_log_start_loc` (`start_location_id`),
  KEY `fk_log_target_loc` (`target_location_id`),
  CONSTRAINT `fk_log_robot` FOREIGN KEY (`robot_id`) REFERENCES `robot_status` (`robot_id`),
  CONSTRAINT `fk_log_start_loc` FOREIGN KEY (`start_location_id`) REFERENCES `map_location` (`location_id`),
  CONSTRAINT `fk_log_target_loc` FOREIGN KEY (`target_location_id`) REFERENCES `map_location` (`location_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `robot_call_log`
--

LOCK TABLES `robot_call_log` WRITE;
/*!40000 ALTER TABLE `robot_call_log` DISABLE KEYS */;
set autocommit=0;
/*!40000 ALTER TABLE `robot_call_log` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `robot_status`
--

DROP TABLE IF EXISTS `robot_status`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `robot_status` (
  `robot_id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(50) NOT NULL,
  `ip_address` varchar(15) DEFAULT NULL,
  `op_status` enum('WAITING','HEADING','BOARDING','RUNNING','STOP','ARRIVED','EXITING','CHARGING','ERROR') NOT NULL,
  `battery_percent` int(11) DEFAULT 0,
  `is_charging` tinyint(4) DEFAULT 0,
  `current_x` double NOT NULL DEFAULT 0,
  `current_y` double NOT NULL DEFAULT 0,
  `current_theta` double NOT NULL DEFAULT 0,
  `start_x` double DEFAULT 0,
  `start_y` double DEFAULT 0,
  `goal_x` double DEFAULT NULL,
  `goal_y` double DEFAULT NULL,
  `sensor` int(11) DEFAULT NULL,
  `order` tinyint(3) unsigned DEFAULT NULL,
  `who_called` char(20) DEFAULT NULL,
  `ultra_distance_cm` int(11) DEFAULT 0,
  `seat_detected` tinyint(1) DEFAULT 0,
  PRIMARY KEY (`robot_id`),
  UNIQUE KEY `uk_robot_name` (`name`)
) ENGINE=InnoDB AUTO_INCREMENT=1006795 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `robot_status`
--

LOCK TABLES `robot_status` WRITE;
/*!40000 ALTER TABLE `robot_status` DISABLE KEYS */;
set autocommit=0;
INSERT INTO `robot_status` VALUES
(976513,'turtlebot3','10.10.14.148','WAITING',100,0,0,0,0,0,-0.782,2.2,1.62,NULL,0,'admin',163,0);
/*!40000 ALTER TABLE `robot_status` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `tb_edges`
--

DROP TABLE IF EXISTS `tb_edges`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `tb_edges` (
  `edge_id` int(11) NOT NULL AUTO_INCREMENT,
  `node1_id` int(11) NOT NULL,
  `node2_id` int(11) NOT NULL,
  PRIMARY KEY (`edge_id`),
  KEY `node1_id` (`node1_id`),
  KEY `node2_id` (`node2_id`),
  CONSTRAINT `tb_edges_ibfk_1` FOREIGN KEY (`node1_id`) REFERENCES `tb_waypoints` (`node_id`) ON DELETE CASCADE,
  CONSTRAINT `tb_edges_ibfk_2` FOREIGN KEY (`node2_id`) REFERENCES `tb_waypoints` (`node_id`) ON DELETE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=11 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `tb_edges`
--

LOCK TABLES `tb_edges` WRITE;
/*!40000 ALTER TABLE `tb_edges` DISABLE KEYS */;
set autocommit=0;
INSERT INTO `tb_edges` VALUES
(1,104,3),
(2,104,6),
(3,104,103),
(4,104,101),
(5,102,103),
(6,101,2),
(7,101,5),
(8,102,1),
(9,103,4),
(10,3,6);
/*!40000 ALTER TABLE `tb_edges` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `tb_waypoints`
--

DROP TABLE IF EXISTS `tb_waypoints`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `tb_waypoints` (
  `node_id` int(11) NOT NULL,
  `node_name` varchar(50) DEFAULT NULL,
  `x` double NOT NULL,
  `y` double NOT NULL,
  `is_destination` tinyint(4) DEFAULT 0,
  PRIMARY KEY (`node_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `tb_waypoints`
--

LOCK TABLES `tb_waypoints` WRITE;
/*!40000 ALTER TABLE `tb_waypoints` DISABLE KEYS */;
set autocommit=0;
INSERT INTO `tb_waypoints` VALUES
(1,'정형외과',2.99,-0.391,1),
(2,'식당',2.2,1.62,1),
(3,'키오스크',0,-0.782,1),
(4,'재활의학과',1.85,-0.34,1),
(5,'입원실',0.193,1.68,1),
(6,'충전스테이션',0,0,1),
(101,'식당_병실_앞',1.35,1.81,0),
(102,'정형외과_앞',2.14,0.61,0),
(103,'재활_앞',2.12,0.61,0),
(104,'중간_포인트',1.35,0.61,0);
/*!40000 ALTER TABLE `tb_waypoints` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Table structure for table `users`
--

DROP TABLE IF EXISTS `users`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8mb4 */;
CREATE TABLE `users` (
  `id` varchar(50) NOT NULL,
  `pw` varchar(50) NOT NULL,
  `role` varchar(20) DEFAULT 'patient',
  `comment` char(255) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `users`
--

LOCK TABLES `users` WRITE;
/*!40000 ALTER TABLE `users` DISABLE KEYS */;
set autocommit=0;
INSERT INTO `users` VALUES
('admin','1234','admin','비밀번호 수정'),
('kiosk','1234','kiosk',''),
('medical','1234','medical','메디컬 계정 추가 테스트');
/*!40000 ALTER TABLE `users` ENABLE KEYS */;
UNLOCK TABLES;
commit;

--
-- Dumping routines for database 'hospital_db'
--

--
-- Final view structure for view `map_location`
--

/*!50001 DROP VIEW IF EXISTS `map_location`*/;
/*!50001 SET @saved_cs_client          = @@character_set_client */;
/*!50001 SET @saved_cs_results         = @@character_set_results */;
/*!50001 SET @saved_col_connection     = @@collation_connection */;
/*!50001 SET character_set_client      = utf8mb4 */;
/*!50001 SET character_set_results     = utf8mb4 */;
/*!50001 SET collation_connection      = utf8mb4_uca1400_ai_ci */;
/*!50001 CREATE ALGORITHM=UNDEFINED */
/*!50013 DEFINER=`root`@`%` SQL SECURITY DEFINER */
/*!50001 VIEW `map_location` AS select `tb_waypoints`.`node_id` AS `location_id`,`tb_waypoints`.`node_name` AS `location_name`,`tb_waypoints`.`x` AS `x`,`tb_waypoints`.`y` AS `y` from `tb_waypoints` where `tb_waypoints`.`is_destination` = 1 */;
/*!50001 SET character_set_client      = @saved_cs_client */;
/*!50001 SET character_set_results     = @saved_cs_results */;
/*!50001 SET collation_connection      = @saved_col_connection */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*M!100616 SET NOTE_VERBOSITY=@OLD_NOTE_VERBOSITY */;

-- Dump completed on 2026-01-04 15:29:37
