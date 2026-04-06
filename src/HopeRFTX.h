#ifndef HOPERFTX_H
#define HOPERFTX_H

#include <stdint.h>

// MACROS
#define BUTTON_GPIO 10

// Packet type definitions
enum PacketType
{
  PACKET_BATT = 1,  // battery data packet
  PACKET_IMU = 2,  // imu data packet
  PACKET_TC = 3,  // thermocouple data packet
};

// Packet frame structuring with tight byte packing 
#pragma pack(push, 1)

struct PacketHeader
{
  // Based on CCSDS
  uint8_t sat_id;         // callsign??
  uint8_t packet_type;    // bitfield
  uint16_t sequence;      // packet order since transmission cycle start
  uint32_t timestamp;     // since program start
  uint8_t payload_len;    // in bytes
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

// This data comes is read from the power board and then sent to avionics stm32
struct BATTPayload
{
 // TODO: add battery data fields
};

struct TCPayload
{
 float tc_avg1;
 float tc_avg2;
 // TODO: verify later with avionics that we are using two averaged values 
};

#pragma pack(pop)

// Function prototypes
int transmitData();
uint8_t encodeHex(uint8_t* queue, PacketHeader* header, void* payload);
bool debounceRead(int gpio);

#endif // HOPERFTX_H