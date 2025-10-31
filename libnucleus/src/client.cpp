// Copyright 2025, Evan Palmer
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "libnucleus/client.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/select.h>
#include <unistd.h>

#include <ctime>
#include <iostream>
#include <ranges>
#include <stdexcept>

#include "libnucleus/packet.hpp"
#include "libnucleus/report.hpp"
#include "login.hpp"

namespace nucleus
{

namespace
{

/// Read n_bytes from a socket and append them to a queue.
auto read_from_socket(int socket, std::deque<std::uint8_t> & buffer, std::size_t n_bytes) -> ssize_t
{
  std::vector<std::uint8_t> data(n_bytes);
  const ssize_t n_read = recv(socket, data.data(), n_bytes, 0);

  if (n_read < 0) {
    return n_read;
  }

  std::ranges::copy(data | std::views::take(n_read), std::back_inserter(buffer));
  return n_read;
}

/// Establish a connection to a socket with a timeout. Returns 0 on success, -1 on failure.
auto connect(int socket, const struct sockaddr * addr, socklen_t addrlen, std::chrono::seconds timeout) -> int
{
  auto set_socket_flags = [](int socket, int flags) -> int { return fcntl(socket, F_SETFL, flags); };

  const int flags = fcntl(socket, F_GETFL, 0);
  if (flags < 0) {
    return -1;
  }

  // Set the socket to non-blocking mode so that we can create a timeout on the connection attempt
  if (set_socket_flags(socket, flags | O_NONBLOCK) < 0) {
    return -1;
  }

  auto deadline = std::chrono::steady_clock::now() + timeout;

  // Attempt to establish a connection
  int rc = ::connect(socket, addr, addrlen);

  if (rc < 0) {
    if (errno != EINPROGRESS && errno != EWOULDBLOCK) {
      set_socket_flags(socket, flags);
      return rc;
    }

    do {
      const int remaining_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();

      if (remaining_time <= 0) {
        rc = 0;
        break;
      }

      struct pollfd pfds[] = {{.fd = socket, .events = POLLOUT, .revents = 0}};  // NOLINT
      rc = poll(pfds, 1, remaining_time);

      // Verify that the poll was *actually* successful
      // See: https://stackoverflow.com/questions/2597608/c-socket-connection-timeout
      if (rc > 0) {
        int error = 0;
        socklen_t error_len = sizeof(error);

        if (getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &error_len) < 0) {
          rc = -1;
        } else {
          errno = error;
        }
      }
    } while (rc == -1 && errno == EINTR);

    // A timeout occurred
    if (rc == 0) {
      errno = ETIMEDOUT;
      set_socket_flags(socket, flags);
      return -1;
    }
  }

  // Restore the original socket flags
  return set_socket_flags(socket, flags) < 0 ? -1 : rc;
}

auto open(const std::string & addr, std::uint16_t port, std::chrono::seconds connection_timeout) -> int
{
  // Open a TCP socket and connect to the DVL
  const int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    throw std::runtime_error("Failed to open TCP socket");
  }

  struct sockaddr_in sockaddr;

  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(port);
  sockaddr.sin_addr.s_addr = inet_addr(addr.c_str());

  if (sockaddr.sin_addr.s_addr == INADDR_NONE) {
    throw std::runtime_error("Invalid socket address " + addr);
  }

  if (connect(fd, reinterpret_cast<struct sockaddr *>(&sockaddr), sizeof(sockaddr), connection_timeout) < 0) {
    throw std::runtime_error(
      "An error occurred while attempting to connect to the DVL. Error: " + std::string(strerror(errno)));
  }

  return fd;
}

}  // namespace

NucleusClient::NucleusClient(const std::string & addr, std::chrono::seconds connection_timeout)
{
  // Open a TCP socket and connect to the DVL
  socket_ = open(addr, 9000, connection_timeout);  // connect to the data-only port
  running_.store(true);
  polling_thread_ = std::thread([this] -> void { poll_connection(); });
}

NucleusClient::NucleusClient(
  const std::string & addr,
  const std::string & password,
  std::chrono::seconds connection_timeout)
: command_interface_available_{true}
{
  // Open a TCP socket and connect to the DVL
  socket_ = open(addr, 9002, connection_timeout);  // connect to the primary port

  // Login to the DVL
  if (!protocol::login(socket_, password)) {
    throw std::runtime_error("Failed to login to the DVL with the provided password.");
  }

  running_.store(true);
  polling_thread_ = std::thread([this] -> void { poll_connection(); });
}

NucleusClient::~NucleusClient()
{
  running_.store(false);

  if (polling_thread_.joinable()) {
    polling_thread_.join();
  }

  close(socket_);
}

auto NucleusClient::send_command(const std::string & command, Mode required_mode) -> std::future<bool>
{
  if (mode_ != required_mode) {
    throw std::runtime_error("Cannot send command in the current operating mode.");
  }

  if (send(socket_, (command + "\r\n").c_str(), command.size() + 2, 0) < 0) {
    throw std::runtime_error("Failed to send command to DVL");
  }

  std::promise<bool> response;
  auto future = response.get_future();

  pending_commands_[command].emplace_back(std::move(response));

  return future;
}

auto NucleusClient::start_measurement() -> std::future<bool>
{
  mode_ = Mode::MEASUREMENT;
  return send_command("START", Mode::COMMAND);
}

auto NucleusClient::stop_measurement() -> std::future<bool>
{
  mode_ = Mode::COMMAND;
  return send_command("STOP", Mode::MEASUREMENT);
}

auto NucleusClient::trigger() -> std::future<bool> { return send_command("TRIG", Mode::MEASUREMENT); }

auto NucleusClient::start_field_calibration() -> std::future<bool> { return send_command("FIELDCAL", Mode::COMMAND); }

auto NucleusClient::enable_fast_pressure(int sampling_rate = 10) -> std::future<bool>
{
  const std::string command = std::format("SETFASTPRESSURE,EN=1,SR={}", sampling_rate);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::disable_fast_pressure() -> std::future<bool>
{
  const std::string command = "SETFASTPRESSURE,EN=0";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::save_settings(const std::string & settings = "ALL") -> std::future<bool>
{
  const std::string command = std::format("SAVE,{}", settings);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::revert_to_default_settings(const std::string & settings = "ALL") -> std::future<bool>
{
  const std::string command = std::format("SETDEFAULT,{}", settings);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::restore_settings(const std::string & settings = "ALL") -> std::future<bool>
{
  const std::string command = std::format("RESTORE,{}", settings);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_mission_settings(
  double offset = 9.5,
  double longitude = 9999,
  double latitude = 9999,
  double declination = 0.0,
  double range = 50.0,
  double blanking_distance = 0.1,
  double speed_of_sound = 1481,
  double salinity = 35.0) -> std::future<bool>
{
  const std::string command = std::format(
    "SETMISSION,POFF={:.2f},LONG={:.4f},LAT={:.4f},DECL={:.2f},RANGE={:.2f},BD={:.2f},SV={:.1f},SA={:.2f}",
    offset,
    longitude,
    latitude,
    declination,
    range,
    blanking_distance,
    speed_of_sound,
    salinity);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::enable_led() -> std::future<bool>
{
  const std::string command = "SETINST,LED=\"ON\"";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::disable_led() -> std::future<bool>
{
  const std::string command = "SETINST,LED=\"OFF\"";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_mounting_orientation(double roll, double pitch, double yaw) -> std::future<bool>
{
  const std::string command = std::format("SETINST,ROTXY={:.2f},ROTXZ={:.2f},ROTYZ={:.2f}", roll, pitch, yaw);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_ahrs_output_frequency(int frequency) -> std::future<bool>
{
  const std::string command = std::format("SETAHRS,FREQ={}", frequency);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_ahrs_mode(int mode) -> std::future<bool>
{
  const std::string command = std::format("SETAHRS,MODE={}", mode);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_navigation_data_output_frequency(int frequency) -> std::future<bool>
{
  const std::string command = std::format("SETNAV,FREQ={}", frequency);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::enable_navigation_water_track() -> std::future<bool>
{
  if (mode_ == Mode::MEASUREMENT) {
    const std::string command = "APPLYNAV,USEWT=\"ON\"";
    return send_command(command, Mode::COMMAND);
  }
  const std::string command = "SETNAV,USEWT=\"ON\"";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::disable_navigation_water_track() -> std::future<bool>
{
  if (mode_ == Mode::MEASUREMENT) {
    const std::string command = "APPLYNAV,USEWT=\"OFF\"";
    return send_command(command, Mode::COMMAND);
  }
  const std::string command = "SETNAV,USEWT=\"OFF\"";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_field_calibration_mode(int mode) -> std::future<bool>
{
  const std::string command = std::format("SETFIELDCAL,MODE={}", mode);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_bottom_track_mode(const std::string & mode) -> std::future<bool>
{
  const std::string command = std::format("SETBT,MODE=\"{}\"", mode);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_bottom_track_velocity_range(double range) -> std::future<bool>
{
  const std::string command = std::format("SETBT,VR={:.2f}", range);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::enable_bottom_track_water_track() -> std::future<bool>
{
  const std::string command = "SETBT,WT=\"ON\"";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::disable_bottom_track_water_track() -> std::future<bool>
{
  const std::string command = "SETBT,WT=\"OFF\"";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_bottom_track_power_mode(const std::string & mode) -> std::future<bool>
{
  const std::string command = std::format("SETBT,PLMODE=\"{}\"", mode);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_bottom_track_power_level(double power_level) -> std::future<bool>
{
  const std::string command = std::format("SETBT,PL={:.2f}", power_level);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::tag(const std::string & name) -> std::future<bool>
{
  const std::string command = std::format("APPLYTAG,NAME=\"{}\"", name);
  return send_command(command, Mode::MEASUREMENT);
}

auto NucleusClient::set_water_track_mode(const std::string & mode) -> std::future<bool>
{
  const std::string command = std::format("SETWT,MODE=\"{}\"", mode);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_water_track_current(double vx, double vy, double vz) -> std::future<bool>
{
  const std::string command = std::format("SETWT,CURX={:.2f},CURY={:.2f},CURZ={:.2f}", vx, vy, vz);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_altimeter_power_level(double power_level) -> std::future<bool>
{
  const std::string command = std::format("SETALTI,PL={:.2f}", power_level);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_current_profile(double range, double cs, double bd, const std::string & coord)
  -> std::future<bool>
{
  const std::string command =
    std::format("SETCURPROF,RANGE={:.2f},CS={:.2f},BD={:.2f},COORD=\"{}\"", range, cs, bd, coord);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_trigger_source(const std::string & source) -> std::future<bool>
{
  const std::string command = std::format("SETTRIG,SRC=\"{}\"", source);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_internal_trigger_frequency(int frequency) -> std::future<bool>
{
  const std::string command = std::format("SETTRIG,FREQ={}", frequency);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_trigger_interleave_ratios(int altimeter_ratio, int current_profile_ratio) -> std::future<bool>
{
  const std::string command = std::format("SETTRIG,ALTI={},CP={}", altimeter_ratio, current_profile_ratio);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_imu_output_frequency(int frequency) -> std::future<bool>
{
  const std::string command = std::format("SETIMU,FREQ={}", frequency);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_magnetometer_output_frequency(int frequency) -> std::future<bool>
{
  const std::string command = std::format("SETMAG,FREQ={}", frequency);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_magnetometer_declination_method(const std::string & method) -> std::future<bool>
{
  const std::string command = std::format("SETMAG,METHOD=\"{}\"", method);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_magnetometer_hard_iron_calibration(double x, double y, double z) -> std::future<bool>
{
  const std::string command = std::format("SETMAGCAL,HX={:.3f},HY={:.3f},HZ={:.3f}", x, y, z);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_magnetometer_compensation_matrix(
  double m11,
  double m12,
  double m13,
  double m21,
  double m22,
  double m23,
  double m31,
  double m32,
  double m33) -> std::future<bool>
{
  const std::string command = std::format(
    "SETMAGCAL,M11={:.3f},M12={:.3f},M13={:.3f},M21={:.3f},M22={:.3f},M23={:.3f},M31={:.3f},M32={:.3f},M33={:.3f}",
    m11,
    m12,
    m13,
    m21,
    m22,
    m23,
    m31,
    m32,
    m33);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::update_mission_local_position(double x, double y) -> std::future<bool>
{
  const std::string command = std::format("UPDATEPOS,X={:.2f},Y={:.2f}", x, y);
  return send_command(command, Mode::MEASUREMENT);
}

auto NucleusClient::update_mission_relative_position(double x, double y) -> std::future<bool>
{
  const std::string command = std::format("UPDATEPOS,DX={:.2f},DY={:.2f}", x, y);
  return send_command(command, Mode::MEASUREMENT);
}

auto NucleusClient::update_mission_global_position(double longitude, double latitude) -> std::future<bool>
{
  const std::string command = std::format("UPDATEPOS,LONG={:.4f},LAT={:.4f}", longitude, latitude);
  return send_command(command, Mode::MEASUREMENT);
}

auto NucleusClient::update_mission_water_track_mode(const std::string & mode) -> std::future<bool>
{
  const std::string command = std::format("UPDATEWT,MODE=\"{}\"", mode);
  return send_command(command, Mode::MEASUREMENT);
}

auto NucleusClient::update_mission_current_velocity(double vx, double vy, double vz) -> std::future<bool>
{
  const std::string command = std::format("UPDATEWT,CURX={:.2f},CURY={:.2f},CURZ={:.2f}", vx, vy, vz);
  return send_command(command, Mode::MEASUREMENT);
}

auto NucleusClient::set_time() -> std::future<bool>
{
  const std::string time = std::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::utc_clock::now());
  const std::string command = std::format("SETCLOCKSTR,TIME=\"{}\"", time);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::reboot() -> std::future<bool> { return send_command("REBOOT", Mode::COMMAND); }

auto NucleusClient::get_error() -> std::future<std::string>
{
  // TODO(evan-palmer): maybe implement this
}

}  // namespace nucleus
