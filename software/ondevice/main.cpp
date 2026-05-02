#include "spike.h"
#include "rasp_gpio.h"
#include "RPLidar_S2L.h"
#include "common_var.h"

void signal_handler(int signum);

pthread_t writer;

int der = 1;
int izq = -1;

static float lidar_shared_buffer[360]; // Your shared buffer

int main(){

    float distancia_frente;
    float distancia_derecha;
    float distancia_izquierda;
    int v = 0;
    int angulo_correccion = 0;
    int angulo_correccion_t = 0;
    int angulo_correccion_t2 = 0;
    int angulo = 0;

    signal(SIGINT, signal_handler); /* Set interrupt for ctrl+C */
    RPLidar_S2L_Init_Lidar();
    pthread_create(&writer, NULL, RPLidar_S2L_Lidar_Writer_Thread, NULL);

    Rasp_Gpio_Init();
    Rasp_Gpio_Power_On_Spike();
    Spike_Serial_Init();
    Spike_Interpreter();
    Spike_Initialize_Libraries();
    Rasp_Gpio_Wait_For_Button();
    Spike_Reset_Gyro(0);
    usleep(200000); //wiating for reset gyro

    RPLidar_S2L_Get_Buffer(&lidar_shared_buffer[0] );
    distancia_frente = lidar_shared_buffer[0];
    distancia_derecha = lidar_shared_buffer[270];
    distancia_izquierda = lidar_shared_buffer[90];

    printf("dsitancia derecha : %f\n", distancia_derecha);
    printf("dsitancia izquierda : %f\n", distancia_izquierda);
    printf("dsitancia frente : %f\n", distancia_frente);

    if((distancia_derecha > 700) || (distancia_izquierda > 700)){

        printf("caso afuera\n");

        direction sentido = RPLidar_S2L_Avanzar_Deteccion_Sentido_Lidar(100, 0);
        printf("sentido %d :\n", sentido);

        if(sentido == right){
            Spike_Turn_For_Degrees(der, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 1600, -90);
            angulo_correccion_t = RPLidar_S2L_Correction_For_Triangles_Left(12);
            angulo_correccion = RPLidar_S2L_Avanzar_Dos_Puntos_Izquierda(80, 500, -90);
            angulo_correccion_t2 = RPLidar_S2L_Correction_For_Triangles_Left(12);
            angulo = RPLidar_S2L_Comparation(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            usleep(200000);
            while (v < 11){
            RPLidar_S2L_Avanzar_Deteccion_Vacio_Derecho_Lidar(80, 0);
            Spike_Turn_For_Degrees(der, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 600, -90);
            angulo_correccion_t = RPLidar_S2L_Correction_For_Triangles_Left(12);
            angulo_correccion = RPLidar_S2L_Avanzar_Dos_Puntos_Izquierda(80, 500, -90);
            angulo_correccion_t2 = RPLidar_S2L_Correction_For_Triangles_Left(12);
            angulo = RPLidar_S2L_Comparation(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            printf("v : %d, angulo : %d \n", v, angulo);
            usleep(200000);
            v = v + 1;
            }

        }
        else if (sentido == left)
        {
            Spike_Turn_For_Degrees(izq, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 1600, 90);
            angulo_correccion_t = RPLidar_S2L_Correction_For_Triangles_Right(12);
            angulo_correccion = RPLidar_S2L_Avanzar_Dos_Puntos_Derecha(80, 500, 90);
            angulo_correccion_t2 = RPLidar_S2L_Correction_For_Triangles_Right(12);
            angulo = RPLidar_S2L_Comparation(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            usleep(200000);
            while (v < 11){
            RPLidar_S2L_Avanzar_Deteccion_Vacio_Izquierdo_Lidar(80, 0);
            Spike_Turn_For_Degrees(izq, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 600, 90);
            angulo_correccion_t = RPLidar_S2L_Correction_For_Triangles_Right(12);
            angulo_correccion = RPLidar_S2L_Avanzar_Dos_Puntos_Derecha(80, 500, 90);
            angulo_correccion_t2 = RPLidar_S2L_Correction_For_Triangles_Right(12);
            angulo = RPLidar_S2L_Comparation(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            printf("v : %d, angulo : %d \n", v, angulo);
            usleep(200000);
            v = v + 1;
            }

        }
    }

    else{

        printf("caso adentro\n");

        direction sentido = RPLidar_S2L_Avanzar_Deteccion_Sentido_Lidar(60, 0);
        printf("sentido : %d \n", sentido);

        if(sentido == right){
            Spike_Turn_For_Degrees(der, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 600, -90);
            angulo_correccion_t = RPLidar_S2L_Correction_For_Triangles_Left(12);
            angulo_correccion = RPLidar_S2L_Avanzar_Dos_Puntos_Izquierda(80, 500, -90);
            angulo_correccion_t2 = RPLidar_S2L_Correction_For_Triangles_Left(12);
            angulo = RPLidar_S2L_Comparation(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            usleep(200000);
            while (v < 11){
            RPLidar_S2L_Avanzar_Deteccion_Vacio_Derecho_Lidar(80, 0);
            Spike_Turn_For_Degrees(der, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 600, -90);
            angulo_correccion_t = RPLidar_S2L_Correction_For_Triangles_Left(12);
            angulo_correccion = RPLidar_S2L_Avanzar_Dos_Puntos_Izquierda(80, 500, -90);
            angulo_correccion_t2 = RPLidar_S2L_Correction_For_Triangles_Left(12);
            angulo = RPLidar_S2L_Comparation(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            printf("v : %d, angulo : %d \n", v, angulo);
            usleep(200000);
            v = v + 1;
            }

        }
        else if (sentido == left)
        {
            Spike_Turn_For_Degrees(izq, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 600, 90);
            angulo_correccion_t = RPLidar_S2L_Correction_For_Triangles_Right(12);
            angulo_correccion = RPLidar_S2L_Avanzar_Dos_Puntos_Derecha(80, 500, 90);
            angulo_correccion_t2 = RPLidar_S2L_Correction_For_Triangles_Right(12);
            angulo = RPLidar_S2L_Comparation(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            usleep(200000);
            while (v < 11){
            RPLidar_S2L_Avanzar_Deteccion_Vacio_Izquierdo_Lidar(80, 0);
            Spike_Turn_For_Degrees(izq, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 600, 90);
            angulo_correccion_t = RPLidar_S2L_Correction_For_Triangles_Right(12);
            angulo_correccion = RPLidar_S2L_Avanzar_Dos_Puntos_Derecha(80, 500, 90);
            angulo_correccion_t2 = RPLidar_S2L_Correction_For_Triangles_Right(12);
            angulo = RPLidar_S2L_Comparation(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            printf("v : %d, angulo : %d \n", v, angulo);
            usleep(200000);
            v = v + 1;
            }
        }
    } 
    
    printf("ultima funcion\n");
    RPLidar_S2L_Avanzar_Hasta_La_Distancia(80, 0, 1400);

    Rasp_Gpio_Clean();
    Spike_Coast_Motors();
    Spike_Close_Serial();
    RPLidar_S2L_Close();

    return 0;
}

void signal_handler(int signum){
    printf("\nCtrl+C detceted\n");
    RPLidar_S2L_Set_Terminating();
    signal(SIGINT, SIG_DFL);
}
