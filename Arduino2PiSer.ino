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
CircularBuffer<byte,64> SERRxBuffer; 
CircularBuffer<byte,64> SERTxBuffer;

void fnSERRecive()
{
  while (Serial.available() > 0)
  {
    SERRxBuffer.push(Serial.read());
  }
}

void fnSERSend()
{
  while (!SERTxBuffer.isEmpty())
  {
    Serial.write(SERTxBuffer.shift());
  }
}

void fnProcessData()
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
  fnSERRecive();
  fnProcessData();
  fnSERSend();
}
