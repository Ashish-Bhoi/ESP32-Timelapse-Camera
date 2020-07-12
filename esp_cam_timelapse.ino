/*
Project       :- ESP-32 Timelapse Camera

Description   :- Implementation of Timelapse camera using ESP-32 Microcontroller with ESP-32 Deep sleep mode Enbaled
                 Deep Sleep fetuare Save the battery usage by putting ESP-32 board into Sleep mode.
                 Deep sleep mode consumes less than 7 microAmperes of current 
                 Normal mode consumes power in milliAmperes, So deep sleep is very battery friendly.

Task Details  :- 1)Take Pictures from ESP-32 CAM.
                 2)Save picture into SD card in searilly fashion.
                 3)Enabled Deep sleep to save battery power.

Date          :- 22/05/2020

Author        :- Ashish Bhoi
*/

/*Accessing the SD card*/
#include "FS.h"                
#include "SD_MMC.h"            

/* Disable brownour problems  */
#include "soc/soc.h"           
#include "soc/rtc_cntl_reg.h"  
#include "driver/rtc_io.h"

/* Access EEPROM for serially saving picture number(or ID) Even after Deep sleep */
#include <Preferences.h>

#include "esp_camera.h"

/* Define time ESP-32 will sleep in seconds */
/* Change this parameter according to your requirements */
#define TIME_TO_SLEEP  10


/* Initializing a instance of preferences */
Preferences preferences;

/* Pin definition for CAMERA_MODEL_AI_THINKER */
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

/* Multiplication factor to convert seconds into microseconds */
/* DONT CHANGE THIS PARAMETERS (IT IS CONSTANT) */
#define uS_TO_S_FACTOR 1000000

void setup()
{
  /* for printing error messages */
  Serial.begin(115200);
  
  /* disable brownout detection */
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  /* Configuring Camera Settings */
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

  /* Image quality can be changed here */
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  /* Init Camera  */
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed");
    return;
  }
  
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    return;
  }

  /* 
     Open Preferences with my-app namespace. Each application module, library, etc
     has to use a namespace name to prevent key name collisions. We will open storage in
     RW-mode (second parameter has to be false).
     Note: Namespace name is limited to 15 chars.
  */
  preferences.begin("my-app", false);

  /* Remove all preferences under the opened namespace  */
  //preferences.clear();

  /* Or remove the counter key only */
  //preferences.remove("counter");
}
  
void loop()
{
  /* Init camera instance */
  camera_fb_t *pic = NULL;
  /* 
     Get the counter value, if the key does not exist, return a default value of 0
     Note: Key name is limited to 15 chars.
   */
  unsigned int counter = preferences.getUInt("counter", 0);

  /* Take picture with camera */
  pic = esp_camera_fb_get();

  /* Check if we get a picture */
  if(!pic)
  {
    Serial.println("Capture Failed...");
    return;
  }

  /* Initialize the path of picture*/
  String pic_Path = "/pictures"+String(counter)+".jpg";

  fs::FS &fs = SD_MMC;
//  Serial.printf("Picture file name: %s\n", path.c_str());
  
  File file = fs.open(pic_Path.c_str(), FILE_WRITE);
  if(!file)
  {
    Serial.println("Failed to open file in writing mode");
  } 
  else 
  {
    /* payload (image), payload length  */
    file.write(pic->buf, pic->len); 
    /* Increment the Image counter */
    counter++;
    /* Store the counter to the Preferences*/
    preferences.putUInt("counter", counter);   
  }  
  /* Close the file after writing image data to it */
  file.close();  
  esp_camera_fb_return(pic);
   
  /* Enable esp-32 deepsleep Timer for a time configured above */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  Serial.flush(); 

  /* Putting esp-32 into deepsleep */
  esp_deep_sleep_start();
}
/* End of File */
