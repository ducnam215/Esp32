#include <Arduino.h>
#include <ModbusMaster.h> //Library for using ModbusMaster

#define Slave_ID 1

bool status = true;
uint16_t Freq_inverter;

ModbusMaster node; // object node for class ModbusMaster

void setup()
{
  Serial.begin(115200); // Baud Rate as 115200
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  node.begin(Slave_ID, Serial2); // Slave ID as 1
}

void loop()
{

  Freq_inverter = node.readHoldingRegisters(0x01001, Slave_ID);
  float value = random(0, 11);
  node.writeSingleRegister(0x40000, value);  //Writes value to 0x40000 holding register
  node.writeSingleCoil(0x00000, status); // Writes value to 0x40000 holding register
  node.writeSingleCoil(0x00001, status); // Writes value to 0x40000 holding register
  status = !status;
  delay(200);

  Serial.print("Freq_inverter:");
  Serial.print(Freq_inverter);
  Serial.print("   ");
  Serial.print("Freq_inverter_res:");
  Serial.print(node.getResponseBuffer(Freq_inverter));
  Serial.println("   ");
}