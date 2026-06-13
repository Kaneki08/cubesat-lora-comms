#include <RadioLib.h>
#include <Arduino.h>
#include <stdint.h>
#include "Packet.h"
#include "HopeRFRX.h"



bool receive(SX1278& radio, uint8_t* buffer, size_t size, int timeout_ms){
	int state = radio.receive(buffer, size, timeout_ms);

	if (state == RADIOLIB_ERR_NONE) {
		// packet passed CRC
		Serial.println("valid packet");
		return true;
	}
	else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
		// packet was received but CRC failed
		Serial.println("bad packet / corrupted");
		return false;
	}
	else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
		Serial.println("receive timeout");
		return false;
	}
	else {
		Serial.print("receive failed: ");
		Serial.println(state);
		return false;
	}
}

bool receive(SX1278& radio, uint8_t* buffer, size_t size){
	return receive(radio, buffer, size, 3000);
}

// ASSUMES LITTLE ENDIAN
bool parseAcknowledgementPacket(uint8_t* buffer, size_t size, AcknowledgementPacket &packet) {
    if (size < 3) {
        Serial.println("ACK packet too small");
        return false;
    }

    packet.packet_type = buffer[0];

    packet.sequence =
        (uint16_t)buffer[1] |
        ((uint16_t)buffer[2] << 8);

    if (packet.packet_type != PACKET_ACK) {
        Serial.println("Received packet is not ACK");
        return false;
    }

    return true;
}

// returns true if the acknowledgement packet with sequence_number was receieved
bool acknowledged(SX1278& radio, uint16_t sequence_number, int timeout_ms) {
    uint8_t buffer[SIZE_OF_PACKET_ACK] = {};

    if (receive(radio, buffer, sizeof(buffer), timeout_ms)) {
        AcknowledgementPacket packet{};

        if (!parseAcknowledgementPacket(buffer, SIZE_OF_PACKET_ACK, packet)) {
            return false;
        }

        if (packet.sequence == sequence_number) {
            return true;
        }

        Serial.println("ACK sequence mismatch");
    }

    return false;
}

// listens for handshake message from GS
// Returns true if handshake packet sends "ZOT ZOT ZOT from GS"
bool receiveHandshake(SX1278& radio) {
    const unsigned long deadline = millis() + 60000UL;
    const char expected[] = "ZOT ZOT ZOT from GS";

    while ((long)(deadline - millis()) > 0) {
        uint8_t buffer[sizeof(StringPayload)] = {};
        int remaining_ms = (int)(deadline - millis());

        if (!receive(radio, buffer, sizeof(buffer), remaining_ms)) {
            continue;
        }

        StringPayload* response = reinterpret_cast<StringPayload*>(buffer);
        response->message[sizeof(response->message) - 1] = '\0';

        if (strcmp(response->message, expected) == 0) {
            return true;
        }

        Serial.println("unexpected handshake response");
    }

    return false;
}


/*
Test function to listen for Hello LoRa
*/

void printRXBuffer(uint8_t* buffer, int len) {    
    Serial.print("Raw buffer: ");
    for (int i = 0; i < len ; i++) {
        Serial.print(buffer[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    char str[len+1];
    memcpy(str, buffer, len);
    str[len] = '\0';
    Serial.println(str);

}

