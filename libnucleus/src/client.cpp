// Copyright 2026, Evan Palmer
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
#include <utility>

#include "ascii.hpp"
#include "libnucleus/packet.hpp"
#include "libnucleus/report.hpp"
#include "login.hpp"

namespace nucleus
{

namespace
{

/// Read n_bytes from a socket and append them to a queue.
auto read_from_socket(
  int socket,
  std::deque<std::uint8_t> & buffer,
  std::size_t n_bytes,
  std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) -> ssize_t
{
  struct pollfd pfds[] = {{.fd = socket, .events = POLLIN, .revents = 0}};  // NOLINT
  const int rc = poll(pfds, 1, timeout.count());

  if (rc < 0) {
    return rc;
  }

  if (((pfds[0].revents & POLLIN) == 0)) {
    return 0;
  }

  std::vector<std::uint8_t> data(n_bytes);
  const ssize_t n_read = recv(socket, data.data(), data.size(), 0);

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

  // keep the socket in non-blocking mode
  return rc;
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
  socket_ = open(addr, 9002, connection_timeout);  // connect to the data-only port
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
  socket_ = open(addr, 9000, connection_timeout);  // connect to the primary port

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

auto NucleusClient::send_command(const std::string & command, Mode required_mode) -> std::future<Response>
{
  if (mode_ != required_mode) {
    throw std::runtime_error("Cannot send command in the current operating mode.");
  }

  {
    std::lock_guard lock(socket_mutex_);
    if (send(socket_, (command + protocol::DELIMITER).c_str(), command.size() + 2, 0) < 0) {
      throw std::runtime_error("Failed to send command to DVL");
    }
  }

  std::promise<Response> response;
  auto future = response.get_future();

  {
    // there isn't any identifier in the text responses, so the best that we can do is set the responses in the order
    // that the commands were sent
    std::lock_guard lock(command_mutex_);
    pending_responses_.emplace_back(std::move(response));
  }

  return future;
}

auto NucleusClient::start_measurement() -> std::future<Response>
{
  auto future = send_command("START", Mode::COMMAND);
  mode_ = Mode::MEASUREMENT;
  return future;
}

auto NucleusClient::stop_measurement() -> std::future<Response>
{
  auto future = send_command("STOP", Mode::MEASUREMENT);
  mode_ = Mode::COMMAND;
  return future;
}

auto NucleusClient::trigger() -> std::future<Response> { return send_command("TRIG", Mode::MEASUREMENT); }

auto NucleusClient::start_field_calibration() -> std::future<Response>
{
  return send_command("FIELDCAL", Mode::COMMAND);
}

auto NucleusClient::enable_fast_pressure(int sampling_rate) -> std::future<Response>
{
  const std::string command = std::format("SETFASTPRESSURE,EN=1,SR={}", sampling_rate);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::disable_fast_pressure() -> std::future<Response>
{
  const std::string command = "SETFASTPRESSURE,EN=0";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::save_settings(const std::string & settings) -> std::future<Response>
{
  const std::string command = std::format("SAVE,{}", settings);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::revert_to_default_settings(const std::string & settings) -> std::future<Response>
{
  const std::string command = std::format("SETDEFAULT,{}", settings);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::restore_settings(const std::string & settings) -> std::future<Response>
{
  const std::string command = std::format("RESTORE,{}", settings);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_mission_settings(
  double offset,
  double longitude,
  double latitude,
  double declination,
  double range,
  double blanking_distance,
  double speed_of_sound,
  double salinity) -> std::future<Response>
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

auto NucleusClient::enable_led() -> std::future<Response>
{
  const std::string command = "SETINST,LED=\"ON\"";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::disable_led() -> std::future<Response>
{
  const std::string command = "SETINST,LED=\"OFF\"";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_mounting_orientation(double roll, double pitch, double yaw) -> std::future<Response>
{
  const std::string command = std::format("SETINST,ROTXY={:.2f},ROTXZ={:.2f},ROTYZ={:.2f}", roll, pitch, yaw);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_ahrs_output_frequency(int frequency) -> std::future<Response>
{
  const std::string command = std::format("SETAHRS,FREQ={}", frequency);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_ahrs_mode(int mode) -> std::future<Response>
{
  const std::string command = std::format("SETAHRS,MODE={}", mode);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_navigation_data_output_frequency(int frequency) -> std::future<Response>
{
  const std::string command = std::format("SETNAV,FREQ={}", frequency);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::enable_navigation_water_track() -> std::future<Response>
{
  if (mode_ == Mode::MEASUREMENT) {
    const std::string command = "APPLYNAV,USEWT=\"ON\"";
    return send_command(command, Mode::COMMAND);
  }
  const std::string command = "SETNAV,USEWT=\"ON\"";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::disable_navigation_water_track() -> std::future<Response>
{
  if (mode_ == Mode::MEASUREMENT) {
    const std::string command = "APPLYNAV,USEWT=\"OFF\"";
    return send_command(command, Mode::COMMAND);
  }
  const std::string command = "SETNAV,USEWT=\"OFF\"";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_field_calibration_mode(int mode) -> std::future<Response>
{
  const std::string command = std::format("SETFIELDCAL,MODE={}", mode);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_bottom_track_mode(const std::string & mode) -> std::future<Response>
{
  const std::string command = std::format("SETBT,MODE=\"{}\"", mode);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_bottom_track_velocity_range(double range) -> std::future<Response>
{
  const std::string command = std::format("SETBT,VR={:.2f}", range);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::enable_bottom_track_water_track() -> std::future<Response>
{
  const std::string command = "SETBT,WT=\"ON\"";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::disable_bottom_track_water_track() -> std::future<Response>
{
  const std::string command = "SETBT,WT=\"OFF\"";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_bottom_track_power_mode(const std::string & mode) -> std::future<Response>
{
  const std::string command = std::format("SETBT,PLMODE=\"{}\"", mode);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_bottom_track_power_level(double power_level) -> std::future<Response>
{
  const std::string command = std::format("SETBT,PL={:.2f}", power_level);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::tag(const std::string & name) -> std::future<Response>
{
  const std::string command = std::format("APPLYTAG,NAME=\"{}\"", name);
  return send_command(command, Mode::MEASUREMENT);
}

auto NucleusClient::set_water_track_mode(const std::string & mode) -> std::future<Response>
{
  const std::string command = std::format("SETWT,MODE=\"{}\"", mode);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_water_track_current(double vx, double vy, double vz) -> std::future<Response>
{
  const std::string command = std::format("SETWT,CURX={:.2f},CURY={:.2f},CURZ={:.2f}", vx, vy, vz);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_altimeter_power_level(double power_level) -> std::future<Response>
{
  const std::string command = std::format("SETALTI,PL={:.2f}", power_level);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_current_profile(double range, double cs, double bd, const std::string & coord)
  -> std::future<Response>
{
  const std::string command =
    std::format("SETCURPROF,RANGE={:.2f},CS={:.2f},BD={:.2f},COORD=\"{}\"", range, cs, bd, coord);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_trigger_source(const std::string & source) -> std::future<Response>
{
  const std::string command = std::format("SETTRIG,SRC=\"{}\"", source);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_internal_trigger_frequency(int frequency) -> std::future<Response>
{
  const std::string command = std::format("SETTRIG,FREQ={}", frequency);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_trigger_interleave_ratios(int altimeter_ratio, int current_profile_ratio)
  -> std::future<Response>
{
  const std::string command = std::format("SETTRIG,ALTI={},CP={}", altimeter_ratio, current_profile_ratio);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_imu_output_frequency(int frequency) -> std::future<Response>
{
  const std::string command = std::format("SETIMU,FREQ={}", frequency);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_magnetometer_output_frequency(int frequency) -> std::future<Response>
{
  const std::string command = std::format("SETMAG,FREQ={}", frequency);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_magnetometer_declination_method(const std::string & method) -> std::future<Response>
{
  const std::string command = std::format("SETMAG,METHOD=\"{}\"", method);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::set_magnetometer_hard_iron_calibration(double x, double y, double z) -> std::future<Response>
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
  double m33) -> std::future<Response>
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

auto NucleusClient::update_mission_local_position(double x, double y) -> std::future<Response>
{
  const std::string command = std::format("UPDATEPOS,X={:.2f},Y={:.2f}", x, y);
  return send_command(command, Mode::MEASUREMENT);
}

auto NucleusClient::update_mission_relative_position(double x, double y) -> std::future<Response>
{
  const std::string command = std::format("UPDATEPOS,DX={:.2f},DY={:.2f}", x, y);
  return send_command(command, Mode::MEASUREMENT);
}

auto NucleusClient::update_mission_global_position(double longitude, double latitude) -> std::future<Response>
{
  const std::string command = std::format("UPDATEPOS,LONG={:.4f},LAT={:.4f}", longitude, latitude);
  return send_command(command, Mode::MEASUREMENT);
}

auto NucleusClient::update_mission_water_track_mode(const std::string & mode) -> std::future<Response>
{
  const std::string command = std::format("UPDATEWT,MODE=\"{}\"", mode);
  return send_command(command, Mode::MEASUREMENT);
}

auto NucleusClient::update_mission_current_velocity(double vx, double vy, double vz) -> std::future<Response>
{
  const std::string command = std::format("UPDATEWT,CURX={:.2f},CURY={:.2f},CURZ={:.2f}", vx, vy, vz);
  return send_command(command, Mode::MEASUREMENT);
}

auto NucleusClient::set_time() -> std::future<Response>
{
  const std::string time = std::format("{:%Y-%m-%d %H:%M:%S}", std::chrono::utc_clock::now());
  const std::string command = std::format("SETCLOCKSTR,TIME=\"{}\"", time);
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::reboot() -> std::future<Response> { return send_command("REBOOT", Mode::COMMAND); }

auto NucleusClient::get_error() -> std::future<Response>
{
  const std::string command = "GETERROR";
  return send_command(command, Mode::COMMAND);
}

auto NucleusClient::process_incoming_packet(const Packet & packet) -> void
{
  auto dispatch_report = [this](const auto & report) -> void {
    std::lock_guard lock(callback_mutex_);
    auto it = callbacks_.find(typeid(report));
    if (it != callbacks_.end()) {
      for (const auto & callback : it->second) {
        callback(&report);
      }
    }
  };

  switch (packet.series_id()) {
    case SeriesId::IMU_DATA:
      dispatch_report(packet.get<IMUReport>());
      break;
    case SeriesId::MAGNETOMETER_DATA:
      dispatch_report(packet.get<MagnetometerReport>());
      break;
    case SeriesId::FIELD_CALIBRATION_DATA:
      dispatch_report(packet.get<FieldCalibrationReport>());
      break;
    case SeriesId::FAST_PRESSURE_DATA:
      dispatch_report(packet.get<FastPressureReport>());
      break;
    case SeriesId::STRING_DATA:
      dispatch_report(packet.get<std::string>());
      break;
    case SeriesId::ALTIMETER_DATA:
      dispatch_report(packet.get<AltimeterReport>());
      break;
    case SeriesId::BOTTOM_TRACK_DATA:
      dispatch_report(packet.get<VelocityReport>());
      break;
    case SeriesId::WATER_TRACK_DATA:
      dispatch_report(packet.get<VelocityReport>());
      break;
    case SeriesId::CURRENT_PROFILER_DATA:
      // TODO(evan-palmer): figure out whether or not this is actually used
      break;
    case SeriesId::AHRS_DATA:
      dispatch_report(packet.get<AHRSReport>());
      break;
    case SeriesId::INS_DATA:
      dispatch_report(packet.get<INSReport>());
      break;
    default:
      const auto id = std::to_string(std::to_underlying(packet.series_id()));
      throw std::runtime_error("Received a packet with an unknown series ID: " + id);
  }
}

auto NucleusClient::process_incoming_response(const Response & response) -> void
{
  std::lock_guard lock(command_mutex_);
  if (!pending_responses_.empty()) {
    auto promise = std::move(pending_responses_.front());
    pending_responses_.pop_front();
    promise.set_value(response);
  } else {
    std::cout << "Received an unexpected response from the DVL: " << response.message << "\n";
  }
}

auto NucleusClient::poll_connection() -> void
{
  // Maintain a queue to store incoming data
  const std::size_t max_buffer_size = 4096;
  const std::size_t max_bytes_to_read = 2048;
  std::deque<std::uint8_t> buffer;

  while (running_.load()) {
    ssize_t n_read = -1;  // NOLINT
    {
      std::lock_guard lock(socket_mutex_);
      n_read = read_from_socket(socket_, buffer, max_bytes_to_read);
    }

    if (n_read < 0) {
      std::cout << "Failed to read from the DVL. The connection was likely lost.\n";
      continue;
    }

    // we are probably in command mode here
    if (buffer.size() == 0) {
      continue;
    }

    // this implements a circular-like buffer
    if (buffer.size() > max_buffer_size) {
      const auto n_to_remove = buffer.size() - max_buffer_size;
      buffer.erase(buffer.begin(), buffer.begin() + n_to_remove);
    }

    auto first_sync = std::ranges::find(buffer, protocol::SYNC_BYTE);
    if (first_sync != buffer.end()) {
      // process all data leading up to the first sync byte as ASCII data, which should contain command responses
      std::deque<std::uint8_t> ascii_data(buffer.begin(), first_sync);
      auto [responses, ascii_erase_iter] = protocol::split_responses(ascii_data);
      for (const auto & response : responses) {
        process_incoming_response(response);
      }
      buffer.erase(buffer.begin(), first_sync);

      // check if the remaining buffer might have a packet. if it does, then try to decode the data and process the
      // resulting packets
      if (protocol::might_contain_packet(buffer)) {
        auto [packets, packet_erase_iter] = protocol::decode_packets(buffer);
        if (!packets.empty()) {
          for (const auto & packet : packets) {
            process_incoming_packet(packet);
          }
        }
        buffer.erase(buffer.begin(), packet_erase_iter);
      }
    } else {
      auto [responses, ascii_erase_iter] = protocol::split_responses(buffer);
      for (const auto & response : responses) {
        process_incoming_response(response);
      }
      buffer.erase(buffer.begin(), ascii_erase_iter);
    }
  }
}

}  // namespace nucleus
