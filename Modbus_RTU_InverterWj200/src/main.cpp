#include <Arduino.h>
#include <ModbusRtu.h>

#define RxPin 16
#define TxPin 17

// assign the Arduino pin that must be connected to RE-DE RS485 transceiver
#define TXEN 4

uint16_t au16data[16];
uint8_t u8state;
uint8_t u8query;

Modbus master(0, Serial2, TXEN);

modbus_t telegram[2];

unsigned long u32wait;

void setup()
{
  Serial.begin(19200);
  Serial2.begin(9600, SERIAL_8N1, RxPin, TxPin);
  master.start();
  master.setTimeOut(5000);
  u32wait = millis() + 1000;
  u8state = u8query = 0;
}

void loop()
{
  switch (u8state)
  {
  case 0:
    if (millis() > u32wait)
      u8state++; // wait state
    break;
  case 1:

    // telegram 0: read registers
    telegram[0].u8id = 1;           // slave address
    telegram[0].u8fct = 3;          // function code (this one is registers read)
    telegram[0].u16RegAdd = 0x0001; // start address in slave
    telegram[0].u16CoilsNo = 4;     // number of elements (coils or registers) to read
    telegram[0].au16reg = au16data; // pointer to a memory array in the Arduino

    // telegram 1: write a single register
    telegram[1].u8id = 1;               // slave address
    telegram[1].u8fct = 6;              // function code (this one is write a single register)
    telegram[1].u16RegAdd = 4;          // start address in slave
    telegram[1].u16CoilsNo = 1;         // number of elements (coils or registers) to read
    telegram[1].au16reg = au16data + 4; // pointer to a memory array in the Arduino

    master.query(telegram[u8query]); // send query (only once)
    u8state++;
    u8query++;
    if (u8query > 2)
      u8query = 0;
    break;
  case 2:
    master.poll(); // check incoming messages
    if (master.getState() == COM_IDLE)
    {
      u8state = 0;
      u32wait = millis() + 1000;
    }
    break;
  }

  au16data[4] = random(0, 255);
}

// Function Code 1 (0x01): Đọc các coil (đầu ra diskrete). Đây là các đầu vào hoặc đầu ra ở dạng on/off.
// Function Code 2 (0x02): Đọc các input discrete. Đây là các đầu vào ở dạng on/off.
// Function Code 3 (0x03): Đọc các holding registers (bộ nhớ dữ liệu được lưu trữ và có thể thay đổi). Đây thường là các biến số.
// Function Code 4 (0x04): Đọc các input registers (bộ nhớ dữ liệu chỉ đọc). Đây thường là các biến số.
// Function Code 5 (0x05): Ghi một coil (đầu ra discrete).
// Function Code 6 (0x06): Ghi một holding register (bộ nhớ dữ liệu có thể thay đổi).
// Function Code 15 (0x0F): Ghi nhiều coil (đầu ra discrete).
// Function Code 16 (0x10): Ghi nhiều holding registers (bộ nhớ dữ liệu có thể thay đổi).
