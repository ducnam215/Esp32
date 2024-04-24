// Nhập các thư viện cần thiết
#include <Arduino.h>           // Thư viện cơ bản cho mọi dự án Arduino
#include <WiFi.h>              // Thư viện cho phép kết nối WiFi trên ESP32
#include <AsyncTCP.h>          // Thư viện hỗ trợ TCP không đồng bộ, dùng cho ESP32
#include <ESPAsyncWebServer.h> // Thư viện tạo web server không đồng bộ cho ESP32
#include <LiquidCrystal_I2C.h> // Thư viện điều khiển màn hình LCD qua giao tiếp I2C
#include <ModbusMaster.h>      // Thư viện cho phép ESP32 hoạt động như một Modbus Master
#include <Register_inverter.h> // Thư viện tự định nghĩa, chứa các định nghĩa register cho inverter

// Định nghĩa ID của thiết bị slave trong mạng Modbus
#define Slave_ID 1

// Thông tin kết nối WiFi
const char *ssid = "Mircocontroller And Simulab"; // SSID của mạng WiFi mà ESP32 sẽ kết nối
const char *password = "244466666";               // Mật khẩu của mạng WiFi

// Định nghĩa chân GPIO cho các nút bấm trên ESP32
#define Btn_Start 27 // Chân GPIO cho nút Start
#define Btn_Up 26    // Chân GPIO cho nút tăng
#define Btn_Down 25  // Chân GPIO cho nút giảm

// Biến để lưu trạng thái hiện tại của các nút bấm
uint8_t Value_Start, Value_Up, Value_Down;
uint8_t Default_Start = 1, Default_Up = 1, Default_Down = 1; // Giá trị mặc định khi nút không được nhấn
uint8_t Count_Start = 0, Count_Level = 0, inputLevel = 0;    // Đếm số lần nhấn nút và mức đầu vào
uint8_t Count_Local = 0;                                     // Biến đếm dùng để xác định vị trí đk

// Biến lưu trữ các thông số đọc từ inverter qua Modbus
uint16_t Tan_so_thuc_te, Chieu_quay_thuc_te, inputFreq; // Tần số thực tế, chiều quay thực tế và tần số đầu vào
uint16_t Cap_cuc, Speed_RPM;                            // Cấp cực và tốc độ động cơ tính bằng vòng/phút
uint16_t Trang_thai, Nguon, Cuong_do, Hieu_dien_the;    // Trạng thái hoạt động, nguồn cấp, cường độ dòng điện và hiệu điện thế

// Khai báo một mảng các tần số cho động cơ
const uint16_t frequencies[] = {
    500, 1000, 1500, 2000, 2500, 3000, 3500, 4000,
    500, 1000, 1500, 2000, 2500, 3000, 3500, 4000}; // Mảng này lưu trữ các giá trị tần số (đơn vị là Hz) mà động cơ có thể đạt được.

// Khai báo mảng chỉ định hướng quay của động cơ cho từng tần số tương ứng
const bool directions[] = {
    Forward, Forward, Forward, Forward, Forward, Forward, Forward, Forward,
    Reverse, Reverse, Reverse, Reverse, Reverse, Reverse, Reverse, Reverse}; // Mỗi phần tử "Forward" hoặc "Reverse" tương ứng với giá trị trong mảng `frequencies` chỉ định chiều quay tiến hoặc lùi.

// Khai báo một mảng chứa số cực của động cơ ứng với mỗi cấu hình
const uint8_t pole[] = {
    0, 2, 4, 6, 8, 10, 12, 14}; // Mảng này chứa thông tin về số cực của động cơ, dùng để điều chỉnh tốc độ quay.

// Biến để lưu thời gian đo để sử dụng trong các hàm hoặc vòng lặp thời gian
unsigned long timer1 = 0; // Biến này thường được dùng để lưu trữ thời điểm cuối cùng khi một sự kiện nào đó xảy ra.

// Biến cờ để kiểm soát các hoạt động hoặc trạng thái trong chương trình
bool flag = false; // Biến `flag` thường được dùng làm cờ hiệu để kiểm soát trạng thái hoặc luồng thực thi của chương trình.

// Khai báo các biến String để lưu trữ các trạng thái đầu vào từ người dùng hoặc cảm biến
String inputStatus;        // Lưu trạng thái hiện tại của hệ thống hoặc động cơ
String inputLocal;         // Lưu trữ địa điểm hoặc vị trí điều khiển
String inputDirectionset;  // Lưu trữ hướng đặt ra cho động cơ
String inputTrangthai;     // Lưu trữ trạng thái thực tế của động cơ, có thể là "Hoạt động" hoặc "Không hoạt động"
String inputChieuquaythuc; // Lưu trữ thông tin chiều quay thực tế của động cơ, tiến hoặc lùi

ModbusMaster node; // Khởi tạo một đối tượng ModbusMaster để giao tiếp với thiết bị qua giao thức Modbus.
// ModbusMaster là một class trong thư viện ModbusMaster.h, cho phép ESP32 hoạt động như một Modbus Master để giao tiếp với các thiết bị hỗ trợ Modbus.

LiquidCrystal_I2C lcd(0x27, 16, 2);
// Khởi tạo đối tượng LiquidCrystal_I2C để điều khiển màn hình LCD kết nối qua giao tiếp I2C.
// Đối số đầu tiên (0x27) là địa chỉ I2C của màn hình LCD.
// Đối số thứ hai (16) là số cột của màn hình LCD.
// Đối số thứ ba (2) là số hàng của màn hình LCD.
// LiquidCrystal_I2C là một class trong thư viện LiquidCrystal_I2C.h, giúp điều khiển màn hình LCD thông qua giao tiếp I2C.

AsyncWebServer server(80);
// Khởi tạo một đối tượng AsyncWebServer để tạo ra một máy chủ web (web server) chạy trên cổng 80.
// AsyncWebServer là một class trong thư viện ESPAsyncWebServer.h, cho phép ESP32 tạo ra các trang web không đồng bộ để tương tác với các thiết bị khác qua mạng.
// Cổng 80 (port 80) là cổng mặc định được sử dụng cho giao thức HTTP trên mạng.

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
  // Khởi tạo giao tiếp Serial với baud rate là 115200
  Serial.begin(115200);

  // Khởi tạo giao tiếp Serial2 với baud rate là 9600, cấu hình SERIAL_8N1, sử dụng chân 16 là TX và chân 17 là RX
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  // Khởi tạo đối tượng ModbusMaster (node) với địa chỉ Slave_ID và sử dụng giao tiếp Serial2
  node.begin(Slave_ID, Serial2);

  // Khởi tạo LCD và bật đèn nền
  lcd.init();
  lcd.backlight();

  // Cài đặt vị trí con trỏ của LCD và hiển thị các thông tin ban đầu
  lcd.setCursor(0, 0);
  lcd.print("TT:"); // Hiển thị "TT:" ở dòng 0, cột 0
  lcd.setCursor(3, 0);
  lcd.print("Dung"); // Hiển thị "Dung" ở dòng 0, cột 3
  lcd.setCursor(0, 1);
  lcd.print("CD:"); // Hiển thị "CD:" ở dòng 1, cột 0
  lcd.setCursor(6, 1);
  lcd.print("VT:"); // Hiển thị "VT:" ở dòng 1, cột 6
  lcd.setCursor(8, 0);
  lcd.print("TD:"); // Hiển thị "TD:" ở dòng 0, cột 8
  lcd.setCursor(3, 1);
  lcd.print(Count_Level); // Hiển thị giá trị của Count_Level ở dòng 1, cột 3

  // Thiết lập chế độ cho các chân nút nhấn là INPUT
  pinMode(Btn_Start, INPUT);
  pinMode(Btn_Up, INPUT);
  pinMode(Btn_Down, INPUT);

  // Kết nối đến mạng Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Đang kết nối đến Wi-Fi..");
  }

  // In địa chỉ IP cục bộ của ESP lên Serial Monitor
  Serial.println(WiFi.localIP());

  // Định tuyến cho trang chính / trang web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  // Gửi nội dung của trang web index_html dưới dạng tệp flash (PROGMEM)
  request->send_P(200, "text/html", index_html); });

  // Thiết lập một endpoint để nhận các yêu cầu GET đến địa chỉ <ESP_IP>/Request_1=Status để thực hiện chức năng chuyển đổi trạng thái
  server.on("/Request_1=Status", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              // Xử lý các yêu cầu GET đến địa chỉ /Request_1=Status
              Count_Start++;   // Tăng biến Count_Start lên một đơn vị
              Count_Local = 2; // Đặt biến cục bộ Count_Local thành 2
              flag = true;     // Đặt cờ flag thành true
              lcd.setCursor(10, 1);
              lcd.print("Web"); // Hiển thị thông tin "Web" lên màn hình LCD
              if (Count_Start >= 2)
              {
                Count_Start = 0; // Nếu Count_Start vượt quá 1, đặt lại về 0
              }
              if (Count_Start == 1)
              {
                lcd.setCursor(3, 0);
                lcd.print("Chay"); // Nếu Count_Start là 1, hiển thị "Chay" lên LCD
              }
              else
              {
                lcd.setCursor(3, 0);
                lcd.print("Dung"); // Ngược lại, hiển thị "Dung" lên LCD
              }
              request->send(200, "text/plain", "OK"); // Gửi phản hồi với mã trạng thái 200 OK và nội dung "OK"
            });

  // Thiết lập một endpoint để nhận các yêu cầu GET đến địa chỉ <ESP_IP>/Request_2=Status để thực hiện chức năng chuyển đổi trạng thái
  server.on("/Request_2=Status", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              // Xử lý các yêu cầu GET đến địa chỉ /Request_2=Status
              if (Count_Start == 0)
              {
                // Nếu Count_Start bằng 0, tăng mức độ (Count_Level) lên một đơn vị
                Count_Level++;
                Count_Local = 2; // Đặt biến cục bộ (Count_Local) thành 2
                lcd.setCursor(10, 1);
                lcd.print("Web"); // Hiển thị thông tin trên LCD
                if (Count_Level >= 16)
                {
                  Count_Level = 0; // Nếu mức độ (Count_Level) vượt quá 15, quay lại mức thấp nhất là 0
                }
                lcd.setCursor(4, 1);
                lcd.print("  "); // Xóa các ký tự cũ trên LCD
                lcd.setCursor(3, 1);
                lcd.print(Count_Level); // Hiển thị mức độ (Count_Level) mới trên LCD
              }
              request->send(200, "text/plain", "OK"); // Gửi phản hồi với mã trạng thái 200 OK và nội dung "OK"
            });

  // Thiết lập một endpoint để nhận các yêu cầu GET đến địa chỉ <ESP_IP>/Request_3=Status để thực hiện chức năng chuyển đổi trạng thái
  server.on("/Request_3=Status", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              // Xử lý các yêu cầu GET đến địa chỉ /Request_3=Status
              if (Count_Start == 0)
              {
                // Nếu Count_Start bằng 0, giảm mức độ (Count_Level) đi một đơn vị
                Count_Level--;
                Count_Local = 2; // Đặt biến cục bộ (Count_Local) thành 2
                lcd.setCursor(10, 1);
                lcd.print("Web"); // Hiển thị thông tin trên LCD
                if (Count_Level == 255)
                {
                  Count_Level = 15; // Nếu mức độ (Count_Level) nhỏ hơn 0, quay lại mức cao nhất (15)
                }
                lcd.setCursor(4, 1);
                lcd.print("  "); // Xóa các ký tự cũ trên LCD
                lcd.setCursor(3, 1);
                lcd.print(Count_Level); // Hiển thị mức độ (Count_Level) mới trên LCD
              }
              request->send(200, "text/plain", "OK"); // Gửi phản hồi với mã trạng thái 200 OK và nội dung "OK"
            });

  // Thiết lập một endpoint để nhận các yêu cầu GET đến địa chỉ <ESP_IP>/get_data để lấy trạng thái đầu ra
  server.on("/get_data", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              // Xử lý các yêu cầu GET đến địa chỉ /get_data
              if (Count_Start == 1)
              {
                inputStatus = "Start"; // Nếu Count_Start bằng 1, gán inputStatus là "Start"
              }
              else
              {
                inputStatus = "Stop"; // Ngược lại, gán inputStatus là "Stop"
              }

              if (Count_Local == 0)
              {
                inputLocal = "---"; // Nếu Count_Local bằng 0, gán inputLocal là "---"
              }
              else if (Count_Local == 1)
              {
                inputLocal = "Esp32 Hardware"; // Nếu Count_Local bằng 1, gán inputLocal là "Esp32 Hardware"
              }
              else if (Count_Local == 2)
              {
                inputLocal = "Web Esp32"; // Nếu Count_Local bằng 2, gán inputLocal là "Web Esp32"
              }

              if (directions[Count_Level] == Forward)
              {
                inputDirectionset = "Quay thuận"; // Nếu hướng quay là Forward, gán inputDirectionset là "Quay thuận"
              }
              else
              {
                inputDirectionset = "Quay Nghịch"; // Ngược lại, gán inputDirectionset là "Quay Nghịch"
              }

              if (Trang_thai == 1)
              {
                inputTrangthai = "Sẵn sàng"; // Nếu Trang_thai là 1, gán inputTrangthai là "Sẵn sàng"
              }
              else
              {
                inputTrangthai = "Không sẵn sàng"; // Ngược lại, gán inputTrangthai là "Không sẵn sàng"
              }

              if (Chieu_quay_thuc_te == 1)
              {
                inputChieuquaythuc = "Quay thuận"; // Nếu Chieu_quay_thuc_te là 1, gán inputChieuquaythuc là "Quay thuận"
              }
              else if (Chieu_quay_thuc_te == 2)
              {
                inputChieuquaythuc = "Quay nghịch"; // Nếu Chieu_quay_thuc_te là 2, gán inputChieuquaythuc là "Quay nghịch"
              }
              else
              {
                inputChieuquaythuc = "Dừng"; // Ngược lại, gán inputChieuquaythuc là "Dừng"
              }

              // Tạo chuỗi response chứa các thông tin cần trả về
              String response = String(inputStatus) + "|" + String(Count_Level) + "|" + String(inputLocal) + "|" + String(inputLevel) + "|" + String(inputFreq) + "|" + String(inputDirectionset) + "|" + String(inputTrangthai) + "|" + String(Tan_so_thuc_te) + "|" + String(inputChieuquaythuc) + "|" + String(Speed_RPM) + "|" + String(Nguon) + "|" + String(Cuong_do) + "|" + String(Hieu_dien_the);

              // Gửi phản hồi với mã trạng thái 200 OK và định dạng text/plain chứa chuỗi response
              request->send(200, "text/plain", response); });

  // Bắt đầu máy chủ để lắng nghe các yêu cầu HTTP
  server.begin();
}

void loop()
{
  // Đọc trạng thái của các nút bấm
  Value_Start = digitalRead(Btn_Start); // Đọc trạng thái của nút bấm Start và lưu vào biến Value_Start
  Value_Up = digitalRead(Btn_Up);       // Đọc trạng thái của nút bấm Up và lưu vào biến Value_Up
  Value_Down = digitalRead(Btn_Down);   // Đọc trạng thái của nút bấm Down và lưu vào biến Value_Down

  // Kiểm tra nút bấm Start
  if (Value_Start != Default_Start)
  {
    // Nếu giá trị nút bấm Start khác giá trị mặc định
    if (Value_Start == 1)
    {
      // Nếu nút bấm Start được nhấn
      Count_Start++;   // Tăng biến đếm số lần nhấn nút Start
      Count_Local = 1; // Đặt biến cục bộ (Count_Local) thành 1
      flag = true;     // Đặt cờ (flag) thành true
      lcd.setCursor(10, 1);
      lcd.print("Esp32"); // Hiển thị thông tin trên LCD
      if (Count_Start >= 2)
      {
        Count_Start = 0; // Nếu số lần nhấn nút Start vượt quá 1, đặt lại về 0 để lặp lại
      }
      delay(300); // Đợi để chống rebound (debounce delay)
    }
    Default_Start = Value_Start; // Lưu giá trị nút bấm Start làm giá trị mặc định để so sánh sau
  }

  // Kiểm tra nút bấm Up
  if (Value_Up != Default_Up)
  {
    // Nếu giá trị nút bấm Up khác giá trị mặc định
    if (Value_Up == 1)
    {
      // Nếu nút bấm Up được nhấn
      if (Count_Start == 0)
      {
        // Nếu chưa bắt đầu (Count_Start == 0), tăng mức độ (Count_Level) lên một đơn vị
        Count_Level++;
        Count_Local = 1; // Đặt biến cục bộ (Count_Local) thành 1
        lcd.setCursor(10, 1);
        lcd.print("Esp32"); // Hiển thị thông tin trên LCD
        if (Count_Level >= 16)
        {
          Count_Level = 0; // Nếu mức độ (Count_Level) vượt quá 15, quay lại mức độ 0
        }
        lcd.setCursor(4, 1);
        lcd.print("  "); // Xóa các ký tự cũ trên LCD
        lcd.setCursor(3, 1);
        lcd.print(Count_Level); // Hiển thị mức độ (Count_Level) mới trên LCD
      }
      delay(300); // Đợi để chống rebound (debounce delay)
    }
    Default_Up = Value_Up; // Lưu giá trị nút bấm Up làm giá trị mặc định để so sánh sau
  }

  // Kiểm tra nút bấm Down
  if (Value_Down != Default_Down)
  {
    // Nếu giá trị nút bấm Down khác giá trị mặc định
    if (Value_Down == 1)
    {
      // Nếu nút bấm Down được nhấn
      if (Count_Start == 0)
      {
        // Nếu chưa bắt đầu (Count_Start == 0), giảm mức độ (Count_Level) đi một đơn vị
        Count_Level--;
        Count_Local = 1; // Đặt biến cục bộ (Count_Local) thành 1
        lcd.setCursor(10, 1);
        lcd.print("Esp32"); // Hiển thị thông tin trên LCD
        if (Count_Level == 255)
        {
          Count_Level = 15; // Nếu mức độ (Count_Level) nhỏ hơn 0, quay lại mức cao nhất (15)
        }
        lcd.setCursor(4, 1);
        lcd.print("  "); // Xóa các ký tự cũ trên LCD
        lcd.setCursor(3, 1);
        lcd.print(Count_Level); // Hiển thị mức độ (Count_Level) mới trên LCD
      }
      delay(300); // Đợi để chống rebound (debounce delay)
    }
    Default_Down = Value_Down; // Lưu giá trị nút bấm Down làm giá trị mặc định để so sánh sau
  }

  if (flag)
  {
    // Kiểm tra nếu số lần nhấn nút Start là 1
    if (Count_Start == 1)
    {
      inputLevel = Count_Level;                           // Lưu trữ mức độ (level) được chọn
      inputFreq = frequencies[Count_Level];               // Lưu trữ tần số tương ứng với mức độ được chọn
      Frequency_setting(frequencies[Count_Level]);        // Thiết lập tần số hoạt động của động cơ với tần số tương ứng
      Select_Rotation_Direction(directions[Count_Level]); // Chọn hướng quay của động cơ với hướng tương ứng
      Control_RunStop(Run);                               // Bật hoạt động chạy của động cơ
      lcd.setCursor(3, 0);
      lcd.print("Chay"); // Hiển thị trạng thái "Chạy" lên màn hình LCD
    }
    else
    {
      Frequency_setting(0);         // Thiết lập tần số hoạt động của động cơ về 0 (dừng)
      Select_Rotation_Direction(0); // Chọn hướng quay của động cơ về 0 (không quay)
      Control_RunStop(Stop);        // Dừng hoạt động của động cơ
      lcd.setCursor(3, 0);
      lcd.print("Dung"); // Hiển thị trạng thái "Dừng" lên màn hình LCD
    }
    flag = false;
  }
  else
  {
    // Kiểm tra thời gian kể từ lần cuối cùng hàm được gọi
    if ((unsigned long)(millis() - timer1) > 1000)
    {
      // Đọc giá trị tần số thực tế từ địa chỉ holding register 0x1001 của thiết bị Slave_ID
      Tan_so_thuc_te = node.readHoldingRegisters(0x1001, Slave_ID);
      Tan_so_thuc_te = node.getResponseBuffer(Tan_so_thuc_te);

      // Đọc giá trị chiều quay thực tế từ địa chỉ holding register 0x1004 của thiết bị Slave_ID
      Chieu_quay_thuc_te = node.readHoldingRegisters(0x1004, Slave_ID);
      Chieu_quay_thuc_te = node.getResponseBuffer(Chieu_quay_thuc_te);

      // Đọc giá trị cấp cực từ địa chỉ holding register 0x1633 của thiết bị Slave_ID
      Cap_cuc = node.readHoldingRegisters(0x1633, Slave_ID);
      Cap_cuc = node.getResponseBuffer(Cap_cuc);

      // Tính toán tốc độ vòng quay theo vòng/phút (RPM) dựa trên thông số thực tế
      Speed_RPM = ((Tan_so_thuc_te * 120) / pole[Cap_cuc]);

      // Hiển thị tốc độ vòng quay trên màn hình LCD tại vị trí cụ thể
      lcd.setCursor(11, 0);
      lcd.print("     "); // Xóa các ký tự hiển thị cũ
      lcd.setCursor(11, 0);
      lcd.print(Speed_RPM); // Hiển thị tốc độ vòng quay mới

      // Đọc các thông số trạng thái của động cơ từ các holding register khác
      Trang_thai = node.readHoldingRegisters(0x0010U, Slave_ID);
      Trang_thai = node.getResponseBuffer(Trang_thai);

      Nguon = node.readHoldingRegisters(0x1012U, Slave_ID);
      Nguon = node.getResponseBuffer(Nguon);

      Cuong_do = node.readHoldingRegisters(0x1003U, Slave_ID);
      Cuong_do = node.getResponseBuffer(Cuong_do);

      Hieu_dien_the = node.readHoldingRegisters(0x1011U, Slave_ID);
      Hieu_dien_the = node.getResponseBuffer(Hieu_dien_the);

      // Cập nhật thời điểm lần gửi yêu cầu đọc mới nhất
      timer1 = millis();
    }
  }
}

// Hàm điều khiển hoạt động chạy/ dừng của động cơ
void Control_RunStop(bool Status)
{
  node.writeSingleCoil(Operation_command, Status);
}
// Hàm này được sử dụng để điều khiển chạy hoặc dừng động cơ dựa trên giá trị trạng thái (Status) được cung cấp.
// Hàm sử dụng đối tượng `node` (ModbusMaster) để ghi trạng thái vào thanh ghi (coil) tương ứng (Operation_command).

// Hàm thiết lập tần số của động cơ
void Frequency_setting(uint16_t Speed)
{
  node.writeSingleRegister(Frequency_source, Speed);
}
// Hàm này được sử dụng để thiết lập tần số hoạt động của động cơ bằng giá trị `Speed` được cung cấp.
// Hàm sử dụng đối tượng `node` (ModbusMaster) để ghi giá trị `Speed` vào thanh ghi (register) tương ứng (Frequency_source).

// Hàm chọn hướng quay của động cơ
void Select_Rotation_Direction(bool Status)
{
  node.writeSingleCoil(Rotation_direction_command, Status);
}
// Hàm này được sử dụng để chọn hướng quay (thuận/nghịch) của động cơ dựa trên giá trị trạng thái (Status) được cung cấp.
// Hàm sử dụng đối tượng `node` (ModbusMaster) để ghi trạng thái vào thanh ghi (coil) tương ứng (Rotation_direction_command).
