#ifndef RETO2_CONFIG_H
#define RETO2_CONFIG_H

// Conversiones del reto 2 (misión escrita para el hub SPIKE) a la placa
// Hiwonder. La calibración del robot (servo de dirección, PD de rumbo,
// velocidad, parada de emergencia) es la del reto 1 y vive en
// ../../ondevice/hiwonder_config.h: lo que se ajuste ahí vale para los dos
// retos. Aquí sólo va lo que el reto 1 no necesita.

// Diámetro de la rueda que mueve M4, en mm. Es lo único que hay que medir:
// con él, Spike_Advance_For_distance(…, mm, …) recorre esos milímetros sin
// pasar por las constantes del LEGO, y los "grados del hub" de
// Spike_Advance_For_Degrees (p. ej. 2250 en el estacionamiento) se convierten
// a la misma distancia que recorría el robot LEGO.
// VERIFICAR: Spike_Advance_For_distance(50, 1000, 0) tiene que recorrer
// 1000 mm; si recorre L mm, multiplicar este valor por L / 1000.
constexpr double kDiametroRuedaMm = 65.0;

// Geometría del hub, tomada de spike.cpp: ag(speed, 1.4 * mm * 360 / 196.035).
// Rueda LEGO de 196.035 mm de perímetro con reducción 1.4 -> mm por grado del
// motor de propulsión del hub.
constexpr double kMmPorGradoSpike = 196.035 / (1.4 * 360.0);

// turn()/small_turn() del hub: run_to_relative_position(port.A,
// tire_turn*4.29*dir). El reto 1 definió el tope de dirección (±25° del
// HX-12H) como 179° de ese motor (kGiroLlantaTope en spike_hiwonder.cpp), así
// que tire_turn 40 = 0.96 del tope, 45 = tope, 30 = 0.72, 15 = 0.36.
constexpr double kGradosMotorPorTireTurn = 4.29;

// turn_special()/turns_special() (await = true) esperaban a que la dirección
// llegara antes de mover. El HX-12H recorre los ±25° en unos 100 ms.
constexpr int kEsperaDireccionAwaitMs = 150;

#endif // RETO2_CONFIG_H
