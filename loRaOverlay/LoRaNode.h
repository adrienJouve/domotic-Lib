#ifndef LORANODE_H
#define LORANODE_H

#include <ArduinoJson.h>
#include <Arduino.h>

#define ARDUINO_NANO_BOARD

class LoRaNode
{
public:

  LoRaNode(uint8_t nodeId,
           unsigned long transmissionTimeInterval = 10000,
           unsigned long processingTimeInterval = 180000,
           bool needTransmissionNow = false);
  
  virtual ~LoRaNode() = default;
  
  /**
    * App processing of the node.
    * Invoke every processing time interval of the nodes before Rx and Tx
    * One should benefit from well defining processingTimeInterval to avoid overloading the node
    * return true if need to run fastly (at the next main loop), even false if no process in progress
    */
  virtual bool appProcessing() = 0;

  /**
   * @brief Retrieves the JSON payload for transmission.
   * 
   * This method is a pure virtual function that must be implemented by derived classes.
   * It is used to retrieve the JSON payload that will be transmitted.
   * 
   * @return The JSON payload as a JsonDocument object.
   */
  virtual JsonDocument getJsonTxPayload() = 0;
  
  /**
    * Parse JSON Rx payload
    * One should avoid any long processing in this routine. LoraNode::AppProcessing is the one to be used for this purpose
    * Limit the processing to parsing the payload and retrieving the expected attributes
    * @param payload the JSON payload received by the node
    * Return true in case of new message received
    */
  virtual bool parseJsonRxPayload(JsonDocument& payload) = 0;

  uint8_t getNodeId();
  unsigned long getTransmissionTimeInterval();
  void setTransmissionTimeInterval(unsigned long timeInterval);
  unsigned long getProcessingTimeInterval();
  void setProcessingTimeInterval(unsigned long timeInterval);
  uint16_t getTxCounter();
  void incrementTxCounter();
  bool getTransmissionNowFlag();
  void setTransmissionNowFlag(bool flag);

protected:
  uint8_t mNodeId;
  uint16_t mTxCounter;
  unsigned long mTransmissionTimeInterval;
  // node processing time interval
  unsigned long mProcessingTimeInterval;
  // to force immediate transmission
  bool mNeedTransmissionNow;
};

#endif