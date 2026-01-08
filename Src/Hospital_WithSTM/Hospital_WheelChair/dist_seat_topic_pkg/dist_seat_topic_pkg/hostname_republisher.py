#!/usr/bin/env python3
import math
import socket

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, DurabilityPolicy

from sensor_msgs.msg import LaserScan, BatteryState
from nav_msgs.msg import Odometry
from geometry_msgs.msg import PoseStamped, PoseWithCovarianceStamped
from std_msgs.msg import Float32


class HostnameRepublisher(Node):
    def __init__(self):
        super().__init__('hostname_republisher')

        # -----------------------------
        # 1) 파라미터 (원하면 런타임에 바꿀 수 있음)
        # -----------------------------
        # robot_ns를 비워두면 hostname을 자동으로 씀: "/pi" or "/ubuntu"
        self.declare_parameter('robot_ns', '')
        self.declare_parameter('use_hostname_namespace', True)

        # 입력 토픽(=이미 시스템에서 발행되는 토픽)
        self.declare_parameter('in_amcl_topic', '/amcl_pose')
        self.declare_parameter('in_odom_topic', '/odom')
        self.declare_parameter('in_battery_topic', '/battery_state')
        self.declare_parameter('in_goal_topic', '/goal_pose')
        self.declare_parameter('in_scan_topic', '/scan')

        # 라이다 근접 거리 계산/발행 옵션
        self.declare_parameter('publish_lidar_min', True)          # True면 /<ns>/lidar_min_range_m 발행
        self.declare_parameter('front_angle_deg', 0.0)             # 전방 기준 각도(도) (보통 0)
        self.declare_parameter('window_deg', 5.0)                  # 전방 ±윈도우(도)
        self.declare_parameter('min_range_floor_m', 0.02)          # 너무 작은 값(노이즈) 컷

        # -----------------------------
        # 2) 네임스페이스 결정 (출력 토픽에만 적용)
        # -----------------------------
        use_host = bool(self.get_parameter('use_hostname_namespace').value)
        robot_ns = str(self.get_parameter('robot_ns').value).strip()

        if not robot_ns and use_host:
            host = socket.gethostname().strip()    # 예: "pi", "ubuntu"
            robot_ns = f'/{host}'                  # 예: "/pi"
        elif robot_ns and not robot_ns.startswith('/'):
            robot_ns = '/' + robot_ns              # "pi" -> "/pi"

        self.robot_ns = robot_ns                   # 최종 네임스페이스

        def out_name(name: str) -> str:
            """
            출력 토픽명 생성:
            - name이 "/scan" 같은 절대경로면 "/pi/scan"처럼 앞에 붙임
            - robot_ns가 ""이면 그대로 사용
            """
            if not self.robot_ns:
                return name
            if name.startswith('/'):
                return self.robot_ns + name
            return self.robot_ns + '/' + name

        # -----------------------------
        # 3) 입력 토픽명 (구독용) / 출력 토픽명 (발행용)
        #    ⚠️ 입력에는 네임스페이스를 붙이지 않음(원본 토픽을 받아야 하니까)
        # -----------------------------
        self.in_amcl = str(self.get_parameter('in_amcl_topic').value)
        self.in_odom = str(self.get_parameter('in_odom_topic').value)
        self.in_batt = str(self.get_parameter('in_battery_topic').value)
        self.in_goal = str(self.get_parameter('in_goal_topic').value)
        self.in_scan = str(self.get_parameter('in_scan_topic').value)

        self.out_amcl = out_name(self.in_amcl)
        self.out_odom = out_name(self.in_odom)
        self.out_batt = out_name(self.in_batt)
        self.out_goal = out_name(self.in_goal)
        self.out_scan = out_name(self.in_scan)

        # 라이다 근접거리 출력 토픽
        self.publish_lidar_min = bool(self.get_parameter('publish_lidar_min').value)
        self.out_lidar_min = out_name('/lidar_min_range_m')  # 예: "/pi/lidar_min_range_m"

        # -----------------------------
        # 4) QoS 설정
        # -----------------------------
        # scan은 센서라 BEST_EFFORT가 흔함 (Nav2/라이다 드라이버도 대개 best_effort)
        qos_sensor = QoSProfile(depth=10)
        qos_sensor.reliability = ReliabilityPolicy.BEST_EFFORT
        qos_sensor.durability = DurabilityPolicy.VOLATILE

        # 나머지는 기본 QoS(대부분 Reliable로 문제 없음)
        qos_default = QoSProfile(depth=10)

        # -----------------------------
        # 5) 퍼블리셔(재발행)
        # -----------------------------
        self.pub_amcl = self.create_publisher(PoseWithCovarianceStamped, self.out_amcl, qos_default)
        self.pub_odom = self.create_publisher(Odometry, self.out_odom, qos_default)
        self.pub_batt = self.create_publisher(BatteryState, self.out_batt, qos_default)
        self.pub_goal = self.create_publisher(PoseStamped, self.out_goal, qos_default)
        self.pub_scan = self.create_publisher(LaserScan, self.out_scan, qos_sensor)

        if self.publish_lidar_min:
            self.pub_lidar_min = self.create_publisher(Float32, self.out_lidar_min, qos_sensor)
        else:
            self.pub_lidar_min = None

        # -----------------------------
        # 6) 라이다 근접거리 계산 파라미터 캐시
        # -----------------------------
        self.front_angle = math.radians(float(self.get_parameter('front_angle_deg').value))
        self.window = math.radians(float(self.get_parameter('window_deg').value))
        self.min_range_floor = float(self.get_parameter('min_range_floor_m').value)

        # -----------------------------
        # 7) 구독자(원본 토픽 받기)
        # -----------------------------
        self.create_subscription(PoseWithCovarianceStamped, self.in_amcl, self.cb_amcl, qos_default)
        self.create_subscription(Odometry, self.in_odom, self.cb_odom, qos_default)
        self.create_subscription(BatteryState, self.in_batt, self.cb_batt, qos_default)
        self.create_subscription(PoseStamped, self.in_goal, self.cb_goal, qos_default)
        self.create_subscription(LaserScan, self.in_scan, self.cb_scan, qos_sensor)

        # 로그로 현재 설정 출력
        self.get_logger().info(
            f"[HostnameRepublisher] robot_ns='{self.robot_ns}'\n"
            f"  SUB: amcl={self.in_amcl}, odom={self.in_odom}, batt={self.in_batt}, goal={self.in_goal}, scan={self.in_scan}\n"
            f"  PUB: amcl={self.out_amcl}, odom={self.out_odom}, batt={self.out_batt}, goal={self.out_goal}, scan={self.out_scan}\n"
            f"  PUB: lidar_min={self.out_lidar_min if self.publish_lidar_min else '(off)'}"
        )

    # -----------------------------
    # 콜백들: "받은 메시지를 그대로" 네임스페이스 토픽으로 재발행
    # -----------------------------
    def cb_amcl(self, msg: PoseWithCovarianceStamped):
        self.pub_amcl.publish(msg)

    def cb_odom(self, msg: Odometry):
        self.pub_odom.publish(msg)

    def cb_batt(self, msg: BatteryState):
        self.pub_batt.publish(msg)

    def cb_goal(self, msg: PoseStamped):
        self.pub_goal.publish(msg)

    def cb_scan(self, msg: LaserScan):
        # 1) scan 자체를 네임스페이스 토픽으로 그대로 재발행
        self.pub_scan.publish(msg)

        # 2) 전방 ±window 구간의 최소거리(min range) 계산해서 Float32로 발행
        if self.pub_lidar_min is None:
            return

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
            self.pub_lidar_min.publish(Float32(data=float(best)))


def main():
    rclpy.init()
    node = HostnameRepublisher()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
