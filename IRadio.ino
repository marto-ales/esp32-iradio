#include "Arduino.h"
#include "AudioTools.h"
#include "AudioCodecs/CodecMP3Helix.h"
#include "LiquidCrystal_I2C.h"
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
  "http://relay.relatores.com.ar/radio.mp3",
  "http://stream.srg-ssr.ch/m/rr/mp3_128",
  "http://streaming.swisstxt.ch/m/drsvirus/mp3_128",
  "http://us-b5-p-e-kj2-audio.cdn.mdstrm.com/live-audio/5942a3a05fa68cca2efb4264/5d9d019112cbbb45d6a50960/chunks.m3u8"
};

const char *titles[] = {
  "FM La Tribu",
  "Nacional Rock",
  "Relatores",
  "Internacional 1",
  "Internacional 2",
  "Futurock"
};

//URLStreamBuffered urlStream(ssid, password);
URLStreamBuffered urlStream(512);
//AudioSourceURL source(urlStream, urls, "audio/mp3");
MetaDataOutput out1;
AnalogAudioStream dac;
EncodedAudioStream out2dec(&dac, new MP3DecoderHelix()); // Decoding stream
MultiOutput out;
StreamCopy copier(out, urlStream); // copy url to decoder

 
int station_index = 0;
int station_current = 0;
int station_count = sizeof(urls) / sizeof(urls[0]);
const int station_delay = 3000;
unsigned long current_time = 0;
const char *last_name = "";

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
    if (station_index == station_current) {
      lcd_text(titles[station_index], last_name);
    }
    else {
      lcd_text(titles[station_index], NULL);
    }

    current_time = millis();
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

void printMetaData(MetaDataType type, const char* str, int len) {
  Serial.print("==> ");
  Serial.print(toStr(type));
  Serial.print(": ");
  Serial.println(str);
  if (type == 4) {
    last_name = str;
    lcd_text(titles[station_current], last_name);
  }
  
}

void ChangeRadio() {
  vTaskSuspend(playerTaskHandle);
  urlStream.end();
  current_time = millis();
  station_current = station_index;
  urlStream.begin(urls[station_current], "audio/mp3");
  out2dec.setNotifyAudioChange(dac);
  out1.end();
  out1.setCallback(printMetaData);
  out1.begin(urlStream.httpRequest());

  vTaskResume(playerTaskHandle);

}

void Player_task(void *arg) {
  while(1) {
    if (copier.available()) {
      copier.copy();
    }
  }
}

void wifi_setup() {
  Serial.print("Conectando a ");
  Serial.println(ssid);
  lcd_text("Conectando...", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi conectado.");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
  lcd_text("WiFi OK", WiFi.localIP().toString().c_str());
  delay(500);
}

void setup() {
  
  pinMode(clkPin,INPUT);
  pinMode(dtPin,INPUT);
  pinMode(swPin,INPUT_PULLUP);

  //Serial
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);
  lcd.init();                      // initialize the lcd
  lcd.backlight();

  wifi_setup();
  

  // setup multi output
  out.add(out1);
  out.add(out2dec);

  urlStream.begin(urls[0],"audio/mp3");
    // setup metadata
  out1.setCallback(printMetaData);
  out1.begin(urlStream.httpRequest());



  // setup output
  auto config = dac.defaultConfig(TX_MODE);
  dac.begin(config);



  // setup I2S based on sampling rate provided by decoder
  out2dec.setNotifyAudioChange(dac);
  out2dec.begin();



  // start on core 0
  xTaskCreatePinnedToCore(Player_task, "Player_Task", 4096, NULL, 10, &playerTaskHandle,0);
  
}


void loop() {
  readRotary();
  waitAndShowCurrent();
}

void waitAndShowCurrent() {
  if (millis() > current_time + station_delay) {
    if (station_index != station_current) {
      station_index = station_current;
      rotVal = station_index;
      lcd_text(titles[station_current], last_name);
    }
  }
}
 
 
void lcd_text(const char* line1, const char* line2) {
  lcd.clear();
  lcd.setCursor(0, 0);             // Start at top-left corner
  lcd.print(line1);
  lcd.setCursor(0, 1);             // Start at top-left corner
  lcd.print(line2);
}

