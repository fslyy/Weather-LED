#include <FastLED.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h>
#include <iostream>
#include <ArduinoJson.h>
#include <String>
#include "index.h"
#include "config.h" //import config file for personal data | REMOVE IF NOT NEEDED

/* ---SETUP VAR--- */ 
AsyncWebServer server(80);
WiFiClient wifi_client;
HTTPClient http;
/* personal data */
const char* ssid = your_ssid;
const char* password =  your_password;
const char* api_key = your_api_key;
const char* lat = your_latitude;
const char* lon = your_longitude;

unsigned long last_call_time = 0; 
const unsigned long interval = 15 * 60 * 1000; 
bool scraper_on = true; // calls scraper once on startup or mode switch
bool scraper_update = true; // enables periodic check for scraper

#define LED_PIN     4
#define NUM_LEDS    300
#define BRIGHTNESS  180
#define LED_TYPE    WS2812
CRGB leds[NUM_LEDS];

#define fade 15
String mode = "Off";

int8_t rain_intensity = 50;
int8_t inc = 0;

DEFINE_GRADIENT_PALETTE(RainPal) {
  0,   182, 203, 238,
  64,  165, 190, 231,
  128, 142, 174, 227,
  192, 128, 160, 214,
  255, 104, 142, 206
};

CRGBPalette16 palette_rain = RainPal;

CRGBPalette16  lavaPalette = CRGBPalette16(
  CRGB::DarkRed,  CRGB::Maroon,   CRGB::DarkRed,  CRGB::Maroon,
  CRGB::DarkRed,  CRGB::Maroon,   CRGB::DarkRed,  CRGB::DarkRed,
  CRGB::DarkRed,  CRGB::DarkRed,  CRGB::Red,      CRGB::Orange,
  CRGB::White,    CRGB::Orange,   CRGB::Red,      CRGB::DarkRed
);

uint16_t brightnessScale = 150;
uint16_t indexScale = 100;

/* Webscraper function*/
void scrape_current_weather() {
  char api_url[256]; 
  snprintf(api_url, sizeof(api_url),
  "http://api.openweathermap.org/data/2.5/weather?lat=%s&lon=%s&appid=%s&units=metric",
  lat, lon, api_key);
  if (!http.begin(wifi_client, api_url)) {
    Serial.println("Failed to initialize HTTP client.");
    return;
  }
  yield();
  http.setTimeout(10000); 

  int http_code = http.GET(); 

  if (http_code > 0) {
    if (http_code == HTTP_CODE_OK) {
      JsonDocument weather_data;

      DeserializationError error = deserializeJson(weather_data, http.getStream());

      if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
      } else {
        if (!weather_data["weather"][0]["main"].isNull()) {
          const char* weather = weather_data["weather"][0]["main"];

          Serial.print("Weather: ");
          Serial.println(weather);
          mode = (String) weather;
        } else {
          Serial.println("Required fields are missing in the JSON response.");
        }
      }
    } else {
      Serial.printf("HTTP GET error: %d\n", http_code);
    }
  } else {
    Serial.printf("HTTP GET failed: %s\n", http.errorToString(http_code).c_str());
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  /*LED intialization*/
  FastLED.addLeds<LED_TYPE, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalPixelString); //.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  std::srand(static_cast<unsigned int>(time(0)));

  /*Webserver initialization*/
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WLAN ...");

  while (WiFi.status() != WL_CONNECTED) {  
          delay(500);
          Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP-Adress: ");
  Serial.println(WiFi.localIP());

  /* Async Webserver Handling */
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Does not exist");
  });
  server.on("/index", [](AsyncWebServerRequest *request){
    String s = MAIN_page; 
    request->send(200, "text/html", s); 
  });
  server.on("/off", [](AsyncWebServerRequest *request){
    Serial.println("led off");
    request->send(200, "text/plain", "OFF");
    mode = "Off";
    scraper_update = false;
  } );
  server.on("/scraper", [](AsyncWebServerRequest *request){
    Serial.println("scraper on");
    request->send(200, "text/plain", "SCRAPER ON");
    scraper_on = true;
    scraper_update = true;
  } );
  server.on("/rain", [](AsyncWebServerRequest *request){
    Serial.println("switch to rain");
    request->send(200, "text/plain", "RAIN");
    mode = "Rain";
    scraper_update = false;
  } );
  server.on("/snow", [](AsyncWebServerRequest *request){
    Serial.println("switch to snow");
    request->send(200, "text/plain", "SNOW");
    mode = "Snow";
    scraper_update = false;
  } );
  server.on("/clear", [](AsyncWebServerRequest *request){
    Serial.println("switch to clear");
    request->send(200, "text/plain", "CLEAR");
    mode = "Clear";
    scraper_update = false;
  } );
  server.begin();
}


/* LED Mode Functions */
void rain(int rain_intensity)
{
  // --- Animation 1 ---
  EVERY_N_MILLISECONDS(rain_intensity) {
    leds[random16(0, NUM_LEDS - 1)] = ColorFromPalette(palette_rain, random8(0, 255), BRIGHTNESS);
  }
  fadeToBlackBy(leds, NUM_LEDS, 1);

// --- Animation 2 ---

}

void snow() {
   EVERY_N_MILLISECONDS(400) {
    leds[random16(0, NUM_LEDS - 1)] = CRGB::White;
  }
  EVERY_N_MILLISECONDS(50) {
    fadeToBlackBy(leds, NUM_LEDS, 2);
  }
}

void clear() {


  for (int i = 0; i < NUM_LEDS; i++) {
      uint8_t brightness2 = inoise8(i * brightnessScale, millis() / 5);
      uint8_t index2 = inoise8(i * indexScale, millis() /10);
      leds[i] = ColorFromPalette(lavaPalette, index2, brightness2);
    }
}

/* Main Loop */
void loop() {
  /*webscraper call -> every 15 min if scraper_update and once on startup or mode switch to scraper mode*/
  unsigned long current_time = millis();
  if (current_time - last_call_time >= interval && scraper_update || scraper_on) {
    last_call_time = current_time;
    scrape_current_weather(); 
    scraper_on = false;
  }

  /* led effect playing*/
  if (mode == "Snow") {
    snow();
  }
  else if (mode == "Clear") {
    clear();
  }
  else if (mode == "Rain") {
    rain(rain_intensity);
  }
  else if (mode == "Clouds") {
    clear();
  } //TO-DO: implement missing weather modes
  else{ //led off
    FastLED.clear();
  }

  FastLED.show();
}