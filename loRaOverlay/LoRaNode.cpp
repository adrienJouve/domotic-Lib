#include "LoRaNode.h"
#include <Arduino.h>

// #define DEBUG

#ifdef DEBUG
#define DEBUG_MSG(x) Serial.println(F(x))
#define DEBUG_MSG_VAR(x) Serial.println(x)
#else
#define DEBUG_MSG(x) // define empty, so macro does nothing
#endif

/**
* LoRaNode Constructor. Empty
*/
LoRaNode::LoRaNode(uint8_t nodeId,
                   unsigned long transmissionTimeInterval,
                   unsigned long processingTimeInterval,
                   bool needTransmissionNow ):
  mNodeId(nodeId),
  mTxCounter(0),
  mTransmissionTimeInterval(transmissionTimeInterval),
  mProcessingTimeInterval(processingTimeInterval),
  mNeedTransmissionNow(needTransmissionNow)
{
}

/**
 * @brief Get the ID of the Node
 * 
 * @return uint8_t the Node Id
 */
uint8_t LoRaNode::getNodeId()
{
  return mNodeId;
}

/**
* Get transmission time interval
* @return transmission time interval in ms. User defined parameter.
*/
unsigned long LoRaNode::getTransmissionTimeInterval()
{
  return mTransmissionTimeInterval;
}

/**
 * @brief Set the Trnasmission time interval of the Node 
 * 
 * @param TimeInterval value in ms
 */
void LoRaNode::setTransmissionTimeInterval(unsigned long timeInterval)
{
  mTransmissionTimeInterval = timeInterval;
}

/**
* Get processing time interval
* @return processing time interval in ms. User defined paramater.
*/
unsigned long LoRaNode::getProcessingTimeInterval()
{
  return mProcessingTimeInterval;
}

/**
 * @brief Set the ProcessingTimeInterval to callback AppProcessing method of the node
 * 
 * @param TimeInterval value in ms
 */
void LoRaNode::setProcessingTimeInterval(unsigned long timeInterval)
{
  mProcessingTimeInterval = timeInterval;
}

/**
 * @brief get the TxCounter value of the Node
 * 
 * @return uint16_t TxCounter of the Node 
 */
uint16_t LoRaNode::getTxCounter()
{
  return mTxCounter;
}

void LoRaNode::incrementTxCounter()
{
  mTxCounter++;
}

bool LoRaNode::getTransmissionNowFlag()
{
  return mNeedTransmissionNow;
}

void LoRaNode::setTransmissionNowFlag(bool flag)
{
  mNeedTransmissionNow = flag;
}
