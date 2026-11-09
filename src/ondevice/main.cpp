// WRO 2026 Future Engineers - Open Challenge program, design v7 (team CSS).
// Runs on the Raspberry Pi 5: waits for the start button (GPIO4), detects the
// driving direction with the RPLidar S2L, then drives 3 laps (12 corners)
// following the walls with the gyro of the Hiwonder board, re-aligning the
// heading on the wall after every turn, and stops in the starting section.
// Optional "Lidar Coordinates" window (lidar_display) for debugging over VNC.
 #include "spike.h"
#include "rasp_gpio.h"
#include "common_var.h"
#include <math.h>

#include "Oradar_S2L.h"
#include "lidar_display.h"
#include "parada_emergencia.h"
#include <cstring>

void signal_handler(int signum);

pthread_t writer;  // lidar writer thread (Oradar_S2L_Lidar_Writer_Thread)

// Turn direction argument of Spike_Turn_For_Degrees: right / left.
int der = 1;
int izq = -1;

static float lidar_shared_buffer[360]; // local copy of the lidar buffer (mm per degree)

// Debounce: a reading is accepted only when 3 consecutive samples differ
// from each other by less than 3 cm.
#define LIDAR_DEBOUNCE_SAMPLES 3
#define LIDAR_DEBOUNCE_TOLERANCE_MM 30.0f // 3 cm

// Returns the distance (mm) at buffer index angle_index (Oradar layout,
// e.g. 270 = front) once LIDAR_DEBOUNCE_SAMPLES consecutive copies of the
// buffer agree within LIDAR_DEBOUNCE_TOLERANCE_MM. Blocks until they do.
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

// Prints the command-line usage ("Uso" = usage).
static void Uso(const char *programa){
    printf("Usage: %s [--no-display]\n"
           "  no arguments:  opens the \"Lidar Coordinates\" window (VNC) with the\n"
           "                 points, the detected walls, their distance and their angle\n"
           "  --no-display:  no window\n", programa);
}

// The complete Open Challenge run ("recorrido"). With the window it runs in
// its own thread (Qt requires the window to live in the main thread); without
// it, in main(). Returns 0 at the end of the run, 1 if the board or the lidar
// could not be started (nothing moves in that case).
static int Recorrido(void){

    float distancia_frente;
    float distancia_derecha;
    float distancia_izquierda;
    direction sentido;
    int v = 0;                       // corners done inside the lap loop
    int angulo_correccion = 0;       // wall angle, tenths of a degree
    int angulo_correccion_t = 0;
    int angulo_correccion_t2 = 0;

    // Initial Ctrl+C handler: asks the lidar thread to stop. The emergency
    // stop handler installed below (parada_emergencia) then replaces it.
    signal(SIGINT, signal_handler); /* Set interrupt for ctrl+C */
    Oradar_S2L_Init_Lidar();
    // From here on Ctrl+C (or kill, or a crash) also stops the lidar motor:
    // the emergency stop ends the process without calling
    // Oradar_S2L_Close(), and without this the lidar would keep spinning.
    {
        unsigned char parada_lidar[16];
        const int largo = Oradar_S2L_Stop_Frames(parada_lidar, sizeof(parada_lidar));
        if(largo > 0){
            try{
                Parada_Emergencia_Agregar_Puerto(Oradar_S2L_Port(), parada_lidar, largo);
            } catch(const std::exception &e){
                printf("Warning: Ctrl+C will not stop the lidar (%s)\n", e.what());
            }
        }
    }
    
    pthread_create(&writer, NULL, Oradar_S2L_Lidar_Writer_Thread, NULL);
    
    Rasp_Gpio_Init();
    // Hiwonder board instead of the SPIKE hub, no relay: opens the board,
    // centres the steering and calibrates the gyro with the robot still.
    if(Spike_Hiwonder_Init() != 0){
        Rasp_Gpio_Clean();
        Oradar_S2L_Set_Terminating();
        pthread_join(writer, NULL);
        Oradar_S2L_Close();
        return 1;
    }
    Lidar_Display_Enable_Gyro();
    // Without lidar data every distance is 0, and the driving loops take 0 as
    // "no reading yet" and keep going: the robot would drive blind.
    // Wait for the first full scan (up to 50 x 100 ms = 5 s); if it does not
    // arrive, exit without moving anything.
    for(int i = 0; i < 50 && Oradar_S2L_Get_Scan_Seq() == 0; i++){
        usleep(100000);
    }
    if(Oradar_S2L_Get_Scan_Seq() == 0){
        printf("ERROR: the lidar is not sending data (no scans for 5 s). Nothing will move.\n");
        Rasp_Gpio_Clean();
        Spike_Coast_Motors();
        Spike_Close_Serial();
        Oradar_S2L_Set_Terminating();
        pthread_join(writer, NULL);
        Oradar_S2L_Close();
        return 1;
    }
    printf("Lidar ready: front %.0f mm, right %.0f mm, left %.0f mm\n",
           Lidar_Debounced_Read(RP_TO_ORADAR_IDX(FRONT)),
           Lidar_Debounced_Read(RP_TO_ORADAR_IDX(RIGHT)),
           Lidar_Debounced_Read(RP_TO_ORADAR_IDX(LEFT)));

    // Wait for the start button, then define the current heading as 0.
    Rasp_Gpio_Wait_For_Button();
    Spike_Reset_Gyro(0);
    usleep(200000); //waiting for the gyro reset

    //Spike_Center_Vehicle_Short();
    // Test (disabled): only drive straight (heading 0) until 600 mm from the
    // front wall. The steering is already centred by Spike_Hiwonder_Init().
    //Oradar_S2L_Advance_Until_Distance(50, 0, 600);

    // Full Open Challenge run.
    Spike_Center_Vehicle_Short();

    // Debug loop (disabled): print the left wall angle continuously.
    //while(1){
      //  angulo_correccion_t = Oradar_S2L_Wall_Slope(LEFT);
       // usleep(100000);
        
    //}

    // Start position: buffer indices 270 = front, 0 = right, 180 = left.
    distancia_frente = Lidar_Debounced_Read(270);
    distancia_derecha = Lidar_Debounced_Read(0);
    distancia_izquierda = Lidar_Debounced_Read(180);

    printf("right distance: %f\n", distancia_derecha);
    printf("left distance: %f\n", distancia_izquierda);
    printf("front distance: %f\n", distancia_frente);

    
    // If one side wall is farther than 800 mm the robot starts with room on
    // that side ("outside" case: faster moves and a longer straight after the
    // first turn); otherwise both walls are close ("inside" case: all moves
    // at 30 % speed).
    //
    // Pattern for each corner, in both cases:
    //   1. Advance_Until_*_Gap: drive at heading 0 until the inner wall ends.
    //   2. Spike_Turn_For_Degrees: full steering lock until |yaw| reaches
    //      70-80 deg (a little less than 90), then centre the steering.
    //   3. Spike_Advance_For_Degrees: drive a fixed distance (motor degrees)
    //      at heading -90 (right turn) / +90 (left turn), which completes the
    //      90 deg turn and brings the robot into the new section.
    //   4. Oradar_S2L_Wall_Slope on the outer wall (continuous along the
    //      section) and Spike_Reset_Gyro(angle): the yaw is redefined so that
    //      heading 0 is parallel to the new wall. This cancels gyro drift and
    //      turn errors at every corner.
    // The first corner is handled before the loop and the loop runs 11 more
    // times: 12 corners = 3 laps.
    if((distancia_derecha > 800) || (distancia_izquierda > 800)){

        printf("outer start case\n");

        // Drive to the first corner and find the driving direction.
        sentido = Oradar_S2L_Advance_And_Detect_Side(30, 0);
        printf("direction: %d\n", sentido);

        if(sentido == right){
            // Clockwise run: inner wall on the right, outer wall on the left.
            Spike_Turn_For_Degrees(der, 50, 80);
            Spike_Center_Vehicle_Short();
            Spike_Advance_For_Degrees(40, 1350, -90);
            angulo_correccion = Oradar_S2L_Wall_Slope(LEFT);
            Spike_Reset_Gyro(angulo_correccion);
            usleep(200000);
            // Remaining 11 corners.
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
            // Counter-clockwise run: inner wall on the left, outer wall on the right.
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

        printf("inner start case\n");

        sentido = Oradar_S2L_Advance_And_Detect_Side(30, 0);
        printf("direction: %d\n", sentido);

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
    // Back in the starting section after 12 corners: stop with the front wall
    // at 1500 mm, i.e. in the middle of the 3000 mm side.
    Oradar_S2L_Advance_Until_Distance(30, 0, 1500);

    // Previous ending (disabled): chose the stop distance from the start
    // position (distances measured before the first corner).

    /*if (sentido == right){
        if(distancia_izquierda < 400){
            Oradar_S2L_Advance_Until_Distance(80, 0, 450);
            printf("outer section\n");
        }   
        else if((distancia_izquierda > 400) && (distancia_izquierda < 600)){
            Oradar_S2L_Advance_Until_Distance(80, 0, 750);
            printf("middle\n");
        }  
        else if (distancia_izquierda > 600){
            Oradar_S2L_Advance_Until_Distance(80, 0, 1000);
        printf("inner section\n");
        }

        Spike_Turn_For_Degrees(der, 100, 70);
        Spike_Center_Vehicle_Short();
        Spike_Advance_For_Degrees(80, 300, -90);

        if(distancia_frente < 1500){
            Oradar_S2L_Advance_Until_Distance(80, -90, 1400);
            printf("front part\n");
        }
        else if(distancia_frente > 1500){
            Oradar_S2L_Advance_Until_Distance(80, -90, 1850);
        printf("back part\n");
        }
    }

    else if (sentido == left){
        if(distancia_derecha < 400){
            Oradar_S2L_Advance_Until_Distance(80, 0, 450);
            printf("outer section\n");
        }   
        else if((distancia_derecha > 400) && (distancia_derecha < 600)){
            Oradar_S2L_Advance_Until_Distance(80, 0, 750);
            printf("middle\n");
        }  
        else if (distancia_derecha > 600){
            Oradar_S2L_Advance_Until_Distance(80, 0, 1000);
            printf("inner section\n");
        }

        Spike_Turn_For_Degrees(izq, 100, 70);
        Spike_Center_Vehicle_Short();
        Spike_Advance_For_Degrees(80, 300, 90);

        if(distancia_frente < 1500){
            Oradar_S2L_Advance_Until_Distance(80, 90, 1400);
            printf("back part\n");
        }
        else if(distancia_frente > 1500){
            Oradar_S2L_Advance_Until_Distance(80, 90, 1850);
            printf("front part\n");
        }

    }*/

    printf("finished");
    
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

// Run thread state, used when the lidar window owns the main thread.
static volatile int recorrido_terminado = 0;   // 1 = run finished, close the window
static int recorrido_resultado = 0;            // return value of Recorrido()

// pthread wrapper around Recorrido().
static void *Recorrido_Thread(void *arg){
    (void)arg;
    recorrido_resultado = Recorrido();
    recorrido_terminado = 1;
    return NULL;
}

// Entry point. Options: --no-display (no lidar window), --help / -h.
// Without a window (or without DISPLAY) the run executes in this thread;
// otherwise it runs in a second thread while this one draws the window.
int main(int argc, char **argv){

    bool no_display = false;
    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "--no-display") == 0){
            no_display = true;
        } else if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0){
            Uso(argv[0]);
            return 0;
        } else {
            printf("Unknown argument: %s\n", argv[i]);
            Uso(argv[0]);
            return 1;
        }
    }

    if(no_display || !Lidar_Display_Available()){
        return Recorrido();
    }

    // The window only reads the lidar and the gyro: the run is the same with
    // or without it. It closes when the run ends.
    pthread_t recorrido;
    if(pthread_create(&recorrido, NULL, Recorrido_Thread, NULL) != 0){
        printf("Warning: could not create the run thread; continuing without the window.\n");
        return Recorrido();
    }
    Lidar_Display_Loop(&recorrido_terminado);
    pthread_join(recorrido, NULL);
    return recorrido_resultado;
}

// SIGINT handler used until the emergency stop is installed: tells the lidar
// thread and loops to finish, and restores the default action for a second Ctrl+C.
void signal_handler(int signum){
    printf("\nCtrl+C detected\n");
    Oradar_S2L_Set_Terminating();
    signal(SIGINT, SIG_DFL);
}
