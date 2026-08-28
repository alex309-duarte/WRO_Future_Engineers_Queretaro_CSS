#pragma once

// Mantiene la API C usada por la misión original, pero la implementación
// delega en hiwonder::Brick y no en el REPL MicroPython del LEGO SPIKE.
#include "spike_compat/spike_compat.h"

extern "C" {
void Spike_Advance_For_distance(int speed, int distance, int reference);
}
