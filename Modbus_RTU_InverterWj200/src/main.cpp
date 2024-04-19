#include <Arduino.h>
#include <ModbusMaster.h> //Library for using ModbusMaster
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
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
uint8_t Count_Local = 0, inputLevel;

uint16_t Chieu_quay_thuc_te;
uint16_t Tan_so_thuc_te, inputFreq;
uint16_t Cap_cuc, Speed_RPM;

String inputStatus;
String inputLocal;

const uint8_t pole[] = {0, 2, 4, 6, 8, 10, 12, 14};

ModbusMaster node;
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char *ssid = "Samsung Galaxy Z Flip4";
const char *password = "241810697";

void Control_RunStop(bool Status);
void Frequency_setting(uint16_t Speed);
void Select_Rotation_Direction(bool Status);
void Operational_information_of_the_inverter(void);

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Modbus RTU</title>
  <style>
    * {
      box-sizing: border-box;
    }

    body {
      font-family: Arial;
      padding: 20px;
      background: #f1f1f1;
    }

    .header {
      padding: 15px;
      font-size: 25px;
      text-align: center;
      background: white;
    }

    .rightcolumn {
      width: 100%;
    }

    .card {
      background-color: white;
      padding: 20px;
      margin-top: 20px;
    }

    .row:after {
      content: "";
      display: table;
      clear: both;
    }

    .footer {
      padding: 5px;
      text-align: center;
      background: #ddd;
      margin-top: 10px;
      font-size: 12px;
    }

    @media screen and (max-width: 800px) {
      .rightcolumn {
        padding: 0;
      }
    }

    .co {
      float: left;
      width: 25%;
      padding: 0 10px;
    }

    .ro {
      margin: 0 -5px;
    }

    .ro:after {
      content: "";
      display: table;
      clear: both;
    }

    @media screen and (max-width: 600px) {
      .co {
        width: 100%;
        display: block;
      }
    }

    .ca {
      box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.2);
      padding: 16px;
      text-align: center;
      background-color: #f1f1f1;
    }
  </style>
</head>

<body>
  <div class="header">
    <h2>Truyền thông Modbus RTU</h2>
    <p>Web điều khiển động cơ 3 pha qua biến tần WJ200 Hitachi 16 cấp tốc độ qua truyền thông Modbus RTU</p>
  </div>
  <div class="row">
    <div class="rightcolumn">
      <div class="card">
        <h2>Điều khiển và Giám sát</h2>
        <div class="ro">
          <div class="co">
            <div class="ca">
              <h3>Điều khiển & Cài Đặt</h3>
              <strong>Trạng thái hoạt động: <span id="Statusbt">---</span></strong>
              <p><button onclick="control_inverter(1)">Chạy/Dừng</button></p>
              <strong>Cấp độ: <span id="Levelbt">---</span></strong>
              <p>
                <button onclick="control_inverter(2)">Tăng</button>
                <button onclick="control_inverter(3)">Giảm</button>
              </p>
              <strong>Vị trí điều khiển gần nhất: <span id="Local_control">---</span></strong>
            </div>
          </div>
          <div class="co">
            <div class="ca">
              <h3>Thông số cài đặt</h3>
              <p><strong>Cấp độ cài đặt: <span id="Level_set">---</span></strong></p>
              <p><strong>Tần số cài đặt: <span id="Freq_set">---</span>[Hz]</strong></p>
              <p><strong>Chiều quay cài đặt: <span id="Direction_set">---</span></strong></p>
            </div>
          </div>
          <div class="co">
            <div class="ca">
              <h3>Thông số thực tế</h3>
              <p><strong>Tần số thực tế: <span id="Freq_real">---</span>[Hz]</strong></p>
              <p><strong>Chiều quay thực: <span id="Direction_real">---</span></strong></p>
              <p><strong>Vận tốc hiện tại: <span id="Speed_current">---</span>RPM</strong></p>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
  <div class="footer">
    <h5>Copyright © 2023 - 2024 Trò chơi của thầy</h5>
  </div>
  <script>
    function control_inverter(index) {
      sendRequest(index, "Status");
    }
    function sendRelayRequest(index, state) {
      fetch("Relay_" + index + "=" + state)
        .then((response) => {
          if (!response.ok) {
            throw new Error("Lỗi kết nối mạng");
          }
        })
        .catch((error) => {
          console.error("Có vấn đề xảy ra trong quá trình fetch:", error);
        });
    }
    function ajaxLoad(ajaxURL) {
      fetch(ajaxURL)
        .then((response) => {
          if (!response.ok) {
            throw new Error("Network response was not ok");
          }
          return response.text();
        })
        .then((data) => {
          var tmpArray = data.split("|", 9);
          document.getElementById("Statusbt").innerHTML = tmpArray[0];
          document.getElementById("Levelbt").innerHTML = tmpArray[1];
          document.getElementById("Local_control").innerHTML = tmpArray[2];
          document.getElementById("Level_set").innerHTML = tmpArray[3];
          document.getElementById("Freq_set").innerHTML = tmpArray[4];
          document.getElementById("Direction_set").innerHTML = tmpArray[5];
          document.getElementById("Freq_real").innerHTML = tmpArray[6];
          document.getElementById("Direction_real").innerHTML = tmpArray[7];
          document.getElementById("Speed_current").innerHTML = tmpArray[8];
        })
        .catch((error) => {
          console.error(
            "There was a problem with the fetch operation:",
            error
          );
        });
    }
    function update_data() {
      ajaxLoad("get_data");
    }
    setInterval(update_data, 1000);
  </script>
</body>
</html>
)rawliteral";

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

  lcd.setCursor(0, 0);
  lcd.print("Tt:"); // chay dừng
  lcd.setCursor(0, 1);
  lcd.print("Cq: "); // thuan nghich
  lcd.setCursor(8, 0);
  lcd.print("Ts:"); // 4000
  Frequency_setting(0);
  Select_Rotation_Direction(Forward);
  Control_RunStop(Stop);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html); });

  // Send a GET request to <ESP_IP>/toggle to toggle the output
  server.on("/Request_1=Status", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              Count_Start++;
              Count_Local = 2;
              flag = true;
              if (Count_Start >= 2)
              {
                Count_Start = 0;
              }
              request->send(200, "text/plain", "OK"); });

  // Send a GET request to <ESP_IP>/toggle to toggle the output
  server.on("/Request_2=Status", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              if (Count_Start == 0)
              {
                Count_Level++;
                Count_Local = 2;
                if (Count_Level >= 16)
                {
                  Count_Level = 0;
                }
              }
              request->send(200, "text/plain", "OK"); });

  // Send a GET request to <ESP_IP>/toggle to toggle the output
  server.on("/Request_3=Status", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              if (Count_Start == 0)
              {
                Count_Level--;
                Count_Local = 2;
                if (Count_Level == 255)
                {
                  Count_Level = 15;
                }
              }
              request->send(200, "text/plain", "OK"); });

  // Send a GET request to <ESP_IP>/state to get the output state
  server.on("/get_data", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
            if (Count_Start == 1)
            {
              inputStatus = "Chạy";
            }else
            {
              inputStatus = "Dừng";
            }

            if (Count_Local == 0)
            {
              inputLocal = "---";
            }
            else if (Count_Local == 1)
            {
              inputLocal = "Esp32 Hardware";
            }
            else if (Count_Local == 2)
            {
              inputLocal = "Web Esp32";
            }
            String response = String(inputStatus) + "|" + String(Count_Level)  + "|" + String(inputLocal) + "|" + String(inputLevel) + "|" + String(inputFreq);

            request->send(200, "text/plain", response); });
  // Start server
  server.begin();
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
        lcd.setCursor(4, 1);
        lcd.print("Thuan ");
        lcd.setCursor(11, 0);
        lcd.print("500");
        inputFreq = 500;
        break;

      case 1:
        Frequency_setting(1000);
        Select_Rotation_Direction(Forward);
        lcd.setCursor(4, 1);
        lcd.print("Thuan ");
        lcd.setCursor(11, 0);
        lcd.print("1000");
        inputFreq = 1000;
        break;

      case 2:
        Frequency_setting(1500);
        Select_Rotation_Direction(Forward);
        lcd.setCursor(4, 1);
        lcd.print("Thuan ");
        lcd.setCursor(11, 0);
        lcd.print("1500");
        inputFreq = 1500;
        break;

      case 3:
        Frequency_setting(2000);
        Select_Rotation_Direction(Forward);
        lcd.setCursor(4, 1);
        lcd.print("Thuan ");
        lcd.setCursor(11, 0);
        lcd.print("2000");
        inputFreq = 2000;
        break;

      case 4:
        Frequency_setting(2500);
        Select_Rotation_Direction(Forward);
        lcd.setCursor(4, 1);
        lcd.print("Thuan ");
        lcd.setCursor(11, 0);
        lcd.print("2500");
        inputFreq = 2500;
        break;

      case 5:
        Frequency_setting(3000);
        Select_Rotation_Direction(Forward);
        lcd.setCursor(4, 1);
        lcd.print("Thuan ");
        lcd.setCursor(11, 0);
        lcd.print("3000");
        inputFreq = 3000;
        break;

      case 6:
        Frequency_setting(3500);
        Select_Rotation_Direction(Forward);
        lcd.setCursor(4, 1);
        lcd.print("Thuan ");
        lcd.setCursor(11, 0);
        lcd.print("3500");
        inputFreq = 3500;
        break;

      case 7:
        Frequency_setting(4000);
        Select_Rotation_Direction(Forward);
        lcd.setCursor(4, 1);
        lcd.print("Thuan ");
        lcd.setCursor(11, 0);
        lcd.print("4000");
        inputFreq = 4000;
        break;

      case 8:
        Frequency_setting(500);
        Select_Rotation_Direction(Reverse);
        lcd.setCursor(4, 1);
        lcd.print("Nghich");
        lcd.setCursor(11, 0);
        lcd.print("500");
        inputFreq = 500;
        break;

      case 9:
        Frequency_setting(1000);
        Select_Rotation_Direction(Reverse);
        lcd.setCursor(4, 1);
        lcd.print("Nghich");
        lcd.setCursor(11, 0);
        lcd.print("1000");
        inputFreq = 1000;
        break;

      case 10:
        Frequency_setting(1500);
        Select_Rotation_Direction(Reverse);
        lcd.setCursor(4, 1);
        lcd.print("Nghich");
        lcd.setCursor(11, 0);
        lcd.print("1500");
        inputFreq = 1500;
        break;

      case 11:
        Frequency_setting(2000);
        Select_Rotation_Direction(Reverse);
        lcd.setCursor(4, 1);
        lcd.print("Nghich");
        lcd.setCursor(11, 0);
        lcd.print("2000");
        inputFreq = 2000;
        break;

      case 12:
        Frequency_setting(2500);
        Select_Rotation_Direction(Reverse);
        lcd.setCursor(4, 1);
        lcd.print("Nghich");
        lcd.setCursor(11, 0);
        lcd.print("2500");
        inputFreq = 2500;
        break;

      case 13:
        Frequency_setting(3000);
        Select_Rotation_Direction(Reverse);
        lcd.setCursor(4, 1);
        lcd.print("Nghich");
        lcd.setCursor(11, 0);
        lcd.print("3000");
        inputFreq = 3000;
        break;

      case 14:
        Frequency_setting(3500);
        Select_Rotation_Direction(Reverse);
        lcd.setCursor(4, 1);
        lcd.print("Nghich");
        lcd.setCursor(11, 0);
        lcd.print("3500");
        inputFreq = 3500;
        break;

      case 15:
        Frequency_setting(4000);
        Select_Rotation_Direction(Reverse);
        lcd.setCursor(4, 1);
        lcd.print("Nghich");
        lcd.setCursor(11, 0);
        lcd.print("4000");
        inputFreq = 4000;
        break;
      default:
        break;
      }
      inputLevel = Count_Level;
      Control_RunStop(Run);
      lcd.setCursor(3, 0);
      lcd.print("Chay");
      digitalWrite(Out_Stop, LOW);
      digitalWrite(Out_Start, HIGH);
    }
    else
    {
      inputFreq = 0;
      Frequency_setting(0);
      Select_Rotation_Direction(Forward);
      Control_RunStop(Stop);
      lcd.setCursor(4, 1);
      lcd.print("Thuan ");
      lcd.setCursor(11, 0);
      lcd.print("0");
      lcd.setCursor(3, 0);
      lcd.print("Dung");
      digitalWrite(Out_Start, LOW);
      digitalWrite(Out_Stop, HIGH);
    }
    flag = false;
  }
  Operational_information_of_the_inverter();
}

void Control_RunStop(bool Status)
{
  node.writeSingleCoil(Operation_command, Status);
}

void Frequency_setting(uint16_t Speed)
{
  node.writeSingleRegister(Frequency_source, Speed);
}

void Select_Rotation_Direction(bool Status)
{
  node.writeSingleCoil(Rotation_direction_command, Status);
}

void Operational_information_of_the_inverter(void)
{
  Tan_so_thuc_te = node.readHoldingRegisters(0x1001, Slave_ID);
  Tan_so_thuc_te = node.getResponseBuffer(Tan_so_thuc_te);
  Serial.print("Tstt: ");
  Serial.print(Tan_so_thuc_te);
  Serial.print(" ");

  Chieu_quay_thuc_te = node.readHoldingRegisters(0x1004, Slave_ID);
  Chieu_quay_thuc_te = node.getResponseBuffer(Chieu_quay_thuc_te);
  Serial.print("cqtt: ");
  Serial.print(Chieu_quay_thuc_te);
  Serial.print(" ");

  Cap_cuc = node.readHoldingRegisters(0x1633, Slave_ID);
  Cap_cuc = node.getResponseBuffer(Cap_cuc);
  Serial.print("Cc: ");
  Serial.print(Cap_cuc);
  Serial.print(" ");

  Speed_RPM = ((Tan_so_thuc_te * 120) / pole[Cap_cuc]);
  Serial.print("rpm: ");
  Serial.print(Cap_cuc);
  Serial.print(" ");

  Serial.println(" ");
}