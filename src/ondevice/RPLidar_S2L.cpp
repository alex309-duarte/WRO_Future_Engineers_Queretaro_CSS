// Original RPLidar S2L driver and mission helpers (see RPLidar_S2L.h). The
// buffer is indexed directly by the lidar angle in degrees (0=front,
// clockwise), so no RP_TO_ORADAR_IDX conversion is needed here.

#include "RPLidar_S2L.h"
#include "spike.h"

using namespace sl;

pthread_mutex_t rplidar_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;  // protects the buffer writes

const char * serial_port_path = "/dev/ttyUSB0";  // USB-serial adapter of the lidar
ILidarDriver * lidar_driver;
IChannel * serial_channel;
sl_result  op_result;

static volatile int rplidar_terminating = 0;
static volatile float rplidar_shared_buffer[360]; // distance in mm per degree of lidar angle

// Creates the SDK driver, opens the serial channel at 1 Mbaud and reads the
// device info, then starts the motor (default speed), waits 200 ms for it to
// spin up and starts a standard scan. Exits the process (-2) if the driver
// cannot be allocated.
void RPLidar_S2L_Init_Lidar(void){
    lidar_driver = *createLidarDriver();
    if (!lidar_driver) {
        fprintf(stderr, "insufficient memory, exit\n");
        exit(-2);
    }
    sl_lidar_response_device_info_t devinfo;
    //bool connectSuccess = false; //unused variable
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

// Writer thread: for every full scan, averages the returns that fall in each
// 1 deg bucket and publishes them under the mutex. Only points whose quality
// field (after the shift) equals 47 are used; a bucket with no such point
// keeps its previous value. dist_mm_q2 is mm * 4 and angle_z_q14
// is deg * 16384 / 90.
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

// Waits until a side opening (> 1350 mm) appears with the front wall
// <= 1100 mm and returns that side (right = clockwise run), or invalid.
// The drive command is commented out here, so this version only prints the
// four distances while it waits.
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
        printf("right distance: %f\n", right_distance);
        printf("left distance: %f\n", left_distance);
        printf("front distance: %f\n", front_distance);
        printf("rear distance: %f\n", back_distance);
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

// Drives forward holding heading "reference" until the front wall is
// <= 1100 mm and either the left wall is >= 1350 mm or the front wall is
// <= 500 mm (the next corner of a counter-clockwise lap).
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
        //printf("left distance : %f\n", left_distance);
        //printf("front distance : %f\n", front_distance);
    }

}

// Clockwise-lap mirror of RPLidar_S2L_Advance_Until_Left_Gap (right wall).
void RPLidar_S2L_Advance_Until_Right_Gap(int speed, int reference){

    float front_distance;
    float right_distance;

    front_distance = rplidar_shared_buffer[FRONT];
    right_distance = rplidar_shared_buffer[RIGHT];

    while((rplidar_terminating == 0) && (((right_distance < 1350) && (front_distance > 500))  || (front_distance > 1100))){
        front_distance = rplidar_shared_buffer[FRONT];
        right_distance = rplidar_shared_buffer[RIGHT];
        Spike_Forward(speed,reference);
        //printf("right distance : %f\n", right_distance);
        //printf("front distance : %f\n", front_distance);
        usleep(1000);
    }

}

// Reads the left distance, drives "degrees" motor degrees (175 mm of travel
// per 360 motor degrees) and reads it again; returns
// atan(distance change / travel) in tenths of a degree.
int RPLidar_S2L_Advance_And_Measure_Left_Slope(int speed, int degrees, int reference){
    float y1 = rplidar_shared_buffer[LEFT];

    Spike_Advance_For_Degrees(speed, degrees, reference);

    float y2 = rplidar_shared_buffer[LEFT];

    float distance_delta = y1-y2;
    int slope = (int)(10 *(RPLidar_S2L_Radians_To_Degrees(atan(distance_delta/(175.0*degrees/360.0)))));
    printf("slope: %d\n", slope);
    return slope;
}

// Same with the right wall (sign reversed).
int RPLidar_S2L_Advance_And_Measure_Right_Slope(int speed, int degrees, int reference){
    float y1 = rplidar_shared_buffer[RIGHT];

    Spike_Advance_For_Degrees(speed, degrees, reference);

    float y2 = rplidar_shared_buffer[RIGHT];

    float distance_delta = y1-y2;
    int slope = (int)(-10 *(RPLidar_S2L_Radians_To_Degrees(atan(distance_delta/(175.0*degrees/360.0)))));
    printf("slope: %d, y1: %f, y2: %f\n", slope, y1, y2);
    return slope;
}

// Drives forward until the front wall is at <= target_distance mm.
void RPLidar_S2L_Advance_Until_Distance(int speed, int reference, int target_distance){
    float front_distance;

    front_distance = rplidar_shared_buffer[FRONT];

    while((rplidar_terminating == 0) && (front_distance > target_distance)){
        front_distance = rplidar_shared_buffer[FRONT];
        Spike_Forward(speed,reference);
        //printf("front distance : %f\n", front_distance);
        usleep(1000);
    }
}

// Robot angle to the left wall from the ray at 270 deg (CA) and the ray
// "degree" deg further (H), using the law of cosines; returns (90 - angle
// between CA and the wall) in tenths of a degree, 0 when parallel.
int RPLidar_S2L_Correction_For_Triangles_Left(int degree){
    float H = rplidar_shared_buffer[LEFT + degree];
    float CA = rplidar_shared_buffer[LEFT];

    float missing_side = sqrt(pow(H,2) + pow(CA,2) - (2*H*CA*cos(RPLidar_S2L_Degrees_To_Radians(degree))));
    float correction_angle = (pow(CA,2) + pow(missing_side,2) - pow(H,2))/(2*CA*missing_side);
    float computed_angle = RPLidar_S2L_Radians_To_Degrees(acos(correction_angle));
    float final_correction = 90 - computed_angle;
    printf("H: %f, CA: %f, missing_side: %f, correction_angle: %f, computed_angle: %f, final_correction: %f\n", H, CA, missing_side, correction_angle, computed_angle, final_correction);
    return (int)(final_correction*10);

}

// Right-wall mirror of RPLidar_S2L_Correction_For_Triangles_Left.
int RPLidar_S2L_Correction_For_Triangles_Right(int degree){
    float CA = rplidar_shared_buffer[RIGHT];
    float H = rplidar_shared_buffer[RIGHT - degree];

    float missing_side = sqrt(pow(H,2) + pow(CA,2) - (2*H*CA*cos(RPLidar_S2L_Degrees_To_Radians(degree))));
    float correction_angle = (pow(CA,2) + pow(missing_side,2) - pow(H,2))/(2*CA*missing_side);
    float computed_angle = RPLidar_S2L_Radians_To_Degrees(acos(correction_angle));
    float final_correction = computed_angle - 90;
    //printf("H: %f, CA: %f, missing_side: %f, correction_angle: %f, computed_angle: %f, final_correction: %f\n", H, CA, missing_side, correction_angle, computed_angle, final_correction);
    return (int)(final_correction*10);

}

// Returns b if it agrees (within 30) with a or c, else the mean of a and c
// if those agree, else b.
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

// Ordinary least-squares line through the "point" rays just below
// middle_point (converted to x,y in mm); returns atan(slope) in tenths of a
// degree, or 0 if all x are equal.
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
        printf("Error: the system has no unique solution (all X values are equal).\n");
        return 0;
    }
    int p = (10)*(RPLidar_S2L_Radians_To_Degrees(atan((point * sum_xy - sum_x * sum_y) / det)));
    printf("slope: %d\n",p);
    return (p);

}

// Orthogonal regression (PCA) over the same window: minimises the
// perpendicular distance, so it also works for walls parallel to the y axis.
// Returns 0.5 * atan2(2*Sxy, Sxx - Syy) in tenths of a degree.
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
    printf("slope2: %d\n",p);
    return (p);
}

// Integer mean of two readings (prints it).
int RPLidar_S2L_Average(int arg_1, int arg_2){
    int average = (arg_1 + arg_2)/2;
    printf("correction: %d\n",average);
    return average;
}

// Stops the scan, sets the motor speed to 0 and deletes the driver.
void RPLidar_S2L_Close(void){
    lidar_driver->stop();
    if(lidar_driver){
        lidar_driver->setMotorSpeed(0);
        delete lidar_driver;
        lidar_driver = NULL;
    }
}

// Makes the writer thread and every blocking loop return.
void RPLidar_S2L_Set_Terminating(void){
    rplidar_terminating = 1;
}

// Copies the 360-entry buffer (mm).
void RPLidar_S2L_Get_Buffer(float *buffer){
    for(int i = 0; i < 360; i++){
        buffer[i] = rplidar_shared_buffer[i];
    }
}
