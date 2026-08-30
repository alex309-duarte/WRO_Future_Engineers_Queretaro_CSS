#include "hiwonder_runtime.h"

#include "hiwonder_brick/hiwonder_brick.hpp"
#include "spike_compat/spike_compat.h"

#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::unique_ptr<hiwonder::Brick> g_brick;
double g_wheel_diameter_mm = 0.0;
double g_distance_scale = 0.0;

std::string required_text(const char *name) {
    const char *value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        throw std::runtime_error(std::string("falta ") + name);
    }
    return value;
}

double required_double(const char *name, double minimum, double maximum,
                       bool allow_negative = false) {
    const std::string text = required_text(name);
    char *end = nullptr;
    errno = 0;
    const double value = std::strtod(text.c_str(), &end);
    if (errno != 0 || end == text.c_str() || *end != '\0' ||
        !std::isfinite(value)) {
        throw std::runtime_error(std::string(name) + " no es un número válido");
    }
    const double checked = allow_negative ? std::fabs(value) : value;
    if (checked < minimum || checked > maximum) {
        throw std::runtime_error(std::string(name) + " fuera de rango");
    }
    return value;
}

long required_integer(const char *name, long minimum, long maximum) {
    const std::string text = required_text(name);
    char *end = nullptr;
    errno = 0;
    const long value = std::strtol(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str() || *end != '\0' ||
        value < minimum || value > maximum) {
        throw std::runtime_error(std::string(name) + " fuera de rango");
    }
    return value;
}

double required_nonzero_factor(const char *name) {
    const double value = required_double(name, 1.0e-9, 1.0, true);
    if (value == 0.0) {
        throw std::runtime_error(std::string(name) + " no puede ser cero");
    }
    return value;
}

std::string discover_device() {
    if (const char *configured = std::getenv("HIWONDER_DEVICE")) {
        if (*configured != '\0') return configured;
    }

    const std::filesystem::path directory("/dev/serial/by-id");
    std::vector<std::string> candidates;
    std::error_code error;
    if (std::filesystem::exists(directory, error)) {
        for (const auto &entry : std::filesystem::directory_iterator(directory, error)) {
            const std::string name = entry.path().filename().string();
            if (name.rfind("usb-1a86_USB_Single_Serial_", 0) == 0) {
                candidates.push_back(entry.path().string());
            }
        }
    }
    if (candidates.size() != 1) {
        throw std::runtime_error(
            "no se pudo descubrir un único Hiwonder; defina HIWONDER_DEVICE");
    }
    return candidates.front();
}

bool parse_arm_value(const char *value) {
    if (value == nullptr || *value == '\0' || std::string(value) == "0" ||
        std::string(value) == "false" || std::string(value) == "no") {
        return false;
    }
    if (std::string(value) == "1" || std::string(value) == "true" ||
        std::string(value) == "yes") {
        return true;
    }
    throw std::runtime_error("HIWONDER_ARM debe ser 0 o 1");
}

}  // namespace

bool Hiwonder_Arm_Requested() {
    return parse_arm_value(std::getenv("HIWONDER_ARM"));
}

void Hiwonder_Initialize_From_Environment() {
    if (g_brick) return;

    // Validar absolutamente todo antes de abrir el puerto o tocar hardware.
    hiwonder::Config brick_config;
    brick_config.device = discover_device();

    spike_compat::Config compat;
    compat.steering.servo_id = static_cast<std::uint8_t>(
        required_integer("HIWONDER_STEERING_ID", 1, 253));
    compat.steering.left = static_cast<std::uint16_t>(
        required_integer("HIWONDER_STEERING_LEFT", 0, 1000));
    compat.steering.center = static_cast<std::uint16_t>(
        required_integer("HIWONDER_STEERING_CENTER", 0, 1000));
    compat.steering.right = static_cast<std::uint16_t>(
        required_integer("HIWONDER_STEERING_RIGHT", 0, 1000));
    compat.steering.duration_ms = static_cast<std::uint16_t>(
        required_integer("HIWONDER_STEERING_DURATION_MS", 50, 3000));
    if (compat.steering.left == compat.steering.center ||
        compat.steering.right == compat.steering.center) {
        throw std::runtime_error("los topes de dirección no pueden coincidir con el centro");
    }

    compat.pd_output_to_steering =
        required_nonzero_factor("HIWONDER_PD_OUTPUT_TO_STEERING");
    compat.tire_turn_to_steering =
        required_nonzero_factor("HIWONDER_TIRE_TURN_TO_STEERING");
    compat.rps_at_speed_100 =
        required_double("HIWONDER_RPS_AT_SPEED_100", 0.01, 3.0);
    compat.traction_direction =
        required_double("HIWONDER_TRACTION_DIRECTION", 1.0, 1.0, true);
    if (compat.traction_direction != 1.0 && compat.traction_direction != -1.0) {
        throw std::runtime_error("HIWONDER_TRACTION_DIRECTION debe ser +1 o -1");
    }
    compat.max_rps_limit = static_cast<float>(
        required_double("HIWONDER_MAX_RPS", 0.01, 3.0));
    brick_config.max_speed_rps = compat.max_rps_limit;

    compat.forward_kp = required_double("HIWONDER_FORWARD_KP", 0.0, 1.0e6, true);
    compat.forward_kd = required_double("HIWONDER_FORWARD_KD", 0.0, 1.0e6, true);
    compat.follow_kp = required_double("HIWONDER_FOLLOW_KP", 0.0, 1.0e6, true);
    compat.follow_kd = required_double("HIWONDER_FOLLOW_KD", 0.0, 1.0e6, true);
    compat.advance_kp = required_double("HIWONDER_ADVANCE_KP", 0.0, 1.0e6, true);
    compat.advance_kd = required_double("HIWONDER_ADVANCE_KD", 0.0, 1.0e6, true);

    compat.position_tolerance_ticks = static_cast<std::uint16_t>(
        required_integer("HIWONDER_POSITION_TOLERANCE_TICKS", 1, 1000));
    compat.position_timeout_ms = static_cast<std::uint16_t>(
        required_integer("HIWONDER_POSITION_TIMEOUT_MS", 100, 60000));
    compat.control_period = std::chrono::milliseconds(
        required_integer("HIWONDER_CONTROL_PERIOD_MS", 1, 200));
    compat.spike_degrees_to_ticks = 1320.0 / 360.0;

    const double wheel_diameter =
        required_double("HIWONDER_WHEEL_DIAMETER_MM", 10.0, 500.0);
    const double distance_scale =
        required_double("HIWONDER_DISTANCE_SCALE", 0.1, 3.0);

    auto brick = std::make_unique<hiwonder::Brick>(brick_config);
    try {
        brick->open();
        const auto capabilities = brick->motor_position_capabilities();
        if (!capabilities.valid || capabilities.protocol_version != 1 ||
            (capabilities.motor_mask & 0x01U) == 0U ||
            capabilities.ticks_per_revolution_m1 != 1320U) {
            throw std::runtime_error(
                "firmware Hiwonder incompatible: se requiere MOTOR_EXT v1, M1 y 1320 ticks");
        }
        if (!brick->wait_for_imu(std::chrono::seconds(2))) {
            throw std::runtime_error("no llegó telemetría IMU en 2 segundos");
        }
        brick->calibrate_gyro(std::chrono::seconds(2));
        spike_compat::Spike_Attach(brick.get(), compat);
    } catch (...) {
        spike_compat::Spike_Detach();
        brick->close();
        throw;
    }

    g_wheel_diameter_mm = wheel_diameter;
    g_distance_scale = distance_scale;
    g_brick = std::move(brick);
    std::cout << "Hiwonder armado en " << brick_config.device
              << "; MOTOR_EXT v1, 1320 ticks/vuelta e IMU verificados\n";
}

void Hiwonder_Shutdown() noexcept {
    if (!g_brick) return;
    try {
        g_brick->stop_all();
    } catch (...) {
    }
    spike_compat::Spike_Detach();
    g_brick->close();
    g_brick.reset();
    g_wheel_diameter_mm = 0.0;
    g_distance_scale = 0.0;
}

bool Hiwonder_Is_Ready() noexcept { return static_cast<bool>(g_brick); }

double Hiwonder_Wheel_Diameter_Mm() {
    if (!g_brick) throw std::logic_error("Hiwonder no está armado");
    return g_wheel_diameter_mm;
}

double Hiwonder_Distance_Scale() {
    if (!g_brick) throw std::logic_error("Hiwonder no está armado");
    return g_distance_scale;
}
