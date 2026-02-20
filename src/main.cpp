#include <Arduino.h>
#include <SoftwareSerial.h>
#include "avr8-stub.h"
#include "app_api.h" // only needed with flash breakpoints

#include "tasks.h"
#include "scheduler.h"

#include "dht11_access.h"
#include "mcu_temperature_access.h"
// MQTT 3.1 (Older protocol) - sometimes more stable on old hardware
// Total Length: 18 bytes
byte connectPacket[] = {
  0x10, 0x10,                          // Header, Remaining Len (16)
  0x00, 0x06, 'M', 'Q', 'I', 's', 'd', 'p', // Protocol Name (MQIsdp)
  0x03,                                // Protocol Version (3)
  0x02,                                // Clean Session
  0x00, 0x3C,                          // Keep Alive (60s)
  0x00, 0x02, 'A', '1'                 // Client ID: Len (2), ID ("A1")
};

// PUBLISH: Topic "t", Message "hi"
// Total Length: 7 bytes
byte pubPacket[] = {
  0x30, 0x05,                          // Header, Remaining Len (5)
  0x00, 0x01, 't',                    // Topic: Len (1), Name "t"
  'h', 'i'                            // Payload "hi"
};

static char * topics[4] = {"/v1.6/devices/central_conforto/dhtemp",
                           "/v1.6/devices/central_conforto/mcu_temp",
                           "/v1.6/devices/central_conforto/loud",
                           "/v1.6/devices/central_conforto/umidade"};

void sendCommand(String command, int timeout);

// Set up a new SoftwareSerial object. Pins 2 and 3 are used here.
// You can use other digital pins, but avoid pins 0 and 1 as they are
// used by the hardware serial for programming/monitoring.
SoftwareSerial ESPserial(7, 8); // RX, TX

const int   MQTT_PORT   = 1883;
const char* MQTT_BROKER = "192.168.15.50"; //endereço do broker MQTT HiveMQ

// Function to send AT commands and wait for a response
void sendCommand(String command, int timeout) {
  Serial.print(command);
  delay(1000);
  ESPserial.println(command); // Send command to ESP-01

  long current_time = millis();
  while (current_time + timeout > millis()) {
    while (ESPserial.available()) {
      Serial.write(ESPserial.read()); // Forward ESP-01 response to Serial Monitor
    }
  }
  Serial.println();
}

bool waitForPrompt(void) {
    unsigned long start = millis();
    while (millis() - start < 3000) { // 3 second timeout
        if (ESPserial.available()) {
            char c = ESPserial.read();
            Serial.write(c); // See EXACTLY what the ESP is sending
            if (c == '>') {
                return true;
            }
        }
    }

    return false;
}

void sendRaw(byte data[], int len) {
    // 1. Clear buffer of any old "busy" or "OK" messages
    while(ESPserial.available()) ESPserial.read();

    ESPserial.print("AT+CIPSEND=");
    ESPserial.println(len);

    // 2. Manual wait for '>'
    if (waitForPrompt()) {
        delay(50); // Vital "breather" for SDK 0.9.5
        ESPserial.write(data, len);
        Serial.println(" -> Bytes Injected.");
    } else {
        Serial.println("\n!!! Error: No '>' prompt from ESP-01");
        // Force a close so we can try again next loop
        ESPserial.println("AT+CIPCLOSE");
    }
}


void mqttPublish(String topic, String message) {
  int topicLen = topic.length();
  int msgLen = message.length();

  // Remaining Length = 2 (topic length bytes) + topic name + message
  byte remainingLen = 2 + topicLen + msgLen;

  // Total bytes to send = 1 (header) + 1 (remaining len) + remainingLen
  int totalLen = 2 + remainingLen;

  ESPserial.print("AT+CIPSEND=");
  ESPserial.println(totalLen);

  // Use your robust '>' waiter here
  if (waitForPrompt()) {
    ESPserial.write(0x30);         // Control Packet Type (Publish)
    ESPserial.write(remainingLen); // Remaining Length

    // Topic Length (Big Endian)
    ESPserial.write(topicLen >> 8);
    ESPserial.write(topicLen & 0xFF);

    ESPserial.print(topic);
    ESPserial.print(message);

    Serial.println(" -> Packet Injected: " + message);
  }
}

static void printIPAddress() {
  ESPserial.println("AT+CIFSR"); // Command to get the IP address
  delay(1000);
  if (ESPserial.available()) {
    while (ESPserial.available()) {
      char c = ESPserial.read();
      Serial.write(c); // Print each character to the Serial Monitor
    }
  }
}



void setup() {

    // put your setup code here, to run once:
    // debug_init();
    Serial.begin(115200);

    // Start the software serial port for communication with the ESP-01
    ESPserial.begin(9600);

    // 0. IMPORTANT: Turn off echo so "AT+CIPSEND" doesn't end up in the MQTT stream
    sendCommand("ATE0", 1000);

    // 1. Connect to WiFi
    sendCommand("AT+CWJAP=\"ponto_de_rede\",\"senha\"", 8000);

    // 2. Open TCP Connection
    sendCommand("AT+CIPSTART=\"TCP\",\"192.168.xx.xx\",1883", 4000);

    // 3. Send MQTT CONNECT Packet (Binary)
    sendRaw(connectPacket, sizeof(connectPacket));
    delay(1000);

    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    EnableSystemTasks();
    TriggerPowerOnTask();

}

void reconnectMQTT() {
  // 1. Close any ghost connections
  ESPserial.println("AT+CIPCLOSE");
  delay(500);

  // 2. ReOpen TCP
  ESPserial.println("AT+CIPSTART=\"TCP\",\"192.168.xx.xx\",1883");
  delay(2000);

  sendRaw(connectPacket, sizeof(connectPacket));
  Serial.println("MQTT Reconnected.");
}

/**
 * \brief Runs main scheduler loop.
 *
 * The **scheduler main loop** is an endless loop that monitors **pending task** flags
 *     to decide if it is time to run any of the predefined system tasks.
 *     Whenever a set flag is found, the flag is cleared, the corresponding task
 *     function is called, and the loop proceeds to its next iteration;\n
 * Since higher frequency tasks flags are pooled first and only one task is
 *     ran per loop iteration, higher frequency task are, naturally, prioritized
 *     over the lower frequencies ones;\n
 * Additionally, the loop code registers the execution time of the tasks it
 *     calls, for processor load estimation.
 */
void loop() {
    SetTasksFlags();
    RunMainLoop();


    static uint8_t minute_counter = 0;
    if (minute_counter == 1)
    {
        DHT11_run(VERY_SLOW_TIME_TASK);
        minute_counter = 0;
    } else
    {
        minute_counter++;
        //GpioAccessSet(7);
    }

    float dht_temp = Dht11GetTemperature();
    float humidity = Dht11GetHumidity();
    float mcu_temp = GetMcuInternalTemperature();
    if (dht_temp > 30)
    {
        digitalWrite(4, LOW); // Ligar ventilador
    } else if (dht_temp < 29)
    {
        digitalWrite(4, HIGH); // Desligar ventilador
    }

    Serial.println("Publishing...");

    char dht_tmp_str[10], dht_hum_str[10], mcu_temp_str[10];
    dtostrf(dht_temp, 4, 2, dht_tmp_str);
    dtostrf(humidity, 4, 2, dht_hum_str);
    dtostrf(mcu_temp, 4, 2, mcu_temp_str);

    Serial.print("Umidade: ");
    Serial.println(humidity);
    Serial.print("DHT: ");
    Serial.println(dht_temp);
    Serial.print("MCU: ");
    Serial.println(mcu_temp);

    // sendRaw(pubPacket, 7);
    static uint8_t i = 0;
    if (i%4 == 0) {
        if (mcu_temp)
        {
            Serial.print("Pucblicar MCU temp: ");
            Serial.println(mcu_temp_str);
            mqttPublish(topics[1], mcu_temp_str);
        }
    } else if (i%4 == 1)
    {
        if (IsSoundAlarm())
        {
            mqttPublish(topics[2], "1");
        } else
        {
            mqttPublish(topics[2], "0");
        }
    } else if (i%4 == 2) {
        if(dht_temp) {
            Serial.print("Publicar DHT temp: ");
            Serial.println(dht_tmp_str);
            mqttPublish(topics[0], dht_tmp_str);
        }
    } else
    {
        if(humidity)
        {
            Serial.print("Publicar Umidade: ");
            Serial.println(dht_hum_str);
            mqttPublish(topics[3], dht_hum_str);
        }
    }

    i++;

    // MQTT Keep-Alive is 60s. We wait 20s.
    unsigned long waitStart = millis();
    while(millis() - waitStart < 20000) {
        if (ESPserial.available()) {
            String resp = ESPserial.readString();
            Serial.print("ESP: "); Serial.println(resp);
            if (resp.indexOf("CLOSED") != -1) {
                reconnectMQTT();
                break;
            }
        }
    }
}