from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, SetEnvironmentVariable, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    pkg_gazebo_ros = get_package_share_directory('gazebo_ros')
    pkg_hospital = get_package_share_directory('hospital_description')

    gazebo_launch = os.path.join(pkg_gazebo_ros, 'launch', 'gazebo.launch.py')

    # ✅ world는 패키지 기준
    world_path = os.path.join(pkg_hospital, 'worlds', 'hospital_stl.world')

    # ✅ URDF
    bot_urdf = os.path.join(pkg_hospital, 'urdf', 'turtlebot3_burger_Hospital_fixed.urdf')

    # ✅ 모델 경로들
    model_path = os.path.dirname(pkg_hospital)  # .../share
    models_path = "/home/ubuntu/project/Project-Hospital-Autonomous-Wheelchair/Hospital_WheelChair/hospital_ws/src/hospital_description/models"

    set_env = [
        SetEnvironmentVariable(name='GAZEBO_MODEL_DATABASE_URI', value=''),
        SetEnvironmentVariable(
            name='GAZEBO_MODEL_PATH',
            value=f"/usr/share/gazebo-11/models:{models_path}:{model_path}:{os.environ.get('GAZEBO_MODEL_PATH','')}"
        ),
        SetEnvironmentVariable(name='LIBGL_ALWAYS_SOFTWARE', value='1'),
    ]

    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(gazebo_launch),
        launch_arguments={'world': world_path}.items()
    )

    # ✅ RSP로 URDF 올리고
    rsp = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='screen',
        parameters=[{'use_sim_time': True, 'robot_description': open(bot_urdf).read()}]
    )

    static_tf = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='base_footprint_to_base_link',
        arguments=['0', '0', '0', '0', '0', '0', 'base_footprint', 'base_link'],
        output='screen'
    )

    # Gazebo 뜬 뒤 topic으로 스폰
    spawn = TimerAction(
        period=3.0,
        actions=[
            Node(
                package='gazebo_ros',
                executable='spawn_entity.py',
                output='screen',
                arguments=['-entity', 'hospital_bot', '-topic', 'robot_description',
                           '-x', '0', '-y', '0', '-z', '0.2']
            )
        ]
    )

    basefootprint_tf = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='base_footprint_to_base_link',
        arguments=['0','0','0','0','0','0','base_footprint','base_link'],
        output='screen'
    )

    return LaunchDescription([
        *set_env,
        gazebo,
        rsp,
        basefootprint_tf,
        spawn
    ])
