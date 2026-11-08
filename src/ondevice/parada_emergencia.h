#ifndef PARADA_EMERGENCIA_H
#define PARADA_EMERGENCIA_H

// Parada de emergencia por señal (Ctrl+C, kill, cierre de la terminal, Ctrl+Z
// y caídas). El firmware no tiene watchdog de comandos: si el programa muere
// con el motor andando, sigue a la última velocidad. Al recibir la señal, el
// manejador manda a la placa la parada de los cuatro motores (en COAST: el
// firmware pone a 0 las dos entradas del puente H) y, si se pide, quita el par
// al servo de dirección. Luego termina el proceso sin volver al programa, así
// ningún bucle a medias puede mandar otra velocidad después de la parada.

#include <cstddef>
#include <cstdint>

// Abre un segundo descriptor del puerto de la placa (la Brick ya lo configuró
// a 1 Mbaud) y prepara las tramas. Instalar después de Brick::open().
// Lanza std::runtime_error si no puede abrir el puerto.
void Parada_Emergencia_Instalar(const char *dispositivo, std::uint8_t servo_direccion,
                                bool soltar_direccion);

// Otro puerto que también hay que parar (el lidar: si el proceso termina sin
// pararlo, su motor sigue girando). Abre un segundo descriptor de
// "dispositivo" (ya configurado por su driver) y copia "tramas", que el
// manejador escribe después de las de la placa. Instala el manejador si
// todavía no lo estaba, así cubre también el arranque antes de la placa.
// Lanza std::runtime_error si no puede abrir el puerto.
void Parada_Emergencia_Agregar_Puerto(const char *dispositivo, const unsigned char *tramas,
                                      std::size_t largo);

#endif // PARADA_EMERGENCIA_H
