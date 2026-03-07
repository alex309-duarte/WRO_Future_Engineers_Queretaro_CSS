#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

int serial_init(void);
void send_serial_data(char data[]);
char* read_data(void);
void interpreter(void);

static int serial_port;

int main(){
    serial_init();
    interpreter();
    send_serial_data("from hub import light_matrix\r");
    send_serial_data("light_matrix.write('Hello World')\r");
    read_data();

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
			buffer_read[i] = '\0';
			break;
		}
		i++;
	}
     printf("menssage: %s\n", buffer_read);
}

char* read_data(void){
	int i = 0;
	int num_bytes;
	char buffer_read[255];
	while(1){
		num_bytes = read(serial_port,&buffer_read[i],1);
		if(num_bytes <= 0){
			printf("Error reading\n");
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
    //printf("%s",cp);
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