#pragma once

// Capa de compatibilidad con la API SPIKE original del proyecto Hailo10H.
// El contrato exacto está en references/hailo10h_spike/spike.{h,cpp}.
//
// El original era un driver de REPL: enviaba MicroPython por serie al hub, que
// definía funciones y las ejecutaba. Aquí las mismas firmas delegan en
// hiwonder::Brick. Las funciones de transporte del REPL quedan como no-op.
//
// IMPORTANTE: hay que llamar a Spike_Attach() antes que a cualquier otra cosa.
// Los factores de escala del LEGO NO son trasladables al varillaje Ackermann
// del HX-12H, así que no traen valor por defecto: si no se calibran, las
// funciones de movimiento lanzan std::logic_error en vez de moverse mal.

#include "hiwonder_brick/hiwonder_brick.hpp"

#include <functional>

namespace spike_compat {

struct Config {
    // --- Dirección -----------------------------------------------------
    // El original hacía run_to_absolute_position(port.F, int(et*6.4), ...):
    // la salida del PD se convertía a grados del motor de dirección. Aquí el
    // HX-12H trabaja normalizado -1..1, así que este factor convierte la
    // salida del PD directamente a esa escala. CALIBRAR CON EL VARILLAJE.
    double pd_output_to_steering = 0.0;

    // El original hacía run_to_relative_position(port.F, tire_turn*4.6*dir).
    // Mismo razonamiento: tire_turn -> normalizado. CALIBRAR.
    double tire_turn_to_steering = 0.0;

    // --- Propulsión ----------------------------------------------------
    // El original usaba set_duty_cycle(port.B, 100*speed): LAZO ABIERTO,
    // speed en 0..100. Brick::set_motor_speed es r/s en LAZO CERRADO. No es
    // sólo un cambio de escala: con carga variable el comportamiento difiere
    // (el SPIKE perdía velocidad en cuesta, el Hiwonder la mantiene).
    // r/s equivalentes a speed=100. CALIBRAR.
    double rps_at_speed_100 = 0.0;

    // Grados del eje de propulsión SPIKE -> ticks del encoder del motor de
    // posición (M4; Brick::position_motor_id()).
    // 1320 ticks/vuelta está MEDIDO (banco 2026-08-22), pero la equivalencia
    // sólo es 1:1 si el diámetro de rueda coincide con el del robot LEGO.
    double spike_degrees_to_ticks = 1320.0 / 360.0;

    // --- Seguridad -----------------------------------------------------
    float max_rps_limit = 1.0f;              // tope duro de velocidad
    std::uint16_t position_tolerance_ticks = 15;
    std::uint16_t position_timeout_ms = 20000;
    // Advance termina cuando faltan como mucho estos ticks: cancela el
    // movimiento y deja la propulsión en COAST, como ag() del hub, en vez de
    // esperar a que el firmware se asiente dentro de la tolerancia (el último
    // tramo va a 0.10 r/s y puede tardar ~1 s). 0 = esperar al firmware.
    std::uint16_t advance_finish_ticks = 0;
    // Los lazos bloqueantes (Advance, Turn) esperan cada muestra NUEVA del IMU
    // (20 Hz) antes del siguiente paso, para que el término D vea un cambio de
    // rumbo real. Este valor sólo acota la espera si el IMU no llega (x5).
    std::chrono::milliseconds control_period{20};

    // Banda muerta del error de RUMBO, en grados (Forward y Advance). 0 = como
    // el original. Con el robot quieto, la deriva residual del giroscopio
    // (~0.01 °/s tras calibrar) deja el servo corrigiendo unas unidades; una
    // banda de 0.3° lo evita. Es continua: fuera de la banda resta su ancho,
    // así no hay salto al cruzarla. No afecta a Follow_Reference.
    double heading_deadband_deg = 0.0;

    // --- HOLD (Spike_Hold_Motors) -----------------------------------------
    // Como motor.HOLD del SPIKE: el motor de propulsión sostiene su posición y
    // vuelve si lo desplazan. PWM limitado y, si queda bloqueado 3 s al tope,
    // el firmware lo suelta (estado stalled). Requiere MOTOR_EXT v3 (pos-v8);
    // con firmware anterior Hold_Motors hace una parada, como antes.
    float hold_max_rps = 1.0f;
    std::uint16_t hold_tolerance_ticks = 8;    // ~2.2° del eje de salida
    std::uint16_t hold_max_pulse = 400;        // 40 % de PWM

    // --- Reversa (Advance_For_Degrees con speed < 0) ------------------------
    // Como ag_u() del hub: el signo de speed da el sentido de marcha. Al ir en
    // reversa la dirección actúa al revés sobre el rumbo, así que el PD
    // invierte el signo de su salida; el hub además usaba un kp 0.15 veces el
    // de ag() (-0.03 frente a 0.2) y sin término D. Se conservan esas
    // proporciones; AJUSTAR en pista (estacionamiento del reto 2).
    double reverse_kp_factor = 0.15;
    double reverse_kd_factor = 0.0;

    hiwonder::SteeringCalibration steering{};
};

// Banda muerta continua: 0 dentro de ±band; fuera, error - signo(error)*band.
// Pura; band <= 0 deja el error igual.
double apply_deadband(double error, double band);

// Asocia la capa a un Brick ya construido. No abre el puerto.
void Spike_Attach(hiwonder::Brick *brick, const Config &config);
void Spike_Detach() noexcept;
bool Spike_Is_Attached() noexcept;
const Config &Spike_Get_Config();
// Cambia la configuración sin reasociar la placa (p. ej. otra escala de
// tire_turn para otra misión). Reinicia el estado del PD.
void Spike_Update_Config(const Config &config);

// Dirección a tire_turn (unidades del hub, con tire_turn_to_steering) sin
// mover la propulsión: lo que turn_special() hacía con `await` antes de andar.
void Steer_Tire_Turn(int direction, int tire_turn);

// Como Spike_Advance_For_Degrees, pero consulta `stop` en cada paso del lazo;
// si devuelve true, cancela el movimiento y para. Devuelve true si se detuvo
// antes de completar los grados. `stop` vacío = sin condición.
bool Advance_For_Degrees_Until(int speed, int degrees, int reference,
                               const std::function<bool()> &stop);

}  // namespace spike_compat

extern "C" {

// --- Transporte del REPL: obsoleto -------------------------------------
int   Spike_Serial_Init(void);
void  Spike_Close_Serial(void);
void  Spike_Send_Serial_Data(char data[]);   // no-op
char *Spike_Read_Serial_Data(void);          // devuelve "" siempre
void  Spike_Interpreter(void);               // no-op
void  Spike_Initialize_Libraries(void);      // no-op: ya no se sube Python

// El header original declaraba Spike_End_Function pero spike.cpp sólo
// implementaba Spike_End_Funcion (sin la "t"): la declaración era un símbolo
// colgante que nunca habría enlazado. Aquí existe con el nombre correcto y es
// no-op, porque sólo servía para cerrar bloques del REPL.
void  Spike_End_Function(void);

// Función pura de cadenas: se conserva idéntica al original.
void  Spike_Concatenate(int list_lenght, const char *argument_1[], char *buffer);

// --- Movimiento y sensores ---------------------------------------------
void  Spike_Center_Vehicle(void);
void  Spike_Center_Vehicle_Short(void);
void  Spike_Coast_Motors(void);
void  Spike_Hold_Motors(void);
void  Spike_Reset_Gyro(float degrees);
float Spike_Get_Gyro(void);
void  Spike_Turn_For_Degrees(int direction, int speed, float degrees, int tire_turn);
// Como ag()/ag_u() del hub: el signo de speed da el sentido (reversa si es
// negativo) y degrees la magnitud.
void  Spike_Advance_For_Degrees(int speed, int degrees, int reference);
void  Spike_Forward(int speed, int reference);
void  Spike_Follow_Reference(int speed, float reference_1, float reference_2);
void  Spike_Small_Turn(int direction, int speed, float degrees, int tire_turn);

}  // extern "C"
