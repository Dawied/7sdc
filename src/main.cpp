/*
 * Todo:
 * dim at night
 * more color effects, soft glow
 * make updateTime periodic
 * done button for color change and effects
 * done mode for starting wifimanager
 * done mode for time set
 */
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h> 
#include <time.h>
#include <OneButton.h>
#include <arduino-timer.h>

 /*
  * Debug
  */
unsigned long currentMillis = 0;
unsigned long lastDebugMillis = 0;
unsigned long debugInterval = 60000;

#define DEBUG 0
#if DEBUG
#define DEBUG_PRINT(x)     Serial.print(x)
#define DEBUG_PRINTLN(x)   Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

/*
 * WiFi
*/
WiFiManager wifiManager;

/*
 * Button
 */
#define PIN_MODE_BTN D6
#define PIN_FX_BTN D7

enum ClockMode {
  CM_SETUP,
  CM_TIME
};

enum SetupMode {
  SM_HOURS,
  SM_MINUTES,
  SM_WIFI
};
const int NUM_SETUP_MODES = 3;

ClockMode clockMode = CM_TIME;
SetupMode setupMode = SM_HOURS;

OneButton modeBtn;
OneButton fxBtn;


/*
 * Time
 * https://werner.rothschopf.net/202011_arduino_esp8266_ntp_en.htm
 */

#define NTP_SERVER "at.pool.ntp.org"           
#define TIMEZONE "CET-1CEST,M3.5.0/02,M10.5.0/03"  

time_t now; //seconds since Epoch (1970) - UTC
tm tms; // the structure tm holds time information in a more convenient way

unsigned long lastUpdateTimeMillis = 0;
unsigned long updateTimeInterval = 2000;

/*
 *  Clock
 */
#define NUMSEGS_DIGIT 7
#define NUMSEGS_DOTS 2
#define NUMLEDS 30
#define PIN_LEDSTRIP D1                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            

Adafruit_NeoPixel ledStrip(NUMLEDS, PIN_LEDSTRIP, NEO_GRB + NEO_KHZ800);
//Adafruit_NeoPixel testStrip(1, D3, NEO_GRB + NEO_KHZ800);

struct Display {
  int ledOffset;
  char curChar;
  bool on;
  bool blink;
};

enum DisplayNum {
  DP_HOURS10,
  DP_HOURS1,
  DP_DOTS,
  DP_MINUTES10,
  DP_MINUTES1,
};


enum LedOffset {
  LO_HOURS10 = 0,
  LO_HOURS1 = 7,
  LO_DOTS = 14,
  LO_MINUTES10 = 16,
  LO_MINUTES1 = 23,
};

Display displays[] = {
  { LO_HOURS10, ' ', true, false},
  { LO_HOURS1, ' ', true, false},
  { LO_DOTS, ' ', true, false},
  { LO_MINUTES10, ' ', true, false},
  { LO_MINUTES1, ' ', true, false}
};

const int DISPLAYCOUNT = 5;

struct CharMap {
  char ch;
  byte segs;
};

// Common cathode configuration
//  -- a --
// |       |
// f       b
// |       |
//  -- g --
// |       |
// e       c
// |       |
//  -- d --
const CharMap segMap[] = {
  {'0', 0x3F}, {'1', 0x06}, {'2', 0x5B}, {'3', 0x4F}, {'4', 0x66},
  {'5', 0x6D}, {'6', 0x7D}, {'7', 0x07}, {'8', 0x7F}, {'9', 0x6F},
  {'A', 0x77}, {'B', 0x7C}, {'C', 0x39}, {'D', 0x5E}, {'E', 0x79},
  {'F', 0x71}, {'G', 0x3D}, {'H', 0x76}, {'I', 0x06}, {'J', 0x1E},
  {'L', 0x38}, {'N', 0x37}, {'O', 0x3F}, {'P', 0x73}, {'Q', 0x67},
  {'R', 0x50}, {'S', 0x6D}, {'T', 0x78}, {'U', 0x3E}, {'Y', 0x6E},
  {'Z', 0x5B}, {'-', 0x40}, {'_', 0x08}, {'=', 0x48}, {' ', 0x00}
};

const uint32_t rainbowColors[30] = {
    0xFF0000, 0xFF3300, 0xFF5500, 0xFF7700, 0xFF9900,
    0xFFAA00, 0xFFBB00, 0xFFCC00, 0xFFDD00, 0xFFEE00,
    0xEEFF00, 0xCCFF00, 0xAAFF00, 0x88FF00, 0x66FF00,
    0x44FF00, 0x22FF00, 0x00FF00, 0x00CC44, 0x009988,
    0x0066CC, 0x0033FF, 0x2200FF, 0x4400FF, 0x6600FF,
    0x8800FF, 0xAA00FF, 0xCC00FF, 0xEE00FF, 0xFF00FF
};

bool showRainbowColors = false;

auto timer = timer_create_default();
Timer<1>::Task blinkTask;
bool showSetupFeedback = false;

/*
 * Function declarations
*/
void setupLeds();
void setupWiFiManager();
void startWiFiManager();
void stopWiFiManager();
void setupTime();
void setupButton();
void displayChar(int displayNum, char ch);
bool blinkDisplays(void*);
void allDisplaysOn();
void testSegments(int displayNum);
byte getCharSegments(char c);
void setDisplay();
void displayTime();
void displayDots();
void updateTime();
void incTime(int minutesInc, int hoursInc);
void printTime();
void processSerialCommand();
uint32_t hexColorToInt(String hexColor);

uint32_t displayColor = hexColorToInt("#FF0000");
int brightness = 90;
int brightnessDay = 90;
int brightnessNight = 20;

void setup() {
#if DEBUG
  Serial.begin(115200);
  while (!Serial) {}
#endif
  setupWiFiManager();
  setupTime();
  setupLeds();
  setupButton();
}

void loop() {
  timer.tick();

  processSerialCommand();

  currentMillis = millis();
  if (currentMillis - lastUpdateTimeMillis >= updateTimeInterval) {
    lastUpdateTimeMillis = currentMillis;
    updateTime();
    setDisplay();
  }

  modeBtn.tick();
  fxBtn.tick();

  currentMillis = millis();
  if (currentMillis - lastDebugMillis >= debugInterval) {
    lastDebugMillis = currentMillis;
    printTime();
  }

  delay(50);
}

/*
 * Clock
 */
void setupLeds() {
  ledStrip.setBrightness(brightness);
  ledStrip.begin();
  ledStrip.clear();
  ledStrip.show();

  /*
  testStrip.setBrightness(brightness);
  testStrip.begin();
  testStrip.clear();
  testStrip.setPixelColor(0, hexColorToInt("#FF0000"));
  testStrip.show();
  */
}

void setDisplay() {
  // show setup feedback for hours once when in setup mode
  if (clockMode == CM_SETUP && setupMode == SM_HOURS && showSetupFeedback) {
    displayChar(DP_HOURS10, '_');
    displayChar(DP_HOURS1, '_');
    ledStrip.show();
    delay(750);
    showSetupFeedback = false;
  }
  // show setup feedback for minutes once when in setup mode
  else if (clockMode == CM_SETUP && setupMode == SM_MINUTES && showSetupFeedback) {
    displayChar(DP_MINUTES10, '_');
    displayChar(DP_MINUTES1, '_');
    ledStrip.show();
    delay(750);
    showSetupFeedback = false;
  }
  else if (clockMode == CM_TIME || setupMode == SM_HOURS || setupMode == SM_MINUTES) {
    displayTime();
    displayDots();
  }
  else if (setupMode == SM_WIFI && wifiManager.getConfigPortalActive()) {
    displayChar(DP_HOURS10, 'A');
    displayChar(DP_HOURS1, 'P');
    displayChar(DP_MINUTES10, '1');
    displayChar(DP_MINUTES1, ' ');
    displayDots();
  }
  else if (setupMode == SM_WIFI && !wifiManager.getConfigPortalActive()) {
    displayChar(DP_HOURS10, 'A');
    displayChar(DP_HOURS1, 'P');
    displayChar(DP_MINUTES10, '0');
    displayChar(DP_MINUTES1, ' ');
    displayDots();
  }
}

void displayChar(int displayNum, char ch) {
  byte segments = getCharSegments(ch);

  int ledOffset = displays[displayNum].ledOffset;
  for (int i = 0; i < 7; i++) {
    uint32_t color = showRainbowColors ? rainbowColors[ledOffset + i] : displayColor;
    bool on = ((segments >> i) & 0x01) && displays[displayNum].on;
    ledStrip.setPixelColor(ledOffset + i, on ? color : 0);
  }

  displays[displayNum].curChar = ch;
  ledStrip.show();
}

void displayDots() {
  int ledOffset = displays[DP_DOTS].ledOffset;
  bool on = displays[DP_DOTS].on;

  uint32_t color = showRainbowColors ? rainbowColors[ledOffset] : displayColor;
  ledStrip.setPixelColor(ledOffset, on ? color : 0);
  color = showRainbowColors ? rainbowColors[ledOffset + 1] : displayColor;
  ledStrip.setPixelColor(ledOffset + 1, on ? color : 0);

  ledStrip.show();
}


/**
 * @brief timer callback, toggles on/off for all displays with blink == true
 *
 */
bool blinkDisplays(void*) {
  for (int i = 0; i < DISPLAYCOUNT; i++) {
    if (displays[i].blink) {
      displays[i].on = !displays[i].on;
    }
  }
  return true;
}

void allDisplaysOn() {
  for (int i = 0; i < DISPLAYCOUNT; i++) {
    displays[i].on = true;
    displays[i].blink = false;
  }
}

void testSegments(int displayNum) {
  for (int hue = 0; hue < 65536; hue += 2048) {
    displayColor = ledStrip.ColorHSV(hue);

    for (int i = 0; i <= 9; i++) {
      ledStrip.clear();
      ledStrip.show();
      displayChar(displayNum, '0' + i);
      delay(750);
    }

    ledStrip.clear();
    ledStrip.show();
  }
}

byte getCharSegments(char c) {
  c = toupper(c);
  for (auto& entry : segMap) {
    if (entry.ch == c) return entry.segs;
  }
  return 0x00; // blank if not found
}

/*
 * WiFi
*/
void setupWiFiManager() {
  wifiManager.setConfigPortalTimeout(15 * 60);
  wifiManager.setCleanConnect(true);
  wifiManager.setHostname("setwifi.dpc");

  // Check if previous credentials were saved, if so try to connect
  if (wifiManager.getWiFiIsSaved()) {
    String ssid = wifiManager.getWiFiSSID();
    String pwd = wifiManager.getWiFiPass();

    WiFi.begin(ssid, pwd);
  }
}

void configModeCallback (WiFiManager *wiFiManager) {
  setDisplay();
}

void startWiFiManager() {
  DEBUG_PRINTLN("startWiFiManager");
  //wifiManager.autoConnect("DP_7S_CLOCK_01", "12605253");
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.startConfigPortal("DP_7S_CLOCK_01", "12605253");
}

/*
 * Time
 */
void setupTime() {
  configTime(TIMEZONE, NTP_SERVER);
}

void updateTime() {
  // Don't update in setup mode
  //if (clockMode == CM_SETUP) return;

  time(&now);                       // read the current time
  localtime_r(&now, &tms);           // update the structure tm with the current time

  // set day or night brightness
  int newBrightness = 0;
  if (tms.tm_hour > 22 && tms.tm_hour < 6) {
    newBrightness = brightnessNight;
  } else {
    newBrightness = brightnessDay;
  }
  if (brightness != newBrightness) {
    brightness = newBrightness;
    ledStrip.setBrightness(brightness);
    DEBUG_PRINTLN("brightness set");
  }
}

void displayTime() {
  int hour1 = tms.tm_hour % 10;
  int hour10 = (tms.tm_hour / 10) % 10;
  int minutes1 = tms.tm_min % 10;
  int minutes10 = (tms.tm_min / 10) % 10;

  displayChar(DP_HOURS10, '0' + hour10);
  displayChar(DP_HOURS1, '0' + hour1);
  displayChar(DP_MINUTES10, '0' + minutes10);
  displayChar(DP_MINUTES1, '0' + minutes1);
}

void incTime(int minutesInc, int hoursInc) {
  // Get current time and fill a tm structure with it
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);

  // Increase
  timeinfo->tm_hour += hoursInc;
  timeinfo->tm_min += minutesInc;

  if (timeinfo->tm_min >= 60) timeinfo->tm_min = 0;
  if (timeinfo->tm_hour >= 24) timeinfo->tm_hour = 0;

  // Normalize the time structure
  time_t newTime = mktime(timeinfo);

  // Set system time to the new value
  struct timeval tv = { .tv_sec = newTime };
  settimeofday(&tv, nullptr);

  printTime();
}

/*
 *  Button
 */


 /**
  * @brief handle a long press on the mode button
  * Enter or exit setup mode
  *
  */
static void handleModeBtnLongPress() {
  showSetupFeedback = true;

  // Enter setup mode
  if (clockMode == CM_TIME) {
    clockMode = CM_SETUP;
    setupMode = SM_HOURS;
    displays[DP_DOTS].blink = true;
    blinkTask = timer.every(1000, blinkDisplays);

    DEBUG_PRINTLN("Setup Mode");
  }
  else {
    // While in setup mode, every subsequent long press advances the setupMode
    setupMode = static_cast<SetupMode>((setupMode + 1) % NUM_SETUP_MODES);

    // When back at first setup mode, exit setup mode
    if (setupMode == 0) {
      timer.cancel(blinkTask);
      allDisplaysOn();
      clockMode = CM_TIME;

      DEBUG_PRINTLN("Time Mode");
    }
  }
}

/**
  * @brief handle a single click on the mode button
  * Change setting in current setupMode
  *
  */
static void handleModeBtnClick() {
  if (clockMode == CM_SETUP && setupMode == SM_HOURS) {
    incTime(0, 1);
  }
  else if (clockMode == CM_SETUP && setupMode == SM_MINUTES) {
    incTime(1, 0);
  }
  else if (clockMode == CM_SETUP && setupMode == SM_WIFI) {
    startWiFiManager();
  }
}


/**
 * @brief handle a double click on the mode button
 *
 */
static void handleModeBtnDoubleClick() {

}

/**
  * @brief handle a long press on the fx button
  * Enter or exit setup mode
  *
  */
static void handleFxBtnLongPress() {
  showRainbowColors = !showRainbowColors;
}

/**
  * @brief handle a single click on the fx button
  * Change colors
  *
  */
static void handleFxBtnClick() {
  if (showRainbowColors) {return;}

  // cycle through RGB
  if (displayColor == hexColorToInt("#FF0000")) {
    displayColor = hexColorToInt("#00FF00");
  } else if (displayColor == hexColorToInt("#00FF00")) {
    displayColor = hexColorToInt("#0000FF");
  } else if (displayColor == hexColorToInt("#0000FF")) {
    displayColor = hexColorToInt("#FF0000");
  }
}


/**
 * @brief handle a double click on the fx button
 *
 */
static void handleFxBtnDoubleClick() {
}

void setupButton() {
  modeBtn.setup(
    PIN_MODE_BTN,
    INPUT_PULLUP,
    true
  );

  fxBtn.setup(
    PIN_FX_BTN,
    INPUT_PULLUP,
    true
  );

  modeBtn.attachClick(handleModeBtnClick);
  modeBtn.attachDoubleClick(handleModeBtnDoubleClick);
  modeBtn.attachLongPressStart(handleModeBtnLongPress);

  fxBtn.attachClick(handleFxBtnClick);
  fxBtn.attachDoubleClick(handleFxBtnDoubleClick);
  fxBtn.attachLongPressStart(handleFxBtnLongPress);

  // longpress duration
  modeBtn.setPressMs(2000);
  fxBtn.setPressMs(2000);
}

/*
 * Util
 */
void processSerialCommand() {
  if (Serial.available()) {
    char input = Serial.read();
    input = toLowerCase(input);

    switch (input) {
      case 'c':
        DEBUG_PRINT("change color ");
        if (displayColor == hexColorToInt("#FF0000")) {
          displayColor = hexColorToInt("#00FF00");
          DEBUG_PRINT("green");
        }
        else if (displayColor == hexColorToInt("#00FF00")) {
          displayColor = hexColorToInt("#0000FF");
          DEBUG_PRINT("blue");
        }
        else if (displayColor == hexColorToInt("#0000FF")) {
          displayColor = hexColorToInt("#FF0000");
          DEBUG_PRINT("red");
        }
        DEBUG_PRINTLN();
        break;
      case 'b':
        brightness += 10;
        if (brightness > 255) {
          brightness = 0;
        }
        ledStrip.setBrightness(brightness);
        DEBUG_PRINT("change brightness ");
        DEBUG_PRINT(brightness);
        DEBUG_PRINTLN();
        break;
      case 'm':
        incTime(1, 0);
        DEBUG_PRINTLN("minutes + 1");
        break;
      case 'h':
        incTime(0, 1);
        DEBUG_PRINTLN("hours + 1");
        break;
      case 't':
        displayTime();
        break;
      case '0':
        DEBUG_PRINTLN("test segments");
        testSegments(DP_HOURS10);
        break;
      case '1':
        DEBUG_PRINTLN("display 1 show 8");
        displayChar(DP_HOURS10, '8');
        break;
      case '2':
        DEBUG_PRINTLN("display 2 show 8");
        displayChar(DP_HOURS1, '8');
        break;
      case '3':
        DEBUG_PRINTLN("display 3 show 8");
        displayChar(DP_DOTS, '8');
        break;
      case '4':
        DEBUG_PRINTLN("display 4 show 8");
        displayChar(DP_MINUTES10, '8');
        break;
      case '5':
        DEBUG_PRINTLN("display 5 show 8");
        displayChar(DP_MINUTES10, '8');
        break;
      case '9':
        DEBUG_PRINTLN("single led test");
        ledStrip.setPixelColor(0, 128);
        break;
      case 'a':
        DEBUG_PRINTLN("all led test");
        for (int i = 0; i < NUMLEDS; i++) {
          ledStrip.setPixelColor(i, 0);
          ledStrip.show();
        }        
        for (int i = 0; i < NUMLEDS; i++) {
          ledStrip.setPixelColor(i, 128);
          ledStrip.show();
          delay(750);
          ledStrip.setPixelColor(i, 0);
        }        
        break;
    }
    input = 0;
  }
}

uint32_t hexColorToInt(String hexColor) {
  if (hexColor.length() < 6) { return 0; }

  int startIndex = hexColor.length() == 7 ? 1 : 0;
  int rgb = (int)strtol(&hexColor[startIndex], NULL, 16);

  int r = (rgb >> 16) & 0xFF;
  int g = (rgb >> 8) & 0xFF;
  int b = rgb & 0xFF;

  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

void printTime() {

  DEBUG_PRINT("year:");
  DEBUG_PRINT(tms.tm_year + 1900);  // years since 1900
  DEBUG_PRINT("\tmonth:");
  DEBUG_PRINT(tms.tm_mon + 1);      // January = 0 (!)
  DEBUG_PRINT("\tday:");
  DEBUG_PRINT(tms.tm_mday);         // day of month
  DEBUG_PRINT("\thour:");
  DEBUG_PRINT(tms.tm_hour);         // hours since midnight  0-23
  DEBUG_PRINT("\tmin:");
  DEBUG_PRINT(tms.tm_min);          // minutes after the hour  0-59
  DEBUG_PRINT("\tsec:");
  DEBUG_PRINT(tms.tm_sec);          // seconds after the minute  0-61*
  DEBUG_PRINT("\twday");
  DEBUG_PRINT(tms.tm_wday);         // days since Sunday 0-6
  if (tms.tm_isdst == 1)             // Daylight Saving Time flag
    DEBUG_PRINT("\tDST");
  else
    DEBUG_PRINT("\tstandard");
  DEBUG_PRINTLN();
}


