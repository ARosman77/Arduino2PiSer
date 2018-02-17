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
