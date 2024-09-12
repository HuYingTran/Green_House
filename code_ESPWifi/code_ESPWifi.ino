#include "esp_system.h"
#include "base64.h"
#include "esp_camera.h"
#include "BluetoothSerial.h"

#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";
const char* serverName = "http://your-server.com/api/upload";
 
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif
// Bluetooth Serial Object (Handle)
BluetoothSerial SerialBT;
String device_name = "ESP32-BT-Slave";

String RxBuffer = "";
char RxByte;
 
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  SerialBT.begin(device_name); //Bluetooth device name
  Serial.printf("The device with name \"%s\" is started.\nNow you can pair it with Bluetooth!\n", device_name.c_str());
}

void BT_read(){
  if (SerialBT.available()) {  // Kiểm tra nếu có dữ liệu qua Bluetooth
    uint8_t buffer[bufferSize]; // Tạo bộ đệm để nhận dữ liệu
    int bytesRead = SerialBT.readBytes(buffer, bufferSize);  // Đọc dữ liệu từ Bluetooth
  
    Serial.print("Nhận được ");
    Serial.print(bytesRead);
    Serial.println(" byte:");
    
    for (int i = 0; i < bytesRead; i++) {
      Serial.print(buffer[i], HEX);  // In từng byte nhận được dưới dạng HEX
      Serial.print(" ");
    }
    Serial.println();
  }
}
 
void loop() {
  BT_read();
}
