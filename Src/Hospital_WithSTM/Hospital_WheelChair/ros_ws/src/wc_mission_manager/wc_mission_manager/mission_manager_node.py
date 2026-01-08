import json
import math
import rclpy
from rclpy.node import Node

from std_msgs.msg import String
from geometry_msgs.msg import PoseStamped, Quaternion
from nav2_msgs.action import NavigateToPose
from rclpy.action import ActionClient


def yaw_to_quaternion(yaw: float) -> Quaternion:
    """
    Yaw 각도(라디안)를 ROS의 geometry_msgs/Quaternion 메시지로 변환하는 함수.
    Nav2는 목표 방향을 Quaternion(x, y, z, w) 형태로 받기 때문에 변환이 필요합니다.
    (Z축 회전만 고려)
    """
    q = Quaternion()
    q.w = math.cos(yaw * 0.5)
    q.x = 0.0
    q.y = 0.0
    q.z = math.sin(yaw * 0.5)
    return q


class MissionManager(Node):
    def __init__(self):
        super().__init__('mission_manager')

        # ---------------------------------------------------------
        # 1. ROS 파라미터 설정 (외부에서 설정 가능)
        # ---------------------------------------------------------
        # robot_id: 다중 로봇 환경에서 내 로봇을 식별하기 위한 ID
        self.robot_id = self.declare_parameter('robot_id', 1).value
        # frame_id: 좌표계 기준 (보통 'map' 사용)
        self.frame_id = self.declare_parameter('frame_id', 'map').value
        # allow_preempt: 이동 중 새 명령이 왔을 때 기존 것을 취소하고 새 것을 수행할지 여부
        self.allow_preempt = self.declare_parameter('allow_preempt', False).value
        # action_name: Nav2 액션 서버 이름 (기본값: navigate_to_pose)
        self.action_name = self.declare_parameter('action_name', 'navigate_to_pose').value

        # ---------------------------------------------------------
        # 2. 내부 상태 변수
        # ---------------------------------------------------------
        self._busy = False          # 로봇이 현재 작업을 수행 중인지 여부
        self._current_order = None  # 현재 수행 중인 명령의 order 번호
        self._goal_handle = None    # 현재 수행 중인 Nav2 액션의 핸들 (취소 시 필요)

        # ---------------------------------------------------------
        # 3. ROS 퍼블리셔/서브스크라이버 설정
        # ---------------------------------------------------------
        # [구독] 서버나 상위 제어기에서 보내는 목표 지점 (JSON 포맷)
        self.goal_sub = self.create_subscription(
            String, 'mission/goal', self.on_goal_msg, 10
        )
        # [발행] 명령을 잘 받았다는 확인 메시지 (ACK)
        self.ack_pub = self.create_publisher(String, 'mission/ack', 10)
        # [발행] 주행 완료/실패 등 최종 결과 보고
        self.result_pub = self.create_publisher(String, 'mission/result', 10)

        # ---------------------------------------------------------
        # 4. Nav2 액션 클라이언트 생성
        # ---------------------------------------------------------
        self.nav_client = ActionClient(self, NavigateToPose, self.action_name)

        self.get_logger().info(
            f"MissionManager started (robot_id={self.robot_id}, frame_id={self.frame_id}, "
            f"allow_preempt={self.allow_preempt}, action={self.action_name})"
        )

    def publish_ack(self, robot_id: int, order: int, accepted: bool, reason: str = ""):
        """
        명령 수신 확인(ACK) 메시지를 JSON으로 발행합니다.
        서버는 이 메시지를 받고 'order' 값을 0으로 초기화할 수 있습니다.
        """
        msg = String()
        msg.data = json.dumps({
            "robot_id": robot_id,
            "order": order,
            "ack": bool(accepted),  # True: 수락, False: 거절
            "reason": reason        # 거절 시 이유 (예: BUSY)
        }, ensure_ascii=False)
        self.ack_pub.publish(msg)

    def publish_result(self, robot_id: int, order: int, result: str, detail: str = ""):
        """
        주행 최종 결과(도착, 실패 등)를 JSON으로 발행합니다.
        """
        msg = String()
        msg.data = json.dumps({
            "robot_id": robot_id,
            "order": order,
            "result": result,   # 예: ARRIVED, FAILED, CANCELED, REJECTED
            "detail": detail    # 상세 내용
        }, ensure_ascii=False)
        self.result_pub.publish(msg)

    def on_goal_msg(self, msg: String):
        """
        [콜백] 'mission/goal' 토픽으로 메시지가 들어오면 실행됩니다.
        예상 JSON 포맷:
        {"robot_id":1,"order":1,"goal_x":1.2,"goal_y":3.4,"goal_theta":0.0,"who_called":"..."}
        """
        # 1. JSON 파싱
        try:
            data = json.loads(msg.data)
        except Exception as e:
            self.get_logger().warn(f"Invalid JSON on mission/goal: {e}")
            return

        # 2. 파라미터 추출 및 유효성 검사
        robot_id = int(data.get("robot_id", -1))
        order = int(data.get("order", 0))

        # 내 로봇 ID가 아니면 무시
        if robot_id != self.robot_id:
            return 

        # order가 1인 경우만 새로운 명령으로 간주
        if order != 1:
            return

        goal_x = float(data.get("goal_x", 0.0))
        goal_y = float(data.get("goal_y", 0.0))
        goal_theta = float(data.get("goal_theta", 0.0))  # 목표 방향 (없으면 0.0)
        who_called = str(data.get("who_called", ""))

        # 3. 중복 명령 / Busy 상태 처리
        # 로봇이 이미 주행 중(_busy=True)이고, 선점(Preempt)이 비활성화된 경우 -> 거절
        if self._busy and not self.allow_preempt:
            self.publish_ack(robot_id, order, False, "BUSY")
            self.publish_result(robot_id, order, "REJECTED", "busy and preempt disabled")
            self.get_logger().warn("Received goal while busy -> rejected (allow_preempt=false)")
            return

        # 선점(Preempt)이 허용된 경우 -> 기존 목표 취소 요청
        if self._busy and self.allow_preempt and self._goal_handle is not None:
            self.get_logger().warn("Preempting current goal...")
            # 비동기 취소 요청 (여기서는 취소 완료를 기다리지 않고 바로 새 명령을 수행하도록 구현됨)
            self._goal_handle.cancel_goal_async()

        # 4. 명령 수락 ACK 전송
        # 서버가 ACK를 받아야 계속해서 명령을 보내는 것을 멈출 수 있음
        self.publish_ack(robot_id, order, True, "")

        # 5. Nav2 액션 서버 연결 확인
        if not self.nav_client.wait_for_server(timeout_sec=2.0):
            self.publish_result(robot_id, order, "FAILED", "nav2 action server not available")
            self.get_logger().error("Nav2 action server not available.")
            return

        # 6. Nav2 목표 메시지(Goal Message) 생성
        pose = PoseStamped()
        pose.header.frame_id = self.frame_id
        pose.header.stamp = self.get_clock().now().to_msg()
        pose.pose.position.x = goal_x
        pose.pose.position.y = goal_y
        pose.pose.position.z = 0.0
        pose.pose.orientation = yaw_to_quaternion(goal_theta)

        goal_msg = NavigateToPose.Goal()
        goal_msg.pose = pose

        # 7. 상태 업데이트 및 액션 전송
        self._busy = True
        self._current_order = order

        self.get_logger().info(
            f"Goal received (who_called={who_called}) -> x={goal_x}, y={goal_y}, yaw={goal_theta}"
        )

        # 비동기로 목표 전송 후, 수락 여부를 확인하는 콜백 등록
        send_future = self.nav_client.send_goal_async(goal_msg)
        send_future.add_done_callback(lambda f: self.on_goal_response(f, robot_id, order))

    def on_goal_response(self, future, robot_id: int, order: int):
        """
        [콜백] Nav2 액션 서버가 목표를 수락했는지 거절했는지 응답이 오면 실행됩니다.
        """
        try:
            goal_handle = future.result()
        except Exception as e:
            self._busy = False
            self.publish_result(robot_id, order, "FAILED", f"send_goal exception: {e}")
            self.get_logger().error(f"send_goal exception: {e}")
            return

        # Nav2가 목표를 거절한 경우 (예: 유효하지 않은 좌표 등)
        if not goal_handle.accepted:
            self._busy = False
            self.publish_result(robot_id, order, "FAILED", "goal rejected by nav2")
            self.get_logger().error("Goal rejected by Nav2.")
            return

        # 목표 수락됨 -> 주행 시작
        self._goal_handle = goal_handle
        self.get_logger().info("Goal accepted by Nav2. Navigating...")

        # 주행 결과(성공/실패)를 기다리는 콜백 등록
        result_future = goal_handle.get_result_async()
        result_future.add_done_callback(lambda f: self.on_result(f, robot_id, order))

    def on_result(self, future, robot_id: int, order: int):
        """
        [콜백] 주행이 완료되거나 취소되었을 때 최종 결과를 처리합니다.
        """
        self._busy = False  # 주행 종료 상태로 변경

        try:
            result = future.result()
        except Exception as e:
            self.publish_result(robot_id, order, "FAILED", f"get_result exception: {e}")
            self.get_logger().error(f"get_result exception: {e}")
            return

        status = result.status
        # action status codes: 
        # STATUS_SUCCEEDED = 4
        # STATUS_ABORTED = 5
        # STATUS_CANCELED = 6
        
        if status == 4: # SUCCEEDED
            self.publish_result(robot_id, order, "ARRIVED", "")
            self.get_logger().info("Navigation succeeded.")
        elif status == 5: # ABORTED (주행 불가, 장애물 막힘 등)
            self.publish_result(robot_id, order, "FAILED", "Goal aborted by Nav2")
            self.get_logger().warn("Navigation aborted.")
        elif status == 6: # CANCELED (사용자 취소 또는 선점)
            self.publish_result(robot_id, order, "CANCELED", "Goal canceled")
            self.get_logger().warn("Navigation canceled.")
        else: # 그 외 상태
            self.publish_result(robot_id, order, "UNKNOWN", f"Status code: {status}")