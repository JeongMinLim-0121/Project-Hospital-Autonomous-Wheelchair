#!/usr/bin/env python3
import math
import socket
import serial
import rclpy
from rclpy.node import Node

from rclpy.qos import QoSProfile, ReliabilityPolicy, DurabilityPolicy

from sensor_msgs.msg import LaserScan, BatteryState
from nav_msgs.msg import Odometry
from geometry_msgs.msg import PoseStamped, PoseWithCovarianceStamped
from std_msgs.msg import Float32, Bool


def quat_to_yaw_deg(x: float, y: float, z: float, w: float) -> float:
    """Quaternion -> yaw(deg)"""
    # yaw (z-axis rotation)
    siny_cosp = 2.0 * (w * z + x * y)
    cosy_cosp = 1.0 - 2.0 * (y * y + z * z)
    yaw = math.atan2(siny_cosp, cosy_cosp)
    return math.degrees(yaw)


class RosStateToStm32(Node):
    def __init__(self):
        super().__init__('ros_state_to_stm32')

        # -----------------------------
        # 파라미터
        # -----------------------------
        self.declare_parameter('use_hostname_namespace', True)
        self.declare_parameter('robot_ns', '')  # 빈 문자열이면 hostname 사용 가능
        self.declare_parameter('port', '/dev/ttyACM0')
        self.declare_parameter('baud', 115200)
        self.declare_parameter('send_rate_hz', 10.0)

        # 라이다 전방 거리 계산 파라미터
        self.declare_parameter('front_angle_deg', 0.0)  # 전방 기준 각도(도) (보통 0)
        self.declare_parameter('window_deg', 5.0)       # 전방 ±윈도우(도)
        self.declare_parameter('min_range_floor_m', 0.02)  # 너무 작은 값(노이즈) 컷

        # 토픽 이름(기본값은 "네임스페이스 + 이름" 조합으로 자동 생성)
        self.declare_parameter('battery_topic', '/battery_state')
        self.declare_parameter('amcl_topic', '/amcl_pose')
        self.declare_parameter('odom_topic', '/odom')
        self.declare_parameter('goal_topic', '/goal_pose')
        self.declare_parameter('scan_topic', '/scan')

        # (선택) 너가 이미 발행중인 초음파/seat 토픽도 같이 포함하려면
        self.declare_parameter('use_ultra_seat', True)
        self.declare_parameter('ultra_topic', '/ultra_distance_cm')
        self.declare_parameter('seat_topic', '/seat_detected')

        # -----------------------------
        # 네임스페이스 결정 (/pi, /ubuntu 등)
        # -----------------------------
        use_host = bool(self.get_parameter('use_hostname_namespace').value)
        robot_ns = str(self.get_parameter('robot_ns').value).strip()

        if not robot_ns and use_host:
            host = socket.gethostname().strip()
            robot_ns = f'/{host}'
        elif robot_ns and not robot_ns.startswith('/'):
            robot_ns = '/' + robot_ns

        self.robot_ns = robot_ns  # 예: "/pi" 또는 "/ubuntu" 또는 ""

        def resolve(name: str) -> str:
            """토픽 이름에 robot_ns 자동 적용.
            - name이 절대경로("/scan")면 robot_ns 붙여서 "/pi/scan" 형태로 만듦
            - robot_ns가 ""면 그대로 사용
            """
            if not self.robot_ns:
                return name
            # name이 "/xxx" 형태면 "/pi" + "/xxx"
            if name.startswith('/'):
                return self.robot_ns + name
            # 상대경로면 "/pi/" + name
            return self.robot_ns + '/' + name

        # 토픽 resolve
        self.battery_topic = resolve(str(self.get_parameter('battery_topic').value))
        self.amcl_topic    = resolve(str(self.get_parameter('amcl_topic').value))
        self.odom_topic    = resolve(str(self.get_parameter('odom_topic').value))
        self.goal_topic    = resolve(str(self.get_parameter('goal_topic').value))
        self.scan_topic    = resolve(str(self.get_parameter('scan_topic').value))

        self.use_ultra_seat = bool(self.get_parameter('use_ultra_seat').value)
        self.ultra_topic = resolve(str(self.get_parameter('ultra_topic').value))
        self.seat_topic  = resolve(str(self.get_parameter('seat_topic').value))

        # -----------------------------
        # 시리얼 설정
        # -----------------------------
        self.port = str(self.get_parameter('port').value)
        self.baud = int(self.get_parameter('baud').value)
        self.ser = None
        self._open_serial()

        # -----------------------------
        # 최신값 저장 변수
        # -----------------------------
        self.batt_percent = None
        self.batt_voltage = None
        self.batt_status  = None  # charging/discharging 등

        self.amcl_x = None
        self.amcl_y = None
        self.amcl_yaw_deg = None

        self.odom_v = None
        self.odom_w = None

        self.goal_x = None
        self.goal_y = None
        self.goal_yaw_deg = None

        self.lidar_front_min_m = None

        self.ultra_cm = None
        self.seat_bool = None

        # -----------------------------
        # QoS (sensor 데이터는 best_effort가 흔함)
        # -----------------------------
        qos_sensor = QoSProfile(depth=10)
        qos_sensor.reliability = ReliabilityPolicy.BEST_EFFORT
        qos_sensor.durability = DurabilityPolicy.VOLATILE

        qos_default = QoSProfile(depth=10)

        # -----------------------------
        # 구독자 생성
        # -----------------------------
        self.create_subscription(BatteryState, self.battery_topic, self.cb_battery, qos_default)
        self.create_subscription(PoseWithCovarianceStamped, self.amcl_topic, self.cb_amcl, qos_default)
        self.create_subscription(Odometry, self.odom_topic, self.cb_odom, qos_default)
        self.create_subscription(PoseStamped, self.goal_topic, self.cb_goal, qos_default)
        self.create_subscription(LaserScan, self.scan_topic, self.cb_scan, qos_sensor)

        if self.use_ultra_seat:
            self.create_subscription(Float32, self.ultra_topic, self.cb_ultra, qos_default)
            self.create_subscription(Bool, self.seat_topic, self.cb_seat, qos_default)

        # -----------------------------
        # 라이다 전방 계산 파라미터 캐시
        # -----------------------------
        self.front_angle = math.radians(float(self.get_parameter('front_angle_deg').value))
        self.window = math.radians(float(self.get_parameter('window_deg').value))
        self.min_range_floor = float(self.get_parameter('min_range_floor_m').value)

        # -----------------------------
        # 주기 송신 타이머
        # -----------------------------
        rate = float(self.get_parameter('send_rate_hz').value)
        period = 1.0 / max(rate, 1e-3)
        self.timer = self.create_timer(period, self.tick_send)

        self.get_logger().info(
            f"robot_ns='{self.robot_ns}'  topics:\n"
            f"  battery={self.battery_topic}\n"
            f"  amcl={self.amcl_topic}\n"
            f"  odom={self.odom_topic}\n"
            f"  goal={self.goal_topic}\n"
            f"  scan={self.scan_topic}\n"
            f"  ultra={self.ultra_topic if self.use_ultra_seat else '(off)'}\n"
            f"  seat ={self.seat_topic  if self.use_ultra_seat else '(off)'}\n"
            f"  serial={self.port}@{self.baud}"
        )

    # -----------------------------
    # 시리얼
    # -----------------------------
    def _open_serial(self):
        try:
            self.ser = serial.Serial(self.port, baudrate=self.baud, timeout=0)
            self.get_logger().info(f"Serial opened: {self.port} @ {self.baud}")
        except Exception as e:
            self.ser = None
            self.get_logger().error(f"Serial open failed: {e}")

    def _safe_write(self, s: str):
        if self.ser is None:
            self._open_serial()
            return
        try:
            self.ser.write(s.encode('ascii', errors='ignore'))
        except (OSError, serial.SerialException) as e:
            self.get_logger().warn(f"Serial write error: {e} (reopen)")
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None

    # -----------------------------
    # 콜백들
    # -----------------------------
    def cb_battery(self, msg: BatteryState):
        # percentage는 0.0~1.0 또는 -1.0(unknown)인 경우가 있음
        if msg.percentage is not None and msg.percentage >= 0.0:
            self.batt_percent = float(msg.percentage) * 100.0
        self.batt_voltage = float(msg.voltage) if msg.voltage is not None else self.batt_voltage
        self.batt_status = int(msg.power_supply_status)

    def cb_amcl(self, msg: PoseWithCovarianceStamped):
        p = msg.pose.pose.position
        q = msg.pose.pose.orientation
        self.amcl_x = float(p.x)
        self.amcl_y = float(p.y)
        self.amcl_yaw_deg = quat_to_yaw_deg(q.x, q.y, q.z, q.w)

    def cb_odom(self, msg: Odometry):
        t = msg.twist.twist
        self.odom_v = float(t.linear.x)
        self.odom_w = float(t.angular.z)

    def cb_goal(self, msg: PoseStamped):
        p = msg.pose.position
        q = msg.pose.orientation
        self.goal_x = float(p.x)
        self.goal_y = float(p.y)
        self.goal_yaw_deg = quat_to_yaw_deg(q.x, q.y, q.z, q.w)

    def cb_scan(self, msg: LaserScan):
        # 전방 ±window 범위에서 최소거리 계산
        a_min = msg.angle_min
        inc = msg.angle_increment
        if inc == 0.0 or not msg.ranges:
            return

        start_ang = self.front_angle - self.window
        end_ang   = self.front_angle + self.window

        start_i = int(round((start_ang - a_min) / inc))
        end_i   = int(round((end_ang   - a_min) / inc))

        start_i = max(0, min(start_i, len(msg.ranges) - 1))
        end_i   = max(0, min(end_i,   len(msg.ranges) - 1))
        if end_i < start_i:
            start_i, end_i = end_i, start_i

        best = None
        for r in msg.ranges[start_i:end_i + 1]:
            if math.isfinite(r) and r > self.min_range_floor:
                if best is None or r < best:
                    best = r

        if best is not None:
            self.lidar_front_min_m = float(best)

    def cb_ultra(self, msg: Float32):
        self.ultra_cm = float(msg.data)

    def cb_seat(self, msg: Bool):
        self.seat_bool = bool(msg.data)

    # -----------------------------
    # 주기 송신
    # -----------------------------
    def tick_send(self):
        # 없는 값은 -1 또는 NaN으로 표시(STM32 파싱 쉽게)
        batt_p = self.batt_percent if self.batt_percent is not None else -1.0
        batt_v = self.batt_voltage if self.batt_voltage is not None else -1.0

        x = self.amcl_x if self.amcl_x is not None else 0.0
        y = self.amcl_y if self.amcl_y is not None else 0.0
        yaw = self.amcl_yaw_deg if self.amcl_yaw_deg is not None else 0.0

        v = self.odom_v if self.odom_v is not None else 0.0
        w = self.odom_w if self.odom_w is not None else 0.0

        gx = self.goal_x if self.goal_x is not None else 0.0
        gy = self.goal_y if self.goal_y is not None else 0.0
        gyaw = self.goal_yaw_deg if self.goal_yaw_deg is not None else 0.0

        lmin = self.lidar_front_min_m if self.lidar_front_min_m is not None else -1.0

        ultra = self.ultra_cm if (self.use_ultra_seat and self.ultra_cm is not None) else -1.0
        seat  = 1 if (self.use_ultra_seat and self.seat_bool) else 0

        # 한 줄 요약 문자열 (STM32에서 ',' split 후 '=' split 하면 끝)
        # 예: BAT=62.3,BV=12.1,X=1.23,Y=-0.45,YAW=92.0,V=0.12,W=0.03,GX=2.00,GY=1.20,LMIN=0.32,U=34.5,SEAT=1
        line = (
            f"BAT={batt_p:.1f},BV={batt_v:.2f},"
            f"X={x:.2f},Y={y:.2f},YAW={yaw:.1f},"
            f"V={v:.2f},W={w:.2f},"
            f"GX={gx:.2f},GY={gy:.2f},GYAW={gyaw:.1f},"
            f"LMIN={lmin:.2f},"
            f"U={ultra:.2f},SEAT={seat}\r\n"
        )

        self._safe_write(line)


def main():
    rclpy.init()
    node = RosStateToStm32()
    try:
        rclpy.spin(node)
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

