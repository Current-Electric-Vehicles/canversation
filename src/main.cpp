#include <Arduino.h>
#include <mcp_can.h>
#include <AsyncTimer.h>
#include "LedManager.h"

#define CAN_CHIP_SELECT_PIN 3
#define CAN_BAUD_RATE CAN_500KBPS
#define CAN_MHZ MCP_20MHZ
#define CAN_EMIT_FREQUENCE_MILLIS 20
#define MAKE_EXT_ID(id) (id | 0x80000000)

LedManager led;
AsyncTimer timer;
MCP_CAN can(CAN_CHIP_SELECT_PIN);
INT8U me((INT8U)random(256));

void readCanMessages();
void emitCanMessage(uint32_t id, uint8_t length);

/**
 * Setup everything
 */
void setup() {
    Serial.begin(9600);
    delay(5000);

    if (can.begin(MCP_ANY, CAN_BAUD_RATE, CAN_MHZ) != CAN_OK) {
        Serial.println("Configuring CAN failed");
        while (true) {
            led.flashError(3);
            delay(1000);
        }
    } else {
        can.setMode(MCP_NORMAL);
        Serial.println("CAN setup successfully");
    }

    timer.setInterval([]() { readCanMessages(); }, 1);
    timer.setInterval([]() { emitCanMessage(MAKE_EXT_ID(0x1800f5e5), 8); }, CAN_EMIT_FREQUENCE_MILLIS);
    timer.setInterval([]() { emitCanMessage(0xf0e, 8); }, CAN_EMIT_FREQUENCE_MILLIS);
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
    if (can.checkReceive() != CAN_MSGAVAIL) {
        return;
    }

    uint8_t inBufferrLen = 0;
    uint8_t inBufferr[8];
    byte msgIsExtended = 0;
    uint32_t msgId = -1;
    if (can.readMsgBuf(&msgId, &msgIsExtended, &inBufferrLen, inBufferr) != CAN_OK) {
        Serial.println("Unable to can.readMsgBuf");
        return;
    }

    if (inBufferrLen >= 1 && inBufferr[0] == me) {
        return;
    }

    Serial.print("IN " ); Serial.print(msgId, HEX);
    for (int i=0; i<inBufferrLen; i++) {
        if (i == 0) {
             Serial.print(":");
        } else {
             Serial.print(",");
        }
        Serial.print(" " ); Serial.print(inBufferr[i], HEX);
    }
    Serial.println("");
}

/**
 * Emits a can message
 */
void emitCanMessage(uint32_t id, uint8_t length) {

    INT8U balls[8] = {
        me,
        (INT8U)random(255),
        (INT8U)random(255),
        (INT8U)random(255),
        (INT8U)random(255),
        (INT8U)random(255),
        (INT8U)random(255),
        (INT8U)random(255)
    };

    can.sendMsgBuf(id, max(1, length), balls);
    led.flashOn(1);
}
