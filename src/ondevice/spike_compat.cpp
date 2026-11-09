// spike_compat implementation: the LEGO SPIKE Prime API of design v6
// (heading PD on the gyro, timed turns, encoder-measured advances) rebuilt on
// top of hiwonder::Brick, so the Open Challenge and Obstacle Challenge
// mission code runs unchanged on the Hiwonder board. Steering goes to the
// HX-35H bus servo, drive to motor M4 (closed-loop speed or firmware position
// control), heading comes from the yaw integrated by the Brick.

#include "spike_compat/spike_compat.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <thread>

namespace spike_compat {
namespace detail {

// Global state of the layer (the original API is a set of free C functions,
// so the binding is global). Set by Spike_Attach().
hiwonder::Brick *g_brick = nullptr;
Config g_config{};
bool g_attached = false;

/* PD state, equivalent to the "global error" of the hub script. */
double g_prev_error = 0.0;

/* Gains copied verbatim from references/hailo10h_spike/spike.cpp (hub
   functions da / apd / ag). The error is in decidegrees (see
   to_decidegrees). */
constexpr double kForwardKp = 0.05;   // da(): Spike_Forward
constexpr double kForwardKd = 0.0;
constexpr double kFollowKp  = 0.08;   // apd(): Spike_Follow_Reference
constexpr double kFollowKd  = 0.7;
constexpr double kAdvanceKp = 0.02;   // ag(): Spike_Advance_For_Degrees
constexpr double kAdvanceKd = 0.05;

// Returns the bound Brick; throws std::logic_error if Spike_Attach() was not
// called.

hiwonder::Brick &brick() {
    if (!g_attached || g_brick == nullptr) {
        throw std::logic_error(
            "spike_compat: llamar a Spike_Attach() antes de usar la API");
    }
    return *g_brick;
}

// Guards that refuse to move with uncalibrated (zero) scale factors.
void require_steering_calibration() {
    if (g_config.pd_output_to_steering == 0.0) {
        throw std::logic_error(
            "spike_compat: pd_output_to_steering sin calibrar; el factor 6.4 "
            "del LEGO no es trasladable al varillaje Ackermann del HX-35H");
    }
}

void require_speed_calibration() {
    if (g_config.rps_at_speed_100 <= 0.0) {
        throw std::logic_error("spike_compat: rps_at_speed_100 sin calibrar");
    }
}

double clamp_unit(double v) { return std::max(-1.0, std::min(1.0, v)); }

/* SPIKE speed (signed, 0..100) -> rev/s, clamped to the hard safety limit. */
float speed_to_rps(int speed) {
    require_speed_calibration();
    const double rps =
        (static_cast<double>(speed) / 100.0) * g_config.rps_at_speed_100;
    const double limit = static_cast<double>(g_config.max_rps_limit);
    return static_cast<float>(std::max(-limit, std::min(limit, rps)));
}

/* The hub yaw came in tenths of a degree and the PD was tuned in those
   units. The scale is kept so the original gains mean the same: the error
   enters the PD in decidegrees. */
double to_decidegrees(double degrees) { return degrees * 10.0; }

/* One step of the original PD:  error = s1 - s2;  et = kp*error + kd*(error - ea).
   Applies steering and, if requested, drive speed. Returns the error and
   stores it, as "global error" did on the hub. output_sign = -1 in reverse:
   the steering acts on the heading the opposite way when the robot drives
   backwards. Side effects: moves the steering servo (and the drive motor if
   drive_motor is true). */
double pd_step(double s1, double s2, int speed, double kp, double kd,
               bool drive_motor, double deadband = 0.0, double output_sign = 1.0) {
    /* Validate the whole calibration BEFORE touching hardware: otherwise the
       steering would move and only then fail on the speed check, leaving the
       robot half-actuated. */
    require_steering_calibration();
    if (drive_motor) require_speed_calibration();
    const double error = spike_compat::apply_deadband(s1 - s2, deadband);
    const double et = (kp * error) + (kd * (error - g_prev_error));
    g_prev_error = error;

    // PD output -> normalized steering, clamped to the calibrated limits.
    brick().set_steering(clamp_unit(output_sign * et * g_config.pd_output_to_steering),
                         g_config.steering);
    if (drive_motor) {
        brick().set_motor_speed(brick().position_motor_id(),
                                speed_to_rps(speed));
    }
    return error;
}

/* Next loop step: waits for a NEW IMU sample (20 Hz). With a fixed period
   the PD sometimes saw the same yaw twice (D = 0) and other times a double
   step (D x2). If the IMU does not arrive, it blocks no longer than 5
   periods. */
void wait_control_step() {
    brick().wait_for_imu(g_config.control_period * 5);
}

/* Heading dead band in PD units (decidegrees). */
double heading_deadband() { return g_config.heading_deadband_deg * 10.0; }

}  // namespace detail

double apply_deadband(double error, double band) {
    if (!(band > 0.0)) return error;
    if (std::fabs(error) <= band) return 0.0;
    return error > 0.0 ? error - band : error + band;
}

// A null brick_ptr leaves the layer unattached.
void Spike_Attach(hiwonder::Brick *brick_ptr, const Config &config) {
    detail::g_brick = brick_ptr;
    detail::g_config = config;
    detail::g_prev_error = 0.0;
    detail::g_attached = (brick_ptr != nullptr);
}

void Spike_Detach() noexcept {
    detail::g_brick = nullptr;
    detail::g_attached = false;
    detail::g_prev_error = 0.0;
}

bool Spike_Is_Attached() noexcept { return detail::g_attached; }

const Config &Spike_Get_Config() { return detail::g_config; }

void Spike_Update_Config(const Config &config) {
    detail::g_config = config;
    detail::g_prev_error = 0.0;
}

// Steering only: normalized = tire_turn * scale * direction, clamped to -1..1.
void Steer_Tire_Turn(int direction, int tire_turn) {
    auto &b = detail::brick();
    if (detail::g_config.tire_turn_to_steering == 0.0) {
        throw std::logic_error(
            "spike_compat: tire_turn_to_steering sin calibrar");
    }
    b.set_steering(detail::clamp_unit(static_cast<double>(tire_turn) *
                                      detail::g_config.tire_turn_to_steering *
                                      static_cast<double>(direction)),
                   detail::g_config.steering);
}

/* Original ag()/ag_u(): reset the relative position of motor B, drove it at
   a constant duty cycle (with the sign of speed) and looped until |degrees|
   was reached, correcting the heading with the PD; then COAST.

   Here the distance is controlled by the STM32 position loop (MOVE_RELATIVE,
   validated on the bench 2026-08-22), and the host only corrects the
   heading. It is more accurate than the original, but it DIFFERS: no speed
   command is sent to the drive (position motor, M4) during the advance,
   because set_motor_speed would cancel the position move in the firmware.
   "speed" becomes the speed limit; its sign, the direction of travel (the
   firmware only accepts a positive limit, the direction goes in the ticks).
   Blocks until the move ends, `stop` fires or position_timeout_ms expires;
   always finishes with the motors in COAST. */
bool Advance_For_Degrees_Until(int speed, int degrees, int reference,
                               const std::function<bool()> &stop) {
    auto &b = detail::brick();
    detail::require_steering_calibration();
    detail::require_speed_calibration();
    const bool reversa = speed < 0;   // reverse
    const double sentido = reversa ? -1.0 : 1.0;
    const auto ticks = static_cast<std::int32_t>(std::llround(
        sentido * std::fabs(static_cast<double>(degrees)) *
        detail::g_config.spike_degrees_to_ticks));
    const float max_rps = std::fabs(detail::speed_to_rps(speed));
    /* ag_u(): in reverse the steering acts the opposite way and with lower gain. */
    const double kp = reversa ? detail::kAdvanceKp * detail::g_config.reverse_kp_factor
                              : detail::kAdvanceKp;
    const double kd = reversa ? detail::kAdvanceKd * detail::g_config.reverse_kd_factor
                              : detail::kAdvanceKd;
    const double signo_salida = reversa ? -1.0 : 1.0;

    if (ticks == 0) {
        /* ag(speed, 0) on the hub did not move; the firmware rejects a delta
           of 0 ticks (this happens when the mission subtracts as much as it
           measures, e.g. hypotenuse - 400 = 0). */
        Spike_Coast_Motors();
        return false;
    }

    // Zero the encoder, then hand the distance to the firmware position loop.
    b.zero_motor_position();
    b.move_motor_relative(ticks, max_rps,
                          detail::g_config.position_tolerance_ticks,
                          detail::g_config.position_timeout_ms);

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(detail::g_config.position_timeout_ms);
    bool detenido = false;   // true if `stop` ended the advance early
    // Heading-correction loop: one PD step (steering only) per IMU sample
    // until the firmware reports the move finished.
    for (;;) {
        if (stop && stop()) {
            b.cancel_motor_position();
            detenido = true;
            break;
        }
        detail::pd_step(detail::to_decidegrees(b.imu().yaw_deg),
                        detail::to_decidegrees(static_cast<double>(reference)),
                        speed, kp, kd, false, detail::heading_deadband(), signo_salida);
        const auto status = b.motor_position_status();
        if (status.valid && !status.active) break;
        if (status.valid && detail::g_config.advance_finish_ticks > 0 &&
            std::abs(status.error_ticks) <= detail::g_config.advance_finish_ticks) {
            /* Already arrived except for the fine settling. If the
               cancellation is not confirmed, the stop below also cancels the
               move. */
            try {
                b.cancel_motor_position();
            } catch (const std::runtime_error &) {
            }
            break;
        }
        if (std::chrono::steady_clock::now() > deadline) break;
        detail::wait_control_step();
    }
    Spike_Coast_Motors();
    return detenido;
}

}  // namespace spike_compat

/* ------------------------------------------------------------------------ */
extern "C" {

namespace sd = spike_compat::detail;

/* --- REPL transport: obsolete ------------------------------------------ */

/* Kept for source compatibility: "opening the serial port" now opens the
   Hiwonder board. Returns 0 on success, -1 on error. */
int Spike_Serial_Init(void) {
    try {
        sd::brick().open();
        return 0;
    } catch (const std::exception &) {
        return -1;
    }
}

void Spike_Close_Serial(void) {
    if (spike_compat::Spike_Is_Attached()) {
        sd::g_brick->close();
    }
}

/* REPL-only functions: no-ops. */
void Spike_Send_Serial_Data(char[]) {}
char *Spike_Read_Serial_Data(void) { static char empty[1] = ""; return empty; }
void Spike_Interpreter(void) {}
void Spike_Initialize_Libraries(void) {}
void Spike_End_Function(void) {}

/* Concatenates list_lenght C strings into buffer (with the final '\0').
   The caller provides a buffer large enough. */
void Spike_Concatenate(int list_lenght, const char *argument_1[], char *buffer) {
    int i = 0;
    for (int j = 0; j < list_lenght; ++j) {
        const char *current_string = argument_1[j];
        int k = 0;
        while (true) {
            buffer[i] = current_string[k];
            if (current_string[k] == '\0') break;
            ++k;
            ++i;
        }
    }
}

/* --- Steering and stop -------------------------------------------------- */

/* The original distinguished LONGEST_PATH and SHORTEST_PATH because motor F
   could make full revolutions. A bus servo with mechanical limits does not
   have that problem: both variants centre the same way. */
void Spike_Center_Vehicle(void) {
    sd::brick().center_steering(sd::g_config.steering);
}

void Spike_Center_Vehicle_Short(void) { Spike_Center_Vehicle(); }

/* All four motors stop and coast; the steering servo keeps its position. */
void Spike_Coast_Motors(void) { sd::brick().stop_all(); }

/* Original: motor.stop(port.F, stop=HOLD) and motor.stop(port.B, stop=HOLD).
   Here: firmware position HOLD (MOTOR_EXT v3) on the drive motor; the
   steering servo already holds its position by itself. If the firmware does
   not support HOLD (older than pos-v8), it falls back to a plain stop. */
void Spike_Hold_Motors(void) {
    auto &b = sd::brick();
    const auto &cfg = sd::g_config;
    try {
        const auto status = b.hold_motor_position(
            cfg.hold_max_rps, cfg.hold_tolerance_ticks, cfg.hold_max_pulse);
        if (status.state == hiwonder::MotorPositionState::holding) return;
    } catch (const std::runtime_error &) {
        /* no reply: fall back to the stop */
    }
    b.stop_all();
}

/* --- Gyro --------------------------------------------------------------- */

/* Sets the yaw and clears the PD memory so the next D term does not see a
   jump. */
void Spike_Reset_Gyro(float degrees) {
    sd::brick().reset_yaw(static_cast<double>(degrees));
    sd::g_prev_error = 0.0;
}

float Spike_Get_Gyro(void) {
    return static_cast<float>(sd::brick().imu().yaw_deg);
}

/* --- Non-blocking loops: one PD step per call --------------------------- */

/* Heading hold: PD on yaw vs reference (deg, converted to decidegrees),
   steering and drive at `speed`. */
void Spike_Forward(int speed, int reference) {
    sd::pd_step(sd::to_decidegrees(sd::brick().imu().yaw_deg),
                sd::to_decidegrees(static_cast<double>(reference)),
                speed, sd::kForwardKp, sd::kForwardKd, true,
                sd::heading_deadband());
}

void Spike_Follow_Reference(int speed, float reference_1, float reference_2) {
    /* Here the references are user quantities (e.g. LiDAR distances), not
       angles: they are not converted to decidegrees. */
    sd::pd_step(static_cast<double>(reference_1),
                static_cast<double>(reference_2),
                speed, sd::kFollowKp, sd::kFollowKd, true);
}

/* --- Blocking ----------------------------------------------------------- */

/* See spike_compat::Advance_For_Degrees_Until: same advance, without a stop
   condition. */
void Spike_Advance_For_Degrees(int speed, int degrees, int reference) {
    spike_compat::Advance_For_Degrees_Until(speed, degrees, reference, {});
}

namespace {

/* Common body of turn / small_turn. The original repeated the duty cycle
   inside the loop; here the speed is closed-loop and setting it once is
   enough. A time guard (position_timeout_ms) is added that the original did
   NOT have: an endless loop with the motor running is a real risk on the
   bench. Steering is fixed at tire_turn for the whole turn; the end
   condition is on the yaw only. Blocks, then leaves the drive in HOLD. */
void turn_impl(int direction, int speed, float degrees, int tire_turn,
               bool small) {
    auto &b = sd::brick();
    if (sd::g_config.tire_turn_to_steering == 0.0) {
        throw std::logic_error(
            "spike_compat: tire_turn_to_steering sin calibrar");
    }
    sd::require_speed_calibration();  /* before moving the steering */
    spike_compat::Steer_Tire_Turn(direction, tire_turn);
    b.set_motor_speed(b.position_motor_id(), sd::speed_to_rps(speed));

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(sd::g_config.position_timeout_ms);
    for (;;) {
        const double yaw = b.imu().yaw_deg;
        // turn: until |yaw| reaches |degrees|.
        // small_turn: until direction * yaw drops to |degrees| (signed).
        const bool done = small
            ? !(std::fabs(static_cast<double>(degrees)) <
                static_cast<double>(direction) * yaw)
            : !(std::fabs(static_cast<double>(degrees)) > std::fabs(yaw));
        if (done) break;
        if (std::chrono::steady_clock::now() > deadline) break;
        sd::wait_control_step();
    }
    Spike_Coast_Motors();
    Spike_Hold_Motors();
}

}  // namespace

void Spike_Turn_For_Degrees(int direction, int speed, float degrees, int tire_turn) {
    turn_impl(direction, speed, degrees, tire_turn, false);
}

void Spike_Small_Turn(int direction, int speed, float degrees, int tire_turn) {
    turn_impl(direction, speed, degrees, tire_turn, true);
}

}  // extern "C"
