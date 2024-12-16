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

/* personal data */
const char* ssid = your_ssid;
const char* password =  your_password;
const char* api_key = your_api_key;
const char* lat = your_latitude;
const char* lon = your_longitude;

/* ---SETUP VAR--- */ 
AsyncWebServer server(80);
WiFiClient wifi_client;
HTTPClient http;

unsigned long last_call_time = 0; 
const unsigned long interval = 15 * 60 * 1000; // 15 minute period for periodic scrape
bool scraper_on = true; // calls scraper once on startup or mode switch
bool scraper_update = true; // enables periodic check for scraper

#define LED_PIN     4
#define NUM_LEDS    300
#define BRIGHTNESS  180
#define LED_TYPE    WS2812
CRGB leds[NUM_LEDS];

#define fade 15
String mode = "Off";


/* CLEAR CONFIG */
static float pulseSpeed = 0.5;  // Larger value gives faster pulse.

uint8_t hueA = 180;  // Start hue at valueMin.
uint8_t satA = 150;  // Start saturation at valueMin.
float valueMin = 180.0;  // Pulse minimum value (Should be less then valueMax).

uint8_t hueB = 210;  // End hue at valueMax.
uint8_t satB = 255;  // End saturation at valueMax.
float valueMax = 255.0;  // Pulse maximum value (Should be larger then valueMin).

uint8_t hue = hueA;  // Do Not Edit
uint8_t sat = satA;  // Do Not Edit
float val = valueMin;  // Do Not Edit
uint8_t hueDelta = hueA - hueB;  // Do Not Edit
static float delta = (valueMax - valueMin) / 2.35040238;  // Do Not Edit

/* CLOUDS CONFIG */
CRGBPalette16  lavaPalette = CRGBPalette16(
  CRGB::Gray,  CRGB::DarkGray,   CRGB::Gray,  CRGB::DarkGray,
  CRGB::Gray,  CRGB::DarkGray,   CRGB::Gray,  CRGB::Gray,
  CRGB::Gray,  CRGB::Gray,  CRGB::Blue,      CRGB::DarkBlue,
  CRGB::White,    CRGB::DarkBlue,   CRGB::Blue,      CRGB::Gray
);

uint16_t brightnessScale = 150;
uint16_t indexScale = 100;

/* RAIN CONFIG */
int8_t rain_intensity = 50;
DEFINE_GRADIENT_PALETTE(RainPal) {
  0,   182, 203, 238,
  64,  165, 190, 231,
  128, 142, 174, 227,
  192, 128, 160, 214,
  255, 104, 142, 206
};
CRGBPalette16 palette_rain = RainPal;

/* THUNDERSTORM CONFIG */
int pattern = 1;

int SPEED = 40;
int FADE = 70;
int CLUSTER = 15;
int CHANCE = 20;

/* SNOW CONFIG */
int16_t snow_intensity = 400;
int8_t snow_fallspeed = 50;


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
  server.on("/clear", [](AsyncWebServerRequest *request){
    Serial.println("switch to clear");
    request->send(200, "text/plain", "CLEAR");
    mode = "Clear";
    scraper_update = false;
  } );
  server.on("/clouds", [](AsyncWebServerRequest *request){
    Serial.println("switch to clouds");
    request->send(200, "text/plain", "CLOUDS");
    mode = "Clouds";
    scraper_update = false;
  } );
  server.on("/rain", [](AsyncWebServerRequest *request){
    Serial.println("switch to rain");
    request->send(200, "text/plain", "RAIN");
    mode = "Rain";
    scraper_update = false;
  } );
  server.on("/thunderstorm", [](AsyncWebServerRequest *request){
    Serial.println("switch to thunderstorm");
    request->send(200, "text/plain", "THUNDERSTORM");
    mode = "Thunderstorm";
    scraper_update = false;
  } );
  server.on("/snow", [](AsyncWebServerRequest *request){
    Serial.println("switch to snow");
    request->send(200, "text/plain", "SNOW");
    mode = "Snow";
    scraper_update = false;
  } );
  server.on("/scraper", [](AsyncWebServerRequest *request){
    Serial.println("scraper on");
    request->send(200, "text/plain", "SCRAPER ON");
    scraper_on = true;
    scraper_update = true;
  } );
  server.on("/off", [](AsyncWebServerRequest *request){
    Serial.println("led off");
    request->send(200, "text/plain", "OFF");
    mode = "Off";
    scraper_update = false;
  } );
  server.begin();
}


/* LED Mode Functions */
void clear() {

  float dV = ((exp(sin(pulseSpeed * millis()/2000.0*PI)) -0.36787944) * delta);
  val = valueMin + dV;
  hue = map(val, valueMin, valueMax, hueA, hueB);  // Map hue based on current val
  sat = map(val, valueMin, valueMax, satA, satB);  // Map sat based on current val

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue, sat, val);

    // You can experiment with commenting out these dim8_video lines
    // to get a different sort of look.
    leds[i].r = dim8_video(leds[i].r);
    leds[i].g = dim8_video(leds[i].g);
    leds[i].b = dim8_video(leds[i].b);
  }

}

void clouds() {
  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t brightness = inoise8(i * brightnessScale, millis() / 5);
    uint8_t index = inoise8(i * indexScale, millis() /10);
    leds[i] = ColorFromPalette(lavaPalette, index, brightness);
  }
}
void rain()
{
  EVERY_N_MILLISECONDS(rain_intensity) {
    leds[random16(0, NUM_LEDS - 1)] = ColorFromPalette(palette_rain, random8(0, 255), BRIGHTNESS);
  }
  fadeToBlackBy(leds, NUM_LEDS, 1);

}

void snow() {
   EVERY_N_MILLISECONDS(snow_intensity) {
    leds[random16(0, NUM_LEDS - 1)] = CRGB::White;
  }
  EVERY_N_MILLISECONDS(snow_fallspeed) {
    fadeToBlackBy(leds, NUM_LEDS, 2);
  }
}

void thunderstorm() {
  if (random(0, CHANCE) == 0) {
    for (int i = 0; i < random16(1, NUM_LEDS); i++) {
      int r = random16(NUM_LEDS);
      for (int j = 0; j < random(1, CLUSTER); j++) {
        if ((r + j) <  NUM_LEDS) {
          leds[(r + j)] = CHSV(random(0, 255), 100, 255);
        }
      }
    }
  }
  FastLED.delay(SPEED);
  fadeToBlackBy(leds, NUM_LEDS, FADE);
}

/* Main Loop */
void loop() {
  /*webscraper call -> every 15 min if scraper_update and once on startup or mode switch to scraper mode*/
  unsigned long current_time = millis();
  if ((current_time - last_call_time >= interval && scraper_update) || scraper_on) {
    last_call_time = current_time;
    scrape_current_weather(); 
    scraper_on = false;
  }

  /* led effect playing*/
  if (mode == "Clear") {
    clear();
  }
  else if (mode == "Clouds") {
    clouds();
  }
  else if (mode == "Rain") {
    rain();
  }
  else if (mode == "Snow") {
    snow();
  }
  else if (mode == "Thunderstorm") {
    thunderstorm();
  }
  else{ //led off
    FastLED.clear();
  }

  FastLED.show();
}