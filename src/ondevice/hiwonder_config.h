#ifndef HIWONDER_CONFIG_H
#define HIWONDER_CONFIG_H

// Calibración del robot con la placa Hiwonder: todo lo que depende del montaje
// está aquí. Los valores marcados VERIFICAR vienen del banco (2026-10-04), no
// del robot armado; HIWONDER.md explica cómo comprobarlos.

#include <cstdint>

// Puerto de la placa (CH343). El número de serie es el de ESTA placa: si se
// cambia, ver ls /dev/serial/by-id/.
#define HIWONDER_DISPOSITIVO "/dev/serial/by-id/usb-1a86_USB_Single_Serial_5C67041838-if00"

// --- Dirección: servo bus HX-12H -------------------------------------------
// Posiciones 0..1000 (~0.24° por unidad). En banco, con la derecha en la
// posición mayor la corrección de rumbo salía al revés, por eso aquí la
// derecha es la menor. VERIFICAR: con las ruedas levantadas, la posición
// derecha tiene que girar las ruedas a la derecha.
constexpr std::uint8_t  kServoDireccion     = 1;
constexpr std::uint16_t kDireccionCentro    = 500;
constexpr std::uint16_t kDireccionDerecha   = 300;
constexpr std::uint16_t kDireccionIzquierda = 700;
constexpr std::uint16_t kDireccionMs        = 50;

// --- Rumbo: PD de spike_compat ---------------------------------------------
// Salida del PD -> dirección normalizada -1..1. Con 0.133 y ±200, 1° de
// error mueve el servo 26.7 unidades, la sensibilidad del hub. AJUSTAR en pista.
constexpr double kPdADireccion           = 0.133;
constexpr double kBandaMuertaRumboGrados = 0.3;

// --- Propulsión: motor M4, con control de posición en el firmware ----------
// speed=100 del hub -> r/s. 5.7 es el tope del firmware (pos-v8). El
// ambiente WRO_MAX_RPS lo puede bajar para las primeras pruebas.
constexpr double kRpsVelocidad100 = 5.7;
// Grados del motor del hub -> ticks del encoder de M4 (1320 por vuelta,
// medido). Sólo es 1:1 si rueda y transmisión avanzan lo mismo por vuelta
// que en el robot LEGO. CALIBRAR: Spike_Advance_For_Degrees(100, 1350, 0)
// tiene que recorrer lo mismo que con el hub.
constexpr double kGradosSpikeATicks = 1320.0 / 360.0;

// Calibración del sesgo del giroscopio al arrancar, con el robot quieto. En
// banco, 2 s dejaban ~0.5° de deriva en 20 s; el error baja con la raíz del
// tiempo, así que 5 s lo reducen ~1.6 veces.
constexpr int kSegundosCalibracionGiro = 5;

#endif // HIWONDER_CONFIG_H
