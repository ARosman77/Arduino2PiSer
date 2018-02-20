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
*   Protocol idea:
*   
*     <[LEN]/[ID*];[CMD]:[PARAM1] [PARAM2] ... [PARAMn]=[CHKSUM*]>
*               * optional ID                                  * optional checksum (sum%256)
*     Each of the data is limited to max 16 chars!
*   
*/

// #define _DEBUG_INIT
#define _DEBUG_

// allowed ASCII for communication
const char gcAllowedStart = 32;
const char gcAllowedEnd = 122;

// start/stop chars
const char gcStartChar = 60;         // <
const char gcPreIDChar = 47;         // /
const char gcPreCMDChar = 59;        // ;
const char gcPreParamChar = 58;      // :
const char gcParamSeparator = 32;    // SPACE
const char gcPreChkSumChar = 61;     // =
const char gcStopChar = 62;          // >

const byte gcMaxDataPartLen = 16;

// debug LED pins
const byte pinLED_R = 3;
const byte pinLED_G = 5;
const byte pinLED_B = 6;

#include <CircularBuffer.h>

// Buffers
CircularBuffer<byte,64> gSERRxBuffer; 
CircularBuffer<byte,64> gSERTxBuffer;

// State machine
enum enProcessStates
{
  STATE_CLEAN_UP,         // before initial state message clean-up is needed ???
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
enum enProcessStates gProcessState = STATE_CLEAN_UP;
byte gMsgCheckSum = 0;
byte gMsgLength = 0;
byte gMsgCmd = 0;
CircularBuffer<int,16> gMsgParams;
// CircularBuffer<char,gcMaxDataPartLen> gParseBuffer;
char gParseBuffer[gcMaxDataPartLen] = {};
byte gnParseBufferCnt = 0;

// Serial Communication recive process
void procSERReceive()
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
void procParseMsg()
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
      #ifdef _DEBUG_
        digitalWrite(pinLED_R,0);
        digitalWrite(pinLED_G,0);
        digitalWrite(pinLED_B,0);
      #endif
        // TODO: Clean up message variables and switch to idle in the same loop
        gMsgCheckSum = 0;
        gMsgLength = 0;
        gMsgCmd = 0;
        gMsgParams.clear();
        gnParseBufferCnt = 0;
        gProcessState = STATE_IDLE;
        
      case STATE_IDLE:
        #ifdef _DEBUG_
          digitalWrite(pinLED_B,1);
        #endif
        if (nData == gcStartChar) gProcessState = STATE_CMD_LEN;
        // else discard data not part of package
        break;
      
      case STATE_CMD_LEN:
        if ((nData == gcPreIDChar)||(nData == gcPreCMDChar))
        {
          // length recived
          gParseBuffer[gnParseBufferCnt++]=0;
          // process buffer and switch to next state
          gMsgLength = String(gParseBuffer).toInt();
          #ifdef _DEBUG_
            Serial.print("MsgLen = ");
            Serial.println(gMsgLength);
          #endif
          gnParseBufferCnt = 0;
          if (nData == gcPreCMDChar) gProcessState = STATE_CMD_NAME;
          else gProcessState = STATE_CMD_ID;
        }
        else
        {
          gParseBuffer[gnParseBufferCnt++]=nData;
        }
        break;

      case STATE_CMD_ID:
        // currently ignoring the ID
        if (nData == gcPreCMDChar) gProcessState = STATE_CMD_NAME;
        break;

      case STATE_CMD_NAME:
        // start calculating checksum
        gMsgCheckSum ^= nData;
        if (nData == gcPreParamChar)
        {
          gParseBuffer[gnParseBufferCnt++]=0;
          // process buffer and switch to next state
          gMsgCmd = String(gParseBuffer).toInt();
          gnParseBufferCnt = 0;
          #ifdef _DEBUG_
            Serial.print("MsgCmd = ");
            Serial.println(gMsgCmd);
          #endif
          gProcessState = STATE_CMD_PARAMS;
        }
        else
        {
          gParseBuffer[gnParseBufferCnt++]=nData;
        }
        break;

      case STATE_CMD_PARAMS:
        // calculate checksum as XOR of all bytes
        gMsgCheckSum ^= nData;
        if ((nData == gcParamSeparator)||(nData == gcPreChkSumChar)||(nData == gcStopChar))
        {
          gParseBuffer[gnParseBufferCnt++]=0;
          // process buffer and switch to next state
          gMsgParams.push(String(gParseBuffer).toInt());
          gnParseBufferCnt = 0;
          #ifdef _DEBUG_
            Serial.print("Parameter ");
            Serial.print(gMsgParams.size());
            Serial.print(" = ");
            Serial.println(gMsgParams.last());
          #endif
          if (nData == gcPreChkSumChar) gProcessState = STATE_CMD_CS;
          if (nData == gcStopChar) gProcessState = STATE_CMD_OK;
        }
        else
        {
          gParseBuffer[gnParseBufferCnt++]=nData;
        }
        break;

      case STATE_CMD_CS:
        if (nData == gcStopChar)
        {
          #ifdef _DEBUG_
          Serial.print("Checksum: ");
          Serial.print(gMsgCheckSum);
          #endif
          gParseBuffer[gnParseBufferCnt++]=0;
          if ( gMsgCheckSum == String(gParseBuffer).toInt())
          {
            gProcessState = STATE_CMD_OK;
            #ifdef _DEBUG_
              Serial.print(" == ");
              Serial.println(String(gParseBuffer).toInt());
              digitalWrite(pinLED_R,0);
              digitalWrite(pinLED_G,1);
              digitalWrite(pinLED_B,0);
            #endif
          }
          else
          {
            gProcessState = STATE_CMD_ERROR;
            #ifdef _DEBUG_
              Serial.print(" != ");
              Serial.println(String(gParseBuffer).toInt());
              digitalWrite(pinLED_R,1);
              digitalWrite(pinLED_G,0);
              digitalWrite(pinLED_B,0);
            #endif
          }
        }
        else
        {
          gParseBuffer[gnParseBufferCnt++]=nData;
        }
        break;

      default:
        break;
    }
  }
}

void procExecMsg()
{
  switch (gProcessState)
  {
    case STATE_CMD_OK:
      gSERTxBuffer.push('A');
      gSERTxBuffer.push('C');
      gSERTxBuffer.push('K');
      gProcessState = STATE_CLEAN_UP;
      break;
      
    case STATE_CMD_ERROR:
      gSERTxBuffer.push('N');
      gSERTxBuffer.push('A');
      gSERTxBuffer.push('K');
      gProcessState = STATE_CLEAN_UP;
      break;
    
    default:
      break;
  }
}

void setup() 
{
  // initialize debug LED pins and test them
  pinMode(pinLED_R,OUTPUT);
  pinMode(pinLED_G,OUTPUT);
  pinMode(pinLED_B,OUTPUT);

#ifdef _DEBUG_INIT
  digitalWrite(pinLED_R,1);
  delay(500);
  digitalWrite(pinLED_R,0);
  digitalWrite(pinLED_G,1);
  delay(500);
  digitalWrite(pinLED_G,0);
  digitalWrite(pinLED_B,1);
  delay(500);
  digitalWrite(pinLED_B,0);
 #endif
 
  // start serial port
  Serial.begin(9600);  
}

void loop() 
{
  // main loop
  procSERReceive();
  procParseMsg();
  procExecMsg();
  procSERSend();
}
