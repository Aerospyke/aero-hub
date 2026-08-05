#include "ah_ros_bridge.h"

#include "ah_common/string_util.hpp"
#include "ah_ros_names.h"

#include <iostream>
#include <utility>

#include <rclcpp/init_options.hpp>

AhRosBridge::AhRosBridge(std::uint8_t ros_domain_id, std::string ros_namespace, Hooks hooks)
    : ros_domain_id_(SanitizeDomainId(ros_domain_id)),
      ros_namespace_(ah::SanitizeNamespace(ros_namespace)),
      hooks_(std::move(hooks)) {
  if (!rclcpp::ok()) {
    rclcpp::InitOptions init_options;
    init_options.set_domain_id(static_cast<size_t>(ros_domain_id_));
    rclcpp::init(0, nullptr, init_options);
  }

  // Namespace applied here; relative topic names pick it up (not absolute /ah/...).
  node_ = std::make_shared<rclcpp::Node>("ah_dashboard", ros_namespace_);

  SetupSubscriptions();

  executor_ = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
  executor_->add_node(node_);

  spin_thread_ = std::thread([this]() {
    RCLCPP_INFO(node_->get_logger(),
                "ah_dashboard online — domain=%u namespace=\"%s\" (fqn=%s)",
                static_cast<unsigned>(ros_domain_id_), ros_namespace_.c_str(),
                node_->get_fully_qualified_name());
    executor_->spin();
    RCLCPP_INFO(node_->get_logger(), "ah_dashboard executor stopped");

    if (hooks_.on_executor_stopped) {
      hooks_.on_executor_stopped();
    }
  });
}

void AhRosBridge::SetupSubscriptions() {
  rclcpp::QoS status_qos(rclcpp::KeepLast(1));
  status_qos.reliable();

  status_sub_ = node_->create_subscription<std_msgs::msg::String>(
      ah_ros_names::StatusTopic, status_qos, [this](const std_msgs::msg::String::SharedPtr msg) {
        if (hooks_.on_status_json && msg) {
          hooks_.on_status_json(msg->data);
        }
      });

  rclcpp::QoS video_qos(rclcpp::KeepLast(1));
  video_qos.best_effort();

  video_sub_ = node_->create_subscription<sensor_msgs::msg::CompressedImage>(
      ah_ros_names::VideoCompressedTopic, video_qos,
      [this](const sensor_msgs::msg::CompressedImage::SharedPtr msg) {
        if (!hooks_.on_video_jpeg || !msg || msg->data.empty()) {
          return;
        }
        hooks_.on_video_jpeg(msg->data);
      });

  // YOLO detections (Task_34): best-effort latest JSON from ah_yolo.
  detections_sub_ = node_->create_subscription<std_msgs::msg::String>(
      ah_ros_names::DetectionsTopic, video_qos,
      [this](const std_msgs::msg::String::SharedPtr msg) {
        if (hooks_.on_detections_json && msg) {
          hooks_.on_detections_json(msg->data);
        }
      });

  RCLCPP_INFO(node_->get_logger(),
              "subscribed to %s + %s + %s (relative; under node namespace)",
              ah_ros_names::StatusTopic, ah_ros_names::VideoCompressedTopic,
              ah_ros_names::DetectionsTopic);
}

AhRosBridge::~AhRosBridge() {
  if (executor_) {
    executor_->cancel();
  }
  if (spin_thread_.joinable()) {
    spin_thread_.join();
  }
  if (executor_ && node_) {
    executor_->remove_node(node_);
  }
  status_sub_.reset();
  video_sub_.reset();
  detections_sub_.reset();
  node_.reset();
  executor_.reset();

  if (rclcpp::ok()) {
    rclcpp::shutdown();
  }
}

std::uint8_t AhRosBridge::SanitizeDomainId(std::uint8_t ros_domain_id) {
  if (ros_domain_id > MaxRosDomainId) {
    std::cerr << "AhRosBridge: invalid ROS domain id " << static_cast<int>(ros_domain_id)
              << " (valid range 0–" << static_cast<int>(MaxRosDomainId) << "); using default "
              << static_cast<int>(DefaultRosDomainId) << '\n';
    return DefaultRosDomainId;
  }
  return ros_domain_id;
}


