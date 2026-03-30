// Copyright 2026, Evan Palmer
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "nucleus_driver/nucleus_driver.hpp"

#include <cmath>

#include "libnucleus/report.hpp"
#include "rclcpp/rclcpp.hpp"

namespace nucleus::ros
{

NucleusDriver::NucleusDriver(const rclcpp::NodeOptions & options)
: rclcpp_lifecycle::LifecycleNode("nucleus_driver", options)
{
}

auto NucleusDriver::on_configure(const rclcpp_lifecycle::State & /*previous_state*/) -> CallbackReturn
{
  RCLCPP_INFO(get_logger(), "Configuring the NucleusDriver");

  try {
    param_listener_ = std::make_shared<nucleus_driver::ParamListener>(get_node_parameters_interface());
    params_ = param_listener_->get_params();
  }
  catch (const std::exception & e) {
    RCLCPP_ERROR(get_logger(), "Failed to get NucleusDriver parameters: %s", e.what());
    return CallbackReturn::ERROR;
  }

  try {
    client_ =
      std::make_unique<NucleusClient>(params_.ip_address, params_.password, std::chrono::seconds(params_.timeout));
  }
  catch (const std::exception & e) {
    RCLCPP_ERROR(get_logger(), "Failed to create NucleusClient. %s", e.what());
    return CallbackReturn::ERROR;
  }

  // Pre-populate the sensor state messages with known, static values
  dvl_msg_.header.frame_id = params_.child_frame_id;
  twist_msg_.header.frame_id = params_.child_frame_id;
  odom_msg_.header.frame_id = params_.frame_id;
  odom_msg_.child_frame_id = params_.child_frame_id;

  // the INS messages don't include the covariances, so we just leave them as a configurable
  // parameter that users can set.
  odom_msg_.pose.covariance[0] = params_.ins_covariance[0];
  odom_msg_.pose.covariance[7] = params_.ins_covariance[1];
  odom_msg_.pose.covariance[14] = params_.ins_covariance[2];
  odom_msg_.pose.covariance[21] = params_.ins_covariance[3];
  odom_msg_.pose.covariance[28] = params_.ins_covariance[4];
  odom_msg_.pose.covariance[35] = params_.ins_covariance[5];

  odom_msg_.twist.covariance[0] = params_.ins_covariance[6];
  odom_msg_.twist.covariance[7] = params_.ins_covariance[7];
  odom_msg_.twist.covariance[14] = params_.ins_covariance[8];
  odom_msg_.twist.covariance[21] = params_.ins_covariance[9];
  odom_msg_.twist.covariance[28] = params_.ins_covariance[10];
  odom_msg_.twist.covariance[35] = params_.ins_covariance[11];

  dvl_msg_.velocity_mode = marine_acoustic_msgs::msg::Dvl::DVL_MODE_BOTTOM;
  dvl_msg_.dvl_type = marine_acoustic_msgs::msg::Dvl::DVL_TYPE_PISTON;  // 3-beam convex Janus array

  // Each beam points 20° away from the center
  // Transducers are rotated 120° around Z.
  // Beam 1 (0° from X)
  dvl_msg_.beam_unit_vec[0].x = 0.9396926207859084;
  dvl_msg_.beam_unit_vec[0].y = 0.0;
  dvl_msg_.beam_unit_vec[0].z = 0.3420201433256687;

  // Beam 2 (+120° from X)
  dvl_msg_.beam_unit_vec[1].x = -0.469846310392954;
  dvl_msg_.beam_unit_vec[1].y = 0.8137976813493738;
  dvl_msg_.beam_unit_vec[1].z = 0.3420201433256687;

  // Beam 3 (-120° from X)
  dvl_msg_.beam_unit_vec[2].x = -0.469846310392954;
  dvl_msg_.beam_unit_vec[2].y = -0.8137976813493738;
  dvl_msg_.beam_unit_vec[2].z = 0.3420201433256687;

  // Beam 4 (altimeter; points straight down)
  dvl_msg_.beam_unit_vec[3].x = 0.0;
  dvl_msg_.beam_unit_vec[3].y = 0.0;
  dvl_msg_.beam_unit_vec[3].z = 1.0;

  dvl_pub_ = create_publisher<marine_acoustic_msgs::msg::Dvl>("~/raw", rclcpp::SystemDefaultsQoS());
  twist_pub_ = create_publisher<geometry_msgs::msg::TwistWithCovarianceStamped>("~/twist", rclcpp::SystemDefaultsQoS());
  odom_pub_ = create_publisher<nav_msgs::msg::Odometry>("~/odom", rclcpp::SystemDefaultsQoS());

  client_->subscribe<BottomTrackReport>([this](const BottomTrackReport & report) -> void {
    twist_msg_.header.stamp = rclcpp::Time(report.timestamp.count());

    dvl_msg_.velocity.x = report.vx;
    dvl_msg_.velocity.y = report.vy;
    dvl_msg_.velocity.z = report.vz;
    dvl_msg_.beam_ranges_valid = true;
    dvl_msg_.beam_velocities_valid = report.velocity_valid;
    dvl_msg_.course_gnd = std::atan2(report.vy, report.vx);
    dvl_msg_.speed_gnd = std::sqrt(report.vx * report.vx + report.vy * report.vy);

    for (std::size_t i = 0; i < 3; ++i) {
      for (std::size_t j = 0; j < 3; ++j) {
        dvl_msg_.velocity_covar[i * 3 + j] = report.covariance(i, j);
      }
    }

    dvl_msg_.num_good_beams = 0;
    for (std::size_t i = 0; i < report.transducers.size(); ++i) {
      dvl_msg_.beam_quality[i] = report.transducers[i].std;
      dvl_msg_.beam_velocity[i] = report.transducers[i].velocity;
      dvl_msg_.range[i] = report.transducers[i].distance;
      dvl_msg_.num_good_beams += report.transducers[i].velocity_valid ? 1 : 0;
    }

    dvl_pub_->publish(dvl_msg_);
  });

  // much of the following code could be moved into the above callback, but we separate it to improve readability
  client_->subscribe<BottomTrackReport>([this](const BottomTrackReport & report) -> void {
    twist_msg_.header.stamp = rclcpp::Time(report.timestamp.count());

    twist_msg_.twist.twist.linear.x = report.vx;
    twist_msg_.twist.twist.linear.y = report.vy;
    twist_msg_.twist.twist.linear.z = report.vz;

    for (std::size_t i = 0; i < 3; ++i) {
      for (std::size_t j = 0; j < 3; ++j) {
        twist_msg_.twist.covariance[i * 6 + j] = report.covariance(i, j);
      }
    }

    twist_pub_->publish(twist_msg_);
  });

  // the Nucleus packs the altimeter data into a separate report - we just need it for altitude readings
  client_->subscribe<AltimeterReport>(
    [this](const AltimeterReport & report) -> void { dvl_msg_.altitude = report.distance; });

  client_->subscribe<INSReport>([this](const INSReport & report) -> void {
    odom_msg_.header.stamp = rclcpp::Time(report.timestamp.count());
    odom_msg_.pose.pose.position.x = report.x;
    odom_msg_.pose.pose.position.y = report.y;
    odom_msg_.pose.pose.position.z = report.z;

    odom_msg_.pose.pose.orientation.x = report.orientation.x();
    odom_msg_.pose.pose.orientation.y = report.orientation.y();
    odom_msg_.pose.pose.orientation.z = report.orientation.z();
    odom_msg_.pose.pose.orientation.w = report.orientation.w();

    // We don't have velocity data in the INS report, so we'll leave that part of the message empty for state estimators to fill in
    odom_msg_.twist.twist.linear.x = report.vx;
    odom_msg_.twist.twist.linear.y = report.vy;
    odom_msg_.twist.twist.linear.z = report.vz;
    odom_msg_.twist.twist.angular.x = report.wx;
    odom_msg_.twist.twist.angular.y = report.wy;
    odom_msg_.twist.twist.angular.z = report.wz;

    odom_pub_->publish(odom_msg_);
  });

  RCLCPP_INFO(get_logger(), "NucleusDriver loaded successfully");

  return CallbackReturn::SUCCESS;
}

auto NucleusDriver::on_activate(const rclcpp_lifecycle::State & /*previous_state*/) -> CallbackReturn
{
  std::future<Response> f = client_->start_measurement();
  std::future_status status = f.wait_for(std::chrono::seconds(1));

  switch (status = f.wait_for(std::chrono::seconds(1))) {
    case std::future_status::ready:
      break;
    case std::future_status::timeout:
      RCLCPP_WARN(get_logger(), "Start measurement attempt timed out: the Nucleus may already be streaming");
      break;
    default:
      RCLCPP_ERROR(get_logger(), "Failed to start measurement: an unexpected error occurred");
      return CallbackReturn::ERROR;
  }

  RCLCPP_INFO(get_logger(), "NucleusDriver was successfully activated");
  return CallbackReturn::SUCCESS;
}

}  // namespace nucleus::ros

auto main(int argc, char * argv[]) -> int
{
  rclcpp::init(argc, argv);
  rclcpp::executors::MultiThreadedExecutor executor;
  auto node = std::make_shared<nucleus::ros::NucleusDriver>();
  executor.add_node(node->get_node_base_interface());
  executor.spin();
  rclcpp::shutdown();
  return 0;
}
