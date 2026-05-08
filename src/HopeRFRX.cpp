#include <RadioLib.h>
#include <Arduino.h>
#include <stdint.h>
#include <Packet.h>
#include <memory>

Module module(7, 1, 2, 0);
SX1278 radio(&module);



bool receive(uint8_t* buffer, size_t size)
{
    int state = radio.receive(buffer, size, 0);

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
bool parseAcknowledgementPacket(uint8_t* buffer, size_t size, AcknowledgementPacket& packet) {
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
bool acknowledged(uint16_t sequence_number) {
    uint8_t buffer[SIZE_OF_PACKET_ACK] = {};

    if (receive(buffer, sizeof(buffer))) {
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


