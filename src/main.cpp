#include <Arduino.h>
#include <CAN.h>
#include <AsyncTimer.h>

#define CAN_BAUD_RATE 500E3

AsyncTimer timer;
uint8_t me;

void readCanMessages();
void emitCanMessage(uint8_t length, long id, bool extended);

/**
 * Setup everything
 */
void setup() {
    Serial.begin(9600);
    delay(5000);

    randomSeed(analogRead(0));
    me = (uint8_t)random(256);

    if (!CAN.begin(CAN_BAUD_RATE)) {
        Serial.println("Configuring CAN failed");
        while (true) {
            delay(1000);
        }
    } else {
        Serial.print("ME: "); Serial.println(me);
        Serial.println("CAN setup successfully");
    }

    timer.setInterval([]() { readCanMessages(); }, 1);
    timer.setInterval([]() { emitCanMessage(8, 0xabcdef, true); }, 20);
    timer.setInterval([]() { emitCanMessage(5, 0xf5, false); }, 200);

}

/**
 * Called by the android subsystem at some frequency.
 */
void loop() {
    timer.handle();
}

/**
 * Read CAN messages
 */
void readCanMessages() {
    int packetSize = CAN.parsePacket();
    if (packetSize == 0) {
        return;
    }

    long id = CAN.packetId();

    int packet[packetSize];
    for (int i = 0; i < packetSize; i++) {
        packet[i] = CAN.read();
    }
    if (packet[0] == me) {
        return;
    }

    Serial.print("IN " ); Serial.print(id, HEX);
    for (int i = 0; i < packetSize; i++) {
        if (i == 0) {
             Serial.print(":");
        } else {
             Serial.print(",");
        }
        Serial.print(" " ); Serial.print(packet[i], HEX);
        i++;
    }
    Serial.println("");
}

/**
 * Emits a can message
 */
void emitCanMessage(uint8_t length, long id, bool extended) {
    if (extended) {
        CAN.beginExtendedPacket(id);
    } else {
        CAN.beginPacket((int)id);
    }

    CAN.write(me);
    for (int i = 1; i < min(8, length); i++) {
        CAN.write((uint8_t)random(255));
    }
    CAN.endPacket();
}
