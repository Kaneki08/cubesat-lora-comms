// include the library
#include <Arduino.h>
#include <RadioLib.h>

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
uint8_t encodeHex(uint8_t* queue, PacketHeader* header, void* payload);



// save transmission state between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate that a packet was sent
volatile bool transmittedFlag = false;

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

void setup() {
  Serial.begin(115200);
  SPI.begin(4, 6, 5, 7);

  // initialize SX1278 with default settings
  Serial.print(F("[SX1278] Initializing ... "));
  int state = radio.begin();
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

  // start transmitting the first packet
  Serial.print(F("[SX1278] Sending first packet ... "));

  // you can transmit C-string or Arduino string up to
  // 255 characters long
  transmissionState = radio.startTransmit("Hello World!");

  // you can also transmit byte array up to 255 bytes long
  /*
    byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                      0x89, 0xAB, 0xCD, 0xEF};
    transmissionState = radio.startTransmit(byteArr, 8);
  */
}

// counter to keep track of transmitted packets
int count = 0;

void loop() {
  // check if the previous transmission finished
  if(transmittedFlag) {
    // reset flag
    transmittedFlag = false;

    if (transmissionState == RADIOLIB_ERR_NONE) {
      // packet was successfully sent
      Serial.println(F("transmission finished!"));

      // NOTE: when using interrupt-driven transmit method,
      //       it is not possible to automatically measure
      //       transmission data rate using getDataRate()

    } else {
      Serial.print(F("failed, code "));
      Serial.println(transmissionState);

    }

    // clean up after transmission is finished
    // this will ensure transmitter is disabled,
    // RF switch is powered down etc.
    radio.finishTransmit();

    // wait a second before transmitting again
    delay(1000);

    // send another one
    Serial.print(F("[SX1278] Sending another packet ... "));

    // you can transmit C-string or Arduino string up to
    // 255 characters long
    String str = "Hello World! #" + String(count++);
    transmissionState = radio.startTransmit(str);

    // you can also transmit byte array up to 255 bytes long
    /*
      byte byteArr[] = {0x01, 0x23, 0x45, 0x67,
                        0x89, 0xAB, 0xCD, 0xEF};
      transmissionState = radio.startTransmit(byteArr, 8);
    */
  }
}
