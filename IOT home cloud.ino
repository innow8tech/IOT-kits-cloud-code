/**************  BLYNK TEMPLATE INFO  **************/
#define BLYNK_TEMPLATE_ID "TMPL3JaxYqQyR"
#define BLYNK_TEMPLATE_NAME "IOTHOME"
#define BLYNK_AUTH_TOKEN "utA1ueBVRx84drCAUvWJaTG0Gc6k7XZ9"

/**************  LIBRARIES  **************/
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_SHT31.h>
#include <ESPAsyncWebServer.h>

/**************  WIFI CREDENTIALS  **************/
char ssid[] = "INNOW8A12";
char pass[] = "Innow8@A12";

/**************  I2C ADDRESSES  **************/
#define RAINFALL_SENSOR_ADDRESS    0x5B
#define FLAME_SENSOR_ADDRESS       0x21
#define RELAY_ADDRESS              0x22
#define AIR_QUALITY_ADDRESS        0x23
#define MOTION_SENSOR_ADDRESS      0x24
#define EARTHQUAKE_SENSOR_ADDRESS  0x25
#define OLED_I2C_ADDRESS           0x3C

#define RAINFALL_COMMAND           0x01
#define COMMAND_REQUEST_DATA       0x01

/**************  DISPLAY & HARDWARE  **************/
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
Adafruit_SHT31 sht31 = Adafruit_SHT31();
AsyncWebServer server(80);

#define BTN_UP        18
#define BTN_DOWN      16
#define BTN_LEFT      17
#define BTN_RIGHT     4
#define BTN_CENTER    5
#define TOUCH_PIN1    32
#define TOUCH_PIN2    33
#define TOUCH_PIN3    14
#define TOUCH_PIN4    12
#define BUZZER_PIN    19

bool relayState = false;

/**************  BLYNK VIRTUAL PINS  **************/
#define VPIN_RELAY_CONTROL     V0
#define VPIN_PIR               V1
#define VPIN_FIRE              V2
#define VPIN_GAS               V3
#define VPIN_RAIN              V4
#define VPIN_EARTHQUAKE        V5
#define VPIN_TEMP              V6
#define VPIN_HUM               V7

BlynkTimer timer;

/**************  RELAY CONTROL FROM APP  **************/
BLYNK_WRITE(VPIN_RELAY_CONTROL) {
  int pinValue = param.asInt();
  controlRelay(pinValue == 1);
}

/**************  SENSOR READ FUNCTIONS  **************/
void controlRelay(bool state) {
  Wire.beginTransmission(RELAY_ADDRESS);
  Wire.write(state ? 0x01 : 0x00);
  Wire.endTransmission();
  relayState = state;
}

String getRainfallStatus() {
  Wire.beginTransmission(RAINFALL_SENSOR_ADDRESS);
  Wire.write(RAINFALL_COMMAND);
  if (Wire.endTransmission(false) != 0) return "Error";
  Wire.requestFrom(RAINFALL_SENSOR_ADDRESS, 1U);
  if (Wire.available()) {
    uint8_t val = Wire.read();
    return val ? "Rain" : "Clear";
  }
  return "--";
}

String readFlameStatus() {
  Wire.beginTransmission(FLAME_SENSOR_ADDRESS);
  Wire.write(COMMAND_REQUEST_DATA);
  if (Wire.endTransmission(false) != 0) return "Err";
  Wire.requestFrom(FLAME_SENSOR_ADDRESS, 1);
  if (Wire.available()) {
    uint8_t val = Wire.read();
    return val ? "Flame" : "Safe";
  }
  return "--";
}

uint16_t readMQ3Value() {
  Wire.beginTransmission(AIR_QUALITY_ADDRESS);
  Wire.write(COMMAND_REQUEST_DATA);
  if (Wire.endTransmission(false) != 0) return 0;
  Wire.requestFrom(AIR_QUALITY_ADDRESS, 2);
  if (Wire.available() < 2) return 0;
  return Wire.read() << 8 | Wire.read();
}

String readMotionStatus() {
  Wire.beginTransmission(MOTION_SENSOR_ADDRESS);
  Wire.write(COMMAND_REQUEST_DATA);
  if (Wire.endTransmission(false) != 0) return "Err";
  Wire.requestFrom(MOTION_SENSOR_ADDRESS, 1);
  if (Wire.available()) {
    uint8_t v = Wire.read();
    return v ? "Moved" : "None";
  }
  return "--";
}

String readEarthquakeStatus() {
  Wire.beginTransmission(EARTHQUAKE_SENSOR_ADDRESS);
  Wire.write(COMMAND_REQUEST_DATA);
  if (Wire.endTransmission(false) != 0) return "Err";
  Wire.requestFrom(EARTHQUAKE_SENSOR_ADDRESS, 1);
  if (Wire.available()) {
    uint8_t v = Wire.read();
    return v ? "Quake" : "Calm";
  }
  return "--";
}

/**************  SEND DATA TO BLYNK  **************/
void sendDataToBlynk() {
  float temp = sht31.readTemperature();
  float hum = sht31.readHumidity();
  uint16_t gas = readMQ3Value();
  String rain = getRainfallStatus();
  String fire = readFlameStatus();
  String motion = readMotionStatus();
  String quake = readEarthquakeStatus();

  Blynk.virtualWrite(VPIN_TEMP, temp);
  Blynk.virtualWrite(VPIN_HUM, hum);
  Blynk.virtualWrite(VPIN_GAS, gas);
  Blynk.virtualWrite(VPIN_RAIN, rain == "Rain" ? 1 : 0);
  Blynk.virtualWrite(VPIN_FIRE, fire == "Flame" ? 1 : 0);
  Blynk.virtualWrite(VPIN_PIR, motion == "Moved" ? 1 : 0);
  Blynk.virtualWrite(VPIN_EARTHQUAKE, quake == "Quake" ? 1 : 0);
}

/**************  DISPLAY FUNCTION  **************/
void updateDisplay() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.setCursor(0,12);
  u8g2.printf("Temp: %.1fC", sht31.readTemperature());
  u8g2.setCursor(0,24);
  u8g2.printf("Hum: %.1f%%", sht31.readHumidity());
  u8g2.setCursor(0,36);
  u8g2.printf("Rain: %s", getRainfallStatus().c_str());
  u8g2.setCursor(0,48);
  u8g2.printf("Gas: %u", readMQ3Value());
  u8g2.setCursor(0,60);
  u8g2.printf("Relay: %s", relayState ? "ON" : "OFF");
  u8g2.sendBuffer();
}

/**************  SETUP  **************/
void setup() {
  Serial.begin(115200);
  Wire.begin();

  u8g2.begin();
  if (!sht31.begin(0x44)) Serial.println("SHT31 not found!");

  pinMode(BUZZER_PIN, OUTPUT);
  controlRelay(false);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Send data every 2 seconds
  timer.setInterval(2000L, sendDataToBlynk);
}

/**************  LOOP  **************/
void loop() {
  Blynk.run();
  timer.run();
  updateDisplay();
}