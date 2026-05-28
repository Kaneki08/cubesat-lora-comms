#include <RadioLib.h>
#include <Arduino.h>
#include <stdint.h>
#include "Packet.h"
#include "HopeRFRX.h"

bool receive(SX1278& radio, uint8_t* buffer, size_t size){
    int state = radio.receive(buffer, size, 3000);

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
        Serial.println("ACK receive timeout");
        return false;
    }
    else {
        Serial.print("receive failed: ");
        Serial.println(state);
        return false;
    }
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
bool acknowledged(SX1278& radio, uint16_t sequence_number) {
    uint8_t buffer[SIZE_OF_PACKET_ACK] = {};

    if (receive(radio, buffer, sizeof(buffer))) {
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
    uint8_t buffer[sizeof(StringPayload)];
    if (receive(radio, buffer, sizeof(buffer))) {
        StringPayload* response = (StringPayload*)buffer;
        if (strcmp(response->message, "ZOT ZOT ZOT from GS") == 0) {
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

