#include "spike.h"

#define GPIO_BUTTON 4
#define GPIO_LED_BUTTON 17
#define GPIO_RELE 12

unsigned int intput_GPIO[] = {GPIO_BUTTON};
unsigned int outputs_GPIO[] = {GPIO_LED_BUTTON, GPIO_RELE};

const char *chip_path = "/dev/gpiochip4";
static int serial_port;

struct gpiod_chip *chip;
struct gpiod_line_settings *settings_input;
struct gpiod_line_settings *settings_outputs;
struct gpiod_line_config *line_cfg;
struct gpiod_request_config *req_cfg;
struct gpiod_line_request *request;

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

void close_serial(void){
    close(serial_port);
}

void send_serial_data(char data[]){
    int num_bytes;

	char buffer_read[255] = "";
	const char * buffer_Spike = data;
    //printf("Data %s\n",buffer_Spike);
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
    //printf("menssage: %s\n", buffer_read);
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
    send_serial_data("error = pd(motion_sensor.tilt_angles()[0],((10)*(referencia)),vel,0.3,1,error)\r");
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

    const char * list[5];
    list[0] = "rg(";
    list[1] = (const char *)string_grados;
    list[2] = ")\r";
    concatenar(3,list, grados_a_recetear);

    send_serial_data(grados_a_recetear);

}

void print_gyro(void){
    send_serial_data("pg()\r");
    char *return_value = read_data();
    printf("gyro: %s\n", return_value);
}

void vuelta_grados(int direccion, int velocidad, int grados){
	char argumentos[255];
	char string_direccion[10] = "";
	char string_velocidad[10] = "";
	char string_grados[10] = "";

	snprintf(string_direccion, sizeof(string_direccion), "%d", direccion);
	snprintf(string_velocidad, sizeof(string_velocidad), "%d", velocidad);
	snprintf(string_grados, sizeof(string_grados), "%d", grados);

	const char * lista_a_concatenar[10];
	lista_a_concatenar[0] = "vuelta(";
	lista_a_concatenar[1] = (const char *)string_direccion;
	lista_a_concatenar[2] = ",";
	lista_a_concatenar[3] = (const char *)string_velocidad;	
	lista_a_concatenar[4] = ",";
	lista_a_concatenar[5] = (const char *)string_grados;	
	lista_a_concatenar[6] = ")\r";
		
	concatenar(7,lista_a_concatenar, argumentos);

	send_serial_data(argumentos);

}

void avanzar_grados(int velocidad, int grados, int referencia){
	char argumentos[255];
	char string_velocidad[10] = "";
	char string_grados[10] = "";
    char string_referencia[10] = "";

	snprintf(string_velocidad, sizeof(string_velocidad), "%d", velocidad);
	snprintf(string_grados, sizeof(string_grados), "%d", grados);
	snprintf(string_referencia, sizeof(string_referencia), "%d", referencia);

	const char * lista_a_concatenar[10];
	lista_a_concatenar[0] = "ag(";
	lista_a_concatenar[1] = (const char *)string_velocidad;	
	lista_a_concatenar[2] = ",";
	lista_a_concatenar[3] = (const char *)string_grados;
    lista_a_concatenar[4] = ",";
    lista_a_concatenar[5] = (const char *)string_referencia;	
	lista_a_concatenar[6] = ")\r";
		
	concatenar(7,lista_a_concatenar, argumentos);

	send_serial_data(argumentos);

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

int init_gpio(void){
    chip = gpiod_chip_open(chip_path);
    if (!chip) {
        printf("Failed to open GPIO chip");
        return EXIT_FAILURE;
    }

    settings_input = gpiod_line_settings_new();
    settings_outputs = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings_input, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_direction(settings_outputs, GPIOD_LINE_DIRECTION_OUTPUT);

    gpiod_line_settings_set_output_value(settings_outputs, GPIOD_LINE_VALUE_INACTIVE);
    gpiod_line_settings_set_bias(settings_input, GPIOD_LINE_BIAS_PULL_UP);

    line_cfg = gpiod_line_config_new();

    gpiod_line_config_add_line_settings(line_cfg, intput_GPIO, 1, settings_input);
    gpiod_line_config_add_line_settings(line_cfg, outputs_GPIO, 2, settings_outputs);

    req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "First challenge program");

    request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    return 0;
}

void clean_GPIO(void){
    gpiod_line_request_release(request);
    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings_input);
    gpiod_line_settings_free(settings_outputs);
    gpiod_chip_close(chip);
    printf("Pines liberados correctamente.\n");
}

void wait_for_button(void){
    while(gpiod_line_request_get_value(request, intput_GPIO[0]) == GPIOD_LINE_VALUE_ACTIVE){
        gpiod_line_request_set_value(request, outputs_GPIO[0], GPIOD_LINE_VALUE_ACTIVE);
        usleep(50000);
        gpiod_line_request_set_value(request, outputs_GPIO[0], GPIOD_LINE_VALUE_INACTIVE);
        usleep(50000);
    }
    gpiod_line_request_set_value(request, outputs_GPIO[0], GPIOD_LINE_VALUE_INACTIVE);
}

void power_On_Spike(void){
    gpiod_line_request_set_value(request, outputs_GPIO[1], GPIOD_LINE_VALUE_ACTIVE);
    usleep(500000);
    gpiod_line_request_set_value(request, outputs_GPIO[1], GPIOD_LINE_VALUE_INACTIVE);
    usleep(1000000); //one second waiting for spikr initialization 
}

void Spike_forward(int velocidad, int referencia){
    char argumentos[255];
	char string_velocidad[10] = "";
    char string_referencia[10] = "";

    snprintf(string_velocidad, sizeof(string_velocidad), "%d", velocidad);
	snprintf(string_referencia, sizeof(string_referencia), "%d", referencia);

    const char * lista_a_concatenar[10];
	lista_a_concatenar[0] = "da(";
	lista_a_concatenar[1] = (const char *)string_velocidad;	
	lista_a_concatenar[2] = ",";
    lista_a_concatenar[3] = (const char *)string_referencia;	
	lista_a_concatenar[4] = ")\r";

    concatenar(5, lista_a_concatenar, argumentos);

    send_serial_data(argumentos);

}