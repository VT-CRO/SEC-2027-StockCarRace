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


class MyTest : public Node
{
private:
    Twist currentSimTwist;
    Subscription<Twist>::SharedPtr subscription;
    

    void update(Twist newTwist){
        currentSimTwist = newTwist;
        cout << currentSimTwist.linear.x << " " << currentSimTwist.linear.y << " " << currentSimTwist.linear.z << endl;
    }

public:
  MyTest()
  : Node("my_test")
  {
    subscription = this->create_subscription<Twist>(
      "cmd_vel", 1, 
      [this](Twist::ConstSharedPtr msg){
        update(*msg);
      }
    );
    
  }

};


int main(int argc, char * argv[])
{
  init(argc, argv);
  spin(std::make_shared<MyTest>());
  shutdown();
  return 0;
}
