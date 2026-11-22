// Emergency stop for the digital twin, linked instead of
// src/ondevice/parada_emergencia.cpp (which writes stop frames to the serial
// ports of the board and the LiDAR). Same contract: on Ctrl+C, kill, a closed
// terminal, Ctrl+Z or a crash the drive motor is left in COAST, the steering
// servo torque is released if requested, and the process ends without
// returning to the program.
#include "parada_emergencia.h"  // src/ondevice

#include <csignal>
#include <initializer_list>
#include <cstdio>
#include <unistd.h>

namespace wro_sim {
void Board_Emergency_Stop(bool release_steering);
}

namespace {

bool g_installed = false;
bool g_release_steering = true;

void Handler(int signum) {
    static volatile sig_atomic_t running = 0;
    if (running) _exit(128 + signum);
    running = 1;
    const char text[] = "\nEmergency stop: drive motor to COAST in the twin\n";
    (void)!write(STDERR_FILENO, text, sizeof(text) - 1);
    try {
        wro_sim::Board_Emergency_Stop(g_release_steering);
    } catch (...) {
    }
    _exit(128 + signum);
}

void Install() {
    if (g_installed) return;
    for (int s : {SIGINT, SIGTERM, SIGHUP, SIGTSTP, SIGQUIT, SIGSEGV, SIGABRT, SIGBUS, SIGFPE}) {
        std::signal(s, Handler);
    }
    g_installed = true;
}

}  // namespace

void Parada_Emergencia_Instalar(const char *, std::uint8_t, bool soltar_direccion) {
    g_release_steering = soltar_direccion;
    Install();
}

// The simulated LiDAR has no motor to stop; the handler is installed as on
// the robot so the start-up phase is covered too.
void Parada_Emergencia_Agregar_Puerto(const char *, const unsigned char *, std::size_t) {
    Install();
}
