#ifndef SPIKE_H
#define SPIKE_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdarg.h>
#include <gpiod.h>

int serial_init(void);
void close_serial(void);
void send_serial_data(char data[]);
char* read_data(void);
void interpreter(void);
void initialize_Libraries(void);
void end_funcion(void);
void centrar_vehiculo(void);
void centrar_vehiculo_corto(void);
void Coast_motors(void);
void Hold_motors(void);
void reset_gyro(int grados);
void print_gyro(void);
void concatenar(int list_lenght,const char * argument_1[],char * buffer);
void vuelta_grados(int direccion, int velocidad, int grados);
void avanzar_grados(int velocidad, int grados, int referencia);
int init_gpio(void);
void clean_GPIO(void);
void wait_for_button(void);
void power_On_Spike(void);
void Spike_forward(int velocidad, int referencia);

// Contenido del archivo de cabecera (prototipos, definiciones, etc.)

#endif // NOMBRE_ARCHIVO_H