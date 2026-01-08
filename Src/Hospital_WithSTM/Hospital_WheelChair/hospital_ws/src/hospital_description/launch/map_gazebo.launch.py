from launch import LaunchDescription
from launch.actions import ExecuteProcess
from launch_ros.actions import Node
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    map_urdf = PathJoinSubstitution([
        FindPackageShare('hospital_description'),
        'urdf',
        'map_hospital.urdf'
    ])

    gazebo = ExecuteProcess(
        cmd=['gazebo', '--verbose',
             '/opt/ros/humble/share/gazebo_ros/worlds/empty.world',
             '-s', 'libgazebo_ros_factory.so'],
        output='screen'
    )

    spawn_map = Node(
        package='gazebo_ros',
        executable='spawn_entity.py',
        arguments=['-entity', 'hospital_map',
                   '-file', map_urdf,
                   '-x', '0', '-y', '0', '-z', '0'],
        output='screen'
    )

    return LaunchDescription([gazebo, spawn_map])
