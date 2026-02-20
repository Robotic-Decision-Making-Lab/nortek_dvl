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

#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <future>
#include <string>
#include <thread>
#include <typeindex>
#include <unordered_map>

#include "libnucleus/mode.hpp"
#include "libnucleus/packet.hpp"
#include "libnucleus/response.hpp"
#include "libnucleus/series_id.hpp"

namespace nucleus
{

class NucleusClient
{
public:
  NucleusClient(const std::string & addr, std::chrono::seconds connection_timeout = std::chrono::seconds(5));

  NucleusClient(
    const std::string & addr,
    const std::string & password,
    std::chrono::seconds connection_timeout = std::chrono::seconds(5));

  ~NucleusClient();

  /// Start measurement, data output, and data recording.
  ///
  /// Measurements will continue until a stop command is issued or power is removed. The Nucleus remains in an idle
  /// state and does not start measurements until a start is issued.
  ///
  /// Command: START
  /// Command type: ACTION
  /// Mode: COMMAND
  [[nodiscard]] auto start_measurement() -> std::future<Response>;

  /// Stop measurement.
  ///
  /// Stops all measurements and data output. The recorded data can be downloaded using the Nortek Nucleus Software.
  ///
  /// Command: STOP
  /// Command type: ACTION
  /// Mode: MEASUREMENT
  [[nodiscard]] auto stop_measurement() -> std::future<Response>;

  /// Trigger an acoustic measurement.
  ///
  /// The triggered acoustic measurement will either be Bottom Track, Altimeter, or Current Profile. The type of
  /// measurement can be configured using the trigger settings.
  ///
  /// INFO: This command is only valid when the trigger source has been set to "COMMAND".
  /// INFO: This command has no effect if measurements have not been started.
  ///
  /// Command: TRIG
  /// Command type: ACTION
  /// Mode: MEASUREMENT
  [[nodiscard]] auto trigger() -> std::future<Response>;

  /// Start field calibration.
  ///
  /// INFO: The field calibration procedure runs until it is stopped by `stop_measurement`.
  ///
  /// Command: FIELDCAL
  /// Command type: ACTION
  /// Mode: COMMAND
  [[nodiscard]] auto start_field_calibration() -> std::future<Response>;

  /// Enable fast pressure reading using the desired sampling rate (10 Hz, 15 Hz, or 30 Hz).
  ///
  /// INFO: The fast pressure reading requires a license.
  ///
  /// Command: SETFASTPRESSURE
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto enable_fast_pressure(int sampling_rate = 10) -> std::future<Response>;

  /// Disable fast pressure reading.
  ///
  /// INFO: The fast pressure reading requires a license.
  ///
  /// Command: SETFASTPRESSURE
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto disable_fast_pressure() -> std::future<Response>;

  /// Save the specified settings and use them as the default for future operation.
  ///
  /// At least one of the following arguments must be provided to specify which settings to save. Defaults to "ALL"
  /// settings.
  ///   - "ALL": Save all settings,
  ///   - "CONFIG": Save all settings except COMM, MISSION, and MAGCAL,
  ///   - "MISSION": Save MISSION settings,
  ///   - "MAGCAL": Save MAGCAL settings.
  ///
  /// INFO: When the `start_measurement` command is issued, CONFIG, COMM, and MISSION settings are saved automatically.
  /// MAGCAL settings are *not* saved; this means that if the instrument is rebooted (e.g. due to a power glitch), the
  /// next time the START command is given the magnetometer calibration values may be different. Use SAVE,MAGCAL or
  /// SAVE,ALL to make magnetometer calibration values permanent.
  ///
  /// Command: SAVE
  /// Command type: ACTION
  /// Mode: COMMAND
  [[nodiscard]] auto save_settings(const std::string & settings = "ALL") -> std::future<Response>;

  /// Revert to the factory default settings.
  ///
  /// At least one of the following arguments must be provided to specify which settings to revert.
  ///   - "ALL": Revert all settings,
  ///   - "CONFIG": Revert all settings except COMM, MISSION, and MAGCAL,
  ///   - "MISSION": Revert MISSION settings,
  ///   - "MAGCAL": Revert MAGCAL settings.
  ///
  /// This does not make the default values permanent; to do so, you must save the corresponding settings after issuing
  /// this command. For example,
  /// ```
  /// nucleus.revert_to_default_settings("CONFIG");
  /// nucleus.save_settings("CONFIG");
  /// ```
  ///
  /// Command: SETDEFAULT
  /// Command type: ACTION
  /// Mode: COMMAND
  [[nodiscard]] auto revert_to_default_settings(const std::string & settings = "ALL") -> std::future<Response>;

  /// Restore previously saved settings.
  ///
  /// At least one of the following arguments must be provided to specify which settings to restore.
  ///   - "ALL": Restore all settings,
  ///   - "CONFIG": Restore all settings except COMM, MISSION, and MAGCAL
  ///   - "MISSION": Restore MISSION settings,
  ///   - "MAGCAL": Restore MAGCAL settings.
  ///
  /// This can be useful if you have unintentionally changed settings, or if you want to discard the magnetometer
  /// calibration after doing a field calibration.
  ///
  /// Command: RESTORE
  /// Command type: ACTION
  /// Mode: COMMAND
  [[nodiscard]] auto restore_settings(const std::string & settings = "ALL") -> std::future<Response>;

  /// Set the mission settings, including:
  ///   - offset value (dBar) of the pressure sensor, [0, 11] dBar,
  ///   - initial longitude (deg), [-180.0, 180.0] deg; set to 9999 if unknown,
  ///   - initial latitude (deg), [-90.0, 90.0] deg; set to 9999 if unknown,
  ///   - magnetic field declination (deg), [-90.0, 90.0] deg,
  ///   - DVL and altimeter range (m), [2.0, 50.0] m,
  ///   - DVL and altimeter blanking distance (m), [0.1, 5.0] m,
  ///   - speed of sound (m/s), [0, 1700] m/s; setting to 0 set sensor to use measured sound velocity,
  ///   - salinity (PPT), [0, 50] ppt
  ///
  /// INFO: The pressure sensor measures the total pressure. The offset is defined as the difference between the
  /// hydrostatic and the measured pressure, enabling the system to calculate the hydrostatic pressure. Any error in
  /// the offset will directly propagate to error in hydrostatic pressure and thus also to depth estimation.
  ///
  /// Command: SETMISSION
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_mission_settings(
    double offset = 9.5,
    double longitude = 9999,
    double latitude = 9999,
    double declination = 0.0,
    double range = 50.0,
    double blanking_distance = 0.1,
    double speed_of_sound = 1481,
    double salinity = 35.0) -> std::future<Response>;

  /// Enable the instrument LED indicator.
  ///
  /// Command: SETINST
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto enable_led() -> std::future<Response>;

  /// Disable the instrument LED indicator.
  ///
  /// Command: SETINST
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto disable_led() -> std::future<Response>;

  /// Set the instrument orientation with respect to the body frame using:
  ///   - yaw (deg),
  ///   - pitch (deg),
  ///   - roll (deg).
  ///
  /// INFO: The rotations should be described using the order Rz(yaw) * Ry(pitch) * Rx(roll).
  ///
  /// Command: SETINST
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_mounting_orientation(double roll, double pitch, double yaw) -> std::future<Response>;

  /// Set the AHRS output frequency (Hz); in the range [1, 100] Hz.
  ///
  /// Command: SETAHRS
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_ahrs_output_frequency(int frequency) -> std::future<Response>;

  /// Set the AHRS mode:
  ///   - 0: Fixed hard iron / soft iron,
  ///   - 1: Hard iron estimation,
  ///   - 2: Hard and soft iron estimation.
  ///
  /// Command: SETAHRS
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_ahrs_mode(int mode) -> std::future<Response>;

  /// Configure the navigation data output frequency (Hz); in the range [1, 100] Hz.
  ///
  /// Command: SETNAV
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_navigation_data_output_frequency(int frequency) -> std::future<Response>;

  /// Use water track in navigation estimation.
  ///
  /// NOTE: If this is called while the instrument is in measurement mode, this function will call the APPLYNAV command
  /// internally to apply the new navigation settings.
  ///
  /// Command: SETNAV
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto enable_navigation_water_track() -> std::future<Response>;

  /// Disable water track in navigation estimation.
  ///
  /// NOTE: If this is called while the instrument is in measurement mode, this function will call the APPLYNAV command
  /// internally to apply the new navigation settings.
  ///
  /// Command: SETNAV
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto disable_navigation_water_track() -> std::future<Response>;

  /// Specify how the field calibration is performed given the desired mode:
  ///   - 1: hard iron estimation,
  ///   - 2: hard and soft iron estimation.
  ///
  /// These settings take effect when starting the instrument with the FIELDCAL command.
  ///
  /// Command: SETFIELDCAL
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_field_calibration_mode(int mode) -> std::future<Response>;

  /// Set the bottom track mode. The mode should be set to one of the following:
  ///   - "FAST_ACQ": fast acquisition mode,
  ///   - "CRAWLER": crawler mode,
  ///   - "AUTO": automatically switch between FAST_ACQ and CRAWLER based on the vehicle speed and ground distance.
  ///
  /// Command: SETBT
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_bottom_track_mode(const std::string & mode) -> std::future<Response>;

  /// Set the bottom track velocity range (m/s).
  ///
  /// The default value in FAST_ACQ mode is 5 m/s. In CRAWLER mode, the value must be in the range [0.05, 0.4] m/s.
  ///
  /// Command: SETBT
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_bottom_track_velocity_range(double range) -> std::future<Response>;

  /// Enable water track measurements in bottom track mode.
  ///
  /// Command: SETBT
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto enable_bottom_track_water_track() -> std::future<Response>;

  /// Disable water track measurements in bottom track mode.
  ///
  /// Command: SETBT
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto disable_bottom_track_water_track() -> std::future<Response>;

  /// Set the power mode for the DVL. This should be set to one of the following:
  ///    - "MAX": The power level is always set to the maximum value,
  ///    - "USER": The power level is set manually using SETBT,PL
  ///
  /// Command: SETBT
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_bottom_track_power_mode(const std::string & mode) -> std::future<Response>;

  /// Set the power level (dB) for the DVL in bottom track mode, in the range [-20, 0] dB.
  ///
  /// Command: SETBT
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_bottom_track_power_level(double power_level) -> std::future<Response>;

  /// Mark the measurement data for future reference in the data log.
  ///
  /// The max length of the tag string is 50 ASCII characters.
  /// Command: APPLYTAG
  /// Command type: CONFIGURATION
  /// Mode: MEASUREMENT
  [[nodiscard]] auto tag(const std::string & name) -> std::future<Response>;

  /// Set the water track mode. The mode should be set to one of the following:
  ///   - "FIXED": assumes that current direction is fixed in NED,
  ///   - "ESTCUR": estimates the current direction in NED from from water track measurements
  ///
  /// Command: SETWT
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_water_track_mode(const std::string & mode) -> std::future<Response>;

  /// Set the initial current velocity (m/s), in the range [-10, 10] m/s.
  ///
  /// Command: SETWT
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_water_track_current(double vx, double vy, double vz) -> std::future<Response>;

  /// Set the altimeter power level (dB), in the range [-20, 0] dB.
  ///
  /// To disable the altimeter transmission, set the power level to -100 dB.
  ///
  /// Command: SETALTI
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_altimeter_power_level(double power_level) -> std::future<Response>;

  /// Set the current profile measurement settings, including:
  ///   - profile range (m), in the range [1, 30],
  ///   - cell size (m), in the range [0.2, 2.0],
  ///   - blanking distance (m), in the range [0.1, 10.0],
  ///   - the coordinate system that cells are referenced to, either "VEHICLE" or "BEAM".
  ///
  /// The instrument can be configured to collect current profile data. When enabled, current profile measurements are
  /// interleaved with bottom track and altimeter measurements.
  ///
  /// When the coordinate system is set to "VEHICLE", cells are returned in the vehicle coordinate frame. When set to
  /// "BEAM", cells are returned in the beam coordinate frame.
  ///
  /// Command: SETCURPROF
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_current_profile(double range, double cs, double bd, const std::string & coord)
    -> std::future<Response>;

  /// Set the trigger source, to one of the following:
  ///   - "INTERNAL": internal triggering at a fixed frequency,
  ///   - "EXTRISE": trigger on the rise edge of an external signal,
  ///   - "EXTFALL": trigger on the fall edge of an external signal,
  ///   - "EXTEDGES": trigger on both edges of an external signal,
  ///   - "COMMAND": trigger on command.
  ///
  /// Command: SETTRIG
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_trigger_source(const std::string & source) -> std::future<Response>;

  /// Set the internal trigger frequency (Hz), in the range [1, 8] Hz.
  ///
  /// The max trigger frequency depends on the acoustic range. For high acoustic range values (as configured in a
  /// mission), the maximum trigger frequency less than 8 Hz. If the trigger frequency is too high for the selected
  /// range, an error will be reported by the SAVE, START, or FIELDCAL commands when they are issued.
  ///
  /// Command: SETTRIG
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_internal_trigger_frequency(int frequency) -> std::future<Response>;

  /// Set the altimeter and current profile interleave ratios.
  ///
  /// The interleave ratios determine how often altimeter and current profile measurements are taken relative to bottom
  /// track measurements. For example, setting an altimeter ratio of 2 and a current profile ratio of 3 means that an
  /// altimeter measurement is taken every 2 bottom track measurements, and a current profile measurement is taken every
  /// 3 bottom track measurements. The ratios can be set to 0 to disable the corresponding measurement.
  ///
  /// Valid ratios are: 0, 2, 3, ..., 20.
  ///
  /// Command: SETTRIG
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_trigger_interleave_ratios(int altimeter_ratio, int current_profile_ratio)
    -> std::future<Response>;

  /// Set the IMU output frequency (Hz) given a frequency in the range [1, 100] Hz.
  ///
  /// Command: SETIMU
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_imu_output_frequency(int frequency) -> std::future<Response>;

  /// Set the magnetometer output frequency (Hz) given a frequency in the range [1, 75] Hz.
  ///
  /// Command: SETMAG
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_magnetometer_output_frequency(int frequency) -> std::future<Response>;

  /// Set the magnetometer declination method, to one of the following:
  ///   - "AUTO": if the initial position is set (using SETMISSION), "WMM" is chosen, otherwise, the declination method
  ///     chosen in the mission is used.
  ///   - "OFF": no declination correction is applied; the magnetometer readings are in magnetic coordinates.
  ///   - "WMM": the World Magnetic Model is used to compute the declination - requires the initial position to be set
  ///     for the mission.
  ///
  /// Command: SETMAG
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_magnetometer_declination_method(const std::string & method) -> std::future<Response>;

  /// Set the magnetometer hard iron calibration values (Gauss) to values in the range [-1, 1].
  ///
  /// Command: SETMAGCAL
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_magnetometer_hard_iron_calibration(double x, double y, double z) -> std::future<Response>;

  /// Set the magnetometer soft iron compensation matrix to values in the range [-2, 2].
  ///
  /// Command: SETMAGCAL
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_magnetometer_compensation_matrix(
    double m11,
    double m12,
    double m13,
    double m21,
    double m22,
    double m23,
    double m31,
    double m32,
    double m33) -> std::future<Response>;

  /// Update the local position (m) during a mission, given x and y coordinates.
  ///
  /// Command: UPDATEPOS
  /// Command type: CONFIGURATION
  /// Mode: MEASUREMENT
  [[nodiscard]] auto update_mission_local_position(double x, double y) -> std::future<Response>;

  /// Update the relative position change (m) with respect to the initial position during a mission, given the relative
  /// x and y coordinates.
  ///
  /// Command: UPDATEPOS
  /// Command type: CONFIGURATION
  /// Mode: MEASUREMENT
  [[nodiscard]] auto update_mission_relative_position(double x, double y) -> std::future<Response>;

  /// Update the global position (deg) during a mission, given longitude and latitude.
  ///
  /// Command: UPDATEPOS
  /// Command type: CONFIGURATION
  /// Mode: MEASUREMENT
  [[nodiscard]] auto update_mission_global_position(double longitude, double latitude) -> std::future<Response>;

  /// Update the water track mode during a mission to one of the following:
  ///   - "FIXED": assumes that current direction is fixed in NED,
  ///   - "ESTCUR": estimates the current direction in NED from from water track measurements
  ///
  /// Command: UPDATEWT
  /// Command type: CONFIGURATION
  /// Mode: MEASUREMENT
  [[nodiscard]] auto update_mission_water_track_mode(const std::string & mode) -> std::future<Response>;

  /// Update the water track current velocity (m/s) during a mission.
  ///
  /// Command: UPDATEWT
  /// Command type: CONFIGURATION
  /// Mode: MEASUREMENT
  [[nodiscard]] auto update_mission_current_velocity(double vx, double vy, double vz) -> std::future<Response>;

  /// Set the real-time clock of the instrument to the current system time.
  ///
  /// Command: SETCLOCKSTR
  /// Command type: CONFIGURATION
  /// Mode: COMMAND
  [[nodiscard]] auto set_time() -> std::future<Response>;

  /// Reboot the instrument.
  ///
  /// Command: REBOOT
  /// Command type: ACTION
  /// Mode: COMMAND
  [[nodiscard]] auto reboot() -> std::future<Response>;

  /// Get the most recent error message from the instrument.
  ///
  /// Command: GETERROR
  /// Command type: INFO
  /// Mode: COMMAND
  [[nodiscard]] auto get_error() -> std::future<Response>;

  /// Register a callback function to receive reports of the specified type.
  ///
  /// For example, to register a callback for AHRS reports:
  /// ```
  /// nucleus.subscribe<AHRSReport>([](const AHRSReport & report) {
  ///   std::format("Received AHRS report with timestamp: {} ns", report.timestamp.count());
  /// });
  /// ```
  template <typename T>
  auto subscribe(std::function<void(const T &)> && callback) -> void
  {
    std::lock_guard<std::mutex> lock(callback_mutex_);  // NOLINT
    callbacks_[typeid(T)].emplace_back(
      [cb = std::move(callback)](const void * report) -> auto { cb(*static_cast<const T *>(report)); });
  }

private:
  /// Send a command to the instrument and return a future for the command response.
  [[nodiscard]] auto send_command(const std::string & command, Mode required_mode) -> std::future<Response>;

  /// Poll the connection for incoming data.
  auto poll_connection() -> void;

  /// Process incoming data and dispatch to registered callbacks.
  auto process_incoming_packet(const Packet & packet) -> void;

  /// Process incoming responses and set the corresponding promises.
  auto process_incoming_response(const Response & response) -> void;

  int socket_;

  std::atomic<bool> running_{false};

  std::mutex socket_mutex_;

  std::deque<std::promise<Response>> pending_responses_;
  std::mutex command_mutex_;

  std::thread polling_thread_;

  std::unordered_map<std::type_index, std::vector<std::function<void(const void *)>>> callbacks_;
  std::mutex callback_mutex_;

  bool command_interface_available_{false};
  Mode mode_{Mode::COMMAND};
};

}  // namespace nucleus
