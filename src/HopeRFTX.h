#pragma once

// Encodes header and payload into a byte array for transmission
// Returns number of bytes written, or 0 on error
uint8_t encodeHex(uint8_t* queue, size_t queue_size, const PacketHeader* header, const void* payload);

// Returns true if button on given GPIO was pressed (with 250ms debounce)
bool debounceRead(int gpio);

// Transmits a packet with the given type and payload over radio
void transmit(SX1278& radio, uint8_t packet_type, void* payload, uint8_t payload_size, uint16_t& count);
