#ifndef HOPERFTX_H
#define HOPERFTX_H

#include <stdint.h>
#include <string.h>

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
/*
uint16_t celltemp1;
uint16_t celltemp2;
uint16_t celltemp3;
uint16_t celltemp4;
uint16_t celltemp5;
uint16_t celltemp6;
uint16_t celltemp7;
uint16_t celltemp8;
uint16_t temperature;
uint16_t board_temperature;
uint16_t temp_range;
std::string gas_gauge_firmware_version;
uint16_t voltage;
int16_t current;
int16_t average_current;
uint8_t relative_state_of_charge;
uint8_t absolute_state_of_charge;
uint16_t remaining_capacity;
uint16_t full_charge_capacity;
uint16_t run_time_to_empty;
uint16_t average_time_to_empty;
uint16_t average_time_to_full;
uint16_t charging_current; 
uint16_t charging_voltage;
struct battery_status {
  uint8_t high_byte;
  uint8_t low_byte;
} battery_status;
uint16_t cycle_count;
int16_t design_capacity;
int16_t design_voltage;
struct safety_alert_registers {
  uint8_t safety_alert_high_byte;
  uint8_t safety_alert_low_byte;
  uint8_t safety_alert2_high_byte;
  uint8_t safety_alert2_low_byte;
}safety_alert_registers;
struct safety_status_registers {
  uint8_t safety_status_high_byte;
  uint8_t safety_status_low_byte;
  uint8_t safety_status2_high_byte;
  uint8_t safety_status2_low_byte;
} safety_status_registers;
struct charging_status_registers {
  uint8_t charging_status_high_byte;
  uint8_t charging_status_low_byte;
  uint8_t charging_status2_high_byte;
  uint8_t charging_status2_low_byte;
} charging_status_registers;
struct pfalert_registers {
  uint8_t pfalert_high_byte;
  uint8_t pfalert_low_byte;
  uint8_t pfalert2_high_byte;
  uint8_t pfalert2_low_byte;
}pfalert_registers;
struct pf_status_registers {
  uint8_t pf_status_high_byte;
  uint8_t pf_status_low_byte;
  uint8_t pf_status2_high_byte;
  uint8_t pf_status2_low_byte;
}pf_status_registers;
uint16_t last_minimum_pack_voltage;
uint16_t last_maximum_pack_voltage;
uint8_t approximate_state_of_charge;
uint16_t current_compensated_voltage;
uint8_t supMCU_status_register;
std::string time_of_last_permanent_failure;
struct registers_for_last_pf {
  uint8_t pfalert_high_byte;
  uint8_t pfalert_low_byte;
  uint8_t pfalert2_high_byte;
  uint8_t pfalert2_low_byte;
}registers_for_last_pf;
// data from requested SMB read
// data from requested flash entry
// data from requested MA entry
uint8_t data_from_last_function_call[8];
uint16_t cell_voltage_4;
uint16_t cell_voltage_3;
uint16_t cell_voltage_2;
uint16_t cell_voltage_1;
uint8_t status_of_cell_balancing_system;
uint8_t data_from_requested_flash_page[32];
// operation status register
uint16_t pack_voltage;
uint16_t average_voltage;
*/

struct combined_telemetry_1{
  int16_t current;
  int16_t avg_current;
  uint16_t voltage;
  uint16_t average_voltage;
  uint16_t cycle_count;
  uint16_t temperature;
  uint16_t external_temp_sensor1;
  uint16_t external_temp_sensor2;
  uint16_t external_temp_sensor3;
  uint16_t external_temp_sensor4;
  uint16_t external_temp_sensor5;
  uint16_t external_temp_sensor6;
  uint16_t external_temp_sensor7;
  uint16_t external_temp_sensor8;
  uint16_t cell_voltage1;
  uint16_t cell_voltage2;
  uint16_t cell_voltage3;
  uint16_t cell_voltage4;
} combined_telemetry_1;



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