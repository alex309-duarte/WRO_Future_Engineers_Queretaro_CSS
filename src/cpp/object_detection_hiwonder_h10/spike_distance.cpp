#include "spike.h"

#include "hiwonder_runtime.h"

#include <cmath>
#include <limits>
#include <stdexcept>

extern "C" void Spike_Advance_For_distance(int speed, int distance,
                                            int reference) {
    constexpr double pi = 3.14159265358979323846;
    const double circumference = pi * Hiwonder_Wheel_Diameter_Mm();
    const double degrees =
        (static_cast<double>(distance) / circumference) * 360.0 *
        Hiwonder_Distance_Scale();
    if (!std::isfinite(degrees) ||
        degrees < static_cast<double>(std::numeric_limits<int>::min()) ||
        degrees > static_cast<double>(std::numeric_limits<int>::max())) {
        throw std::out_of_range("distancia fuera del rango de M1");
    }
    Spike_Advance_For_Degrees(speed, static_cast<int>(std::lround(degrees)),
                              reference);
}
