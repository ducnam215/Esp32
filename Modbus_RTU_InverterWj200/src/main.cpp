#include <Arduino.h>
#include <ModbusMaster.h> //Library for using ModbusMaster
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"

#define Slave_ID 1

#define Btn_Start 27
#define Btn_Up 26
#define Btn_Down 25

#define Out_Start 13
#define Out_Stop 14

bool flag = false;

uint8_t Value_Start, Value_Up, Value_Down;
uint8_t Default_Start = 1, Default_Up = 1, Default_Down = 1;
uint8_t Count_Start = 0, Count_Level = 0;
uint8_t Count_Local = 0;

uint16_t Trang_thai_hoat_dong, Chieu_quay;
uint16_t Tan_so;

uint16_t Tinh_trang;
uint16_t Chieu_quay_thuc_te;
uint16_t Trang_thai_hoat_dong_thuc_te;
uint16_t Tan_so_thuc_te;
uint16_t Cuong_do_dong_dien_thuc_te;
uint16_t Hieu_dien_the_thuc_te;
uint16_t Nguon_thuc_te;
uint16_t Giam_sat_huong_quay;

ModbusMaster node;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void Control_RunStop(bool Status);
void Frequency_setting(uint16_t Speed);
void Select_Rotation_Direction(bool Status);
void Operational_information_of_the_inverter(void);

void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8E1, 16, 17);
  node.begin(Slave_ID, Serial2);

  lcd.init();
  lcd.backlight();

  pinMode(Btn_Start, INPUT);
  pinMode(Btn_Up, INPUT);
  pinMode(Btn_Down, INPUT);

  pinMode(Out_Start, OUTPUT);
  pinMode(Out_Stop, OUTPUT);
  digitalWrite(Out_Start, LOW);
  digitalWrite(Out_Stop, LOW);

  Frequency_setting(0);
  Select_Rotation_Direction(Forward);
  Control_RunStop(Stop);
}

void loop()
{
  Value_Start = digitalRead(Btn_Start);
  Value_Up = digitalRead(Btn_Up);
  Value_Down = digitalRead(Btn_Down);

  // Check Button Start
  if (Value_Start != Default_Start)
  {
    if (Value_Start == 1)
    {
      Count_Start++;
      Count_Local = 1;
      flag = true;
      if (Count_Start >= 2)
      {
        Count_Start = 0;
      }
      delay(300); // Debounce delay
    }
    Default_Start = Value_Start;
  }

  // Check Button Up
  if (Value_Up != Default_Up)
  {
    if (Value_Up == 1)
    {
      if (Count_Start == 0)
      {
        Count_Level++;
        Count_Local = 1;
        if (Count_Level >= 16)
        {
          Count_Level = 0;
        }
      }
      delay(300); // Debounce delay
    }
    Default_Up = Value_Up;
  }

  // Check Button Down
  if (Value_Down != Default_Down)
  {
    if (Value_Down == 1)
    {
      if (Count_Start == 0)
      {
        Count_Level--;
        Count_Local = 1;
        if (Count_Level == 255)
        {
          Count_Level = 15;
        }
      }
      delay(300); // Debounce delay
    }
    Default_Down = Value_Down;
  }

  if (flag)
  {
    if (Count_Start == 1)
    {
      switch (Count_Level)
      {
      case 0:
        Frequency_setting(500);
        Select_Rotation_Direction(Forward);
        break;

      case 1:
        Frequency_setting(1000);
        Select_Rotation_Direction(Forward);
        break;

      case 2:
        Frequency_setting(1500);
        Select_Rotation_Direction(Forward);
        break;

      case 3:
        Frequency_setting(2000);
        Select_Rotation_Direction(Forward);
        break;

      case 4:
        Frequency_setting(2500);
        Select_Rotation_Direction(Forward);
        break;

      case 5:
        Frequency_setting(3000);
        Select_Rotation_Direction(Forward);
        break;

      case 6:
        Frequency_setting(3500);
        Select_Rotation_Direction(Forward);
        break;

      case 7:
        Frequency_setting(4000);
        Select_Rotation_Direction(Forward);
        break;

      case 8:
        Frequency_setting(500);
        Select_Rotation_Direction(Reverse);
        break;

      case 9:
        Frequency_setting(1000);
        Select_Rotation_Direction(Reverse);
        break;

      case 10:
        Frequency_setting(1500);
        Select_Rotation_Direction(Reverse);
        break;

      case 11:
        Frequency_setting(2000);
        Select_Rotation_Direction(Reverse);
        break;

      case 12:
        Frequency_setting(2500);
        Select_Rotation_Direction(Reverse);
        break;

      case 13:
        Frequency_setting(3000);
        Select_Rotation_Direction(Reverse);
        break;

      case 14:
        Frequency_setting(3500);
        Select_Rotation_Direction(Reverse);
        break;

      case 15:
        Frequency_setting(4000);
        Select_Rotation_Direction(Reverse);
        break;

      default:
        break;
      }
      Control_RunStop(Run);
      digitalWrite(Out_Stop, LOW);
      digitalWrite(Out_Start, HIGH);
    }
    else
    {
      Control_RunStop(Stop);
      digitalWrite(Out_Start, LOW);
      digitalWrite(Out_Stop, HIGH);
    }
    flag = false;
  }

  //Operational_information_of_the_inverter();

  lcd.setCursor(0, 1);
  lcd.print(Count_Level);
  lcd.setCursor(0, 0);
  lcd.print(flag);
}

void Control_RunStop(bool Status)
{
  if (Status == 1) // Run
  {
    node.writeSingleCoil(Operation_command, Run);
  }
  else
  {
    node.writeSingleCoil(Operation_command, Stop);
  }
}

void Frequency_setting(uint16_t Speed)
{
  node.writeSingleRegister(Frequency_source, Speed);
}

void Select_Rotation_Direction(bool Status)
{
  if (Status == 1) // ngược
  {
    node.writeSingleCoil(Rotation_direction_command, Reverse);
  }
  else
  {
    node.writeSingleCoil(Rotation_direction_command, Forward);
  }
}

void Operational_information_of_the_inverter(void)
{
  Tan_so_thuc_te = node.readHoldingRegisters(0x1001, Slave_ID);
  Serial.print("Tstt: ");
  Serial.print(node.getResponseBuffer(Tan_so_thuc_te));
  Serial.println(" ");
}