-- 작업 공간
cd ~/ros_ws/src

-- tcp_bridge 패키지 생성
ros2 pkg create wc_server_bridge --build-type ament_python --dependencies rclpy

-- 노드코드
vi wc_server_bridge/wc_server_bridge/tcp_bridge.py

-- setup.py에 entry_points 등록
entry_points={
    'console_scripts': [
        'tcp_bridge = wc_server_bridge.tcp_bridge:main',
    ],
},

-- bringup launch 생성
cd ~/ros_ws/src
ros2 pkg create --build-type ament_python wc_bringup


-- 빌드/실행
cd ~/ros_ws
colcon build
source install/setup.bash
ros2 launch wc_bringup bringup.launch.py


//251223 임정민

병원맵 런치 파일
map_gazebo.launch (맵이랑 봇이랑 따로)
cd ~/project/Project-Hospital-Autonomous-Wheelchair/Hospital_WheelChair/hospital_ws
colcon build --symlink-install

터미널1
source /opt/ros/humble/setup.bash
source ~/project/Project-Hospital-Autonomous-Wheelchair/Hospital_WheelChair/hospital_ws/install/setup.bash

export GAZEBO_MODEL_PATH=$GAZEBO_MODEL_PATH:$(ros2 pkg prefix hospital_description)/share
echo $GAZEBO_MODEL_PATH

ros2 launch gazebo_ros gazebo.launch.py

터미널2

source /opt/ros/humble/setup.bash
source ~/project/Project-Hospital-Autonomous-Wheelchair/Hospital_WheelChair/hospital_ws/install/setup.bash

ros2 run gazebo_ros spawn_entity.py \
  -entity hospital_map \
  -file $(ros2 pkg prefix hospital_description)/share/hospital_description/urdf/map_hospital.urdf \
  -x 0 -y 0 -z 0.2

터미널 3
source /opt/ros/humble/setup.bash
source ~/project/Project-Hospital-Autonomous-Wheelchair/Hospital_WheelChair/hospital_ws/install/setup.bash

ros2 run gazebo_ros spawn_entity.py \
  -entity hospital_bot \
  -file $(ros2 pkg prefix hospital_description)/share/hospital_description/urdf/turtlebot3_burger_Hospital_fixed.urdf \
  -x 0.5 -y -1 -z 0.2

++++
터미널 한개로 (hospital_gazebo_spawn.launch.py 로 맵+봇 한번에)
source /opt/ros/humble/setup.bash
source ~/project/Project-Hospital-Autonomous-Wheelchair/Hospital_WheelChair/hospital_ws/install/setup.bash

ros2 launch hospital_description hospital_gazebo_spawn.launch.py

