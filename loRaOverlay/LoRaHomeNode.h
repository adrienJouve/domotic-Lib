#ifndef LORAHOMENODE_H
#define LORAHOMENODE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <loRaOverlay/LoRaHomeFrame.h>

class LoRaHomeNode
{
public:
    LoRaHomeNode(uint8_t nodeId);
    virtual ~LoRaHomeNode() = default;

    void setup();
    void sendToGateway(const JsonDocument& payload, uint8_t txCounter);
    void retrySendToGateway();
    bool receiveLoraMessage(JsonDocument& payload);
    unsigned long getRetrySendMessageInterval();
    inline bool isWaitingForAck() { return !mIsTxAvailable; };

protected:
    void send(LoRaHomeFrame& frame, uint8_t bufferSize);
    void rxMode();
    void txMode();
    void flushLoRaFifo();

    uint8_t mNodeId;
    LoRaHomeFrame mTxFrame;
    LoRaHomeFrame mAckFrame;
    bool mIsTxAvailable;
    uint8_t mTxRetryCounter;
};

#endif