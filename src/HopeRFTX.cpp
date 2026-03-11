// include the library
#include <Arduino.h>
#include <RadioLib.h>

// MACROS
#define BUTTON_GPIO 10

// Initialize HopeRF
Module module(7, 1, 2, 0);
SX1278 radio(&module);

enum PacketType
{
  PACKET_BATT = 1,  // battery data packet
  PACKET_IMU = 2,  // imu data packet
  PACKET_TC = 3,  // thermocouple data packet
};

// Packet frame structuring
#pragma pack(push, 1)
struct PacketHeader
{
  // Based on CCSDS
  uint8_t sat_id;  // callsign??
  uint8_t packet_type;  // bitfield
  uint16_t sequence;  // packet order since transmission cycle start
  uint32_t timestamp;  // since program start
  uint8_t payload_len;  // in bytes
};


/*
Expand out the following payloads with all necessary info according to https://docs.google.com/spreadsheets/d/13qsSh9g_9VDnHY6Gt52Ryx6YPZemS3kZyaQAJXoeYj8/edit?usp=sharing
*/
struct IMUPayload
{
  int16_t gx_dps;
  int16_t gy_dps;
  int16_t gz_dps;

  int16_t ax_mg;
  int16_t ay_mg;
  int16_t az_mg;

  int16_t qi;
  int16_t qj;
  int16_t qk;
  int16_t qr;
};

struct BATTPayload
{
  
};

struct TCPayload
{

};
#pragma pack(pop)

// Place custom function prototypes:
int transmitData();
uint8_t encodeHex(uint8_t* queue, PacketHeader* header, void* payload);
bool debounceRead(int gpio);

byte test[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate that a packet was sent
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

void loop() {
  bool buttonFlag = debounceRead(BUTTON_GPIO);

  // check if the previous transmission finished
  if(transmittedFlag && buttonFlag)
  {

    // reset transmittedFlag (no need to reset buttonFlag since it updates automatically)
    transmittedFlag = false;

    for (int i=0; i < 10; i++)
    {
      Serial.print(F("[SX1278] Sending packet ... "));
    transmissionState = radio.startTransmit(test, 8);

    if (transmissionState == RADIOLIB_ERR_NONE) {
      // packet was successfully sent
      Serial.println(F("transmission finished!"));
    }
    else {
      Serial.print(F("failed, code "));
      Serial.println(transmissionState);
    }


    delay(500);

    // clean up after transmission is finished
    // this will ensure transmitter is disabled,
    // RF switch is powered down etc.
    radio.finishTransmit();
    }
  }
}