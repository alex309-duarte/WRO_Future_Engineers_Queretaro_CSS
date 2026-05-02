#include "spike.h"

static int serial_port;

int Spike_Serial_Init(void){

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

void Spike_Close_Serial(void){
    close(serial_port);
}

void Spike_Send_Serial_Data(char data[]){
    int num_bytes;

	char buffer_read[255] = "";
	const char * buffer_Spike = data;
    //printf("Data %s\n",buffer_Spike);
    int i = 0;
    write(serial_port,buffer_Spike,strlen(buffer_Spike));

	while(1){
        num_bytes = read(serial_port,&buffer_read[i],1);
        if(num_bytes <= 0){
			//printf("Error reading\n");
			break;
		}
		if((buffer_read[i] == '\r') || (i > 255)){
			buffer_read[i] = '\0';  //special character to convert data to string
			break;
		}
		i++;
	}
    //printf("menssage: %s\n", buffer_read);
}

char* Spike_Read_Serial_Data(void){
	int i = 0;
	int num_bytes;
	static char buffer_read[255] = "";
	while(1){
		num_bytes = read(serial_port,&buffer_read[i],1);
		if(num_bytes <= 0){
			//printf("Error reading %s\n", buffer_read);
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

void Spike_Interpreter(void){
    char control_c = '\003';
    char msg[10] = "";
    msg[0] = control_c;
    msg[1] = '\r';
    Spike_Send_Serial_Data(msg);
    Spike_Read_Serial_Data();
    Spike_Read_Serial_Data();
    Spike_Read_Serial_Data();
    Spike_Send_Serial_Data("\r");
}

void Spike_End_Funcion(void){
    Spike_Send_Serial_Data("\r");
    Spike_Send_Serial_Data("\r");
    Spike_Send_Serial_Data("\r"); 
}

void Spike_Initialize_Libraries(void){
    char command = char(127);
    char remove[10] = "";
    remove[0] = command;
    remove[1] = '\r';


    Spike_Send_Serial_Data("import motor\r");
    Spike_Send_Serial_Data("from hub import port\r");
    Spike_Send_Serial_Data("from hub import motion_sensor\r");
    Spike_Send_Serial_Data("import distance_sensor\r");
    Spike_Send_Serial_Data("import runloop\r");
    //variables del spike
    Spike_Send_Serial_Data("der = -1\r");
    Spike_Send_Serial_Data("izq = 1\r");
    Spike_Send_Serial_Data("error = 0\r");
    //Funciones
    Spike_Send_Serial_Data("def Hold():\r"); // motores en hold
    Spike_Send_Serial_Data("motor.stop(port.F, stop = motor.HOLD)\r");
    Spike_Send_Serial_Data("motor.stop(port.B, stop = motor.HOLD)\r");
    Spike_End_Funcion();

    Spike_Send_Serial_Data("def fc():\r"); // motores libres
    Spike_Send_Serial_Data("motor.stop(port.F, stop = motor.COAST)\r");
    Spike_Send_Serial_Data("motor.stop(port.B, stop = motor.COAST)\r");
    Spike_End_Funcion();

    Spike_Send_Serial_Data("async def cv_especial():\r");
    Spike_Send_Serial_Data("await motor.run_to_absolute_position(port.F, 0, 550,\r");
    Spike_Send_Serial_Data("direction = motor.LONGEST_PATH, stop = motor.HOLD, acceleration = 1000, deceleration = 1000)\r");
    Spike_End_Funcion();

    Spike_Send_Serial_Data("def cv():\r"); // centrar vehiculo
    Spike_Send_Serial_Data("runloop.run(cv_especial())\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Funcion();

    Spike_Send_Serial_Data("async def cvc_especial():\r"); //cntrar vehiculo parte corta
    Spike_Send_Serial_Data("await motor.run_to_absolute_position(port.F, 0, 550,\r");
    Spike_Send_Serial_Data("direction = motor.SHORTEST_PATH, stop = motor.HOLD, acceleration = 1000, deceleration = 1000)\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Funcion();

    Spike_Send_Serial_Data("def cvc():\r"); 
    Spike_Send_Serial_Data("runloop.run(cvc_especial())\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Funcion();

    Spike_Send_Serial_Data("def pd(s1,s2,vel,kp,kd,ea):\r");
    Spike_Send_Serial_Data("error=s1-s2\r");
    Spike_Send_Serial_Data("et= (kp*error) + (kd*(error-ea))\r");
    Spike_Send_Serial_Data("motor.run_to_absolute_position(port.F, int(et*6.4), 600, direction = motor.SHORTEST_PATH, stop = motor.HOLD, acceleration = 10000)\r");
    Spike_Send_Serial_Data("motor.set_duty_cycle(port.B, (100)*(vel))\r");
    Spike_Send_Serial_Data("return error\r");
    Spike_End_Funcion();

    Spike_Send_Serial_Data("def rg(degrees):\r");
    Spike_Send_Serial_Data("motion_sensor.reset_yaw(degrees)\r");
    Spike_End_Funcion();

    Spike_Send_Serial_Data("def pg():\r");
    Spike_Send_Serial_Data("return motion_sensor.tilt_angles()[0]\r");
    Spike_End_Funcion();

    Spike_Send_Serial_Data("def turn(direction,speed,degrees):\r");
    Spike_Send_Serial_Data("motor.run_to_relative_position(port.F, 115*(direction), 550)\r");
    Spike_Send_Serial_Data("while abs(degrees*10) > abs(motion_sensor.tilt_angles()[0]):\r");
    Spike_Send_Serial_Data("motor.set_duty_cycle(port.B, (100)*(speed))\r");
    Spike_Send_Serial_Data(remove);
    Spike_Send_Serial_Data("fc()\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Funcion();

    Spike_Send_Serial_Data("def da(speed, reference):\r");
    Spike_Send_Serial_Data("global error\r");
    Spike_Send_Serial_Data("error = pd(motion_sensor.tilt_angles()[0],((10)*(reference)),speed,0.11,0.7,error)\r");
    Spike_End_Funcion();

    Spike_Send_Serial_Data("def ag(speed,degrees,reference):\r");
    Spike_Send_Serial_Data("error = 0\r");
    Spike_Send_Serial_Data("motor.reset_relative_position(port.B,0)\r");
    Spike_Send_Serial_Data("while abs(degrees) > abs(motor.relative_position(port.B)):\r");
    Spike_Send_Serial_Data("error = pd(motion_sensor.tilt_angles()[0],((10)*(reference)),speed,0.1,0.5,error)\r");
    Spike_Send_Serial_Data(remove);
    Spike_Send_Serial_Data("fc()\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Funcion();
}


void Spike_Center_Vehicle(void){
    Spike_Send_Serial_Data("cv()\r");
    char * return_value = Spike_Read_Serial_Data();
    if (return_value == ""){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        return_value = Spike_Read_Serial_Data();
        if(return_value == ""){
            return_value = "0";
        }
    }
}

void Spike_Center_Vehicle_Short(void){
    Spike_Send_Serial_Data("cvc()\r");
    char * return_value = Spike_Read_Serial_Data();
    if (return_value == ""){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        return_value = Spike_Read_Serial_Data();
        if(return_value == ""){
            return_value = "0";
        }
    }
}

void Spike_Coast_Motors(void){
    Spike_Send_Serial_Data("fc()\r");
}

void Spike_Hold_Motors(void){
    Spike_Send_Serial_Data("Hold()\r");   
}

void Spike_Concatenate(int list_lenght,const char * argument_1[],char * buffer){
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
    //printf("%s\n",buffer);
}

void Spike_Reset_Gyro(int degrees){
    //printf("concatenar \n");
    int i = 0;
    int a = 0;
    char reset_degrees[255];
    char degrees_get_string[10]  = "";
    char principle[10] = "";
    principle[0] = 'r';
    principle[1] = 'g';
    principle[2] = '(';
    principle[3] = '\r';
    snprintf(degrees_get_string, sizeof(degrees_get_string), "%d", degrees);

    const char * list[5];
    list[0] = "rg(";
    list[1] = (const char *)degrees_get_string;
    list[2] = ")\r";
    Spike_Concatenate(3,list, reset_degrees);

    Spike_Send_Serial_Data(reset_degrees);

}

void Spike_Print_Gyro(void){
    Spike_Send_Serial_Data("pg()\r");
    char *return_value = Spike_Read_Serial_Data();
    printf("gyro: %s\n", return_value);
}

void Spike_Turn_For_Degrees(int direction, int speed, int degrees){
	char arguments[255];
	char string_direction[10] = "";
	char string_speed[10] = "";
	char string_degrees[10] = "";

	snprintf(string_direction, sizeof(string_direction), "%d", direction);
	snprintf(string_speed, sizeof(string_speed), "%d", speed);
	snprintf(string_degrees, sizeof(string_degrees), "%d", degrees);

	const char * cocatenate_list[10];
	cocatenate_list[0] = "turn(";
	cocatenate_list[1] = (const char *)string_direction;
	cocatenate_list[2] = ",";
	cocatenate_list[3] = (const char *)string_speed;	
	cocatenate_list[4] = ",";
	cocatenate_list[5] = (const char *)string_degrees;	
	cocatenate_list[6] = ")\r";
		
	Spike_Concatenate(7,cocatenate_list, arguments);

	Spike_Send_Serial_Data(arguments);

    char * return_value = Spike_Read_Serial_Data();
    if (return_value == ""){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        return_value = Spike_Read_Serial_Data();
        if(return_value == ""){
            return_value = "0";
        }
    }

    Spike_Hold_Motors();
}

void Spike_Advance_For_Degrees(int speed, int degrees, int reference){
	char arguments[255];
	char string_speed[10] = "";
	char string_degrees[10] = "";
    char string_reference[10] = "";

	snprintf(string_speed, sizeof(string_speed), "%d", speed);
	snprintf(string_degrees, sizeof(string_degrees), "%d", degrees);
	snprintf(string_reference, sizeof(string_reference), "%d", reference);

	const char * cocatenate_list[10];
	cocatenate_list[0] = "ag(";
	cocatenate_list[1] = (const char *)string_speed;	
	cocatenate_list[2] = ",";
	cocatenate_list[3] = (const char *)string_degrees;
    cocatenate_list[4] = ",";
    cocatenate_list[5] = (const char *)string_reference;	
	cocatenate_list[6] = ")\r";
		
	Spike_Concatenate(7,cocatenate_list, arguments);

	Spike_Send_Serial_Data(arguments);

    char * return_value = Spike_Read_Serial_Data();
    if (return_value == ""){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        return_value = Spike_Read_Serial_Data();
        if(return_value == ""){
            return_value = "0";
        }
    }
    Spike_Coast_Motors();
}

void Spike_Forward(int speed, int reference){
    char arguments[255];
	char string_speed[10] = "";
    char string_reference[10] = "";

    snprintf(string_speed, sizeof(string_speed), "%d", speed);
	snprintf(string_reference, sizeof(string_reference), "%d", reference);

    const char * cocatenate_list[10];
	cocatenate_list[0] = "da(";
	cocatenate_list[1] = (const char *)string_speed;	
	cocatenate_list[2] = ",";
    cocatenate_list[3] = (const char *)string_reference;	
	cocatenate_list[4] = ")\r";

    Spike_Concatenate(5, cocatenate_list, arguments);

    Spike_Send_Serial_Data(arguments);

}