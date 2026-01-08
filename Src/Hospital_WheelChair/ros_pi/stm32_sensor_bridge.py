#!/usr/bin/env python3
import re
import serial
import rclpy
from rclpy.node import Node
from std_msgs.msg import Float32, Bool

# 예: U=12.34,FSR=59,SEAT=0
LINE_RE = re.compile(
    r'U\s*=\s*([-+]?\d+(?:\.\d+)?)\s*,\s*FSR\s*=\s*(\d+)\s*,\s*SEAT\s*=\s*(\d+)'
)

class Stm32SensorBridge(Node):
    def __init__(self):
        super().__init__('stm32_sensor_bridge')

        # 파라미터
        self.declare_parameter('port', '/dev/ttyACM0')
        self.declare_parameter('baud', 115200)
        self.declare_parameter('distance_topic', '/ultra_distance_cm')
        self.declare_parameter('seat_topic', '/seat_detected')

        self.port = self.get_parameter('port').value
        self.baud = int(self.get_parameter('baud').value)
        self.distance_topic = self.get_parameter('distance_topic').value
        self.seat_topic = self.get_parameter('seat_topic').value

        # 퍼블리셔
        self.pub_dist = self.create_publisher(Float32, self.distance_topic, 10)
        self.pub_seat = self.create_publisher(Bool, self.seat_topic, 10)

        # 시리얼
        self.ser = None
        self.rx_buf = bytearray()
        self._open_serial()

        # 50Hz 폴링(STM32가 60ms마다 쏘므로 충분)
        self.timer = self.create_timer(0.02, self._tick)

    def _open_serial(self):
        try:
            # timeout=0 : 논블로킹
            self.ser = serial.Serial(self.port, baudrate=self.baud, timeout=0)
            self.rx_buf.clear()
            self.get_logger().info(f"Serial opened: {self.port} @ {self.baud}")
        except Exception as e:
            self.ser = None
            self.get_logger().error(f"Serial open failed: {e}")

    def _process_line(self, line: str):
        line = line.strip()
        if not line:
            return

        m = LINE_RE.search(line)
        if not m:
            return

        dist = float(m.group(1))
        seat = int(m.group(3))
        seat_bool = (seat != 0)

        # ✅ 값이 같아도 "무조건 publish" (echo/hz 테스트 및 안정성)
        self.pub_dist.publish(Float32(data=dist))
        self.pub_seat.publish(Bool(data=seat_bool))

    def _tick(self):
        if self.ser is None:
            self._open_serial()
            return

        try:
            n = self.ser.in_waiting
            if n <= 0:
                return

            data = self.ser.read(n)
            if not data:
                return

            self.rx_buf.extend(data)

            # '\n' 단위로 프레임 분리 (STM32에서 \r\n 보내므로 OK)
            while b'\n' in self.rx_buf:
                line_bytes, _, rest = self.rx_buf.partition(b'\n')
                self.rx_buf = bytearray(rest)

                line = line_bytes.decode(errors='ignore')
                self._process_line(line)

        except (OSError, serial.SerialException) as e:
            self.get_logger().warn(f"Serial error: {e} (reopen)")
            try:
                self.ser.close()
            except Exception:
                pass
            self.ser = None

def main():
    rclpy.init()
    node = Stm32SensorBridge()
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

