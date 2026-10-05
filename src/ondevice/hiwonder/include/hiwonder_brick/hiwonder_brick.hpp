#pragma once

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace hiwonder {

// Unidades del host: aceleración en g y giroscopio en °/s. El firmware
// extendido publica m/s² y rad/s (QMI8658.c sin QMI8658_UINT_MG_DPS); la
// conversión la hace decode_imu_payload(). Hasta 2026-10-04 el giroscopio se
// tomaba como °/s sin convertir y el yaw salía 57 veces más pequeño.
struct ImuSample {
    float accel_x_g = 0.0F;
    float accel_y_g = 0.0F;
    float accel_z_g = 0.0F;
    float gyro_x_dps = 0.0F;
    float gyro_y_dps = 0.0F;
    float gyro_z_dps = 0.0F;
    float roll_deg = 0.0F;
    float pitch_deg = 0.0F;
    double yaw_deg = 0.0;
    std::uint64_t sequence = 0;
    bool valid = false;
};

struct Config {
    std::string device =
        "/dev/serial/by-id/usb-1a86_USB_Single_Serial_5C67041838-if00";
    // Tope del host. Coincide con el rps_limit del firmware (pos-v6): el motor
    // del proyecto da 5.76 r/s en vacío a 12 V con PWM al 100 %.
    float max_speed_rps = 5.7F;
    // Motor con control de posición en el firmware (0..3 = M1..M4). Debe
    // coincidir con POSITION_MOTOR_INDEX del firmware; el bit correspondiente
    // de MotorPositionCapabilities::motor_mask lo confirma en la placa.
    std::uint8_t position_motor_id = 3;
};

struct BusServoFeedback {
    std::uint8_t id = 0;
    std::int16_t position = 0;
    std::uint64_t sequence = 0;
    bool valid = false;
};

// Valores 0..1000 corresponden aproximadamente a 0..240 grados en HX-12H.
// Se exige pasar los tres puntos de forma explícita para que el programa no
// pueda asumir topes mecánicos peligrosos para el varillaje Ackermann.
struct SteeringCalibration {
    std::uint8_t servo_id = 1;
    std::uint16_t left = 400;
    std::uint16_t center = 500;
    std::uint16_t right = 600;
    std::uint16_t duration_ms = 300;
};

// JGB37-520R30-12, variante 1:30 usada en M1 y en M4 (motor de posición):
// 11 líneas del encoder * cuadratura x4 * reducción 30 = 1320 ticks
// por vuelta del eje de salida.
struct EncoderCalibration {
    std::int32_t ticks_per_output_revolution = 1320;
};

enum class MotorPositionState : std::uint8_t {
    idle = 0,
    moving = 1,
    reached = 2,
    cancelled = 3,
    timeout = 4,
    invalid_command = 5,
    holding = 6,   // HOLD activo (MOTOR_EXT v3)
    stalled = 7,   // HOLD soltado por bloqueo: 3 s al tope de PWM sin moverse
};

// v1: posición sólo en M1 (máscara 0x01). v2: el motor de posición es el bit
// de motor_mask, y ticks_per_revolution es el de ese motor.
struct MotorPositionCapabilities {
    std::uint8_t protocol_version = 0;
    std::uint8_t motor_mask = 0;
    std::uint32_t ticks_per_revolution = 0;
    std::uint64_t sequence = 0;
    bool valid = false;
};

struct MotorPositionStatus {
    std::uint8_t command = 0;
    std::uint8_t motor_id = 0;
    MotorPositionState state = MotorPositionState::idle;
    bool active = false;
    std::int32_t position_ticks = 0;
    std::int32_t target_ticks = 0;
    std::int32_t error_ticks = 0;
    float speed_rps = 0.0F;
    std::uint64_t sequence = 0;
    bool valid = false;
};

class Brick {
public:
    explicit Brick(Config config = {});
    ~Brick();

    Brick(const Brick &) = delete;
    Brick &operator=(const Brick &) = delete;

    void open();
    void close() noexcept;
    bool is_open() const noexcept;

    void set_motor_speed(std::uint8_t motor_id, float speed_rps);
    void stop_motor(std::uint8_t motor_id);
    void stop_all();

    // Motor al que van las órdenes MOTOR_EXT (Config::position_motor_id).
    std::uint8_t position_motor_id() const noexcept {
        return config_.position_motor_id;
    }

    MotorPositionCapabilities motor_position_capabilities(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(800));
    MotorPositionStatus move_motor_relative(
        std::int32_t delta_ticks, float max_rps,
        std::uint16_t tolerance_ticks, std::uint16_t motion_timeout_ms,
        std::chrono::milliseconds reply_timeout = std::chrono::milliseconds(800));
    MotorPositionStatus motor_position_status(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(800));
    MotorPositionStatus cancel_motor_position(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(800));
    MotorPositionStatus zero_motor_position(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(800));
    MotorPositionStatus wait_motor_position(
        std::chrono::milliseconds timeout,
        std::chrono::milliseconds poll_period = std::chrono::milliseconds(50));
    // HOLD de posición (MOTOR_EXT v3, como motor.HOLD del SPIKE): sostiene la
    // posición actual sin timeout y vuelve a ella si se desplaza, con el PWM
    // limitado a max_pulse (300..1000). Lo cancelan una orden de velocidad,
    // move_motor_relative, cancel_motor_position o stop.
    MotorPositionStatus hold_motor_position(
        float max_rps, std::uint16_t tolerance_ticks, std::uint16_t max_pulse,
        std::chrono::milliseconds reply_timeout = std::chrono::milliseconds(800));

    void set_bus_servo_position(std::uint8_t servo_id,
                                std::uint16_t position,
                                std::uint16_t duration_ms);
    void set_bus_servo_power(std::uint8_t servo_id, bool enabled);
    std::uint8_t read_bus_servo_id(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(800)) const;
    BusServoFeedback read_bus_servo_position(
        std::uint8_t servo_id, std::chrono::milliseconds timeout) const;
    void set_steering(double normalized,
                      const SteeringCalibration &calibration);
    void center_steering(const SteeringCalibration &calibration);

    ImuSample imu() const;
    bool wait_for_imu(std::chrono::milliseconds timeout) const;
    void calibrate_gyro(std::chrono::milliseconds duration);
    void reset_yaw(double yaw_deg = 0.0);

    // La API existe en el cliente, pero sólo responde después de instalar el
    // firmware extendido. motor_position_capabilities() es la comprobación
    // operativa que evita asumir que la placa ya fue actualizada.
    static constexpr bool has_motor_position_feedback() noexcept {
        return true;
    }

    // Conversión pura y comprobable. No envía ninguna orden al motor.
    static std::int64_t motor_degrees_to_ticks(
        double degrees,
        const EncoderCalibration &calibration = {});
    static double motor_ticks_to_degrees(
        std::int64_t ticks,
        const EncoderCalibration &calibration = {});

    // Trama IMU del firmware (24 bytes: 3 floats de aceleración en m/s² y 3 de
    // giroscopio en rad/s) -> ImuSample en g y °/s, con roll y pitch del
    // acelerómetro. No integra el yaw ni marca la muestra como válida. Pura.
    static ImuSample decode_imu_payload(const std::uint8_t *payload);

    static std::uint8_t crc8_maxim(const std::uint8_t *data,
                                   std::size_t size) noexcept;
    static std::vector<std::uint8_t>
    encode_frame(std::uint8_t function,
                 const std::vector<std::uint8_t> &payload);

private:
    void send_frame(std::uint8_t function,
                    const std::vector<std::uint8_t> &payload);
    void reader_loop() noexcept;
    void consume_bytes(const std::uint8_t *data, std::size_t size);
    void process_frame(std::uint8_t function,
                       const std::vector<std::uint8_t> &payload);

    Config config_;
    int fd_ = -1;
    bool running_ = false;
    std::thread reader_;
    mutable std::mutex state_mutex_;
    mutable std::condition_variable imu_cv_;
    mutable std::condition_variable servo_cv_;
    mutable std::condition_variable motor_position_cv_;
    std::mutex write_mutex_;
    ImuSample imu_;
    BusServoFeedback bus_servo_;
    MotorPositionCapabilities motor_position_capabilities_;
    MotorPositionStatus motor_position_status_;
    std::array<double, 3> gyro_bias_{{0.0, 0.0, 0.0}};
    std::chrono::steady_clock::time_point last_imu_time_{};
    std::vector<std::uint8_t> rx_buffer_;
};

}  // namespace hiwonder
