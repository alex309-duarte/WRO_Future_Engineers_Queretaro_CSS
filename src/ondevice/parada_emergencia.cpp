// Parada de emergencia por señal: ver parada_emergencia.h.
//
// El manejador sólo usa funciones seguras en una señal (write, tcdrain,
// pthread_kill, _exit) sobre tramas ya armadas en Parada_Emergencia_Instalar
// y Parada_Emergencia_Agregar_Puerto.
// No pasa por hiwonder::Brick: sus mutex pueden estar tomados por el hilo
// interrumpido y la parada se quedaría esperando con el motor andando.

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

constexpr std::uint8_t kFuncionMotor = 3;
constexpr std::uint8_t kFuncionServoBus = 5;

int g_fd = -1;
pthread_t g_hilo_principal;
unsigned char g_tramas[128];
std::size_t g_largo = 0;

struct PuertoExtra {
    int fd = -1;
    unsigned char tramas[32];
    std::size_t largo = 0;
};
PuertoExtra g_extras[2];
int g_num_extras = 0;
bool g_instalado = false;

const int kSenales[] = {SIGINT, SIGTERM, SIGHUP, SIGQUIT, SIGTSTP,
                        SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT};

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

void manejador(int senal) {
    // Las órdenes de movimiento salen sólo del hilo principal. Si la señal cayó
    // en otro hilo (lidar, lector de la placa), se pasa al principal: así,
    // mientras se para, el principal no puede mandar otra velocidad.
    if (!pthread_equal(pthread_self(), g_hilo_principal)) {
        pthread_kill(g_hilo_principal, senal);
        return;
    }
    static const char aviso[] = "\nParada de emergencia: motores en COAST\n";
    static const char aviso_extra[] = "Parada de emergencia: lidar parado\n";

    // Antes de instalar la placa, g_fd es -1 y sólo se paran los extras.
    if (g_fd >= 0) {
        (void)!::write(STDERR_FILENO, aviso, sizeof(aviso) - 1);
        // Tres veces: si la señal cortó una trama a medias del programa, la placa
        // puede tragarse los primeros bytes de la siguiente como parte de aquella.
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

void agregar(std::uint8_t funcion, const std::vector<std::uint8_t> &datos) {
    const auto trama = hiwonder::Brick::encode_frame(funcion, datos);
    if (g_largo + trama.size() > sizeof(g_tramas)) {
        throw std::logic_error("parada de emergencia: tramas demasiado largas");
    }
    std::memcpy(g_tramas + g_largo, trama.data(), trama.size());
    g_largo += trama.size();
}

void instalar_manejador() {
    g_hilo_principal = pthread_self();
    if (g_instalado) return;
    struct sigaction accion {};
    accion.sa_handler = manejador;
    sigfillset(&accion.sa_mask);  // nada interrumpe la parada
    accion.sa_flags = SA_RESTART;
    for (const int senal : kSenales) {
        sigaction(senal, &accion, nullptr);
    }
    g_instalado = true;
}

}  // namespace

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
    ++g_num_extras;  // después de llenarlo: el manejador sólo ve puertos completos
    instalar_manejador();
}

void Parada_Emergencia_Instalar(const char *dispositivo, std::uint8_t servo_direccion,
                                bool soltar_direccion) {
    g_largo = 0;
    agregar(kFuncionMotor, {3, 0x0F});  // parar M1..M4 de una vez
    for (std::uint8_t motor = 0; motor < 4; ++motor) {
        agregar(kFuncionMotor, {2, motor});  // y uno por uno, por si acaso
    }
    if (soltar_direccion) {
        agregar(kFuncionServoBus, {0x0B, servo_direccion});  // quitar par
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
