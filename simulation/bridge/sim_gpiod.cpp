// libgpiod v2 stand-in for the digital twin; see include/gpiod.h. The
// repository's rasp_gpio.cpp runs unchanged on top of it: line 4 (start
// button, pull-up, pressed = low) reads the START button of the scene, and
// line 3 (the LED inside the button) lights the LED of the simulated Grove
// button. WRO_SIM_AUTOSTART=<seconds> presses the button by itself after that
// many seconds of waiting, for unattended runs.
#include "gpiod.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace wro_sim {
bool Board_Button_Pressed();
void Board_Led(bool on);
void Board_Press_Button();
}

struct gpiod_chip { int dummy = 0; };
struct gpiod_line_settings { int dummy = 0; };
struct gpiod_line_config { int dummy = 0; };
struct gpiod_request_config { int dummy = 0; };
struct gpiod_line_request { int dummy = 0; };

namespace {
constexpr unsigned int kButtonLine = 4;  // GPIO_BUTTON in rasp_gpio.h
constexpr unsigned int kLedLine = 3;     // GPIO_LED_BUTTON
bool g_led = false;
bool g_waiting = false;
std::chrono::steady_clock::time_point g_wait_start;
}  // namespace

gpiod_chip *gpiod_chip_open(const char *path) {
    std::printf("Twin: GPIO chip %s -> start button of the CoppeliaSim scene\n", path);
    return new gpiod_chip;
}
void gpiod_chip_close(gpiod_chip *chip) { delete chip; }
gpiod_line_settings *gpiod_line_settings_new(void) { return new gpiod_line_settings; }
void gpiod_line_settings_free(gpiod_line_settings *s) { delete s; }
int gpiod_line_settings_set_direction(gpiod_line_settings *, gpiod_line_direction) { return 0; }
int gpiod_line_settings_set_output_value(gpiod_line_settings *, gpiod_line_value) { return 0; }
int gpiod_line_settings_set_bias(gpiod_line_settings *, gpiod_line_bias) { return 0; }
gpiod_line_config *gpiod_line_config_new(void) { return new gpiod_line_config; }
void gpiod_line_config_free(gpiod_line_config *c) { delete c; }
int gpiod_line_config_add_line_settings(gpiod_line_config *, const unsigned int *, size_t,
                                        gpiod_line_settings *) {
    return 0;
}
gpiod_request_config *gpiod_request_config_new(void) { return new gpiod_request_config; }
void gpiod_request_config_free(gpiod_request_config *c) { delete c; }
void gpiod_request_config_set_consumer(gpiod_request_config *, const char *) {}
gpiod_line_request *gpiod_chip_request_lines(gpiod_chip *, gpiod_request_config *, gpiod_line_config *) {
    return new gpiod_line_request;
}
void gpiod_line_request_release(gpiod_line_request *r) {
    if (g_led) {
        try { wro_sim::Board_Led(false); } catch (...) {}
        g_led = false;
    }
    delete r;
}

gpiod_line_value gpiod_line_request_get_value(gpiod_line_request *, unsigned int offset) {
    if (offset != kButtonLine) return GPIOD_LINE_VALUE_INACTIVE;
    if (!g_waiting) {
        g_waiting = true;
        g_wait_start = std::chrono::steady_clock::now();
        std::printf("Twin: waiting for the START button (scene window)\n");
    }
    const char *autostart = std::getenv("WRO_SIM_AUTOSTART");
    if (autostart != nullptr && *autostart != '\0') {
        const double after = std::atof(autostart);
        const double waited = std::chrono::duration<double>(std::chrono::steady_clock::now() -
                                                             g_wait_start).count();
        if (waited >= after) {
            try { wro_sim::Board_Press_Button(); } catch (...) {}
        }
    }
    bool pressed = false;
    try {
        pressed = wro_sim::Board_Button_Pressed();
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Twin: %s\n", e.what());
    }
    if (pressed) g_waiting = false;
    // Pull-up: released = high (ACTIVE), pressed = low (INACTIVE).
    return pressed ? GPIOD_LINE_VALUE_INACTIVE : GPIOD_LINE_VALUE_ACTIVE;
}

int gpiod_line_request_set_value(gpiod_line_request *, unsigned int offset, gpiod_line_value value) {
    if (offset != kLedLine) return 0;
    const bool on = value == GPIOD_LINE_VALUE_ACTIVE;
    if (on != g_led) {
        try { wro_sim::Board_Led(on); } catch (...) {}
        g_led = on;
    }
    return 0;
}
