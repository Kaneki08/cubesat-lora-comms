// include the library
#include <Arduino.h>
#include <RadioLib.h>
#include "HopeRFTX.h"

namespace {
  constexpr size_t kTxBufferSize = 256;
}

// Initialize HopeRF
Module module(7, 1, 2, 0);
SX1278 radio(&module);

    /*!
      \brief Module constructor.
      \param hal A Hardware abstraction layer instance. An ArduinoHal instance for example.
      \param cs Pin to be used as chip select.
      \param irq Pin to be used as interrupt/GPIO.
      \param rst Pin to be used as hardware reset for the module.
      \param gpio Pin to be used as additional interrupt/GPIO.
    */

// Dummy data to test physical configuration and radio transmission functionality 
byte test[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

// Test string transmission 
byte helloLora[10] = {0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x4C, 0x6F, 0x52, 0x61};

// String packet
StringPayload stringPacket;

// Save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// Flag to indicate that a packet was sent
volatile bool transmittedFlag = true;

// this function is called when a complete packet
// is transmitted by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  // we sent a packet, set the flag
  transmittedFlag = true;
}

/*
encodeHex():
Parses header and payload data into a byte array for radio.transmit().
Uses memcpy() to copy information first from the header, then the payload.
*/
uint8_t encodeHex(uint8_t* queue, size_t queue_size, const PacketHeader* header, const void* payload)
{
  const size_t required_size = sizeof(PacketHeader) + header->payload_len;
  if ((required_size > queue_size) || (required_size > UINT8_MAX)) {
    return 0;
  }

  uint8_t dataLen = 0;

  memcpy(queue + dataLen, header, sizeof(PacketHeader));  // copy all header data to the queue, indicating the size of header data
  dataLen += sizeof(PacketHeader);  // track data length: queue has now incremented from current position to (sizeof(PacketHeader))

  // Add a indicator character between packet header and packet payload
  queue[dataLen++] = 0x24;  // Example indicator character // Encode $ into 0x24

  memcpy(queue + dataLen, payload, header->payload_len);  // copy all payload data to the queue. Get info from header about the payload size
  dataLen += header->payload_len;

  return dataLen;
}

// Reads falling edges from a GPIO and includes a 250 ms window to filter button bounces
bool debounceRead(int gpio)
{
  static unsigned long t1 = millis();  // read initial time t1 to compare with final time t2 for debounce delay
  static uint8_t prevButtonState = 1;  // track previous button state to detect falling edges only

  uint8_t currButtonState = digitalRead(gpio);  // read button input pin
  bool fallEdge = ((prevButtonState == 1) && (currButtonState == 0));  // flag if falling edge detected
  prevButtonState = currButtonState;  // update state

  if (fallEdge)  // Falling edge detect from 1 to 0
  {
    unsigned long t2 = millis();
    if ((t2 - t1) >= 250)  // 250 ms debounce
    {
      t1 = t2;
      return true;
    }
    
  }
  
  return false;
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  // SPI.begin(4, 6, 5, 7);  // this is wrong pin mapping
  SPI.begin(4, 5, 6, 7);  // SCK, MISO, MOSI, SS (CS) pins for ESP32-C3

  pinMode(BUTTON_GPIO, INPUT_PULLUP);

  // initialize SX1278 with default settings
  Serial.print(F("[SX1278] Initializing ... "));

  // frequency, bandwidth, sf, cr, syncword, TX Power, LoRa PHY preamble length, rx gain (AGC)
  int state = radio.begin(433.0, 125.0, 9, 7, RADIOLIB_SX127X_SYNC_WORD, 10, 8, 0);


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

  // set the function that will be called
  // when packet transmission is finished
  radio.setPacketSentAction(setFlag);

  strcpy(stringPacket.message, "Hello World!");

}

// counter to keep track of transmitted packets
uint16_t count = 0;

/* Initialize IMU Payload */
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
/* Initialize TC Payload */
struct TCPayload newTCPayload = {
  .tc_avg1 = 0,
  .tc_avg2 = 0
};


struct full_telemetry newfull_telemetry = {
  .imu_payload = newIMUPayload,
  .batt_payload = newcombined_telemetry_1,
  .tc_payload = newTCPayload
};



// helper function to transit packages
void transmit(uint8_t packet_type, void* payload, uint8_t payload_size) {

  // Translate numerical data into ascii-hex encoding too

  uint8_t TXBuffer[128];    // Holds raw binary data
  uint8_t asciiBuffer[256]; // Holds coverted ASCII-hex data (needs to be 2x the size)

  int packets = 10;  // Number of packets to transmit in the loop
  for (int i = 0; i < packets; i++) {
    // Create fresh header for each transmission in the loop
    PacketHeader header = {
      .sat_id = 0x01,
      .packet_type = packet_type,      // Use passed packet type
      .sequence = count++,              // Increment sequence number
      .timestamp = millis(),            // Fresh timestamp
      .payload_len = payload_size
    };
    
    // Pack raw binary data into TXBuffer // Encode header + payload
    uint8_t dataLen = encodeHex(TXBuffer, &header, payload);
    
    // Translate numerical data into ASCII-hex encoding
    const char hexChars[] = "0123456789ABCDEF";
    uint16_t asciiLen = 0;  // Length of the new ASCII payload

    for (int j = 0; j < dataLen; j++ ) {
      // Extract top 4 bits (high nibble) and get corresponding hex char
      asciiBuffer[asciiLen++] = hexChars[(TXBuffer[j] >> 4) & 0x0F];
      // Extract bottom 4 bits (low nibble) and get corresponding hex char
      asciiBuffer[asciiLen++] = hexChars[TXBuffer[j] & 0x0F];
    }

    Serial.print(F("[SX1278] Sending packet type "));
    Serial.print(packet_type);
    Serial.print(F(" (attempt "));
    Serial.print(i + 1);
    Serial.print(F("/"));
    Serial.print(packets);
    Serial.print(F(") ... "));

    transmittedFlag = false;  // Reset flag before transmission
    transmissionState = radio.startTransmit(asciiBuffer, asciiLen);

    if (transmissionState == RADIOLIB_ERR_NONE) {
      // Wait for setFlag() ISR to set transmittedFlag = true
      unsigned long timeout = millis() + 1000;  // 1 second timeout
      while (!transmittedFlag && millis() < timeout) {
        delay(10);
      }
      
      if (transmittedFlag) {
        Serial.println(F("transmission finished!"));
      } else {
        Serial.println(F("transmission timeout!"));
      }
    }
    else {
      Serial.print(F("failed, code "));
      Serial.println(transmissionState);
    }

    delay(500);  // Wait before next transmission
  }
}

void loop() {

  
  transmit(PACKET_IMU, &newIMUPayload, sizeof(newIMUPayload));
  delay(2000);  // 2 second delay between transmissions for testing

  // bool buttonFlag = debounceRead(BUTTON_GPIO);
  // // Serial.print("Button Flag: ");
  // // Serial.println(buttonFlag);

  // if(transmittedFlag && buttonFlag)
  // {
  //   transmittedFlag = false;

  //   // Update TC payload with fresh data
  //   TCPayload tcData = {
  //     .tc_avg1 = 25.5,
  //     .tc_avg2 = 26.3
  //   };
    
  //   // Try transmitting the IMU packet with fresh data
  //   // IMUPayload imuData = {
  //   //   .gx_dps = 1.2,
  //   //   .gy_dps = 0.5,
  //   //   .gz_dps = -0.8,
  //   //   .ax_mg = 100,
  //   //   .ay_mg = -50,
  //   //   .az_mg = 980,

  //   //   .qi = 0.707,
  //   //   .qj = 0,
  //   //   .qk = 0.707,
  //   //   .qr = 0
  //   // };

  //   // transmit(PACKET_IMU, &newIMUPayload, sizeof(newIMUPayload));
  //   transmit(PACKET_TC, &tcData, sizeof(TCPayload));
  //   // transmit(PACKET_STRING, &stringPacket, sizeof(StringPayload));

  //   // transmit(PACKET_FULL_TELEMETRY, &newfull_telemetry, sizeof(full_telemetry));
  //   // wait(30000);  // Wait 30 secs before next transmission
  //   // if (heard successful reception from GS with checksum verification) {
  //   //    then move on to next data to transmit  
  //   // } else {
  //   //   retry the same data transmission in the next loop 
  //   // }
  // }
}




/*
Ideal Downlink Situation: 

1. Collect all onboard sensor data and process it in the avionics MCU 
2. Transmit 1 packet that contains all telemetry in a well defined and ordered format
3. Receive the full packet without any errors

Problems with Ideal Situation: 
1. Overflow of data
2. Packet loss/ corruption due to interference or unreliable radio link


Realistic Downlink Situation:
1. Collect all onboard sensor data and process it in the avionics MCU
2. Transmit multiple packets separating different sensor categories to reduce overflow 
3. Need CRC and FEC but adds overhead 


*/


/*
Process to transmit data:
1. Create packet header and payload structs, fill in all necessary information
2. Create a byte array, TXBuffer, to hold the encoded packet header and payload information
3. Call encodeHex(TXBuffer, sizeof(TXBuffer), &PacketHeader, &IMUPayload) to serialize the frame, returning the number of bytes to be transmitted in radio.transmit(TXBuffer, dataLen)
4. Call radio.transmit(TXBuffer, dataLen) to transmit the data in the TXBuffer byte array, where dataLen is the number of bytes to be transmitted as returned by encodeHex()

TXBuffer[256];

encodeHex(TXBuffer, sizeof(TXBuffer), &PacketHeader, &IMUPayload);

TXBuffer -> {packet header, payload}

transmit(txbuffer)
*/

