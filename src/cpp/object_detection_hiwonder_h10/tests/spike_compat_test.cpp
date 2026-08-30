// Pruebas offline de la capa de compatibilidad SPIKE.
// NO usan assert: assert desaparece con -DNDEBUG (que añade
// CMAKE_BUILD_TYPE=Release) y la suite pasaría sin verificar nada.

#include "spike_compat/spike_compat.h"

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

}  // namespace

int main() {
    std::printf("spike_compat: pruebas offline\n");
    test_concatenate();
    test_repl_stubs_are_safe();
    test_requires_attach();
    test_requires_calibration();
    test_default_tick_conversion();
    std::printf("%d comprobaciones, %d fallos\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
