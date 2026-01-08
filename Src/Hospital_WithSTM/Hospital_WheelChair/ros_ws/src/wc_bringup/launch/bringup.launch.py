from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, GroupAction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node, PushRosNamespace

def generate_launch_description():
    # 1. 런치 설정 변수 가져오기
    namespace       = LaunchConfiguration('namespace')
    server_ip       = LaunchConfiguration('server_ip')
    server_port     = LaunchConfiguration('server_port')
    robot_name      = LaunchConfiguration('robot_name')
    tx_hz           = LaunchConfiguration('tx_hz')
    use_amcl        = LaunchConfiguration('use_amcl_pose')
    use_sim_time    = LaunchConfiguration('use_sim_time')

    return LaunchDescription([
        # 2. 인자(Argument) 선언
        DeclareLaunchArgument('namespace', default_value='wc1'),
        DeclareLaunchArgument('server_ip', default_value='10.10.14.138'),
        DeclareLaunchArgument('server_port', default_value='8080'),
        DeclareLaunchArgument('robot_name', default_value='wc1'),
        DeclareLaunchArgument('tx_hz', default_value='2.0'),
        DeclareLaunchArgument('use_amcl_pose', default_value='true'),
        # 기본값 false, 시뮬레이션 시 true로 변경 가능
        DeclareLaunchArgument('use_sim_time', default_value='false'),

        # 3. 노드 실행 그룹 (namespace=/wc1 적용)
        GroupAction([
            # 이 그룹 안의 모든 노드는 /wc1 네임스페이스 안에서 실행됨
            PushRosNamespace(namespace),

            # (A) TCP Bridge 노드
            Node(
                package='wc_server_bridge',
                executable='tcp_bridge',   # tcp_bridge 패키지의 console_scripts 이름
                name='tcp_bridge',
                output='screen',
                parameters=[{
                    'use_sim_time': use_sim_time, # [추가]
                    'server_ip': server_ip,
                    'server_port': server_port,
                    'robot_name': robot_name,
                    'tx_hz': tx_hz,
                    'use_amcl_pose': use_amcl,
                }],
                # TF 네임스페이스 꼬임 방지
                remappings=[
                    ('/tf', 'tf'),
                    ('/tf_static', 'tf_static'),
                ],
            ),

            # (B) Mission Manager 노드
            Node(
                package='wc_mission_manager',
                executable='mission_manager',
                name='mission_manager',
                output='screen',
                parameters=[{
                    'use_sim_time': use_sim_time,
                    # [핵심] Nav2 액션 서버는 네임스페이스 밖(/)에 있으므로 절대 경로 사용
                    'action_name': '/navigate_to_pose',
                    # 토픽들은 네임스페이스 안(/wc1/goal)을 사용하므로 상대 경로 유지
                    'goal_topic': 'goal',
                    'status_topic': 'mission_status',
                }]
            ),
            # # (C) STM32 UART 노드
            # Node(
            #     package='wc_stm_driver',
            #     executable='stm_driver',
            #     name='stm_driver',
            #     output='screen',
            #     parameters=[{
            #         'port': '/dev/ttyUSB1', 
            #         'baudrate': 115200
            #     }]
            # ),
        ]),
    ])