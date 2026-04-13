#include "spike.h"
#include "rasp_gpio.h"
#include "RPLidar_S2L.h"
#include "common_var.h"

pthread_t writer;

int der = 1;
int izq = -1;

static float lidar_shared_buffer[360]; // Your shared buffer

int main(){

    float distancia_frente;
    float distancia_derecha;
    float distancia_izquierda;
    int v = 0;

    signal(SIGINT, signal_handler); /* Set interrupt for ctrl+C */
    init_lidar();
    pthread_create(&writer, NULL, lidar_writer_thread, NULL);
    
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

        direction sentido = avanzar_deteccion_sentido_lidar(100, 0);
        printf("sentido %d :\n", sentido);

        if(sentido == right){
            Spike_Turn_For_Degrees(der, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 1600, -90);
            int angulo_correccion = avanzar_dos_puntos_izquierda(80, 500, -90);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            while (v < 11){
            avanzar_deteccion_vacio_derecho_lidar(80, 0);
            Spike_Turn_For_Degrees(der, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 600, -90);
            int angulo_correccion = avanzar_dos_puntos_izquierda(80, 500, -90);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            v = v + 1;
            }

        }
        else if (sentido == left)
        {
            Spike_Turn_For_Degrees(izq, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 1600, 90);
            int angulo_correccion = avanzar_dos_puntos_derecha(80, 500, 90);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            while (v < 11){
            avanzar_deteccion_vacio_izquierdo_lidar(80, 0);
            Spike_Turn_For_Degrees(izq, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 600, 90);
            int angulo_correccion = avanzar_dos_puntos_derecha(80, 500, 90);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            v = v + 1;
            }

        }
    }

    else{

        printf("caso adentro\n");

        direction sentido = avanzar_deteccion_sentido_lidar(60, 0);
        printf("sentido : %d \n", sentido);

        if(sentido == right){
            Spike_Turn_For_Degrees(der, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 600, -90);
            int angulo_correccion = avanzar_dos_puntos_izquierda(80, 500, -90);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            while (v < 11){
            avanzar_deteccion_vacio_derecho_lidar(80, 0);
            Spike_Turn_For_Degrees(der, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 600, -90);
            int angulo_correccion = avanzar_dos_puntos_izquierda(80, 500, -90);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            v = v + 1;
            }

        }
        else if (sentido == left)
        {
            Spike_Turn_For_Degrees(izq, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 600, 90);
            int angulo_correccion = avanzar_dos_puntos_derecha(80, 500, 90);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            while (v < 11){
            avanzar_deteccion_vacio_izquierdo_lidar(80, 0);
            Spike_Turn_For_Degrees(izq, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 600, 90);
            int angulo_correccion = avanzar_dos_puntos_derecha(80, 500, 90);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            v = v + 1;
            }
        }
    } 
    
    printf("ultima funcion\n");
    avanzar_hasta_la_distancia(80, 0, 1400);

    Rasp_Gpio_Clean();
    Spike_Coast_Motors();
    Spike_Close_Serial();
    RPLidar_S2L_Close();

    return 0;
}

void signal_handler(int signum){
    printf("\nCtrl+C detceted\n");
    rplidar_s2l_set_terminating();
    signal(SIGINT, SIG_DFL);
}
