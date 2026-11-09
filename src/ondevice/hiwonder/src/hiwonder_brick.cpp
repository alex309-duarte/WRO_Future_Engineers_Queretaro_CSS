// Implementation of hiwonder::Brick: serial port setup (raw, 8N1, 1 Mbaud),
// frame encoding/decoding for the Hiwonder ROS Robot Controller protocol,
// request/reply commands for the motors (including the MOTOR_EXT position
// controller of firmware pos-v8), the steering bus servo and the IMU, and the
// reader thread that integrates the yaw used for heading control in both the
// Open Challenge and the Obstacle Challenge.

#include "hiwonder_brick/hiwonder_brick.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <stdexcept>
#include <sys/select.h>
#include <system_error>
#include <termios.h>
#include <type_traits>
#include <unistd.h>
#include <utility>

namespace hiwonder {
namespace {

// Frame layout: 0xAA 0x55 | function | payload length | payload | CRC-8.
constexpr std::uint8_t kHeader1 = 0xAA;
constexpr std::uint8_t kHeader2 = 0x55;
// Function codes of the board protocol.
constexpr std::uint8_t kFunctionMotor = 3;        // speed / stop of M1..M4
constexpr std::uint8_t kFunctionMotorExt = 0x0B;  // extended firmware: position control
constexpr std::uint8_t kFunctionBusServo = 5;     // serial bus servos
constexpr std::uint8_t kFunctionImu = 7;          // IMU telemetry pushed by the board
constexpr std::array<std::uint8_t, 2> kHeader{{kHeader1, kHeader2}};

// Appends the raw little-endian bytes of `value` (the STM32 and the Pi are
// both little-endian, so a byte copy is enough).
template <typename T>
void append_le(std::vector<std::uint8_t> &out, const T &value) {
    static_assert(std::is_trivially_copyable<T>::value,
                  "append_le requires a trivially copyable type");
    const auto *bytes = reinterpret_cast<const std::uint8_t *>(&value);
    out.insert(out.end(), bytes, bytes + sizeof(T));
}

// Reads a little-endian float / integer from an unaligned buffer.
float read_float_le(const std::uint8_t *data) {
    float value = 0.0F;
    std::memcpy(&value, data, sizeof(value));
    return value;
}

template <typename T>
T read_le(const std::uint8_t *data) {
    static_assert(std::is_trivially_copyable<T>::value,
                  "read_le requires a trivially copyable type");
    T value{};
    std::memcpy(&value, data, sizeof(value));
    return value;
}

// Builds an exception that includes the current errno text.
std::runtime_error system_error(const std::string &what) {
    return std::runtime_error(what + ": " + std::strerror(errno));
}

}  // namespace

Brick::Brick(Config config) : config_(std::move(config)) {
    if (config_.position_motor_id > 3) {
        throw std::invalid_argument("position_motor_id must be between 0 and 3");
    }
}

Brick::~Brick() { close(); }

void Brick::open() {
    if (is_open()) {
        return;
    }

    const std::string device =
        config_.device.empty() ? discover_device() : config_.device;
    const int fd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        throw system_error("could not open " + device);
    }

    // Raw 8N1 at 1 Mbaud, no flow control. VMIN = VTIME = 0 makes read()
    // non-blocking; the reader thread waits with select() instead.
    termios options{};
    if (tcgetattr(fd, &options) != 0) {
        ::close(fd);
        throw system_error("tcgetattr");
    }
    cfmakeraw(&options);
    options.c_cflag = CS8 | CREAD | CLOCAL;
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 0;
    if (cfsetispeed(&options, B1000000) != 0 ||
        cfsetospeed(&options, B1000000) != 0 ||
        tcsetattr(fd, TCSANOW, &options) != 0) {
        ::close(fd);
        throw system_error("could not set the port to 1000000 baud");
    }
    tcflush(fd, TCIFLUSH);   // drop stale bytes received before opening

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        fd_ = fd;
        device_ = device;
        running_ = true;
        imu_ = {};
        last_imu_time_ = {};
        rx_buffer_.clear();
    }
    reader_ = std::thread(&Brick::reader_loop, this);
}

std::string Brick::discover_device(const std::string &by_id_dir) {
    if (const char *forced = std::getenv("HIWONDER_DEVICE");
        forced != nullptr && *forced != '\0') {
        return forced;
    }
    // The board's CH343 enumerates as QinHeng (1a86) "USB Single Serial";
    // only the serial number changes. The LiDAR adapter (CP2102N) and CH340
    // adapters ("USB Serial") do not match this prefix.
    static const std::string kPrefix = "usb-1a86_USB_Single_Serial_";
    std::vector<std::string> found;
    std::error_code error;
    for (std::filesystem::directory_iterator it(by_id_dir, error), end;
         !error && it != end; it.increment(error)) {
        const std::string name = it->path().filename().string();
        if (name.rfind(kPrefix, 0) == 0) {
            found.push_back(it->path().string());
        }
    }
    std::sort(found.begin(), found.end());
    if (found.empty()) {
        throw std::runtime_error(
            "Hiwonder board not found in " + by_id_dir +
            " (is the UART2 USB-C cable connected?); to force a port, set "
            "HIWONDER_DEVICE=/dev/...");
    }
    if (found.size() > 1) {
        std::string list;
        for (const auto &path : found) list += "\n  " + path;
        throw std::runtime_error(
            "several Hiwonder boards are connected; choose one with "
            "HIWONDER_DEVICE:" + list);
    }
    return found.front();
}

// Order matters: motors are stopped while the port is still open, then the
// reader thread is stopped and joined, and only then is the fd closed.
void Brick::close() noexcept {
    int fd = -1;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (fd_ < 0) {
            return;
        }
        fd = fd_;
    }

    try {
        stop_all();
    } catch (...) {
    }

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        running_ = false;
    }
    if (reader_.joinable()) {
        reader_.join();
    }
    ::close(fd);
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        fd_ = -1;
    }
}

bool Brick::is_open() const noexcept {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return fd_ >= 0;
}

void Brick::set_motor_speed(std::uint8_t motor_id, float speed_rps) {
    if (motor_id > 3) {
        throw std::invalid_argument("motor_id must be between 0 and 3");
    }
    if (!std::isfinite(speed_rps) ||
        std::abs(speed_rps) > config_.max_speed_rps) {
        throw std::invalid_argument("speed_rps outside the configured limit");
    }
    // Sub-command 0 = set speed: [0, motor, float rev/s].
    std::vector<std::uint8_t> payload{0, motor_id};
    append_le(payload, speed_rps);
    send_frame(kFunctionMotor, payload);
}

void Brick::stop_motor(std::uint8_t motor_id) {
    if (motor_id > 3) {
        throw std::invalid_argument("motor_id must be between 0 and 3");
    }
    send_frame(kFunctionMotor, {2, motor_id});   // sub-command 2 = stop
}

void Brick::stop_all() {
    for (std::uint8_t motor = 0; motor < 4; ++motor) {
        stop_motor(motor);
    }
}

// All MOTOR_EXT requests follow the same pattern: remember the sequence
// number of the last reply, send the request, then wait on
// motor_position_cv_ until the reader thread stores a newer reply for the
// same sub-command. The first payload byte is the sub-command:
// 0x00 capabilities, 0x01 move relative, 0x02 cancel, 0x03 zero,
// 0x04 status, 0x05 hold.
MotorPositionCapabilities Brick::motor_position_capabilities(
    std::chrono::milliseconds timeout) {
    std::uint64_t initial = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        initial = motor_position_capabilities_.sequence;
    }
    send_frame(kFunctionMotorExt, {0x00});
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (!motor_position_cv_.wait_for(lock, timeout, [&] {
            return motor_position_capabilities_.sequence != initial;
        })) {
        throw std::runtime_error(
            "the firmware did not answer the position capabilities query");
    }
    return motor_position_capabilities_;
}

MotorPositionStatus Brick::move_motor_relative(
    std::int32_t delta_ticks, float max_rps,
    std::uint16_t tolerance_ticks, std::uint16_t motion_timeout_ms,
    std::chrono::milliseconds reply_timeout) {
    if (delta_ticks == 0) {
        throw std::invalid_argument("delta_ticks cannot be zero");
    }
    if (!std::isfinite(max_rps) || max_rps <= 0.0F ||
        max_rps > config_.max_speed_rps) {
        throw std::invalid_argument("max_rps outside the configured limit");
    }
    if (tolerance_ticks == 0 || motion_timeout_ms < 100) {
        throw std::invalid_argument("invalid motion tolerance or timeout");
    }
    std::uint64_t initial = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        initial = motor_position_status_.sequence;
    }
    // [0x01, motor, int32 delta ticks, float max rev/s, uint16 tolerance,
    //  uint16 timeout ms]
    std::vector<std::uint8_t> payload{0x01, config_.position_motor_id};
    append_le(payload, delta_ticks);
    append_le(payload, max_rps);
    append_le(payload, tolerance_ticks);
    append_le(payload, motion_timeout_ms);
    send_frame(kFunctionMotorExt, payload);
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (!motor_position_cv_.wait_for(lock, reply_timeout, [&] {
            return motor_position_status_.sequence != initial &&
                   motor_position_status_.command == 0x01;
        })) {
        throw std::runtime_error("no acknowledgement of the position move");
    }
    return motor_position_status_;
}

MotorPositionStatus Brick::motor_position_status(
    std::chrono::milliseconds timeout) {
    std::uint64_t initial = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        initial = motor_position_status_.sequence;
    }
    send_frame(kFunctionMotorExt, {0x04, config_.position_motor_id});
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (!motor_position_cv_.wait_for(lock, timeout, [&] {
            return motor_position_status_.sequence != initial &&
                   motor_position_status_.command == 0x04;
        })) {
        throw std::runtime_error("no position status response");
    }
    return motor_position_status_;
}

MotorPositionStatus Brick::cancel_motor_position(
    std::chrono::milliseconds timeout) {
    std::uint64_t initial = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        initial = motor_position_status_.sequence;
    }
    send_frame(kFunctionMotorExt, {0x02, config_.position_motor_id});
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (!motor_position_cv_.wait_for(lock, timeout, [&] {
            return motor_position_status_.sequence != initial &&
                   motor_position_status_.command == 0x02;
        })) {
        throw std::runtime_error("no acknowledgement of the position cancel");
    }
    return motor_position_status_;
}

MotorPositionStatus Brick::zero_motor_position(
    std::chrono::milliseconds timeout) {
    std::uint64_t initial = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        initial = motor_position_status_.sequence;
    }
    send_frame(kFunctionMotorExt, {0x03, config_.position_motor_id});
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (!motor_position_cv_.wait_for(lock, timeout, [&] {
            return motor_position_status_.sequence != initial &&
                   motor_position_status_.command == 0x03;
        })) {
        throw std::runtime_error("no acknowledgement of the position zeroing");
    }
    return motor_position_status_;
}

MotorPositionStatus Brick::hold_motor_position(
    float max_rps, std::uint16_t tolerance_ticks, std::uint16_t max_pulse,
    std::chrono::milliseconds reply_timeout) {
    if (!std::isfinite(max_rps) || max_rps <= 0.0F ||
        max_rps > config_.max_speed_rps) {
        throw std::invalid_argument("max_rps outside the configured limit");
    }
    if (tolerance_ticks == 0 || max_pulse < 300 || max_pulse > 1000) {
        throw std::invalid_argument("invalid HOLD tolerance or PWM limit");
    }
    std::uint64_t initial = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        initial = motor_position_status_.sequence;
    }
    // [0x05, motor, float max rev/s, uint16 tolerance, uint16 max PWM pulse]
    std::vector<std::uint8_t> payload{0x05, config_.position_motor_id};
    append_le(payload, max_rps);
    append_le(payload, tolerance_ticks);
    append_le(payload, max_pulse);
    send_frame(kFunctionMotorExt, payload);
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (!motor_position_cv_.wait_for(lock, reply_timeout, [&] {
            return motor_position_status_.sequence != initial &&
                   motor_position_status_.command == 0x05;
        })) {
        throw std::runtime_error("no acknowledgement of the HOLD");
    }
    return motor_position_status_;
}

// Polls the status until the firmware reports the move as inactive.
MotorPositionStatus Brick::wait_motor_position(
    std::chrono::milliseconds timeout, std::chrono::milliseconds poll_period) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    MotorPositionStatus status;
    do {
        status = motor_position_status(std::chrono::milliseconds(800));
        if (!status.active) {
            return status;
        }
        std::this_thread::sleep_for(poll_period);
    } while (std::chrono::steady_clock::now() < deadline);
    throw std::runtime_error("timed out waiting for the M1 position");
}

void Brick::set_bus_servo_position(std::uint8_t servo_id,
                                   std::uint16_t position,
                                   std::uint16_t duration_ms) {
    if (servo_id > 253) {
        throw std::invalid_argument("servo_id must be between 0 and 253");
    }
    if (position > 1000) {
        throw std::invalid_argument("HX-35H position must be between 0 and 1000");
    }
    if (duration_ms < 20 || duration_ms > 30000) {
        throw std::invalid_argument("duration must be between 20 and 30000 ms");
    }
    // Sub-command 0x01 = move: [0x01, uint16 duration ms, servo count,
    // then per servo: id, uint16 position].
    std::vector<std::uint8_t> payload{0x01};
    append_le(payload, duration_ms);
    payload.push_back(1);  // number of servos
    payload.push_back(servo_id);
    append_le(payload, position);
    send_frame(kFunctionBusServo, payload);
}

void Brick::set_bus_servo_power(std::uint8_t servo_id, bool enabled) {
    if (servo_id > 253) {
        throw std::invalid_argument("servo_id must be between 0 and 253");
    }
    // 0x0C = torque on, 0x0B = torque off (unload).
    send_frame(kFunctionBusServo,
               {static_cast<std::uint8_t>(enabled ? 0x0C : 0x0B), servo_id});
}

std::uint8_t Brick::read_bus_servo_id(
    std::chrono::milliseconds timeout) const {
    std::uint64_t initial = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        initial = bus_servo_.sequence;
    }
    // Broadcast query (ID 0xFE) documented by Hiwonder. Only one bus servo
    // may be connected, otherwise the replies collide.
    const_cast<Brick *>(this)->send_frame(kFunctionBusServo, {0x12, 0xFE});
    std::unique_lock<std::mutex> lock(state_mutex_);
    const bool received = servo_cv_.wait_for(lock, timeout, [&] {
        return bus_servo_.sequence != initial && bus_servo_.valid;
    });
    if (!received) {
        throw std::runtime_error("timed out reading the bus servo ID");
    }
    return bus_servo_.id;
}

BusServoFeedback Brick::read_bus_servo_position(
    std::uint8_t servo_id, std::chrono::milliseconds timeout) const {
    if (servo_id > 253) {
        throw std::invalid_argument("servo_id must be between 0 and 253");
    }
    std::uint64_t initial = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        initial = bus_servo_.sequence;
    }
    // 0x05 = read position. send_frame() is not const (it locks write_mutex_),
    // but reading does not change the logical state of the Brick.
    const_cast<Brick *>(this)->send_frame(kFunctionBusServo, {0x05, servo_id});
    std::unique_lock<std::mutex> lock(state_mutex_);
    const bool received = servo_cv_.wait_for(lock, timeout, [&] {
        return bus_servo_.sequence != initial && bus_servo_.id == servo_id;
    });
    if (!received) {
        throw std::runtime_error("timed out reading the bus servo position");
    }
    return bus_servo_;
}

void Brick::set_steering(double normalized,
                         const SteeringCalibration &calibration) {
    if (!std::isfinite(normalized) || normalized < -1.0 || normalized > 1.0) {
        throw std::invalid_argument("normalized steering must be between -1 and 1");
    }
    if (calibration.left > 1000 || calibration.center > 1000 ||
        calibration.right > 1000) {
        throw std::invalid_argument("steering calibration outside 0..1000");
    }
    // Linear interpolation on each side of the centre: negative values move
    // toward `left`, positive toward `right`. The two halves may have
    // different lengths if the calibration is asymmetric.
    const double target = normalized < 0.0
        ? calibration.center + (-normalized) *
              (static_cast<double>(calibration.left) - calibration.center)
        : calibration.center + normalized *
              (static_cast<double>(calibration.right) - calibration.center);
    set_bus_servo_position(calibration.servo_id,
                           static_cast<std::uint16_t>(std::lround(target)),
                           calibration.duration_ms);
}

void Brick::center_steering(const SteeringCalibration &calibration) {
    set_steering(0.0, calibration);
}

ImuSample Brick::imu() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return imu_;
}

bool Brick::wait_for_imu(std::chrono::milliseconds timeout) const {
    std::unique_lock<std::mutex> lock(state_mutex_);
    const auto initial = imu_.sequence;
    return imu_cv_.wait_for(lock, timeout, [&] {
        return imu_.valid && imu_.sequence != initial;
    });
}

// Gyro bias estimation: average every new IMU sample received during
// `duration` while the robot stands still. The bias is subtracted from gyro_z
// when the yaw is integrated in process_frame().
void Brick::calibrate_gyro(std::chrono::milliseconds duration) {
    if (duration < std::chrono::milliseconds(500)) {
        throw std::invalid_argument("calibration must last at least 500 ms");
    }
    if (!wait_for_imu(std::chrono::seconds(2))) {
        throw std::runtime_error("no IMU telemetry received");
    }

    const auto deadline = std::chrono::steady_clock::now() + duration;
    std::array<double, 3> sum{{0.0, 0.0, 0.0}};
    std::size_t count = 0;
    std::uint64_t sequence = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        std::unique_lock<std::mutex> lock(state_mutex_);
        imu_cv_.wait_for(lock, std::chrono::milliseconds(100), [&] {
            return imu_.sequence != sequence;
        });
        if (!imu_.valid || imu_.sequence == sequence) {
            continue;
        }
        sequence = imu_.sequence;
        sum[0] += imu_.gyro_x_dps;
        sum[1] += imu_.gyro_y_dps;
        sum[2] += imu_.gyro_z_dps;
        ++count;
    }
    if (count < 10) {
        throw std::runtime_error("not enough samples to calibrate the IMU");
    }
    std::lock_guard<std::mutex> lock(state_mutex_);
    gyro_bias_[0] = sum[0] / static_cast<double>(count);
    gyro_bias_[1] = sum[1] / static_cast<double>(count);
    gyro_bias_[2] = sum[2] / static_cast<double>(count);
    // Restart integration from 0 so drift accumulated during the
    // calibration window is discarded.
    imu_.yaw_deg = 0.0;
    last_imu_time_ = std::chrono::steady_clock::now();
}

void Brick::reset_yaw(double yaw_deg) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    imu_.yaw_deg = yaw_deg;
    last_imu_time_ = std::chrono::steady_clock::now();
}

std::int64_t Brick::motor_degrees_to_ticks(
    double degrees, const EncoderCalibration &calibration) {
    if (!std::isfinite(degrees)) {
        throw std::invalid_argument("degrees must be finite");
    }
    if (calibration.ticks_per_output_revolution <= 0) {
        throw std::invalid_argument(
            "ticks_per_output_revolution must be positive");
    }
    return static_cast<std::int64_t>(std::llround(
        degrees * calibration.ticks_per_output_revolution / 360.0));
}

double Brick::motor_ticks_to_degrees(
    std::int64_t ticks, const EncoderCalibration &calibration) {
    if (calibration.ticks_per_output_revolution <= 0) {
        throw std::invalid_argument(
            "ticks_per_output_revolution must be positive");
    }
    return static_cast<double>(ticks) * 360.0 /
           calibration.ticks_per_output_revolution;
}

ImuSample Brick::decode_imu_payload(const std::uint8_t *payload) {
    // ONE_G from QMI8658.c: the firmware multiplies by this value, so
    // dividing by the same value gives back exactly the reading in g.
    constexpr double kFirmwareOneG = 9.807;
    constexpr double kRadiansToDegrees = 57.29577951308232;
    ImuSample s;
    s.accel_x_g = static_cast<float>(read_float_le(payload) / kFirmwareOneG);
    s.accel_y_g = static_cast<float>(read_float_le(payload + 4) / kFirmwareOneG);
    s.accel_z_g = static_cast<float>(read_float_le(payload + 8) / kFirmwareOneG);
    s.gyro_x_dps = static_cast<float>(read_float_le(payload + 12) * kRadiansToDegrees);
    s.gyro_y_dps = static_cast<float>(read_float_le(payload + 16) * kRadiansToDegrees);
    s.gyro_z_dps = static_cast<float>(read_float_le(payload + 20) * kRadiansToDegrees);
    // Tilt from the gravity vector (valid only when the robot is not
    // accelerating strongly).
    s.roll_deg = static_cast<float>(
        std::atan2(s.accel_y_g, s.accel_z_g) * kRadiansToDegrees);
    s.pitch_deg = static_cast<float>(
        std::atan2(-s.accel_x_g, std::hypot(s.accel_y_g, s.accel_z_g)) *
        kRadiansToDegrees);
    return s;
}

// Bitwise CRC-8/MAXIM (Dallas 1-Wire): reflected polynomial 0x8C, init 0.
std::uint8_t Brick::crc8_maxim(const std::uint8_t *data,
                               std::size_t size) noexcept {
    std::uint8_t crc = 0;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1U) ? static_cast<std::uint8_t>((crc >> 1U) ^ 0x8CU)
                             : static_cast<std::uint8_t>(crc >> 1U);
        }
    }
    return crc;
}

std::vector<std::uint8_t>
Brick::encode_frame(std::uint8_t function,
                    const std::vector<std::uint8_t> &payload) {
    if (payload.size() > 255) {
        throw std::invalid_argument("payload too large");
    }
    std::vector<std::uint8_t> frame;
    frame.reserve(payload.size() + 5);
    frame.push_back(kHeader1);
    frame.push_back(kHeader2);
    frame.push_back(function);
    frame.push_back(static_cast<std::uint8_t>(payload.size()));
    frame.insert(frame.end(), payload.begin(), payload.end());
    // The CRC covers function, length and payload (not the 0xAA 0x55 header).
    frame.push_back(crc8_maxim(frame.data() + 2, frame.size() - 2));
    return frame;
}

// Writes one complete frame. write_mutex_ keeps frames from different
// threads from interleaving; the port is non-blocking, so partial writes and
// EAGAIN are retried until the whole frame is out.
void Brick::send_frame(std::uint8_t function,
                       const std::vector<std::uint8_t> &payload) {
    std::lock_guard<std::mutex> write_lock(write_mutex_);
    int fd = -1;
    {
        std::lock_guard<std::mutex> state_lock(state_mutex_);
        fd = fd_;
    }
    if (fd < 0) {
        throw std::runtime_error("the controller board is not open");
    }
    const auto frame = encode_frame(function, payload);
    std::size_t offset = 0;
    while (offset < frame.size()) {
        const auto written = ::write(fd, frame.data() + offset,
                                     frame.size() - offset);
        if (written > 0) {
            offset += static_cast<std::size_t>(written);
        } else if (written < 0 && errno != EINTR && errno != EAGAIN) {
            throw system_error("error writing to the controller board");
        }
    }
    tcdrain(fd);   // wait until the bytes have physically left the UART
}

// Reader thread body (started by open()). Waits up to 100 ms for data with
// select() so it notices running_ = false promptly, and feeds every chunk to
// the frame parser.
void Brick::reader_loop() noexcept {
    std::array<std::uint8_t, 4096> buffer{};
    for (;;) {
        int fd = -1;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (!running_) {
                return;
            }
            fd = fd_;
        }
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(fd, &read_set);
        timeval timeout{0, 100000};
        const int ready = select(fd + 1, &read_set, nullptr, nullptr, &timeout);
        if (ready <= 0) {
            continue;
        }
        const auto n = ::read(fd, buffer.data(), buffer.size());
        if (n > 0) {
            consume_bytes(buffer.data(), static_cast<std::size_t>(n));
        }
    }
}

// Stream parser: appends the new bytes, then extracts every complete frame.
// Bytes before a header are discarded (resynchronisation after noise), and a
// frame with a bad CRC is dropped as a whole.
void Brick::consume_bytes(const std::uint8_t *data, std::size_t size) {
    rx_buffer_.insert(rx_buffer_.end(), data, data + size);
    for (;;) {
        const auto header = std::search(rx_buffer_.begin(), rx_buffer_.end(),
                                        kHeader.begin(), kHeader.end());
        if (header == rx_buffer_.end()) {
            // Keep the last byte: it may be the 0xAA of a header split
            // between two reads.
            if (rx_buffer_.size() > 1) {
                rx_buffer_.erase(rx_buffer_.begin(), rx_buffer_.end() - 1);
            }
            return;
        }
        rx_buffer_.erase(rx_buffer_.begin(), header);
        if (rx_buffer_.size() < 5) {
            return;   // header, function, length and CRC not all here yet
        }
        const std::size_t payload_size = rx_buffer_[3];
        const std::size_t total = payload_size + 5;
        if (rx_buffer_.size() < total) {
            return;
        }
        const auto expected = crc8_maxim(rx_buffer_.data() + 2,
                                         payload_size + 2);
        if (expected == rx_buffer_[total - 1]) {
            process_frame(rx_buffer_[2],
                          {rx_buffer_.begin() + 4,
                           rx_buffer_.begin() + 4 + payload_size});
        }
        rx_buffer_.erase(rx_buffer_.begin(), rx_buffer_.begin() + total);
    }
}

// Dispatches one valid frame (reader thread). Each reply type is stored under
// state_mutex_ with an incremented sequence number and its condition variable
// is notified, which wakes the request that is waiting for it.
void Brick::process_frame(std::uint8_t function,
                          const std::vector<std::uint8_t> &payload) {
    // MOTOR_EXT capabilities: [0x00, version, motor mask, -, uint32 ticks/rev].
    if (function == kFunctionMotorExt && payload.size() == 8 &&
        payload[0] == 0x00) {
        MotorPositionCapabilities capabilities;
        capabilities.protocol_version = payload[1];
        capabilities.motor_mask = payload[2];
        capabilities.ticks_per_revolution =
            read_le<std::uint32_t>(payload.data() + 4);
        std::lock_guard<std::mutex> lock(state_mutex_);
        capabilities.sequence = motor_position_capabilities_.sequence + 1;
        capabilities.valid = true;
        motor_position_capabilities_ = capabilities;
        motor_position_cv_.notify_all();
        return;
    }
    // MOTOR_EXT status (reply to sub-commands 0x01..0x05): [cmd, motor, state,
    // active, int32 position, int32 target, int32 error, float rev/s].
    if (function == kFunctionMotorExt && payload.size() == 20 &&
        payload[0] >= 0x01 && payload[0] <= 0x05) {
        MotorPositionStatus status;
        status.command = payload[0];
        status.motor_id = payload[1];
        status.state = static_cast<MotorPositionState>(payload[2]);
        status.active = payload[3] != 0;
        status.position_ticks = read_le<std::int32_t>(payload.data() + 4);
        status.target_ticks = read_le<std::int32_t>(payload.data() + 8);
        status.error_ticks = read_le<std::int32_t>(payload.data() + 12);
        status.speed_rps = read_float_le(payload.data() + 16);
        std::lock_guard<std::mutex> lock(state_mutex_);
        status.sequence = motor_position_status_.sequence + 1;
        status.valid = true;
        motor_position_status_ = status;
        motor_position_cv_.notify_all();
        return;
    }
    // Bus servo ID reply: [0xFE, 0x12, status (0 = ok), id].
    if (function == kFunctionBusServo && payload.size() == 4 &&
        payload[0] == 0xFE && payload[1] == 0x12 &&
        static_cast<std::int8_t>(payload[2]) == 0) {
        BusServoFeedback feedback;
        feedback.id = payload[3];
        std::lock_guard<std::mutex> lock(state_mutex_);
        feedback.sequence = bus_servo_.sequence + 1;
        feedback.valid = true;
        bus_servo_ = feedback;
        servo_cv_.notify_all();
        return;
    }
    // Bus servo position reply: [id, 0x05, status (0 = ok), int16 position].
    if (function == kFunctionBusServo && payload.size() == 5 &&
        payload[1] == 0x05 && static_cast<std::int8_t>(payload[2]) == 0) {
        BusServoFeedback feedback;
        feedback.id = payload[0];
        std::memcpy(&feedback.position, payload.data() + 3,
                    sizeof(feedback.position));
        std::lock_guard<std::mutex> lock(state_mutex_);
        feedback.sequence = bus_servo_.sequence + 1;
        feedback.valid = true;
        bus_servo_ = feedback;
        servo_cv_.notify_all();
        return;
    }
    // Anything else except a 24-byte IMU frame is ignored.
    if (function != kFunctionImu || payload.size() != 24) {
        return;
    }
    ImuSample next = decode_imu_payload(payload.data());

    // Yaw integration with host timestamps: yaw += (gyro_z - bias) * dt. A gap
    // of 0.1 s or more (twice the 20 Hz period, e.g. lost frames) is skipped
    // instead of integrated, to avoid a jump; the very first sample only sets
    // the timestamp.

    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(state_mutex_);
    next.yaw_deg = imu_.yaw_deg;
    if (last_imu_time_.time_since_epoch().count() != 0) {
        const double dt = std::chrono::duration<double>(now - last_imu_time_).count();
        if (dt > 0.0 && dt < 0.1) {
            next.yaw_deg += (static_cast<double>(next.gyro_z_dps) -
                             gyro_bias_[2]) * dt;
        }
    }
    last_imu_time_ = now;
    next.sequence = imu_.sequence + 1;
    next.valid = true;
    imu_ = next;
    imu_cv_.notify_all();
}

}  // namespace hiwonder
