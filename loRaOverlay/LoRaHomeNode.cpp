
#include "LoRaHomeNode.h"
#include <LoRa.h>
#include "LoRaNode.h"
#include <ArduinoJson.h>
#include "LoraConfig.h"

#define DEBUG

#ifdef DEBUG
#define DEBUG_MSG_ONELINE(x) Serial.print(F(x))
#define DEBUG_MSG(x) Serial.println(F(x))
#define DEBUG_MSG_VAR(x) Serial.println(x)
#else
#define DEBUG_MSG(x) // define empty, so macro does nothing
#define DEBUG_MSG_VAR(x)
#define DEBUG_MSG_ONELINE(x)
#endif

LoRaHomeNode::LoRaHomeNode(uint8_t nodeId):
  mNodeId(nodeId),
  mTxFrame(MY_NETWORK_ID, nodeId, LH_NODE_ID_GATEWAY, LH_MSG_TYPE_NODE_MSG_ACK_REQ),
  mAckFrame(MY_NETWORK_ID, nodeId, LH_NODE_ID_GATEWAY, LH_MSG_TYPE_NODE_ACK),
  mIsTxAvailable(true),
  mTxRetryCounter(0)
{}

/**
* initialize LoRa communication with #define settings (pins, SD, bandwidth, coding rate, frequency, sync word)
* CRC is enabled
* set in Rx Mode by default
*/
void LoRaHomeNode::setup()
{
  DEBUG_MSG("LoRaHomeNode::setup");
  //setup LoRa transceiver module
  DEBUG_MSG("--- LoRa Begin");
  DEBUG_MSG_VAR(LORA_FREQUENCY);

  while (!LoRa.begin(LORA_FREQUENCY))
  {
    DEBUG_MSG_ONELINE(".");
    delay(500);
  }
  LoRa.setPins(SS, RST, DIO0);
  DEBUG_MSG("--- setSpreadingFactor");
  DEBUG_MSG_VAR(LORA_SPREADING_FACTOR);
  LoRa.setSpreadingFactor(LORA_SPREADING_FACTOR);
  DEBUG_MSG("--- setSignalBandwidth");
  DEBUG_MSG_VAR(LORA_SIGNAL_BANDWIDTH);
  LoRa.setSignalBandwidth(LORA_SIGNAL_BANDWIDTH);
  DEBUG_MSG("--- setCodingRate");
  DEBUG_MSG_VAR(LORA_CODING_RATE_DENOMINATOR);
  LoRa.setCodingRate4(LORA_CODING_RATE_DENOMINATOR);
  DEBUG_MSG("--- setSyncWord");
  DEBUG_MSG_VAR(LORA_SYNC_WORD);
  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(LORA_SYNC_WORD);
  DEBUG_MSG("--- enableCrc");
  LoRa.enableCrc();
  
  // set in rx mode.
  this->rxMode();
}

/** 
 * Send a message to the LoRa2MQTT gateway
 * @param payload the JSON payload to be sent
 */
void LoRaHomeNode::sendToGateway(const JsonDocument& payload, uint8_t txCounter)
{
  // DEBUG_MSG("LoRaHomeNode::sendToGateway()");
  if(false == mIsTxAvailable){
    DEBUG_MSG("--- Tx not available");
    return;
  }

  mIsTxAvailable = false;
  mTxRetryCounter = 0;
  
  // DEBUG_MSG("--- create LoraHomeFrame");
  // create frame
  mTxFrame.setCounter(txCounter);
  // create payload
  // DEBUG_MSG("--- create LoraHomePayload");
  JsonDocument jsonDoc = payload;;
  jsonDoc[MSG_SNR] = LoRa.packetSnr();
  jsonDoc[MSG_RSSI] = LoRa.packetRssi();
  
  mTxFrame.setPayload(jsonDoc);

  send(mTxFrame, LH_FRAME_MAX_SIZE);
  mTxRetryCounter++;
}

/**
 * @brief Retries sending a message to the gateway.
 * 
 * This method is called when an acknowledgment (ack) for the message is not received.
 * It checks if the maximum number of retries without a valid ack has been reached.
 * If the maximum retry limit is reached, it sets the transmission availability flag to true and returns.
 * Otherwise, it calls the send() method to resend the message and increments the retry counter.
 */
void LoRaHomeNode::retrySendToGateway()
{
  // Can't received ack for this message, so skip it to enable next message
  if(MAX_RETRY_NO_VALID_ACK <= mTxRetryCounter){
    DEBUG_MSG("--- Max retry reached");
    DEBUG_MSG_VAR(mTxFrame.getCounter());
    DEBUG_MSG_ONELINE(" -> Send FAILLURE");
    mIsTxAvailable = true;
    return;
  }

  send(mTxFrame, LH_FRAME_MAX_SIZE);
  mTxRetryCounter++;
}

/**
* [receiveLoraMessage description]
*/
bool LoRaHomeNode::receiveLoraMessage(JsonDocument& payload)
{
  //try to parse packet
  int packetSize = LoRa.parsePacket();

  // return immediately if no message available
  if (0 >= packetSize)
  {
    return false;
  }
  // check if we can accept the message
  if ((packetSize > LH_FRAME_MAX_SIZE)
      || (packetSize < LH_FRAME_MIN_SIZE))
  {
    flushLoRaFifo();
    return false;
  }

  DEBUG_MSG("LoRaHomeNode::receiveLoraMessage");

  // read available bytes
  uint8_t rxMessage[LH_FRAME_MAX_SIZE];
  uint8_t msgSize(0);

  for (msgSize = 0; msgSize < packetSize; msgSize++)
  {
    // read available bytes
    rxMessage[msgSize] = (char)LoRa.read();
  }
  // create LoRa Home frame
  LoRaHomeFrame rxFrame;
  bool noError = rxFrame.createFromRxMessage(rxMessage, msgSize, true);

  if (false == noError)
  {
    DEBUG_MSG("--- bad message received");
    return false;
  }

  // check if the message is for me
  if (rxFrame.getNetworkID() != MY_NETWORK_ID)
  {
    DEBUG_MSG("--- ignore message, not the right network ID");
    return false;
  }

  DEBUG_MSG("--- message received");
  // rxFrame.print();

  // Handle ack message
  if(mNodeId == rxFrame.getNodeIdRecipient()
     && (rxFrame.getMessageType() == LH_MSG_TYPE_GW_ACK)
     && (rxFrame.getNodeIdEmitter() == LH_NODE_ID_GATEWAY))
  {

      if(mTxFrame.getCounter() == rxFrame.getCounter()) {
        mIsTxAvailable = true;
        mTxFrame.clear();

        DEBUG_MSG_ONELINE("--- ack received for Tx counter: ");
        DEBUG_MSG_VAR(rxFrame.getCounter());
        DEBUG_MSG(" -> Send SUCCESS");
        return false;
      }else {
        DEBUG_MSG_ONELINE("--- ack received but not for this message, ack counter: ");
        DEBUG_MSG_VAR(rxFrame.getCounter());
      }
  }

  // Am I the node invoked for this messages
  if (mNodeId == rxFrame.getNodeIdRecipient())
  {
    // deserializeJson error
    DeserializationError error = deserializeJson(payload, rxFrame.getPayload());
    
    if (error)
    {
      DEBUG_MSG("--- deserializeJson error");
      return false;
    }
    // if message received request an ack
    if ((rxFrame.getMessageType() == LH_MSG_TYPE_GW_MSG_ACK) || (rxFrame.getMessageType() == LH_MSG_TYPE_NODE_MSG_ACK_REQ))
    {
      mAckFrame.setNodeIdRecipient(rxFrame.getNodeIdEmitter());
      mAckFrame.setCounter(rxFrame.getCounter());

      send(mAckFrame, LH_FRAME_MIN_SIZE);
      DEBUG_MSG("--- ack sent");
    }
  }
  else
  {
    DEBUG_MSG("--- ignore message, not for me");
    return false;
  }

  return true;
}

/**
 * Returns the interval for retrying to send a message.
 *
 * @return The interval for retrying to send a message in milliseconds.
 */
unsigned long LoRaHomeNode::getRetrySendMessageInterval()
{
  return ACK_TIMEOUT;
}

/**
 * Send a message to the LoRa2MQTT gateway
 */
void LoRaHomeNode::send(LoRaHomeFrame& frame, uint8_t bufferSize)
{
  // DEBUG_MSG("LoRaHomeNode::send");
  // DEBUG_MSG("--- sending LoRa message to LoRa2MQTT gateway");

  uint8_t txBuffer[bufferSize];
  uint8_t size = frame.serialize(txBuffer);
  // DEBUG_MSG("--- LoraHomeFrame serialized");

  this->txMode();
  LoRa.beginPacket();
  for (uint8_t i = 0; i < size; i++)
  {
    LoRa.write(txBuffer[i]);
    // DEBUG_MSG_VAR(txBuffer[i]);
  }
  LoRa.endPacket();
  this->rxMode();
}

/**
* Set Node in Rx Mode with active invert IQ
* LoraWan principle to avoid node talking to each other
* This way a Gateway only reads messages from Nodes and never reads messages from other Gateway, and Node never reads messages from other Node.
*/
void LoRaHomeNode::rxMode()
{
  LoRa.enableInvertIQ(); // active invert I and Q signals
  LoRa.receive();        // set receive mode
}

/**
* Set Node in Tx Mode with active invert IQ
* LoraWan principle to avoid node talking to each other
* This way a Gateway only reads messages from Nodes and never reads messages from other Gateway, and Node never reads messages from other Node.
*/
void LoRaHomeNode::txMode()
{
  LoRa.idle();            // set standby mode
  LoRa.disableInvertIQ(); // normal mode
}


/**
 * @brief Flushes the LoRa FIFO by reading and discarding all available data.
 * 
 * This function continuously reads data from the LoRa module until the FIFO is empty.
 * It is useful to clear the FIFO before sending or receiving new data.
 */
void LoRaHomeNode::flushLoRaFifo()
{
  while (LoRa.available())
  {
    LoRa.read();
  }
}