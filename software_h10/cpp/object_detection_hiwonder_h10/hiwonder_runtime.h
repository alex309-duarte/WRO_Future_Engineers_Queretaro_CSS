#pragma once

// El modo armado nunca es implícito. Sin HIWONDER_ARM=1 el programa conserva
// cámara y LiDAR, pero no abre el controlador ni arranca el hilo de misión.
bool Hiwonder_Arm_Requested();

// Lee y valida todas las variables HIWONDER_*, abre USART3 a 1 Mbaud,
// comprueba MOTOR_EXT/IMU y enlaza spike_compat. Lanza std::exception al fallar.
void Hiwonder_Initialize_From_Environment();
void Hiwonder_Shutdown() noexcept;
bool Hiwonder_Is_Ready() noexcept;

// Parámetros usados por Spike_Advance_For_distance.
double Hiwonder_Wheel_Diameter_Mm();
double Hiwonder_Distance_Scale();
