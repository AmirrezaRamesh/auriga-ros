from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():

    publisher = Node(
        package='collatz_conjecture',
        executable='odd_node',
        name='odd_node',
        output='screen'
    )

    subscriber = Node(
        package='collatz_conjecture',
        executable='even_node',
        name='even_node',
        output='screen'
    )

    return LaunchDescription([
       publisher,
       subscriber
    ])