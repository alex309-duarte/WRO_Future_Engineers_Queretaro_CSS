#include "Oradar_S2L.h"
#include <ord_lidar_driver.h>
#include <cstdio>
#include <cmath>
#include <unistd.h>
#include <pthread.h>

using namespace ordlidar;

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

    while(oradar_terminating == 0){
        if(oradar_device->GrabFullScan(scan_data)){
            pthread_mutex_lock(&oradar_buffer_mutex);
            for(int i = 0; i < scan_data.vailtidy_point_num; i++){
                int angle = (int)scan_data.data[i].angle;
                if(angle >= 0 && angle < 360){
                    oradar_shared_buffer[angle] = scan_data.data[i].distance;
                }
            }
            oradar_scan_seq++;
            pthread_mutex_unlock(&oradar_buffer_mutex);
        }
        usleep(1000);
    }
    return NULL;
}

void Oradar_S2L_Get_Buffer(float *buffer){
    for(int i = 0; i < 360; i++){
        buffer[i] = oradar_shared_buffer[i];
    }
}

void Oradar_S2L_Set_Terminating(void){
    oradar_terminating = 1;
}

void Oradar_S2L_Close(void){
    if(oradar_device){
        oradar_device->Disconnect();
        delete oradar_device;
        oradar_device = nullptr;
    }
}

float Oradar_S2L_Radianes_A_Grados(float radianes){
    double grados = ((radianes * 180.f)/ M_PI);
    return grados;
}

float Oradar_S2L_Grados_A_Radianes(float grados){
    double radianes = ((grados * M_PI)/ 180.f);
    return radianes;
}

unsigned long Oradar_S2L_Get_Scan_Seq(void){
    return oradar_scan_seq;
}
