#include "hiwonder_brick/hiwonder_brick.hpp"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/select.h>
#include <termios.h>
#include <type_traits>
#include <unistd.h>
#include <utility>

namespace hiwonder {
namespace {

constexpr std::uint8_t kHeader1 = 0xAA;
constexpr std::uint8_t kHeader2 = 0x55;
constexpr std::uint8_t kFunctionMotor = 3;
constexpr std::uint8_t kFunctionMotorExt = 0x0B;
constexpr std::uint8_t kFunctionBusServo = 5;
constexpr std::uint8_t kFunctionImu = 7;
constexpr std::array<std::uint8_t, 2> kHeader{{kHeader1, kHeader2}};

template <typename T>
void append_le(std::vector<std::uint8_t> &out, const T &value) {
    static_assert(std::is_trivially_copyable<T>::value,
                  "append_le requires a trivially copyable type");
    const auto *bytes = reinterpret_cast<const std::uint8_t *>(&value);
    out.insert(out.end(), bytes, bytes + sizeof(T));
}

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

std::runtime_error system_error(const std::string &what) {
    return std::runtime_error(what + ": " + std::strerror(errno));
}

}  // namespace

Brick::Brick(Config config) : config_(std::move(config)) {}

Brick::~Brick() { close(); }

void Brick::open() {
    if (is_open()) {
        return;
    }

    const int fd = ::open(config_.device.c_str(),
                          O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        throw system_error("no se pudo abrir " + config_.device);
    }

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
        throw system_error("no se pudo configurar el puerto a 1000000 baud");
    }
    tcflush(fd, TCIFLUSH);

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        fd_ = fd;
        running_ = true;
        imu_ = {};
        last_imu_time_ = {};
        rx_buffer_.clear();
    }
    reader_ = std::thread(&Brick::reader_loop, this);
}

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
        throw std::invalid_argument("motor_id debe estar entre 0 y 3");
    }
    if (!std::isfinite(speed_rps) ||
        std::abs(speed_rps) > config_.max_speed_rps) {
        throw std::invalid_argument("speed_rps fuera del límite configurado");
    }
    std::vector<std::uint8_t> payload{0, motor_id};
    append_le(payload, speed_rps);
    send_frame(kFunctionMotor, payload);
}

void Brick::stop_motor(std::uint8_t motor_id) {
    if (motor_id > 3) {
        throw std::invalid_argument("motor_id debe estar entre 0 y 3");
    }
    send_frame(kFunctionMotor, {2, motor_id});
}

void Brick::stop_all() {
    for (std::uint8_t motor = 0; motor < 4; ++motor) {
        stop_motor(motor);
    }
}

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
            "el firmware no respondió a capacidades de posición M1");
    }
    return motor_position_capabilities_;
}

MotorPositionStatus Brick::move_motor_relative(
    std::int32_t delta_ticks, float max_rps,
    std::uint16_t tolerance_ticks, std::uint16_t motion_timeout_ms,
    std::chrono::milliseconds reply_timeout) {
    if (delta_ticks == 0) {
        throw std::invalid_argument("delta_ticks no puede ser cero");
    }
    if (!std::isfinite(max_rps) || max_rps <= 0.0F ||
        max_rps > config_.max_speed_rps) {
        throw std::invalid_argument("max_rps fuera del límite configurado");
    }
    if (tolerance_ticks == 0 || motion_timeout_ms < 100) {
        throw std::invalid_argument("tolerancia o timeout de movimiento inválido");
    }
    std::uint64_t initial = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        initial = motor_position_status_.sequence;
    }
    std::vector<std::uint8_t> payload{0x01, 0x00};
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
        throw std::runtime_error("sin confirmación del movimiento M1");
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
    send_frame(kFunctionMotorExt, {0x04, 0x00});
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (!motor_position_cv_.wait_for(lock, timeout, [&] {
            return motor_position_status_.sequence != initial &&
                   motor_position_status_.command == 0x04;
        })) {
        throw std::runtime_error("sin respuesta de estado de posición M1");
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
    send_frame(kFunctionMotorExt, {0x02, 0x00});
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (!motor_position_cv_.wait_for(lock, timeout, [&] {
            return motor_position_status_.sequence != initial &&
                   motor_position_status_.command == 0x02;
        })) {
        throw std::runtime_error("sin confirmación de cancelación M1");
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
    send_frame(kFunctionMotorExt, {0x03, 0x00});
    std::unique_lock<std::mutex> lock(state_mutex_);
    if (!motor_position_cv_.wait_for(lock, timeout, [&] {
            return motor_position_status_.sequence != initial &&
                   motor_position_status_.command == 0x03;
        })) {
        throw std::runtime_error("sin confirmación de cero M1");
    }
    return motor_position_status_;
}

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
    throw std::runtime_error("tiempo agotado esperando posición M1");
}

void Brick::set_bus_servo_position(std::uint8_t servo_id,
                                   std::uint16_t position,
                                   std::uint16_t duration_ms) {
    if (servo_id > 253) {
        throw std::invalid_argument("servo_id debe estar entre 0 y 253");
    }
    if (position > 1000) {
        throw std::invalid_argument("posición HX-12H debe estar entre 0 y 1000");
    }
    if (duration_ms < 20 || duration_ms > 30000) {
        throw std::invalid_argument("duración debe estar entre 20 y 30000 ms");
    }
    std::vector<std::uint8_t> payload{0x01};
    append_le(payload, duration_ms);
    payload.push_back(1);  // cantidad de servos
    payload.push_back(servo_id);
    append_le(payload, position);
    send_frame(kFunctionBusServo, payload);
}

void Brick::set_bus_servo_power(std::uint8_t servo_id, bool enabled) {
    if (servo_id > 253) {
        throw std::invalid_argument("servo_id debe estar entre 0 y 253");
    }
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
    // Consulta broadcast documentada por Hiwonder. Debe conectarse un solo
    // bus-servo para evitar colisiones de respuestas.
    const_cast<Brick *>(this)->send_frame(kFunctionBusServo, {0x12, 0xFE});
    std::unique_lock<std::mutex> lock(state_mutex_);
    const bool received = servo_cv_.wait_for(lock, timeout, [&] {
        return bus_servo_.sequence != initial && bus_servo_.valid;
    });
    if (!received) {
        throw std::runtime_error("tiempo agotado leyendo ID del bus servo");
    }
    return bus_servo_.id;
}

BusServoFeedback Brick::read_bus_servo_position(
    std::uint8_t servo_id, std::chrono::milliseconds timeout) const {
    if (servo_id > 253) {
        throw std::invalid_argument("servo_id debe estar entre 0 y 253");
    }
    std::uint64_t initial = 0;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        initial = bus_servo_.sequence;
    }
    const_cast<Brick *>(this)->send_frame(kFunctionBusServo, {0x05, servo_id});
    std::unique_lock<std::mutex> lock(state_mutex_);
    const bool received = servo_cv_.wait_for(lock, timeout, [&] {
        return bus_servo_.sequence != initial && bus_servo_.id == servo_id;
    });
    if (!received) {
        throw std::runtime_error("tiempo agotado leyendo posición del bus servo");
    }
    return bus_servo_;
}

void Brick::set_steering(double normalized,
                         const SteeringCalibration &calibration) {
    if (!std::isfinite(normalized) || normalized < -1.0 || normalized > 1.0) {
        throw std::invalid_argument("dirección normalizada debe estar entre -1 y 1");
    }
    if (calibration.left > 1000 || calibration.center > 1000 ||
        calibration.right > 1000) {
        throw std::invalid_argument("calibración de dirección fuera de 0..1000");
    }
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

void Brick::calibrate_gyro(std::chrono::milliseconds duration) {
    if (duration < std::chrono::milliseconds(500)) {
        throw std::invalid_argument("la calibración debe durar al menos 500 ms");
    }
    if (!wait_for_imu(std::chrono::seconds(2))) {
        throw std::runtime_error("no se recibió telemetría IMU");
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
        throw std::runtime_error("muestras insuficientes para calibrar el IMU");
    }
    std::lock_guard<std::mutex> lock(state_mutex_);
    gyro_bias_[0] = sum[0] / static_cast<double>(count);
    gyro_bias_[1] = sum[1] / static_cast<double>(count);
    gyro_bias_[2] = sum[2] / static_cast<double>(count);
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
        throw std::invalid_argument("degrees debe ser finito");
    }
    if (calibration.ticks_per_output_revolution <= 0) {
        throw std::invalid_argument(
            "ticks_per_output_revolution debe ser positivo");
    }
    return static_cast<std::int64_t>(std::llround(
        degrees * calibration.ticks_per_output_revolution / 360.0));
}

double Brick::motor_ticks_to_degrees(
    std::int64_t ticks, const EncoderCalibration &calibration) {
    if (calibration.ticks_per_output_revolution <= 0) {
        throw std::invalid_argument(
            "ticks_per_output_revolution debe ser positivo");
    }
    return static_cast<double>(ticks) * 360.0 /
           calibration.ticks_per_output_revolution;
}

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
        throw std::invalid_argument("payload demasiado grande");
    }
    std::vector<std::uint8_t> frame;
    frame.reserve(payload.size() + 5);
    frame.push_back(kHeader1);
    frame.push_back(kHeader2);
    frame.push_back(function);
    frame.push_back(static_cast<std::uint8_t>(payload.size()));
    frame.insert(frame.end(), payload.begin(), payload.end());
    frame.push_back(crc8_maxim(frame.data() + 2, frame.size() - 2));
    return frame;
}

void Brick::send_frame(std::uint8_t function,
                       const std::vector<std::uint8_t> &payload) {
    std::lock_guard<std::mutex> write_lock(write_mutex_);
    int fd = -1;
    {
        std::lock_guard<std::mutex> state_lock(state_mutex_);
        fd = fd_;
    }
    if (fd < 0) {
        throw std::runtime_error("la controladora no está abierta");
    }
    const auto frame = encode_frame(function, payload);
    std::size_t offset = 0;
    while (offset < frame.size()) {
        const auto written = ::write(fd, frame.data() + offset,
                                     frame.size() - offset);
        if (written > 0) {
            offset += static_cast<std::size_t>(written);
        } else if (written < 0 && errno != EINTR && errno != EAGAIN) {
            throw system_error("error escribiendo a la controladora");
        }
    }
    tcdrain(fd);
}

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

void Brick::consume_bytes(const std::uint8_t *data, std::size_t size) {
    rx_buffer_.insert(rx_buffer_.end(), data, data + size);
    for (;;) {
        const auto header = std::search(rx_buffer_.begin(), rx_buffer_.end(),
                                        kHeader.begin(), kHeader.end());
        if (header == rx_buffer_.end()) {
            if (rx_buffer_.size() > 1) {
                rx_buffer_.erase(rx_buffer_.begin(), rx_buffer_.end() - 1);
            }
            return;
        }
        rx_buffer_.erase(rx_buffer_.begin(), header);
        if (rx_buffer_.size() < 5) {
            return;
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

void Brick::process_frame(std::uint8_t function,
                          const std::vector<std::uint8_t> &payload) {
    if (function == kFunctionMotorExt && payload.size() == 8 &&
        payload[0] == 0x00) {
        MotorPositionCapabilities capabilities;
        capabilities.protocol_version = payload[1];
        capabilities.motor_mask = payload[2];
        capabilities.ticks_per_revolution_m1 =
            read_le<std::uint32_t>(payload.data() + 4);
        std::lock_guard<std::mutex> lock(state_mutex_);
        capabilities.sequence = motor_position_capabilities_.sequence + 1;
        capabilities.valid = true;
        motor_position_capabilities_ = capabilities;
        motor_position_cv_.notify_all();
        return;
    }
    if (function == kFunctionMotorExt && payload.size() == 20 &&
        payload[0] >= 0x01 && payload[0] <= 0x04) {
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
    if (function != kFunctionImu || payload.size() != 24) {
        return;
    }
    ImuSample next;
    next.accel_x_g = read_float_le(payload.data());
    next.accel_y_g = read_float_le(payload.data() + 4);
    next.accel_z_g = read_float_le(payload.data() + 8);
    next.gyro_x_dps = read_float_le(payload.data() + 12);
    next.gyro_y_dps = read_float_le(payload.data() + 16);
    next.gyro_z_dps = read_float_le(payload.data() + 20);
    constexpr double kRadiansToDegrees = 57.29577951308232;
    next.roll_deg = static_cast<float>(
        std::atan2(next.accel_y_g, next.accel_z_g) * kRadiansToDegrees);
    next.pitch_deg = static_cast<float>(
        std::atan2(-next.accel_x_g,
                   std::hypot(next.accel_y_g, next.accel_z_g)) *
        kRadiansToDegrees);

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
