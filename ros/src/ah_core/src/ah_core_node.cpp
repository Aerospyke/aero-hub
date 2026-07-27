// ah_core_node — Milestone_1 stub
// Publishes /ah/system/status as JSON in std_msgs/String (see ros2-interface-map §3.1).

#include <chrono>
#include <cmath>
#include <memory>
#include <sstream>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

namespace
{
std::string make_status_json(double stamp_sec)
{
  // Fixed stub flags; stamp advances so subscribers can see liveliness.
  std::ostringstream oss;
  oss.setf(std::ios::fixed);
  oss.precision(3);
  oss << '{'
      << "\"smart_mode_active\":false,"
      << "\"tracking_started\":false,"
      << "\"segmentation_active\":false,"
      << "\"following_active\":false,"
      << "\"video_status\":\"unavailable\","
      << "\"tracker_type\":\"stub\","
      << "\"follower_mode\":\"none\","
      << "\"stamp\":" << stamp_sec
      << '}';
  return oss.str();
}
}  // namespace

class AhCoreNode : public rclcpp::Node
{
public:
  AhCoreNode()
  : Node("ah_core")
  {
    // Reliable, small queue — matches interface intent for status.
    rclcpp::QoS qos(rclcpp::KeepLast(1));
    qos.reliable();

    status_pub_ = create_publisher<std_msgs::msg::String>("/ah/system/status", qos);

    // 5 Hz — within 5–10 Hz guidance in interface map.
    timer_ = create_wall_timer(200ms, std::bind(&AhCoreNode::on_timer, this));

    RCLCPP_INFO(get_logger(), "ah_core stub started; publishing /ah/system/status at 5 Hz");
  }

private:
  void on_timer()
  {
    const auto now = get_clock()->now();
    std_msgs::msg::String msg;
    msg.data = make_status_json(now.seconds());
    status_pub_->publish(msg);
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<AhCoreNode>());
  rclcpp::shutdown();
  return 0;
}
