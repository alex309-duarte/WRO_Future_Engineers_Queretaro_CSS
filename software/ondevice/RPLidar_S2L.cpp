#include "RPLidar_S2L.h"
#include "spike.h"

using namespace sl;

pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

const char * opt_channel_param_first = "/dev/ttyUSB0";
ILidarDriver * drv;
IChannel * _channel;
sl_result  op_result;

static volatile int terminating = 0;
static volatile float lidar_shared_buffer[360]; // Your shared buffer

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
    //drv->startScan(false, false);
}

void *lidar_writer_thread(void *arg){

    float angle_temp = 0;
    sl_lidar_response_measurement_node_hq_t nodes[8192];
    size_t   count = _countof(nodes);

    while(1){

        op_result = drv->grabScanDataHq(nodes, count);
        temp_distance_struct promedio[360] = {0};
         
        if (SL_IS_OK(op_result)) {
            drv->ascendScanData(nodes, count);
            //printf("\n counts: %d \n", count);
            for (int pos = 0; pos < (int)count ; ++pos) {
                angle_temp = ((nodes[pos].angle_z_q14) * 90.f) / 16384.f;
                promedio[int(angle_temp)].distance += nodes[pos].dist_mm_q2/4.0f;
                promedio[int(angle_temp)].repetitions += 1;

                if(int(angle_temp) == 0 || int(angle_temp) == 270 || int(angle_temp) == 90)
                {
                    
                    /*printf("%s theta: %03.2f Dist: %08.2f Q: %d \n", 
                        (nodes[pos].flag & SL_LIDAR_RESP_HQ_FLAG_SYNCBIT) ?"S ":"  ", 
                        (nodes[pos].angle_z_q14 * 90.f) / 16384.f,
                        nodes[pos].dist_mm_q2/4.0f,
                        nodes[pos].quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);*/
                    
                }
            }

            pthread_mutex_lock(&buffer_mutex);
            for (int i = 0; i < 360; i++) {
                if(promedio[i].repetitions > 0){
                    lidar_shared_buffer[i] = promedio[i].distance/promedio[i].repetitions;
                }
                else{
                    //printf("No measurements for angle %d\n", i);
                }
            }
            pthread_mutex_unlock(&buffer_mutex); // Unlock after writing
        }
    }
    return NULL;
}

float radianes_a_grados(float radianes){
    double grados = ((radianes * 180.f)/ M_PI);
    return grados;
}

direction avanzar_deteccion_sentido_lidar(int vel, int referencia){

    float distancia_frente;
    float distancia_derecha;
    float distancia_izquierda;

    distancia_frente = lidar_shared_buffer[0];
    distancia_derecha = lidar_shared_buffer[90];
    distancia_izquierda = lidar_shared_buffer[270];


    while((terminating == 0) && (((distancia_derecha < 1350) && (distancia_izquierda < 1350)) || (distancia_frente > 1100))){
        distancia_frente = lidar_shared_buffer[0];
        distancia_derecha = lidar_shared_buffer[270];
        distancia_izquierda = lidar_shared_buffer[90];
        Spike_Forward(vel,referencia);
        
        //printf("dsitancia derecha : %f\n", distancia_derecha);
        //printf("dsitancia izquierda : %f\n", distancia_izquierda);
        //printf("dsitancia frente : %f\n", distancia_frente);
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

void avanzar_deteccion_vacio_izquierdo_lidar(int vel, int referencia){

    float distancia_frente;
    float distancia_izquierda;

    distancia_frente = lidar_shared_buffer[0];
    distancia_izquierda = lidar_shared_buffer[90];

    while((terminating == 0) && ((distancia_izquierda < 1350) || (distancia_frente > 1100))){
        distancia_frente = lidar_shared_buffer[0];
        distancia_izquierda = lidar_shared_buffer[90];
        Spike_Forward(vel,referencia);
        //printf("dsitancia izquierda : %f\n", distancia_izquierda);
        //printf("dsitancia frente : %f\n", distancia_frente);
    }
    
} 

void avanzar_deteccion_vacio_derecho_lidar(int vel, int referencia){

    float distancia_frente;
    float distancia_derecha;

    distancia_frente = lidar_shared_buffer[0];
    distancia_derecha = lidar_shared_buffer[270];

    while((terminating == 0) && ((distancia_derecha < 1350)  || (distancia_frente > 1100))){
        distancia_frente = lidar_shared_buffer[0];
        distancia_derecha = lidar_shared_buffer[270];
        Spike_Forward(vel,referencia);
        //printf("dsitancia derecha : %f\n", distancia_derecha);
        //printf("dsitancia frente : %f\n", distancia_frente);
    }

} 

int avanzar_dos_puntos_izquierda(int vel, int grados, int referencia){
    float y1 = lidar_shared_buffer[90];

    Spike_Advance_For_Degrees(vel, grados, referencia);

    float y2 = lidar_shared_buffer[90];

    float variacion = y1-y2;
    int pendiente = (int)(10 *(radianes_a_grados(atan(variacion/(175.0*grados/360.0)))));
    printf("pendiente: %d\n", pendiente);
    return pendiente;
}

int avanzar_dos_puntos_derecha(int vel, int grados, int referencia){
    float y1 = lidar_shared_buffer[270];

    Spike_Advance_For_Degrees(vel, grados, referencia);

    float y2 = lidar_shared_buffer[270];

    float variacion = y1-y2;
    int pendiente = (int)(-10 *(radianes_a_grados(atan(variacion/(175.0*grados/360.0)))));
    printf("pendiente: %d, y1: %f, y2: %f\n", pendiente, y1, y2);
    return pendiente;
}

void avanzar_hasta_la_distancia(int vel, int referencia, int distancia_objetivo){
    float distancia_frente;

    distancia_frente = lidar_shared_buffer[0];

    while((terminating == 0) && (distancia_frente > distancia_objetivo)){
        distancia_frente = lidar_shared_buffer[0];
        Spike_Forward(vel,referencia);
        printf("dsitancia frente : %f\n", distancia_frente);
    }
}

void RPLidar_S2L_Close(void){
    drv->stop();
    if(drv){
        drv->setMotorSpeed(0);
        delete drv;
        drv = NULL;
    }
}

void rplidar_s2l_set_terminating(void){
    terminating = 1;
}

void RPLidar_S2L_Get_Buffer(float *buffer){
    *buffer = lidar_shared_buffer[0];
}