#pragma once

// hiwonder_brick: host-side driver for the Hiwonder ROS Robot Controller
// (STM32F407, extended firmware pos-v8) connected to the Raspberry Pi 5 over
// USB serial (CH343) at 1 Mbaud. It speaks the board's packet protocol
// (0xAA 0x55, function, length, payload, CRC-8/MAXIM) to drive the DC motors,
// the HX-35H steering bus servo and to read the on-board IMU. A background
// reader thread decodes replies and integrates the yaw. Used by both the Open
// Challenge and the Obstacle Challenge through spike_compat.

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace hiwonder {

// One IMU reading in host units: acceleration in g and gyro rate in °/s. The
// extended firmware publishes m/s^2 and rad/s (QMI8658.c built without
// QMI8658_UINT_MG_DPS); decode_imu_payload() does the conversion. Until
// 2026-10-04 the gyro was taken as °/s without conversion and the yaw came
// out 57 times too small.
struct ImuSample {
    float accel_x_g = 0.0F;
    float accel_y_g = 0.0F;
    float accel_z_g = 0.0F;
    float gyro_x_dps = 0.0F;
    float gyro_y_dps = 0.0F;
    float gyro_z_dps = 0.0F;
    float roll_deg = 0.0F;    // from the accelerometer
    float pitch_deg = 0.0F;   // from the accelerometer
    double yaw_deg = 0.0;     // integrated on the host from gyro_z minus bias
    std::uint64_t sequence = 0;  // increments on every new sample
    bool valid = false;
};

// Connection settings for Brick.
struct Config {
    // Board serial port. Empty = discover it on open() with
    // Brick::discover_device(), so any board works without editing code
    // (the CH343 serial number differs from board to board).
    std::string device;
    // Host-side speed limit (output shaft rev/s). Matches the firmware
    // rps_limit (pos-v6): the project motor reaches 5.76 rev/s unloaded at
    // 12 V with 100 % PWM.
    float max_speed_rps = 5.7F;
    // Motor with firmware position control (0..3 = M1..M4). Must match the
    // firmware POSITION_MOTOR_INDEX; the matching bit of
    // MotorPositionCapabilities::motor_mask confirms it on the board.
    std::uint8_t position_motor_id = 3;
};

// Last reply received from a bus servo (ID query or position read).
struct BusServoFeedback {
    std::uint8_t id = 0;
    std::int16_t position = 0;
    std::uint64_t sequence = 0;
    bool valid = false;
};

// Steering servo calibration. Values 0..1000 correspond to about 0..240 deg
// on the HX-35H. The three points must be passed explicitly so the program
// can never assume mechanical limits that would damage the Ackermann linkage.
// set_steering() maps -1 -> left, 0 -> center, +1 -> right.
struct SteeringCalibration {
    std::uint8_t servo_id = 1;
    std::uint16_t left = 400;
    std::uint16_t center = 500;
    std::uint16_t right = 600;
    std::uint16_t duration_ms = 300;   // ms, servo move time per command
};

// JGB37-520R30-12, 1:30 variant used on M1 and M4 (position motor):
// 11 encoder lines * x4 quadrature * 30 reduction = 1320 ticks per output
// shaft revolution.
struct EncoderCalibration {
    std::int32_t ticks_per_output_revolution = 1320;
};

// State of the firmware position controller, as reported in MOTOR_EXT replies.
enum class MotorPositionState : std::uint8_t {
    idle = 0,
    moving = 1,
    reached = 2,
    cancelled = 3,
    timeout = 4,
    invalid_command = 5,
    holding = 6,   // HOLD active (MOTOR_EXT v3)
    stalled = 7,   // HOLD released because of a stall: 3 s at max PWM without moving
};

// Reply to the MOTOR_EXT capabilities query. v1: position control only on M1
// (mask 0x01). v2: the position motor is the bit set in motor_mask, and
// ticks_per_revolution belongs to that motor.
struct MotorPositionCapabilities {
    std::uint8_t protocol_version = 0;
    std::uint8_t motor_mask = 0;
    std::uint32_t ticks_per_revolution = 0;
    std::uint64_t sequence = 0;
    bool valid = false;
};

// Reply to a MOTOR_EXT command (move, cancel, zero, status, hold).
struct MotorPositionStatus {
    std::uint8_t command = 0;   // MOTOR_EXT sub-command this reply answers
    std::uint8_t motor_id = 0;
    MotorPositionState state = MotorPositionState::idle;
    bool active = false;              // true while a move or HOLD is running
    std::int32_t position_ticks = 0;  // encoder ticks since the last zero
    std::int32_t target_ticks = 0;
    std::int32_t error_ticks = 0;     // target - position
    float speed_rps = 0.0F;           // output shaft rev/s
    std::uint64_t sequence = 0;
    bool valid = false;
};

// Connection to one Hiwonder board. Thread-safe: commands may be sent from
// any thread while the reader thread updates IMU, servo and motor state.
// Destroying the Brick stops all motors and closes the port.
class Brick {
public:
    // Throws std::invalid_argument if position_motor_id is not 0..3.
    explicit Brick(Config config = {});
    ~Brick();

    Brick(const Brick &) = delete;
    Brick &operator=(const Brick &) = delete;

    // Opens the serial port at 1 Mbaud (discovering it if Config::device is
    // empty) and starts the reader thread. Throws on failure.
    void open();
    // Stops all motors, joins the reader thread and closes the port.
    void close() noexcept;
    bool is_open() const noexcept;

    // Port opened by open(): Config::device or the discovered one. Empty
    // before opening.
    const std::string &device() const noexcept { return device_; }

    // Finds the board: HIWONDER_DEVICE if set and not empty; otherwise the
    // single usb-1a86_USB_Single_Serial_* entry (the board's CH343) in
    // by_id_dir. Throws std::runtime_error if there is none or more than one.
    static std::string discover_device(
        const std::string &by_id_dir = "/dev/serial/by-id");

    // Closed-loop speed command (output shaft rev/s, signed) for motor 0..3.
    // Cancels any running position move or HOLD on that motor.
    void set_motor_speed(std::uint8_t motor_id, float speed_rps);
    // Stops one motor (COAST) / all four motors.
    void stop_motor(std::uint8_t motor_id);
    void stop_all();

    // Motor that receives the MOTOR_EXT commands (Config::position_motor_id).
    std::uint8_t position_motor_id() const noexcept {
        return config_.position_motor_id;
    }

    // MOTOR_EXT commands. Each one sends a request and blocks until the
    // matching reply arrives or the timeout expires (then throws
    // std::runtime_error).
    // Queries which motors support position control.
    MotorPositionCapabilities motor_position_capabilities(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(800));
    // Starts a relative move of delta_ticks (sign = direction) at up to
    // max_rps; the firmware finishes when |error| <= tolerance_ticks or after
    // motion_timeout_ms (>= 100). Returns the acknowledgement, not the end.
    MotorPositionStatus move_motor_relative(
        std::int32_t delta_ticks, float max_rps,
        std::uint16_t tolerance_ticks, std::uint16_t motion_timeout_ms,
        std::chrono::milliseconds reply_timeout = std::chrono::milliseconds(800));
    // Current position, target and state of the position motor.
    MotorPositionStatus motor_position_status(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(800));
    // Aborts the running move or HOLD.
    MotorPositionStatus cancel_motor_position(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(800));
    // Sets the encoder position of the position motor to 0.
    MotorPositionStatus zero_motor_position(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(800));
    // Polls motor_position_status() every poll_period until the move is no
    // longer active; throws if it is still active after timeout.
    MotorPositionStatus wait_motor_position(
        std::chrono::milliseconds timeout,
        std::chrono::milliseconds poll_period = std::chrono::milliseconds(50));
    // Position HOLD (MOTOR_EXT v3, like motor.HOLD on SPIKE): holds the current
    // position with no timeout and returns to it if pushed away, with the PWM
    // limited to max_pulse (300..1000). It is cancelled by a speed command,
    // move_motor_relative, cancel_motor_position or stop.
    MotorPositionStatus hold_motor_position(
        float max_rps, std::uint16_t tolerance_ticks, std::uint16_t max_pulse,
        std::chrono::milliseconds reply_timeout = std::chrono::milliseconds(800));

    // Bus servo commands. Moves servo_id (0..253) to position 0..1000 in
    // duration_ms (20..30000 ms). Does not wait for the move to finish.
    void set_bus_servo_position(std::uint8_t servo_id,
                                std::uint16_t position,
                                std::uint16_t duration_ms);
    // Turns the servo torque on or off (off = shaft free).
    void set_bus_servo_power(std::uint8_t servo_id, bool enabled);
    // Broadcast ID query; only one servo may be connected. Blocks until reply.
    std::uint8_t read_bus_servo_id(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(800)) const;
    // Reads the current position of servo_id. Blocks until reply or timeout.
    BusServoFeedback read_bus_servo_position(
        std::uint8_t servo_id, std::chrono::milliseconds timeout) const;
    // Steering in normalized units: -1 = calibration.left, 0 = center,
    // +1 = calibration.right, linear in between. Throws outside -1..1.
    void set_steering(double normalized,
                      const SteeringCalibration &calibration);
    void center_steering(const SteeringCalibration &calibration);

    // Latest IMU sample (copy).
    ImuSample imu() const;
    // Blocks until a new valid IMU sample arrives (20 Hz); false on timeout.
    bool wait_for_imu(std::chrono::milliseconds timeout) const;
    // Averages the gyro over `duration` (>= 500 ms, robot still) to get the
    // bias, then sets yaw to 0. Blocks for the whole duration.
    void calibrate_gyro(std::chrono::milliseconds duration);
    // Sets the integrated yaw to yaw_deg.
    void reset_yaw(double yaw_deg = 0.0);

    // The API exists in the client, but only answers once the extended
    // firmware is installed. motor_position_capabilities() is the runtime
    // check that avoids assuming the board was already updated.
    static constexpr bool has_motor_position_feedback() noexcept {
        return true;
    }

    // Pure, testable degree <-> tick conversion. Sends nothing to the motor.
    static std::int64_t motor_degrees_to_ticks(
        double degrees,
        const EncoderCalibration &calibration = {});
    static double motor_ticks_to_degrees(
        std::int64_t ticks,
        const EncoderCalibration &calibration = {});

    // Firmware IMU payload (24 bytes: 3 acceleration floats in m/s^2 and 3
    // gyro floats in rad/s) -> ImuSample in g and °/s, with roll and pitch
    // from the accelerometer. Does not integrate yaw or mark the sample valid.
    // Pure.
    static ImuSample decode_imu_payload(const std::uint8_t *payload);

    // CRC-8/MAXIM (reflected polynomial 0x8C, init 0) used by the protocol.
    static std::uint8_t crc8_maxim(const std::uint8_t *data,
                                   std::size_t size) noexcept;
    // Builds a frame: 0xAA 0x55, function, length, payload, CRC over
    // function+length+payload. Payload is limited to 255 bytes.
    static std::vector<std::uint8_t>
    encode_frame(std::uint8_t function,
                 const std::vector<std::uint8_t> &payload);

private:
    void send_frame(std::uint8_t function,
                    const std::vector<std::uint8_t> &payload);
    void reader_loop() noexcept;
    void consume_bytes(const std::uint8_t *data, std::size_t size);
    void process_frame(std::uint8_t function,
                       const std::vector<std::uint8_t> &payload);

    Config config_;
    std::string device_;
    int fd_ = -1;
    bool running_ = false;           // reader thread keeps running while true
    std::thread reader_;
    // state_mutex_ guards fd_, running_ and all received state below; each
    // condition variable is notified when its kind of reply arrives.
    mutable std::mutex state_mutex_;
    mutable std::condition_variable imu_cv_;
    mutable std::condition_variable servo_cv_;
    mutable std::condition_variable motor_position_cv_;
    std::mutex write_mutex_;         // serialises whole frames on the port
    ImuSample imu_;
    BusServoFeedback bus_servo_;
    MotorPositionCapabilities motor_position_capabilities_;
    MotorPositionStatus motor_position_status_;
    std::array<double, 3> gyro_bias_{{0.0, 0.0, 0.0}};   // °/s, from calibrate_gyro()
    std::chrono::steady_clock::time_point last_imu_time_{};  // for yaw integration dt
    std::vector<std::uint8_t> rx_buffer_;   // unparsed bytes (reader thread only)
};

}  // namespace hiwonder
