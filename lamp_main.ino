// This code was developed for the DOIT ESP32 DEVKIT V1

#include <Arduino_JSON.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoHttpClient.h>
#include <Adafruit_NeoPixel.h>

#define BRIGHT 13
#define DIM 12
#define COLOR 27
#define TIMEPLUS 26
#define TIMEMINUS 25
#define SENSOR 5
#define RING1PIN 15 
#define RING2PIN 2
#define NUMPIXELS1 24
#define NUMPIXELS2 16
// SENSORBUTTON is an additional button that can be used for anything else, in this code it is used to override the sensor 
#define SENSORBUTTON 14 

// replace Wi-Fi credentials with own
#define SSID "YOUR_SSID_GOES_HERE"
#define PASS "YOUR_PASSWORD_GOES HERE"

// see openweathermap.org for instructions
String city = "CITY";
String country = "COUNTRY_CODE";
String key = "YOUR_OPENWEATHERMAP_KEY";

String serverName = "api.openweathermap.org"; 
String serverPath = "/data/2.5/weather?q=" + city + "," + country + "&units=metric&APPID=" + key;

unsigned long lastWeatherUpdate = 0;
unsigned long lastPresence = 0;
unsigned long weatherDelay = 1000;

unsigned long timer = 5000;                 // default lamp on-time in [ms]
unsigned long dur[3] = {2000, 5000, 10000}; // lemp time presets [ms]
unsigned long b = 1;                        // default brightness level (1-10)

bool lampOn = true;
bool isWarm = false;
int weatherID;
int duration = 1;
String jsonBuffer;
int port = 443;

Adafruit_NeoPixel ring(NUMPIXELS1, RING1PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel lamp(NUMPIXELS2, RING2PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  pinMode(BRIGHT, INPUT_PULLUP);
  pinMode(DIM, INPUT_PULLUP);
  pinMode(COLOR, INPUT_PULLUP);
  pinMode(TIMEPLUS, INPUT_PULLUP);
  pinMode(TIMEMINUS, INPUT_PULLUP);
  pinMode(SENSORBUTTON, INPUT_PULLUP);
  pinMode(SENSOR, INPUT_PULLDOWN);

  ring.begin();
  lamp.begin();
  Serial.begin(115200);

  WiFi.begin(SSID,PASS);
  Serial.print("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    blinkRed();
  }
  Serial.println("\nConnected");
  connected();
}

void loop() {
  int presenceOverride = digitalRead(SENSORBUTTON);
  int dimPress = digitalRead(DIM);
  int brightPress = digitalRead(BRIGHT);
  int lampColorPress = digitalRead(COLOR);
  int timePlusPress = digitalRead(TIMEPLUS);
  int timeMinusPress = digitalRead(TIMEMINUS);
  int presence = digitalRead(SENSOR);

  // Check for brightness - press

  if (!dimPress && (b > 1)) {
    b--;
    if (lampOn) {
      updateRing();
      updateLamp();
    }
    Serial.println(b);
    delay(250);
  }

  // Check for brightness + press

  if (!brightPress && (b < 10)) {
    b++;
    if (lampOn) {
      updateRing();
      updateLamp();
    }
    Serial.println(b);
    delay(250);
  }

  // Check for lamp color press

  if (!lampColorPress) {
    if (isWarm) {
      isWarm = false;
    } else {
      isWarm = true;
    }
    updateLamp();
    Serial.println(isWarm);
    delay(250);
  }

  // Check for timer + press

  if (!timePlusPress) {
    if(duration <= 1) {
      duration++;
      timer = dur[duration];
      Serial.println(timer);
    }
    for (int i = 0; i <= duration; i++) {
      ring.clear();
      ring.show();
      delay(500);
      for (int i = 0; i<NUMPIXELS1; i++){
        ring.setPixelColor(i, ring.Color(255*b/10,255*b/10,255*b/10));
      }
      ring.show();
      delay(500);
    }
    updateLamp();
    updateRing();
    delay(250);
  }

  // Check for timer - press

  if (!timeMinusPress) {
    if(duration >= 1) {
      duration--;
      timer = dur[duration];
      Serial.println(timer);
    }
    for (int i = 0; i <= duration; i++) {
      ring.clear();
      ring.show();
      delay(500);
      for (int i = 0; i<NUMPIXELS1; i++){
        ring.setPixelColor(i, ring.Color(255*b/10,255*b/10,255*b/10));
      }
      ring.show();
      delay(500);
    }
    updateLamp();
    updateRing();
    delay(250);
  }

  // Check for presence

  if ((presence || !presenceOverride) && ((millis() - lastPresence) >= 5000)) {
    lastPresence = millis();
    if (!lampOn) {
      updateLamp();
      updateRing();
      lampOn = true;
    }
    // Serial.println("trig"); // for debugging
    delay(250);
  } 

  // Turn off lamp after time passed

  if (lampOn && (millis() - lastPresence >= timer)) {
    lamp.clear();
    lamp.show();
    ring.clear();
    ring.show();
    lampOn = false;
  }

  // Get weather info

  if ((millis() - lastWeatherUpdate) >= weatherDelay) {
    if(WiFi.status()== WL_CONNECTED){
      
      jsonBuffer = httpGETRequest(serverPath);
      Serial.println(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
    
      String weatherCondition = (const char*)myObject["weather"][0]["main"];
      weatherID = myObject["weather"][0]["id"];
      Serial.print("Weather: ");
      Serial.println(weatherCondition);

      updateRing();
      weatherDelay = 3600000;
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastWeatherUpdate = millis();
  }
}



String httpGETRequest(const String& path) {

  WiFiClientSecure wifiClient;
  wifiClient.setInsecure();
  HttpClient httpClient = HttpClient(wifiClient, serverName, port);

  httpClient.beginRequest();
  httpClient.get(path);
  httpClient.endRequest();

  int statusCode = httpClient.responseStatusCode();
  String responseBody = httpClient.responseBody();

  Serial.print("HTTP Status Code: ");
  Serial.println(statusCode);

  if (statusCode != 200) {
    Serial.println("Failed to fetch data!");
    return "";
  }

  return responseBody;
}

void updateRing() {
  if (weatherID < 300) {
        setThunder();
      } else if ((weatherID < 700) && (weatherID >= 600)) {
        setWhite();
      } else if (((weatherID < 600) && (weatherID >= 500)) || ((weatherID >= 300) && (weatherID < 400))) {
        setBlue();
      } else if (weatherID == 800) {
        setYellow();
      } else if ((weatherID < 900) && (weatherID > 800)) {
        setLightBlue();
      } else if ((weatherID < 800) && (weatherID >= 700)) {
        setFog();
      } else {
        ring.clear();
        ring.show();
      }
}

void updateLamp() {
  if (isWarm) {
    lampWarm();
  } else {
    lampCold();
  }
}

void lampCold() {
  for (int i = 0; i<NUMPIXELS2; i++){
        lamp.setPixelColor(i, ring.Color(255*b/10,255*b/10,255*b/10));
      }
      lamp.show();
}

void lampWarm() {
  for (int i = 0; i<NUMPIXELS2; i++){
        lamp.setPixelColor(i, ring.Color(255*b/10,255*b/10,63*b/10));
      }
      lamp.show();
}

void setBlue() {
  ring.clear();
  for (int i = 0; i<NUMPIXELS1; i++){
    ring.setPixelColor(i, ring.Color(0,0,255*b/10));
  }
  ring.show();
}

void setYellow() {
  ring.clear();
  for (int i = 0; i<NUMPIXELS1; i++){
    ring.setPixelColor(i, ring.Color(255*b/10,255*b/10,0));
  }
  ring.show();
}

void setWhite() {
  ring.clear();
  for (int i = 0; i<NUMPIXELS1; i++){
    ring.setPixelColor(i, ring.Color(255*b/10,255*b/10,255*b/10));
  }
  ring.show();
}

void setLightBlue() {
  ring.clear();
  for (int i = 0; i<NUMPIXELS1; i++){
    ring.setPixelColor(i, ring.Color(0,0,255*b/10));
    //ring.setPixelColor(i, ring.Color(127*b/10,127*b/10,255*b/10));
  }
  ring.show();
}

void setThunder() {
  ring.clear();
  for (int i = 0; i<NUMPIXELS1; i++){
    if (i%2 == 0) {
      ring.setPixelColor(i, ring.Color(0,0,255*b/10));
    } else {
      ring.setPixelColor(i, ring.Color(255*b/10,255*b/10,0));
    }
  }
  ring.show();
}

void setFog() {
  ring.clear();
  for (int i = 0; i<NUMPIXELS1; i++){
    ring.setPixelColor(i, ring.Color(255*b/10,0,255*b/10));
  }
  ring.show();
}

void blinkRed() {
  ring.clear();
  ring.show();
  delay(1000);
  for (int i = 0; i<NUMPIXELS1; i++){
    ring.setPixelColor(i, ring.Color(255*b/10,0,0));
  }
  ring.show();
  delay(1000);
}

void connected() {
  for (int i=0; i<3; i++) {
    ring.clear();
    ring.show();
    delay(250);
    for (int i = 0; i<NUMPIXELS1; i++){
      ring.setPixelColor(i, ring.Color(0,255*b/10,0));
    }
    ring.show();
    delay(250);
  }
}
