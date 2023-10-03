#include <Arduino.h>
#include <CAN.h>
#include <AsyncTimer.h>

#define min(a,b) ((a)>(b)?(b):(a))

#define CAN_BAUD_RATE 500E3

AsyncTimer timer;
uint8_t me;

void readCanMessages();
void emitCanMessage(uint8_t length, long id, bool extended);

void modifyRegister(uint8_t address, uint8_t mask, uint8_t value) {
  volatile uint32_t* reg = (volatile uint32_t*)(0x3ff6b000 + address * 4);

  *reg = (*reg & ~mask) | value;
}

/**
 * Setup everything
 */
void setup() {
    Serial.begin(9600);
    delay(5000);
    Serial.println("Hello, lets do this");

    #ifdef ARDUINO_ARCH_ESP32
    pinMode(GPIO_NUM_15, OUTPUT);
    digitalWrite(GPIO_NUM_15, LOW);
    #endif

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
    timer.setInterval([]() { 
        for (int i = 0; i<100; i++) {
            emitCanMessage(8, 0x01 + i, false); 
        }
    }, 100);

    #ifdef ARDUINO_ARCH_ESP32
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    Serial.print("chip revision: "); Serial.println(chip.revision);
    if (chip.revision >= 2) {
        Serial.println("updating CAN register for weird bug");
        modifyRegister(0x04, 0x10, 0); // From rev2 used as "divide BRP by 2"
    }
    #endif
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
    }
    Serial.println("");
}

/**
 * Emits a can message
 */
void emitCanMessage(uint8_t length, long id, bool extended) {
    if (extended) {
        CAN.beginExtendedPacket(id, length);
    } else {
        CAN.beginPacket((int)id, length);
    }

    CAN.write(me);
    for (int i = 1; i < min(8, length); i++) {
        CAN.write((uint8_t)random(255));
    }
    CAN.endPacket();
}
