#include "MQTTClient.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>   // File control options
#include <errno.h>
#include <termios.h> // POSIX terminal control definitions
#include <unistd.h>
#include <stdbool.h>

// EMQX Connection Details
// #define ADDRESS     "mqtt://industrial.api.ubidots.com"
#define ADDRESS         "mqtt://127.0.0.1:1883"
#define CLIENTID        "Paho_C_Tutorial_Client"
// #define CLIENTID    "697673c66470a2873c4f6fdc"
#define QOS         1
#define TIMEOUT     10000L // 10 seconds

const char * UBIDOTS_TOKEN = "BBUS-7mSAw9bBmsabAdXxFYuo2K50LubNjd"; // username


static char * topics[4] = {"/v1.6/devices/central_conforto/dhtemp", 
                           "/v1.6/devices/central_conforto/mcu_temp",
                           "/v1.6/devices/central_conforto/loud",
                           "/v1.6/devices/central_conforto/umidade"};

int set_interface_attribs (int fd, int speed, int parity)
{
        struct termios tty;
        if (tcgetattr (fd, &tty) != 0)
        {
                // error_message ("error %d from tcgetattr", errno);
                return -1;
        }

        cfsetospeed (&tty, speed);
        cfsetispeed (&tty, speed);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
        // disable IGNBRK for mismatched speed tests; otherwise receive break
        // as \000 chars
        tty.c_iflag &= ~IGNBRK;         // disable break processing
        tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = 0;            // read doesn't block
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
                // error_message ("error %d from tcsetattr", errno);
                return -1;
        }
        return 0;
}

static bool chegou = false;

// The callback function executed when a message is received
int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    printf("Message received on topic: %s\n", topicName);
    printf("Payload: %.*s\n", message->payloadlen, (char*)message->payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    chegou = true;
    return 1;
}

static void mqttCreateClient(MQTTClient * client)
{
	MQTTClient_create(client, ADDRESS, CLIENTID,
    MQTTCLIENT_PERSISTENCE_NONE, NULL);

    printf("Cliente criado!\n");
    
    // Set the message callback function
    MQTTClient_setCallbacks(*client, NULL, NULL, messageArrived, NULL);
}

static void configMqttConnection(void)
{
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20; // Send PINGREQ every 20 seconds
    conn_opts.cleansession = 1;       // Start a new session every time
}

static uint8_t mqttConnectToBroker(MQTTClient client)
{
    uint8_t rc;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.username = UBIDOTS_TOKEN;
    printf("Attempting to connect to EMQX Broker...\n");
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect to EMQX, return code %d\n", rc);
        return rc;
    }
    printf("Successfully connected to EMQX!\n");
    return 0;
}

static void mqttSubscribe(MQTTClient client)
{
    printf("Subscribing to topic \"%s\" with QoS %d\n", topics[0], QOS);
    MQTTClient_subscribe(client, topics[0], QOS);

    printf("Subscribing to topic \"%s\" with QoS %d\n", topics[1], QOS);
    MQTTClient_subscribe(client, topics[1], QOS);
}

static void mqttSend(MQTTClient client, char payload[5], uint8_t msg_index)
{
    
    // --- Step 5: Publish a Message ---
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = payload;
    if(msg_index <=1 || msg_index ==3)
    {
        if(msg_index==3)
        {
            pubmsg.payloadlen = 4;
        } else
        {
            pubmsg.payloadlen = 5;
        }
    } else
    {
        pubmsg.payloadlen = (int)strlen(payload);
    }
    
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    MQTTClient_deliveryToken token;
    printf("Publishing message: \"%s\"\n", payload);
    if(msg_index==3)
    {
        printf("Valor de umidade eh %s\n", payload);
        printf("Tamanho da string: %d\n", pubmsg.payloadlen);
    }

    // Publish and wait for delivery confirmation (Synchronous)
    MQTTClient_publishMessage(client, topics[msg_index], &pubmsg, &token);
    int rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    // printf("Message with token value %d delivered.\n", token);
}

static void mqttDisconnect(MQTTClient client)
{
    // Keep the main thread alive to receive messages (Subscriber loop)
    printf("Client is listening for messages. Press Enter to disconnect...\n");
    getchar();

    // --- Step 6: Disconnect and Cleanup ---
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
}

int main(int argc, char* argv[]) {

    MQTTClient client;
    
    // --- Step 1: Create the Client ---
	mqttCreateClient(&client);
	// --- Step 2: Configure Connection Options ---
	configMqttConnection();
	// --- Step 3: Connect to the Broker ---
    if(mqttConnectToBroker(client))
    {
        return 0;
    } else
    {
        printf("Conectado com sucesso ao broker!\n");
    }
    // --- Step 4: Subscribe to the Topic ---
    mqttSubscribe(client);
    
    // -- Step A: Abrir serial
    int fd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        printf("Error opening port: %s\n", strerror(errno));
        return -1; // Or handle error appropriately
    }
    
    // -- Step B: Configurar BAUD
    set_interface_attribs(fd, B115200, 0); // Set baud rate to 115200

    uint8_t str_i = 0;
    char ntc_string[5]; // temp do termistor
    float ntc_val;
    char mcu_temp_string[5];   // temp do microcontrolador
    float mcu_temp_val;
    char sound_alarm_char;
    uint8_t is_sound_alarm;
    char hum_string[4]; // string de humidade
    float humidity_val;
    uint8_t simple_parser = 0;  // 0 = ntc
    				// 1 = microcontrolador
    				// 2 = sensor de umidade
    				// 3 = sensor de barulho
    				// 4 = situacao do atuador 1
    				// 5 = situacao do atuador 2
    				
    char *end_ptr;	
    
    while(1)
    {
    	   // sleep(3);
  
	    // -- Step C: Read data
	    char buffer[15];
	    // Inside your loop
	    ssize_t length = read(fd, buffer, sizeof(buffer) - 1);
	    if (length >1) {
            // Process received data
            for(uint8_t i=0, switch_index = 0 ; i<length ; i++)
            {
                //printf("Letra!\n");
                printf("%c", buffer[i]);
                if (buffer[i] == ' ' || switch_index > 4)
                {
                    printf("Vai incrementar o parser...\n");
                    simple_parser++;
                    switch_index = 0;
                    continue;
                } else if (buffer[i] == '\n')
                {
                    simple_parser = 0;
                    switch_index = 0;
                    break;
                }

                // printf("Switch_index = %d\n", switch_index);
                switch (simple_parser)
                {
                    case 0: ntc_string[switch_index] = buffer[i]; break;
                    case 1: mcu_temp_string[switch_index] = buffer[i]; break;
                    case 2: sound_alarm_char = buffer[i]; break;
                    case 3: hum_string[switch_index] = buffer[i]; break;
                }
                switch_index++;
            }

            // NTC: Convert the string to a float
            // printf("Vai converter a string NTC_TMP: %s\n", ntc_string);
            ntc_val = strtof(ntc_string, &end_ptr);
            if (ntc_string == end_ptr) {
                printf("Error: No valid number found in string NTC: %s\n", ntc_string);
            } else
            {
                if(ntc_val > 10 && ntc_val < 60)
                {
                    printf("\n\n Temp no NTC: %.2f\n", ntc_val);
                    printf("Enviando a string: %s\n", ntc_string);
                    mqttSend(client, ntc_string, 0);
                }else
                {
                    printf("Temperatura NTC invalida: %.2f\n", ntc_val);
                }
            }

            // MCU TEMP: Convert the string to a float
            // printf("Vai converter a string MCU_TMP: %s\n", mcu_temp_string);
            mcu_temp_val = strtof(mcu_temp_string, &end_ptr);
            if (mcu_temp_string == end_ptr) {
                printf("Error: No valid number found in string MCU TEMP: %s\n\n", mcu_temp_string);
            } else
            {
                if(mcu_temp_val > 10 && mcu_temp_val < 60)
                {
                    printf("\n\n Temp no MCU: %.2f\n", mcu_temp_val);
                    printf("\n\n Enviando a string: %s\n", mcu_temp_string);
                    mqttSend(client, mcu_temp_string, 1);
                } else
                {
                    printf("Temperatura MCU invalida: %.2f\n", mcu_temp_val);
                }
            }



            // SOUND ALARM: Convert the string to a bool
            // printf("Vai converter o char SOUND ALARM: %c\n", sound_alarm_char);
            sscanf(&sound_alarm_char, "%hhd", &is_sound_alarm); // num will be the integer 7
            printf("Valor convertido: %d\n", is_sound_alarm);
            if(is_sound_alarm)
            {
                printf("\n\n Som alto detectado!\n");
                mqttSend(client, "Som alto no ambiente detectado!", 2);
            }

            // HUMIDITY: Convert the string to a float
            // printf("Vai converter a string HUMIDITY: %s\n", hum_string);
            humidity_val = strtof(hum_string, &end_ptr);
            if (hum_string == end_ptr) {
                printf("Error: No valid number found in string HUMIDITY: %s\n\n", hum_string);
            } else
            {
                if(humidity_val > 40)
                {
                    printf("\n\n Umidade: %.2f\n", humidity_val);
                    snprintf(hum_string, 5, "%f", humidity_val);
                    // printf("\n\n Enviando a string: %s\n", hum_string);
                    mqttSend(client, hum_string ,3);
                } else
                {
                    printf("Humidade invalida: %.2f\n", humidity_val);
                }
            }

            

	    } else if (length < 0) {
		// Handle read error
	    }
	    // If length is 0, it's a timeout (in non-canonical mode with VMIN/VTIME set)
    }

    
}
