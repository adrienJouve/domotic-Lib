#ifndef LORAHOMEFRAME_H
#define LORAHOMEFRAME_H

#include <Arduino.h>
#include <ArduinoJson.h>

const uint8_t LH_FRAME_HEADER_SIZE = 8;
const uint8_t LH_FRAME_FOOTER_SIZE = 2; // only CRC for now. TODO: add security
const uint8_t LH_FRAME_MAX_PAYLOAD_SIZE = 128;
const uint8_t LH_FRAME_MIN_SIZE = LH_FRAME_HEADER_SIZE + LH_FRAME_FOOTER_SIZE;
const uint8_t LH_FRAME_ACK_SIZE = LH_FRAME_HEADER_SIZE + LH_FRAME_FOOTER_SIZE;
const uint8_t LH_FRAME_MAX_SIZE = LH_FRAME_HEADER_SIZE + LH_FRAME_FOOTER_SIZE + LH_FRAME_MAX_PAYLOAD_SIZE;

const uint8_t LH_FRAME_INDEX_EMITTER = 0;
const uint8_t LH_FRAME_INDEX_RECIPIENT = 1;
const uint8_t LH_FRAME_INDEX_MESSAGE_TYPE = 2;
const uint8_t LH_FRAME_INDEX_NETWORK_ID = 3;
const uint8_t LH_FRAME_INDEX_COUNTER = 5;
const uint8_t LH_FRAME_INDEX_PAYLOAD_SIZE = 7; // 2 bytes
const uint8_t LH_FRAME_INDEX_PAYLOAD = 8;

const uint8_t LH_NODE_ID_GATEWAY = 0x00;
const uint8_t LH_NODE_ID_BROADCAST = 0xFF;

const uint8_t LH_MSG_TYPE_NODE_MSG_ACK_REQ = 0x01;
const uint8_t LH_MSG_TYPE_NODE_MSG_NO_ACK_REQ = 0x00;
// TODO not used. should be removed?
const uint8_t LH_MSG_TYPE_GW_MSG_NO_ACK = 0x02;
const uint8_t LH_MSG_TYPE_GW_MSG_ACK = 0x03;

const uint8_t LH_MSG_TYPE_NODE_ACK = 0x04;
const uint8_t LH_MSG_TYPE_GW_ACK = 0x06;

class LoRaHomeFrame
{
public:
    LoRaHomeFrame();
    LoRaHomeFrame(uint16_t networkID, uint8_t nodeIdEmitter, uint8_t nodeIdRecipient, uint8_t messageType);
    virtual ~LoRaHomeFrame() = default;

    void setCounter(uint16_t counter);
    uint16_t getCounter() const { return mCounter; }
    void setPayload(const JsonDocument& payload);
    const char* getPayload() const { return mJsonPayload; }

    void clear();

    uint8_t serialize(uint8_t *txBuffer);
    bool createFromRxMessage(uint8_t *rawBytesWithCRC, uint8_t length, bool checkCRC);

    uint8_t getNodeIdEmitter() const { return mNodeIdEmitter; }
    inline uint16_t getNetworkID() { return mNetworkID; };
    void setNodeIdRecipient(uint8_t nodeIdRecipient) { mNodeIdRecipient = nodeIdRecipient; }
    inline uint8_t getNodeIdRecipient() { return mNodeIdRecipient; };
    uint8_t getMessageType() const { return mMessageType; }

    bool checkCRC(uint8_t *rawBytesWithCRC, uint8_t length);

    void print();
    
private:
    uint16_t crc16_ccitt(uint8_t *data, unsigned int data_len);

protected:
    uint16_t mNetworkID;
    uint8_t mNodeIdEmitter;
    uint8_t mNodeIdRecipient;
    uint8_t mMessageType;
    uint16_t mCounter;
    uint8_t mPayloadSize;
    uint8_t mAes_IV;
    uint16_t mCrc16;
    char mJsonPayload[LH_FRAME_MAX_PAYLOAD_SIZE];
};

#endif