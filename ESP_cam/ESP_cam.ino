#include <WiFi.h>
#include <WiFiClient.h>
#include "esp_camera.h"
extern "C" {
  esp_err_t camera_enable_out_clock(camera_config_t *config);
  void camera_disable_out_clock();
}

#define CAMERA_MODEL_AI_THINKER

#define DEFAULT_SSID "kwok512"
#define DEFAULT_PASS "332032wong"

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

String ssid;
String password;
//IPAddress staticIP(192, 168, 86, 200); // Set your desired static IP address
//IPAddress gateway(192, 168, 1, 1);    // Set your network gateway IP address
//IPAddress subnet(255, 255, 255, 0);   // Set your network subnet mask

WiFiServer server(28012);
camera_config_t config;

static void blink_led(int counts){
  for(int i = 0; i < counts; i++){
    digitalWrite(4, HIGH);
    delay(100);
    digitalWrite(4, LOW);
    delay(100);
  }
}

void configInitCamera(){
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
  config.pixel_format = PIXFORMAT_JPEG; //YUV422,GRAYSCALE,RGB565,JPEG
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.fb_count = 1;

  // Select lower framesize if the camera doesn't support PSRAM
  if(psramFound()){
    config.frame_size = FRAMESIZE_QSXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10; //10-63 lower number means higher quality
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Initialize the Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  else{
    Serial.printf("Camera init succeed\n");
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QSXGA);
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 0);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
  s->set_aec2(s, 0);           // 0 = disable , 1 = enable
  s->set_ae_level(s, 0);       // -2 to 2
  s->set_aec_value(s, 300);    // 0 to 1200
  s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);       // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  s->set_bpc(s, 0);            // 0 = disable , 1 = enable
  s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  s->set_lenc(s, 1);           // 0 = disable , 1 = enable
  s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
  s->set_vflip(s, 0);          // 0 = disable , 1 = enable
  s->set_dcw(s, 1);            // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
}

int offset = 1;

void camConfig(const char *input){
  /*
  Serial.println((int)(input[offset+0]-16));
  Serial.println((int)(input[offset+11]-16));
  Serial.println((int)(input[offset+13]-16));
  Serial.println((int)(input[offset+14]-16));
  Serial.println((int)(input[offset+22]-16));
  */
  
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, (framesize_t)(input[offset+0]-16));
  s->set_brightness(s, input[offset+1]-16);     // -2 to 2
  s->set_contrast(s, input[offset+2]-16);       // -2 to 2
  s->set_saturation(s, input[offset+3]-16);     // -2 to 2
  s->set_special_effect(s, input[offset+4]-16); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_whitebal(s, input[offset+5]-16);       // 0 = disable , 1 = enable
  s->set_awb_gain(s, input[offset+6]-16);       // 0 = disable , 1 = enable
  s->set_wb_mode(s, input[offset+7]-16);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, input[offset+8]-16);  // 0 = disable , 1 = enable
  s->set_aec2(s, input[offset+9]-16);           // 0 = disable , 1 = enable
  s->set_ae_level(s, input[offset+10]-16);       // -2 to 2
  s->set_aec_value(s, (input[offset+11]-16)*5);    // 0 to 1200
  s->set_gain_ctrl(s, input[offset+12]-16);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, input[offset+13]-16);       // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)(input[offset+14]-16));  // 0 to 6
  s->set_bpc(s, input[offset+15]-16);            // 0 = disable , 1 = enable
  s->set_wpc(s, input[offset+16]-16);            // 0 = disable , 1 = enable
  s->set_raw_gma(s, input[offset+17]-16);        // 0 = disable , 1 = enable
  s->set_lenc(s, input[offset+18]-16);           // 0 = disable , 1 = enable
  s->set_hmirror(s, input[offset+19]-16);        // 0 = disable , 1 = enable
  s->set_vflip(s, input[offset+20]-16);          // 0 = disable , 1 = enable
  s->set_dcw(s, input[offset+21]-16);            // 0 = disable , 1 = enable
  s->set_colorbar(s, input[offset+22]-16);       // 0 = disable , 1 = enable
}

void setup() {
  Serial.begin(115200);

  // Wait for serial connection
  int counter = 0;
  while (!Serial && counter < 5) {
    counter++;
    delay(1000);
  }

  configInitCamera();

  ssid = DEFAULT_SSID;
  password = DEFAULT_PASS;

  // Connect to WiFi with static IP configuration
  //WiFi.config(staticIP, gateway, subnet);
  WiFi.setHostname("ESP32_CAM");
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Start the server
  server.begin();
  Serial.print("Local address at : ");
  Serial.print(WiFi.localIP());
  Serial.print("\n");
}

camera_fb_t  * fb = NULL;

void takeAndSendPhoto(WiFiClient &client){
  // Take Picture with Camera
  unsigned long imgStart = millis();
  fb = esp_camera_fb_get();
  //Serial.printf("Imaging took %d ms\n", (unsigned int)(millis()-imgStart));

  // Clear cahce
  if(!fb) {
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);
    return;
  }
  
  //Serial.print("Sending buffer with length of ");
  //Serial.println(fb->len);

  client.write((const char *)fb->buf, fb->len);
  
  //return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb); 
}

int connected = false;
int camera_rest = false;
char request[31];

void loop() {
  // Check for client connections
  WiFiClient client = server.available();
  while (client) {
    if(!connected){
      Serial.println("New client connected; Power on camera;");
      camera_enable_out_clock(&config);
      connected = true;
      //client.setTimeout(500);
    }

    // Read the request from the client
    unsigned long recvStart = millis();
    char buf = 1;
    int count = 0;
    unsigned long timeout = millis();

    while(buf != 0 && count < 30 && client){
      buf = client.read();
      if(!(count == 0 && buf != '/')){
        request[count++] = buf;
      }
    }

    if(camera_rest){
      camera_rest = false;
    }

    //Serial.printf("\n");
    client.flush();

    //Serial.println(request);
    //Serial.printf("Recv took %d ms\n", (unsigned int)(millis()-recvStart));

    // Process the request
    if (request[0] == '/') {
      unsigned long sendStart = millis();
      camConfig(request);
      camera_enable_out_clock(&config);
      takeAndSendPhoto(client);
      //camera_disable_out_clock();
      //Serial.printf("Send took %d ms\n", (unsigned int)(millis()-sendStart));
      // Capture and send an image using the OV5640 camera
    }
    else {
      Serial.println("Waiting for message");
    }
    // Close the connection
    //client.stop();
    //Serial.println("Client disconnected");
  }
  if(connected){
    Serial.println("Client disconnected; Power down camera;");
    camera_disable_out_clock();
    connected = false;
  }
  else{
    delay(100);
  }
}