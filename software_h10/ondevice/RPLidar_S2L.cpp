#include "RPLidar_S2L.h"
#include "spike.h"

using namespace sl;

pthread_mutex_t rplidar_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

const char * serial_port_path = "/dev/ttyUSB0";
ILidarDriver * lidar_driver;
IChannel * serial_channel;
sl_result  op_result;

static volatile int rplidar_terminating = 0;
static volatile float rplidar_shared_buffer[360]; // Your shared buffer

void RPLidar_S2L_Init_Lidar(void){
    lidar_driver = *createLidarDriver();
    if (!lidar_driver) {
        fprintf(stderr, "insufficent memory, exit\n");
        exit(-2);
    }
    sl_lidar_response_device_info_t devinfo;
    //bool connectSuccess = false; //variable que no se utiliza
    serial_channel = (*createSerialPortChannel(serial_port_path, 1000000));
    if (SL_IS_OK((lidar_driver)->connect(serial_channel))) {
        op_result = lidar_driver->getDeviceInfo(devinfo);

        if (SL_IS_OK(op_result))
        {
	        //connectSuccess = true;
            printf("\nconnection successful \n");
        }
        else{
            delete lidar_driver;
				lidar_driver = NULL;
        }
    }

    lidar_driver->setMotorSpeed(DEFAULT_MOTOR_SPEED);
    usleep(200000);
    // start scan...
    lidar_driver->startScan(0,1);
    //lidar_driver->startScan(false, false);
}

void *RPLidar_S2L_Lidar_Writer_Thread(void *arg){

    float angle_deg = 0;
    sl_lidar_response_measurement_node_hq_t nodes[8192];
    size_t   count = _countof(nodes);
    DistanceAccumulator average[360] = {0};

    while(rplidar_terminating == 0){

        op_result = lidar_driver->grabScanDataHq(nodes, count);
        if (SL_IS_OK(op_result)) {
            for (int i = 0; i < 360; i++) {
                average[i].sample_count = 0;
                average[i].distance_sum = 0;
            }
            lidar_driver->ascendScanData(nodes, count);
            //printf("\n counts: %d \n", count);
            for (int pos = 0; pos < (int)count ; ++pos) {
                if((nodes[pos].quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT) == 47){
                    angle_deg = ((nodes[pos].angle_z_q14) * 90.f) / 16384.f;
                    average[int(angle_deg)].distance_sum += nodes[pos].dist_mm_q2/4.0f;
                    average[int(angle_deg)].sample_count += 1;
                }
                /*if( int(angle_deg) == 90)
                {

                    printf("%s theta: %03.2f Dist: %08.2f Q: %d \n",
                        (nodes[pos].flag & SL_LIDAR_RESP_HQ_FLAG_SYNCBIT) ?"S ":"  ",
                        (nodes[pos].angle_z_q14 * 90.f) / 16384.f,
                        nodes[pos].dist_mm_q2/4.0f,
                        nodes[pos].quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);

                }*/
            }

            pthread_mutex_lock(&rplidar_buffer_mutex);
            for (int i = 0; i < 360; i++) {
                if(average[i].sample_count > 0){
                    rplidar_shared_buffer[i] = average[i].distance_sum/average[i].sample_count;
                }
                else{
                    //printf("No measurements for angle %d\n", i);
                }
            }
            pthread_mutex_unlock(&rplidar_buffer_mutex); // Unlock after writing
        }
    }
    return NULL;
}

float RPLidar_S2L_Radians_To_Degrees(float radians){
    double degrees = ((radians * 180.f)/ M_PI);
    return degrees;
}

float RPLidar_S2L_Degrees_To_Radians(float degrees){
    double radians = ((degrees * M_PI)/ 180.f);
    return radians;
}

direction RPLidar_S2L_Advance_And_Detect_Side(int speed, int reference){

    float front_distance;
    float right_distance;
    float left_distance;
    float back_distance;

    front_distance = rplidar_shared_buffer[FRONT];
    right_distance = rplidar_shared_buffer[RIGHT];
    left_distance = rplidar_shared_buffer[LEFT];


    while((rplidar_terminating == 0) && (((right_distance < 1350) && (left_distance < 1350)) || (front_distance > 1100))){
        front_distance = rplidar_shared_buffer[FRONT];
        right_distance = rplidar_shared_buffer[RIGHT];
        left_distance = rplidar_shared_buffer[LEFT];
        back_distance = rplidar_shared_buffer[LEFT];
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

void RPLidar_S2L_Advance_Until_Left_Gap(int speed, int reference){

    float front_distance;
    float left_distance;

    front_distance = rplidar_shared_buffer[FRONT];
    left_distance = rplidar_shared_buffer[LEFT];

    while((rplidar_terminating == 0) && (((left_distance < 1350) && (front_distance > 500)) || (front_distance > 1100))){
        front_distance = rplidar_shared_buffer[FRONT];
        left_distance = rplidar_shared_buffer[LEFT];
        Spike_Forward(speed,reference);
        usleep(1000);
        //printf("dsitancia izquierda : %f\n", left_distance);
        //printf("dsitancia frente : %f\n", front_distance);
    }

}

void RPLidar_S2L_Advance_Until_Right_Gap(int speed, int reference){

    float front_distance;
    float right_distance;

    front_distance = rplidar_shared_buffer[FRONT];
    right_distance = rplidar_shared_buffer[RIGHT];

    while((rplidar_terminating == 0) && (((right_distance < 1350) && (front_distance > 500))  || (front_distance > 1100))){
        front_distance = rplidar_shared_buffer[FRONT];
        right_distance = rplidar_shared_buffer[RIGHT];
        Spike_Forward(speed,reference);
        //printf("dsitancia derecha : %f\n", right_distance);
        //printf("dsitancia frente : %f\n", front_distance);
        usleep(1000);
    }

}

int RPLidar_S2L_Advance_And_Measure_Left_Slope(int speed, int degrees, int reference){
    float y1 = rplidar_shared_buffer[LEFT];

    Spike_Advance_For_Degrees(speed, degrees, reference);

    float y2 = rplidar_shared_buffer[LEFT];

    float distance_delta = y1-y2;
    int slope = (int)(10 *(RPLidar_S2L_Radians_To_Degrees(atan(distance_delta/(175.0*degrees/360.0)))));
    printf("pendiente: %d\n", slope);
    return slope;
}

int RPLidar_S2L_Advance_And_Measure_Right_Slope(int speed, int degrees, int reference){
    float y1 = rplidar_shared_buffer[RIGHT];

    Spike_Advance_For_Degrees(speed, degrees, reference);

    float y2 = rplidar_shared_buffer[RIGHT];

    float distance_delta = y1-y2;
    int slope = (int)(-10 *(RPLidar_S2L_Radians_To_Degrees(atan(distance_delta/(175.0*degrees/360.0)))));
    printf("pendiente: %d, y1: %f, y2: %f\n", slope, y1, y2);
    return slope;
}

void RPLidar_S2L_Advance_Until_Distance(int speed, int reference, int target_distance){
    float front_distance;

    front_distance = rplidar_shared_buffer[FRONT];

    while((rplidar_terminating == 0) && (front_distance > target_distance)){
        front_distance = rplidar_shared_buffer[FRONT];
        Spike_Forward(speed,reference);
        //printf("dsitancia frente : %f\n", front_distance);
        usleep(1000);
    }
}

int RPLidar_S2L_Correction_For_Triangles_Left(int degree){
    float H = rplidar_shared_buffer[LEFT + degree];
    float CA = rplidar_shared_buffer[LEFT];

    float missing_side = sqrt(pow(H,2) + pow(CA,2) - (2*H*CA*cos(RPLidar_S2L_Degrees_To_Radians(degree))));
    float correction_angle = (pow(CA,2) + pow(missing_side,2) - pow(H,2))/(2*CA*missing_side);
    float computed_angle = RPLidar_S2L_Radians_To_Degrees(acos(correction_angle));
    float final_correction = 90 - computed_angle;
    printf("H: %f, CA: %f, lado_faltante: %f, angulo_correcion: %f, comparacion: %f, correcion_final: %f\n", H, CA, missing_side, correction_angle, computed_angle, final_correction);
    return (int)(final_correction*10);

}

int RPLidar_S2L_Correction_For_Triangles_Right(int degree){
    float CA = rplidar_shared_buffer[RIGHT];
    float H = rplidar_shared_buffer[RIGHT - degree];

    float missing_side = sqrt(pow(H,2) + pow(CA,2) - (2*H*CA*cos(RPLidar_S2L_Degrees_To_Radians(degree))));
    float correction_angle = (pow(CA,2) + pow(missing_side,2) - pow(H,2))/(2*CA*missing_side);
    float computed_angle = RPLidar_S2L_Radians_To_Degrees(acos(correction_angle));
    float final_correction = computed_angle - 90;
    //printf("H: %f, CA: %f, lado_faltante: %f, angulo_correcion: %f, comparacion: %f, correcion_final: %f\n", H, CA, missing_side, correction_angle, computed_angle, final_correction);
    return (int)(final_correction*10);

}

int RPLidar_S2L_Reconcile_Readings(int reading_a, int reading_b, int reading_c){
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

int RPLidar_S2L_Slope(const int point, const int middle_point){
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    float x_point[point] = {0};
    float y_point[point] = {0};
    float dis[point] = {0};
    int angle[point] = {0};

    for(int i = 0; i < point; i++){
        dis[i] = rplidar_shared_buffer[middle_point - point + i];
        angle[i] = middle_point - point  +i;
    }

    for(int i = 0; i < point; i++) {
        x_point[i] = (dis[i] * cos(RPLidar_S2L_Degrees_To_Radians(angle[i])));
        y_point[i] = (dis[i] * sin(RPLidar_S2L_Degrees_To_Radians(angle[i])));
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
    int p = (10)*(RPLidar_S2L_Radians_To_Degrees(atan((point * sum_xy - sum_x * sum_y) / det)));
    printf("pendiente : %d\n",p);
    return (p);

}

int RPLidar_S2L_Slope2(const int point, const int middle_point){
    float x_point[point] = {0};
    float y_point[point] = {0};
    float dis[point] = {0};
    int angle[point] = {0};

    for(int i = 0; i < point; i++){
        dis[i] = rplidar_shared_buffer[middle_point - point + i];
        angle[i] = middle_point - point  +i;
    }

    for(int i = 0; i < point; i++) {
        x_point[i] = (dis[i] * cos(RPLidar_S2L_Degrees_To_Radians(angle[i])));
        y_point[i] = (dis[i] * sin(RPLidar_S2L_Degrees_To_Radians(angle[i])));
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

    int p = (10)*(RPLidar_S2L_Radians_To_Degrees(0.5 * atan2(2.0 * sxy, sxx - syy)));
    printf("pendiente2 : %d\n",p);
    return (p);
}

int RPLidar_S2L_Average(int arg_1, int arg_2){
    int average = (arg_1 + arg_2)/2;
    printf("correcion: %d\n",average);
    return average;
}

void RPLidar_S2L_Close(void){
    lidar_driver->stop();
    if(lidar_driver){
        lidar_driver->setMotorSpeed(0);
        delete lidar_driver;
        lidar_driver = NULL;
    }
}

void RPLidar_S2L_Set_Terminating(void){
    rplidar_terminating = 1;
}

void RPLidar_S2L_Get_Buffer(float *buffer){
    for(int i = 0; i < 360; i++){
        buffer[i] = rplidar_shared_buffer[i];
    }
}
