#include <Arduino.h>
#include <RadioLib.h>
#include <stdint.h>
#include "Packet.h"
#include "HopeRFTX.h"
#include "HopeRFRX.h"

enum STATE {BEACON, DOWNLINK, UPLINK};

STATE currentState = BEACON;

// String packet
StringPayload stringPacket;

// Counter to keep track of transmitted packets 
uint16_t count = 0;

// handshake flag
bool handshakeComplete = false;

// Initialize HopeRF
// Module(cs=7, irq=1, rst=2, gpio=0)
Module module(7, 1, 2, 0);
SX1278 radio(&module);

// Dummy data to test physical configuration and radio transmission functionality 
byte test[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

/* Initialize dummy IMU Payload */
struct IMUPayload newIMUPayload = {
    .gx_dps = 9,
    .gy_dps = 1,
    .gz_dps = 1,

    .ax_mg = 1,
    .ay_mg = 1,
    .az_mg = 1,

    .qi = 1,
    .qj = 6,
    .qk = 1,
    .qr = 7
};

// initialize dummy battery combined telemetry payload
struct batt_combined_telemetry_1 newcombined_telemetry_1 = {
    .current_mA = -10,
    .avg_current_mA = -154,
    .voltage_mV = 13926,
    .average_voltage_mV = 13825,
    .cycle_count = 187,
    .temperature_0_1K = 3003,
    .external_temp_sensor1_0_1K = 3003,
    .external_temp_sensor2_0_1K = 3013,
    .external_temp_sensor3_0_1K = 3015,
    .external_temp_sensor4_0_1K = 3016,
    .external_temp_sensor5_0_1K = 3015,
    .external_temp_sensor6_0_1K = 3016,
    .external_temp_sensor7_0_1K = 3017,
    .external_temp_sensor8_0_1K = 2977,
    .cell_voltage1_mV = 0,
    .cell_voltage2_mV = 0,
    .cell_voltage3_mV = 0,
    .cell_voltage4_mV = 0
};

/* Initialize dummy TC Payload */
struct TCPayload newTCPayload = {
    .tc_avg1 = 0,
    .tc_avg2 = 0
};

// initialize dummy full telemetry data
struct full_telemetry newfull_telemetry = {
    .imu_payload = newIMUPayload,
    .batt_payload = newcombined_telemetry_1,
    .tc_payload = newTCPayload
};

// Test string transmission 
byte helloLora[10] = {0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x4C, 0x6F, 0x52, 0x61};


volatile bool rxReady = false;

void IRAM_ATTR onReceive() {
    rxReady = true;
}

// setup esp
void setup() {
    Serial.begin(115200);
    delay(1000);
    SPI.begin(4, 5, 6, 7);  // SCK, MISO, MOSI, SS (CS) pins for ESP32-C3

    pinMode(BUTTON_GPIO, INPUT_PULLUP);

    // initialize SX1278 with default settings
    Serial.print(F("[SX1278] Initializing ... "));

    // frequency, bandwidth, sf, cr, syncword, TX Power, LoRa PHY preamble length, rx gain (AGC)
    // int state = radio.begin(433.0, 125.0, 9, 7, RADIOLIB_SX127X_SYNC_WORD, 10, 8, 0);
    int state = radio.begin(433.0, 125.0, 9, 4, 0x12, 10, 8, 0);

    if (state == RADIOLIB_ERR_NONE) {
        state = radio.explicitHeader();
    }

    if (state == RADIOLIB_ERR_NONE) {
        state = radio.setCRC(true);
    }

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(state);
        while (true) { delay(10); }
    }

    radio.setPacketReceivedAction(onReceive);   // Register ISR
    radio.startReceive();
}


void loop() {

    if (rxReady) {
        rxReady = false;    // clear flag

        int len = radio.getPacketLength();
        uint8_t buf[len];  
        int state = radio.readData(buf, len);

        if (state == RADIOLIB_ERR_NONE) {
            Serial.println("valid packet");
            printRXBuffer(buf, len);
        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
            Serial.println("CRC mismatch");
        } else {
            Serial.print("read data failed: ");
            Serial.println(state);
        }

        radio.startReceive();   // re-arm receiver
    }

    // Serial.println(F("Hello from main!"));

    switch (currentState) {
    
    case BEACON:    
        // 1.   Beacon transmit the handshake
        if (!handshakeComplete) {       // handshake
            strcpy(stringPacket.message, "ZOT ZOT ZOT from SAT");
            for (int repeat = 0; repeat < 2; repeat ++)     // Send two beacon packets
                transmitOnce(radio, PACKET_STRING, &stringPacket, sizeof(stringPacket), count, 0, 0);
            if (receiveHandshake(radio)) {
                handshakeComplete = true;
                currentState = DOWNLINK;
            }
            return;   // No Downlink before handshake is established
        }

        delay(30000);      // 2.   Wait for 30 seconds for a response from GS
        
        // 3.   If ACK: complete handshake with ACK back and begin downlink, else return
        if (acknowledged(radio, count - 1)) {         // wait for ACK
            Serial.println("ACK received");
        } else {
            Serial.println("ACK failed, retrying...");
        }

        break;
    
    case DOWNLINK:
        transmitOnce(radio, PACKET_IMU, &newIMUPayload, sizeof(newIMUPayload), count, 1, 0);
        transmitNTimes(radio, PACKET_IMU, &newIMUPayload, sizeof(newIMUPayload), count, 5);
        transmitOnce(radio, PACKET_IMU, &newIMUPayload, sizeof(newIMUPayload), count, 0, 1);

        currentState = UPLINK;
        

        break;
    
    case UPLINK:
        testReceive(radio);
        break;

    default:
        break;
    }


    
}

