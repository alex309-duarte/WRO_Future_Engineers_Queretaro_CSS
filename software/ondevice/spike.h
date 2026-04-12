#ifndef SPIKE_H
#define SPIKE_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdarg.h>


int Spike_Serial_Init(void);
void Spike_Close_Serial(void);
void Spike_Send_Serial_Data(char data[]);
char* Spike_Read_Serial_Data(void);
void Spike_Interpreter(void);
void Spike_Initialize_Libraries(void);
void Spike_End_Function(void);
void Spike_Center_Vehicle(void);
void Spike_Center_Vehicle_Short(void);
void Spike_Coast_Motors(void);
void Spike_Hold_Motors(void);
void Spike_Reset_Gyro(int degrees);
void Spike_Print_Gyro(void);
void Spike_Concatenate(int list_lenght,const char * argument_1[],char * buffer);
void Spike_Turn_For_Degrees(int direction, int speed, int degrees);
void Spike_Advance_For_Degrees(int speed, int degrees, int reference);
void Spike_Forward(int speed, int reference);

// Contenido del archivo de cabecera (prototipos, definiciones, etc.)

#endif // NOMBRE_ARCHIVO_H