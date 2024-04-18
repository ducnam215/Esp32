#include <Arduino.h>
#include <ModbusMaster.h>

#define Slave_ID 1

#define Run 1
#define Stop 0

#define Reverse Run
#define Forward Stop

// ModBus Coil List
#define Operation_command 0x0001U
#define Rotation_direction_command 0x0002U
#define Operation_status 0x000FU
#define Rotation_direction 0x0010U
#define Inverter_ready 0x0011U

// ModBus Holding Registers
#define Frequency_source 0x0001U
#define Output_frequency_monitor 0x1001U
#define Output_current_monitor 0x1003U
#define Rotation_direction_minitoring 0x1004U
#define Output_voltage_monitor 0x1011U
#define Power_monitor 0x1012U

#define Btn_Start 16
#define Btn_Up 17
#define Btn_Down 18

#define Out_Start 13
#define Out_Stop 14

bool flag = false;
uint8_t Value_Start, Value_Up, Value_Down;
uint8_t Default_Start = 1, Default_Up = 1, Default_Down = 1;
uint8_t Count_Start = 0, Count_Level = 0;
uint8_t Count_Local = 0;

uint16_t Trang_thai_hoat_dong, Chieu_quay, Tinh_trang;
uint16_t Tan_so, Tan_so_thuc_te;
uint16_t Cuong_do_dong_dien_thuc_te, Chieu_quay_thuc_te, Hieu_dien_the_thuc_te, Nguon_thuc_te;

ModbusMaster node;

void Control_RunStop(bool Status);
void Frequency_setting(uint16_t Speed);
void Select_Rotation_Direction(bool Status);
void Operational_information_of_the_inverter(void);

void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  node.begin(Slave_ID, Serial2);

  pinMode(Btn_Start, INPUT);
  pinMode(Btn_Up, INPUT);
  pinMode(Btn_Down, INPUT);

  pinMode(Out_Start, OUTPUT);
  pinMode(Out_Stop, OUTPUT);
  digitalWrite(Out_Start, LOW);
  digitalWrite(Out_Stop, LOW);
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
        if (Count_Level >= 8)
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
          Count_Level = 7;
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
        Frequency_setting(1000);
        delay(200);
        Select_Rotation_Direction(Forward);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 1:
        Frequency_setting(1500);
        delay(200);
        Select_Rotation_Direction(Forward);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 2:
        Frequency_setting(2000);
        delay(200);
        Select_Rotation_Direction(Forward);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 3:
        Frequency_setting(2500);
        delay(200);
        Select_Rotation_Direction(Forward);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 4:
        Frequency_setting(3000);
        delay(200);
        Select_Rotation_Direction(Forward);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 5:
        Frequency_setting(3500);
        delay(200);
        Select_Rotation_Direction(Forward);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 6:
        Frequency_setting(4000);
        delay(200);
        Select_Rotation_Direction(Forward);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 7:
        Frequency_setting(1000);
        delay(200);
        Select_Rotation_Direction(Reverse);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 8:
        Frequency_setting(1500);
        delay(200);
        Select_Rotation_Direction(Reverse);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 9:
        Frequency_setting(2000);
        delay(200);
        Select_Rotation_Direction(Reverse);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 10:
        Frequency_setting(2500);
        delay(200);
        Select_Rotation_Direction(Reverse);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 11:
        Frequency_setting(3000);
        delay(200);
        Select_Rotation_Direction(Reverse);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 12:
        Frequency_setting(3500);
        delay(200);
        Select_Rotation_Direction(Reverse);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 13:
        Frequency_setting(4000);
        delay(200);
        Select_Rotation_Direction(Reverse);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 14:
        Frequency_setting(500);
        delay(200);
        Select_Rotation_Direction(Reverse);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      case 15:
        Frequency_setting(500);
        delay(200);
        Select_Rotation_Direction(Forward);
        delay(200);
        Control_RunStop(Run);
        delay(200);
        break;

      default:
        break;
      }
      digitalWrite(Out_Stop, LOW);
      digitalWrite(Out_Start, HIGH);
    }
    else
    {
      Frequency_setting(0);
      delay(200);
      Control_RunStop(Stop);
      delay(200);
      digitalWrite(Out_Start, LOW);
      digitalWrite(Out_Stop, HIGH);
    }
    flag = false;
  }
  Operational_information_of_the_inverter();
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

  Tinh_trang = node.readHoldingRegisters(0x0011, Slave_ID);
  Serial.print("Tình trạng hoạt động:");
  Serial.print(node.getResponseBuffer(Tinh_trang));
  Serial.print("   ");

  Tan_so_thuc_te = node.readHoldingRegisters(0x1001, Slave_ID);
  Serial.print("Tần số thực tế:");
  Serial.print(node.getResponseBuffer(Tan_so_thuc_te));
  Serial.print("   ");

  Cuong_do_dong_dien_thuc_te = node.readHoldingRegisters(0x1003, Slave_ID);
  Serial.print("Cường độ dòng điện thực tế:");
  Serial.print(node.getResponseBuffer(Cuong_do_dong_dien_thuc_te));
  Serial.print("   ");

  Chieu_quay_thuc_te = node.readHoldingRegisters(0x1004, Slave_ID);
  Serial.print("Chiều quay thực tế:");
  Serial.print(node.getResponseBuffer(Chieu_quay_thuc_te));
  Serial.print("   ");

  Hieu_dien_the_thuc_te = node.readHoldingRegisters(0x1011, Slave_ID);
  Serial.print("Hiệu điện thế thực tế:");
  Serial.print(node.getResponseBuffer(Hieu_dien_the_thuc_te));
  Serial.print("   ");

  Nguon_thuc_te = node.readHoldingRegisters(0x1022, Slave_ID);
  Serial.print("Nguồn điện thực tế:");
  Serial.print(node.getResponseBuffer(Nguon_thuc_te));
  Serial.print("   ");
  Serial.println("   ");
}
