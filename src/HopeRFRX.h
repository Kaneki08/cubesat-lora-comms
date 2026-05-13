#ifndef HOPERFRX_H
#define HOPERFRX_H

#pragma once

#include <RadioLib.h>
#include <Arduino.h>
#include <stdint.h>
#include "Packet.h"

// Receives a packet into buffer over radio.
// Returns true if packet passed CRC, false otherwise.
bool receive(SX1278& radio, uint8_t* buffer, size_t size);

// Parses a raw buffer into an AcknowledgementPacket struct.
// Assumes little endian byte order.
// Returns false if buffer is too small or packet type is not ACK.
bool parseAcknowledgementPacket(uint8_t* buffer, size_t size, AcknowledgementPacket& packet);

// Returns true if an ACK packet matching sequence_number was received.
bool acknowledged(SX1278& radio, uint16_t sequence_number);

// listens for handshake message from GS
// Returns true if handshake packet sends "ZOT ZOT ZOT from GS"
bool receiveHandshake(SX1278& radio);

#endif // HOPERFRX_H