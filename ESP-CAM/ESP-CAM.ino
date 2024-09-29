#include "esp_camera.h"
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

#include "FS.h" // Thẻ SD ESP32 
#include "SD_MMC.h" // Thẻ SD ESP32 
#include "soc/soc.h" // Vô hiệu hóa sự cố mất điện 
#include "soc/rtc_cntl_reg.h" // Vô hiệu hóa sự cố mất điện 
#include "driver/rtc_io.h" 

#include "BluetoothSerial.h"
#include "WiFiUdp.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"
#include "Base_64.h"

const char *ssid_wifi = "HuynhTran";
const char *pass_wifi = "Huynh123";
const char *ssid_tello = "drone_tello";
const char *pass_tello = "Huynh123";

BluetoothSerial SerialBT;
uint8_t SMA[6] = {0xCC, 0xDB, 0xA7, 0x3F, 0x6F, 0x6A}; // address MAC ESP32-Wifi

WiFiUDP udp;
const char* TELLO_IP = "192.168.10.1";
const int TELLO_PORT = 8889;
const int LOCAL_PORT = 9000;

const char *serverName = "https://detect.roboflow.com/esp32-czges/1?api_key=FW1NoA7cKPA9GWYkP1SC";

unsigned long previousMillis = 0;  // Lưu thời gian lần cuối tín hiệu được phát
const unsigned long interval = 4 * 60 * 1000;  // 4 phút (4 phút * 60 giây * 1000 ms)
const unsigned long interval_wifi = 2 * 60 * 100;

float ripeness_cam;
float ripeness_tello = 0;

void setup() {
  Serial.begin(9600);
  wifi_connect_ap(ssid_wifi, pass_wifi);

  udp.begin(LOCAL_PORT);
  initCamera();
}
bool tello_wifi = true;
void wifi_connect_ap(const char *ssid, const char *pass)
{
  tello_wifi = true;
  unsigned long currentMillis = millis();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if((millis()-currentMillis)>interval_wifi){
      tello_wifi = false;
      break;
    }
  }
  Serial.println("");
  Serial.println("WiFi connected.");
}

void sendCommand( char* cmd) {
  udp.beginPacket(TELLO_IP, TELLO_PORT);
  udp.write((uint8_t*)cmd, strlen(cmd));
  udp.endPacket();
  Serial.printf("Sent command: %s\n", cmd);
}

void initCamera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  // init with high specs to pre-allocate larger buffers
  {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
}

String Photo2Base64()
{
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    return "";
  }
  String imageFile = "";
  char *input = (char *)fb->buf;
  char output[base_64_enc_len(3)];
  for (int i = 0; i < fb->len; i++)
  {
    base_64_encode(output, (input++), 3);
    if (i % 3 == 0)
      imageFile += String(output);
  }
  esp_camera_fb_return(fb);
//  esp_camera_deinit();
  return imageFile;
}

float http_robotflow(String raw)
{
  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpResponseCode = http.POST(raw);
  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println("Imgae upload successfully!");
    return json_data(response);
  }
  else
  {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }
  http.end();
  return 0;
}

float json_data(String response)
{
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, response);

  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return 0;
  }

  JsonArray predictions = doc["predictions"];
  int greenCount = 0;
  int ripeCount = 0;
  float ripeness_json = 0;

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
    ripeness_json = ripeCount*100 / (ripeCount + greenCount);
  }else{
    ripeness_json = 0;
  }
  // In kết quả ra Serial Monitor
  Serial.print("All tomatoes: ");
  Serial.print(greenCount + ripeCount);
  Serial.print("; ripe tomatoes: ");
  Serial.println(ripeCount);
  Serial.print("Ripeness of tomatoes: ");
  Serial.println(ripeness_json);
  return ripeness_json;
}

// Initialize the micro SD card
void initMicroSDCard(){
  // Start Micro SD card
  Serial.println("Starting SD Card");
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    return;
  }
}

// Take photo and save to microSD card
void takeSavePhoto(){
  for(int stt = 0; stt < 5; stt++)
  {
    // Path where new picture will be saved in SD Card
    String path = String(stt);
    Serial.printf("Picture file name: %s\n", path.c_str());
    
    // Save picture to microSD card
    fs::FS &fs = SD_MMC; 
    File file = fs.open(path.c_str(),FILE_WRITE);
    if(!file){
      Serial.printf("Failed to open file in writing mode");
    } 
    else {
      int packetSize = udp.parsePacket();
      uint8_t incomingPacket[255];
      int len;
      if (packetSize) {
        len = udp.read(incomingPacket, 255);
        if (len > 0) {
          incomingPacket[len] = 0;
        }
      }
      Serial.printf("Received: %s\n", incomingPacket);
      file.write(incomingPacket, len); // payload (image), payload length
      Serial.printf("Saved: %s\n", path.c_str());
    }
    file.close();
  }
}

void control_drone_TELLO()
{
  //control_drone
  wifi_connect_ap(ssid_tello, pass_tello);
  if(tello_wifi){
    initMicroSDCard();
    takeSavePhoto();
    caculat_ripeness();
  }else{
    Serial.println("Cant't connect Tello");
    ripeness_tello = 0;
  }
}

void caculat_ripeness(){
  for(int i = 0; i < 5; i++){
    readPhotoFromSD(String(i).c_str());
  }
  ripeness_tello/5;
}

void readPhotoFromSD(const char* path) {
  // Mở file ảnh từ thẻ SD
  File file = SD_MMC.open(path);
  
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Reading file: ");
  Serial.println(path);

  // Đọc dữ liệu ảnh
  size_t fileSize = file.size();
  Serial.printf("File size: %d bytes\n", fileSize);

  uint8_t *imageBuffer = (uint8_t*)malloc(fileSize);  // Tạo bộ nhớ tạm cho dữ liệu ảnh
  if (imageBuffer == NULL) {
    Serial.println("Failed to allocate memory for image buffer.");
    file.close();
    return;
  }

  file.read(imageBuffer, fileSize);  // Đọc dữ liệu từ file vào bộ nhớ
  Serial.println("Image data read successfully");

  // base_64
  String imageFile = "";
  char *input = (char *)imageBuffer;
  char output[base_64_enc_len(3)];
  for (int i = 0; i < fileSize; i++)
  {
    base_64_encode(output, (input++), 3);
    if (i % 3 == 0)
      imageFile += String(output);
  }

  free(imageBuffer);  // Giải phóng bộ nhớ sau khi sử dụng
  file.close();  // Đóng file sau khi đọc xong

  ripeness_tello += http_robotflow(imageFile);

  Serial.println("Done reading file.");
}

void loop() {
  unsigned long currentMillis = millis(); // Lấy thời gian hiện tại

  // Kiểm tra nếu đủ 4 phút (240000 ms)
  if (currentMillis - previousMillis >= interval/10) 
  {
    previousMillis = currentMillis;  // Cập nhật thời gian
    String image_str = Photo2Base64();
    
    float ripeness = http_robotflow(image_str);
    if(ripeness > 30){
      WiFi.disconnect(true);
      control_drone_TELLO();
      ripeness = (ripeness + ripeness_tello)/2;
      delay(1000);
      wifi_connect_ap(ssid_wifi, pass_wifi);
    }
      // send_esp_wifi(ripeness);
      if (!SerialBT.begin("ESP32-BT-Master", true))
      {
        Serial.println("An error occurred initializing Bluetooth");
        ESP.restart();
      }
      bool connected = SerialBT.connect(SMA);
      if (connected){
        Serial.println("Connected Successfully!");
        SerialBT.print(ripeness);
      }else{
        Serial.println("Can't Connect!");
        ESP.restart();
      }
      delay(1000);
      SerialBT.end();
      delay(1000);
  }
}
