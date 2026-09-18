#include "spike.h"
#include "rasp_gpio.h"
#include "common_var.h"
#include <math.h>

#include "Oradar_S2L.h"

void signal_handler(int signum);

pthread_t writer;

int der = 1;
int izq = -1;

static float lidar_shared_buffer[360]; // Your shared buffer

// Debounce: solo se acepta una lectura cuando 3 muestras consecutivas
// difieren entre si por menos de 3 cm.
#define LIDAR_DEBOUNCE_SAMPLES 3
#define LIDAR_DEBOUNCE_TOLERANCE_MM 30.0f // 3 cm

static float Lidar_Debounced_Read(int angle_index){
    float samples[LIDAR_DEBOUNCE_SAMPLES];
    int count = 0;

    while(count < LIDAR_DEBOUNCE_SAMPLES){
        Oradar_S2L_Get_Buffer(&lidar_shared_buffer[0]);
        float value = lidar_shared_buffer[angle_index];

        if(count == 0 || fabsf(value - samples[count - 1]) <= LIDAR_DEBOUNCE_TOLERANCE_MM){
            samples[count] = value;
            count++;
        } else {
            samples[0] = value;
            count = 1;
        }
    }

    return samples[LIDAR_DEBOUNCE_SAMPLES - 1];
}

int main(){

    float distancia_frente;
    float distancia_derecha;
    float distancia_izquierda;
    direction sentido;
    int v = 0;
    int angulo_correccion = 0;
    int angulo_correccion_t = 0;
    int angulo_correccion_t2 = 0;

    signal(SIGINT, signal_handler); /* Set interrupt for ctrl+C */
    Oradar_S2L_Init_Lidar();
    
    pthread_create(&writer, NULL, Oradar_S2L_Lidar_Writer_Thread, NULL);
    
    Rasp_Gpio_Init();
    Rasp_Gpio_Power_On_Spike();
    Spike_Serial_Init();
    Spike_Interpreter();
    Spike_Initialize_Libraries();
    Rasp_Gpio_Wait_For_Button();
    Spike_Reset_Gyro(0);
    usleep(200000); //wiating for reset gyro
    Spike_Center_Vehicle_Short();

    //while(1){
      //  angulo_correccion_t = Oradar_S2L_Wall_Slope(LEFT);
       // usleep(100000);
        
    //}

    distancia_frente = Lidar_Debounced_Read(270);
    distancia_derecha = Lidar_Debounced_Read(0);
    distancia_izquierda = Lidar_Debounced_Read(180);

    printf("dsitancia derecha : %f\n", distancia_derecha);
    printf("dsitancia izquierda : %f\n", distancia_izquierda);
    printf("dsitancia frente : %f\n", distancia_frente);

    if((distancia_derecha > 800) || (distancia_izquierda > 800)){

        printf("caso afuera\n");

        sentido = Oradar_S2L_Advance_And_Detect_Side(80, 0);
        printf("sentido %d :\n", sentido);

        if(sentido == right){
            Spike_Turn_For_Degrees(der, 100, 70);
            Spike_Center_Vehicle_Short(true);
            Spike_Advance_For_Degrees(80, 1350, -90);
            angulo_correccion = Oradar_S2L_Wall_Slope(LEFT);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            while (v < 10){
            Oradar_S2L_Advance_Until_Right_Gap(80, 0);
            Spike_Turn_For_Degrees(der, 100, 70);
            Spike_Center_Vehicle_Short(true);
            Spike_Advance_For_Degrees(80, 600, -90);
            angulo_correccion = Oradar_S2L_Wall_Slope(LEFT);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            v = v + 1;
            }

        }
        else if (sentido == left)
        {
            Spike_Turn_For_Degrees(izq, 100, 70);
            Spike_Center_Vehicle_Short(true);
            Spike_Advance_For_Degrees(80, 1350, 90);
            angulo_correccion = Oradar_S2L_Wall_Slope(RIGHT);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            while (v < 10){
            Oradar_S2L_Advance_Until_Left_Gap(80, 0);
            Spike_Turn_For_Degrees(izq, 100, 70);
            Spike_Center_Vehicle_Short(true);
            Spike_Advance_For_Degrees(80, 600, 90);
            angulo_correccion = Oradar_S2L_Wall_Slope(RIGHT);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            v = v + 1;
            }

        }
    }

    else{

        printf("caso adentro\n");

        sentido = Oradar_S2L_Advance_And_Detect_Side(60, 0);
        printf("sentido : %d \n", sentido);

        if(sentido == right){
            Spike_Turn_For_Degrees(der, 100, 70);
            Spike_Center_Vehicle_Short(true);
            Spike_Advance_For_Degrees(80, 600, -90);
            angulo_correccion = Oradar_S2L_Wall_Slope(LEFT);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            while (v < 10){
            Oradar_S2L_Advance_Until_Right_Gap(80, 0);
            Spike_Turn_For_Degrees(der, 100, 70);
            Spike_Center_Vehicle_Short(true);
            Spike_Advance_For_Degrees(80, 600, -90);
            angulo_correccion = Oradar_S2L_Wall_Slope(LEFT);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            v = v + 1;
            }

        }
        else if (sentido == left)
        {
            Spike_Turn_For_Degrees(izq, 100, 70);
            Spike_Center_Vehicle_Short(true);
            Spike_Advance_For_Degrees(80, 600, 90);
            angulo_correccion = Oradar_S2L_Wall_Slope(RIGHT);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            while (v < 10){
            Oradar_S2L_Advance_Until_Left_Gap(80, 0);
            Spike_Turn_For_Degrees(izq, 100, 70);
            Spike_Center_Vehicle_Short(true);
            Spike_Advance_For_Degrees(80, 600, 90);
            angulo_correccion = Oradar_S2L_Wall_Slope(RIGHT);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            v = v + 1;
            }
        }
    } 

    if (sentido == right){
        if(distancia_izquierda < 400){
            Oradar_S2L_Advance_Until_Distance(80, 0, 450);
            printf("seccion por fuera\n");
        }   
        else if((distancia_izquierda > 400) && (distancia_izquierda < 600)){
            Oradar_S2L_Advance_Until_Distance(80, 0, 750);
            printf("en medio\n");
        }  
        else if (distancia_izquierda > 600){
            Oradar_S2L_Advance_Until_Distance(80, 0, 1000);
        printf("seccion por dentro\n");
        }

        Spike_Turn_For_Degrees(der, 100, 70);
        Spike_Center_Vehicle_Short();
        Spike_Advance_For_Degrees(80, 300, -90);

        if(distancia_frente < 1500){
            Oradar_S2L_Advance_Until_Distance(80, -90, 1400);
            printf("parte de adelante\n");
        }
        else if(distancia_frente > 1500){
            Oradar_S2L_Advance_Until_Distance(80, -90, 1850);
        printf("parte de atras\n");
        }
    }

    else if (sentido == left){
        if(distancia_derecha < 400){
            Oradar_S2L_Advance_Until_Distance(80, 0, 450);
            printf("seccion por fuera\n");
        }   
        else if((distancia_derecha > 400) && (distancia_derecha < 600)){
            Oradar_S2L_Advance_Until_Distance(80, 0, 750);
            printf("en medio\n");
        }  
        else if (distancia_derecha > 600){
            Oradar_S2L_Advance_Until_Distance(80, 0, 1000);
            printf("seccion por dentro\n");
        }

        Spike_Turn_For_Degrees(izq, 100, 70);
        Spike_Center_Vehicle_Short();
        Spike_Advance_For_Degrees(80, 300, 90);

        if(distancia_frente < 1500){
            Oradar_S2L_Advance_Until_Distance(80, 90, 1400);
            printf("parte de atras\n");
        }
        else if(distancia_frente > 1500){
            Oradar_S2L_Advance_Until_Distance(80, 90, 1850);
            printf("parte de adelante\n");
        }

    }
    
    printf("acabe");
    
    Rasp_Gpio_Clean();
    Spike_Coast_Motors();
    Spike_Close_Serial();

    // Stop and join the writer thread before releasing the lidar driver it
    // still reads from, otherwise Close() can delete the driver object while
    // the thread is mid-scan (use-after-free).
    Oradar_S2L_Set_Terminating();
    pthread_join(writer, NULL);
    Oradar_S2L_Close();

    return 0;
}

void signal_handler(int signum){
    printf("\nCtrl+C detceted\n");
    Oradar_S2L_Set_Terminating();
    signal(SIGINT, SIG_DFL);
}
