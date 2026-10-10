import rclpy
from rclpy.node import Node

from interface.msg import Robot
from interface.srv import FoodReady

import time

class RobotNode(Node):

    def __init__(self):
        super().__init__('robot_node')
        self.publisher_ = self.create_publisher(
                                Robot,
                                'robot_state',
                                10)
        self.timer_period = 0.20 # second
        self.start_time_ = time.time() # second
        
        self.timer = self.create_timer(
                        self.timer_period,
                        self.timer_callback)
        
        self.robot_state_ = "free"
        self.food_recieve_service_ = self.create_service(
            FoodReady,
            "food_ready",
            self.food_recieve_callback
        )

    def timer_callback(self):
        msg = Robot()
        self.robot_state_ = self.handle_robot_state()
        msg.robot_state = self.robot_state_
        self.publisher_.publish(msg)
    
    def handle_robot_state(self) -> str:
        elapsed_time = time.time() - self.start_time_ 
        
        state = "free"
        if(elapsed_time > 2 and elapsed_time < 4):
            state = "busy"
        
        # reset timer
        if(elapsed_time > 5):
            self.start_time_ = time.time()
        
        return state

    def food_recieve_callback(self, request, response):
        self.get_logger().info(f"order {request.name} was delivered !")
        return response

def main(args=None):
    rclpy.init(args=args)

    robot_node = RobotNode()

    rclpy.spin(robot_node)

    robot_node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()