// hiwonder::Brick for the digital twin: the same class the robot uses
// (src/ondevice/hiwonder/include/hiwonder_brick/hiwonder_brick.hpp), but each
// call goes to the emulated board in the CoppeliaSim scene instead of the
// serial port. It is linked instead of hiwonder_brick.cpp; spike_compat.cpp,
// spike_hiwonder.cpp and the mission call it exactly as on the robot.
//
// What the scene emulates (scene/twin_car.lua) is the firmware pos-v8 of the
// board: speed loop and position loop (MOVE_RELATIVE, HOLD, CANCEL) of the drive
// motor M4 with its 1320-tick encoder, the HX-12H bus servo with its travel
// time, and the IMU at 20 Hz. As on the robot, the yaw is a gyro integral with
// drift, and reset_yaw()/calibrate_gyro() act on the host side.
#include "hiwonder_brick/hiwonder_brick.hpp"
#include "sim_link.hpp"

#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <mutex>
#include <thread>

namespace {

struct BoardState {
    double t = 0.0;
    long position = 0;
    double rps = 0.0;
    int pstate = 0;
    bool active = false;
    long target = 0;
    long error = 0;
    unsigned long imu_seq = 0;
    double yaw = 0.0;
    double gyro_z = 0.0;
    int servo = 750;
    bool power = true;
    bool button = false;
};

wro_sim::SimChannel &Channel() {
    static wro_sim::SimChannel channel(wro_sim::kPortBoard);
    return channel;
}

std::mutex g_yaw_mutex;
double g_yaw_offset = 0.0;  // host-side yaw zero (reset_yaw / calibrate_gyro)

BoardState Call(const std::string &command) {
    const std::string reply = Channel().Request(command);
    BoardState s;
    int active = 0, power = 0, button = 0;
    const int n = std::sscanf(reply.c_str(), "%lf %ld %lf %d %d %ld %ld %lu %lf %lf %d %d %d", &s.t,
                              &s.position, &s.rps, &s.pstate, &active, &s.target, &s.error,
                              &s.imu_seq, &s.yaw, &s.gyro_z, &s.servo, &power, &button);
    if (n != 13) throw std::runtime_error("twin board: unexpected reply '" + reply + "'");
    s.active = active != 0;
    s.power = power != 0;
    s.button = button != 0;
    return s;
}

hiwonder::MotorPositionStatus ToStatus(const BoardState &s, std::uint8_t motor_id) {
    hiwonder::MotorPositionStatus st;
    st.motor_id = motor_id;
    st.state = static_cast<hiwonder::MotorPositionState>(s.pstate);
    st.active = s.active;
    st.position_ticks = static_cast<std::int32_t>(s.position);
    st.target_ticks = static_cast<std::int32_t>(s.target);
    st.error_ticks = static_cast<std::int32_t>(s.error);
    st.speed_rps = static_cast<float>(s.rps);
    st.sequence = s.imu_seq;
    st.valid = true;
    return st;
}

std::string Num(double v) {
    char text[48];
    std::snprintf(text, sizeof(text), "%.6g", v);
    return text;
}

}  // namespace

namespace wro_sim {
// Used by the start button stand-in (sim_gpiod.cpp).
bool Board_Button_Pressed() { return Call("st").button; }
void Board_Led(bool on) { Call(on ? "led 1" : "led 0"); }
void Board_Press_Button() { Call("press"); }
double Board_Sim_Time() { return Call("st").t; }
// Used by the emergency stop stand-in.
void Board_Emergency_Stop(bool release_steering) {
    Call("stop");
    if (release_steering) Call("pwr 0");
}
}  // namespace wro_sim

namespace hiwonder {

Brick::Brick(Config config) : config_(config) {}
Brick::~Brick() { close(); }

std::string Brick::discover_device(const std::string &) { return "sim://coppeliasim"; }

void Brick::open() {
    Call("hello");
    device_ = "sim://coppeliasim:" + std::to_string(wro_sim::kPortBoard);
    fd_ = 1;
    running_ = true;
}

void Brick::close() noexcept {
    if (fd_ >= 0) {
        try {
            Call("stop");
        } catch (...) {
        }
    }
    fd_ = -1;
    running_ = false;
}

bool Brick::is_open() const noexcept { return fd_ >= 0; }

void Brick::set_motor_speed(std::uint8_t, float speed_rps) {
    const float limit = config_.max_speed_rps;
    const float rps = std::max(-limit, std::min(limit, speed_rps));
    Call("sp " + Num(rps));
}

void Brick::stop_motor(std::uint8_t) { Call("stop"); }
void Brick::stop_all() { Call("stop"); }

MotorPositionCapabilities Brick::motor_position_capabilities(std::chrono::milliseconds) {
    MotorPositionCapabilities caps;
    caps.protocol_version = 3;
    caps.motor_mask = static_cast<std::uint8_t>(1U << config_.position_motor_id);
    caps.ticks_per_revolution = 1320;
    caps.valid = true;
    return caps;
}

MotorPositionStatus Brick::move_motor_relative(std::int32_t delta_ticks, float max_rps,
                                               std::uint16_t tolerance_ticks,
                                               std::uint16_t motion_timeout_ms,
                                               std::chrono::milliseconds) {
    if (delta_ticks == 0 || !(max_rps > 0.0F)) {
        throw std::invalid_argument("move_motor_relative: delta 0 or max_rps <= 0");
    }
    return ToStatus(Call("mv " + std::to_string(delta_ticks) + " " + Num(max_rps) + " " +
                         std::to_string(tolerance_ticks) + " " + std::to_string(motion_timeout_ms)),
                    config_.position_motor_id);
}

MotorPositionStatus Brick::motor_position_status(std::chrono::milliseconds) {
    return ToStatus(Call("st"), config_.position_motor_id);
}

MotorPositionStatus Brick::cancel_motor_position(std::chrono::milliseconds) {
    return ToStatus(Call("cancel"), config_.position_motor_id);
}

MotorPositionStatus Brick::zero_motor_position(std::chrono::milliseconds) {
    return ToStatus(Call("zero"), config_.position_motor_id);
}

MotorPositionStatus Brick::wait_motor_position(std::chrono::milliseconds timeout,
                                               std::chrono::milliseconds poll_period) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;) {
        auto status = motor_position_status();
        if (!status.active || std::chrono::steady_clock::now() >= deadline) return status;
        std::this_thread::sleep_for(poll_period);
    }
}

MotorPositionStatus Brick::hold_motor_position(float max_rps, std::uint16_t tolerance_ticks,
                                               std::uint16_t, std::chrono::milliseconds) {
    return ToStatus(Call("hold " + Num(max_rps) + " " + std::to_string(tolerance_ticks)),
                    config_.position_motor_id);
}

void Brick::set_bus_servo_position(std::uint8_t, std::uint16_t position, std::uint16_t duration_ms) {
    Call("servo " + std::to_string(position) + " " + std::to_string(duration_ms));
}

void Brick::set_bus_servo_power(std::uint8_t, bool enabled) { Call(enabled ? "pwr 1" : "pwr 0"); }

std::uint8_t Brick::read_bus_servo_id(std::chrono::milliseconds) const { return 1; }

BusServoFeedback Brick::read_bus_servo_position(std::uint8_t servo_id,
                                                std::chrono::milliseconds) const {
    static std::uint64_t sequence = 0;
    const BoardState s = Call("st");
    BusServoFeedback f;
    f.id = servo_id;
    f.position = static_cast<std::int16_t>(s.servo);
    f.sequence = ++sequence;
    f.valid = true;
    return f;
}

// Same mapping as the real Brick::set_steering (hiwonder_brick.cpp).
void Brick::set_steering(double normalized, const SteeringCalibration &calibration) {
    if (!std::isfinite(normalized) || normalized < -1.0 || normalized > 1.0) {
        throw std::invalid_argument("normalized steering must be between -1 and 1");
    }
    if (calibration.left > 1000 || calibration.center > 1000 || calibration.right > 1000) {
        throw std::invalid_argument("steering calibration outside 0..1000");
    }
    const double target = normalized < 0.0
        ? calibration.center + (-normalized) * (static_cast<double>(calibration.left) - calibration.center)
        : calibration.center + normalized * (static_cast<double>(calibration.right) - calibration.center);
    set_bus_servo_position(calibration.servo_id, static_cast<std::uint16_t>(std::lround(target)),
                           calibration.duration_ms);
}

void Brick::center_steering(const SteeringCalibration &calibration) { set_steering(0.0, calibration); }

ImuSample Brick::imu() const {
    const BoardState s = Call("st");
    ImuSample sample;
    sample.gyro_z_dps = static_cast<float>(s.gyro_z);
    sample.accel_z_g = 1.0F;
    {
        std::lock_guard<std::mutex> lock(g_yaw_mutex);
        sample.yaw_deg = s.yaw - g_yaw_offset;
    }
    sample.sequence = s.imu_seq;
    sample.valid = true;
    return sample;
}

// One request: the scene keeps the reply until the next IMU sample (20 Hz)
// newer than the last one seen here, or until the timeout.
bool Brick::wait_for_imu(std::chrono::milliseconds timeout) const {
    static std::mutex seq_mutex;
    static unsigned long last_seen = 0;
    unsigned long since;
    {
        std::lock_guard<std::mutex> lock(seq_mutex);
        since = last_seen;
    }
    const BoardState s = Call("waitimu " + std::to_string(since) + " " +
                              std::to_string(timeout.count()));
    std::lock_guard<std::mutex> lock(seq_mutex);
    const bool fresh = s.imu_seq != since;
    last_seen = s.imu_seq;
    return fresh;
}

// As on the board: averages the gyro for `duration` with the robot still and
// restarts the yaw from 0. The scene draws the residual bias that remains
// after the calibration (about 1 deg/min, as measured on the robot).
void Brick::calibrate_gyro(std::chrono::milliseconds duration) {
    Call("calib " + std::to_string(duration.count()));
    std::this_thread::sleep_for(duration);
    const BoardState s = Call("st");
    std::lock_guard<std::mutex> lock(g_yaw_mutex);
    g_yaw_offset = s.yaw;
}

void Brick::reset_yaw(double yaw_deg) {
    const BoardState s = Call("st");
    std::lock_guard<std::mutex> lock(g_yaw_mutex);
    g_yaw_offset = s.yaw - yaw_deg;
}

std::int64_t Brick::motor_degrees_to_ticks(double degrees, const EncoderCalibration &calibration) {
    return std::llround(degrees * static_cast<double>(calibration.ticks_per_output_revolution) / 360.0);
}

double Brick::motor_ticks_to_degrees(std::int64_t ticks, const EncoderCalibration &calibration) {
    return static_cast<double>(ticks) * 360.0 /
           static_cast<double>(calibration.ticks_per_output_revolution);
}

ImuSample Brick::decode_imu_payload(const std::uint8_t *) { return ImuSample{}; }

std::uint8_t Brick::crc8_maxim(const std::uint8_t *data, std::size_t size) noexcept {
    std::uint8_t crc = 0;
    for (std::size_t i = 0; i < size; ++i) {
        std::uint8_t byte = data[i];
        for (int b = 0; b < 8; ++b) {
            const std::uint8_t mix = (crc ^ byte) & 0x01U;
            crc >>= 1;
            if (mix) crc ^= 0x8CU;
            byte >>= 1;
        }
    }
    return crc;
}

std::vector<std::uint8_t> Brick::encode_frame(std::uint8_t function,
                                              const std::vector<std::uint8_t> &payload) {
    std::vector<std::uint8_t> frame{0xAA, 0x55, function, static_cast<std::uint8_t>(payload.size())};
    frame.insert(frame.end(), payload.begin(), payload.end());
    frame.push_back(crc8_maxim(frame.data() + 2, frame.size() - 2));
    return frame;
}

}  // namespace hiwonder
