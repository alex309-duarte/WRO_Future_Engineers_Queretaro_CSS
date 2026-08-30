#include "Oradar_S2L.h"
#include <ord_lidar_driver.h>
#include "spike.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <pthread.h>

using namespace ordlidar;

struct DistanceAccumulator{
    float distance_sum;
    int sample_count;
};

static OrdlidarDriver *oradar_device = nullptr;
static pthread_mutex_t oradar_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile int oradar_terminating = 0;
static volatile float oradar_shared_buffer[360] = {0};
static volatile unsigned long oradar_scan_seq = 0; // incremented every time a new full scan is written

// MS200 default: 230400 baud.
static const char *serial_port_path = "/dev/ttyUSB0";
static const int   serial_baudrate = 230400;

void Oradar_S2L_Init_Lidar(void){
    oradar_device = new OrdlidarDriver(ORADAR_TYPE_SERIAL, ORADAR_MS200);
    oradar_device->SetSerialPort(serial_port_path, serial_baudrate);

    while (oradar_terminating == 0 && !oradar_device->Connect()){
        printf("Oradar lidar connect failed, retrying...\n");
        usleep(1000000);
    }
    printf("Oradar lidar connect successful\n");
}

void *Oradar_S2L_Lidar_Writer_Thread(void *arg){

    full_scan_data_st scan_data;
    DistanceAccumulator average[360] = {0};

    while(oradar_terminating == 0){

        if(oradar_device->GrabFullScan(scan_data)){
            for (int i = 0; i < 360; i++) {
                average[i].sample_count = 0;
                average[i].distance_sum = 0;
            }
            for (int pos = 0; pos < scan_data.vailtidy_point_num; ++pos) {
                int angle_bucket = (int)scan_data.data[pos].angle;
                if (angle_bucket >= 0 && angle_bucket < 360) {
                    average[angle_bucket].distance_sum += scan_data.data[pos].distance;
                    average[angle_bucket].sample_count += 1;
                }
            }

            pthread_mutex_lock(&oradar_buffer_mutex);
            for (int i = 0; i < 360; i++) {
                if(average[i].sample_count > 0){
                    oradar_shared_buffer[i] = average[i].distance_sum/average[i].sample_count;
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

float Oradar_S2L_Radians_To_Degrees(float radians){
    double degrees = ((radians * 180.f)/ M_PI);
    return degrees;
}

float Oradar_S2L_Degrees_To_Radians(float degrees){
    double radians = ((degrees * M_PI)/ 180.f);
    return radians;
}

direction Oradar_S2L_Advance_And_Detect_Side(int speed, int reference){

    float front_distance;
    float right_distance;
    float left_distance;
    float back_distance;

    front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
    right_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];
    left_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];


    while((oradar_terminating == 0) && (((right_distance < 1350) && (left_distance < 1350)) || (front_distance > 1100))){
        front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
        right_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];
        left_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];
        back_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(BACK)];
        //Spike_Forward(speed,reference);
        usleep(1000);
        printf("dsitancia derecha : %f\n", right_distance);
        printf("dsitancia izquierda : %f\n", left_distance);
        printf("dsitancia frente : %f\n", front_distance);
        printf("dsitancia atras : %f\n", back_distance);
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
        //printf("dsitancia izquierda : %f\n", left_distance);
        //printf("dsitancia frente : %f\n", front_distance);
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
        //printf("dsitancia derecha : %f\n", right_distance);
        //printf("dsitancia frente : %f\n", front_distance);
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

void Oradar_S2L_Advance_Until_Distance(int speed, int reference, int target_distance){
    float front_distance;

    front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];

    while((oradar_terminating == 0) && (front_distance > target_distance)){
        front_distance = oradar_shared_buffer[RP_TO_ORADAR_IDX(FRONT)];
        Spike_Forward(speed,reference);
        //printf("dsitancia frente : %f\n", front_distance);
        usleep(1000);
    }

    Spike_Hold_Motors();
}

int Oradar_S2L_Correction_For_Triangles_Left(int degree){
    float H = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT + degree)];
    float CA = oradar_shared_buffer[RP_TO_ORADAR_IDX(LEFT)];

    float missing_side = sqrt(pow(H,2) + pow(CA,2) - (2*H*CA*cos(Oradar_S2L_Degrees_To_Radians(degree))));
    float correction_angle = (pow(CA,2) + pow(missing_side,2) - pow(H,2))/(2*CA*missing_side);
    float computed_angle = Oradar_S2L_Radians_To_Degrees(acos(correction_angle));
    float final_correction = 90 - computed_angle;
    printf("H: %f, CA: %f, lado_faltante: %f, angulo_correcion: %f, comparacion: %f, correcion_final: %f\n", H, CA, missing_side, correction_angle, computed_angle, final_correction);
    return (int)(final_correction*10);

}

int Oradar_S2L_Correction_For_Triangles_Right(int degree){
    float CA = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT)];
    float H = oradar_shared_buffer[RP_TO_ORADAR_IDX(RIGHT - degree)];

    float missing_side = sqrt(pow(H,2) + pow(CA,2) - (2*H*CA*cos(Oradar_S2L_Degrees_To_Radians(degree))));
    float correction_angle = (pow(CA,2) + pow(missing_side,2) - pow(H,2))/(2*CA*missing_side);
    float computed_angle = Oradar_S2L_Radians_To_Degrees(acos(correction_angle));
    float final_correction = computed_angle - 90;
    //printf("H: %f, CA: %f, lado_faltante: %f, angulo_correcion: %f, comparacion: %f, correcion_final: %f\n", H, CA, missing_side, correction_angle, computed_angle, final_correction);
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
    for(int i = 0; i < 360; i++){
        buffer[i] = oradar_shared_buffer[i];
    }
}

unsigned long Oradar_S2L_Get_Scan_Seq(void){
    return oradar_scan_seq;
}
