#include "LoRaHomeFrame.h"

// #define DEBUG

#ifdef DEBUG
#define DEBUG_MSG_ONELINE(x) Serial.print(F(x))
#define DEBUG_MSG(x) Serial.println(F(x))
#define DEBUG_MSG_VAR(x) Serial.println(x)
#else
#define DEBUG_MSG_ONELINE(x)
#define DEBUG_MSG(x) // define empty, so macro does nothing
#define DEBUG_MSG_VAR(x)
#endif

/**
 * @brief Construct a new LoRaHomeFrame:: LoRaHomeFrame object
 * Usefull when receiving a frame
 *
 */
LoRaHomeFrame::LoRaHomeFrame():
    mNetworkID(0),
    mNodeIdEmitter(0),
    mNodeIdRecipient(0),
    mMessageType(0),
    mCounter(0)
{
    mJsonPayload[0] = '\0';
}

/**
 * @brief Constructs a new LoRaHomeFrame object.
 *
 * This constructor initializes a LoRaHomeFrame object with the specified parameters.
 *
 * @param networkID The network ID of the frame.
 * @param nodeIdEmitter Our node Id to identify message sent to gateway.
 * @param nodeIdRecipient Node ID of recipient.
 * @param messageType The type of the message.
 */
LoRaHomeFrame::LoRaHomeFrame(uint16_t networkID,
                             uint8_t nodeIdEmitter,
                             uint8_t nodeIdRecipient,
                             uint8_t messageType):
    mNetworkID(networkID),
    mNodeIdEmitter(nodeIdEmitter),
    mNodeIdRecipient(nodeIdRecipient),
    mMessageType(messageType),
    mCounter(0)
{
    mJsonPayload[0] = '\0';
}

/**
 * @brief Set the TxCounter object
 *
 * @param counter
 */
void LoRaHomeFrame::setCounter(uint16_t counter){
    mCounter = counter;
}

/**
 * @brief Set the Payload object
 *
 * @param payload
 */
void LoRaHomeFrame::setPayload(const JsonDocument& payload){
    serializeJson(payload, mJsonPayload, LH_FRAME_MAX_PAYLOAD_SIZE);
}

/**
 * @brief Clears the LoRaHomeFrame object by setting the payload to an empty JSON document.
 * 
 * This method clears the LoRaHomeFrame object by creating a new empty JSON document and setting it as the payload.
 * After calling this method, the payload will be an empty JSON document.
 */
void LoRaHomeFrame::clear(){
    JsonDocument payload;
    setPayload(payload);
}

/**
 * @brief serialize a LoRaHomeFrame into the given txBuffer
 * the size of the txBuffer shall be large enough to welcome the LoRaHomeFrame
 *
 * @param txBuffer
 * @return uint8_t
 */
uint8_t LoRaHomeFrame::serialize(uint8_t* txBuffer)
{
    DEBUG_MSG("LoRaHomeFrame::serialize");
    txBuffer[LH_FRAME_INDEX_EMITTER] = this->mNodeIdEmitter;
    txBuffer[LH_FRAME_INDEX_RECIPIENT] = this->mNodeIdRecipient;
    txBuffer[LH_FRAME_INDEX_MESSAGE_TYPE] = this->mMessageType;
    txBuffer[LH_FRAME_INDEX_NETWORK_ID] = (uint8_t)(this->mNetworkID & 0xff);
    txBuffer[LH_FRAME_INDEX_NETWORK_ID + 1] = (uint8_t)((this->mNetworkID >> 8)) & 0xff;
    txBuffer[LH_FRAME_INDEX_COUNTER] = (uint8_t)(this->mCounter & 0xff);
    txBuffer[LH_FRAME_INDEX_COUNTER + 1] = (uint8_t)((this->mCounter >> 8)) & 0xff;
    uint8_t payloadSize = strlen(this->mJsonPayload);
    txBuffer[LH_FRAME_INDEX_PAYLOAD_SIZE] = payloadSize;
    if (payloadSize > 0)
    {
        memcpy((char*)&txBuffer[LH_FRAME_INDEX_PAYLOAD], this->mJsonPayload, payloadSize);
    }
    this->mCrc16 = crc16_ccitt(txBuffer, LH_FRAME_HEADER_SIZE + payloadSize);
    txBuffer[LH_FRAME_HEADER_SIZE + payloadSize + LH_FRAME_FOOTER_SIZE - 2] = this->mCrc16 & 0xff;
    txBuffer[LH_FRAME_HEADER_SIZE + payloadSize + LH_FRAME_FOOTER_SIZE - 1] = (this->mCrc16 >> 8) & 0xff;
    return LH_FRAME_HEADER_SIZE + payloadSize + LH_FRAME_FOOTER_SIZE;
}

/**
 * @brief Create a LoRaHomeFrame from a raw bytes message
 *
 * @param rawBytesWithCRC raw bytes message with CRC included
 * @param length length of the message (number of bytes)
 * @param checkCRC indicate whether the CRC should be checked or not
 *
 * @return true
 * @return false
 */
bool LoRaHomeFrame::createFromRxMessage(uint8_t* rawBytesWithCRC, uint8_t length, bool checkCRC)
{
    DEBUG_MSG("LoRaHomeFrame::createFromRxMessage");
    if (checkCRC)
    {
        if (!this->checkCRC(rawBytesWithCRC, length))
        {
            return false;
        }
    }
    // TODO should check whether data are valid or not?
    this->mNetworkID = rawBytesWithCRC[LH_FRAME_INDEX_NETWORK_ID] | (rawBytesWithCRC[LH_FRAME_INDEX_NETWORK_ID + 1] << 8);
    this->mNodeIdEmitter = rawBytesWithCRC[LH_FRAME_INDEX_EMITTER];
    this->mNodeIdRecipient = rawBytesWithCRC[LH_FRAME_INDEX_RECIPIENT];
    this->mMessageType = rawBytesWithCRC[LH_FRAME_INDEX_MESSAGE_TYPE];
    this->mCounter = rawBytesWithCRC[LH_FRAME_INDEX_COUNTER] | (rawBytesWithCRC[LH_FRAME_INDEX_COUNTER + 1] << 8);
    this->mPayloadSize = rawBytesWithCRC[LH_FRAME_INDEX_PAYLOAD_SIZE];
    if (this->mPayloadSize > LH_FRAME_MAX_PAYLOAD_SIZE)
    {
        DEBUG_MSG("--- invalid payload size");
        return false;
    }
    // copy the json payload if any
    if (this->mPayloadSize != 0)
    {
        memcpy(this->mJsonPayload, &rawBytesWithCRC[LH_FRAME_INDEX_PAYLOAD], rawBytesWithCRC[LH_FRAME_INDEX_PAYLOAD_SIZE]);
        this->mJsonPayload[this->mPayloadSize] = '\0';
    }
    return true;
}

/**
 * @brief
 *
 * @param rawBytesWithCRC
 * @param length
 * @return true
 * @return false
 */
bool LoRaHomeFrame::checkCRC(uint8_t* rawBytesWithCRC, uint8_t length)
{
    DEBUG_MSG("LoRaHomeFrame::checkCRC");
    // check if packet is potentially valid. At least 11 bytes
    if (length < LH_FRAME_MIN_SIZE)
    {
        DEBUG_MSG("--- bad packet received too small");
        return false;
    }
    if (length > LH_FRAME_MAX_SIZE)
    {
        DEBUG_MSG("--- bad packet received too big");
        return false;
    }
    // check CRC - last 2 bytes should contain CRC16
    uint8_t lowCRC = rawBytesWithCRC[length - 2];
    uint8_t highCRC = rawBytesWithCRC[length - 1];
    uint16_t rx_crc16 = lowCRC | (highCRC << 8);
    // compute CRC16 without the last 2 bytes
    uint16_t crc16 = crc16_ccitt(rawBytesWithCRC, length - 2);
    // if CRC16 not valid, ignore LoRa message
    if (rx_crc16 != crc16)
    {
        DEBUG_MSG("--- CRC Error");
        return false;
    }
    DEBUG_MSG("--- valid CRC");
    return true;
}

/**
 * @brief compute CRC16 ccitt
 *
 * @param data data to be used to compute CRC16
 * @param data_len length of the buffer
 * @return uint16_t
 */
uint16_t LoRaHomeFrame::crc16_ccitt(uint8_t* data, unsigned int data_len)
{
    uint16_t crc = 0xFFFF;

    if (data_len == 0)
        return 0;

    for (unsigned int i = 0; i < data_len; ++i)
    {
        uint16_t dbyte = data[i];
        crc ^= dbyte << 8;

        for (unsigned char j = 0; j < 8; ++j)
        {
            uint16_t mix = crc & 0x8000;
            crc = (crc << 1);
            if (mix)
                crc = crc ^ 0x1021;
        }
    }
    return crc;
}

void LoRaHomeFrame::print()
{
    DEBUG_MSG("LoRaHomeFrame::print");
    DEBUG_MSG_ONELINE("NetworkID: ");
    DEBUG_MSG_VAR(this->mNetworkID);
    DEBUG_MSG_ONELINE("NodeIdEmitter: ");
    DEBUG_MSG_VAR(this->mNodeIdEmitter);
    DEBUG_MSG_ONELINE("NodeIdRecipient: ");
    DEBUG_MSG_VAR(this->mNodeIdRecipient);
    DEBUG_MSG_ONELINE("MessageType: ");
    DEBUG_MSG_VAR(this->mMessageType);
    DEBUG_MSG_ONELINE("Counter: ");
    DEBUG_MSG_VAR(this->mCounter);
    DEBUG_MSG_ONELINE("Payload: ");
    DEBUG_MSG_VAR(this->mJsonPayload);
}