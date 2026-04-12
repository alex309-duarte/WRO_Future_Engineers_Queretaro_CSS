#include "rasp_gpio.h"

unsigned int intput_GPIO[] = {GPIO_BUTTON};
unsigned int outputs_GPIO[] = {GPIO_LED_BUTTON, GPIO_RELE};
const char *chip_path = "/dev/gpiochip4";

struct gpiod_chip *chip;
struct gpiod_line_settings *settings_input;
struct gpiod_line_settings *settings_outputs;
struct gpiod_line_config *line_cfg;
struct gpiod_request_config *req_cfg;
struct gpiod_line_request *request;

int Rasp_Gpio_Init(void){
    chip = gpiod_chip_open(chip_path);
    if (!chip) {
        printf("Failed to open GPIO chip");
        return EXIT_FAILURE;
    }

    settings_input = gpiod_line_settings_new();
    settings_outputs = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings_input, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_direction(settings_outputs, GPIOD_LINE_DIRECTION_OUTPUT);

    gpiod_line_settings_set_output_value(settings_outputs, GPIOD_LINE_VALUE_INACTIVE);
    gpiod_line_settings_set_bias(settings_input, GPIOD_LINE_BIAS_PULL_UP);

    line_cfg = gpiod_line_config_new();

    gpiod_line_config_add_line_settings(line_cfg, intput_GPIO, 1, settings_input);
    gpiod_line_config_add_line_settings(line_cfg, outputs_GPIO, 2, settings_outputs);

    req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "First challenge program");

    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    return 0;
}

void Rasp_Gpio_Clean(void){
    gpiod_line_request_release(request);
    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings_input);
    gpiod_line_settings_free(settings_outputs);
    gpiod_chip_close(chip);
    printf("Pines liberados correctamente.\n");
}

void Rasp_Gpio_Wait_For_Button(void){
    while(gpiod_line_request_get_value(request, intput_GPIO[0]) == GPIOD_LINE_VALUE_ACTIVE){
        gpiod_line_request_set_value(request, outputs_GPIO[0], GPIOD_LINE_VALUE_ACTIVE);
        usleep(50000);
        gpiod_line_request_set_value(request, outputs_GPIO[0], GPIOD_LINE_VALUE_INACTIVE);
        usleep(50000);
    }
    gpiod_line_request_set_value(request, outputs_GPIO[0], GPIOD_LINE_VALUE_INACTIVE);
}

void Rasp_Gpio_Power_On_Spike(void){
    gpiod_line_request_set_value(request, outputs_GPIO[1], GPIOD_LINE_VALUE_ACTIVE);
    usleep(500000);
    gpiod_line_request_set_value(request, outputs_GPIO[1], GPIOD_LINE_VALUE_INACTIVE);
    usleep(1000000); //one second waiting for spikr initialization 
}