#ifndef LIDAR_DISPLAY_H
#define LIDAR_DISPLAY_H

// Ventana "Lidar Coordinates" (OpenCV) para ver el lidar por VNC, como la de
// object_detection_original_h10: puntos en mm, paredes segmentadas y, por
// lado, la pared que detecta Oradar_S2L_Wall_Slope con su distancia y grados.
// Sólo lee: no cambia lo que hace el robot.

// false (con un aviso) si no hay DISPLAY: así el servicio de arranque y
// "sudo" siguen corriendo sin ventana.
bool Lidar_Display_Available(void);
// Dibuja la ventana hasta que *terminado deje de ser 0. Llamar desde el hilo
// principal: el backend Qt de OpenCV no admite la ventana en otro hilo. Si la
// ventana falla, avisa y espera igual, sin parar el recorrido.
void Lidar_Display_Loop(volatile int *terminado);
// Muestra el yaw del giroscopio de la placa. Llamar sólo después de que
// Spike_Hiwonder_Init() funcione: antes, la placa puede no existir todavía.
void Lidar_Display_Enable_Gyro(void);

#endif // LIDAR_DISPLAY_H
