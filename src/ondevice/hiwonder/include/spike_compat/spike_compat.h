#pragma once

// Compatibility layer for the original LEGO SPIKE Prime API of the Hailo10H
// project (design v6). The exact contract is in
// references/hailo10h_spike/spike.{h,cpp}. Mission code for both the Open
// Challenge and the Obstacle Challenge calls these functions unchanged.
//
// The original was a REPL driver: it sent MicroPython over serial to the hub,
// which defined functions and ran them. Here the same signatures delegate to
// hiwonder::Brick (motor M4, HX-35H steering servo, on-board IMU). The REPL
// transport functions remain as no-ops.
//
// IMPORTANT: Spike_Attach() must be called before anything else. The LEGO
// scale factors do NOT carry over to the Ackermann linkage of the HX-35H, so
// they have no default value: if they are not calibrated, the motion
// functions throw std::logic_error instead of moving incorrectly.

#include "hiwonder_brick/hiwonder_brick.hpp"

#include <functional>

namespace spike_compat {

// Calibration and tuning of the compatibility layer. Filled in by
// Spike_Hiwonder_Init() (spike_hiwonder.cpp) from hiwonder_config.h.
struct Config {
    // --- Steering ------------------------------------------------------
    // The original did run_to_absolute_position(port.F, int(et*6.4), ...):
    // the PD output was converted to degrees of the steering motor. Here the
    // HX-35H works in normalized units -1..1, so this factor converts the PD
    // output directly to that scale. CALIBRATE WITH THE LINKAGE.
    double pd_output_to_steering = 0.0;

    // The original did run_to_relative_position(port.F, tire_turn*4.6*dir).
    // Same reasoning: tire_turn -> normalized. CALIBRATE.
    double tire_turn_to_steering = 0.0;

    // --- Drive ---------------------------------------------------------
    // The original used set_duty_cycle(port.B, 100*speed): OPEN LOOP, speed
    // in 0..100. Brick::set_motor_speed is rev/s in CLOSED LOOP. It is not
    // just a change of scale: under varying load the behaviour differs (the
    // SPIKE lost speed on a slope, the Hiwonder board keeps it).
    // rev/s equivalent to speed=100. CALIBRATE.
    double rps_at_speed_100 = 0.0;

    // SPIKE drive shaft degrees -> encoder ticks of the position motor
    // (M4; Brick::position_motor_id()).
    // 1320 ticks/rev is MEASURED (bench 2026-08-22), but the equivalence is
    // only 1:1 if the wheel diameter matches that of the LEGO robot.
    double spike_degrees_to_ticks = 1320.0 / 360.0;

    // --- Safety --------------------------------------------------------
    float max_rps_limit = 1.0f;              // hard speed limit, rev/s
    std::uint16_t position_tolerance_ticks = 15;   // firmware "reached" band
    std::uint16_t position_timeout_ms = 20000;     // ms, per move / turn
    // Advance finishes when at most this many ticks remain: it cancels the
    // move and leaves the drive in COAST, like ag() on the hub, instead of
    // waiting for the firmware to settle within the tolerance (the last part
    // runs at 0.10 rev/s and can take ~1 s). 0 = wait for the firmware.
    std::uint16_t advance_finish_ticks = 0;
    // The blocking loops (Advance, Turn) wait for each NEW IMU sample (20 Hz)
    // before the next step, so the D term sees a real heading change. This
    // value only bounds the wait if the IMU does not arrive (x5).
    std::chrono::milliseconds control_period{20};

    // HEADING error dead band, in degrees (Forward and Advance). 0 = same as
    // the original. With the robot still, the residual gyro drift (~0.01 °/s
    // after calibration) keeps the servo correcting by a few units; a 0.3°
    // band prevents that. It is continuous: outside the band its width is
    // subtracted, so there is no jump when crossing it. Does not affect
    // Follow_Reference.
    double heading_deadband_deg = 0.0;

    // --- HOLD (Spike_Hold_Motors) -----------------------------------------
    // Like motor.HOLD on SPIKE: the drive motor holds its position and comes
    // back if pushed. PWM is limited and, if it stays stalled for 3 s at the
    // limit, the firmware releases it (state stalled). Requires MOTOR_EXT v3
    // (pos-v8); with older firmware Hold_Motors performs a plain stop.
    float hold_max_rps = 1.0f;
    std::uint16_t hold_tolerance_ticks = 8;    // ~2.2° of the output shaft
    std::uint16_t hold_max_pulse = 400;        // 40 % PWM

    // --- Reverse (Advance_For_Degrees with speed < 0) ------------------------
    // Like ag_u() on the hub: the sign of speed gives the direction of travel.
    // In reverse the steering acts on the heading the opposite way, so the PD
    // inverts the sign of its output; the hub also used a kp 0.15 times that
    // of ag() (-0.03 versus 0.2) and no D term. Those ratios are kept; TUNE on
    // the track (parking manoeuvre of the Obstacle Challenge).
    double reverse_kp_factor = 0.15;
    double reverse_kd_factor = 0.0;

    hiwonder::SteeringCalibration steering{};   // servo ID, left/center/right
};

// Continuous dead band: 0 inside ±band; outside, error - sign(error)*band.
// Pure; band <= 0 leaves the error unchanged.
double apply_deadband(double error, double band);

// Binds the layer to an already constructed Brick. Does not open the port.
void Spike_Attach(hiwonder::Brick *brick, const Config &config);
void Spike_Detach() noexcept;
bool Spike_Is_Attached() noexcept;
const Config &Spike_Get_Config();
// Changes the configuration without rebinding the board (e.g. a different
// tire_turn scale for another mission). Resets the PD state.
void Spike_Update_Config(const Config &config);

// Steers to tire_turn (hub units, scaled by tire_turn_to_steering) without
// moving the drive: what turn_special() did with `await` before driving.
void Steer_Tire_Turn(int direction, int tire_turn);

// Like Spike_Advance_For_Degrees, but checks `stop` on every loop step; if it
// returns true, the move is cancelled and the robot stops. Returns true if it
// stopped before completing the degrees. Empty `stop` = no condition.
bool Advance_For_Degrees_Until(int speed, int degrees, int reference,
                               const std::function<bool()> &stop);

}  // namespace spike_compat

extern "C" {

// --- REPL transport: obsolete ------------------------------------------
int   Spike_Serial_Init(void);               // opens the Brick; 0 = ok, -1 = error
void  Spike_Close_Serial(void);
void  Spike_Send_Serial_Data(char data[]);   // no-op
char *Spike_Read_Serial_Data(void);          // always returns ""
void  Spike_Interpreter(void);               // no-op
void  Spike_Initialize_Libraries(void);      // no-op: Python is no longer uploaded

// The original header declared Spike_End_Function but spike.cpp only
// implemented Spike_End_Funcion (without the "t"): the declaration was a
// dangling symbol that would never have linked. Here it exists with the
// correct name and is a no-op, because it only served to close REPL blocks.
void  Spike_End_Function(void);

// Pure string function: kept identical to the original.
void  Spike_Concatenate(int list_lenght, const char *argument_1[], char *buffer);

// --- Motion and sensors ------------------------------------------------
// Steering to centre (wheels straight); both variants behave the same.
void  Spike_Center_Vehicle(void);
void  Spike_Center_Vehicle_Short(void);
// Stops all motors (COAST).
void  Spike_Coast_Motors(void);
// Drive motor position HOLD (plain stop with older firmware).
void  Spike_Hold_Motors(void);
// Sets the yaw to `degrees` and resets the PD state.
void  Spike_Reset_Gyro(float degrees);
// Current yaw in degrees, integrated by the Brick from the IMU gyro.
float Spike_Get_Gyro(void);
// Blocking turn: steers to tire_turn in `direction` (+1 / -1), drives at
// `speed` and stops when |yaw| reaches `degrees`; then COAST and HOLD.
void  Spike_Turn_For_Degrees(int direction, int speed, float degrees, int tire_turn);
// Like ag()/ag_u() on the hub: the sign of speed gives the direction (reverse
// if negative) and degrees the magnitude.
void  Spike_Advance_For_Degrees(int speed, int degrees, int reference);
// One non-blocking PD step: steers toward heading `reference` (deg) and
// drives at `speed`. Must be called repeatedly by the mission loop.
void  Spike_Forward(int speed, int reference);
// One non-blocking PD step on two user quantities (e.g. LiDAR distances):
// steers to make reference_1 equal reference_2 while driving at `speed`.
void  Spike_Follow_Reference(int speed, float reference_1, float reference_2);
// Like Spike_Turn_For_Degrees, but with a signed condition for short
// corrections: keeps turning while direction * yaw is greater than |degrees|
// and stops once it drops to that value.
void  Spike_Small_Turn(int direction, int speed, float degrees, int tire_turn);

}  // extern "C"
