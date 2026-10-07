import rclpy
from rclpy.node import Node

from interface.msg import Robot

class RobotNode(Node):

    def __init__(self):
        super().__init__('robot_node')
        self.publisher_ = self.create_publisher(
                                Robot,
                                'robot_state',
                                10)
        timer_period = 0.5 # second
        self.timer = self.create_timer(
                        timer_period,
                        self.timer_callback)
        
        self.robot_state_ = "free"

    def timer_callback(self):
        msg = Robot()
        msg.robot_state = self.robot_state_
        self.publisher_.publish(msg)

def main(args=None):
    rclpy.init(args=args)

    robot_node = RobotNode()

    rclpy.spin(robot_node)

    robot_node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()