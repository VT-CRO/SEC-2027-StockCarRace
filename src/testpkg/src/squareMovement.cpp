#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>

#include <rclcpp/node.hpp>
#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>


using namespace geometry_msgs::msg;
using namespace std;
using namespace rclcpp;

class SquareMovement : public Node
{
private:
    Publisher<Twist>::SharedPtr publisher_;
    TimerBase::SharedPtr timer;

    const vector<pair<double, double>> movements = {
        {0.5, 0.0},
        {0, 0.5},
        {-0.5, 0.0},
        {0.0, -0.5}
    };

    unsigned int current_step = 0;


public:
  SquareMovement()
  : Node("square_movement")
  {
    publisher_ = this->create_publisher<Twist>("cmd_vel", 1);
    timer = this->create_wall_timer(
      chrono::milliseconds(1000),
      [this](){ step(); }
    );
  }

  void step(){
    Twist twist;
    twist.linear.x = movements[current_step].first;
    twist.angular.z = movements[current_step].second;
    publisher_->publish(twist);
    current_step++;
    if (current_step >= movements.size()) current_step = 0;
    cout << "Current step: " << current_step << endl;
  }
  
};


int main(int argc, char * argv[])
{
  init(argc, argv);
  spin(make_shared<SquareMovement>());
  shutdown();
  return 0;
}
