#include "BluetoothSerial.h"
#include "WiFi.h"
#include "time.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

// Bluetooth Serial Object (Handle)
BluetoothSerial SerialBT;
String device_name = "ESP32-BT-Slave";

const char *ssid = "HuynhTran";
const char *password = "Huynh123";
const char *serverName = "https://detect.roboflow.com/esp32-czges/1?api_key=4NITBHY9LYoMI89e45kx";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 25200; //GMT+7 => 3600*7=25200
const int daylightOffset_sec = 0;

float ripeness = 0;

void setup()
{
  Serial.begin(115200);
  // Kết nối WiFi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  bt_Init();
}

void bt_Init(){
  SerialBT.begin(device_name); // Bluetooth device name
  Serial.printf("The device with name \"%s\" is started.\nNow you can pair it with Bluetooth!\n", device_name.c_str());
  SerialBT.register_callback(btCallback);
}

void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  if (event == ESP_SPP_SRV_OPEN_EVT)
  {
    Serial.println("Client Connected!");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
      Serial.println("Failed to obtain time");
      ESP.restart();
    }
    SerialBT.println(&timeinfo, "%H");
    SerialBT.flush();
  }
}

void http_post(String raw)
{
  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpResponseCode = http.POST(raw);
  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
    json_data(response);
  }
  else
  {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void json_data(String response)
{
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, response);

  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  JsonArray predictions = doc["predictions"];
  int greenCount = 0;
  int ripeCount = 0;

  for (JsonObject prediction : predictions)
  {
    const char *classType = prediction["class"];

    // Ckeck class and counter
    if (strcmp(classType, "green") == 0)
    {
      greenCount++;
    }
    else if (strcmp(classType, "unripe") == 0)
    {
      ripeCount++;
    }
    else if (strcmp(classType, "ripe") == 0)
    {
      ripeCount++;
    }
  }
  int sum = ripeCount + greenCount;
  if(ripeCount){
    ripeness = ripeCount / (ripeCount + greenCount);
  }else{
    ripeness = 0;
  }

  // In kết quả ra Serial Monitor
  Serial.print("Total green: ");
  Serial.println(greenCount);
  Serial.print("Total ripe: ");
  Serial.println(ripeCount);
  Serial.print("ripeness of fruit");
  Serial.println(ripeness);
}

int bt_wait = 0;
void loop()
{
  String data_bt = "";
  while (SerialBT.available()) {
    bt_wait = 1;
    char c = SerialBT.read();    // Đọc một ký tự từ SerialBT
    data_bt += c;
  }  

  if (bt_wait && data_bt > "can_not")
  {
    bt_wait = 0;
    delay(2000);
    SerialBT.end();
    Serial.println("Http post Robotflow");
    http_post(data_bt);
    delay(1000);
    bt_Init();
  }
  delay(1000);
}
