// Original LEGO SPIKE Prime hub driver of design v6 (kept for reference; not
// compiled in design v7, where spike_compat + the Hiwonder board replace it).
// It talks to the hub's MicroPython REPL over USB serial (/dev/ttyACM0,
// 115200 baud): Spike_Initialize_Libraries() types helper functions into the
// REPL, and each API call sends a one-line Python call and, for blocking
// moves, waits for the hub to print 255. Hub ports: A = steering motor,
// E = drive motor. Used by the Open and Obstacle Challenge code of v6.

#include "spike.h"

static int serial_port;   // fd of the hub serial port

// Opens /dev/ttyACM0 at 115200 8N1, raw mode, 0.5 s read timeout.
// Returns 0 on success, 1 on error.
int Spike_Serial_Init(void){

    const char* port = "/dev/ttyACM0";
    struct termios tty;

    serial_port = open(port, O_RDWR | O_NOCTTY);
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
    tty.c_cc[VTIME] = 5;              //timeout of 500 milliseconds
 
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

// Sends a string to the REPL, then reads and discards the echoed line (up to
// '\r', 254 chars or the read timeout).
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
    //printf("menssage: %s\n", buffer_read);
}

// Reads one line from the hub (until '\r', 254 chars or timeout). Returns a
// pointer to a static buffer, overwritten by the next call.
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
		if((buffer_read[i] == '\r') || (i >= (int)sizeof(buffer_read) - 1)){
			buffer_read[i] = '\0';
			break;
		}
		i++;
	}
	char *cp = buffer_read;
    // printf("%s",cp);
	return cp;
}

// Sends Ctrl+C to interrupt any running program and get the REPL prompt,
// then discards the banner lines.
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

// Three empty lines end an indented block (def / while) in the REPL.
void Spike_End_Function(void){
    Spike_Send_Serial_Data("\r");
    Spike_Send_Serial_Data("\r");
    Spike_Send_Serial_Data("\r"); 
}

// Defines the MicroPython helper functions on the hub. Each Python line is
// sent separately; the REPL auto-indents, so `remove` (ASCII 127 + CR) is sent to
// step out of a while body. Every blocking helper prints/returns 255 when it
// finishes, which the C side waits for.
void Spike_Initialize_Libraries(void){
    char command = char(127);   // ASCII 127 (delete): removes one auto-indent level
    char remove[10] = "";
    remove[0] = command;
    remove[1] = '\r';


    Spike_Send_Serial_Data("import motor\r");
    Spike_Send_Serial_Data("from hub import port\r");
    Spike_Send_Serial_Data("from hub import motion_sensor\r");
    Spike_Send_Serial_Data("import distance_sensor\r");
    Spike_Send_Serial_Data("import runloop\r");
    //hub-side variables: steering signs and the PD previous error
    Spike_Send_Serial_Data("der = -1\r");
    Spike_Send_Serial_Data("izq = 1\r");
    Spike_Send_Serial_Data("error = 0\r");
    //Functions
    Spike_Send_Serial_Data("def Hold():\r"); // motors in hold
    Spike_Send_Serial_Data("motor.stop(port.A, stop = motor.HOLD)\r");
    Spike_Send_Serial_Data("motor.stop(port.E, stop = motor.HOLD)\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def fc():\r"); // motors coast (free)
    Spike_Send_Serial_Data("motor.stop(port.A, stop = motor.COAST)\r");
    Spike_Send_Serial_Data("motor.stop(port.E, stop = motor.COAST)\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def br():\r"); // motors brake
    Spike_Send_Serial_Data("motor.stop(port.A, stop = motor.BRAKE)\r");
    Spike_Send_Serial_Data("motor.stop(port.E, stop = motor.BRAKE)\r");
    Spike_End_Function();

    // cv(): centre the steering (port A to absolute 0), longest path.
    Spike_Send_Serial_Data("async def cv_especial():\r");
    Spike_Send_Serial_Data("await motor.run_to_absolute_position(port.A, 0, 550,\r");
    Spike_Send_Serial_Data("direction = motor.LONGEST_PATH, stop = motor.HOLD, acceleration = 1000, deceleration = 1000)\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def cv():\r"); // centre vehicle
    Spike_Send_Serial_Data("runloop.run(cv_especial())\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("async def cvc_especial():\r"); //centre vehicle, shortest path
    Spike_Send_Serial_Data("await motor.run_to_absolute_position(port.A, 0, 550,\r");
    Spike_Send_Serial_Data("direction = motor.SHORTEST_PATH, stop = motor.HOLD, acceleration = 1000, deceleration = 1000)\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();

    // cvca(): centre while driving at 70 % duty; cvc(): centre only.
    Spike_Send_Serial_Data("def cvca():\r"); 
    Spike_Send_Serial_Data("motor.set_duty_cycle(port.E, (100)*(70))\r");
    Spike_Send_Serial_Data("runloop.run(cvc_especial())\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def cvc():\r"); 
    Spike_Send_Serial_Data("runloop.run(cvc_especial())\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();

    // pd(): one PD step. error = s1 - s2, output et -> steering position
    // (x4.29 motor degrees), drive duty = 100*vel. Returns the error so the
    // caller can pass it back as ea (previous error) for the D term.
    Spike_Send_Serial_Data("def pd(s1,s2,vel,kp,kd,ea):\r");
    Spike_Send_Serial_Data("error=s1-s2\r");
    Spike_Send_Serial_Data("et= (kp*error) + (kd*(error-ea))\r");
    Spike_Send_Serial_Data("motor.run_to_absolute_position(port.A, int(et*4.29), 600, direction = motor.SHORTEST_PATH, stop = motor.HOLD, acceleration = 10000)\r");
    Spike_Send_Serial_Data("motor.set_duty_cycle(port.E, (100)*(vel))\r");
    Spike_Send_Serial_Data("return error\r");
    Spike_End_Function();

    // rg(): reset yaw; pg(): return yaw (decidegrees).
    Spike_Send_Serial_Data("def rg(degrees):\r");
    Spike_Send_Serial_Data("motion_sensor.reset_yaw(degrees)\r");
    Spike_End_Function();

    Spike_Send_Serial_Data("def pg():\r");
    Spike_Send_Serial_Data("return motion_sensor.tilt_angles()[0]\r");
    Spike_End_Function();

    // turn(): full-lock steering (179 deg relative), drive until
    // |yaw| >= degrees (yaw in decidegrees, hence degrees*10), then coast.
    Spike_Send_Serial_Data("def turn(direction,speed,degrees):\r");
    Spike_Send_Serial_Data("motor.run_to_relative_position(port.A, 179*(direction), 550)\r");
    Spike_Send_Serial_Data("while abs(degrees*10) > abs(motion_sensor.tilt_angles()[0]):\r");
    Spike_Send_Serial_Data("motor.set_duty_cycle(port.E, (100)*(speed))\r");
    Spike_Send_Serial_Data(remove);
    Spike_Send_Serial_Data("fc()\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();

    // da(): one non-blocking heading-hold PD step toward `reference` (deg).
    Spike_Send_Serial_Data("def da(speed, reference):\r");
    Spike_Send_Serial_Data("global error\r");
    Spike_Send_Serial_Data("error = pd(motion_sensor.tilt_angles()[0],((10)*(reference)),speed,0.2,500000,error)\r");
    Spike_End_Function();

    // ag(): advance |degrees| of drive motor rotation holding heading
    // `reference` with the PD, then coast.
    Spike_Send_Serial_Data("def ag(speed,degrees,reference):\r");
    Spike_Send_Serial_Data("error = 0\r");
    Spike_Send_Serial_Data("motor.reset_relative_position(port.E,0)\r");
    Spike_Send_Serial_Data("while abs(degrees) > abs(motor.relative_position(port.E)):\r");
    Spike_Send_Serial_Data("error = pd(motion_sensor.tilt_angles()[0],((10)*(reference)),speed,0.2,300000,error)\r");
    Spike_Send_Serial_Data(remove);
    Spike_Send_Serial_Data("fc()\r");
    Spike_Send_Serial_Data("return 255\r");
    Spike_End_Function();
}


// Blocking: calls cv() and waits until the hub prints 255.
void Spike_Center_Vehicle(void){
    Spike_Send_Serial_Data("cv()\r");
    const char * return_value = Spike_Read_Serial_Data();
    if (strcmp(return_value, "") == 0){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        return_value = Spike_Read_Serial_Data();
        if(strcmp(return_value, "") == 0){
            return_value = "0";
        }
        usleep(1000);
    }
}

// Blocking: centres by the shortest path; advance = true also drives at 70 %.
void Spike_Center_Vehicle_Short(bool advance){
    if( advance == false){
        Spike_Send_Serial_Data("cvc()\r");
    }
    else{
        Spike_Send_Serial_Data("cvca()\r");
    }
    const char * return_value = Spike_Read_Serial_Data();
    if (strcmp(return_value, "") == 0){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        return_value = Spike_Read_Serial_Data();
        if(strcmp(return_value, "") == 0){
            return_value = "0";
        }
        usleep(1000);
    }
}

// Non-blocking motor stop commands: coast, hold, brake.
void Spike_Coast_Motors(void){
    Spike_Send_Serial_Data("fc()\r");
}

void Spike_Hold_Motors(void){
    Spike_Send_Serial_Data("Hold()\r");   
}

void Spike_Break_Motors(void){
    Spike_Send_Serial_Data("br()\r");   
}


// Concatenates list_lenght C strings into buffer (including the final '\0').
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

// Sends rg(degrees): resets the hub yaw to `degrees` (decidegrees).
void Spike_Reset_Gyro(int degrees){
    //printf("concatenar \n");
    char reset_degrees[255];
    char degrees_get_string[10]  = "";
    
    snprintf(degrees_get_string, sizeof(degrees_get_string), "%d", degrees);

    const char * list[5];
    list[0] = "rg(";
    list[1] = (const char *)degrees_get_string;
    list[2] = ")\r";
    Spike_Concatenate(3,list, reset_degrees);

    Spike_Send_Serial_Data(reset_degrees);

}

// Prints the yaw returned by pg() (decidegrees).
void Spike_Print_Gyro(void){
    Spike_Send_Serial_Data("pg()\r");
    char *return_value = Spike_Read_Serial_Data();
    printf("gyro: %s\n", return_value);
}

// Blocking: builds "turn(direction,speed,degrees)", waits for 255, then brakes.
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

    const char * return_value = Spike_Read_Serial_Data();
    if (strcmp(return_value, "") == 0){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        return_value = Spike_Read_Serial_Data();
        if(strcmp(return_value, "") == 0){
            return_value = "0";
        }
        usleep(1000);
    }

    Spike_Break_Motors();
}

// Blocking: builds "ag(speed,degrees,reference)", waits for 255, then brakes.
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

    const char * return_value = Spike_Read_Serial_Data();
    if (strcmp(return_value, "") == 0){
        return_value = "0";
    }
    while (atoi(return_value) != 255){
        return_value = Spike_Read_Serial_Data();
        if(strcmp(return_value, "") == 0){
            return_value = "0";
        }
        usleep(1000);
    }
    Spike_Break_Motors();
}

// Non-blocking: sends one "da(speed,reference)" PD step.
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