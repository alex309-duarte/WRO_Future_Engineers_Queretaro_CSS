// Stand-in for libgpiod v2 (digital twin build only). The repository's start
// button driver (object_detection_hiwonder_h10/rasp_gpio.cpp) is compiled
// unchanged against it; sim_gpiod.cpp reads the button and drives its LED in
// CoppeliaSim instead of the Raspberry Pi header.
#pragma once

#include <cstddef>

enum gpiod_line_direction {
    GPIOD_LINE_DIRECTION_AS_IS = 1,
    GPIOD_LINE_DIRECTION_INPUT,
    GPIOD_LINE_DIRECTION_OUTPUT,
};
enum gpiod_line_value {
    GPIOD_LINE_VALUE_ERROR = -1,
    GPIOD_LINE_VALUE_INACTIVE = 0,
    GPIOD_LINE_VALUE_ACTIVE = 1,
};
enum gpiod_line_bias {
    GPIOD_LINE_BIAS_AS_IS = 1,
    GPIOD_LINE_BIAS_UNKNOWN,
    GPIOD_LINE_BIAS_DISABLED,
    GPIOD_LINE_BIAS_PULL_UP,
    GPIOD_LINE_BIAS_PULL_DOWN,
};

struct gpiod_chip;
struct gpiod_line_settings;
struct gpiod_line_config;
struct gpiod_request_config;
struct gpiod_line_request;

gpiod_chip *gpiod_chip_open(const char *path);
void gpiod_chip_close(gpiod_chip *chip);
gpiod_line_settings *gpiod_line_settings_new(void);
void gpiod_line_settings_free(gpiod_line_settings *settings);
int gpiod_line_settings_set_direction(gpiod_line_settings *settings, gpiod_line_direction direction);
int gpiod_line_settings_set_output_value(gpiod_line_settings *settings, gpiod_line_value value);
int gpiod_line_settings_set_bias(gpiod_line_settings *settings, gpiod_line_bias bias);
gpiod_line_config *gpiod_line_config_new(void);
void gpiod_line_config_free(gpiod_line_config *config);
int gpiod_line_config_add_line_settings(gpiod_line_config *config, const unsigned int *offsets,
                                        size_t num_offsets, gpiod_line_settings *settings);
gpiod_request_config *gpiod_request_config_new(void);
void gpiod_request_config_free(gpiod_request_config *config);
void gpiod_request_config_set_consumer(gpiod_request_config *config, const char *consumer);
gpiod_line_request *gpiod_chip_request_lines(gpiod_chip *chip, gpiod_request_config *req_cfg,
                                             gpiod_line_config *line_cfg);
void gpiod_line_request_release(gpiod_line_request *request);
gpiod_line_value gpiod_line_request_get_value(gpiod_line_request *request, unsigned int offset);
int gpiod_line_request_set_value(gpiod_line_request *request, unsigned int offset, gpiod_line_value value);
