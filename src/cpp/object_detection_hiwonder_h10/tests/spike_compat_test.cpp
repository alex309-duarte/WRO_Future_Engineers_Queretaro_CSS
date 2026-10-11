// Pruebas offline de la capa de compatibilidad SPIKE.
// NO usan assert: assert desaparece con -DNDEBUG (que añade
// CMAKE_BUILD_TYPE=Release) y la suite pasaría sin verificar nada.

#include "spike_compat/spike_compat.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool ok, const char *what) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::printf("  FALLO: %s\n", what);
    }
}

template <typename F>
void check_throws(F &&fn, const char *what) {
    ++g_checks;
    try {
        fn();
        ++g_failures;
        std::printf("  FALLO: %s (no lanzo excepcion)\n", what);
    } catch (const std::logic_error &) {
        /* esperado */
    } catch (const std::exception &e) {
        ++g_failures;
        std::printf("  FALLO: %s (excepcion inesperada: %s)\n", what, e.what());
    }
}

void test_concatenate() {
    char buffer[256] = {};
    const char *list[3] = {"turn(", "1,50,90.0,30", ")\r"};
    Spike_Concatenate(3, list, buffer);
    check(std::string(buffer) == "turn(1,50,90.0,30)\r",
          "Spike_Concatenate reproduce el formato original");

    char one[16] = {};
    const char *single[1] = {"cv()\r"};
    Spike_Concatenate(1, single, one);
    check(std::string(one) == "cv()\r", "Spike_Concatenate con un solo elemento");
}

void test_repl_stubs_are_safe() {
    Spike_End_Function();
    Spike_Interpreter();
    Spike_Initialize_Libraries();
    char msg[] = "cualquier cosa\r";
    Spike_Send_Serial_Data(msg);
    check(std::strcmp(Spike_Read_Serial_Data(), "") == 0,
          "Spike_Read_Serial_Data devuelve cadena vacia");
    check(true, "los stubs del REPL no revientan");
}

void test_requires_attach() {
    spike_compat::Spike_Detach();
    check(!spike_compat::Spike_Is_Attached(), "Spike_Detach desasocia");
    check_throws([] { Spike_Coast_Motors(); },
                 "usar la API sin Spike_Attach lanza logic_error");
    check_throws([] { Spike_Get_Gyro(); },
                 "Spike_Get_Gyro sin attach lanza logic_error");
}

void test_requires_calibration() {
    // Brick sin abrir: el constructor no toca hardware.
    hiwonder::Brick brick;
    spike_compat::Config cfg;  // sin calibrar a proposito
    spike_compat::Spike_Attach(&brick, cfg);
    check(spike_compat::Spike_Is_Attached(), "Spike_Attach asocia");

    check_throws([] { Spike_Forward(50, 0); },
                 "Spike_Forward sin pd_output_to_steering lanza");
    check_throws([] { Spike_Follow_Reference(50, 10.0F, 12.0F); },
                 "Spike_Follow_Reference sin calibrar lanza");
    check_throws([] { Spike_Turn_For_Degrees(1, 50, 90.0F, 30); },
                 "Spike_Turn_For_Degrees sin tire_turn_to_steering lanza");
    check_throws([] { Spike_Small_Turn(1, 50, 15.0F, 20); },
                 "Spike_Small_Turn sin calibrar lanza");

    // Con dirección calibrada pero sin velocidad, debe seguir protegiendo.
    spike_compat::Config partial;
    partial.pd_output_to_steering = 0.01;
    partial.tire_turn_to_steering = 0.02;
    spike_compat::Spike_Attach(&brick, partial);
    check_throws([] { Spike_Turn_For_Degrees(1, 50, 90.0F, 30); },
                 "sin rps_at_speed_100 lanza aunque la direccion este calibrada");

    spike_compat::Spike_Detach();
}

void test_default_tick_conversion() {
    const spike_compat::Config cfg;
    const double ticks_por_grado = cfg.spike_degrees_to_ticks;
    check(std::abs(ticks_por_grado * 360.0 - 1320.0) < 1e-9,
          "la conversion por defecto da 1320 ticks por vuelta (medido en banco)");
    check(std::abs(ticks_por_grado * 30.0 - 110.0) < 1e-9,
          "30 grados = 110 ticks, como documenta WRO_MIGRACION.md");
    check(cfg.pd_output_to_steering == 0.0 && cfg.tire_turn_to_steering == 0.0 &&
              cfg.rps_at_speed_100 == 0.0,
          "los factores del LEGO NO traen valor por defecto");
}

void test_position_motor_config() {
    check(hiwonder::Config{}.position_motor_id == 3,
          "el motor de posicion por defecto es M4 (indice 3), como el firmware");
    hiwonder::Config bad;
    bad.position_motor_id = 4;
    check_throws([&] { hiwonder::Brick brick(bad); },
                 "position_motor_id fuera de 0..3 se rechaza sin abrir el puerto");
}

void test_imu_units() {
    // Trama tal como la publica el firmware: aceleración en m/s², giro en rad/s.
    const float raw[6] = {0.0F, 0.0F, 9.807F, 0.0F, 0.0F, 1.5707963F};
    std::uint8_t payload[24];
    std::memcpy(payload, raw, sizeof(payload));
    const auto s = hiwonder::Brick::decode_imu_payload(payload);
    check(std::abs(s.accel_z_g - 1.0F) < 1e-5F,
          "9.807 m/s2 del firmware se leen como 1 g");
    check(std::abs(s.gyro_z_dps - 90.0F) < 1e-3F,
          "pi/2 rad/s del firmware se leen como 90 dps (antes 1.57: yaw 57x menor)");
    check(std::abs(s.roll_deg) < 1e-3F && std::abs(s.pitch_deg) < 1e-3F,
          "placa plana: roll y pitch 0");
}

void test_variant_defaults() {
    const spike_compat::Config c{};
    check(c.forward_kp == 0.11 && c.forward_kd == 0.7 &&
              c.follow_kp == 0.08 && c.follow_kd == 0.7 &&
              c.advance_kp == 0.1 && c.advance_kd == 0.5,
          "ganancias configurables: por defecto las de spike.cpp");
    check(c.traction_direction == 1.0, "traction_direction por defecto +1");
}

void test_traction_direction_validated() {
    // Brick sin abrir: la validación debe fallar antes de tocar el puerto.
    hiwonder::Brick brick;
    spike_compat::Config c;
    c.pd_output_to_steering = 0.1;
    c.rps_at_speed_100 = 1.0;
    c.traction_direction = 0.5;
    spike_compat::Spike_Attach(&brick, c);
    check_throws([] { Spike_Forward(50, 0); },
                 "traction_direction distinto de +-1 se rechaza antes de mover");
    spike_compat::Spike_Detach();
}

void test_heading_deadband() {
    check(spike_compat::apply_deadband(0.2, 0.3) == 0.0,
          "dentro de la banda el error de rumbo es 0");
    check(std::abs(spike_compat::apply_deadband(1.0, 0.3) - 0.7) < 1e-12 &&
              std::abs(spike_compat::apply_deadband(-1.0, 0.3) + 0.7) < 1e-12,
          "fuera de la banda resta su ancho (continua, sin salto)");
    check(spike_compat::apply_deadband(0.05, 0.0) == 0.05 &&
              spike_compat::Config{}.heading_deadband_deg == 0.0,
          "banda 0 por defecto: mismo comportamiento que el SPIKE original");
}

}  // namespace

int main() {
    std::printf("spike_compat: pruebas offline\n");
    test_concatenate();
    test_repl_stubs_are_safe();
    test_requires_attach();
    test_requires_calibration();
    test_default_tick_conversion();
    test_position_motor_config();
    test_imu_units();
    test_heading_deadband();
    test_variant_defaults();
    test_traction_direction_validated();
    std::printf("%d comprobaciones, %d fallos\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
