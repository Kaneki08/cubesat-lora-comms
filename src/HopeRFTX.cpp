// include the library
#include <Arduino.h>
#include <RadioLib.h>
#include "HopeRFTX.h"

// Initialize HopeRF
Module module(7, 1, 2, 0);
SX1278 radio(&module);

// Dummy data to test physical configuration and radio transmission functionality 
byte test[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

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
Parses header and payload data into a byte array, queue, returning # of indexes (dataLen) to be transmitted in radio.transmit(queue, dataLen)
Uses memcpy() to copy information first from the header, then the payload. In between, dataLen is incremented before being returned in the final line.
*/
uint8_t encodeHex(uint8_t* queue, PacketHeader* header, void* payload)
{
  uint8_t dataLen = 0;

  memcpy(queue + dataLen, header, sizeof(PacketHeader));  // copy all header data to the queue, indicating the size of header data
  dataLen += sizeof(PacketHeader);  // track data length: queue has now incremented from current position to (sizeof(PacketHeader))

  // add a space between packet header and packet payload

  memcpy(queue + dataLen, payload, header->payload_len);  // copy all payload data to the queue. Get info from header about the payload size
  dataLen += header->payload_len;

  // REQUIRED: add overflow checks, clear memory if needed, etc.

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
  SPI.begin(4, 6, 5, 7);

  pinMode(BUTTON_GPIO, INPUT_PULLUP);

  // initialize SX1278 with default settings
  Serial.print(F("[SX1278] Initializing ... "));

  // frequency, bandwidth, sf, cr, syncword, TX Power, LoRa PHY preamble length, rx gain (AGC)
  int state = radio.begin(433.0, 125.0, 9, 7, RADIOLIB_SX127X_SYNC_WORD, 10, 8, 0);


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
}

// counter to keep track of transmitted packets
int count = 0;

/* Initialize Packet Header */
struct PacketHeader newPacketHeader = {
  .sat_id = 0x01,                     // callsign??
  .packet_type = PACKET_IMU,          // bitfield
  .sequence = count,                  // packet order since transmission cycle start
  .timestamp = millis(),              // since program start
  .payload_len = sizeof(IMUPayload),  // in bytes
};

/* Initialize IMU Payload */
struct IMUPayload newIMUPayload = {
  .gx_dps = 0,
  .gy_dps = 0,
  .gz_dps = 0,

  .ax_mg = 0,
  .ay_mg = 0,
  .az_mg = 0,

  .qi = 0,
  .qj = 0,
  .qk = 0,
  .qr = 0
};

/* Initialize TX Buffer */
uint8_t TXBuffer[256];

/* Encode Packet Into Hex */
uint8_t dataLen = encodeHex(TXBuffer, &newPacketHeader, &newIMUPayload);

void loop() {
  bool buttonFlag = debounceRead(BUTTON_GPIO);

  // Check if the previous transmission finished
  if(transmittedFlag && buttonFlag)
  {

    // Reset transmittedFlag (no need to reset buttonFlag since it updates automatically)
    transmittedFlag = false;

    for (int i=0; i < 10; i++)
    {
      Serial.print(F("[SX1278] Sending packet ... "));
      // transmissionState = radio.startTransmit(test, 8);
      transmissionState = radio.startTransmit(TXBuffer, dataLen);

    if (transmissionState == RADIOLIB_ERR_NONE) {
      // Packet was successfully sent
      Serial.println(F("transmission finished!"));
    }
    else {
      Serial.print(F("failed, code "));
      Serial.println(transmissionState);
    }

    // 500 ms delay necessary for proper reception but I forgot why @Ibrahim 
    delay(500);

    // clean up after transmission is finished
    // this will ensure transmitter is disabled,
    // RF switch is powered down etc.
    radio.finishTransmit();
    }
  }
}



/*
Process to transmit data:
1. Create packet header and payload structs, fill in all necessary information
2. Create a byte array, TXBuffer, to hold the encoded packet header and payload information
3. Call encodeHex(TXBuffer, &PacketHeader, &IMUPayload) to parse the header and payload data into the TXBuffer byte array, returning the number of bytes to be transmitted in radio.transmit(TXBuffer, dataLen)
4. Call radio.transmit(TXBuffer, dataLen) to transmit the data in the TXBuffer byte array, where dataLen is the number of bytes to be transmitted as returned by encodeHex()

TXBuffer[256];

encodeHex(TXBuffer, &PacketHeader, &IMUPayload);

TXBuffer -> {first few bytes = packet header, all bytes of payload}

transmit(txbuffer)
*/