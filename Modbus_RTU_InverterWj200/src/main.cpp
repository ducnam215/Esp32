#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LiquidCrystal_I2C.h>
#include <ModbusMaster.h> //Library for using ModbusMaster
#include <Register_inverter.h>

#define Slave_ID 1

const char *ssid = "Samsung Galaxy Z Flip4";
const char *password = "241810697";

#define Btn_Start 27
#define Btn_Up 26
#define Btn_Down 25

uint8_t Value_Start, Value_Up, Value_Down;
uint8_t Default_Start = 1, Default_Up = 1, Default_Down = 1;
uint8_t Count_Start = 0, Count_Level = 0, inputLevel = 0;
uint8_t Count_Local = 0;

uint16_t Tan_so_thuc_te, Chieu_quay_thuc_te, inputFreq;
uint16_t Cap_cuc, Speed_RPM;
uint16_t Trang_thai, Nguon, Cuong_do, Hieu_dien_the;

const uint16_t frequencies[] = {500, 1000, 1500, 2000, 2500, 3000, 3500, 4000,
                                500, 1000, 1500, 2000, 2500, 3000, 3500, 4000};

const bool directions[] = {Forward, Forward, Forward, Forward, Forward, Forward, Forward, Forward,
                           Reverse, Reverse, Reverse, Reverse, Reverse, Reverse, Reverse, Reverse};

const uint8_t pole[] = {0, 2, 4, 6, 8, 10, 12, 14};

unsigned long timer1 = 0;

bool flag = false;

String inputStatus;
String inputLocal;
String inputDirectionset;
String inputTrangthai;
String inputChieuquaythuc;

ModbusMaster node;
LiquidCrystal_I2C lcd(0x27, 16, 2);
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
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
              <p><strong>Trạng Thái: <span id="Status_real">---</span></strong></p>
              <p><strong>Tần số thực tế: <span id="Freq_real">---</span>[Hz]</strong></p>
              <p><strong>Chiều quay thực: <span id="Direction_real">---</span></strong></p>
              <p><strong>Vận tốc hiện tại: <span id="Speed_current">---</span>[RPM]</strong></p>
            </div>
          </div>

          <div class="co">
            <div class="ca">
              <h3>Thông tin thêm</h3>
              <p><strong>Nguồn điện: <span id="Power_monitor">---</span>[kW]</strong></p>
              <p><strong>Cường độ dòng điện: <span id="Current_monitor">---</span> [A]</strong></p>
              <p><strong>Hiệu điện thế: <span id="Voltage_monitor">---</span>[V]</strong></p>
            </div>
          </div>

        </div>
      </div>
    </div>
  </div>
  <div class="footer">
    <h5>Copyright © 2023 - 2024 Trò chơi của Phạm Đức Nam</h5>
  </div>
  <script>
    function control_inverter(index) {
      sendRequest(index, "Status");
    }
    function sendRequest(index, state) {
      fetch("Request_" + index + "=" + state)
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
          var tmpArray = data.split("|", 13);
          document.getElementById("Statusbt").innerHTML = tmpArray[0];
          document.getElementById("Levelbt").innerHTML = tmpArray[1];
          document.getElementById("Local_control").innerHTML = tmpArray[2];
          document.getElementById("Level_set").innerHTML = tmpArray[3];
          document.getElementById("Freq_set").innerHTML = tmpArray[4];
          document.getElementById("Direction_set").innerHTML = tmpArray[5];
          document.getElementById("Status_real").innerHTML = tmpArray[6];
          document.getElementById("Freq_real").innerHTML = tmpArray[7];
          document.getElementById("Direction_real").innerHTML = tmpArray[8];
          document.getElementById("Speed_current").innerHTML = tmpArray[9];
          document.getElementById("Power_monitor").innerHTML = tmpArray[10];
          document.getElementById("Current_monitor").innerHTML = tmpArray[11];
          document.getElementById("Voltage_monitor").innerHTML = tmpArray[12];
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

  lcd.setCursor(0, 0);
  lcd.print("TT:");
  lcd.setCursor(3, 0);
  lcd.print("Dung");
  lcd.setCursor(0, 1);
  lcd.print("CD:");
  lcd.setCursor(6, 1);
  lcd.print("VT:");
  lcd.setCursor(8, 0);
  lcd.print("TD:");
  lcd.setCursor(3, 1);
  lcd.print(Count_Level);

  pinMode(Btn_Start, INPUT);
  pinMode(Btn_Up, INPUT);
  pinMode(Btn_Down, INPUT);

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
            lcd.setCursor(10, 1);
            lcd.print("Web");
            if (Count_Start >= 2)
            {
              Count_Start = 0;
            }
            if (Count_Start == 1)
            {
              lcd.setCursor(3, 0);
              lcd.print("Chay");
            }
            else
            {
              lcd.setCursor(3, 0);
              lcd.print("Dung");
            }
            request->send(200, "text/plain", "OK"); });

  // Send a GET request to <ESP_IP>/toggle to toggle the output
  server.on("/Request_2=Status", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
             if (Count_Start == 0)
              {
                Count_Level++;
                Count_Local = 2;
                lcd.setCursor(10, 1);
                lcd.print("Web");
                if (Count_Level >= 16)
                {
                  Count_Level = 0;
                }
                lcd.setCursor(4, 1);
                lcd.print("  ");
                lcd.setCursor(3, 1);
                lcd.print(Count_Level);
              }
            request->send(200, "text/plain", "OK"); });

  // Send a GET request to <ESP_IP>/toggle to toggle the output
  server.on("/Request_3=Status", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
            if (Count_Start == 0)
            {
              Count_Level--;
              Count_Local = 2;
              lcd.setCursor(10, 1);
              lcd.print("Web");
              if (Count_Level == 255)
              {
                Count_Level = 15;
              }
              lcd.setCursor(4, 1);
              lcd.print("  ");
              lcd.setCursor(3, 1);
              lcd.print(Count_Level);
            }
            request->send(200, "text/plain", "OK"); });

  // Send a GET request to <ESP_IP>/state to get the output state
  server.on("/get_data", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              if (Count_Start == 1)
              {
                inputStatus = "Start";
              }
              else
              {
                inputStatus = "Stop";
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

              if (directions[Count_Level] == Forward)
              {
                inputDirectionset = "Quay thuận";
              }else
              {
                inputDirectionset = "Quay Nghịch";
              }
              
              if (Trang_thai == 1)
              {
                inputTrangthai = "Sẵn sàng";
              }else
              {
                inputTrangthai = "Không sẵn sàng";
              }

              if (Chieu_quay_thuc_te == 1)
              {
                inputChieuquaythuc = "Quay thuận";
              }else if (Chieu_quay_thuc_te == 2)
              {
               inputChieuquaythuc = "Quay nghịch";
              }
              else
              {
                inputChieuquaythuc = "Dừng";
              }
              
              String response = String(inputStatus) + "|" + String(Count_Level) + "|" + String(inputLocal) + "|" + String(inputLevel)+ "|" + String(inputFreq) + "|" + String(inputDirectionset) + "|" + String(inputTrangthai) + "|" + String(Tan_so_thuc_te) + "|" + String(inputChieuquaythuc) + "|" + String(Speed_RPM) + "|" + String(Nguon) + "|" + String(Cuong_do) + "|" + String(Hieu_dien_the);
              request->send(200, "text/plain", response); });
  // Start server
  server.begin();
}

void loop()
{
  // Read the state of all buttons
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
      lcd.setCursor(10, 1);
      lcd.print("Esp32");
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
        lcd.setCursor(10, 1);
        lcd.print("Esp32");
        if (Count_Level >= 16)
        {
          Count_Level = 0;
        }
        lcd.setCursor(4, 1);
        lcd.print("  ");
        lcd.setCursor(3, 1);
        lcd.print(Count_Level);
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
        lcd.setCursor(10, 1);
        lcd.print("Esp32");
        if (Count_Level == 255)
        {
          Count_Level = 15;
        }
        lcd.setCursor(4, 1);
        lcd.print("  ");
        lcd.setCursor(3, 1);
        lcd.print(Count_Level);
      }
      delay(300); // Debounce delay
    }
    Default_Down = Value_Down;
  }

  if (flag)
  {
    if (Count_Start == 1)
    {
      inputLevel = Count_Level;
      inputFreq = frequencies[Count_Level];
      Frequency_setting(frequencies[Count_Level]);
      Select_Rotation_Direction(directions[Count_Level]);
      Control_RunStop(Run);
      lcd.setCursor(3, 0);
      lcd.print("Chay");
    }
    else
    {
      Frequency_setting(0);
      Select_Rotation_Direction(0);
      Control_RunStop(Stop);
      lcd.setCursor(3, 0);
      lcd.print("Dung");
    }
    flag = false;
  }
  else
  {
    if ((unsigned long)(millis() - timer1) > 1000)
    {
      Tan_so_thuc_te = node.readHoldingRegisters(0x1001, Slave_ID);
      Tan_so_thuc_te = node.getResponseBuffer(Tan_so_thuc_te);
      // Serial.print("Tstt: ");
      // Serial.print(Tan_so_thuc_te);
      // Serial.print(" ");
      Chieu_quay_thuc_te = node.readHoldingRegisters(0x1004, Slave_ID);
      Chieu_quay_thuc_te = node.getResponseBuffer(Chieu_quay_thuc_te);

      Cap_cuc = node.readHoldingRegisters(0x1633, Slave_ID);
      Cap_cuc = node.getResponseBuffer(Cap_cuc);

      Speed_RPM = ((Tan_so_thuc_te * 120) / pole[Cap_cuc]);
      lcd.setCursor(11, 0);
      lcd.print("     ");
      lcd.setCursor(11, 0);
      lcd.print(Speed_RPM);

      Trang_thai = node.readHoldingRegisters(0x0010U, Slave_ID);
      Trang_thai = node.getResponseBuffer(Trang_thai);

      Nguon = node.readHoldingRegisters(0x1012U, Slave_ID);
      Nguon = node.getResponseBuffer(Nguon);

      Cuong_do = node.readHoldingRegisters(0x1003U, Slave_ID);
      Cuong_do = node.getResponseBuffer(Cuong_do);

      Hieu_dien_the = node.readHoldingRegisters(0x1011U, Slave_ID);
      Hieu_dien_the = node.getResponseBuffer(Hieu_dien_the);

      //Serial.println(" ");
      timer1 = millis();
    }
  }
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
