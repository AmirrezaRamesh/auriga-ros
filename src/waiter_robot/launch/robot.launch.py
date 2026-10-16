from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():

    return LaunchDescription([
        Node(package='waiter_robot', executable='customer_node' , name='customer_node' , output='screen'),
        Node(package='waiter_robot' , executable='robot_node' , name='robot_node' , output='screen'),
        Node(package='waiter_robot' , executable='kitchen_node' , name='kitchen_node' , output='screen')
    ])