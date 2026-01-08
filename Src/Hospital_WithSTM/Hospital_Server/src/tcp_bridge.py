#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
============================================================================
 파일명: tcp_bridge.py (Final Perfect Version - No Spin / Detailed Comments)
 설명:   ROS 2(Nav2) <-> TCP(C Server) 통신 브리지 및 로봇 통합 제어기
 
 [핵심 기능 요약]
 1. TCP 통신: C언어 관제 서버와 소켓 통신 (명령 수신, 상태 송신)
 2. 경로 생성: A* 알고리즘을 사용하여 최단 경로(Waypoints) 생성 및 Nav2 전송
 3. UI 동기화: STM32 화면 표시를 위한 데이터 프로토콜(Format) 맞춤 전송
 4. 정지 제어: 비상 정지 및 도착 시 '제자리 회전(Spin)' 없이 즉시 정지
 5. 상태 제어: 물리 버튼 입력에 따른 FSM(Finite State Machine) 상태 전이
 
 [최종 설정 값]
 - 도착 판정 거리 (Final Tolerance): 0.8m (넉넉하게 설정하여 멈춤 현상 방지)
 - 장소 이름 매칭 거리 (Name Match Dist): 1.0m (좌표 -> 이름 변환용)
============================================================================
"""

import sys
import socket
import struct
import threading
import time
import math
import json
import heapq

# -------------------------------------------------------------------------
# ROS 2 관련 라이브러리 임포트
# -------------------------------------------------------------------------
import rclpy
from rclpy.node import Node
# PoseStamped: 목표 지점(위치+방향) 메시지
# Quaternion: 방향(회전)을 표현하는 4원수 (x, y, z, w)
from geometry_msgs.msg import PoseStamped, Quaternion
from nav_msgs.msg import Odometry        # 로봇 현재 위치 정보
from sensor_msgs.msg import BatteryState # 배터리 정보
from std_msgs.msg import Int32, Bool, String, Float32 # 기본 데이터 타입

# =========================================================================
# 1. 프로토콜 상수 및 설정 정의
# =========================================================================

# 패킷 유효성 검사용 매직 넘버 (헤더 맨 앞 1바이트)
MAGIC_NUMBER = 0xAB
# 장치 식별 ID (0x02: ROS Robot)
DEVICE_ROBOT_ROS = 0x02

# 메시지 타입 (서버와 약속된 프로토콜 ID)
MSG_LOGIN_REQ   = 0x01  # 로봇 로그인 (이름 전송)
MSG_ROBOT_STATE = 0x20  # 로봇 상태 보고 (주기적 전송)
MSG_ASSIGN_GOAL = 0x30  # 작업 지시 (서버 -> 로봇)

# 로봇 상태 코드 (FSM State) - STM32와 공유
STATE_WAITING  = 0  # 대기 중 (IDLE)
STATE_HEADING  = 1  # 환자 픽업지로 이동 중
STATE_BOARDING = 2  # 도착 후 환자 탑승 대기
STATE_RUNNING  = 3  # 목적지(병원 등)로 이동 중
STATE_STOP     = 4  # 비상 정지 상태
STATE_ARRIVED  = 5  # 목적지 도착 (하차 대기 / 완료 대기)
STATE_EXITING  = 6  # 하차 완료 (예비 상태)
STATE_CHARGING = 7  # 충전 중
STATE_ERROR    = 99 # 에러 발생

# 네트워크 패킷 포맷 (struct 라이브러리 포맷 스트링)
# 헤더: Magic(1) + Device(1) + Type(1) + Length(1) = 4 Bytes
HDR_FMT = "<BBBB"
HDR_SIZE = struct.calcsize(HDR_FMT)

# 상태 보고 패킷 본문: 배터리, x, y, theta, 상태, 초음파, 착석여부
STATE_FMT = "<ifffBiB"
# 작업 지시 패킷 본문: 명령코드, 시작x, y, 목표x, y, 호출자이름(64bytes)
GOAL_FMT = "<iffff64s"
GOAL_SIZE = struct.calcsize(GOAL_FMT)

# 하드웨어 버튼 매핑값 (STM32 -> ROS Topic으로 들어오는 값)
BTN_BOARDING_COMPLETE = 1  # 탑승 완료 (출발)
BTN_RESUME            = 3  # 주행 재개 (정지 해제)
BTN_EMERGENCY         = 4  # 비상 정지
BTN_EXIT_COMPLETE     = 5  # 하차 완료 (복귀/대기)

# [중요 설정] 주행 허용 오차 (단위: 미터)
# 도착지에 몇 m까지 근접했으면 도착 판정을 할 것인가
DIST_TOLERANCE_FINAL    = 0.5
# 경유지에 몇 m까지 근겁했으면 도착 판정을 할 것인가
DIST_TOLERANCE_WAYPOINT = 0.7

# =========================================================================
# 2. 길찾기 및 맵 데이터 관리 클래스
# =========================================================================
class SimplePathFinder:
    """
    map_graph.json 파일을 읽어서 다음 기능을 수행합니다.
    1. A* 알고리즘을 통한 노드 간 최단 경로 탐색
    2. 현재 좌표(x, y)가 어떤 '장소(Locations)'인지 이름 찾기
    """
    def __init__(self, json_path):
        self.nodes = {}      # 노드 ID -> (x, y)
        self.edges = {}      # 노드 연결 정보 (그래프)
        self.locations = {}  # 장소 이름 -> (x, y) 매핑 정보
        self.load_map(json_path)

    def load_map(self, json_path):
        """ JSON 파일을 파싱하여 메모리에 적재 """
        print(f"[Map] 맵 파일 로딩 시작: {json_path}")
        try:
            with open(json_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
            
            # 1. 노드 정보 로드
            self.nodes = {int(k): tuple(v) for k, v in data.get('nodes', {}).items()}
            
            # 2. 간선 정보 로드 (양방향 그래프 구성)
            for item in data.get('edges', []):
                if len(item) >= 3:
                    u, v, w = item[0], item[1], item[2]
                    self.edges.setdefault(u, []).append((v, w))
                    self.edges.setdefault(v, []).append((u, w))

            # 3. 장소 이름 정보 로드 (STM32 UI 표시용 - 정형외과, 약국 등)
            raw_locs = data.get('locations', {})
            for name, coords in raw_locs.items():
                self.locations[name] = tuple(coords)

            print(f"[Map] 로딩 완료: 노드 {len(self.nodes)}개, 장소 {len(self.locations)}개")
            
        except Exception as e:
            print(f"[Map] ⚠️ 맵 로딩 실패: {e}")
            # 실패 시 빈 딕셔너리로 초기화하여 프로그램 크래시 방지
            self.nodes = {}; self.edges = {}; self.locations = {}

    def find_location_name(self, target_x, target_y):
        """ 
        좌표(target_x, target_y) 주변의 장소 이름을 반환합니다.
        1.0m 이내에 등록된 장소가 있으면 그 이름을, 없으면 "?"를 리턴.
        """
        if not self.locations: return "?"
        
        # [설정] 검색 반경 1.0m (너무 넓으면 옆방 이름이 뜰 수 있음)
        min_dist = 1.0 
        found_name = "?"
        
        for name, coords in self.locations.items():
            dist = math.dist((target_x, target_y), coords)
            if dist < min_dist:
                min_dist = dist
                found_name = name
        return found_name

    def find_nearest_node(self, tx, ty):
        """ 좌표에서 가장 가까운 노드 ID를 찾습니다. """
        if not self.nodes: return None
        return min(self.nodes.keys(), key=lambda k: math.dist((tx, ty), self.nodes[k]))

    def get_path(self, sx, sy, gx, gy):
        """ A* 알고리즘: 시작점(sx,sy) -> 목표점(gx,gy) 경로 생성 """
        if not self.nodes: return [(gx, gy)]
        
        start_node = self.find_nearest_node(sx, sy)
        end_node = self.find_nearest_node(gx, gy)
        
        if start_node is None or end_node is None: return [(gx, gy)]
        
        # (비용, 현재노드, 경로리스트)
        queue = [(0, start_node, [])]
        visited = set()
        
        while queue:
            (cost, curr, path) = heapq.heappop(queue)
            if curr in visited: continue
            visited.add(curr)
            
            new_path = path + [curr]
            
            # 목표 노드 도착
            if curr == end_node:
                return [self.nodes[n] for n in new_path] + [(gx, gy)]
            
            # 인접 노드 탐색
            for neighbor, weight in self.edges.get(curr, []):
                if neighbor not in visited:
                    # 휴리스틱: 현재 비용 + 간선 비용 + 남은 직선 거리
                    h = math.dist(self.nodes[neighbor], self.nodes[end_node])
                    heapq.heappush(queue, (cost + weight + h, neighbor, new_path))
        
        # 경로 생성 실패 시 직선 경로 반환
        return [(gx, gy)]

# =========================================================================
# 3. 메인 ROS 2 노드 클래스
# =========================================================================
class TcpBridge(Node):
    def __init__(self, robot_name_arg, map_file_arg):
        super().__init__("tcp_bridge")
        
        # 3.1 변수 초기화
        self.robot_name = robot_name_arg
        self.server_ip = self.declare_parameter("server_ip", "127.0.0.1").value
        self.server_port = 8080
        
        # 로봇 상태 정보
        self.current_state = STATE_WAITING
        self.mission_mode = "NONE" # PICKUP(픽업), DELIVER(이송), MOVE(단순이동)
        
        # 센서 데이터
        self.battery_percent = 100
        self.x = 0.0; self.y = 0.0; self.theta = 0.0
        self.seat_detected = False; self.ultra_distance = 0
        
        # 네비게이션 목표 및 경로
        self.final_goal_x = 0.0; self.final_goal_y = 0.0
        self.current_goal_x = 0.0; self.current_goal_y = 0.0
        self.waypoint_queue = [] # 경유지 큐
        
        # UI 표시 정보
        self.current_caller = ""     
        self.current_dest_name = "?" 
        
        # TCP 소켓
        self.sock = None; self.lock = threading.Lock()
        self.logged_in = False; self.running = True
        
        # 맵 로더 인스턴스
        self.pathfinder = SimplePathFinder(map_file_arg)

        # 3.2 ROS 통신 설정 (토픽 이름은 /{robot_name}/... 형식)
        prefix = f"/{self.robot_name}"
        
        # [Subscriber] 센서 -> 로봇
        self.create_subscription(Odometry, f"{prefix}/odom", self.odom_cb, 10)
        self.create_subscription(BatteryState, f"{prefix}/battery_state", self.batt_cb, 10)
        self.create_subscription(Float32, f"{prefix}/ultra_distance_cm", self.ultra_cb, 10)
        self.create_subscription(Bool, f"{prefix}/seat_detected", self.seat_cb, 10)
        self.create_subscription(Int32, f"{prefix}/stm32/button", self.button_cb, 10)
        
        # [Publisher] 로봇 -> 외부
        self.ui_pub = self.create_publisher(String, f"{prefix}/ui/info", 10)
        self.goal_pub = self.create_publisher(PoseStamped, "/goal_pose", 10)
        self.caller_pub = self.create_publisher(String, f"{prefix}/caller_name", 10)
        
        # 3.3 타이머 및 스레드 시작
        self.create_timer(0.5, self.control_loop) # 0.5초 주기 제어 루프
        self.rx_thread = threading.Thread(target=self.rx_loop, daemon=True)
        self.rx_thread.start()
        
        print(f"\n🚀 [System] {self.robot_name} 시스템 시작됨.")
        print(f"   - 도착 판정 오차: {DIST_TOLERANCE_FINAL}m")

    # ---------------------------------------------------------------------
    # 콜백 함수 (데이터 수신)
    # ---------------------------------------------------------------------
    def odom_cb(self, msg):
        """ 로봇의 현재 위치(x,y) 및 방향(theta) 업데이트 """
        self.x = msg.pose.pose.position.x
        self.y = msg.pose.pose.position.y
        # 쿼터니언 -> 오일러 각(Yaw/Theta) 변환
        q = msg.pose.pose.orientation
        siny_cosp = 2.0 * (q.w * q.z + q.x * q.y)
        cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z)
        self.theta = math.atan2(siny_cosp, cosy_cosp)
        
    def batt_cb(self, msg):
        """ 배터리 잔량 업데이트 (0~100%) """
        val = int(msg.percentage) if msg.percentage > 1.0 else int(msg.percentage * 100)
        self.battery_percent = val
        
    def ultra_cb(self, msg): self.ultra_distance = int(msg.data)
    def seat_cb(self, msg): self.seat_detected = msg.data

    def button_cb(self, msg):
        """ 물리 버튼 입력 처리 핸들러 """
        btn = msg.data
        if btn == 0: return # 노이즈 필터링
        print(f"\n[Button] 🔘 입력 감지: {btn}")
        
        # 1. 탑승 대기 상태 -> [탑승 완료] -> 출발
        if self.current_state == STATE_BOARDING and btn == BTN_BOARDING_COMPLETE:
            print("✅ 탑승 완료 확인. 목적지로 출발합니다.")
            self.change_state(STATE_RUNNING)
            # 최종 목적지로 경로 재생성 및 주행 시작
            self.start_path_navigation(self.final_goal_x, self.final_goal_y)
            
        # 2. 도착 완료 상태 -> [하차 완료] -> 대기 모드
        elif self.current_state == STATE_ARRIVED and btn == BTN_EXIT_COMPLETE:
            print("✅ 임무 종료. 대기 모드로 복귀합니다.")
            self.reset_to_waiting()

        # 3. 언제든 [비상 정지]
        elif btn == BTN_EMERGENCY:
            print("🚨 비상 정지 명령! (제자리 정지)")
            self.change_state(STATE_STOP)
            # [중요] 현재 위치 + 현재 방향으로 정지 명령 전송 (회전 방지)
            self.publish_stop_packet()

        # 4. 정지 상태 -> [주행 재개]
        elif self.current_state == STATE_STOP and btn == BTN_RESUME:
            print("▶️ 주행 재개.")
            self.change_state(STATE_RUNNING)
            # 멈췄던 지점(current_goal)으로 다시 이동 명령
            self.publish_nav2_goal(self.current_goal_x, self.current_goal_y)

    # ---------------------------------------------------------------------
    # 핵심 로직 (FSM & Control)
    # ---------------------------------------------------------------------
    def change_state(self, new_state):
        """ 로봇 상태 변경 및 로그 출력 """
        if self.current_state != new_state:
            print(f"[State] 🔄 상태 전환: {self.current_state} -> {new_state}")
            self.current_state = new_state
        self.publish_ui_info()

    def control_loop(self):
        """ 메인 제어 루프 (0.5초마다 실행) """
        # 1. 서버 연결 및 데이터 전송
        if not self.connect(): return
        self.send_login_once()
        self.publish_ui_info()
        self.send_robot_state()

        # 2. 주행 감시 및 도착 판정
        if self.current_state in [STATE_HEADING, STATE_RUNNING]:
            dist = math.dist((self.x, self.y), (self.current_goal_x, self.current_goal_y))
            
            # [디버깅] 목표 지점 2m 이내 진입 시 남은 거리 출력
            if dist < 2.0:
                print(f"   >>> 남은 거리: {dist:.3f}m (목표: {self.current_dest_name})")

            # 경유지인지 최종 목적지인지 확인
            is_intermediate = (len(self.waypoint_queue) > 0)
            tolerance = DIST_TOLERANCE_WAYPOINT if is_intermediate else DIST_TOLERANCE_FINAL
            
            if dist < tolerance:
                if is_intermediate:
                    self.pop_and_drive() # 다음 경유지로 이동
                else:
                    print(f"[Nav] 🏁 최종 목적지 도착 완료! (오차: {dist:.2f}m)")
                    self.handle_arrival()

    def handle_arrival(self):
        """ 목적지 도착 시 처리 로직 """
        # [중요] 도착 시에도 0도로 돌지 않고 현재 방향 유지하며 정지
        self.publish_stop_packet()

        # 1. 픽업하러 왔을 때
        if self.current_state == STATE_HEADING:
            print("[Event] 픽업지 도착. 환자 탑승 대기.")
            self.change_state(STATE_BOARDING) # 즉시 전환 (버튼 대기)
            
        # 2. 목적지에 왔을 때
        elif self.current_state == STATE_RUNNING:
            print("[Event] 목적지 도착.")
            
            # [중요] Admin 호출(MOVE)은 1초 후 자동 대기 복귀
            if self.mission_mode == "MOVE":
                print(">> 단순 이동 미션 완료. 1초 후 대기 상태로 자동 복귀합니다.")
                self.change_state(STATE_ARRIVED)
                time.sleep(1.0) 
                self.reset_to_waiting()
            else:
                # 환자 이송(DELIVER)은 하차 버튼 누를 때까지 대기
                self.change_state(STATE_ARRIVED)

    def reset_to_waiting(self):
        """ 대기 상태로 초기화 """
        self.change_state(STATE_WAITING)
        self.mission_mode = "NONE"
        self.current_caller = ""
        self.current_dest_name = "?"
        self.waypoint_queue = []
        print("[Logic] 시스템 대기 상태로 초기화됨.")

    # ---------------------------------------------------------------------
    # 네비게이션 헬퍼
    # ---------------------------------------------------------------------
    def start_path_navigation(self, tx, ty):
        """ 경로 생성 및 주행 시작 """
        # 1. A* 경로 계산
        path = self.pathfinder.get_path(self.x, self.y, tx, ty)
        self.waypoint_queue = path
        
        # 2. 목적지 이름 찾기 (UI 표시용)
        found_name = self.pathfinder.find_location_name(tx, ty)
        
        # [예외 처리] 이름을 못 찾으면 기본값 부여
        if found_name == "?":
            if self.mission_mode == "PICKUP": found_name = "환자 위치"
            elif self.mission_mode == "MOVE": found_name = "지정 위치"
            
        self.current_dest_name = found_name
        print(f"[Nav] 경로 시작: {self.current_dest_name} ({tx:.1f}, {ty:.1f})")
        self.pop_and_drive()

    def pop_and_drive(self):
        """ 큐에서 웨이포인트 꺼내서 이동 """
        if self.waypoint_queue:
            wp = self.waypoint_queue.pop(0)
            self.publish_nav2_goal(wp[0], wp[1])

    def publish_nav2_goal(self, x, y):
        """ ROS 2 Goal 발행 (이동 시: 방향은 0도/진행방향 기준) """
        self.current_goal_x = x; self.current_goal_y = y
        goal = PoseStamped()
        goal.header.frame_id = "map"; goal.header.stamp = self.get_clock().now().to_msg()
        goal.pose.position.x = float(x); goal.pose.position.y = float(y)
        goal.pose.orientation.w = 1.0 # 이동 중에는 일단 0도 기준 (Nav2가 알아서 함)
        self.goal_pub.publish(goal)

    def publish_stop_packet(self):
        """ [추가] 정지 명령 (현재 로봇이 보고 있는 방향 유지) """
        stop_goal = PoseStamped()
        stop_goal.header.frame_id = "map"
        stop_goal.header.stamp = self.get_clock().now().to_msg()
        stop_goal.pose.position.x = float(self.x)
        stop_goal.pose.position.y = float(self.y)
        
        # 현재 각도(Theta)를 Quaternion으로 변환하여 유지 -> 회전 방지
        q = Quaternion()
        q.w = math.cos(self.theta * 0.5)
        q.z = math.sin(self.theta * 0.5)
        stop_goal.pose.orientation = q
        
        self.goal_pub.publish(stop_goal)
        
        # 목표 변수 갱신 (재개 시 튀는 현상 방지)
        self.current_goal_x = self.x
        self.current_goal_y = self.y

    def publish_ui_info(self):
        """ 
        [STM32 프로토콜 동기화]
        포맷: 모드@속도@배터리@호출자@출발지@도착지
        """
        s_mode = str(self.current_state)
        s_speed = "0.0" 
        s_batt = str(int(self.battery_percent))
        s_caller = self.current_caller if self.current_caller else "Waiting"
        s_start = "-" # 출발지는 미사용 (STM32 코드에서 처리)
        s_dest = self.current_dest_name if self.current_dest_name else "?"
        
        msg = f"{s_mode}@{s_speed}@{s_batt}@{s_caller}@{s_start}@{s_dest}"
        self.ui_pub.publish(String(data=msg))

    # ---------------------------------------------------------------------
    # 통신 연결 유틸리티
    # ---------------------------------------------------------------------
    def connect(self):
        if self.sock: return True
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(3.0) 
            self.sock.connect((self.server_ip, self.server_port))
            self.sock.settimeout(None)
            self.logged_in = False
            print("[Net] 서버 연결 성공")
            return True
        except: return False

    def send_login_once(self):
        if not self.logged_in and self.sock:
            try:
                pkt = struct.pack(HDR_FMT, MAGIC_NUMBER, DEVICE_ROBOT_ROS, MSG_LOGIN_REQ, len(self.robot_name)) + self.robot_name.encode()
                self.sock.sendall(pkt)
                self.logged_in = True
            except: self.close_socket()

    def send_robot_state(self):
        if not self.sock: return
        try:
            payload = struct.pack(STATE_FMT, int(self.battery_percent), self.x, self.y, self.theta,
                                  int(self.current_state), int(self.ultra_distance), int(1 if self.seat_detected else 0))
            header = struct.pack(HDR_FMT, MAGIC_NUMBER, DEVICE_ROBOT_ROS, MSG_ROBOT_STATE, len(payload))
            self.sock.sendall(header + payload)
        except: self.close_socket()

    def rx_loop(self):
        """ 데이터 수신 스레드 """
        while self.running and rclpy.ok():
            if not self.sock: time.sleep(1); continue
            try:
                # 헤더 읽기
                hdr = self.sock.recv(HDR_SIZE)
                if len(hdr) != HDR_SIZE: self.close_socket(); continue
                magic, dev, mtype, dlen = struct.unpack(HDR_FMT, hdr)
                payload = self.sock.recv(dlen) if dlen > 0 else b""
                
                # 메시지 처리 (작업 할당)
                if mtype == MSG_ASSIGN_GOAL:
                    order, sx, sy, gx, gy, rname = struct.unpack(GOAL_FMT, payload)
                    caller = rname.split(b'\x00')[0].decode('utf-8')
                    print(f"\n[Server] 📩 명령 수신: Order {order}, Caller {caller}")
                    
                    self.current_caller = caller
                    self.caller_pub.publish(String(data=caller))
                    self.final_goal_x = gx; self.final_goal_y = gy
                    
                    if order == 6: # 환자 픽업
                        self.mission_mode = "PICKUP"
                        self.change_state(STATE_HEADING)
                        self.start_path_navigation(sx, sy) # 출발 -> 픽업지
                    elif order in [1, 4, 5]: # 단순 이동
                        self.mission_mode = "MOVE"
                        self.change_state(STATE_RUNNING)
                        self.start_path_navigation(gx, gy) # 출발 -> 목적지
            except: self.close_socket()

    def close_socket(self):
        if self.sock: self.sock.close(); self.sock = None; self.logged_in = False

# =========================================================================
# 메인 실행부
# =========================================================================
def main():
    rclpy.init()
    # 인자값: 로봇이름(기본 wc1), 맵파일(기본 map_graph.json)
    name = sys.argv[1] if len(sys.argv) > 1 else "wc1"
    mapf = sys.argv[2] if len(sys.argv) > 2 else "map_graph.json"
    
    node = TcpBridge(name, mapf)
    try: rclpy.spin(node)
    except KeyboardInterrupt: pass
    finally: 
        node.running = False
        node.close_socket()
        node.destroy_node()
        rclpy.shutdown()

if __name__ == "__main__":
    main()