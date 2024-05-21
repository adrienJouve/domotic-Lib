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
    bool sendToGateway(const JsonDocument& payload);
    void retrySendToGateway();
    bool receiveLoraMessage(JsonDocument& payload);
    unsigned long getRetrySendMessageInterval();
    inline bool isWaitingForAck() { return !mIsTxAvailable; };
    inline uint16_t getTxCounter() { return mTxCounter; };

protected:
    void send(LoRaHomeFrame& frame, uint8_t bufferSize);
    void rxMode();
    void txMode();
    void flushLoRaFifo();
    inline void incrementTxCounter() { mTxCounter++; };

    uint8_t mNodeId;
    LoRaHomeFrame mTxFrame;
    LoRaHomeFrame mAckFrame;
    bool mIsTxAvailable;
    uint8_t mTxRetryCounter;
    uint16_t mTxCounter;
};

#endif