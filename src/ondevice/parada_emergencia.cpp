// Emergency stop on signals: see parada_emergencia.h.
//
// The handler only uses async-signal-safe functions (write, tcdrain,
// pthread_kill, _exit) on frames already built in Parada_Emergencia_Instalar
// and Parada_Emergencia_Agregar_Puerto.
// It does not go through hiwonder::Brick: its mutexes may be held by the
// interrupted thread, and the stop would then wait forever with the motor
// running.

#include "parada_emergencia.h"

#include "hiwonder_brick/hiwonder_brick.hpp"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>

namespace {

// Function codes of the Hiwonder board packet protocol.
constexpr std::uint8_t kFuncionMotor = 3;      // DC motor commands
constexpr std::uint8_t kFuncionServoBus = 5;   // serial bus servo commands

// State read by the signal handler. Everything is prepared before the
// handler is installed, so the handler never allocates or locks.
int g_fd = -1;                  // second descriptor on the board port (-1 = not installed)
pthread_t g_hilo_principal;     // the only thread that sends motion commands
unsigned char g_tramas[128];    // pre-encoded board stop frames
std::size_t g_largo = 0;        // bytes used in g_tramas

// Additional port to stop (the lidar), with its own raw stop frames.
struct PuertoExtra {
    int fd = -1;
    unsigned char tramas[32];
    std::size_t largo = 0;
};
PuertoExtra g_extras[2];
int g_num_extras = 0;
bool g_instalado = false;

// Termination signals (Ctrl+C, kill, hang-up, quit, Ctrl+Z) and crash signals.
const int kSenales[] = {SIGINT, SIGTERM, SIGHUP, SIGQUIT, SIGTSTP,
                        SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT};

// Writes all "largo" bytes, retrying on partial writes, EINTR and EAGAIN.
// Gives up silently on any other error (nothing else can be done from a
// signal handler).
void escribir_todo(int fd, const unsigned char *datos, std::size_t largo) {
    while (largo > 0) {
        const ssize_t n = ::write(fd, datos, largo);
        if (n > 0) {
            datos += n;
            largo -= static_cast<std::size_t>(n);
        } else if (n < 0 && errno != EINTR && errno != EAGAIN) {
            return;
        }
    }
}

// Signal handler: stops the motors and the lidar, then _exit(128 + signal).
void manejador(int senal) {
    // Motion commands are only sent from the main thread. If the signal was
    // delivered to another thread (lidar, board reader), it is forwarded to
    // the main thread: that way the main thread cannot send another speed
    // while the stop is in progress.
    if (!pthread_equal(pthread_self(), g_hilo_principal)) {
        pthread_kill(g_hilo_principal, senal);
        return;
    }
    static const char aviso[] = "\nParada de emergencia: motores en COAST\n";
    static const char aviso_extra[] = "Parada de emergencia: lidar parado\n";

    // Before the board is installed g_fd is -1 and only the extra ports are stopped.
    if (g_fd >= 0) {
        (void)!::write(STDERR_FILENO, aviso, sizeof(aviso) - 1);
        // Sent three times: if the signal interrupted a frame of the program
        // half-way, the board may absorb the first bytes of the next frame as
        // part of the interrupted one.
        for (int i = 0; i < 3; ++i) {
            escribir_todo(g_fd, g_tramas, g_largo);
        }
        tcdrain(g_fd);
    }
    for (int p = 0; p < g_num_extras; ++p) {
        for (int i = 0; i < 2; ++i) {
            escribir_todo(g_extras[p].fd, g_extras[p].tramas, g_extras[p].largo);
        }
        tcdrain(g_extras[p].fd);
        (void)!::write(STDERR_FILENO, aviso_extra, sizeof(aviso_extra) - 1);
    }
    _exit(128 + senal);
}

// Encodes one board packet (function code + payload) and appends it to
// g_tramas. Throws std::logic_error if the buffer would overflow.
void agregar(std::uint8_t funcion, const std::vector<std::uint8_t> &datos) {
    const auto trama = hiwonder::Brick::encode_frame(funcion, datos);
    if (g_largo + trama.size() > sizeof(g_tramas)) {
        throw std::logic_error("parada de emergencia: tramas demasiado largas");
    }
    std::memcpy(g_tramas + g_largo, trama.data(), trama.size());
    g_largo += trama.size();
}

// Records the calling thread as the main thread and installs the handler
// for every signal in kSenales (only once).
void instalar_manejador() {
    g_hilo_principal = pthread_self();
    if (g_instalado) return;
    struct sigaction accion {};
    accion.sa_handler = manejador;
    sigfillset(&accion.sa_mask);  // nothing interrupts the stop
    accion.sa_flags = SA_RESTART;
    for (const int senal : kSenales) {
        sigaction(senal, &accion, nullptr);
    }
    g_instalado = true;
}

}  // namespace

// See parada_emergencia.h. At most 2 extra ports of up to 32 bytes of frames.
void Parada_Emergencia_Agregar_Puerto(const char *dispositivo, const unsigned char *tramas,
                                      std::size_t largo) {
    if (g_num_extras >= static_cast<int>(sizeof(g_extras) / sizeof(g_extras[0])) ||
        largo > sizeof(g_extras[0].tramas)) {
        throw std::logic_error("parada de emergencia: demasiados puertos o tramas");
    }
    const int fd = ::open(dispositivo, O_WRONLY | O_NOCTTY | O_CLOEXEC);
    if (fd < 0) {
        throw std::runtime_error(std::string("parada de emergencia: no se pudo abrir ") +
                                 dispositivo + ": " + std::strerror(errno));
    }
    PuertoExtra &extra = g_extras[g_num_extras];
    std::memcpy(extra.tramas, tramas, largo);
    extra.largo = largo;
    extra.fd = fd;
    ++g_num_extras;  // after filling it: the handler only sees complete ports
    instalar_manejador();
}

// See parada_emergencia.h. Builds the stop frames, then opens the port
// (replacing a previous descriptor) and installs the handler.
void Parada_Emergencia_Instalar(const char *dispositivo, std::uint8_t servo_direccion,
                                bool soltar_direccion) {
    g_largo = 0;
    agregar(kFuncionMotor, {3, 0x0F});  // stop M1..M4 at once (bit mask 0x0F)
    for (std::uint8_t motor = 0; motor < 4; ++motor) {
        agregar(kFuncionMotor, {2, motor});  // and one by one, as a redundant stop
    }
    if (soltar_direccion) {
        agregar(kFuncionServoBus, {0x0B, servo_direccion});  // steering servo torque off
    }

    const int fd = ::open(dispositivo, O_WRONLY | O_NOCTTY | O_CLOEXEC);
    if (fd < 0) {
        throw std::runtime_error(std::string("parada de emergencia: no se pudo abrir ") +
                                 dispositivo + ": " + std::strerror(errno));
    }
    if (g_fd >= 0) ::close(g_fd);
    g_fd = fd;
    instalar_manejador();
}
