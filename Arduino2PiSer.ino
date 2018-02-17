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
  STATE_CLEAN_UP,         // before initial state message clean-up is needed
  STATE_IDLE,             // inital state, waiting for packet to start
  STATE_CMD_LEN,          // start-of-packet received, waiting for packet length
  STATE_CMD_ID,           // packet length recived, waiting for ID [optional?]
  STATE_CMD_NAME,         // ID received, waiting for command name
  STATE_CMD_PARAMS,       // command name received, receiving command parameters
  STATE_CMD_CS,           // parameters received, waiting for checksum
  STATE_CMD_END,          // checksum received, waiting for end-of-packet
  STATE_CMD_ERROR,        // error state
  STATE_CMD_OK,           // message received OK, ready to process
  STATE_CMD_ACK,          // message processing finished, answer prepared to be sent
  STATE_CMD_NAK,          // message processing finished, error prepared to be sent
};
enum enProcessStates gProcessState = STATE_IDLE;
byte gMsgCheckSum = 0;
byte gMsgLength = 0;
byte gMsgCmd = 0;
CircularBuffer<char,64> gMsgParams;

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
  if (!gSERRxBuffer.isEmpty())           // process one data at the time
  // while (!gSERRxBuffer.isEmpty())     // process whole buffer; 
                                         // might be too fast if more than one package in the buffer
  {
    // state machine so the communication is not limited to one loop
    byte nData = gSERRxBuffer.shift();

    // TODO: before checking state machine check for special characters and act acordingly
    // IDEA: if SOP received, switch to STATE_CLEAN_UP ?? depends on state
    // IDEA: if EOP received, switch to STATE_CLEAN_UP ?? depends on state
    // IDEA: check message length
    
    switch (gProcessState)
    {
      case STATE_CLEAN_UP:
        // TODO: Clean up message variables and switch to idle in the same loop
        gMsgCheckSum = 0;
        gMsgLength = 0;
        gMsgCmd = 0;
        gMsgParams.clear();
        gProcessState = STATE_IDLE;
        
      case STATE_IDLE:
        if (nData == 0xF0) gProcessState = STATE_CMD_LEN;   // TODO: Change SOP byte
        // else discard data not part of package
        break;
      
      case STATE_CMD_LEN:
        gMsgLength = nData;
        gProcessState = STATE_CMD_ID;
        break;

      case STATE_CMD_ID:
        // currently ignoring the ID
        gProcessState = STATE_CMD_NAME;
        break;

      case STATE_CMD_NAME:
        gMsgCmd = nData;
        // start calculating checksum
        gMsgCheckSum = nData;
        gProcessState = STATE_CMD_PARAMS;
        break;

      case STATE_CMD_PARAMS:
        gMsgParams.push(nData);
        // calculate checksum as XOR of all bytes
        gMsgCheckSum ^= nData;
        if (nData == 0xF0) gProcessState = STATE_CMD_CS;     // TODO: Change end of params byte
        break;

      case STATE_CMD_CS:
        if ( gMsgCheckSum != nData)
        {
          // checksum ERROR, currently ignored
          // gProcessState = STATE_CMD_ERROR;
        }
        // else
        gProcessState = STATE_CMD_END;
        break;
     
     case STATE_CMD_END:
        if (nData == 0xF0) gProcessState = STATE_CMD_OK;   // TODO: Change EOP byte
        else gProcessState = STATE_CMD_ERROR;
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
