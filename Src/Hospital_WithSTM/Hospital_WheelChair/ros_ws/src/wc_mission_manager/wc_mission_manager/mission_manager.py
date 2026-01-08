#!/usr/bin/env python3
import math
from typing import Optional

import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient

from std_msgs.msg import String
from geometry_msgs.msg import PoseStamped
from nav2_msgs.action import NavigateToPose


class WcMissionManager(Node):
    """
    - Sub:  goal (PoseStamped)          [상대 토픽]
    - Pub:  mission_status (String)     [상대 토픽]
    - Action: navigate_to_pose (NavigateToPose) [상대 액션명]

    기본 정책:
    - RUNNING 중 새 goal 들어오면 기존 goal cancel 후 새 goal 전송 (preempt=true)
    """

    def __init__(self):
        super().__init__('wc_mission_manager')

        # -------- Parameters --------
        self.declare_parameter('goal_topic', 'goal')
        self.declare_parameter('status_topic', 'mission_status')
        # 절대 경로 사용 (/를 붙여서 네임스페이스 영향 안 받게 설정)
        self.declare_parameter('action_name', '/navigate_to_pose')
        self.declare_parameter('preempt_running_goal', True)

        self.goal_topic = self.get_parameter('goal_topic').value
        self.status_topic = self.get_parameter('status_topic').value
        self.action_name = self.get_parameter('action_name').value
        self.preempt = bool(self.get_parameter('preempt_running_goal').value)

        # -------- Pub/Sub --------
        self.status_pub = self.create_publisher(String, self.status_topic, 10)
        self.goal_sub = self.create_subscription(PoseStamped, self.goal_topic, self.on_goal, 10)

        # -------- Action Client --------
        self.nav_client = ActionClient(self, NavigateToPose, self.action_name)

        # -------- State --------
        self.current_goal_handle = None
        self.pending_goal: Optional[PoseStamped] = None
        self.running = False
        self.last_goal: Optional[PoseStamped] = None

        # 초기 상태 WAITING
        self.publish_status("WAITING")

        # Nav2 서버 준비 체크(주기)
        self.timer = self.create_timer(0.5, self._tick_wait_nav2)

    def publish_status(self, text: str, log: bool = True):
        """
        상태를 토픽으로 발행하고, log가 True일 때만 콘솔에 출력합니다.
        """
        msg = String()
        msg.data = text
        self.status_pub.publish(msg)
        
        # log 옵션이 True일 때만 콘솔 출력 (도배 방지)
        if log:
            self.get_logger().info(f"STATUS: {text}")

    def _tick_wait_nav2(self):
        if self.nav_client.server_is_ready():
            # 한번만 출력하고 타이머 정지
            self.publish_status("READY (Nav2 action server up)")
            self.timer.cancel()

# 1. 기존 on_goal을 이걸로 대체 (Order 분기 처리)
    def on_goal(self, msg: PoseStamped):
        # TCP Bridge가 z 좌표에 숨겨 보낸 order 값을 추출 (int 변환)
        order = int(msg.pose.position.z)
        
        self.get_logger().info(f"[RX] Order: {order}, x={msg.pose.position.x:.2f}, y={msg.pose.position.y:.2f}")

        if order == 1:
            # [Order 1] 일반 이동
            msg.pose.position.z = 0.0  
            self.last_goal = msg       # 재개를 위해 저장
            self._handle_nav_request(msg) 

        elif order == 2:
            # [Order 2] 정지 (취소)
            self.get_logger().warn(">> Order 2 Received: STOPPING ROBOT")
            self.cancel_current_goal()
            self.publish_status("STOPPED_BY_ORDER")

        elif order == 3:
            # [Order 3] 동작 재개
            if self.last_goal is not None:
                self.get_logger().info(">> Order 3 Received: RESUMING last goal")
                self._handle_nav_request(self.last_goal)
            else:
                self.get_logger().warn("Order 3 ignored: No last goal to resume")

        elif order == 4:
            # [Order 4] 대기 스테이션 귀환
            self.get_logger().info(">> Order 4 Received: Return to Waiting Station")
            self.go_to_station(msg)

        elif order == 5:
            # [Order 5] 충전 스테이션 이동
            self.get_logger().info(">> Order 5 Received: Go to Charging Station")
            self.go_to_station(msg)
        
        else:
            self.get_logger().warn(f"Unknown Order ID: {order}")

    def _handle_nav_request(self, pose: PoseStamped):
        # Nav2 서버 체크
        if not self.nav_client.server_is_ready():
            self.publish_status("WAITING_NAV2 (action server not ready)")
            self.pending_goal = pose
            return

        # 이미 주행 중일 때 처리 (Preempt 설정에 따름)
        if self.running:
            if self.preempt:
                self.publish_status("PREEMPT (cancel current goal)")
                self.pending_goal = pose  
                self.cancel_current_goal()
            else:
                self.publish_status("IGNORED (already running)")
            return

        # 아무 문제 없으면 전송
        self.send_goal(pose)

    def send_goal(self, pose: PoseStamped):
        goal_msg = NavigateToPose.Goal()
        goal_msg.pose = pose

        # 목표 수락됨 로그
        self.get_logger().info(f"Sending Goal to Nav2: x={pose.pose.position.x:.2f}, y={pose.pose.position.y:.2f}")

        send_future = self.nav_client.send_goal_async(goal_msg, feedback_callback=self.feedback_cb)
        send_future.add_done_callback(self.goal_response_cb)

    def goal_response_cb(self, future):
        goal_handle = future.result()
        if not goal_handle.accepted:
            self.running = False
            self.current_goal_handle = None
            self.publish_status("REJECTED (Nav2 did not accept goal)")
            self._send_pending_if_any()
            return

        self.current_goal_handle = goal_handle
        self.running = True
        
        # 여기서 "RUNNING"을 한 번만 출력합니다.
        self.publish_status("RUNNING")

        result_future = goal_handle.get_result_async()
        result_future.add_done_callback(self.result_cb)

    def feedback_cb(self, feedback_msg):
        fb = feedback_msg.feedback
        if hasattr(fb, 'distance_remaining'):
            # log=False로 설정하여 이동 중에는 콘솔에 로그를 찍지 않고 토픽만 보냅니다.
            self.publish_status(f"RUNNING dist_remaining={fb.distance_remaining:.2f}", log=False)

    def result_cb(self, future):
        res = future.result()
        status = res.status  # 4:SUCCEEDED, 5:CANCELED, 6:ABORTED 등이 일반적

        self.running = False
        self.current_goal_handle = None

        if status == 4:
            self.publish_status("SUCCEEDED")
        elif status == 5:
            self.publish_status("CANCELED")
        elif status == 6:
            self.publish_status("FAILED (ABORTED)")
        else:
            self.publish_status(f"FINISHED status_code={status}")

        self._send_pending_if_any()
        
        if not self.running:
            # 도착 후 대기 상태일 때 WAITING 출력
            self.publish_status("WAITING")

    def cancel_current_goal(self):
        if self.current_goal_handle is None:
            self._send_pending_if_any()
            return
        cancel_future = self.current_goal_handle.cancel_goal_async()
        cancel_future.add_done_callback(self.cancel_done_cb)

    def cancel_done_cb(self, future):
        _ = future.result()
        self.publish_status("CANCEL_REQUEST_SENT")
        # 실제 CANCELED 결과는 result_cb에서 들어오는 게 정상 흐름

    def _send_pending_if_any(self):
        if self.pending_goal is None:
            return
        pose = self.pending_goal
        self.pending_goal = None
        # cancel 직후라면 아직 running이 false가 아닐 수 있어, 안전하게 재시도
        if not self.running and self.nav_client.server_is_ready():
            self.send_goal(pose)

def main(args=None):
    rclpy.init(args=args)
    node = WcMissionManager()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()