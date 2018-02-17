/*
*   Arduino2PiSer
*
*   Serial protocol for communication between OrangePi PC and Arduino. 
*   Used for simple sensor data requests.
*   
*   Test version, to be upgraded to library once done and tested.
*   
*   17.2.2018 / inital version 0.1.
*   
*/

#include <CircularBuffer.h>

// Buffers
CircularBuffer<byte,64> gSERRxBuffer; 
CircularBuffer<byte,64> gSERTxBuffer;

// State machine
enum enProcessStates
{
  STATE_IDLE,             // inital state, waiting for packet to start
  STATE_CMD_LEN,          // start-of-packet received, waiting for packet length
  STATE_CMD_ID,           // packet length recived, waiting for ID [optional]
};
enum enProcessStates gProcessState = STATE_IDLE;
byte gMsgCheckSum = 0;
byte gMsgLength = 0;

// Serial Communication recive process
void procSERRecive()
{
  while (Serial.available() > 0)
  {
    if (!gSERRxBuffer.push(Serial.read()))
    {
      // buffer overrun
      // TODO: do something if overrun happens
      // IDEA: clean buffer, send error (copy msg to tx buffer)
    }
  }
}

// Serial Communication send process
void procSERSend()
{
  while (!gSERTxBuffer.isEmpty())
  {
    Serial.write(gSERTxBuffer.shift());
  }
}

// Process recive buffer
void procProcessData()
{
  
  // if (!gSERRxBuffer.isEmpty())     // process one data at the time
  while (!gSERRxBuffer.isEmpty())     // process whole buffer; 
                                      // might be to fast if more than one package in the buffer
  {
    // state machine so the communication is not limited to one loop
    byte nData = gSERRxBuffer.shift();
    switch (gProcessState)
    {
      case STATE_IDLE:
        if (nData == 0xF0) gProcessState = STATE_CMD_LEN;
        // else discard data not part of package
        break;
      
      case STATE_CMD_LEN:
        gMsgLength = nData;
        gProcessState = STATE_CMD_ID;
        break;
      
      default:
        break;
    }
  }
}

void setup() 
{
  // start serial port
  Serial.begin(9600);  
}

void loop() 
{
  // main loop
  procSERRecive();
  procProcessData();
  procSERSend();
}
