#include "spike.h"

static int serial_port;

int Spike_Serial_Init(void){

    const char* port = "/dev/ttyACM0";
    struct termios tty;

    // SPIKE Prime needs several seconds to boot and enumerate after the GPIO
    // power-button pulse. The original single open() raced USB enumeration.
    serial_port = -1;
    printf("Waiting for SPIKE Prime USB serial port /dev/ttyACM0...\n");
    for (int attempt = 0; attempt < 120; ++attempt) {
        serial_port = open(port, O_RDWR | O_NOCTTY);
        if (serial_port >= 0) {
            break;
        }
        usleep(100000);
    }
    if (serial_port < 0) {
        printf("Error opening serial port /dev/ttyACM0 after 12 seconds\n");
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
    tty.c_cc[VTIME] = 1;              //timeout de 500 milisegundos
 
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error setting serial port attributes");
        close(serial_port);
        return 1;
    }
    tcflush(serial_port, TCIOFLUSH);  // ← add this: discard stale bytes

    printf("Serial port %d opened successfully with 115200 baud.\n", serial_port);

    return 0;
}

void Spike_Close_Serial(void){
    close(serial_port);
}

// Descarta cualquier byte pendiente de leer en el puerto serie. Pensado para
// llamarse justo despues de una fase de manejo continuo (Spike_Forward llamado
// en loop, p.ej. Oradar_S2L_Advance_Until_Distance) que manda comandos "da()"
// mas rapido de lo que el hub alcanza a ecoar: los ecos sobrantes se quedan
// en cola y, si nadie los descarta, el siguiente comando que sí espera una
// respuesta limpia (turn/ag/etc) tiene que drenarlos uno por uno antes de
// llegar al "255" real -- eso es lo que se veia como lineas "inesperadas"
// repetidas (da(80,0), Hold(), ...) en el log. Como esos ecos de da() nunca
// se leen ni se usan, es seguro tirarlos aqui en vez de dejar que se acumulen.
void Spike_Flush_Serial_Input(void){
    tcflush(serial_port, TCIFLUSH);
}

// El REPL de SPIKE Prime ecoa la linea de entrada (p.ej. "\n>>> turn_special(...)")
// antes de mandar el "255" real. Ese eco es normal y no indica ningun problema,
// asi que se filtra de los prints "linea inesperada" para no ensuciar el log.
bool Spike_Is_Repl_Echo(const char* line){
    return strstr(line, ">>>") != NULL;
}

void Spike_Send_Serial_Data(const char* data){
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
		if((buffer_read[i] == '\r') || (i >= (int)sizeof(buffer_read) - 1)){
			buffer_read[i] = '\0';  //special character to convert data to string
			break;
		}
		i++;
	}
    //printf("menssage send to spike: %s\n", buffer_read);
}

char* Spike_Read_Serial_Data(void){
	int i = 0;
	int num_bytes;
	static char buffer_read[255] = "";
	// Invalida el buffer al empezar cada llamada: si read() hace timeout (sin
	// bytes nuevos) antes de leer nada, buffer_read debe quedar vacio en vez
	// de regresar intacto el string de la llamada anterior. Sin esto, un
	// timeout justo despues de mandar un comando (turn/ag/etc, que tardan
	// mucho mas que el timeout de 100ms del puerto) hace que se regrese el
	// "255" que dejo el comando ANTERIOR -- el caller cree que el movimiento
	// ya termino cuando apenas empezo, y corta el giro/avance antes de tiempo.
	buffer_read[0] = '\0';
	while(1){
		num_bytes = read(serial_port,&buffer_read[i],1);
		if(num_bytes <= 0){
			//printf("Error reading %s\n", buffer_read);
			break;
		}
		// printf("%c\n",buffer_read[i]);
		if((buffer_read[i] == '\r') || (i >= (int)sizeof(buffer_read) - 1)){
			buffer_read[i] = '\0';
			break;
		}
		i++;
	}
	char *cp = buffer_read;
    //printf("data read from spike: %s\n",cp);
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

void Spike_End_Function(void){
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
    Spike_Send_Serial_Data("motor.stop(port.A, stop = motor.HOLD)\r");
    Spike_Send_Serial_Data("motor.stop(port.E, stop = motor.HOLD)\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def fc():\r"); // motores libres
    Spike_Send_Serial_Data("motor.stop(port.A, stop = motor.COAST)\r");
    Spike_Send_Serial_Data("motor.stop(port.E, stop = motor.COAST)\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def br():\r"); // motores break
    Spike_Send_Serial_Data("motor.stop(port.A, stop = motor.BRAKE)\r");
    Spike_Send_Serial_Data("motor.stop(port.E, stop = motor.BRAKE)\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("async def cv_especial():\r");
    Spike_Send_Serial_Data("await motor.run_to_absolute_position(port.A, 0, 550,\r");
    Spike_Send_Serial_Data("direction = motor.LONGEST_PATH, stop = motor.HOLD, acceleration = 1000, deceleration = 1000)\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def cv():\r"); // centrar vehiculo
    Spike_Send_Serial_Data("runloop.run(cv_especial())\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("async def cvc_especial():\r"); //cntrar vehiculo parte corta
    Spike_Send_Serial_Data("await motor.run_to_absolute_position(port.A, 0, 550,\r");
    Spike_Send_Serial_Data("direction = motor.SHORTEST_PATH, stop = motor.HOLD, acceleration = 1000, deceleration = 1000)\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def cvc():\r"); 
    Spike_Send_Serial_Data("runloop.run(cvc_especial())\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def pd(s1,s2,vel,kp,kd,ea):\r");
    Spike_Send_Serial_Data("error=s1-s2\r");
    Spike_Send_Serial_Data("et= (kp*error) + (kd*(error-ea))\r");
    Spike_Send_Serial_Data("motor.run_to_absolute_position(port.A, int(et*4.29), 600, direction = motor.SHORTEST_PATH, stop = motor.HOLD, acceleration = 10000)\r");
    Spike_Send_Serial_Data("motor.set_duty_cycle(port.E, (100)*(vel))\r");
    Spike_Send_Serial_Data("return error\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def rg(degrees):\r");
    Spike_Send_Serial_Data("motion_sensor.reset_yaw(degrees)\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def pg():\r");
    Spike_Send_Serial_Data("return motion_sensor.tilt_angles()[0]\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def turn(direction,speed,degrees, tire_turn):\r");
    Spike_Send_Serial_Data("motor.run_to_relative_position(port.A, int((tire_turn)*(4.29)*(direction)), 550)\r");
    Spike_Send_Serial_Data("while abs(degrees*10) > abs(motion_sensor.tilt_angles()[0]):\r");
    Spike_Send_Serial_Data("motor.set_duty_cycle(port.E, (100)*(speed))\r");
    Spike_Send_Serial_Data(remove);
    Spike_Send_Serial_Data("fc()\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("async def turn_wait(direction,speed,degrees, tire_turn):\r");
    Spike_Send_Serial_Data("await motor.run_to_relative_position(port.A, int((tire_turn)*(4.29)*(direction)), 550)\r");
    Spike_Send_Serial_Data("while abs(degrees*10) > abs(motion_sensor.tilt_angles()[0]):\r");
    Spike_Send_Serial_Data("motor.set_duty_cycle(port.E, (100)*(speed))\r");
    Spike_Send_Serial_Data(remove);
    Spike_Send_Serial_Data("fc()\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def turn_special(direction,speed,degrees, tire_turn):\r");
    Spike_Send_Serial_Data("runloop.run(turn_wait(direction,speed,degrees, tire_turn))\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def small_turn(direction,speed,degrees, tire_turn):\r");
    Spike_Send_Serial_Data("motor.run_to_relative_position(port.A, int((tire_turn)*(4.29)*(direction)), 550)\r");
    Spike_Send_Serial_Data("while abs(degrees*10) < (direction)*motion_sensor.tilt_angles()[0]:\r");
    Spike_Send_Serial_Data("motor.set_duty_cycle(port.E, (100)*(speed))\r");
    Spike_Send_Serial_Data(remove);
    Spike_Send_Serial_Data("fc()\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("async def turns_wait(direction,speed,degrees, tire_turn):\r");
    Spike_Send_Serial_Data("await motor.run_to_relative_position(port.A, int((tire_turn)*(4.29)*(direction)), 550)\r");
    Spike_Send_Serial_Data("while abs(degrees*10) < (direction)*motion_sensor.tilt_angles()[0]:\r");
    Spike_Send_Serial_Data("motor.set_duty_cycle(port.E, (100)*(speed))\r");
    Spike_Send_Serial_Data(remove);
    Spike_Send_Serial_Data("fc()\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def turns_special(direction,speed,degrees, tire_turn):\r");
    Spike_Send_Serial_Data("runloop.run(turns_wait(direction,speed,degrees, tire_turn))\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();

    //kp y kd para el forward
    Spike_Send_Serial_Data("def da(speed, reference):\r");
    Spike_Send_Serial_Data("global error\r");
    Spike_Send_Serial_Data("error = pd(motion_sensor.tilt_angles()[0],((10)*(reference)),speed,0.2,500000,error)\r");
    Spike_End_Function();

    //kp y kd para seguir los cubos por la camara
    Spike_Send_Serial_Data("def apd(speed, reference_1, reference_2):\r");
    Spike_Send_Serial_Data("global error\r"); /* para seguir pared 60 y kd 100 */
    Spike_Send_Serial_Data("error = pd(reference_1 ,reference_2,speed,150,100,error)\r");
    Spike_End_Function();

    //kp y kd para avanzar por grados
    Spike_Send_Serial_Data("def ag(speed,degrees,reference):\r");
    Spike_Send_Serial_Data("error = 0\r");
    Spike_Send_Serial_Data("motor.reset_relative_position(port.E,0)\r");
    Spike_Send_Serial_Data("while abs(degrees) > abs(motor.relative_position(port.E)):\r");
    Spike_Send_Serial_Data("error = pd(motion_sensor.tilt_angles()[0],((10)*(reference)),speed,0.2,300000,error)\r");
    Spike_Send_Serial_Data(remove);
    Spike_Send_Serial_Data("fc()\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def ag_u(speed,degrees,reference):\r");
    Spike_Send_Serial_Data("error = 0\r");
    Spike_Send_Serial_Data("motor.reset_relative_position(port.E,0)\r");
    Spike_Send_Serial_Data("while abs(degrees) > abs(motor.relative_position(port.E)):\r");
    Spike_Send_Serial_Data("error = pd(motion_sensor.tilt_angles()[0],((10)*(reference)),speed,-0.03,0,error)\r");
    Spike_Send_Serial_Data(remove);
    Spike_Send_Serial_Data("fc()\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();
}


void Spike_Center_Vehicle(void){
    Spike_Send_Serial_Data("cv()\r");
    const char * return_value = Spike_Read_Serial_Data();
    if (strcmp(return_value, "") == 0){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        usleep(1000);
        return_value = Spike_Read_Serial_Data();
        if((strcmp(return_value, "") != 0) && (atoi(return_value) != 255) && !Spike_Is_Repl_Echo(return_value)){
            printf("Spike wait (cv): linea inesperada '%s'\n", return_value);
        }
        if(strcmp(return_value, "") == 0){
            return_value = "0";
        }
    }
}

void Spike_Center_Vehicle_Short(void){
    Spike_Send_Serial_Data("cvc()\r");
    const char * return_value = Spike_Read_Serial_Data();
    if (strcmp(return_value, "") == 0){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        usleep(1000);
        return_value = Spike_Read_Serial_Data();
        if((strcmp(return_value, "") != 0) && (atoi(return_value) != 255) && !Spike_Is_Repl_Echo(return_value)){
            printf("Spike wait (cvc): linea inesperada '%s'\n", return_value);
        }
        if(strcmp(return_value, "") == 0){
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

void Spike_Break_Motors(void){
    Spike_Send_Serial_Data("br()\r");   
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

void Spike_Reset_Gyro(float degrees){
    //printf("concatenar \n");
    int degrees_to_int = (int)(degrees*10);
    char reset_degrees[255];
    char degrees_get_string[255]  = "";

    snprintf(degrees_get_string, sizeof(degrees_get_string), "%d", degrees_to_int);

    const char * list[5];
    list[0] = "rg(";
    list[1] = (const char *)degrees_get_string;
    list[2] = ")\r";
    Spike_Concatenate(3,list, reset_degrees);

    Spike_Send_Serial_Data(reset_degrees);

}

float Spike_Get_Gyro(void){
    Spike_Send_Serial_Data("pg()\r");
    char *return_value = Spike_Read_Serial_Data();
    printf("gyro: %s\n", return_value);
    return (atof(return_value))/10;
}

void Spike_Turn_For_Degrees(int direction, int speed, float degrees, int tire_turn, bool await){
	char arguments[255] = "";
	char string_direction[10] = "";
	char string_speed[10] = "";
	char string_degrees[10] = "";
    char string_tire_turn[10] = "";

	snprintf(string_direction, sizeof(string_direction), "%d", direction);
	snprintf(string_speed, sizeof(string_speed), "%d", speed);
	snprintf(string_degrees, sizeof(string_degrees), "%.1f", degrees);
    snprintf(string_tire_turn, sizeof(string_tire_turn), "%d", tire_turn);

	const char * cocatenate_list[10];
    if(await == false){
        cocatenate_list[0] = "turn(";
    }
    else{
        cocatenate_list[0] = "turn_special(";
    }
	cocatenate_list[1] = (const char *)string_direction;
	cocatenate_list[2] = ",";
	cocatenate_list[3] = (const char *)string_speed;	
	cocatenate_list[4] = ",";
	cocatenate_list[5] = (const char *)string_degrees;	
    cocatenate_list[6] = ",";
    cocatenate_list[7] = (const char *)string_tire_turn;
	cocatenate_list[8] = ")\r";
		
	Spike_Concatenate(9,cocatenate_list, arguments);

	Spike_Send_Serial_Data(arguments);

    const char * return_value = Spike_Read_Serial_Data();
    if (strcmp(return_value, "") == 0){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        usleep(1000);
        return_value = Spike_Read_Serial_Data();
        if((strcmp(return_value, "") != 0) && (atoi(return_value) != 255) && !Spike_Is_Repl_Echo(return_value)){
            printf("Spike wait (turn): linea inesperada '%s'\n", return_value);
        }
        if(strcmp(return_value, "") == 0){
            return_value = "0";
        }
    }

    Spike_Hold_Motors();
    //printf("direccion: %d, speed; %d, degrees: %.1f, tire_turn: %d, await: %d, gyro_fina: %f \n", direction, speed, degrees,tire_turn, await,Spike_Get_Gyro());
}

void Spike_Small_Turn(int direction, int speed, float degrees, int tire_turn,  bool await){
	char arguments[255];
	char string_direction[10] = "";
	char string_speed[10] = "";
	char string_degrees[10] = "";
    char string_tire_turn[10] = "";

	snprintf(string_direction, sizeof(string_direction), "%d", direction);
	snprintf(string_speed, sizeof(string_speed), "%d", speed);
	snprintf(string_degrees, sizeof(string_degrees), "%.1f", degrees);
    snprintf(string_tire_turn, sizeof(string_tire_turn), "%d", tire_turn);

	const char * cocatenate_list[10];
    if(await == false){
        cocatenate_list[0] = "small_turn(";
    }
    else{
        cocatenate_list[0] = "turns_special(";
    }
	cocatenate_list[1] = (const char *)string_direction;
	cocatenate_list[2] = ",";
	cocatenate_list[3] = (const char *)string_speed;	
	cocatenate_list[4] = ",";
	cocatenate_list[5] = (const char *)string_degrees;	
    cocatenate_list[6] = ",";
    cocatenate_list[7] = (const char *)string_tire_turn;
	cocatenate_list[8] = ")\r";
		
	Spike_Concatenate(9,cocatenate_list, arguments);

	Spike_Send_Serial_Data(arguments);

    const char * return_value = Spike_Read_Serial_Data();
    if (strcmp(return_value, "") == 0){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        usleep(1000);
        return_value = Spike_Read_Serial_Data();
        if((strcmp(return_value, "") != 0) && (atoi(return_value) != 255) && !Spike_Is_Repl_Echo(return_value)){
            printf("Spike wait (small_turn): linea inesperada '%s'\n", return_value);
        }
        if(strcmp(return_value, "") == 0){
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
    if(speed > 0){
	    cocatenate_list[0] = "ag(";
    }else{
        cocatenate_list[0] = "ag_u(";
    }
	cocatenate_list[1] = (const char *)string_speed;
	cocatenate_list[2] = ",";
	cocatenate_list[3] = (const char *)string_degrees;
    cocatenate_list[4] = ",";
    cocatenate_list[5] = (const char *)string_reference;
	cocatenate_list[6] = ")\r";

	Spike_Concatenate(7,cocatenate_list, arguments);

	Spike_Send_Serial_Data(arguments);

    const char * return_value = Spike_Read_Serial_Data();
    if (strcmp(return_value, "") == 0){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        usleep(1000);
        return_value = Spike_Read_Serial_Data();
        if((strcmp(return_value, "") != 0) && (atoi(return_value) != 255) && !Spike_Is_Repl_Echo(return_value)){
            printf("Spike wait (ag/degrees): linea inesperada '%s'\n", return_value);
        }
        usleep(1000);
        if(strcmp(return_value, "") == 0){
            return_value = "0";
        }
    }
    Spike_Break_Motors();
}

void Spike_Advance_For_distance(int speed, int distance, int reference){
    int degrees = (int)((1.4)*((distance*360)/(196.035)));
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

    const char * return_value = Spike_Read_Serial_Data();
    if (strcmp(return_value, "") == 0){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        usleep(1000);
        return_value = Spike_Read_Serial_Data();
        if((strcmp(return_value, "") != 0) && (atoi(return_value) != 255) && !Spike_Is_Repl_Echo(return_value)){
            printf("Spike wait (ag/distance): linea inesperada '%s'\n", return_value);
        }
        usleep(1000);
        if(strcmp(return_value, "") == 0){
            return_value = "0";
        }
    }
    Spike_Break_Motors();
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

void Spike_Follow_Reference(int speed, float reference_1, float reference_2){
    char arguments[255];
	char string_speed[10] = "";
    char string_reference_1[10] = "";
    char string_reference_2[10] = "";

    snprintf(string_speed, sizeof(string_speed), "%d", speed);
	snprintf(string_reference_1, sizeof(string_reference_1), "%.3f", reference_1);
	snprintf(string_reference_2, sizeof(string_reference_2), "%.3f", reference_2);

    const char * cocatenate_list[10];
	cocatenate_list[0] = "apd(";
	cocatenate_list[1] = (const char *)string_speed;	
	cocatenate_list[2] = ",";
    cocatenate_list[3] = (const char *)string_reference_1;	
	cocatenate_list[4] = ",";
    cocatenate_list[5] = (const char *)string_reference_2;	
	cocatenate_list[6] = ")\r";

    Spike_Concatenate(7, cocatenate_list, arguments);

    Spike_Send_Serial_Data(arguments);

}
