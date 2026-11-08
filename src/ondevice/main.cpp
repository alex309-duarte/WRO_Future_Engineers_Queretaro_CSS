 #include "spike.h"
#include "rasp_gpio.h"
#include "common_var.h"
#include <math.h>

#include "Oradar_S2L.h"
#include "lidar_display.h"
#include "parada_emergencia.h"
#include <cstring>

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

static void Uso(const char *programa){
    printf("Uso: %s [--no-display]\n"
           "  sin argumentos: abre la ventana \"Lidar Coordinates\" (VNC) con los\n"
           "                  puntos, las paredes detectadas, su distancia y sus grados\n"
           "  --no-display:   sin ventana\n", programa);
}

// El recorrido completo del reto. Con ventana corre en su propio hilo (Qt
// exige que la ventana viva en el hilo principal); sin ella, en main().
static int Recorrido(void){

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
    // Desde aqui Ctrl+C (o kill, o una caida) para tambien el motor del lidar:
    // la parada de emergencia termina el proceso sin pasar por
    // Oradar_S2L_Close(), y sin esto el lidar se quedaba girando.
    {
        unsigned char parada_lidar[16];
        const int largo = Oradar_S2L_Stop_Frames(parada_lidar, sizeof(parada_lidar));
        if(largo > 0){
            try{
                Parada_Emergencia_Agregar_Puerto(Oradar_S2L_Port(), parada_lidar, largo);
            } catch(const std::exception &e){
                printf("Aviso: Ctrl+C no parara el lidar (%s)\n", e.what());
            }
        }
    }
    
    pthread_create(&writer, NULL, Oradar_S2L_Lidar_Writer_Thread, NULL);
    
    Rasp_Gpio_Init();
    // Placa Hiwonder en lugar del hub SPIKE, sin rele: abre la placa, centra la
    // direccion y calibra el giroscopio con el robot quieto.
    if(Spike_Hiwonder_Init() != 0){
        Rasp_Gpio_Clean();
        Oradar_S2L_Set_Terminating();
        pthread_join(writer, NULL);
        Oradar_S2L_Close();
        return 1;
    }
    Lidar_Display_Enable_Gyro();
    // Sin datos del lidar la distancia vale 0, y los bucles de avance toman 0
    // como "todavia no hay lectura" y siguen: el robot avanzaria a ciegas.
    // Esperar el primer barrido completo; si no llega, salir sin mover nada.
    for(int i = 0; i < 50 && Oradar_S2L_Get_Scan_Seq() == 0; i++){
        usleep(100000);
    }
    if(Oradar_S2L_Get_Scan_Seq() == 0){
        printf("ERROR: el lidar no envia datos (5 s sin barridos). No se mueve nada.\n");
        Rasp_Gpio_Clean();
        Spike_Coast_Motors();
        Spike_Close_Serial();
        Oradar_S2L_Set_Terminating();
        pthread_join(writer, NULL);
        Oradar_S2L_Close();
        return 1;
    }
    printf("Lidar listo: frente %.0f mm, derecha %.0f mm, izquierda %.0f mm\n",
           Lidar_Debounced_Read(RP_TO_ORADAR_IDX(FRONT)),
           Lidar_Debounced_Read(RP_TO_ORADAR_IDX(RIGHT)),
           Lidar_Debounced_Read(RP_TO_ORADAR_IDX(LEFT)));

    Rasp_Gpio_Wait_For_Button();
    Spike_Reset_Gyro(0);
    usleep(200000); //wiating for reset gyro

    //Spike_Center_Vehicle_Short();
    // Prueba: solo avanzar recto (rumbo 0) hasta 1600 mm de la pared de enfrente.
    // La direccion ya quedo centrada en Spike_Hiwonder_Init().
    //Oradar_S2L_Advance_Until_Distance(50, 0, 600);

    // Reto completo, desactivado para la prueba de avance.
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

        sentido = Oradar_S2L_Advance_And_Detect_Side(30, 0);
        printf("sentido %d :\n", sentido);

        if(sentido == right){
            Spike_Turn_For_Degrees(der, 50, 80);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(40, 1350, -90);
            angulo_correccion = Oradar_S2L_Wall_Slope(LEFT);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            while (v < 11){
            Oradar_S2L_Advance_Until_Right_Gap(50, 0);
            Spike_Turn_For_Degrees(der, 40, 80);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(50, 600, -90);
            angulo_correccion = Oradar_S2L_Wall_Slope(LEFT);
            usleep(50);
            Spike_Reset_Gyro(angulo_correccion);
            v = v + 1;
            }

        }
        else if (sentido == left)
        {
            Spike_Turn_For_Degrees(izq, 40, 80);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(50, 1350, 90);
            angulo_correccion = Oradar_S2L_Wall_Slope(RIGHT);
            Spike_Reset_Gyro(angulo_correccion);
            while (v < 11){
            Oradar_S2L_Advance_Until_Left_Gap(50, 0);
            Spike_Turn_For_Degrees(izq, 40, 70);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(50, 600, 90);
            angulo_correccion = Oradar_S2L_Wall_Slope(RIGHT);
            Spike_Reset_Gyro(angulo_correccion);
            v = v + 1;
            }

        }
    }

    else{

        printf("caso adentro\n");

        sentido = Oradar_S2L_Advance_And_Detect_Side(30, 0);
        printf("sentido : %d \n", sentido);

        if(sentido == right){
            Spike_Turn_For_Degrees(der, 30, 70);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(30, 600, -90);
            angulo_correccion = Oradar_S2L_Wall_Slope(LEFT);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            while (v < 11){
            Oradar_S2L_Advance_Until_Right_Gap(30, 0);
            Spike_Turn_For_Degrees(der, 30, 70);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(30, 600, -90);
            angulo_correccion = Oradar_S2L_Wall_Slope(LEFT);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            v = v + 1;
            }

        }
        else if (sentido == left)
        {
            Spike_Turn_For_Degrees(izq, 30, 70);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(30, 600, 90);
            angulo_correccion = Oradar_S2L_Wall_Slope(RIGHT);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            while (v < 11){
            Oradar_S2L_Advance_Until_Left_Gap(30, 0);
            Spike_Turn_For_Degrees(izq, 30, 70);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(30, 600, 90);
            angulo_correccion = Oradar_S2L_Wall_Slope(RIGHT);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            v = v + 1;
            }
        }
    } 
    Oradar_S2L_Advance_Until_Distance(30, 0, 1500);

    /*if (sentido == right){
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

    }*/

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

static volatile int recorrido_terminado = 0;
static int recorrido_resultado = 0;

static void *Recorrido_Thread(void *arg){
    (void)arg;
    recorrido_resultado = Recorrido();
    recorrido_terminado = 1;
    return NULL;
}

int main(int argc, char **argv){

    bool no_display = false;
    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "--no-display") == 0){
            no_display = true;
        } else if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0){
            Uso(argv[0]);
            return 0;
        } else {
            printf("Argumento desconocido: %s\n", argv[i]);
            Uso(argv[0]);
            return 1;
        }
    }

    if(no_display || !Lidar_Display_Available()){
        return Recorrido();
    }

    // La ventana sólo lee el lidar y el giroscopio: el recorrido es el mismo
    // con o sin ella. Se cierra cuando termina el recorrido.
    pthread_t recorrido;
    if(pthread_create(&recorrido, NULL, Recorrido_Thread, NULL) != 0){
        printf("Aviso: no se pudo crear el hilo del recorrido; sigue sin ventana.\n");
        return Recorrido();
    }
    Lidar_Display_Loop(&recorrido_terminado);
    pthread_join(recorrido, NULL);
    return recorrido_resultado;
}

void signal_handler(int signum){
    printf("\nCtrl+C detceted\n");
    Oradar_S2L_Set_Terminating();
    signal(SIGINT, SIG_DFL);
}
