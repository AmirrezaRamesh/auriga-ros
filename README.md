# Waiter Robot

A small restaurant simulation we built with ROS 2 Humble and C++ on Ubuntu. A waiter robot goes around 3 tables, takes orders, checks with the kitchen, and brings the food out when it's ready.

## Project structure

We ended up with 3 nodes:

| Node | What it does |
| --- | --- |
| `customer_node` | Makes up the orders and decides when the robot starts and stops taking one. |
| `robot_node` | Receives orders, asks the kitchen if the food is available, and delivers it once it's cooked. |
| `kitchen_node` | Keeps track of stock, cooks the orders, and tells the robot when something is ready. |

The code is split into two ROS 2 packages. `waiter_robot` has the nodes and the launch file, and `waiter_robot_interfaces` has our custom messages and services.

## How it works

1. While the robot is FREE, the customer waits 2 seconds.
2. The customer sends `START`. Once the robot answers with an ACK, it's BUSY and takes the order for 2 seconds.
3. The customer sends `FINISH`. After that ACK it publishes the order, then waits 1 second before starting the next cycle.
4. The robot asks the kitchen if the food is available. The kitchen either reserves it or offers a different food that's still in stock.
5. The kitchen cooks the accepted order. When it's done and the robot is FREE, the robot picks it up and delivers it.
6. When everything is sold out, the kitchen tells the customer to stop ordering. Orders that were already accepted still get delivered.

These timings aren't exact. If the robot is in the middle of a delivery or waiting on a service reply, a cycle takes longer.

### Topics

| Topic | Direction | Use |
| --- | --- | --- |
| `/customer_order` | Customer → Robot | A new order. |
| `/order` | Robot → Kitchen | An order the kitchen accepted. |
| `/robot_state` | Robot → Kitchen | `true` means FREE, `false` means BUSY or unavailable. |

### Services

| Service | Direction | Use |
| --- | --- | --- |
| `/order_phase` | Customer → Robot | START, FINISH, HEARTBEAT and RESYNC. |
| `/food_available` | Robot → Kitchen | Checks the stock and reserves the food. |
| `/food_ready` | Kitchen → Robot | Asks the robot to pick up finished food. |
| `/close_resturant` | Kitchen → Customer | Closes the restaurant when the food runs out. |

Yes, `/close_resturant` is spelled like that. It's the same typo everywhere in our code, so we left it.

### Menu

| Food | Starting stock | Cooking time |
| --- | --- | --- |
| Pizza | 2 | 15 s |
| Burger | 3 | 5 s |
| Pasta | 3 | 7 s |

## How to run

You need Ubuntu 22.04, ROS 2 Humble and colcon. From the folder where you cloned the repo:

```bash
cd ~/auriga_repo
source /opt/ros/humble/setup.bash
colcon build --packages-up-to waiter_robot
source install/setup.bash
ros2 launch waiter_robot robot.launch.py
```

If you want to look at the nodes, topics and services, open a second terminal and run:

```bash
source /opt/ros/humble/setup.bash
source ~/auriga_repo/install/setup.bash
ros2 node list
ros2 topic list
ros2 service list
rqt_graph
```

## ROS 2 graph

Here's the topic graph for the three nodes:

![AURIGA rqt_graph](docs/rqt_graph.png)

`rqt_graph` only shows topics, so services won't appear in it. Use `ros2 service list` or `ros2 node info /robot_node` for those.

## Problems we ran into

**1. FREE and BUSY kept getting out of sync.**
At first we only used timers, and the customer had no way of knowing when the robot had actually started taking the order. So we added `START` and `FINISH` service calls with ACK replies. The customer's 2-second BUSY timer now only starts after it gets `START ACK`.

**2. Stock was being subtracted twice.**
Both the availability check and the incoming order were touching the stock. We fixed it by making `/food_available` the only place that reserves stock. When an order arrives on `/order` the kitchen just starts cooking and doesn't change the stock.

**3. Cooking blocked everything else in the kitchen.**
Waiting for a pizza to finish froze the rest of the node. We switched to ROS 2 timers instead of blocking, so the kitchen can handle several orders at once.

**4. The robot could get stuck in BUSY if the customer disappeared.**
We added a heartbeat with a timeout. If the customer stops responding, the robot goes to `NOT_READY`. When the customer comes back it sends `RESYNC` before any new orders are taken.

**5. Service responses can get delayed or lost.**
We added timeouts and retries. A retry reuses the same `session_id` and order/cycle IDs, so retrying doesn't create a duplicate order.

**6. ROS 2 didn't recognize our custom message in a new terminal.**
This one was simple. We hadn't sourced the workspace in that terminal. Running `source install/setup.bash` after sourcing ROS 2 fixed it.

## Testing

We tested everything on Ubuntu with ROS 2 Humble.

- `colcon build` finishes without errors.
- The robot receives orders and delivers the food.
- The kitchen closes the restaurant once all the food is reserved.
- `rqt_graph` shows the topic connections we expected.
- We stopped and restarted the customer, and the robot and customer completed `RESYNC` successfully.

## Limitations

This is only a simulation, there's no physical robot. Order and reservation history lives in memory, so restarting the robot or the kitchen loses it. We only tested recovery for restarting the **customer** while the robot and kitchen kept running.
