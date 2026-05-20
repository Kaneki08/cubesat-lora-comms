// include the library
#include <Arduino.h>
#include <RadioLib.h>
#include "Packet.h"
#include "HopeRFTX.h"



void transmit_beacon() {

}

/*
encodeHex():
Parses header and payload data into a byte array for radio.transmit().
Uses memcpy() to copy information first from the header, then the payload.
*/
uint8_t encodeHex(uint8_t* queue, size_t queue_size, const PacketHeader* header, const void* payload) {
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
bool debounceRead(int gpio) {
    static unsigned long t1 = millis();  // read initial time t1 to compare with final time t2 for debounce delay
    static uint8_t prevButtonState = 1;  // track previous button state to detect falling edges only

    uint8_t currButtonState = digitalRead(gpio);  // read button input pin
    bool fallEdge = ((prevButtonState == 1) && (currButtonState == 0));  // flag if falling edge detected
    prevButtonState = currButtonState;  // update state

    if (fallEdge) { // Falling edge detect from 1 to 0
        unsigned long t2 = millis();
        if ((t2 - t1) >= 250) { // 250 ms debounce
            t1 = t2;
            return true;
        }
    }

    return false;
}

// Transmit wrapper around transmitOnce
void transmitNTimes(SX1278& radio, uint8_t packet_type, void* payload, uint8_t payload_size, uint16_t& count, uint8_t repeats) {
    for (int i = 0; i < repeats; i++)
        transmitOnce(radio, packet_type, payload, payload_size, count, 0, 0);
}

// Transmit a packet once
void transmitOnce(SX1278& radio, uint8_t packet_type, void* payload, uint8_t payload_size, uint16_t& count, uint8_t downlink_start, uint8_t downlink_end) {
    int transmissionState = RADIOLIB_ERR_NONE;
    uint8_t TXBuffer[128];    // Holds raw binary data
    uint8_t asciiBuffer[256]; // Holds coverted ASCII-hex data (needs to be 2x the size)

        PacketHeader header = {
            .sat_id = 0x01,
            .packet_type = packet_type,      // Use passed packet type
            .sequence = count++,              // Increment sequence number
            .timestamp = millis(),            // Fresh timestamp
            .payload_len = payload_size
        };

    // Pack raw binary data into TXBuffer // Encode header + payload
    uint8_t dataLen = encodeHex(TXBuffer, sizeof(TXBuffer), &header, payload); 
    
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
    
    transmissionState = radio.transmit(asciiBuffer, asciiLen);

    if (transmissionState == RADIOLIB_ERR_NONE) {
        Serial.println(F("transmission finished!"));
    } else if (transmissionState == RADIOLIB_ERR_TX_TIMEOUT) {
        Serial.println(F("transmission timeout!"));
    } else {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);
    }

    delay(500); // Temporary Safe Time on Air value of 0.5s

}


// helper function to transit packages
void transmit(SX1278& radio, uint8_t packet_type, void* payload, uint8_t payload_size, uint16_t& count) {
    int transmissionState = RADIOLIB_ERR_NONE;
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
        uint8_t dataLen = encodeHex(TXBuffer, sizeof(TXBuffer), &header, payload); 
        
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
        
        // Uses RadioLib's internal TX timeout (see RadioLib docs)
        // For custom timeout control, use startTransmit() with ISR flag instead
        transmissionState = radio.transmit(asciiBuffer, asciiLen);

        if (transmissionState == RADIOLIB_ERR_NONE) {
            Serial.println(F("transmission finished!"));
        } else if (transmissionState == RADIOLIB_ERR_TX_TIMEOUT) {
            Serial.println(F("transmission timeout!"));
        } else {
            Serial.print(F("failed, code "));
            Serial.println(transmissionState);
        }

        // delay(500);  // Wait before next transmission  // depends on the Time on Air 
    }
}

/*  
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
*/



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

