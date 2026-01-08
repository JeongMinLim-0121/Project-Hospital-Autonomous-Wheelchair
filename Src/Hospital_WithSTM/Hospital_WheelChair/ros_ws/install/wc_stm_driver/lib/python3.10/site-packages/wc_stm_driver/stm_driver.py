import rclpy
from rclpy.node import Node
import serial
import threading
import time

# 사용할 메시지 타입들
from std_msgs.msg import Float32, Bool, String

class StmDriver(Node):
    def __init__(self):
        super().__init__('stm_driver')
        
        # 1. 파라미터 설정
        self.declare_parameter('port', '/dev/ttyACM0')
        self.declare_parameter('baudrate', 115200)
        
        self.port = self.get_parameter('port').value
        self.baud = self.get_parameter('baudrate').value

        # 2. 퍼블리셔 생성 (각 센서별로 토픽 분리)
        # (1) 압력 센서 (실수형 데이터)
        self.pressure_pub = self.create_publisher(Float32, 'stm/pressure', 10)
        
        # (2) 비상정지 버튼 (참/거짓 데이터)
        self.estop_pub = self.create_publisher(Bool, 'stm/estop', 10)
        
        # (3) 초음파 센서 (실수형 데이터)
        self.sonar_pub = self.create_publisher(Float32, 'stm/ultrasonic', 10)

        # 디버깅용 (원본 데이터 확인)
        self.raw_pub = self.create_publisher(String, 'stm/raw_data', 10)

        # 3. 시리얼 연결
        self.ser = None
        self.connect_serial()

        # 4. 수신 스레드 시작
        self.thread = threading.Thread(target=self.read_serial_loop)
        self.thread.daemon = True
        self.thread.start()

    def connect_serial(self):
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=1)
            self.get_logger().info(f"✅ STM32 Connected: {self.port} ({self.baud})")
        except Exception as e:
            self.get_logger().error(f"❌ Connection Failed: {e}")

    def read_serial_loop(self):
        while rclpy.ok():
            if self.ser and self.ser.is_open:
                try:
                    if self.ser.in_waiting > 0:
                        # 데이터 읽기 (utf-8 디코딩, 양옆 공백 제거)
                        line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                        if line:
                            self.parse_data(line)
                except Exception as e:
                    self.get_logger().warn(f"Serial Read Error: {e}")
                    time.sleep(1)
            else:
                time.sleep(1)

    def parse_data(self, data):
        """
        프로토콜: "DAT,압력,Estop,초음파"
        예시: "DAT,500,0,120"
        """
        # 1. 원본 데이터 발행 (디버깅용)
        raw_msg = String()
        raw_msg.data = data
        self.raw_pub.publish(raw_msg)

        try:
            parts = data.split(',')
            
            # 헤더가 'DAT'이고 데이터 개수가 4개(헤더포함)인지 확인
            if parts[0] == "DAT" and len(parts) >= 4:
                
                # --- [1] 압력 센서 ---
                press_val = float(parts[1])
                msg_p = Float32()
                msg_p.data = press_val
                self.pressure_pub.publish(msg_p)

                # --- [2] E-Stop 버튼 ---
                # 0 또는 1로 온다고 가정 (1이 눌림/비상 이라고 가정)
                estop_val = int(parts[2])
                msg_e = Bool()
                msg_e.data = bool(estop_val) # 1이면 True, 0이면 False
                self.estop_pub.publish(msg_e)

                # 비상정지 눌렸으면 로그 띄우기
                if msg_e.data:
                    self.get_logger().warn("🚨 EMERGENCY STOP BUTTON PRESSED!")

                # --- [3] 초음파 센서 ---
                sonar_val = float(parts[3])
                msg_s = Float32()
                msg_s.data = sonar_val
                self.sonar_pub.publish(msg_s)

        except ValueError:
            pass # 숫자가 아닌 값이 오면 무시
        except Exception as e:
            self.get_logger().warn(f"Parsing Error: {e}")

def main(args=None):
    rclpy.init(args=args)
    node = StmDriver()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()