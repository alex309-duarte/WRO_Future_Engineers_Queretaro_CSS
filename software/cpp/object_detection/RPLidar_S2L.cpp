#include "RPLidar_S2L.h"
#include "spike.h"
#include <opencv2/opencv.hpp>

using namespace sl;

pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

const char * opt_channel_param_first = "/dev/ttyUSB0";
ILidarDriver * drv;
IChannel * _channel;
sl_result  op_result;

static volatile int terminating = 0;
static volatile float lidar_shared_buffer[360]; // Your shared buffer

void RPLidar_S2L_Init_Lidar(void){
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

    drv->setMotorSpeed(DEFAULT_MOTOR_SPEED);
    usleep(200000);
    // start scan...
  
    std::vector<LidarScanMode> scanModes;
    drv->getAllSupportedScanModes(scanModes);
    for(const auto& mode : scanModes) {
        printf("mode id  = %d\n",mode.id);
    }
    //drv->startScanExpress(false, scanModes[2].id);
    drv->startScan(false, true);
}

void *RPLidar_S2L_Lidar_Writer_Thread(void *arg){

    float angle_temp = 0;
    sl_lidar_response_measurement_node_hq_t nodes[8192];
    size_t   count = _countof(nodes);
    temp_distance_struct promedio[360] = {0};

    while(terminating == 0){

        count = _countof(nodes);
        op_result = drv->grabScanDataHq(nodes, count);
        if (SL_IS_OK(op_result)) {
            for (int i = 0; i < 360; i++) {
                promedio[i].repetitions = 0;
                promedio[i].distance = 0;
            }
            drv->ascendScanData(nodes, count);
            //printf("\n counts: %d \n", count);
            for (int pos = 0; pos < (int)count ; ++pos) {
                if((nodes[pos].quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT) > 20){
                    angle_temp = ((nodes[pos].angle_z_q14) * 90.f) / 16384.f;
                    promedio[int(angle_temp)].distance += nodes[pos].dist_mm_q2/4.0f;
                    promedio[int(angle_temp)].repetitions += 1;
                }
                /*if( int(angle_temp) == 100 || int(angle_temp) == 90)
                {
                    
                    printf("%s theta: %03.2f Dist: %08.2f Q: %d \n", 
                        (nodes[pos].flag & SL_LIDAR_RESP_HQ_FLAG_SYNCBIT) ?"S ":"  ", 
                        (nodes[pos].angle_z_q14 * 90.f) / 16384.f,
                        nodes[pos].dist_mm_q2/4.0f,
                        nodes[pos].quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
                    
                }*/
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

float RPLidar_S2L_Radianes_A_Grados(float radianes){
    double grados = ((radianes * 180.f)/ M_PI);
    return grados;
}

float RPLidar_S2L_Grados_A_Radianes(float grados){
    double radianes = ((grados * M_PI)/ 180.f);
    return radianes;
}

direction RPLidar_S2L_Avanzar_Deteccion_Sentido_Lidar(int vel, int referencia){

    float distancia_frente;
    float distancia_derecha;
    float distancia_izquierda;

    distancia_frente = lidar_shared_buffer[0];
    distancia_derecha = lidar_shared_buffer[90];
    distancia_izquierda = lidar_shared_buffer[270];


    while(    (terminating == 0)
            && (((distancia_derecha < 1350)
            && (distancia_izquierda < 1350)) 
            || (distancia_frente > 1100))){
        distancia_frente = lidar_shared_buffer[0];
        distancia_derecha = lidar_shared_buffer[270];
        distancia_izquierda = lidar_shared_buffer[90];
        Spike_Forward(vel,referencia);
        usleep(1000);
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

void RPLidar_S2L_Avanzar_Deteccion_Vacio_Izquierdo_Lidar(int vel, int referencia){

    float distancia_frente;
    float distancia_izquierda;

    distancia_frente = lidar_shared_buffer[0];
    distancia_izquierda = lidar_shared_buffer[90];

    while((terminating == 0) && (((distancia_izquierda < 1350) && (distancia_frente > 500)) || (distancia_frente > 1100))){
        distancia_frente = lidar_shared_buffer[0];
        distancia_izquierda = lidar_shared_buffer[90];
        Spike_Forward(vel,referencia);
        usleep(1000);
        //printf("dsitancia izquierda : %f\n", distancia_izquierda);
        //printf("dsitancia frente : %f\n", distancia_frente);
    }
    
} 

void RPLidar_S2L_Avanzar_Deteccion_Vacio_Derecho_Lidar(int vel, int referencia){

    float distancia_frente;
    float distancia_derecha;

    distancia_frente = lidar_shared_buffer[0];
    distancia_derecha = lidar_shared_buffer[270];

    while((terminating == 0) && (((distancia_derecha < 1350) && (distancia_frente > 500))  || (distancia_frente > 1100))){
        distancia_frente = lidar_shared_buffer[0];
        distancia_derecha = lidar_shared_buffer[270];
        Spike_Forward(vel,referencia);
        //printf("dsitancia derecha : %f\n", distancia_derecha);
        //printf("dsitancia frente : %f\n", distancia_frente);
        usleep(1000);
    }

} 

int RPLidar_S2L_Avanzar_Dos_Puntos_Izquierda(int vel, int grados, int referencia){
    float y1 = lidar_shared_buffer[90];

    Spike_Advance_For_Degrees(vel, grados, referencia);

    float y2 = lidar_shared_buffer[90];

    float variacion = y1-y2;
    int pendiente = (int)(10 *(RPLidar_S2L_Radianes_A_Grados(atan(variacion/(175.0*grados/360.0)))));
    printf("pendiente: %d\n", pendiente);
    return pendiente;
}

int RPLidar_S2L_Avanzar_Dos_Puntos_Derecha(int vel, int grados, int referencia){
    float y1 = lidar_shared_buffer[270];

    Spike_Advance_For_Degrees(vel, grados, referencia);

    float y2 = lidar_shared_buffer[270];

    float variacion = y1-y2;
    int pendiente = (int)(-10 *(RPLidar_S2L_Radianes_A_Grados(atan(variacion/(175.0*grados/360.0)))));
    printf("pendiente: %d, y1: %f, y2: %f\n", pendiente, y1, y2);
    return pendiente;
}

void RPLidar_S2L_Avanzar_Hasta_La_Distancia(int vel, int referencia, int distancia_objetivo){
    float distancia_frente;

    distancia_frente = lidar_shared_buffer[0];

    while((terminating == 0) && (distancia_frente > distancia_objetivo)){
        distancia_frente = lidar_shared_buffer[0];
        Spike_Forward(vel,referencia);
        printf("dsitancia frente : %f\n", distancia_frente);
        usleep(1000);
    }
}

int RPLidar_S2L_Correction_For_Triangles_Left(int degree){
    float H = lidar_shared_buffer[90 - degree];
    float CA = lidar_shared_buffer[90];

    float lado_faltante = sqrt(pow(H,2) + pow(CA,2) - (2*H*CA*cos(RPLidar_S2L_Grados_A_Radianes(degree))));
    float angulo_correcion = (pow(CA,2) + pow(lado_faltante,2) - pow(H,2))/(2*CA*lado_faltante);
    float comparacion = RPLidar_S2L_Radianes_A_Grados(acos(angulo_correcion));
    float correcion_final = 90 - comparacion;
    printf("H: %f, CA: %f, lado_faltante: %f, angulo_correcion: %f, comparacion: %f, correcion_final: %f\n", H, CA, lado_faltante, angulo_correcion, comparacion, correcion_final);
    return (int)(correcion_final*10);

}

int RPLidar_S2L_Correction_For_Triangles_Right(int degree){
    float CA = lidar_shared_buffer[270];
    float H = lidar_shared_buffer[90 + degree];

    float lado_faltante = sqrt(pow(H,2) + pow(CA,2) - (2*H*CA*cos(RPLidar_S2L_Grados_A_Radianes(degree))));
    float angulo_correcion = (pow(CA,2) + pow(lado_faltante,2) - pow(H,2))/(2*CA*lado_faltante);
    float comparacion = RPLidar_S2L_Radianes_A_Grados(acos(angulo_correcion));
    float correcion_final = comparacion - 90;
    //printf("H: %f, CA: %f, lado_faltante: %f, angulo_correcion: %f, comparacion: %f, correcion_final: %f\n", H, CA, lado_faltante, angulo_correcion, comparacion, correcion_final);
    return (int)(correcion_final*10);

}

int RPLidar_S2L_Comparation(int arg1, int arg2, int arg3){
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

void RPLidar_S2L_Close(void){
    drv->stop();
    if(drv){
        drv->setMotorSpeed(0);
        delete drv;
        drv = NULL;
    }
}

void RPLidar_S2L_Set_Terminating(void){
    terminating = 1;
}

void RPLidar_S2L_Get_Buffer(float *buffer){
    for(int i = 0; i < 360; i++){
        buffer[i] = lidar_shared_buffer[i];
    }
}