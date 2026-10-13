#include "ah_ros_bridge.h"

#include "ah_common/string_util.hpp"
#include "ah_ros_names.h"

#include <cmath>
#include <iostream>
#include <utility>

#include <rclcpp/expand_topic_or_service_name.hpp>
#include <rclcpp/init_options.hpp>
#include <rclcpp/qos.hpp>

#include "px4_msgs/msg/sensor_gps.hpp"
#include "px4_msgs/msg/vehicle_odometry.hpp"
#include "px4_msgs/msg/vehicle_status.hpp"

namespace {

AhPx4OdomSample MakeOdomSample(const px4_msgs::msg::VehicleOdometry& msg) {
  AhPx4OdomSample sample;
  sample.north_m = msg.position[0];
  sample.east_m = msg.position[1];
  sample.down_m = msg.position[2];
  sample.position_valid =
      std::isfinite(sample.north_m) && std::isfinite(sample.east_m) && std::isfinite(sample.down_m);

  AhPx4Attitude attitude;
  sample.attitude_valid = AhPx4AttitudeFromQuaternion(msg.q[0], msg.q[1], msg.q[2], msg.q[3], attitude);
  if (sample.attitude_valid) {
    sample.roll_deg = attitude.roll_deg;
    sample.pitch_deg = attitude.pitch_deg;
    sample.heading_deg = attitude.heading_deg;
  }
  return sample;
}

// PX4 publishes sensor streams best-effort. A reliable subscriber matches nothing.
// Keep-last + volatile is the ROS 2 sensor-data profile from the PX4 user guide.
// A volatile subscriber still matches a transient-local publisher.
rclcpp::QoS Px4SensorQos() {
  rclcpp::QoS qos(rclcpp::KeepLast(5));
  qos.best_effort();
  qos.durability_volatile();
  return qos;
}

}  // namespace

AhRosBridge::AhRosBridge(std::uint8_t ros_domain_id, std::string ros_namespace, std::string vehicle_namespace,
                         Hooks hooks)
    : ros_domain_id_(SanitizeDomainId(ros_domain_id)),
      ros_namespace_(ah::SanitizeNamespace(ros_namespace)),
      vehicle_namespace_(ah::SanitizeNamespace(std::move(vehicle_namespace))),
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

  // Separate node so relative fmu/out/... names pick up the vehicle namespace
  // without moving the dashboard's own ah/... graph.
  try {
    px4_node_ = std::make_shared<rclcpp::Node>("ah_dashboard_px4", vehicle_namespace_);
    SetupPx4Subscriptions();
    executor_->add_node(px4_node_);
    px4_node_added_ = true;
  } catch (const std::exception& ex) {
    std::cerr << "AhRosBridge: PX4 node unavailable (" << ex.what() << "); dashboard continues without it\n";
    px4_odom_sub_.reset();
    px4_status_sub_.reset();
    px4_gps_sub_.reset();
    px4_node_.reset();
  }

  spin_thread_ = std::thread([this]() {
    RCLCPP_INFO(node_->get_logger(), "ah_dashboard online — domain=%u namespace=\"%s\" (fqn=%s)",
                static_cast<unsigned>(ros_domain_id_), ros_namespace_.c_str(), node_->get_fully_qualified_name());
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

  status_sub_ = node_->create_subscription<std_msgs::msg::String>(ah_ros_names::StatusTopic, status_qos,
                                                                  [this](const std_msgs::msg::String::SharedPtr msg) {
                                                                    if (hooks_.on_status_json && msg) {
                                                                      hooks_.on_status_json(msg->data);
                                                                    }
                                                                  });

  rclcpp::QoS video_qos(rclcpp::KeepLast(1));
  video_qos.best_effort();

  video_sub_ = node_->create_subscription<sensor_msgs::msg::CompressedImage>(
      ah_ros_names::VideoCompressedTopic, video_qos, [this](const sensor_msgs::msg::CompressedImage::SharedPtr msg) {
        if (!hooks_.on_video_jpeg || !msg || msg->data.empty()) {
          return;
        }
        hooks_.on_video_jpeg(msg->data);
      });

  // YOLO detections (Task_34): best-effort latest JSON from ah_yolo.
  detections_sub_ = node_->create_subscription<std_msgs::msg::String>(
      ah_ros_names::DetectionsTopic, video_qos, [this](const std_msgs::msg::String::SharedPtr msg) {
        if (hooks_.on_detections_json && msg) {
          hooks_.on_detections_json(msg->data);
        }
      });

  RCLCPP_INFO(node_->get_logger(), "subscribed to %s + %s + %s (relative; under node namespace)",
              ah_ros_names::StatusTopic, ah_ros_names::VideoCompressedTopic, ah_ros_names::DetectionsTopic);
}

void AhRosBridge::SetupPx4Subscriptions() {
  if (!px4_node_) {
    return;
  }

  const rclcpp::QoS px4_qos = Px4SensorQos();
  const auto LogResolved = [this](const char* relative_topic) {
    try {
      const std::string resolved = rclcpp::expand_topic_or_service_name(
          relative_topic, px4_node_->get_name(), px4_node_->get_namespace(), false);
      RCLCPP_INFO(px4_node_->get_logger(), "PX4 subscribe %s -> %s (best_effort, volatile, keep_last)",
                  relative_topic, resolved.c_str());
    } catch (const std::exception& ex) {
      RCLCPP_ERROR(px4_node_->get_logger(), "PX4 topic %s did not resolve: %s", relative_topic, ex.what());
    }
  };

  try {
    LogResolved(ah_ros_names::Px4VehicleOdometryTopic);
    px4_odom_sub_ = px4_node_->create_subscription<px4_msgs::msg::VehicleOdometry>(
        ah_ros_names::Px4VehicleOdometryTopic, px4_qos,
        [this](const px4_msgs::msg::VehicleOdometry::SharedPtr msg) {
          if (!msg) {
            return;
          }
          try {
            const AhPx4OdomSample sample = MakeOdomSample(*msg);
            if (!logged_px4_odom_) {
              logged_px4_odom_ = true;
              RCLCPP_INFO(px4_node_->get_logger(),
                          "PX4 odometry live N=%.2f E=%.2f D=%.2f heading=%.1f roll=%.1f pitch=%.1f",
                          sample.north_m, sample.east_m, sample.down_m, sample.heading_deg, sample.roll_deg,
                          sample.pitch_deg);
            }
            if (hooks_.on_px4_odometry) {
              hooks_.on_px4_odometry(sample);
            }
          } catch (const std::exception& ex) {
            RCLCPP_ERROR(px4_node_->get_logger(), "PX4 odometry callback: %s", ex.what());
          } catch (...) {
            RCLCPP_ERROR(px4_node_->get_logger(), "PX4 odometry callback failed");
          }
        });
  } catch (const std::exception& ex) {
    RCLCPP_ERROR(px4_node_->get_logger(), "PX4 odometry subscribe failed: %s", ex.what());
  }

  try {
    LogResolved(ah_ros_names::Px4VehicleStatusTopic);
    px4_status_sub_ = px4_node_->create_subscription<px4_msgs::msg::VehicleStatus>(
        ah_ros_names::Px4VehicleStatusTopic, px4_qos,
        [this](const px4_msgs::msg::VehicleStatus::SharedPtr msg) {
          if (!msg) {
            return;
          }
          try {
            AhPx4StatusSample sample;
            sample.arming_state = msg->arming_state;
            sample.nav_state = msg->nav_state;
            if (!logged_px4_status_) {
              logged_px4_status_ = true;
              RCLCPP_INFO(px4_node_->get_logger(), "PX4 status live arming=%u nav=%u",
                          static_cast<unsigned>(sample.arming_state), static_cast<unsigned>(sample.nav_state));
            }
            if (hooks_.on_px4_status) {
              hooks_.on_px4_status(sample);
            }
          } catch (const std::exception& ex) {
            RCLCPP_ERROR(px4_node_->get_logger(), "PX4 status callback: %s", ex.what());
          } catch (...) {
            RCLCPP_ERROR(px4_node_->get_logger(), "PX4 status callback failed");
          }
        });
  } catch (const std::exception& ex) {
    RCLCPP_ERROR(px4_node_->get_logger(), "PX4 status subscribe failed: %s", ex.what());
  }

  try {
    LogResolved(ah_ros_names::Px4VehicleGpsTopic);
    px4_gps_sub_ = px4_node_->create_subscription<px4_msgs::msg::SensorGps>(
        ah_ros_names::Px4VehicleGpsTopic, px4_qos, [this](const px4_msgs::msg::SensorGps::SharedPtr msg) {
          if (!msg) {
            return;
          }
          try {
            AhPx4GpsSample sample;
            sample.fix_type = msg->fix_type;
            sample.satellites_used = static_cast<int>(msg->satellites_used);
            if (!logged_px4_gps_) {
              logged_px4_gps_ = true;
              RCLCPP_INFO(px4_node_->get_logger(), "PX4 GPS live fix_type=%u satellites=%d",
                          static_cast<unsigned>(sample.fix_type), sample.satellites_used);
            }
            if (hooks_.on_px4_gps) {
              hooks_.on_px4_gps(sample);
            }
          } catch (const std::exception& ex) {
            RCLCPP_ERROR(px4_node_->get_logger(), "PX4 GPS callback: %s", ex.what());
          } catch (...) {
            RCLCPP_ERROR(px4_node_->get_logger(), "PX4 GPS callback failed");
          }
        });
  } catch (const std::exception& ex) {
    RCLCPP_ERROR(px4_node_->get_logger(), "PX4 GPS subscribe failed: %s", ex.what());
  }
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
  if (executor_ && px4_node_added_ && px4_node_) {
    executor_->remove_node(px4_node_);
  }
  status_sub_.reset();
  video_sub_.reset();
  detections_sub_.reset();
  px4_odom_sub_.reset();
  px4_status_sub_.reset();
  px4_gps_sub_.reset();
  node_.reset();
  px4_node_.reset();
  executor_.reset();

  if (rclcpp::ok()) {
    rclcpp::shutdown();
  }
}

std::uint8_t AhRosBridge::SanitizeDomainId(std::uint8_t ros_domain_id) {
  if (ros_domain_id > MaxRosDomainId) {
    std::cerr << "AhRosBridge: invalid ROS domain id " << static_cast<int>(ros_domain_id) << " (valid range 0–"
              << static_cast<int>(MaxRosDomainId) << "); using default " << static_cast<int>(DefaultRosDomainId)
              << '\n';
    return DefaultRosDomainId;
  }
  return ros_domain_id;
}
