#include "Oradar_S2L.h"
#include <ord_lidar_driver.h>
#include "spike.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <pthread.h>

using namespace ordlidar;

struct temp_distance_struct{
    float  distance;
    int repetitions;
};

static OrdlidarDriver *oradar_device = nullptr;
static pthread_mutex_t oradar_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int oradar_terminating = 0;
static volatile float oradar_shared_buffer[360] = {0};
static volatile unsigned long oradar_scan_seq = 0; // incremented every time a new full scan is written

// MS200 default: 230400 baud.
static const char *opt_oradar_port = "/dev/ttyUSB0";
static const int   opt_oradar_baudrate = 230400;

void Oradar_S2L_Init_Lidar(void){
    oradar_device = new OrdlidarDriver(ORADAR_TYPE_SERIAL, ORADAR_MS200);
    oradar_device->SetSerialPort(opt_oradar_port, opt_oradar_baudrate);

    while (oradar_terminating == 0 && !oradar_device->Connect()){
        printf("Oradar lidar connect failed, retrying...\n");
        usleep(1000000);
    }
    printf("Oradar lidar connect successful\n");
}

void *Oradar_S2L_Lidar_Writer_Thread(void *arg){

    full_scan_data_st scan_data;
    temp_distance_struct promedio[360] = {0};

    while(oradar_terminating == 0){

        if(oradar_device->GrabFullScan(scan_data)){
            for (int i = 0; i < 360; i++) {
                promedio[i].repetitions = 0;
                promedio[i].distance = 0;
            }
            for (int pos = 0; pos < scan_data.vailtidy_point_num; ++pos) {
                int angle_bucket = (int)scan_data.data[pos].angle;
                if (angle_bucket >= 0 && angle_bucket < 360) {
                    promedio[angle_bucket].distance += scan_data.data[pos].distance;
                    promedio[angle_bucket].repetitions += 1;
                }
            }

            pthread_mutex_lock(&oradar_buffer_mutex);
            for (int i = 0; i < 360; i++) {
                if(promedio[i].repetitions > 0){
                    oradar_shared_buffer[i] = promedio[i].distance/promedio[i].repetitions;
                }
                else{
                    //printf("No measurements for angle %d\n", i);
                }
            }
            oradar_scan_seq++;
            pthread_mutex_unlock(&oradar_buffer_mutex); // Unlock after writing
        }
        usleep(1000);
    }
    return NULL;
}

float Oradar_S2L_Radianes_A_Grados(float radianes){
    double grados = ((radianes * 180.f)/ M_PI);
    return grados;
}

float Oradar_S2L_Grados_A_Radianes(float grados){
    double radianes = ((grados * M_PI)/ 180.f);
    return radianes;
}

direction Oradar_S2L_Avanzar_Deteccion_Sentido_Lidar(int vel, int referencia){

    float distancia_frente;
    float distancia_derecha;
    float distancia_izquierda;

    distancia_frente = oradar_shared_buffer[RP_TO_ORADAR_IDX(0)];
    distancia_derecha = oradar_shared_buffer[RP_TO_ORADAR_IDX(90)];
    distancia_izquierda = oradar_shared_buffer[RP_TO_ORADAR_IDX(270)];


    while((oradar_terminating == 0) && (((distancia_derecha < 1350) && (distancia_izquierda < 1350)) || (distancia_frente > 1100))){
        distancia_frente = oradar_shared_buffer[RP_TO_ORADAR_IDX(0)];
        distancia_derecha = oradar_shared_buffer[RP_TO_ORADAR_IDX(270)];
        distancia_izquierda = oradar_shared_buffer[RP_TO_ORADAR_IDX(90)];
        Spike_Forward(vel,referencia);
        usleep(1000);
        printf("dsitancia derecha : %f\n", distancia_derecha);
        printf("dsitancia izquierda : %f\n", distancia_izquierda);
        printf("dsitancia frente : %f\n", distancia_frente);
    }

    if(distancia_derecha > 1350){
        return right;
    }
    else if (distancia_izquierda > 1350)
    {
        return left;
    }
    else{
        return invalid;
    }

}

void Oradar_S2L_Avanzar_Deteccion_Vacio_Izquierdo_Lidar(int vel, int referencia){

    float distancia_frente;
    float distancia_izquierda;

    distancia_frente = oradar_shared_buffer[RP_TO_ORADAR_IDX(0)];
    distancia_izquierda = oradar_shared_buffer[RP_TO_ORADAR_IDX(90)];

    while((oradar_terminating == 0) && (((distancia_izquierda < 1350) && (distancia_frente > 500)) || (distancia_frente > 1100))){
        distancia_frente = oradar_shared_buffer[RP_TO_ORADAR_IDX(0)];
        distancia_izquierda = oradar_shared_buffer[RP_TO_ORADAR_IDX(90)];
        Spike_Forward(vel,referencia);
        usleep(1000);
        //printf("dsitancia izquierda : %f\n", distancia_izquierda);
        //printf("dsitancia frente : %f\n", distancia_frente);
    }

}

void Oradar_S2L_Avanzar_Deteccion_Vacio_Derecho_Lidar(int vel, int referencia){

    float distancia_frente;
    float distancia_derecha;

    distancia_frente = oradar_shared_buffer[RP_TO_ORADAR_IDX(0)];
    distancia_derecha = oradar_shared_buffer[RP_TO_ORADAR_IDX(270)];

    while((oradar_terminating == 0) && (((distancia_derecha < 1350) && (distancia_frente > 500))  || (distancia_frente > 1100))){
        distancia_frente = oradar_shared_buffer[RP_TO_ORADAR_IDX(0)];
        distancia_derecha = oradar_shared_buffer[RP_TO_ORADAR_IDX(270)];
        Spike_Forward(vel,referencia);
        //printf("dsitancia derecha : %f\n", distancia_derecha);
        //printf("dsitancia frente : %f\n", distancia_frente);
        usleep(1000);
    }

}

int Oradar_S2L_Avanzar_Dos_Puntos_Izquierda(int vel, int grados, int referencia){
    float y1 = oradar_shared_buffer[RP_TO_ORADAR_IDX(90)];

    Spike_Advance_For_Degrees(vel, grados, referencia);

    float y2 = oradar_shared_buffer[RP_TO_ORADAR_IDX(90)];

    float variacion = y1-y2;
    int pendiente = (int)(10 *(Oradar_S2L_Radianes_A_Grados(atan(variacion/(175.0*grados/360.0)))));
    printf("pendiente: %d\n", pendiente);
    return pendiente;
}

int Oradar_S2L_Avanzar_Dos_Puntos_Derecha(int vel, int grados, int referencia){
    float y1 = oradar_shared_buffer[RP_TO_ORADAR_IDX(270)];

    Spike_Advance_For_Degrees(vel, grados, referencia);

    float y2 = oradar_shared_buffer[RP_TO_ORADAR_IDX(270)];

    float variacion = y1-y2;
    int pendiente = (int)(-10 *(Oradar_S2L_Radianes_A_Grados(atan(variacion/(175.0*grados/360.0)))));
    printf("pendiente: %d, y1: %f, y2: %f\n", pendiente, y1, y2);
    return pendiente;
}

void Oradar_S2L_Avanzar_Hasta_La_Distancia(int vel, int referencia, int distancia_objetivo){
    float distancia_frente;

    distancia_frente = oradar_shared_buffer[RP_TO_ORADAR_IDX(0)];

    while((oradar_terminating == 0) && (distancia_frente > distancia_objetivo)){
        distancia_frente = oradar_shared_buffer[RP_TO_ORADAR_IDX(0)];
        Spike_Forward(vel,referencia);
        printf("dsitancia frente : %f\n", distancia_frente);
        usleep(1000);
    }
}

int Oradar_S2L_Correction_For_Triangles_Left(int degree){
    float H = oradar_shared_buffer[RP_TO_ORADAR_IDX(90 - degree)];
    float CA = oradar_shared_buffer[RP_TO_ORADAR_IDX(90)];

    float lado_faltante = sqrt(pow(H,2) + pow(CA,2) - (2*H*CA*cos(Oradar_S2L_Grados_A_Radianes(degree))));
    float angulo_correcion = (pow(CA,2) + pow(lado_faltante,2) - pow(H,2))/(2*CA*lado_faltante);
    float comparacion = Oradar_S2L_Radianes_A_Grados(acos(angulo_correcion));
    float correcion_final = 90 - comparacion;
    printf("H: %f, CA: %f, lado_faltante: %f, angulo_correcion: %f, comparacion: %f, correcion_final: %f\n", H, CA, lado_faltante, angulo_correcion, comparacion, correcion_final);
    return (int)(correcion_final*10);

}

int Oradar_S2L_Correction_For_Triangles_Right(int degree){
    float CA = oradar_shared_buffer[RP_TO_ORADAR_IDX(270)];
    float H = oradar_shared_buffer[RP_TO_ORADAR_IDX(90 + degree)];

    float lado_faltante = sqrt(pow(H,2) + pow(CA,2) - (2*H*CA*cos(Oradar_S2L_Grados_A_Radianes(degree))));
    float angulo_correcion = (pow(CA,2) + pow(lado_faltante,2) - pow(H,2))/(2*CA*lado_faltante);
    float comparacion = Oradar_S2L_Radianes_A_Grados(acos(angulo_correcion));
    float correcion_final = comparacion - 90;
    //printf("H: %f, CA: %f, lado_faltante: %f, angulo_correcion: %f, comparacion: %f, correcion_final: %f\n", H, CA, lado_faltante, angulo_correcion, comparacion, correcion_final);
    return (int)(correcion_final*10);

}

int Oradar_S2L_Comparation(int arg1, int arg2, int arg3){
    if(((abs(arg1 - arg2)) < 30) || ((abs(arg2 - arg3)) < 30)){
        return arg2;
    }
    else if ((abs(arg1 - arg3)) < 30)
    {
        return ((arg1 + arg3)/2);
    }
    else{
        return arg2;
    }
}

void Oradar_S2L_Close(void){
    if(oradar_device){
        oradar_device->Disconnect();
        delete oradar_device;
        oradar_device = nullptr;
    }
}

void Oradar_S2L_Set_Terminating(void){
    oradar_terminating = 1;
}

void Oradar_S2L_Get_Buffer(float *buffer){
    *buffer = oradar_shared_buffer[0];
}

unsigned long Oradar_S2L_Get_Scan_Seq(void){
    return oradar_scan_seq;
}
