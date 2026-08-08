#include "spike.h"
#include "rasp_gpio.h"
#include "common_var.h"

// Switch de lidar en tiempo de compilacion: make LIDAR=oradar define USE_ORADAR.
// LIDAR_FN(nombre) se expande a Oradar_S2L_nombre o RPLidar_S2L_nombre segun
// el build; el resto de main.cpp no necesita saber cual de los dos esta activo.
#ifdef USE_ORADAR
#include "Oradar_S2L.h"
#define LIDAR_FN(name) Oradar_S2L_##name
#else
#include "RPLidar_S2L.h"
#define LIDAR_FN(name) RPLidar_S2L_##name
#endif

void signal_handler(int signum);

pthread_t writer;

int der = 1;
int izq = -1;

static float lidar_shared_buffer[360]; // Your shared buffer

int main(){

    float distancia_frente;
    float distancia_derecha;
    float distancia_izquierda;
    int v = 0;
    int angulo_correccion = 0;
    int angulo_correccion_t = 0;
    int angulo_correccion_t2 = 0;
    int angulo = 0;

    signal(SIGINT, signal_handler); /* Set interrupt for ctrl+C */
    LIDAR_FN(Init_Lidar)();
    
    pthread_create(&writer, NULL, LIDAR_FN(Lidar_Writer_Thread), NULL);
    
    Rasp_Gpio_Init();
    Rasp_Gpio_Power_On_Spike();
    Spike_Serial_Init();
    Spike_Interpreter();
    Spike_Initialize_Libraries();
    Rasp_Gpio_Wait_For_Button();
    printf("hola4\n");
    Spike_Reset_Gyro(0);
    usleep(200000); //wiating for reset gyro

    LIDAR_FN(Get_Buffer)(&lidar_shared_buffer[0]);
    distancia_frente = lidar_shared_buffer[270];
    distancia_derecha = lidar_shared_buffer[0];
    distancia_izquierda = lidar_shared_buffer[18|0];

    printf("dsitancia derecha : %f\n", distancia_derecha);
    printf("dsitancia izquierda : %f\n", distancia_izquierda);
    printf("dsitancia frente : %f\n", distancia_frente);
    
    /*angulo_correccion_t = LIDAR_FN(Correction_For_Triangles_Left)(7);
    angulo_correccion = LIDAR_FN(Advance_And_Measure_Left_Slope)(80, 357, 0);
    angulo_correccion_t2 = LIDAR_FN(Correction_For_Triangles_Left)(12);
    angulo = LIDAR_FN(Reconcile_Readings)(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
    printf("angulo : %d\n", angulo);
    Spike_Reset_Gyro(angulo);
    usleep(200000);
    
    Spike_Advance_For_Degrees(80, 2000, 0);*/
    Spike_Advance_For_Degrees(80, 500, 0);
    angulo_correccion_t = LIDAR_FN(Slope)(21,RIGHT);
    Spike_Advance_For_Degrees(80, 500, 0);
    angulo_correccion_t2 = LIDAR_FN(Slope)(21,RIGHT);
    angulo_correccion = LIDAR_FN(Average)(angulo_correccion_t,angulo_correccion_t2);
    Spike_Reset_Gyro(angulo_correccion);
    usleep(200000);
    Spike_Advance_For_Degrees(80, 700, 0);

    /*if((distancia_derecha > 700) || (distancia_izquierda > 700)){

        printf("caso afuera\n");

        direction sentido = LIDAR_FN(Advance_And_Detect_Side)(60, 0);
        printf("sentido %d :\n", sentido);

        if(sentido == right){
            Spike_Turn_For_Degrees(der, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 1142, -90);
            angulo_correccion_t = LIDAR_FN(Correction_For_Triangles_Left)(12);
            angulo_correccion = LIDAR_FN(Advance_And_Measure_Left_Slope)(80, 357, -90);
            angulo_correccion_t2 = LIDAR_FN(Correction_For_Triangles_Left)(12);
            angulo = LIDAR_FN(Reconcile_Readings)(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            usleep(200000);
            while (v < 11){
            LIDAR_FN(Advance_Until_Right_Gap)(80, 0);
            Spike_Turn_For_Degrees(der, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 428, -90);
            angulo_correccion_t = LIDAR_FN(Correction_For_Triangles_Left)(12);
            angulo_correccion = LIDAR_FN(Advance_And_Measure_Left_Slope)(80, 357, -90);
            angulo_correccion_t2 = LIDAR_FN(Correction_For_Triangles_Left)(12);
            angulo = LIDAR_FN(Reconcile_Readings)(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            printf("v : %d, angulo : %d \n", v, angulo);
            usleep(200000);
            v = v + 1;
            }

        }
        else if (sentido == left)
        {
            Spike_Turn_For_Degrees(izq, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 1142, 90);
            angulo_correccion_t = LIDAR_FN(Correction_For_Triangles_Right)(12);
            angulo_correccion = LIDAR_FN(Advance_And_Measure_Right_Slope)(80, 357, 90);
            angulo_correccion_t2 = LIDAR_FN(Correction_For_Triangles_Right)(12);
            angulo = LIDAR_FN(Reconcile_Readings)(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            usleep(200000);
            while (v < 11){
            LIDAR_FN(Advance_Until_Left_Gap)(80, 0);
            Spike_Turn_For_Degrees(izq, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 428, 90);
            angulo_correccion_t = LIDAR_FN(Correction_For_Triangles_Right)(12);
            angulo_correccion = LIDAR_FN(Advance_And_Measure_Right_Slope)(80, 357, 90);
            angulo_correccion_t2 = LIDAR_FN(Correction_For_Triangles_Right)(12);
            angulo = LIDAR_FN(Reconcile_Readings)(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            printf("v : %d, angulo : %d \n", v, angulo);
            usleep(200000);
            v = v + 1;
            }

        }
    }

    else{

        printf("caso adentro\n");

        direction sentido = LIDAR_FN(Advance_And_Detect_Side)(60, 0);
        printf("sentido : %d \n", sentido);

        if(sentido == right){
            Spike_Turn_For_Degrees(der, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 428, -90);
            angulo_correccion_t = LIDAR_FN(Correction_For_Triangles_Left)(12);
            angulo_correccion = LIDAR_FN(Advance_And_Measure_Left_Slope)(80, 357, -90);
            angulo_correccion_t2 = LIDAR_FN(Correction_For_Triangles_Left)(12);
            angulo = LIDAR_FN(Reconcile_Readings)(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            usleep(200000);
            while (v < 11){
            LIDAR_FN(Advance_Until_Right_Gap)(80, 0);
            Spike_Turn_For_Degrees(der, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 428, -90);
            angulo_correccion_t = LIDAR_FN(Correction_For_Triangles_Left)(12);
            angulo_correccion = LIDAR_FN(Advance_And_Measure_Left_Slope)(80, 357, -90);
            angulo_correccion_t2 = LIDAR_FN(Correction_For_Triangles_Left)(12);
            angulo = LIDAR_FN(Reconcile_Readings)(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            printf("v : %d, angulo : %d \n", v, angulo);
            usleep(200000);
            v = v + 1;
            }

        }
        else if (sentido == left)
        {
            Spike_Turn_For_Degrees(izq, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 428, 90);
            angulo_correccion_t = LIDAR_FN(Correction_For_Triangles_Right)(12);
            angulo_correccion = LIDAR_FN(Advance_And_Measure_Right_Slope)(80, 357, 90);
            angulo_correccion_t2 = LIDAR_FN(Correction_For_Triangles_Right)(12);
            angulo = LIDAR_FN(Reconcile_Readings)(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            usleep(200000);
            while (v < 11){
            LIDAR_FN(Advance_Until_Left_Gap)(80, 0);
            Spike_Turn_For_Degrees(izq, 100, 88);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(80, 428, 90);
            angulo_correccion_t = LIDAR_FN(Correction_For_Triangles_Right)(12);
            angulo_correccion = LIDAR_FN(Advance_And_Measure_Right_Slope)(80, 357, 90);
            angulo_correccion_t2 = LIDAR_FN(Correction_For_Triangles_Right)(12);
            angulo = LIDAR_FN(Reconcile_Readings)(angulo_correccion_t, angulo_correccion, angulo_correccion_t2);
            Spike_Reset_Gyro(angulo);
            printf("v : %d, angulo : %d \n", v, angulo);
            usleep(200000);
            v = v + 1;
            }
        }
    } 
    
    printf("ultima funcion\n");
    LIDAR_FN(Advance_Until_Distance)(80, 0, 1400);
    */
    Rasp_Gpio_Clean();
    Spike_Coast_Motors();
    Spike_Close_Serial();

    // Stop and join the writer thread before releasing the lidar driver it
    // still reads from, otherwise Close() can delete the driver object while
    // the thread is mid-scan (use-after-free).
    LIDAR_FN(Set_Terminating)();
    pthread_join(writer, NULL);
    LIDAR_FN(Close)();

    return 0;
}

void signal_handler(int signum){
    printf("\nCtrl+C detceted\n");
    LIDAR_FN(Set_Terminating)();
    signal(SIGINT, SIG_DFL);
}
