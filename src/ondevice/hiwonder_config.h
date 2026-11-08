#ifndef HIWONDER_CONFIG_H
#define HIWONDER_CONFIG_H

// Calibración del robot con la placa Hiwonder: todo lo que depende del montaje
// está aquí. Los valores marcados VERIFICAR vienen del banco (2026-10-04), no
// del robot armado; HIWONDER.md explica cómo comprobarlos.

#include <cstdint>

// Puerto de la placa (CH343): se descubre solo al arrancar, así sirve
// cualquier placa. Con varias placas conectadas hay que elegir una:
// HIWONDER_DEVICE=/dev/serial/by-id/usb-1a86_USB_Single_Serial_<serie>-if00

// --- Dirección: servo bus HX-12H -------------------------------------------
// Posiciones 0..1000 (~0.24° por unidad). En banco, con la derecha en la
// posición mayor la corrección de rumbo salía al revés, por eso aquí la
// derecha es la menor. VERIFICAR: con las ruedas levantadas, la posición
// derecha tiene que girar las ruedas a la derecha.
//
// TOPE DE DIRECCIÓN: ±25° desde el centro; más grados y la rueda pega con la
// base. Todo movimiento de dirección (PD, giros a tope, centrado) pasa por
// set_steering, que acota a ±1 = estas posiciones, así que éste es el tope
// duro. 25° / 0.24 = 104.2 unidades; se redondea hacia abajo (104 = 24.96°).
constexpr double        kGradosPorUnidadServo = 0.24;
constexpr double        kDireccionMaxGrados   = 35.0;
constexpr std::uint16_t kDireccionRecorrido   =
    static_cast<std::uint16_t>(kDireccionMaxGrados / kGradosPorUnidadServo);  // 104
constexpr std::uint8_t  kServoDireccion     = 1;
// Centro = ruedas rectas. 180° del servo (180 / 0.24 = 750): el varillaje se
// arma con el servo en esta posición, para que coincida con los tornillos.
constexpr std::uint16_t kDireccionCentro    = 750;
constexpr std::uint16_t kDireccionDerecha   = kDireccionCentro - kDireccionRecorrido;  // 646
constexpr std::uint16_t kDireccionIzquierda = kDireccionCentro + kDireccionRecorrido;  // 854
constexpr std::uint16_t kDireccionMs        = 50;
static_assert(kDireccionRecorrido * kGradosPorUnidadServo <= kDireccionMaxGrados,
              "el recorrido de la dirección supera el tope de grados");
// El servo sólo acepta órdenes 0..1000: el centro tiene que dejar ±104 libres.
// Con el centro en 0 un lado quedaría en -104, imposible de ordenar. Si recto
// cae cerca de 0 o de 1000, hay que remontar el horn del servo.
static_assert(kDireccionCentro >= kDireccionRecorrido &&
              kDireccionCentro + kDireccionRecorrido <= 1000,
              "kDireccionCentro no deja ±25° dentro de 0..1000: remontar el horn");
// Al arrancar, el servo tiene que estar ya dentro de [derecha, izquierda] con
// este margen (21 unidades = ~5°); si no, Spike_Hiwonder_Init() se niega a
// moverlo. Centrar desde fuera del recorrido llevaría la rueda contra la base.
constexpr int kDireccionMargenArranque = 21;
// Parada de emergencia (Ctrl+C): además de dejar los motores en COAST, quita
// el par al servo para que no empuje el varillaje contra un tope. Las ruedas
// quedan libres. false = el servo sostiene la última posición.
constexpr bool kSoltarDireccionAlParar = true;

// --- Rumbo: PD de spike_compat ---------------------------------------------
// Salida del PD -> dirección normalizada -1..1. Con 0.133 y ±200 unidades, 1°
// de error movía el servo 26.7 unidades, la sensibilidad del hub. Se escala
// con el recorrido para conservarla con el tope de ±25° (0.256 con ±104): la
// corrección satura antes, pero responde igual a errores pequeños. AJUSTAR en
// pista.
constexpr double kPdADireccion           = 0.133 * 200.0 / kDireccionRecorrido;
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
// Spike_Advance_For_Degrees termina cuando faltan 40 ticks (~11 grados del
// hub, ~1/33 de vuelta) y deja la propulsión en COAST, como ag() del hub. Si
// espera al firmware (0), el asentamiento final dentro de ±15 ticks añade
// hasta ~1 s de avance lento al final de cada tramo. AJUSTAR en pista: el
// robot aún rueda un poco al soltar.
constexpr std::uint16_t kTicksFinAvance = 40;

// Calibración del sesgo del giroscopio al arrancar, con el robot quieto. En
// banco, 2 s dejaban ~0.5° de deriva en 20 s; el error baja con la raíz del
// tiempo, así que 5 s lo reducen ~1.6 veces.
constexpr int kSegundosCalibracionGiro = 5;

#endif // HIWONDER_CONFIG_H
