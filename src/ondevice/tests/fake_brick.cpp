// Fake implementation of hiwonder::Brick and Parada_Emergencia_Instalar for
// the offline tests (linked instead of hiwonder_brick.cpp and
// parada_emergencia.cpp). No hardware is touched; see fake_brick.h.

#include "fake_brick.h"
#include "../parada_emergencia.h"

#include <string>

namespace fake {

namespace {
Estado g_estado;
std::uint64_t g_secuencia = 0;   // IMU sample sequence number
// Degrees of yaw per IMU sample with steering ±1 and 1 rev/s.
constexpr double kGradosPorPaso = 2.0;
}  // namespace

Estado &estado() { return g_estado; }
void reiniciar() { g_estado = Estado{}; }

}  // namespace fake

// The real one opens the port and installs signal handlers: here it is only
// recorded.
void Parada_Emergencia_Instalar(const char *dispositivo, std::uint8_t servo, bool soltar) {
    fake::estado().dispositivo_parada = dispositivo != nullptr ? dispositivo : "";
    fake::estado().eventos.push_back("parada_emergencia " + std::to_string(servo) +
                                     (soltar ? " soltar" : " sostener"));
}

namespace hiwonder {

using fake::estado;

Brick::Brick(Config config) : config_(config) {
    estado().dispositivo_config = config_.device;
}
Brick::~Brick() { close(); }

void Brick::open() {
    fd_ = 1;
    // Like the real one: without Config::device, the port is discovered.
    device_ = config_.device.empty() ? fake::kDispositivoDescubierto : config_.device;
    estado().eventos.push_back("open");
}
void Brick::close() noexcept { fd_ = -1; }
bool Brick::is_open() const noexcept { return fd_ >= 0; }

void Brick::set_motor_speed(std::uint8_t, float speed_rps) {
    estado().ultima_velocidad_rps = speed_rps;
}
void Brick::stop_all() {
    estado().ultima_velocidad_rps = 0.0F;
    ++estado().paradas;
}

MotorPositionCapabilities Brick::motor_position_capabilities(std::chrono::milliseconds) {
    MotorPositionCapabilities caps;
    caps.valid = true;
    caps.motor_mask = estado().motor_mask;
    caps.ticks_per_revolution = 1320;
    return caps;
}

MotorPositionStatus Brick::move_motor_relative(std::int32_t delta_ticks, float, std::uint16_t,
                                               std::uint16_t, std::chrono::milliseconds) {
    estado().ultimo_avance_ticks = delta_ticks;
    MotorPositionStatus s;
    s.valid = true;
    return s;
}
MotorPositionStatus Brick::motor_position_status(std::chrono::milliseconds) {
    MotorPositionStatus s;  // the advance finishes as soon as it is queried
    s.valid = true;
    s.active = false;
    return s;
}
MotorPositionStatus Brick::zero_motor_position(std::chrono::milliseconds) {
    MotorPositionStatus s;
    s.valid = true;
    return s;
}
// Used by spike_compat::Advance_For_Degrees_Until (Obstacle Challenge) when
// the stop condition is met; the Open Challenge does not call it.
MotorPositionStatus Brick::cancel_motor_position(std::chrono::milliseconds) {
    estado().eventos.push_back("cancel");
    MotorPositionStatus s;
    s.valid = true;
    s.state = MotorPositionState::cancelled;
    return s;
}
MotorPositionStatus Brick::hold_motor_position(float, std::uint16_t, std::uint16_t,
                                               std::chrono::milliseconds) {
    ++estado().holds;
    MotorPositionStatus s;
    s.valid = true;
    s.state = MotorPositionState::holding;
    return s;
}

void Brick::set_bus_servo_position(std::uint8_t servo_id, std::uint16_t position,
                                   std::uint16_t duration_ms) {
    estado().eventos.push_back("servo " + std::to_string(servo_id) + " " +
                               std::to_string(position) + " " + std::to_string(duration_ms));
}
BusServoFeedback Brick::read_bus_servo_position(std::uint8_t servo_id,
                                                std::chrono::milliseconds) const {
    BusServoFeedback f;
    f.id = servo_id;
    f.position = estado().posicion_servo;
    f.valid = estado().servo_responde;
    return f;
}
void Brick::set_steering(double normalized, const SteeringCalibration &calibration) {
    estado().ultima_direccion = normalized;
    estado().ultima_calibracion = calibration;
}
void Brick::center_steering(const SteeringCalibration &calibration) {
    set_steering(0.0, calibration);
    estado().eventos.push_back("centrar");
}

ImuSample Brick::imu() const {
    ImuSample s;
    s.yaw_deg = estado().yaw_deg;
    s.sequence = ++fake::g_secuencia;
    s.valid = true;
    return s;
}
bool Brick::wait_for_imu(std::chrono::milliseconds) const {
    // One new sample: the robot advances one step with the current steering
    // and speed. Right (+1) makes the yaw decrease.
    estado().yaw_deg -= estado().ultima_direccion * estado().ultima_velocidad_rps *
                        fake::kGradosPorPaso;
    return true;
}
void Brick::calibrate_gyro(std::chrono::milliseconds duration) {
    estado().eventos.push_back("calibrate " + std::to_string(duration.count()));
}
void Brick::reset_yaw(double yaw_deg) {
    estado().yaw_deg = yaw_deg;
    estado().ultimo_reset_yaw = yaw_deg;
}

}  // namespace hiwonder
