#!/usr/bin/env python3
"""
파일명: stm32_bridge_all.py
설명:
- STM32(시리얼) <-> ROS2 브리지
- TurtleBot3 원본 토픽(입력) 구독 → 네임스페이스 붙은 토픽(출력)으로 재발행
- STM32 센서(초음파/좌석/버튼) → 네임스페이스 붙은 토픽으로 발행
- tcp_bridge가 /turtlebot3/* 를 구독한다면, robot_ns='turtlebot3'로 맞추면 됨
"""

import math
import socket
import serial

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, DurabilityPolicy

from std_msgs.msg import Float32, Bool, Int32, String
from sensor_msgs.msg import LaserScan, BatteryState
from nav_msgs.msg import Odometry
from geometry_msgs.msg import PoseWithCovarianceStamped


def quat_to_yaw_deg(x, y, z, w):
    siny_cosp = 2.0 * (w * z + x * y)
    cosy_cosp = 1.0 - 2.0 * (y * y + z * z)
    return math.degrees(math.atan2(siny_cosp, cosy_cosp))


class Stm32BridgeAll(Node):
    def __init__(self):
        super().__init__('stm32_bridge_all')

        # =====================================================================
        # 1) Parameters
        # =====================================================================
        self.declare_parameter('port', '/dev/stm32')
        self.declare_parameter('baud', 115200)

        # 출력 네임스페이스 결정용
        self.declare_parameter('robot_ns', '')                 # 예: "turtlebot3"
        self.declare_parameter('use_hostname_namespace', True) # robot_ns 비었으면 hostname 기반

        # 입력 토픽(원본)
        self.declare_parameter('in_amcl_topic', '/amcl_pose')
        self.declare_parameter('in_odom_topic', '/odom')
        self.declare_parameter('in_battery_topic', '/battery_state')
        self.declare_parameter('in_scan_topic', '/scan')

        self.port = str(self.get_parameter('port').value)
        self.baud = int(self.get_parameter('baud').value)

        # =====================================================================
        # 2) Output namespace 결정
        # =====================================================================
        p_use = bool(self.get_parameter('use_hostname_namespace').value)
        p_ns = str(self.get_parameter('robot_ns').value).strip()

        if (not p_ns) and p_use:
            host = socket.gethostname().strip()
            self.robot_ns = f'/{host}'
        elif p_ns:
            self.robot_ns = p_ns if p_ns.startswith('/') else f'/{p_ns}'
        else:
            self.robot_ns = ''

        def out_name(name: str) -> str:
            """출력 토픽명에 네임스페이스를 붙인다."""
            if not self.robot_ns:
                return name
            if name.startswith('/'):
                return self.robot_ns + name
            return self.robot_ns + '/' + name

        self.get_logger().info(f"Output Namespace: '{self.robot_ns}'")

        # =====================================================================
        # 3) Topic mapping (Input vs Output)
        # =====================================================================
        # Input (원본)
        self.in_amcl = str(self.get_parameter('in_amcl_topic').value)
        self.in_odom = str(self.get_parameter('in_odom_topic').value)
        self.in_batt = str(self.get_parameter('in_battery_topic').value)
        self.in_scan = str(self.get_parameter('in_scan_topic').value)

        # Output (네임스페이스 붙여 재발행)
        self.out_amcl = out_name(self.in_amcl)
        self.out_odom = out_name(self.in_odom)
        self.out_batt = out_name(self.in_batt)
        self.out_scan = out_name(self.in_scan)

        # Output (STM32 센서)
        self.out_ui = out_name('ui/info')              # tcp_bridge가 발행하는 UI를 받는 용도(구독)
        self.out_dist = out_name('/ultra_distance_cm')
        self.out_seat = out_name('/seat_detected')
        self.out_btn = out_name('/stm32/button')

        # =====================================================================
        # 4) QoS
        # =====================================================================
        qos_default = 10
        qos_sensor = QoSProfile(
            depth=10,
            reliability=ReliabilityPolicy.BEST_EFFORT,
            durability=DurabilityPolicy.VOLATILE
        )

        # =====================================================================
        # 5) Serial init
        # =====================================================================
        self.ser = None
        self.rx_buf = bytearray()
        self._open_serial()

        # =====================================================================
        # 6) Publishers (Output)
        # =====================================================================
        # STM32 센서
        self.pub_dist = self.create_publisher(Float32, self.out_dist, qos_default)
        self.pub_seat = self.create_publisher(Bool, self.out_seat, qos_default)
        self.pub_btn  = self.create_publisher(Int32, self.out_btn, qos_default)

        # 재발행
        self.pub_odom = self.create_publisher(Odometry, self.out_odom, qos_default)
        self.pub_batt = self.create_publisher(BatteryState, self.out_batt, qos_default)
        self.pub_amcl = self.create_publisher(PoseWithCovarianceStamped, self.out_amcl, qos_default)
        self.pub_scan = self.create_publisher(LaserScan, self.out_scan, qos_sensor)

        # =====================================================================
        # 7) Subscribers (Input)
        # =====================================================================
        # UI는 tcp_bridge가 (보통) /turtlebot3/ui/info 로 발행 → 이 노드가 받아서 STM32로 전달
        self.create_subscription(String, self.out_ui, self.cb_ui, qos_default)

        self.create_subscription(BatteryState, self.in_batt, self.cb_battery, qos_default)
        self.create_subscription(Odometry, self.in_odom, self.cb_odom, qos_default)
        self.create_subscription(PoseWithCovarianceStamped, self.in_amcl, self.cb_amcl, qos_default)
        self.create_subscription(LaserScan, self.in_scan, self.cb_scan, qos_sensor)

        # =====================================================================
        # 8) Internal state
        # =====================================================================
        self.ui_data = "0@0.0@100@Waiting@-@-"
        self.batt_p = 100.0

        self.x = 0.0
        self.y = 0.0
        self.yaw = 0.0
        self.v = 0.0
        self.w = 0.0
        self.using_amcl = False

        self.send_cnt = 0
        self.create_timer(0.02, self.control_loop)

        self.get_logger().info(
            "STM32 Bridge Started\n"
            f"  SUB(Input):  odom={self.in_odom}, batt={self.in_batt}, amcl={self.in_amcl}, scan={self.in_scan}, ui={self.out_ui}\n"
            f"  PUB(Output): odom={self.out_odom}, batt={self.out_batt}, amcl={self.out_amcl}, scan={self.out_scan}\n"
            f"  PUB(STM32):  dist={self.out_dist}, seat={self.out_seat}, btn={self.out_btn}"
        )

    # =====================================================================
    # Serial
    # =====================================================================
    def _open_serial(self):
        try:
            self.ser = serial.Serial(self.port, baudrate=self.baud, timeout=0)
            self.rx_buf.clear()
            self.get_logger().info(f"Serial opened: {self.port} @ {self.baud}")
        except Exception as e:
            self.ser = None
            self.get_logger().warn(f"Serial open failed: {e}")

    # =====================================================================
    # Callbacks (Input -> update + republish)
    # =====================================================================
    def cb_odom(self, msg: Odometry):
        self.v = msg.twist.twist.linear.x
        self.w = msg.twist.twist.angular.z

        if not self.using_amcl:
            self.x = msg.pose.pose.position.x
            self.y = msg.pose.pose.position.y
            q = msg.pose.pose.orientation
            self.yaw = quat_to_yaw_deg(q.x, q.y, q.z, q.w)

        # 재발행 (/turtlebot3/odom 등)
        self.pub_odom.publish(msg)

    def cb_amcl(self, msg: PoseWithCovarianceStamped):
        self.using_amcl = True
        self.x = msg.pose.pose.position.x
        self.y = msg.pose.pose.position.y
        q = msg.pose.pose.orientation
        self.yaw = quat_to_yaw_deg(q.x, q.y, q.z, q.w)

        # AMCL도 필요하면 namespaced로 재발행
        self.pub_amcl.publish(msg)

        # (선택) AMCL 기반 pose를 odom으로 변환해 /<ns>/odom 로도 발행
        odom = Odometry()
        odom.header = msg.header
        # AMCL이면 일반적으로 map 좌표계가 의미적으로 맞음 (필요 시 'odom'으로 변경)
        odom.header.frame_id = 'map'
        odom.child_frame_id = 'base_footprint'
        odom.pose.pose = msg.pose.pose
        odom.twist.twist.linear.x = self.v
        odom.twist.twist.angular.z = self.w

        self.pub_odom.publish(odom)

    def cb_battery(self, msg: BatteryState):
        # percentage가 0~1 또는 0~100으로 올 수 있어 보정
        p = float(msg.percentage)
        self.batt_p = p * 100.0 if p <= 1.0 else p

        self.pub_batt.publish(msg)

    def cb_scan(self, msg: LaserScan):
        # scan 재발행(필요 없으면 주석 가능)
        self.pub_scan.publish(msg)

    def cb_ui(self, msg: String):
        self.ui_data = msg.data

    # =====================================================================
    # Main loop
    # =====================================================================
    def control_loop(self):
        # Serial reconnect
        if self.ser is None:
            self._open_serial()
            return

        # Read STM32 -> ROS
        try:
            n = self.ser.in_waiting
            if n and n > 0:
                self.rx_buf.extend(self.ser.read(n))

                while b'\n' in self.rx_buf:
                    line, _, rest = self.rx_buf.partition(b'\n')
                    self.rx_buf = bytearray(rest)
                    self.parse(line.decode(errors='ignore').strip())
        except Exception as e:
            self.get_logger().warn(f"Serial read error: {e}")
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None
            return

        # Write ROS -> STM32 (주기 조절)
        self.send_cnt += 1
        if self.send_cnt >= 5:
            self.send_cnt = 0
            self.send_stm32()

    def parse(self, line: str):
        """
        STM32 -> ROS
        format: distance_cm@seat@button
        """
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
                self.pub_btn.publish(Int32(data=btn))
        except Exception:
            pass

    def send_stm32(self):
        """
        ROS -> STM32
        format: mode@v@batt@caller@start@dest
        ui_data format assumed: mode@...@...@caller@start@dest
        """
        try:
            parts = self.ui_data.split('@')
            if len(parts) >= 6:
                mode = parts[0]
                caller = parts[3]
                st = parts[4]
                dest = parts[5]
            else:
                mode, caller, st, dest = "0", "Init", "-", "-"
        except Exception:
            mode, caller, st, dest = "0", "Err", "-", "-"

        msg = f"{mode}@{self.v:.2f}@{int(self.batt_p)}@{caller}@{st}@{dest}\n"

        try:
            self.ser.write(msg.encode())
        except Exception as e:
            self.get_logger().warn(f"Serial write error: {e}")
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None


def main():
    rclpy.init()
    node = Stm32BridgeAll()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        try:
            if node.ser:
                node.ser.close()
        except Exception:
            pass
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
