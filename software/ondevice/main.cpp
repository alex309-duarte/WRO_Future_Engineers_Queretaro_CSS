#include "cstdint"
#include <math.h>
#include "spike.h"
#include "signal.h"
#include <pthread.h>
#include "sl_lidar.h" 
#include "sl_lidar_driver.h"
#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>
#endif

using namespace sl;

pthread_t writer;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
float lidar_shared_buffer[360]; // Your shared buffer
volatile int terminating = 0;
const char * opt_channel_param_first = "/dev/ttyUSB0";
ILidarDriver * drv;
IChannel * _channel;
sl_result  op_result;

int der = 1;
int izq = -1;

void signal_handler(int signum);
void init_lidar(void);
void *lidar_writer_thread(void *arg);
float radianes_a_grados(float radianes);
int avanzar_deteccion_sentido_lidar(int vel, int referencia);
void avanzar_deteccion_vacio_izquierdo_lidar(int vel, int referencia);
void avanzar_deteccion_vacio_derecho_lidar(int vel, int referencia);
int avanzar_dos_puntos_izquierda(int vel, int grados, int referencia);
int avanzar_dos_puntos_derecha(int vel, int grados, int referencia);

int main(){
    signal(SIGINT, signal_handler); /* Set interrupt for ctrl+C */
    init_lidar();
    pthread_create(&writer, NULL, lidar_writer_thread, NULL);
    
    init_gpio();
    power_On_Spike();
    serial_init();
    interpreter();
    initialize_Libraries();
    wait_for_button();
    reset_gyro(0);
    usleep(200000); //wiating for reset gyro



    int sentido = avanzar_deteccion_sentido_lidar(60, 0);
    printf("sentido %d :\n", sentido);

    if(sentido == der){
        vuelta_grados(der, 60, 88);
        centrar_vehiculo();
        avanzar_grados(60, 600, -90);
        int angulo_correccion = avanzar_dos_puntos_derecha(60, 500, -90);
        reset_gyro(angulo_correccion);
        usleep(200000);
        int v;
        while (v < 11){
        avanzar_deteccion_vacio_derecho_lidar(60, 0);
        vuelta_grados(der, 60, 88);
        centrar_vehiculo();
        avanzar_grados(60, 600, -90);
        int angulo_correccion = avanzar_dos_puntos_derecha(60, 500, -90);
        reset_gyro(angulo_correccion);
        usleep(200000);
        v = v + 1;
        }

    }
    else if (sentido == izq)
    {
        vuelta_grados(izq, 60, 88);
        centrar_vehiculo();
        avanzar_grados(60, 600, 90);
        avanzar_deteccion_vacio_izquierdo_lidar(60, 90);


    }




    clean_GPIO();
    Coast_motors();
    close_serial();

    drv->stop();
    drv->setMotorSpeed(0);

    return 0;
}

void init_lidar(void){
    drv = *createLidarDriver();
    if (!drv) {
        fprintf(stderr, "insufficent memory, exit\n");
        exit(-2);
    }
    sl_lidar_response_device_info_t devinfo;
    bool connectSuccess = false;
    _channel = (*createSerialPortChannel(opt_channel_param_first, 1000000));
    if (SL_IS_OK((drv)->connect(_channel))) {
        op_result = drv->getDeviceInfo(devinfo);

        if (SL_IS_OK(op_result)) 
        {
	        connectSuccess = true;
            printf("\nconnection successful \n");
        }
        else{
            delete drv;
			drv = NULL;
        }
    }

    drv->setMotorSpeed();
    // start scan...
    drv->startScan(0,1);
    
}

void *lidar_writer_thread(void *arg){

    float buffer_temp[360] = {0};
    while(1){
        sl_lidar_response_measurement_node_hq_t nodes[8192];
        size_t   count = _countof(nodes);

        op_result = drv->grabScanDataHq(nodes, count);
        float angle_temp = 0; 
        
    
        if (SL_IS_OK(op_result)) {
            drv->ascendScanData(nodes, count);
            //printf("\n counts: %d \n", count);
            for (int pos = 0; pos < (int)count ; ++pos) {
                angle_temp = nodes[pos].angle_z_q14 * 90.f / 16384.f;
                buffer_temp[int(angle_temp)] = nodes[pos].dist_mm_q2/4.0f;
                if(int(angle_temp) == 0 || int(angle_temp) == 270 || int(angle_temp) == 90)
                {
                    /* pensar en una estrategia de promedio o de puntos maximos */
                    /*printf("%s theta: %03.2f Dist: %08.2f Q: %d \n", 
                        (nodes[pos].flag & SL_LIDAR_RESP_HQ_FLAG_SYNCBIT) ?"S ":"  ", 
                        (nodes[pos].angle_z_q14 * 90.f) / 16384.f,
                        nodes[pos].dist_mm_q2/4.0f,
                        nodes[pos].quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);*/
                    
                }
            }
        }
        pthread_mutex_lock(&buffer_mutex);
        for (int i = 0; i < 360; i++) {
            lidar_shared_buffer[i] = buffer_temp[i];
        }
        pthread_mutex_unlock(&buffer_mutex); // Unlock after writing
    }
    return NULL;
}

float radianes_a_grados(float radianes){
    double grados = radianes * 180/ M_PI;
    return grados;
}

int avanzar_deteccion_sentido_lidar(int vel, int referencia){

    float distancia_frente;
    float distancia_derecha;
    float distancia_izquierda;

    pthread_mutex_lock(&buffer_mutex);
    distancia_frente = lidar_shared_buffer[0];
    distancia_derecha = lidar_shared_buffer[90];
    distancia_izquierda = lidar_shared_buffer[270];
    pthread_mutex_unlock(&buffer_mutex);

    while((terminating == 0) && (((distancia_derecha < 1350) && (distancia_izquierda < 1350)) || (distancia_frente > 1100))){
        pthread_mutex_lock(&buffer_mutex);
        distancia_frente = lidar_shared_buffer[0];
        distancia_derecha = lidar_shared_buffer[270];
        distancia_izquierda = lidar_shared_buffer[90];
        pthread_mutex_unlock(&buffer_mutex); // Unlock after reading
        Spike_forward(vel,referencia);
        //printf("dsitancia derecha : %f\n", distancia_derecha);
        //printf("dsitancia izquierda : %f\n", distancia_izquierda);
        //printf("dsitancia frente : %f\n", distancia_frente);
    }
    
    if(distancia_derecha > 1350){
        return 1;
    }
    else if (distancia_izquierda > 1350)
    {
        return -1;
    }
    else{
        return 0;
    }

} 

void avanzar_deteccion_vacio_izquierdo_lidar(int vel, int referencia){

    float distancia_frente;
    float distancia_izquierda;

    pthread_mutex_lock(&buffer_mutex);
    distancia_frente = lidar_shared_buffer[0];
    distancia_izquierda = lidar_shared_buffer[90];
    pthread_mutex_unlock(&buffer_mutex);

    while((terminating == 0) && ((distancia_izquierda < 1350) || (distancia_frente > 1100))){
        pthread_mutex_lock(&buffer_mutex);
        distancia_frente = lidar_shared_buffer[0];
        distancia_izquierda = lidar_shared_buffer[90];
        pthread_mutex_unlock(&buffer_mutex); // Unlock after reading
        Spike_forward(vel,referencia);
        //printf("dsitancia izquierda : %f\n", distancia_izquierda);
        //printf("dsitancia frente : %f\n", distancia_frente);
    }
    
} 

void avanzar_deteccion_vacio_derecho_lidar(int vel, int referencia){

    float distancia_frente;
    float distancia_derecha;

    pthread_mutex_lock(&buffer_mutex);
    distancia_frente = lidar_shared_buffer[0];
    distancia_derecha = lidar_shared_buffer[270];
    pthread_mutex_unlock(&buffer_mutex);

    while((terminating == 0) && ((distancia_derecha < 1350)  || (distancia_frente > 1100))){
        pthread_mutex_lock(&buffer_mutex);
        distancia_frente = lidar_shared_buffer[0];
        distancia_derecha = lidar_shared_buffer[270];
        pthread_mutex_unlock(&buffer_mutex); // Unlock after reading
        Spike_forward(vel,referencia);
        //printf("dsitancia derecha : %f\n", distancia_derecha);
        //printf("dsitancia frente : %f\n", distancia_frente);
    }

} 

int avanzar_dos_puntos_izquierda(int vel, int grados, int referencia){
    pthread_mutex_lock(&buffer_mutex);
    float y1 = lidar_shared_buffer[90];
    pthread_mutex_unlock(&buffer_mutex); // Unlock after reading

    avanzar_grados(vel, grados, referencia);

    pthread_mutex_lock(&buffer_mutex);
    float y2 = lidar_shared_buffer[90];
    pthread_mutex_unlock(&buffer_mutex); // Unlock after reading

    float variacion = y1-y2;
    int pendiente = (int)(10 *(radianes_a_grados(atan(variacion/(175.0*grados/360.0)))));
    return pendiente;
}

int avanzar_dos_puntos_derecha(int vel, int grados, int referencia){
    pthread_mutex_lock(&buffer_mutex);
    float y1 = lidar_shared_buffer[270];
    pthread_mutex_unlock(&buffer_mutex); // Unlock after reading

    avanzar_grados(vel, grados, referencia);

    pthread_mutex_lock(&buffer_mutex);
    float y2 = lidar_shared_buffer[270];
    pthread_mutex_unlock(&buffer_mutex); // Unlock after reading

    float variacion = y1-y2;
    int pendiente = (int)(-10 *(radianes_a_grados(atan(variacion/(175.0*grados/360.0)))));
    return pendiente;
}

void signal_handler(int signum){
    printf("\nCtrl+C detceted\n");
    terminating = 1;
    signal(SIGINT, SIG_DFL);
}
