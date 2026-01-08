#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
============================================================================
 파일명: tcp_bridge.py (Fully Automatic / No-Button Version)
 설명:   ROS 2(Nav2) <-> TCP(C Server) 통신 브리지
 
 [이 버전의 특징]
 1. 물리 버튼(STM32 Button) 기능이 완전히 제거되었습니다.
 2. 도착 시 버튼을 기다리지 않고 일정 시간(5초) 후 자동으로 다음 단계로 넘어갑니다.
 3. 최신 주행 로직(0.8m 도착 판정, 스마트 정지, UI 프로토콜)은 그대로 유지됩니다.
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
# ROS 2 라이브러리
# -------------------------------------------------------------------------
import rclpy
from rclpy.node import Node
from rclpy.parameter import Parameter
from rclpy.qos import QoSProfile, DurabilityPolicy, ReliabilityPolicy
from rclpy.action import ActionClient

# 메시지 타입
from geometry_msgs.msg import PoseWithCovarianceStamped, Quaternion, PoseStamped, Twist
from nav_msgs.msg import Odometry
from sensor_msgs.msg import BatteryState
from std_msgs.msg import Int32, Bool, String
from action_msgs.msg import GoalStatus
from nav2_msgs.action import NavigateToPose

# =========================================================================
# 1. 프로토콜 상수 및 설정
# =========================================================================
MAGIC_NUMBER = 0xAB
DEVICE_ROBOT_ROS = 0x02

MSG_LOGIN_REQ   = 0x01
MSG_ROBOT_STATE = 0x20
MSG_ASSIGN_GOAL = 0x30

# 로봇 상태 (FSM)
STATE_WAITING  = 0
STATE_HEADING  = 1
STATE_BOARDING = 2
STATE_RUNNING  = 3
STATE_STOP     = 4
STATE_ARRIVED  = 5
STATE_EXITING  = 6
STATE_CHARGING = 7
STATE_ERROR    = 99

HDR_FMT = "<BBBB"
HDR_SIZE = struct.calcsize(HDR_FMT)
STATE_FMT = "<ifffBiB"
GOAL_FMT = "<iffff64s"
GOAL_SIZE = struct.calcsize(GOAL_FMT)

# [설정] 주행 허용 오차
DIST_TOLERANCE_FINAL    = 0.8  # 0.8m 이내면 도착
DIST_TOLERANCE_WAYPOINT = 1.0  # 경유지는 1.0m

# [설정] 자동 대기 시간 (초)
AUTO_WAIT_SEC = 5.0 

# =========================================================================
# 2. 길찾기 클래스
# =========================================================================
class SimplePathFinder:
    def __init__(self, json_path):
        self.nodes = {}      
        self.edges = {}      
        self.locations = {}  
        self.load_map(json_path)

    def load_map(self, json_path):
        print(f"[Map] 맵 로딩: {json_path}")
        try:
            with open(json_path, 'r', encoding='utf-8') as f:
                data = json.load(f)
            self.nodes = {int(k): tuple(v) for k, v in data.get('nodes', {}).items()}
            for item in data.get('edges', []):
                if len(item) >= 3:
                    u, v, w = item[0], item[1], item[2]
                    self.edges.setdefault(u, []).append((v, w))
                    self.edges.setdefault(v, []).append((u, w))
            raw_locs = data.get('locations', {})
            for name, coords in raw_locs.items():
                self.locations[name] = tuple(coords)
            print(f"[Map] 로드 완료: 노드 {len(self.nodes)}개, 장소 {len(self.locations)}개")
        except Exception as e:
            print(f"[Map] 로딩 실패: {e}")
            self.nodes = {}; self.edges = {}; self.locations = {}

    def find_location_name(self, target_x, target_y):
        if not self.locations: return "?"
        min_dist = 1.5 # 이름 매칭 범위
        found_name = "?"
        for name, coords in self.locations.items():
            dist = math.dist((target_x, target_y), coords)
            if dist < min_dist:
                min_dist = dist
                found_name = name
        return found_name

    def find_nearest_node(self, tx, ty):
        if not self.nodes: return None
        return min(self.nodes.keys(), key=lambda k: math.dist((tx, ty), self.nodes[k]))

    def get_path(self, sx, sy, gx, gy):
        if not self.nodes: return [(gx, gy)]
        start_node = self.find_nearest_node(sx, sy)
        end_node = self.find_nearest_node(gx, gy)
        if start_node is None or end_node is None: return [(gx, gy)]
        
        queue = [(0, start_node, [])]
        visited = set()
        while queue:
            (cost, curr, path) = heapq.heappop(queue)
            if curr in visited: continue
            visited.add(curr)
            new_path = path + [curr]
            if curr == end_node:
                return [self.nodes[n] for n in new_path] + [(gx, gy)]
            for neighbor, weight in self.edges.get(curr, []):
                if neighbor not in visited:
                    h = math.dist(self.nodes[neighbor], self.nodes[end_node])
                    heapq.heappush(queue, (cost + weight + h, neighbor, new_path))
        return [(gx, gy)]

# =========================================================================
# 3. 메인 노드
# =========================================================================
class TcpBridge(Node):
    def __init__(self, parameter_overrides=None, map_file="map_graph.json"):
        super().__init__("tcp_bridge", parameter_overrides=parameter_overrides)

        self.robot_name = "wc1"
        self.server_ip = self.declare_parameter("server_ip", "127.0.0.1").value
        self.server_port = 8080
        
        # Action Client (비상 정지용)
        self.nav2_client = ActionClient(self, NavigateToPose, 'navigate_to_pose')

        prefix = f"/{self.robot_name}"
        
        # 토픽 설정
        self.topic_odom = f"{prefix}/odom"
        self.topic_battery = f"{prefix}/battery_state"
        self.topic_ultra = f"{prefix}/ultra_distance_cm"
        self.topic_seat = f"{prefix}/seat_detected"
        self.topic_cmd_vel = f"{prefix}/cmd_vel"
        self.topic_goal = "/goal_pose"
        self.topic_caller = f"{prefix}/caller_name"
        self.topic_ui = f"{prefix}/ui/info"

        # Publisher
        self.cmd_vel_pub = self.create_publisher(Twist, self.topic_cmd_vel, 10)
        self.goal_pub = self.create_publisher(PoseStamped, self.topic_goal, 10)
        self.caller_pub = self.create_publisher(String, self.topic_caller, 10)
        self.ui_pub = self.create_publisher(String, self.topic_ui, 10)

        # Subscriber (버튼 관련 제거됨)
        self.create_subscription(Odometry, self.topic_odom, self.odom_cb, 10)
        self.create_subscription(BatteryState, self.topic_battery, self.batt_cb, 10)
        self.create_subscription(Int32, self.topic_ultra, self.ultra_cb, 10)
        self.create_subscription(Bool, self.topic_seat, self.seat_cb, 10)

        # 변수 초기화
        self.x = 0.0; self.y = 0.0; self.theta = 0.0
        self.battery_percent = 100
        self.ultra_distance = 0; self.seat_detected = False
        
        self.current_state = STATE_WAITING
        self.prev_state = STATE_WAITING
        self.mission_mode = "NONE" 
        
        # 주행 관련 변수
        self.current_goal_x = 0.0; self.current_goal_y = 0.0
        self.final_goal_x = 0.0; self.final_goal_y = 0.0
        self.waypoint_queue = []
        self.paused_queue = [] 
        self.paused_goal = None
        
        # UI 정보
        self.current_caller = ""
        self.current_dest_name = "?"
        self.pickup_loc_name = "-"

        # 통신
        self.sock = None; self.lock = threading.Lock(); self.logged_in = False; self.running = True
        self.pathfinder = SimplePathFinder(map_file)

        # 타이머
        self.create_timer(0.5, self.control_loop)
        self.rx_thread = threading.Thread(target=self.rx_loop, daemon=True)
        self.rx_thread.start()
        
        print(f"\n🚀 [System] {self.robot_name} 자동 모드 시작 (No-Button)")
        print(f"   - 도착 오차: {DIST_TOLERANCE_FINAL}m")
        print(f"   - 자동 대기 시간: {AUTO_WAIT_SEC}초")

    # --- Callbacks ---
    def odom_cb(self, msg):
        self.x = msg.pose.pose.position.x
        self.y = msg.pose.pose.position.y
        q = msg.pose.pose.orientation
        siny_cosp = 2.0 * (q.w * q.z + q.x * q.y)
        cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z)
        self.theta = math.atan2(siny_cosp, cosy_cosp)

    def batt_cb(self, msg):
        val = int(msg.percentage) if msg.percentage > 1.0 else int(msg.percentage * 100)
        self.battery_percent = val
    def ultra_cb(self, msg): self.ultra_distance = int(msg.data)
    def seat_cb(self, msg): self.seat_detected = msg.data

    # [삭제됨] button_cb 함수는 더 이상 필요하지 않습니다.

    # --- Logic: Auto Transitions ---
    def auto_start_delivery(self):
        """ [자동] 환자 탑승 후 목적지 출발 """
        print("🕒 자동 출발: 환자 탑승 완료 간주 -> 목적지로 이동")
        self.change_state(STATE_RUNNING)
        self.mission_mode = "DELIVER"
        # 저장해둔 최종 목적지(병원)로 주행 시작
        self.start_path_navigation(self.final_goal_x, self.final_goal_y)

    def auto_finish_mission(self):
        """ [자동] 하차 완료 후 대기 복귀 """
        print("🕒 자동 복귀: 환자 하차 완료 간주 -> 대기 모드")
        self.reset_to_waiting()

    # --- Logic: Stop & Resume (서버 명령으로만 작동) ---
    def handle_emergency_stop(self):
        if self.current_state == STATE_STOP: return
        self.prev_state = self.current_state
        self.change_state(STATE_STOP)
        
        self.paused_queue = self.waypoint_queue.copy()
        self.paused_goal = (self.current_goal_x, self.current_goal_y)
        self.waypoint_queue.clear()
        
        self._cancel_nav2()
        
        stop_cmd = Twist()
        for _ in range(10):
            self.cmd_vel_pub.publish(stop_cmd)
            time.sleep(0.01)
        self.publish_nav2_goal(self.x, self.y)

    def handle_resume(self):
        prev = self.prev_state if self.prev_state != STATE_STOP else STATE_RUNNING
        self.change_state(prev)
        
        if self.paused_queue:
            print(f"📍 경로 복원: {len(self.paused_queue)}개 경유지")
            self.waypoint_queue = self.paused_queue
            self.pop_and_drive()
        elif self.paused_goal:
            self.publish_nav2_goal(self.paused_goal[0], self.paused_goal[1])
        
        self.paused_queue = []
        self.paused_goal = None

    def _cancel_nav2(self):
        try:
            if self.nav2_client and self.nav2_client.server_is_ready():
                self.nav2_client.cancel_all_goals_async()
        except: pass

    # --- Main Logic ---
    def change_state(self, new_state):
        if self.current_state != new_state:
            print(f"[State] 🔄 {self.current_state} -> {new_state}")
            self.current_state = new_state
        self.publish_ui_info()

    def control_loop(self):
        if not self.connect(): return
        self.send_login_once()
        self.publish_ui_info()
        self.send_robot_state()

        if self.current_state in [STATE_HEADING, STATE_RUNNING]:
            dist = math.dist((self.x, self.y), (self.current_goal_x, self.current_goal_y))
            
            if dist < 2.0:
                print(f"   >>> 남은 거리: {dist:.3f}m (목표: {self.current_dest_name})")

            is_intermediate = (len(self.waypoint_queue) > 0)
            tolerance = DIST_TOLERANCE_WAYPOINT if is_intermediate else DIST_TOLERANCE_FINAL
            
            if dist < tolerance:
                if is_intermediate:
                    self.pop_and_drive() 
                else:
                    print(f"[Nav] 🏁 도착 완료 (오차: {dist:.2f}m)")
                    self.handle_arrival()

    def handle_arrival(self):
        """ 도착 시 자동 타이머 실행 """
        self.publish_nav2_goal(self.x, self.y) # 정지
        
        # 1. 환자 픽업지 도착
        if self.current_state == STATE_HEADING:
            print(f"[Event] 픽업지 도착. {AUTO_WAIT_SEC}초 후 자동 출발합니다.")
            self.change_state(STATE_BOARDING)
            # 5초 뒤에 자동으로 출발 함수 실행
            threading.Timer(AUTO_WAIT_SEC, self.auto_start_delivery).start()
            
        # 2. 목적지(병원) 도착
        elif self.current_state == STATE_RUNNING:
            print(f"[Event] 목적지 도착. {AUTO_WAIT_SEC}초 후 대기로 복귀합니다.")
            self.change_state(STATE_ARRIVED)
            
            if self.mission_mode == "DELIVER":
                # 환자 하차 시간 5초 부여 후 초기화
                threading.Timer(AUTO_WAIT_SEC, self.auto_finish_mission).start()
            else:
                # Admin 호출 등은 2초 뒤 바로 복귀
                time.sleep(2.0)
                self.reset_to_waiting()

    def reset_to_waiting(self):
        self.change_state(STATE_WAITING)
        self.mission_mode = "NONE"
        self.current_caller = ""
        self.current_dest_name = "?"
        self.pickup_loc_name = "-"
        self.waypoint_queue = []
        print("[Logic] 대기 상태 초기화.")

    def start_path_navigation(self, tx, ty):
        path = self.pathfinder.get_path(self.x, self.y, tx, ty)
        self.waypoint_queue = path
        
        found = self.pathfinder.find_location_name(tx, ty)
        if found == "?":
            if self.mission_mode == "PICKUP": found = "환자 위치"
            elif self.mission_mode == "MOVE": found = "지정 위치"
        self.current_dest_name = found
        
        print(f"[Nav] 경로 시작: {self.current_dest_name} ({tx:.1f}, {ty:.1f})")
        self.pop_and_drive()

    def pop_and_drive(self):
        if self.waypoint_queue:
            wp = self.waypoint_queue.pop(0)
            self.publish_nav2_goal(wp[0], wp[1])

    def publish_nav2_goal(self, x, y):
        self.current_goal_x = x; self.current_goal_y = y
        goal = PoseStamped()
        goal.header.frame_id = "map"; goal.header.stamp = self.get_clock().now().to_msg()
        goal.pose.position.x = float(x); goal.pose.position.y = float(y)
        goal.pose.orientation = Quaternion(w=1.0, x=0.0, y=0.0, z=0.0)
        self.goal_pub.publish(goal)

    def publish_ui_info(self):
        """ UI Protocol: Mode@Speed@Batt@Caller@Start@Dest """
        s_mode = str(self.current_state)
        s_speed = "0.0" 
        s_batt = str(int(self.battery_percent))
        s_caller = self.current_caller if self.current_caller else "Waiting"
        
        # UI 표시 로직 (픽업 중엔 픽업지, 이동 중엔 목적지)
        if self.current_state == STATE_HEADING:
            s_start = "-"
            s_dest = self.current_dest_name # 픽업지
        elif self.current_state in [STATE_BOARDING, STATE_RUNNING, STATE_ARRIVED]:
            s_start = self.pickup_loc_name 
            s_dest = self.current_dest_name # 목적지
        else:
            s_start = "-"; s_dest = "-"
        
        msg = f"{s_mode}@{s_speed}@{s_batt}@{s_caller}@{s_start}@{s_dest}"
        self.ui_pub.publish(String(data=msg))

    # --- Connection ---
    def connect(self):
        if self.sock: return True
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(3.0)
            self.sock.connect((self.server_ip, self.server_port))
            self.sock.settimeout(None)
            self.logged_in = False
            print("[Net] 연결 성공")
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
        while self.running and rclpy.ok():
            if not self.sock: time.sleep(1); continue
            try:
                hdr = self.sock.recv(HDR_SIZE)
                if len(hdr) != HDR_SIZE: self.close_socket(); continue
                magic, dev, mtype, dlen = struct.unpack(HDR_FMT, hdr)
                payload = self.sock.recv(dlen) if dlen > 0 else b""
                
                if mtype == MSG_ASSIGN_GOAL:
                    order, sx, sy, gx, gy, rname = struct.unpack(GOAL_FMT, payload)
                    caller = rname.split(b'\x00')[0].decode('utf-8')
                    print(f"\n[Server] 명령: {order}, 호출자: {caller}")
                    
                    self.current_caller = caller
                    self.caller_pub.publish(String(data=caller))
                    self.final_goal_x = gx; self.final_goal_y = gy
                    self.pickup_loc_name = self.pathfinder.find_location_name(sx, sy) # 픽업지 이름 저장
                    
                    if order == 6: # 배차 명령
                        self.mission_mode = "PICKUP"
                        self.change_state(STATE_HEADING)
                        self.start_path_navigation(sx, sy)
                    elif order in [1, 4, 5]: # 단순 이동
                        self.mission_mode = "MOVE"
                        self.change_state(STATE_RUNNING)
                        self.start_path_navigation(gx, gy)
                    elif order == 2: # 비상정지
                        self.handle_emergency_stop()
                    elif order == 3: # 재개
                        self.handle_resume()
            except: self.close_socket()

    def close_socket(self):
        if self.sock: self.sock.close(); self.sock = None; self.logged_in = False

def main():
    rclpy.init()
    name = sys.argv[1] if len(sys.argv) > 1 else "wc1"
    mapf = sys.argv[2] if len(sys.argv) > 2 else "map_graph.json"
    node = TcpBridge(parameter_overrides=[Parameter("robot_name", Parameter.Type.STRING, name)], map_file=mapf)
    try: rclpy.spin(node)
    except: pass
    finally: node.destroy_node(); rclpy.shutdown()

if __name__ == "__main__":
    main()