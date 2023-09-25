#include "Arduino.h"
#include "AudioTools.h"
#include "AudioCodecs/CodecMP3Helix.h"
#include <LiquidCrystal_I2C.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "WiFi.h"

TaskHandle_t playerTaskHandle = NULL;
 
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display


 
const char *ssid = "Bartolo";
const char *password = "remolacha";
const char *urls[] = {
  "https://radio01.ferozo.com/proxy/ra01000659?mp=/;",
  "https://sa.mp3.icecast.magma.edge-access.net/sc_rad39",
  "http://stream.srg-ssr.ch/m/rr/mp3_128",
  "http://sunshineradio.ice.infomaniak.ch/sunshineradio-128.mp3",
  "http://streaming.swisstxt.ch/m/drsvirus/mp3_128"
};

const char *titles[] = {
  "FM La Tribu",
  "Nacional Rock",
  "Internacional 1",
  "Internacional 2",
  "Internacional 3"
};

URLStreamBuffered urlStream(ssid, password);
AudioSourceURL source(urlStream, urls, "audio/mp3");
AnalogAudioStream out; 
AudioInfo info(44100, 2, 16);
MP3DecoderHelix decoder;
AudioPlayer player(source, out, decoder);
 
int station_index = 0;
int station_count = sizeof(urls) / sizeof(urls[0]);

//Parameters
const int clkPin  = 34;
const int dtPin  = 39;
const int swPin  = 4;

//Variables
int rotVal  = 0;
int rotMax = station_count - 1;
int rotMin = 0;
int clkState;
int clkLast;
int swState  = HIGH;
int swLast  = HIGH;


void readRotary() {
   /* function readRotary */
  ////Test routine for Rotary
  // gestion position
  clkState = digitalRead(clkPin);
  
  if (clkLast != clkState) {//rotary moving
    //Serial.print("Rotary position ");
    if (digitalRead(dtPin) != clkState) {
      rotVal++;
      if ( rotVal > rotMax ) {
        rotVal = rotMax;
      }
    }
    else {
      rotVal--;
      if ( rotVal < rotMin ) {
        rotVal = rotMin;
      }
    }
    Serial.println(rotVal);
    station_index = rotVal;
    Serial.println(urls[station_index]);
    Serial.println(titles[station_index]);
    lcd_text(titles[station_index]);
  }
  clkLast = clkState;

  //gestion bouton
  swState = digitalRead(swPin);
  if (swState == LOW && swLast == HIGH) {
    Serial.println("Rotary pressed");
    delay(100);//debounce
    ChangeRadio();

  }
  swLast = swState;
  delay(1);
}

void callbackPrintMetaData(MetaDataType type, const char* str, int len) {
  Serial.print("==> ");
  Serial.print(toStr(type));
  Serial.print(": ");
  Serial.println(str);
}

void ChangeRadio() {
  player.stop();
  vTaskSuspend(playerTaskHandle);
  source.selectStream(station_index);
  player.setAudioSource(source);
  player.play();
  vTaskResume(playerTaskHandle);

}

void Player_task(void *arg) {
  while(1){
    player.copy();
  }
}

void setup() {
  
  pinMode(clkPin,INPUT);
  pinMode(dtPin,INPUT);
  pinMode(swPin,INPUT_PULLUP);

  //Serial
  Serial.begin(115200);

  lcd.init();                      // initialize the lcd
  lcd.backlight();


  // setup output
  auto config = out.defaultConfig(TX_MODE);
  config.copyFrom(info); 
  out.begin(config);


  // setup player
  source.setMetadataCallback(callbackPrintMetaData);
  player.begin();
  lcd_text("IRadio");
  // start on core 0
  xTaskCreate(Player_task, "Player_Task", 4096, NULL, 10, &playerTaskHandle);
    
}

 
void loop() {
  readRotary();
   
}
 
 
void lcd_text(String text) {
  lcd.clear();
  lcd.setCursor(0, 0);             // Start at top-left corner
  lcd.print(text);
}

