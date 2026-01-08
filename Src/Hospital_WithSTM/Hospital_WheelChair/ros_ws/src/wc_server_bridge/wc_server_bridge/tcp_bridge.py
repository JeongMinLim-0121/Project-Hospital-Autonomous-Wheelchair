import socket
import struct
import threading
import time
import math

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import PoseWithCovarianceStamped, Quaternion, PoseStamped
from nav_msgs.msg import Odometry
from sensor_msgs.msg import BatteryState

# ==========================================
# 1. 프로토콜 정의
# ==========================================
MAGIC_NUMBER = 0xAB
DEVICE_ROBOT_ROS = 0x02

# 메시지 타입
MSG_LOGIN_REQ   = 0x01
MSG_ROBOT_STATE = 0x20
MSG_ASSIGN_GOAL = 0x30

# 로봇 상태 상수
STATE_WAITING  = 0
STATE_HEADING  = 1
STATE_BOARDING = 2
STATE_RUNNING  = 3
STATE_STOP     = 4
STATE_ARRIVED  = 5
STATE_EXITING  = 6
STATE_CHARGING = 7
STATE_ERROR    = 99

# 헤더 포맷
HDR_FMT = "<BBBB"
HDR_SIZE = struct.calcsize(HDR_FMT)

# 상태 데이터 포맷
STATE_FMT = "<ifffB"
STATE_SIZE = struct.calcsize(STATE_FMT)

# 목표 데이터 포맷
GOAL_FMT = "<iff"
GOAL_SIZE = struct.calcsize(GOAL_FMT)


class TcpBridge(Node):
    def __init__(self):
        super().__init__("tcp_bridge")

        # ------------------------------------------
        # 2. 파라미터 설정
        # ------------------------------------------
        self.server_ip = self.declare_parameter("server_ip", "10.10.14.138").value
        self.server_port = int(self.declare_parameter("server_port", 8080).value)
        self.robot_name = self.declare_parameter("robot_name", "wc1").value
        self.use_amcl_pose = bool(self.declare_parameter("use_amcl_pose", True).value)
        self.tx_hz = float(self.declare_parameter("tx_hz", 2.0).value)
        #goal publish 설정
        self.goal_frame = self.declare_parameter("goal_frame", "map").value
        self.goal_topic = self.declare_parameter("goal_topic", "goal").value

        # ------------------------------------------
        # 3. 상태 변수 초기화
        # ------------------------------------------
        self.x = 0.0
        self.y = 0.0
        self.theta = 0.0
        self.battery_percent = 90
        
        # 현재 상태
        self.current_state = STATE_WAITING 

        # TCP 네트워크 변수
        self.sock = None
        self.lock = threading.Lock()
        self.logged_in = False
        self.running = True 

        # 재접속(Backoff) 변수
        self.backoff = 1.0
        self.backoff_max = 10.0
        self.next_connect_time = 0.0

        # ------------------------------------------
        # 4. 토픽 구독
        # ------------------------------------------
        if self.use_amcl_pose:
            self.create_subscription(PoseWithCovarianceStamped, "/amcl_pose", self.pose_cb, 10)
        else:
            self.create_subscription(Odometry, "/odom", self.odom_pose_cb, 10)

        self.create_subscription(Odometry, "/odom", self.odom_twist_cb, 10)
        self.create_subscription(BatteryState, "/battery_state", self.batt_cb, 10)
        # 서버 goal -> ROS goal publish
        self.goal_pub = self.create_publisher(PoseStamped, self.goal_topic, 10)
        self.order_pub = self.create_publisher(PoseStamped, "mission_order", 10)

        # ------------------------------------------
        # 5. 타이머 및 스레드 시작
        # ------------------------------------------
        period = 1.0 / max(0.1, self.tx_hz)
        self.create_timer(period, self.tx_timer_cb)

        self.rx_thread = threading.Thread(target=self.rx_loop, daemon=True)
        self.rx_thread.start()

        # 이제 이 함수가 아래에 정의되어 있으므로 에러가 나지 않습니다.
        state_name = self.get_state_name(self.current_state)
        self.get_logger().info(f"TCP Bridge Started. Robot: {self.robot_name}, State: {state_name}")

    # ==========================================
    # 상태 이름 변환 헬퍼 함수
    # ==========================================
    def get_state_name(self, state_id):
        names = {
            STATE_WAITING: "WAITING",
            STATE_HEADING: "HEADING",
            STATE_BOARDING: "BOARDING",
            STATE_RUNNING: "RUNNING",
            STATE_STOP: "STOP",
            STATE_ARRIVED: "ARRIVED",
            STATE_EXITING: "EXITING",
            STATE_CHARGING: "CHARGING",
            STATE_ERROR: "ERROR"
        }
        return names.get(state_id, "UNKNOWN")

    def change_state(self, new_state):
        if self.current_state != new_state:
            old_name = self.get_state_name(self.current_state)
            new_name = self.get_state_name(new_state)
            self.get_logger().info(f"State Change: {old_name} -> {new_name}")
            self.current_state = new_state

    # ==========================================
    # ROS 콜백
    # ==========================================
    def quaternion_to_yaw(self, q: Quaternion) -> float:
        siny_cosp = 2.0 * (q.w * q.z + q.x * q.y)
        cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z)
        return math.atan2(siny_cosp, cosy_cosp)

    def yaw_to_quaternion(self, yaw: float) -> Quaternion:
        q = Quaternion()
        q.x = 0.0
        q.y = 0.0
        q.z = math.sin(yaw * 0.5)
        q.w = math.cos(yaw * 0.5)
        return q

    def pose_cb(self, msg: PoseWithCovarianceStamped):
        self.x = float(msg.pose.pose.position.x)
        self.y = float(msg.pose.pose.position.y)
        self.theta = self.quaternion_to_yaw(msg.pose.pose.orientation)

    def odom_pose_cb(self, msg: Odometry):
        self.x = float(msg.pose.pose.position.x)
        self.y = float(msg.pose.pose.position.y)
        self.theta = self.quaternion_to_yaw(msg.pose.pose.orientation)

    def odom_twist_cb(self, msg: Odometry):
        pass 

    def batt_cb(self, msg: BatteryState):
        if msg.percentage is not None and msg.percentage >= 0.0:
            raw_val = msg.percentage

            if raw_val > 1.0:
                p = int(raw_val)
            else:
                p = int(raw_val * 100.0)
                
            self.battery_percent = max(0, min(100, p))

    # ==========================================
    # TCP 네트워크 관리
    # ==========================================
    def _set_keepalive(self, s: socket.socket):
        try:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
        except Exception:
            pass

    def connect(self) -> bool:
        now = time.time()
        if now < self.next_connect_time:
            return False

        with self.lock:
            if self.sock: return True
            
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self._set_keepalive(s)
                s.settimeout(3.0)
                s.connect((self.server_ip, self.server_port))
                s.settimeout(None) 
                
                self.sock = s
                self.logged_in = False
                self.backoff = 1.0
                self.next_connect_time = 0.0
                self.get_logger().info("TCP connected")
                return True
            except Exception as e:
                if self.sock: s.close()
                self.sock = None
                self.logged_in = False
                self.next_connect_time = now + self.backoff
                self.backoff = min(self.backoff * 2.0, self.backoff_max)
                return False

    def close_socket(self, reason: str):
        with self.lock:
            if self.sock:
                try:
                    self.sock.close()
                except Exception:
                    pass
            self.sock = None
            self.logged_in = False
        self.get_logger().warn(f"Socket closed: {reason}")

    def build_packet(self, msg_type: int, payload: bytes) -> bytes:
        if len(payload) > 255:
            raise ValueError("payload_len max 255")
        header = struct.pack(HDR_FMT, MAGIC_NUMBER, DEVICE_ROBOT_ROS, msg_type, len(payload))
        return header + payload

    def send_packet(self, msg_type: int, payload: bytes):
        pkt = self.build_packet(msg_type, payload)
        with self.lock:
            if not self.sock:
                raise ConnectionError("no socket")
            self.sock.sendall(pkt)

    # ==========================================
    # 수신 루프 (RX Loop)
    # ==========================================
    def recvall(self, sock: socket.socket, n: int) -> bytes:
        data = b""
        while len(data) < n:
            chunk = sock.recv(n - len(data))
            if not chunk:
                return b""
            data += chunk
        return data


    def rx_loop(self):
        while self.running and rclpy.ok():
            with self.lock:
                current_sock = self.sock
            
            if current_sock is None:
                time.sleep(1.0)
                continue

            try:
                header_data = self.recvall(current_sock, HDR_SIZE)
                if not header_data:
                    self.close_socket("Remote close")
                    continue

                magic, dev, msg_type, payload_len = struct.unpack(HDR_FMT, header_data)
                if magic != MAGIC_NUMBER:
                    continue

                payload_data = b""
                if payload_len > 0:
                    payload_data = self.recvall(current_sock, payload_len)
                    if not payload_data:
                        self.close_socket("Remote close (payload)")
                        continue

                self.handle_server_message(msg_type, payload_data)

            except OSError:
                self.close_socket("RX Error")
            except Exception as e:
                self.get_logger().error(f"RX Loop Error: {e}")
                self.close_socket("RX Exception")

    def handle_server_message(self, msg_type, payload):
        if msg_type != MSG_ASSIGN_GOAL:
            return

        if len(payload) != GOAL_SIZE:
            self.get_logger().warn("Goal payload size mismatch")
            return

        order_val, gx, gy = struct.unpack(GOAL_FMT, payload)
        self.get_logger().info(f"★ NEW GOAL: x={gx:.2f}, y={gy:.2f}")

        goal = PoseStamped()
        goal.header.stamp = self.get_clock().now().to_msg()
        goal.header.frame_id = self.goal_frame

        goal.pose.position.x = float(gx)
        goal.pose.position.y = float(gy)
        goal.pose.position.z = float(order_val)
        goal.pose.orientation = self.yaw_to_quaternion(0.0)

        self.goal_pub.publish(goal)
        self.get_logger().info(
            f"[TCP->ROS] publish {self.goal_topic}: x={gx:.2f}, y={gy:.2f}, frame={self.goal_frame}"
        )

        if order_val == 1:
            if self.current_state == STATE_WAITING:
                self.change_state(STATE_HEADING)
        elif order_val == 2:
            self.change_state(STATE_STOP)
        elif order_val == 3:
            self.change_state(STATE_HEADING)
        else:
            self.change_state(STATE_CHARGING)

    # ==========================================
    # 메인 송신 로직
    # ==========================================
    def send_login_once(self):
        if self.logged_in: return
        
        name_bytes = self.robot_name.encode("utf-8")[:32]
        self.send_packet(MSG_LOGIN_REQ, name_bytes)
        self.logged_in = True
        self.get_logger().info(f"LOGIN sent: {self.robot_name}")

    def tx_timer_cb(self):
        if not self.connect(): return

        try:
            self.send_login_once()

            payload = struct.pack(
                STATE_FMT,
                int(self.battery_percent),
                float(self.x),
                float(self.y),
                float(self.theta),
                int(self.current_state)
            )
            
            self.send_packet(MSG_ROBOT_STATE, payload)

        except Exception as e:
            self.close_socket(f"TX failed: {e}")


def main():
    rclpy.init()
    node = TcpBridge()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.running = False
        node.close_socket("shutdown")
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()