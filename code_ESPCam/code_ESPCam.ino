#include "esp_camera.h"
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"
#include "BluetoothSerial.h"
#include "Base_64.h"

uint8_t SMA[6] = {0xCC, 0xDB, 0xA7, 0x3F, 0x6F, 0x6A}; // address MAC ESP32-Wifi
String myName = "ESP32-BT-Master";
BluetoothSerial SerialBT;

#define uS_TO_S_FACTOR 3600000000 // ms->hour

void setup()
{
  Serial.begin(115200);

  initCamera();
  delay(500);

  BT_init();
  BT_connect();
}

void BT_init()
{
  if (!SerialBT.begin(myName, true))
  {
    Serial.println("An error occurred initializing Bluetooth");
    ESP.restart();
  }
  else
  {
    Serial.println("Bluetooth initialized");
  }
  SerialBT.register_callback(btCallback);
}

void BT_connect()
{
  bool connected;
  connected = SerialBT.connect(SMA);
  if (connected)
  {
    Serial.println("Connected Successfully!");
  }
  else
  {
    Serial.println("Can't Connect!");
    ESP.restart();
  }
}

void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  if (event == ESP_SPP_SRV_OPEN_EVT)
  {
    Serial.println("Connected!");
  }
  else if (event == ESP_SPP_DATA_IND_EVT)
  {
    String stringRead = String(*param->data_ind.data);
    int hour = stringRead.toInt() - 48;
    send_image_robot_flow(hour);
    calculate_time_sleep(hour);
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
      imageFile += urlencode(String(output));
  }
  esp_camera_fb_return(fb);
  esp_camera_deinit();
  return imageFile;
}

String urlencode(String str)
{
  const char *msg = str.c_str();
  const char *hex = "0123456789ABCDEF";
  String encodedMsg = "";
  while (*msg != '\0')
  {
    if (('a' <= *msg && *msg <= 'z') || ('A' <= *msg && *msg <= 'Z') || ('0' <= *msg && *msg <= '9') || *msg == '-' || *msg == '_' || *msg == '.' || *msg == '~')
    {
      encodedMsg += *msg;
    }
    else
    {
      encodedMsg += '%';
      encodedMsg += hex[(unsigned char)*msg >> 4];
      encodedMsg += hex[*msg & 0xf];
    }
    msg++;
  }
  return encodedMsg;
}

void capture()
{
  Serial.println("Encode base64");
  String encodedString = Photo2Base64();
  Serial.println("Send image base64 by Bluetooth");
  SerialBT.println(encodedString);
  SerialBT.flush();
}

void send_image_robot_flow(int time_now)
{
  if (time_now == 6 || time_now == 11 || time_now == 16)
  {
    initCamera();
    delay(500);

    capture();
    delay(1000);
  }else{
    Serial.println("It's not time to work yet");
  }
  delay(100);
}

void calculate_time_sleep(int time_now)
{
  int time_sleep = 0;
  if (time_now < 6)
  {
    time_sleep = 6 - time_now;
  }
  else if (time_now < 11)
  {
    time_sleep = 11 - time_now;
  }
  else if (time_now < 16)
  {
    time_sleep = 16 - time_now;
  }
  else
  {
    time_sleep = 6 + 24 - time_now;
  }
  esp_sleep_enable_timer_wakeup(time_sleep * uS_TO_S_FACTOR);
  delay(100);
  esp_deep_sleep_start();
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
  if (psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
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

void loop(){}
