#!/usr/bin/env python3
"""
파일명: stm32_bridge_all.py
설명: STM32 양방향 통신 통합 노드 (프로토콜 매칭 수정판)
"""

import serial
import struct
import math
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, DurabilityPolicy

# 메시지 타입
from std_msgs.msg import Float32, Bool, Int32, String
from sensor_msgs.msg import LaserScan, BatteryState
from nav_msgs.msg import Odometry
from geometry_msgs.msg import PoseStamped, PoseWithCovarianceStamped

def quat_to_yaw_deg(x, y, z, w):
    siny_cosp = 2.0 * (w * z + x * y)
    cosy_cosp = 1.0 - 2.0 * (y * y + z * z)
    return math.degrees(math.atan2(siny_cosp, cosy_cosp))

class Stm32BridgeAll(Node):
    def __init__(self):
        super().__init__('stm32_bridge_all')

        # -----------------------------
        # 1. 파라미터 설정
        # -----------------------------
        self.declare_parameter('port', '/dev/stm32')
        self.declare_parameter('baud', 115200)
        self.port = self.get_parameter('port').value
        self.baud = int(self.get_parameter('baud').value)

        # 토픽 이름
        self.ui_topic = 'ui/info'
        self.battery_topic = 'battery_state'
        self.odom_topic = 'odom'
        self.amcl_topic = 'amcl_pose'
        self.goal_topic = 'goal_pose'
        self.scan_topic = 'scan'
        
        self.dist_topic = 'ultra_distance_cm'
        self.seat_topic = 'seat_detected'
        self.btn_topic = 'stm32/button'

        # -----------------------------
        # 2. 시리얼 연결
        # -----------------------------
        self.ser = None
        self.rx_buf = bytearray()
        self._open_serial()

        # -----------------------------
        # 3. 퍼블리셔 (STM32 -> ROS)
        # -----------------------------
        self.pub_dist = self.create_publisher(Float32, self.dist_topic, 10)
        self.pub_seat = self.create_publisher(Bool, self.seat_topic, 10)
        self.pub_btn  = self.create_publisher(Int32, self.btn_topic, 10)

        # -----------------------------
        # 4. 구독자 (ROS -> STM32)
        # -----------------------------
        # 센서용 QoS 설정 (Best Effort)
        qos_sensor = QoSProfile(depth=10)
        qos_sensor.reliability = ReliabilityPolicy.BEST_EFFORT
        qos_sensor.durability = DurabilityPolicy.VOLATILE
        
        qos_default = QoSProfile(depth=10)

        self.create_subscription(String, self.ui_topic, self.cb_ui, qos_default)
        self.create_subscription(BatteryState, self.battery_topic, self.cb_battery, qos_default)
        self.create_subscription(Odometry, self.odom_topic, self.cb_odom, qos_default)
        self.create_subscription(PoseWithCovarianceStamped, self.amcl_topic, self.cb_amcl, qos_default)
        self.create_subscription(PoseStamped, self.goal_topic, self.cb_goal, qos_default)
        self.create_subscription(LaserScan, self.scan_topic, self.cb_scan, qos_sensor)

        # -----------------------------
        # 5. 상태 변수들
        # -----------------------------
        # 초기값 포맷도 6개 필드로 맞춰줍니다 (안전장치)
        self.ui_data = "0@0.0@100@Waiting@-@-" 
        self.batt_p = 100.0; self.batt_v = 0.0
        self.x = 0.0; self.y = 0.0; self.yaw = 0.0
        self.v = 0.0; self.w = 0.0
        self.gx = 0.0; self.gy = 0.0; self.gyaw = 0.0
        self.lmin = -1.0
        
        self.front_angle = 0.0
        self.window = math.radians(5.0)

        # -----------------------------
        # 6. 타이머
        # -----------------------------
        self.create_timer(0.02, self.control_loop)
        self.send_cnt = 0 

        self.get_logger().info(f"✨ STM32 Bridge All-in-One Started on {self.port}")

    def _open_serial(self):
        try:
            self.ser = serial.Serial(self.port, baudrate=self.baud, timeout=0)
            self.rx_buf.clear()
        except Exception as e:
            self.ser = None
            self.get_logger().error(f"Serial Open Failed: {e}")

    # =========================
    # ROS 콜백
    # =========================
    def cb_ui(self, msg): 
        # tcp_bridge에서 오는 데이터: "모드@0.0@배터리@호출자@출발지@도착지"
        self.ui_data = msg.data
    
    def cb_battery(self, msg):
        if msg.percentage > 1.0:
            self.batt_p = float(msg.percentage)
        else:
            self.batt_p = float(msg.percentage) * 100.0
        self.batt_v = float(msg.voltage) if msg.voltage else 0.0

    def cb_odom(self, msg):
        self.v = msg.twist.twist.linear.x
        self.w = msg.twist.twist.angular.z

    def cb_amcl(self, msg):
        p = msg.pose.pose.position; q = msg.pose.pose.orientation
        self.x = p.x; self.y = p.y
        self.yaw = quat_to_yaw_deg(q.x, q.y, q.z, q.w)

    def cb_goal(self, msg):
        p = msg.pose.position; q = msg.pose.orientation
        self.gx = p.x; self.gy = p.y
        self.gyaw = quat_to_yaw_deg(q.x, q.y, q.z, q.w)

    def cb_scan(self, msg):
        if not msg.ranges: return
        mid = len(msg.ranges) // 2
        sample = msg.ranges[mid-5 : mid+5]
        valid = [r for r in sample if 0.05 < r < 10.0]
        if valid: self.lmin = min(valid)

    # =========================
    # 메인 루프
    # =========================
    def control_loop(self):
        if self.ser is None:
            self._open_serial()
            return

        # 1. 수신 (STM32 -> ROS)
        try:
            if self.ser.in_waiting > 0:
                data = self.ser.read(self.ser.in_waiting)
                # print(f"RAW: {data}")
                self.rx_buf.extend(data)
                
                while b'\n' in self.rx_buf:
                    line_bytes, _, rest = self.rx_buf.partition(b'\n')
                    self.rx_buf = bytearray(rest)
                    try:
                        line = line_bytes.decode('utf-8', errors='ignore').strip()
                        self.parse_stm32_data(line)
                    except: pass
        except Exception as e:
            self.get_logger().warn(f"RX Error: {e}")
            self.ser = None; return

        # 2. 송신 (ROS -> STM32) - 10Hz
        self.send_cnt += 1
        if self.send_cnt >= 5:
            self.send_cnt = 0
            self.send_to_stm32()

    def parse_stm32_data(self, line):
        parts = line.split('@')
        if len(parts) < 3: 
            return

        try:
            dist = float(parts[0])
            seat = (int(parts[1]) != 0)
            btn = int(parts[2])

            self.pub_dist.publish(Float32(data=dist))
            self.pub_seat.publish(Bool(data=seat))

            if btn != 0: 
                self.get_logger().info(f"🔘 Button Clicked! Value: {btn}")
            
            self.pub_btn.publish(Int32(data=btn))

        except Exception as e:
            self.get_logger().error(f"Parsing Error: {e} | Line: {line}")

    def send_to_stm32(self):
        # ---------------------------------------------------------
        # [수정됨] tcp_bridge.py와의 프로토콜 매칭
        # tcp_bridge 보냄: Mode(0) @ Speed(1) @ Batt(2) @ Caller(3) @ Start(4) @ Dest(5)
        # ---------------------------------------------------------
        try:
            parts = self.ui_data.split('@')
            
            # 데이터가 6개(신규 프로토콜)로 오는지 확인
            if len(parts) >= 6:
                mode = parts[0]
                # parts[1]은 속도, parts[2]는 배터리인데 이건 tcp_bridge가 모름(더미값)
                # 그래서 여기서 실제 센서값(self.v, self.batt_p)으로 교체함
                caller = parts[3]
                start_loc = parts[4]
                dest_loc = parts[5]
            else:
                # 혹시 예전 데이터가 오면 안전하게 처리
                mode = parts[0]
                caller = "Init"
                start_loc = "-"
                dest_loc = "-"

        except:
            mode = "0"; caller = "Error"; start_loc = "-"; dest_loc = "-"

        # 최종 조립: 실제 속도와 배터리 값 주입
        # 순서: Mode @ Speed @ Battery @ Caller @ Start @ Dest
        msg = f"{mode}@{self.v:.2f}@{int(self.batt_p)}@{caller}@{start_loc}@{dest_loc}\n"

        try:
            self.ser.write(msg.encode('utf-8'))
        except Exception as e:
            self.get_logger().warn(f"Serial Write Error: {e}")
            self.ser = None

def main():
    rclpy.init()
    node = Stm32BridgeAll()
    try: rclpy.spin(node)
    except: pass
    finally:
        if node.ser: node.ser.close()
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()