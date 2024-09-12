#include "esp_system.h"
#include "base64.h"
#include "SPIFFS.h"

#include "BluetoothSerial.h"

#include <WiFi.h>
#include <HTTPClient.h>

char str_arr[4096]={0};

const char* ssid = "HuynhTran";
const char* password = "Huynh123";
const char* serverName = "https://detect.roboflow.com/esp32-czges/1?api_key=4NITBHY9LYoMI89e45kx";

String imageBase64;

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif
// Bluetooth Serial Object (Handle)
BluetoothSerial SerialBT;
String device_name = "ESP32-BT-Slave";
 
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  SerialBT.begin(device_name); //Bluetooth device name
//  spiff_read();
//  http_post();
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

void http_post() {
  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type","application/x-www-form-urlencoded");
  int httpResponseCode = http.POST(imageBase64);
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
  } else {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void spiff_read(){
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  String str;
  File file = SPIFFS.open("/image_test.txt");
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  while (file.available()) {
    imageBase64 += (char)file.read();
  }
  file.close();
  Serial.println(imageBase64);
  
}
 
void loop() {
   BT_read();
}
