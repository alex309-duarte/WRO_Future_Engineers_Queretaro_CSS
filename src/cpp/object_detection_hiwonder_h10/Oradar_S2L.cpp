#include "Oradar_S2L.h"
#include "spike.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <unistd.h>
#include <pthread.h>

// El driver se elige al compilar (CMake: -DWRO_LIDAR=ORADAR|RPLIDAR_S2L). Los
// dos llenan el mismo buffer de 360 con la orientacion del Oradar (derecha=0,
// atras=90, izquierda=180, frente=270), que es la que usan los algoritmos de
// abajo y object_detection.cpp; asi la mision no cambia con el lidar.

static pthread_mutex_t oradar_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int oradar_terminating = 0;
static volatile float oradar_shared_buffer[360] = {0};
static volatile unsigned long oradar_scan_seq = 0; // incremented every time a new full scan is written

// WRO_LIDAR_PORT cambia el puerto sin recompilar.
static const char *lidar_port(const char *fallback){
    const char *port = std::getenv("WRO_LIDAR_PORT");
    return (port != nullptr && *port != '\0') ? port : fallback;
}

#if defined(WRO_LIDAR_RPLIDAR_S2L)

#include <sl_lidar.h>
#include <sl_lidar_driver.h>

using namespace sl;

// RPLidar S2L: 1 Mbaud. Su angulo crece en sentido horario desde la marca
// frontal del lidar. RPLIDAR_FRONT_DEG es el angulo del lidar que apunta al
// frente del robot y RPLIDAR_CLOCKWISE=0 invierte el sentido (lidar boca
// abajo). VERIFICAR en el robot: ver README.md.
#ifndef RPLIDAR_FRONT_DEG
#define RPLIDAR_FRONT_DEG 0
#endif
#ifndef RPLIDAR_CLOCKWISE
#define RPLIDAR_CLOCKWISE 1
#endif

static ILidarDriver *rplidar_device = nullptr;
static IChannel *rplidar_channel = nullptr;

// Angulo del lidar -> indice del buffer en la orientacion del Oradar.
static int rplidar_to_buffer_index(float lidar_deg){
    const int raw = ((int)lidar_deg % 360 + 360) % 360;
    const int robot_cw = RPLIDAR_CLOCKWISE ? raw - RPLIDAR_FRONT_DEG : RPLIDAR_FRONT_DEG - raw;
    return RP_TO_ORADAR_IDX(robot_cw);
}

void Oradar_S2L_Init_Lidar(void){
    const char *port = lidar_port("/dev/ttyUSB0");
    sl_lidar_response_device_info_t devinfo;

    while (oradar_terminating == 0) {
        rplidar_device = *createLidarDriver();
        rplidar_channel = *createSerialPortChannel(port, 1000000);
        if (SL_IS_OK(rplidar_device->connect(rplidar_channel)) &&
            SL_IS_OK(rplidar_device->getDeviceInfo(devinfo))) {
            break;
        }
        printf("RPLidar S2L connect failed on %s, retrying...\n", port);
        rplidar_device->disconnect();
        delete rplidar_device;
        delete rplidar_channel;
        rplidar_device = nullptr;
        rplidar_channel = nullptr;
        usleep(1000000);
    }
    if (rplidar_device == nullptr) return;

    rplidar_device->setMotorSpeed(DEFAULT_MOTOR_SPEED);
    usleep(200000);
    rplidar_device->startScan(false, true);
    printf("RPLidar S2L connect successful on %s\n", port);
}

void *Oradar_S2L_Lidar_Writer_Thread(void *arg){
    (void)arg;
    static sl_lidar_response_measurement_node_hq_t nodes[8192];
    float sum[360];
    int count_per_index[360];

    while(oradar_terminating == 0 && rplidar_device != nullptr){
        size_t count = sizeof(nodes) / sizeof(nodes[0]);
        if (SL_IS_OK(rplidar_device->grabScanDataHq(nodes, count))) {
            rplidar_device->ascendScanData(nodes, count);
            for (int i = 0; i < 360; i++) {
                sum[i] = 0.0f;
                count_per_index[i] = 0;
            }
            // Como el driver S2L anterior: calidad > 20 y promedio por grado.
            for (size_t pos = 0; pos < count; ++pos) {
                const int quality = nodes[pos].quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT;
                const float distance = nodes[pos].dist_mm_q2 / 4.0f;
                if (quality > 20 && distance > 0.0f) {
                    const int idx = rplidar_to_buffer_index(nodes[pos].angle_z_q14 * 90.f / 16384.f);
                    sum[idx] += distance;
                    count_per_index[idx]++;
                }
            }
            pthread_mutex_lock(&oradar_buffer_mutex);
            for (int i = 0; i < 360; i++) {
                if (count_per_index[i] > 0) {
                    oradar_shared_buffer[i] = sum[i] / count_per_index[i];
                }
            }
            oradar_scan_seq++;
            pthread_mutex_unlock(&oradar_buffer_mutex);
        }
    }
    return NULL;
}

void Oradar_S2L_Close(void){
    if(rplidar_device){
        rplidar_device->stop();
        rplidar_device->setMotorSpeed(0);
        rplidar_device->disconnect();
        delete rplidar_device;
        rplidar_device = nullptr;
    }
    delete rplidar_channel;
    rplidar_channel = nullptr;
}

#else  // Oradar MS200

#include <ord_lidar_driver.h>

using namespace ordlidar;

static OrdlidarDriver *oradar_device = nullptr;

// MS200 default: 230400 baud.
static const int   opt_oradar_baudrate = 230400;

void Oradar_S2L_Init_Lidar(void){
    oradar_device = new OrdlidarDriver(ORADAR_TYPE_SERIAL, ORADAR_MS200);
    oradar_device->SetSerialPort(lidar_port("/dev/ttyUSB0"), opt_oradar_baudrate);

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

void Oradar_S2L_Close(void){
    if(oradar_device){
        oradar_device->Disconnect();
        delete oradar_device;
        oradar_device = nullptr;
    }
}

#endif

void Oradar_S2L_Get_Buffer(float *buffer){
    for(int i = 0; i < 360; i++){
        buffer[i] = oradar_shared_buffer[i];
    }
}

void Oradar_S2L_Set_Terminating(void){
    oradar_terminating = 1;
}

float Oradar_S2L_Radians_To_Degrees(float radians){
    double degrees = ((radians * 180.f)/ M_PI);
    return degrees;
}

float Oradar_S2L_Degrees_To_Radians(float degrees){
    double radians = ((degrees * M_PI)/ 180.f);
    return radians;
}

void Oradar_S2L_Advance_Until_Distance(int vel, int referencia, int distancia_objetivo, Brake_type brake){
    float distancia_frente;

    distancia_frente = oradar_shared_buffer[270];

    while((terminating_main == 0) && (distancia_frente > distancia_objetivo)){
        distancia_frente = oradar_shared_buffer[270];
        Spike_Forward(vel,referencia);
        usleep(1000);
    }
    if(brake == Coast)
    {
        Spike_Coast_Motors();
    }
    else if(brake == Hold)
    {
        Spike_Hold_Motors();
    }
    else{
        /* No break applied */
    }

}

unsigned long Oradar_S2L_Get_Scan_Seq(void){
    return oradar_scan_seq;
}

direction Oradar_S2L_Advance_And_Detect_Side(int speed, int reference){

    float front_distance;
    float right_distance;
    float left_distance;

    front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
    right_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];
    left_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];

    while((oradar_terminating == 0) && (((right_distance < 1350) && (left_distance < 1350)) || (front_distance > 1100))){
        front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
        right_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];
        left_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];
        Spike_Forward(speed,reference);
        usleep(1000);
    }

    if(right_distance > 1350){
        return right;
    }
    else if (left_distance > 1350)
    {
        return left;
    }
    else{
        return invalid;
    }

}

void Oradar_S2L_Advance_Until_Left_Gap(int speed, int reference){

    float front_distance;
    float left_distance;

    front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
    left_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];

    while((oradar_terminating == 0) && (((left_distance < 1350) && (front_distance > 500)) || (front_distance > 1100))){
        front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
        left_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];
        Spike_Forward(speed,reference);
        usleep(1000);
    }

}

void Oradar_S2L_Advance_Until_Right_Gap(int speed, int reference){

    float front_distance;
    float right_distance;

    front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
    right_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];

    while((oradar_terminating == 0) && (((right_distance < 1350) && (front_distance > 500))  || (front_distance > 1100))){
        front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
        right_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];
        Spike_Forward(speed,reference);
        usleep(1000);
    }

}

int Oradar_S2L_Advance_And_Measure_Left_Slope(int speed, int degrees, int reference){
    float y1 = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];

    Spike_Advance_For_Degrees(speed, degrees, reference);

    float y2 = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];

    float distance_delta = y1-y2;
    int slope = (int)(10 *(Oradar_S2L_Radians_To_Degrees(atan(distance_delta/(175.0*degrees/360.0)))));
    printf("pendiente: %d\n", slope);
    return slope;
}

int Oradar_S2L_Advance_And_Measure_Right_Slope(int speed, int degrees, int reference){
    float y1 = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];

    Spike_Advance_For_Degrees(speed, degrees, reference);

    float y2 = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];

    float distance_delta = y1-y2;
    int slope = (int)(-10 *(Oradar_S2L_Radians_To_Degrees(atan(distance_delta/(175.0*degrees/360.0)))));
    printf("pendiente: %d, y1: %f, y2: %f\n", slope, y1, y2);
    return slope;
}

int Oradar_S2L_Correction_For_Triangles_Left(int degree){
    float H = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT + degree)];
    float CA = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];

    float missing_side = sqrt(pow(H,2) + pow(CA,2) - (2*H*CA*cos(Oradar_S2L_Degrees_To_Radians(degree))));
    float correction_angle = (pow(CA,2) + pow(missing_side,2) - pow(H,2))/(2*CA*missing_side);
    float computed_angle = Oradar_S2L_Radians_To_Degrees(acos(correction_angle));
    float final_correction = 90 - computed_angle;
    return (int)(final_correction*10);

}

int Oradar_S2L_Correction_For_Triangles_Right(int degree){
    float CA = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];
    float H = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT - degree)];

    float missing_side = sqrt(pow(H,2) + pow(CA,2) - (2*H*CA*cos(Oradar_S2L_Degrees_To_Radians(degree))));
    float correction_angle = (pow(CA,2) + pow(missing_side,2) - pow(H,2))/(2*CA*missing_side);
    float computed_angle = Oradar_S2L_Radians_To_Degrees(acos(correction_angle));
    float final_correction = computed_angle - 90;
    return (int)(final_correction*10);

}

int Oradar_S2L_Reconcile_Readings(int reading_a, int reading_b, int reading_c){
    if(((abs(reading_a - reading_b)) < 30) || ((abs(reading_b - reading_c)) < 30)){
        return reading_b;
    }
    else if ((abs(reading_a - reading_c)) < 30)
    {
        return ((reading_a + reading_c)/2);
    }
    else{
        return reading_b;
    }
}

int Oradar_S2L_Slope(const int point, const int middle_point){
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    float x_point[point] = {0};
    float y_point[point] = {0};
    float dis[point] = {0};
    int angle[point] = {0};

    for(int i = 0; i < point; i++){
        dis[i] = oradar_shared_buffer[RP_TO_ORADAR_IDX(middle_point - point + i)];
        angle[i] = middle_point - point  +i;
    }

    for(int i = 0; i < point; i++) {
        x_point[i] = (dis[i] * cos(Oradar_S2L_Degrees_To_Radians(angle[i])));
        y_point[i] = (dis[i] * sin(Oradar_S2L_Degrees_To_Radians(angle[i])));
    }

    for (int i = 0; i < point; i++) {
        sum_x += x_point[i];
        sum_y += y_point[i];
        sum_xy += x_point[i] * y_point[i];
        sum_x2 += x_point[i] * x_point[i];
    }

    double det = point * sum_x2 - sum_x * sum_x;

    if (det == 0) {
        printf("Error: El sistema no tiene solución única (todos los X son iguales).\n");
        return 0;
    }
    int p = (10)*(Oradar_S2L_Radians_To_Degrees(atan((point * sum_xy - sum_x * sum_y) / det)));
    printf("pendiente : %d\n",p);
    return (p);

}

int Oradar_S2L_Slope2(const int point, const int middle_point){
    float x_point[point] = {0};
    float y_point[point] = {0};
    float dis[point] = {0};
    int angle[point] = {0};

    for(int i = 0; i < point; i++){
        dis[i] = oradar_shared_buffer[RP_TO_ORADAR_IDX(middle_point - point + i)];
        angle[i] = middle_point - point  +i;
    }

    for(int i = 0; i < point; i++) {
        x_point[i] = (dis[i] * cos(Oradar_S2L_Degrees_To_Radians(angle[i])));
        y_point[i] = (dis[i] * sin(Oradar_S2L_Degrees_To_Radians(angle[i])));
    }

    double mean_x = 0, mean_y = 0;
    for(int i = 0; i < point; i++){
        mean_x += x_point[i];
        mean_y += y_point[i];
    }
    mean_x /= point;
    mean_y /= point;

    double sxx = 0, syy = 0, sxy = 0;
    for(int i = 0; i < point; i++){
        double dx = x_point[i] - mean_x;
        double dy = y_point[i] - mean_y;
        sxx += dx * dx;
        syy += dy * dy;
        sxy += dx * dy;
    }

    int p = (10)*(Oradar_S2L_Radians_To_Degrees(0.5 * atan2(2.0 * sxy, sxx - syy)));
    printf("pendiente2 : %d\n",p);
    return (p);
}

int Oradar_S2L_Average(int arg_1, int arg_2){
    int average = (arg_1 + arg_2)/2;
    printf("correcion: %d\n",average);
    return average;
}
