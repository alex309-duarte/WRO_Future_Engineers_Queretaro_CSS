#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdarg.h>

#include "sl_lidar.h" 
#include "sl_lidar_driver.h"
#ifndef _countof
#define _countof(_Array) (int)(sizeof(_Array) / sizeof(_Array[0]))
#endif

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>
static inline void delay(sl_word_size_t ms){
    while (ms>=1000){
        usleep(1000*1000);
        ms-=1000;
    };
    if (ms!=0)
        usleep(ms*1000);
}
#endif

int serial_init(void);
void send_serial_data(char data[]);
char* read_data(void);
void interpreter(void);
void initialize_Libraries(void);
void end_funcion(void);
void centrar_vehiculo(void);
void centrar_vehiculo_corto(void);
void Coast_motors(void);
void Hold_motors(void);
void reset_gyro(int grados);
void print_gyro(void);
void concatenar(int list_lenght,const char * argument_1[],char * buffer);

static int serial_port;

int main(){
    serial_init();
    interpreter();
    initialize_Libraries();
    reset_gyro(-10);
    print_gyro();

    return 0;
}

int serial_init(void){

    const char* port = "/dev/serial0";
    struct termios tty;

    serial_port = open("/dev/ttyACM0", O_RDWR | O_NOCTTY);
    if (serial_port < 0) {
        printf("Error opening serial port\n");
        return 1;
    }

    if (tcgetattr(serial_port, &tty) != 0) {
        printf("Error getting serial port attributes\n");
        close(serial_port);
        return 1;
    }

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    
	tty.c_cflag |= (CLOCAL | CREAD);    // Disable modem control lines, enable receiver
	tty.c_cflag &= ~PARENB;             // No parity
	tty.c_cflag &= ~CSTOPB;             // 1 stop bit
	tty.c_cflag &= ~CSIZE;              // Clear data size bits
	tty.c_cflag |= CS8;                // 8 data bits
	tty.c_lflag = 0;                   // Raw mode
	tty.c_oflag = 0;
	tty.c_iflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;              //timeout de 500 milisegundos
 
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error setting serial port attributes");
        close(serial_port);
        return 1;
    }

    printf("Serial port %d opened successfully with 115200 baud.\n", serial_port);

    return 0;
}


void send_serial_data(char data[]){
    int num_bytes;

	char buffer_read[255] = "";
	const char * buffer_Spike = data;
    printf("Data %s\n",buffer_Spike);
    int i = 0;
    write(serial_port,buffer_Spike,strlen(buffer_Spike));

	while(1){
        num_bytes = read(serial_port,&buffer_read[i],1);
        if(num_bytes <= 0){
			printf("Error reading\n");
			break;
		}
		if((buffer_read[i] == '\r') || (i > 255)){
			buffer_read[i] = '\0';  //special character to convert data to string
			break;
		}
		i++;
	}
     printf("menssage: %s\n", buffer_read);
}

char* read_data(void){
	int i = 0;
	int num_bytes;
	static char buffer_read[255] = "";
	while(1){
		num_bytes = read(serial_port,&buffer_read[i],1);
		if(num_bytes <= 0){
			printf("Error reading %s\n", buffer_read);
			break;
		}
		// printf("%c\n",buffer_read[i]);
		if((buffer_read[i] == '\r') || (i > 255)){
			buffer_read[i] = '\0';
			break;
		}
		i++;
	}
	char *cp = buffer_read;
    // printf("%s",cp);
	return cp;
}

void interpreter(void){
    char control_c = '\003';
    char msg[10] = "";
    msg[0] = control_c;
    msg[1] = '\r';
    send_serial_data(msg);
    read_data();
    read_data();
    read_data();
    send_serial_data("\r");
}

void end_funcion(void){
    send_serial_data("\r");
    send_serial_data("\r");
    send_serial_data("\r"); 
}

void initialize_Libraries(void){
    char command = char(127);
    char remove[10] = "";
    remove[0] = command;
    remove[1] = '\r';


    send_serial_data("import motor\r");
    send_serial_data("from hub import port\r");
    send_serial_data("from hub import motion_sensor\r");
    send_serial_data("import distance_sensor\r");
    send_serial_data("import runloop\r");
    //variables del spike
    send_serial_data("der = -1\r");
    send_serial_data("izq = 1\r");
    send_serial_data("error = 0\r");
    //Funciones
    send_serial_data("def Hold():\r"); // motores en hold
    send_serial_data("motor.stop(port.F, stop = motor.HOLD)\r");
    send_serial_data("motor.stop(port.B, stop = motor.HOLD)\r");
    end_funcion();

    send_serial_data("def fc():\r"); // motores libres
    send_serial_data("motor.stop(port.F, stop = motor.COAST)\r");
    send_serial_data("motor.stop(port.B, stop = motor.COAST)\r");
    end_funcion();

    send_serial_data("async def cv_especial():\r");
    send_serial_data("await motor.run_to_absolute_position(port.F, 0, 550,\r");
    send_serial_data("direction = motor.LONGEST_PATH, stop = motor.HOLD, acceleration = 1000, deceleration = 1000)\r");
    end_funcion();

    send_serial_data("def cv():\r"); // centrar vehiculo
    send_serial_data("runloop.run(cv_especial())\r");
    send_serial_data("return 255\r");
    end_funcion();

    send_serial_data("def cvc():\r"); //cntrar vehiculo parte corta
    send_serial_data("motor.run_to_absolute_position(port.F, 0, 550,\r");
    send_serial_data("direction = motor.SHORTEST_PATH, stop = motor.HOLD, acceleration = 1000, deceleration = 1000)\r");
    send_serial_data("return 255\r");
    end_funcion();

    send_serial_data("def pd(s1,s2,vel,kp,kd,ea):\r");
    send_serial_data("error=s1-s2\r");
    send_serial_data("et= (kp*error) + (kd*(error-ea))\r");
    send_serial_data("motor.run_to_absolute_position(port.F, int(et*6.4), 600, direction = motor.SHORTEST_PATH, stop = motor.HOLD, acceleration = 10000)\r");
    send_serial_data("motor.set_duty_cycle(port.B, (100)*(vel))\r");
    send_serial_data("return error\r");
    end_funcion();

    send_serial_data("def rg(grados):\r");
    send_serial_data("motion_sensor.reset_yaw(grados)\r");
    end_funcion();

    send_serial_data("def pg():\r");
    send_serial_data("return motion_sensor.tilt_angles()[0]\r");
    end_funcion();

    send_serial_data("def ad(vel,referencia):\r");
    send_serial_data("global error\r");
    send_serial_data("error = pd(((10)*(referencia)),motion_sensor.tilt_angles()[0],vel,0.3,1,error)\r");
    end_funcion();

    send_serial_data("def vuelta(direccion,velocidad,grados):\r");
    send_serial_data("motor.run_to_relative_position(port.F, 313*(direccion), 550)\r");
    send_serial_data("while abs(grados*10) > abs(motion_sensor.tilt_angles()[0]):\r");
    send_serial_data("motor.set_duty_cycle(port.B, (100)*(velocidad))\r");
    send_serial_data(remove);
    send_serial_data("fc()\r");
    send_serial_data("return 255\r");
    end_funcion();

    send_serial_data("def da(vel, referencia):\r");
    send_serial_data("global error\r");
    send_serial_data("error = pd(motion_sensor.tilt_angles()[0],referencia,vel,0.3,1,error)\r");
    end_funcion();

    send_serial_data("def ag(vel,grados,referencia):\r");
    send_serial_data("error = 0\r");
    send_serial_data("motor.reset_relative_position(port.B,0)\r");
    send_serial_data("while abs(grados) > abs(motor.relative_position(port.B)):\r");
    send_serial_data("error = pd(motion_sensor.tilt_angles()[0],((10)*(referencia)),vel,0.3,10,error)\r");
    send_serial_data(remove);
    send_serial_data("fc()\r");
    send_serial_data("return 255\r");
    end_funcion();
    end_funcion();
}


void centrar_vehiculo(void){
    send_serial_data("cv()\r");
    char * return_value = read_data();
    if (return_value == ""){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        return_value = read_data();
        if(return_value == ""){
            return_value = "0";
        }
    }
}

void centrar_vehiculo_corto(void){
    send_serial_data("cvc()\r");
    char * return_value = read_data();
    if (return_value == ""){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        return_value = read_data();
        if(return_value == ""){
            return_value = "0";
        }
    }
}

void Coast_motors(void){
    send_serial_data("fc()\r");
}

void Hold_motors(void){
    send_serial_data("fc()\r");   
}

void concatenar(int list_lenght,const char * argument_1[],char * buffer){
    int i = 0;
    int k = 0;
    const char * current_string;

    for(int j = 0; j< list_lenght; j++){
        current_string = argument_1[j];
        k = 0;
        while(1){
            buffer[i] = current_string[k];
            if(current_string[k] == '\0'){
                break;
            }
            k++;
            i++;
        }
    }
    printf("%s\n",buffer);
}

void reset_gyro(int grados){
    printf("concatenar \n");
    int i = 0;
    int a = 0;
    char grados_a_recetear[255];
    char string_grados[10]  = "";
    char principio[10] = "";
    principio[0] = 'r';
    principio[1] = 'g';
    principio[2] = '(';
    principio[3] = '\r';
    snprintf(string_grados, sizeof(string_grados), "%d", grados);

    //const char *grados_string = (const char*)string_grados;
    const char * list[5];
    list[0] = "rg(";
    list[1] = (const char *)string_grados;
    list[2] = ")\r";
    concatenar(3,list, grados_a_recetear);
    /*while (1){
        grados_a_recetear[i] = principio[i];
        if (principio[i] == '\r'){
            while (1){
                grados_a_recetear[i] = string_grados[a];
                if (string_grados[a] == '\0'){
                    break;
                }
                a++;
                i++;
            }
            break;
        }
        i++;
    }
    grados_a_recetear[i] = ')';
    grados_a_recetear[i+1] = '\r';
    printf("grados a resetear: %s\n", grados_a_recetear);*/
    send_serial_data(grados_a_recetear);

}

void print_gyro(void){
    send_serial_data("pg()\r");
    char *return_value = read_data();
    printf("gyro: %s\n", return_value);
}

