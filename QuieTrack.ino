/*
    QuieTrack - A Noise Monitoring System

    Jerrald J. Guiriba
    https://github.com/Jed556
*/

#define ESP_DRD_USE_SPIFFS true

#include <Arduino.h>        // Arduino Core Library
#include <FS.h>             // File System Library
#include <SPIFFS.h>         // SPI Flash Syetem Library
#include <WiFiManager.h>    // WiFiManager Library
#include <FirebaseClient.h> // Firebase Library
#include <SD.h>             // Secure Digital Card Library
#include <ArduinoJson.h>    // Arduino JSON library
#include <U8g2lib.h>        // Graphics Library
#include <math.h>           // Math Library

// Wifi Library
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_GIGA) || defined(ARDUINO_OPTA)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>) || defined(ARDUINO_NANO_RP2040_CONNECT)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>) || defined(ARDUINO_UNOWIFIR4)
#include <WiFiS3.h>
#elif __has_include(<WiFiC3.h>) || defined(ARDUINO_PORTENTA_C33)
#include <WiFiC3.h>
#elif __has_include(<WiFi.h>)
#include <WiFi.h>
#endif

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// U8g2 Contructor List (Frame Buffer) | Use U8X8_PIN_NONE if the reset pin is not connected
// Complete list: https://github.com/olikraus/u8g2/wiki/u8g2setupcpp

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2);  //This inverts Bitmap and text

// Define Variables for Noise Reading
float noiseLevel = 0;

// ***** Configuration ***** //

// Development mode
#define DEVELOPMENT_MODE false

// Button configuration
#define BUTTON_PIN 19
#define BUTTON_DEBOUNCE 10
#define BUTTON_TIMEOUT 300

// Noise sensor pin
#define NOISE_SENSOR_PIN 35

// ADC configuration
#define RMS_SAMPLES 100
#define REF_VOLTAGE 5
#define ADC_RESOLUTION 4095

// JSON configuration file
#define JSON_CONFIG_FILE "/config.json"

// OLED Configuration
#define OLED_RESET -1       // Reset pin
#define SCREEN_ADDRESS 0x3C // 0x3C or 0x3D
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels

// Menu items configuration
#define NUM_MENU_ITEMS 4          // number of items in the list
#define MAX_MENU_ITEM_NAME_LEN 20 // maximum characters for the item name

// Flag for saving data to json
bool shouldSaveConfig = true;

// Database timeout in milliseconds
int dbTimeout = 1000;

// Variables to hold data from custom textboxes

enum InputType
// Custom input types
{
  INT,
  FLOAT,
  DOUBLE,
  BOOL,
  STRING,
  ARRAY,
  JSON
};

struct wmInput
// Struct to hold custom WiFiManager inputs
{
  char *key;
  char *textValue;
  int intValue;
  char *text;
  InputType type;
  int max;
  WiFiManagerParameter *param;
};

wmInput wmInputs[] = {
    {"apSsid", "QuieTrack", 0.0, "Access Point SSID", STRING, 50, nullptr},
    {"apPass", "Jed55611", 0.0, "Access Point Password", STRING, 50, nullptr},
    {"firebaseAPI", "API Key", 0.0, "Firebase API Key", STRING, 50, nullptr},
    {"firebaseEmail", "Email", 0.0, "Firebase Account Email", STRING, 50, nullptr},
    {"firebasePassword", "Password", 0.0, "Firebase Account Password", STRING, 50, nullptr},
    {"firebaseDatabaseURL", "Database URL", 0.0, "Firebase Database URL", STRING, 100, nullptr},
    {"firebaseUpdateInterval", "", 1000, "Database Update Interval (ms)", INT, 3, nullptr},
    {"noiseRefDbDiff", "", 78, "Noise Decibel Reference", INT, 3, nullptr},
    {"noiseRefRead", "", 239, "Noise Sensor Reading Reference", INT, 4, nullptr},
};

// ***** Internal Variables ***** //

// Get wmInputs size
int wmInputsSize = sizeof(wmInputs) / sizeof(wmInput);

// Define WiFiManager Object
WiFiManager wm;

// Noise Variabls
float noise, logTen;

// Define Variables for Double Click Detection
byte buttonLast;
enum
{
  None,
  SingleClick,
  DoubleClick
};

// Define Menu Options and Variables
int itemSelected = 0; // which item in the menu is selected

int itemSelPrevious; // previous item - used in the menu screen to draw the item before the selected one
int itemSelNext;     // next item - used in the menu screen to draw next item after the selected one

int currentPage = 0; // 0 = menu, 1 = sub-page

// Bitmaps
const unsigned char bitmap_logo[] PROGMEM = {0x04, 0x74, 0x67, 0x24, 0x16, 0x11, 0x54, 0x35, 0x33, 0x35, 0x45, 0x54, 0x62, 0x36, 0x23};

// 'item_sel_outline', 128x21px
const unsigned char bitmap_item_sel_outline[] PROGMEM = {0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x03};

FirebaseApp FbApp;

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFiClientSecure.h>
WiFiClientSecure ssl_client;
#elif defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_UNOWIFIR4) || defined(ARDUINO_GIGA) || defined(ARDUINO_OPTA) || defined(ARDUINO_PORTENTA_C33) || defined(ARDUINO_NANO_RP2040_CONNECT)
#include <WiFiSSLClient.h>
WiFiSSLClient ssl_client;
#endif

using AsyncClient = AsyncClientClass;

DefaultNetwork network;

AsyncClient aClient(ssl_client, getNetwork(network));

RealtimeDatabase Database;
unsigned long tmo = 0;

// void asyncCB(AsyncResult &aResult);
void printResult(AsyncResult &aResult);

// ***** Function Prototypes ***** //

int checkButton(void)
// Check button presses
{
  static unsigned long msecLast;
  unsigned long msec = millis();

  if (msecLast && (msec - msecLast) > BUTTON_TIMEOUT)
  {
    msecLast = 0;
    return SingleClick;
  }

  byte button = digitalRead(BUTTON_PIN);
  if (buttonLast != button)
  {
    buttonLast = button;
    delay(BUTTON_DEBOUNCE); // **** debounce

    if (LOW == button)
    { // press
      if (msecLast)
      { // 2nd press
        msecLast = 0;
        return DoubleClick;
      }
      else
        msecLast = 0 == msec ? 1 : msec;
    }
  }

  return None;
}

// 'icon_config', 16x16px
const unsigned char bitmap_icon_config[] PROGMEM = {0xc0, 0x03, 0x48, 0x12, 0x34, 0x2c, 0x02, 0x40, 0xc4, 0x23, 0x24, 0x24, 0x13, 0xc8, 0x11, 0x88, 0x11, 0x88, 0x13, 0xc8, 0x24, 0x24, 0xc4, 0x23, 0x02, 0x40, 0x34, 0x2c, 0x48, 0x12, 0xc0, 0x03};

// 'icon_measure', 16x16px
const unsigned char bitmap_icon_measure[] PROGMEM = {0xE0, 0x07, 0x18, 0x18, 0x84, 0x24, 0x0A, 0x40, 0x12, 0x50, 0x21, 0x80, 0xC1, 0x81, 0x45, 0xA2, 0x41, 0x82, 0x81, 0x81, 0x05, 0xA0, 0x02, 0x40, 0xD2, 0x4B, 0xC4, 0x23, 0x18, 0x18, 0xE0, 0x07};

// 'icon_reboot', 16x16px
const unsigned char bitmap_icon_reboot[] PROGMEM = {0x80, 0x00, 0x80, 0x00, 0x98, 0x0c, 0xa4, 0x12, 0x92, 0x24, 0x8a, 0x28, 0x85, 0x50, 0x05, 0x50, 0x05, 0x50, 0x05, 0x50, 0x05, 0x50, 0x05, 0x50, 0x05, 0x50, 0x0a, 0x28, 0x12, 0x24, 0xe4, 0x13, 0x18, 0x0c, 0xe0, 0x03};

// 'icon_fireworks', 16x16px
const unsigned char bitmap_icon_fireworks[] PROGMEM = {0x00, 0x00, 0x00, 0x10, 0x00, 0x29, 0x08, 0x10, 0x08, 0x00, 0x36, 0x00, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x63, 0x00, 0x00, 0x00, 0x08, 0x20, 0x08, 0x50, 0x00, 0x20, 0x00, 0x00, 0x00};

// Menu Item Icons
const unsigned char *bitmap_icons[NUM_MENU_ITEMS] = {
    bitmap_icon_config,
    bitmap_icon_measure,
    bitmap_icon_reboot,
    bitmap_icon_fireworks,
};

// Menu Item Names
char menu_items[NUM_MENU_ITEMS][MAX_MENU_ITEM_NAME_LEN] = {
    {"Config"},
    {"Measure"},
    {"Reboot"},
    {"Format"},
};

void page_intro()
{
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(38, 30, "QuieTrack");
  u8g2.drawFrame(34, 17, 61, 20);
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(45, 47, "by Group 8");
}

void page_config()
{
  int checkButton();
  if (checkButton() == DoubleClick)
  {
    currentPage = 0;
  }

  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setFont(u8g2_font_ncenB08_tr);
  if (WiFi.status() == WL_CONNECTED)
  {
    u8g2.drawStr(0, 10, "Connected");
    u8g2.drawStr(0, 30, WiFi.SSID().c_str());
    u8g2.drawStr(0, 50, WiFi.localIP().toString().c_str());
  }
  else
  {
    u8g2.drawStr(0, 10, "Not Connected");
    u8g2.drawStr(0, 30, (const char *)getWmInputValue(wmInputs, wmInputsSize, "apSsid", STRING));
    u8g2.drawStr(0, 50, WiFi.softAPIP().toString().c_str());
  }
}

void drawLoudnessBar(float noiseLevel)
// Draw the loudness bar
{
  int barWidth = map(noiseLevel, 0, 1023, 0, SCREEN_WIDTH); // Map noise level to screen width
  u8g2.drawFrame(0, 40, SCREEN_WIDTH, 10);                  // Draw the outline of the bar
  u8g2.drawBox(0, 40, barWidth, 10);                        // Draw the filled bar based on noise level
}

void page_measure()
{
  int checkButton();
  if (checkButton() == DoubleClick)
  {
    currentPage = 0;
  }

  char noiseStr[10];

  int scaledNoise = (int)(noiseLevel * 100); // Scale the float and convert to integer
  int wholePart = scaledNoise / 100;         // Get the whole number part
  int decimalPart = scaledNoise % 100;       // Get the decimal part

  // Create the formatted string manually
  itoa(wholePart, noiseStr, 10);
  strcat(noiseStr, ".");
  char decimalStr[5];
  itoa(decimalPart, decimalStr, 10);
  strcat(noiseStr, decimalStr);
  strcat(noiseStr, " db");

  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "Measure");
  u8g2.drawStr(0, 30, "Noise Level: ");
  u8g2.drawStr(70, 30, noiseStr);
  drawLoudnessBar(noiseLevel);

  Serial.println();
  Serial.print(noise);
  Serial.print("\t");
  Serial.println(noiseLevel);
}

void page_reboot()
{
  char str[] = "Rebooting";
  for (int i = 0; i < 8; i++)
  {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr((SCREEN_WIDTH - u8g2.getStrWidth(str)) / 2, 55, str);
    u8g2.drawCircle(64, 32, 10, U8G2_DRAW_ALL);
    u8g2.drawLine(64, 32, 64 + 10 * cos(i * 3.14 / 4), 32 + 10 * sin(i * 3.14 / 4));
    u8g2.sendBuffer();
    delay(250); // Adjust delay for desired spinning speed
    yield();
  }
  ESP.restart(); // Restart the ESP microcontroller
}

void page_format()
{
  char str[] = "Formatting";
  for (int i = 0; i < 8; i++)
  {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr((SCREEN_WIDTH - u8g2.getStrWidth(str)) / 2, 55, str);
    u8g2.drawCircle(64, 32, 10, U8G2_DRAW_ALL);
    u8g2.drawLine(64, 32, 64 + 10 * cos(i * 3.14 / 4), 32 + 10 * sin(i * 3.14 / 4));
    u8g2.sendBuffer();
    delay(250); // Adjust delay for desired spinning speed
  }
  wm.resetSettings(); // Reset WiFiManager settings
  SPIFFS.format();    // Format the SPIFFS filesystem

  ESP.restart(); // Restart the ESP microcontroller
  yield();
}

// Pages
void (*displayPages[8])() = {
    page_config,
    page_measure,
    page_reboot,
    page_format,
};

void pageMenu()
{
  if (currentPage == 0) // MENU SCREEN
  {
    // Handle button clicks
    int buttonState = checkButton();

    if (buttonState == SingleClick)
    {
      if (currentPage == 0)
      {
        itemSelected = itemSelected + 1; // select next item
        if (itemSelected >= NUM_MENU_ITEMS)
        { // last item was selected, jump to first menu item
          itemSelected = 0;
        }
      }
    }
    else if (buttonState == DoubleClick && currentPage == 0)
    {
      currentPage = 1;
      // menu items screen --> sub-page screen
    }

    // Set correct values for the previous and next items
    itemSelPrevious = itemSelected - 1;
    if (itemSelPrevious < 0)
    {
      itemSelPrevious = NUM_MENU_ITEMS - 1;
    } // previous item would be below first = make it the last
    itemSelNext = itemSelected + 1;
    if (itemSelNext >= NUM_MENU_ITEMS)
    {
      itemSelNext = 0;
    } // next item would be after last = make it the first

    // selected item background
    u8g2.drawXBMP(0, 22, 128, 21, bitmap_item_sel_outline);

    // draw previous item as icon + label
    u8g2.setFont(u8g_font_7x14);
    u8g2.drawStr(25, 15, menu_items[itemSelPrevious]);
    u8g2.drawXBMP(4, 2, 16, 16, bitmap_icons[itemSelPrevious]);

    // draw selected item as icon + label in bold font
    u8g2.setFont(u8g_font_7x14B);
    u8g2.drawStr(25, 15 + 20 + 2, menu_items[itemSelected]);
    u8g2.drawXBMP(4, 24, 16, 16, bitmap_icons[itemSelected]);

    // draw next item as icon + label
    u8g2.setFont(u8g_font_7x14);
    u8g2.drawStr(25, 15 + 20 + 20 + 2 + 2, menu_items[itemSelNext]);
    u8g2.drawXBMP(4, 46, 16, 16, bitmap_icons[itemSelNext]);

    // draw scrollbar background
    for (int i = 0; i < SCREEN_HEIGHT; i += 2)
    {
      u8g2.drawPixel(126, i);
    }

    // draw scrollbar handle
    int scrollbarHeight = SCREEN_HEIGHT / NUM_MENU_ITEMS;
    u8g2.drawBox(125, scrollbarHeight * itemSelected, 3, scrollbarHeight);

    // draw logo
    u8g2.drawXBMP(128 - 16 - 4, 64 - 4, 16, 4, bitmap_logo);
  }
  else if (currentPage == 1) // DISPLAY SUB-PAGE
  {
    displayPages[itemSelected](); // run the function corresponding to the selected item
  }
}

float computeDecibels(int read, int refRead, int refDbDiff)
// Handles decibel calculation
{
  if (read == 0 || refRead == 0)
    logTen = 0;
  else
    logTen = log10(noise / refRead);

  noiseLevel = 20 * logTen + refDbDiff;

  return noiseLevel;
}

void buildWmInputs(wmInput *wmInputs, int size)
// Build WiFiManager custom inputs
{
  // Loop through wmInputs and create parameters
  for (int i = 0; i < size; i++)
  {
    if (wmInputs[i].type == STRING)
    {
      wmInputs[i].param = new WiFiManagerParameter(wmInputs[i].key, wmInputs[i].text, wmInputs[i].textValue, wmInputs[i].max);
    }
    else if (wmInputs[i].type == INT || wmInputs[i].type == FLOAT)
    {
      char convertedValue[10];
      sprintf(convertedValue, "%d", wmInputs[i].intValue);

      wmInputs[i].param = new WiFiManagerParameter(wmInputs[i].key, wmInputs[i].text, convertedValue, wmInputs[i].max);
    }
    wm.addParameter(wmInputs[i].param);
  }
}

void updateWmInputs(wmInput *wmInputs, int size)
// Update WiFiManager custom inputs
{
  for (int i = 0; i < size; i++)
  {
    WiFiManagerParameter *param = wmInputs[i].param;
    if (wmInputs[i].type == STRING)
    {
      wmInputs[i].textValue = strdup(param->getValue());
    }
    else if (wmInputs[i].type == INT || wmInputs[i].type == FLOAT)
    {
      wmInputs[i].intValue = atof(param->getValue());
    }
  }
}

void *getWmInputValue(wmInput *wmInputs, int size, const char *key, InputType type)
// Gets a value from wmInputs based on the specified type
{
  for (int i = 0; i < size; i++)
  {
    if (strcmp(wmInputs[i].key, key) == 0)
    {
      switch (type)
      {
      case STRING:
        return wmInputs[i].textValue;
      case INT:
      case FLOAT:
        return &wmInputs[i].intValue;
      default:
        return nullptr;
      }
    }
  }
  return nullptr;
}

void saveConfigFile(wmInput *wmInputs, int size)
// Save Config in JSON format
{
  Serial.println(F("Saving configuration..."));

  // Create a JSON document
  StaticJsonDocument<512> json;

  // Add values to JSON document
  for (int i = 0; i < size; i++)
  {
    if (wmInputs[i].type == STRING)
    {
      json[wmInputs[i].key] = wmInputs[i].textValue;
    }
    else if (wmInputs[i].type == INT || wmInputs[i].type == FLOAT)
    {
      json[wmInputs[i].key] = wmInputs[i].intValue;
    }
  }

  // Open config file
  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile)
  {
    // Error, file did not open
    Serial.println("failed to open config file for writing");
  }

  // Serialize JSON data to write to file
  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0)
  {
    // Error writing file
    Serial.println(F("Failed to write to file"));
  }
  // Close file
  configFile.close();
}

bool loadConfigFile(wmInput *wmInputs, int size)
// Load existing configuration file
{
  // Uncomment if we need to format filesystem
  // SPIFFS.format();

  // Read configuration from FS json
  Serial.println("Mounting File System...");

  // May need to make it begin(true) first time you are using SPIFFS
  if (SPIFFS.begin(false) || SPIFFS.begin(true))
  {
    Serial.println("mounted file system");
    if (SPIFFS.exists(JSON_CONFIG_FILE))
    {
      // The file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
      if (configFile)
      {
        Serial.println("Opened configuration file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error)
        {
          Serial.println("Parsing JSON");

          // Copy values from JSON to variables
          for (int i = 0; i < size; i++)
          {
            if (wmInputs[i].type == STRING)
            {
              wmInputs[i].textValue = strdup(json[wmInputs[i].key]);
            }
            else if (wmInputs[i].type == INT || wmInputs[i].type == FLOAT)
            {
              wmInputs[i].intValue = json[wmInputs[i].key].as<int>();
            }
          }

          return true;
        }
        else
        {
          // Error loading JSON data
          Serial.println("Failed to load json config");
        }
      }
    }
  }
  else
  {
    // Error mounting file system
    Serial.println("Failed to mount FS");
  }

  return false;
}

void saveConfigCallback()
// Callback notifying us of the need to save configuration
{
  Serial.println("[CALLBACK] saveConfigCallback fired");
  shouldSaveConfig = true;
}

void saveParamsCallback()
{
  if (shouldSaveConfig)
  {
    updateWmInputs(wmInputs, wmInputsSize);
    saveConfigFile(wmInputs, wmInputsSize);
  }
}

void connectCallback(int failed = 0)
// Called when trying to connect to WiFi
{
  u8g2.clearBuffer();
  page_config();
  u8g2.sendBuffer();

  if (failed == 1)
  {
    Serial.println("Failed to connect to WiFi and hit timeout");
    delay(3000);
    ESP.restart(); // Reset on failure
    delay(5000);
  }
  Serial.println("");

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi connected");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("WiFi not connected");
    Serial.println("Entered Configuration Mode");
    Serial.println("SSID: ");
    Serial.println(wm.getConfigPortalSSID());
    Serial.println("Config IP Address: ");
    Serial.println(WiFi.softAPIP());
  }
  Serial.println("");
}

void configModeCallback(WiFiManager *myWiFiManager)
// Called when config mode launched
{
  Serial.println("");
  Serial.println("WiFi not connected");
  Serial.println("Entered Configuration Mode");
  Serial.println("SSID: ");
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.println("Config IP Address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("");
}

void newFirebasePushTask(const String &path, void *value, InputType type)
{
  if (FbApp.ready())
  {
    switch (type)
    {
    case INT:
      Serial.println("Asynchronous Push Int... ");
      Database.push<int>(aClient, path, *(int *)value, asyncCB, "pushIntTask");
      break;
    case BOOL:
      Serial.println("Asynchronous Push Bool... ");
      Database.push<bool>(aClient, path, *(bool *)value, asyncCB, "pushBoolTask");
      break;
    case STRING:
      Serial.println("Asynchronous Push String... ");
      Database.push<String>(aClient, path, *(String *)value, asyncCB, "pushStringTask");
      break;
    case JSON:
      Serial.println("Asynchronous Push JSON... ");
      Database.push<object_t>(aClient, path, *(object_t *)value, asyncCB, "pushJsonTask");
      break;
    case FLOAT:
      Serial.println("Asynchronous Push Float... ");
      Database.push<number_t>(aClient, path, number_t(*(float *)value, 2), asyncCB, "pushFloatTask");
      break;
    case DOUBLE:
      Serial.println("Asynchronous Push Double... ");
      Database.push<number_t>(aClient, path, number_t(*(double *)value, 4), asyncCB, "pushDoubleTask");
      break;
    default:
      Serial.println("Unsupported data type for Firebase push.");
      break;
    }
  }
}

void asyncCB(AsyncResult &aResult)
// Callback  for Firebase async tasks
{
  // WARNING!
  // Do not put your codes inside the callback and printResult.

  printFirebaseResult(aResult);
}

void printFirebaseResult(AsyncResult &aResult)
// Print the result of the Firebase async task
{
  if (aResult.isEvent())
  {
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
  }

  if (aResult.isDebug())
  {
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
  }

  if (aResult.isError())
  {
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
  }

  if (aResult.available())
  {
    if (aResult.to<RealtimeDatabaseResult>().name().length())
      Firebase.printf("task: %s, name: %s\n", aResult.uid().c_str(), aResult.to<RealtimeDatabaseResult>().name().c_str());
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
  }
}

// ***** Setup and Loop ***** //

void setup()
{
  // Change to true when testing to force run configuration
  bool forceConfig = DEVELOPMENT_MODE;

  // Setup button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Setup noise sensor
  pinMode(NOISE_SENSOR_PIN, INPUT);

  // Check for saved configuration
  bool spiffsSetup = loadConfigFile(wmInputs, wmInputsSize);
  if (!spiffsSetup)
  {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }

  WiFi.mode(WIFI_STA); // Explicitly set WiFi mode

  // Setup OLED
  u8g2.setColorIndex(1); // set the color to white
  u8g2.begin();
  u8g2.setBitmapMode(1);
  u8g2.clearBuffer();
  page_intro();
  u8g2.sendBuffer();

  // Setup Serial monitor
  Serial.begin(115200);
  delay(10);

  // Reset settings (only for development)
  if (DEVELOPMENT_MODE)
  {
    wm.resetSettings();
  }

  // Dark mode
  wm.setDarkMode(true);

  // Set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);

  // Set custom parameters save callback
  wm.setSaveParamsCallback(saveParamsCallback);

  // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);

  // Custom elements
  buildWmInputs(wmInputs, wmInputsSize);

  if (forceConfig) // Run configuration is needed
  {
    if (!wm.startConfigPortal((const char *)getWmInputValue(wmInputs, wmInputsSize, "apSsid", STRING), (const char *)getWmInputValue(wmInputs, wmInputsSize, "apPass", STRING)))
    {
      connectCallback(1);
    }
  }
  else
  {
    if (!wm.autoConnect((const char *)getWmInputValue(wmInputs, wmInputsSize, "apSsid", STRING), (const char *)getWmInputValue(wmInputs, wmInputsSize, "apPass", STRING)))
    {
      connectCallback(1);
    }
  }

  connectCallback();

  // Update and display user config values
  updateWmInputs(wmInputs, wmInputsSize);
  for (int i = 0; i < wmInputsSize; i++)
  {
    if (wmInputs[i].type == STRING)
    {
      Serial.print(wmInputs[i].key);
      Serial.print(": ");
      Serial.println(wmInputs[i].textValue);
    }
    else if (wmInputs[i].type == INT || wmInputs[i].type == FLOAT)
    {
      Serial.print(wmInputs[i].key);
      Serial.print(": ");
      Serial.println(wmInputs[i].intValue);
    }
  }

  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
  Serial.println("Initializing Firebase App...");

#if defined(ESP32) || defined(ESP8266) || defined(PICO_RP2040)
  ssl_client.setInsecure();
#if defined(ESP8266)
  ssl_client.setBufferSizes(4096, 1024);
#endif
#endif

  // Create an auth object using credentials from wmInputs
  UserAuth user_auth(
      (char *)getWmInputValue(wmInputs, wmInputsSize, "firebaseAPI", STRING),
      (char *)getWmInputValue(wmInputs, wmInputsSize, "firebaseEmail", STRING),
      (char *)getWmInputValue(wmInputs, wmInputsSize, "firebasePassword", STRING),
      3000 /* expire period in seconds (<3600) */
  );

  // Initialize the FirebaseApp or auth task handler.To deinitialize, use deinitializeApp(app).initializeApp(aClient, FbApp, getAuth(user_auth), asyncCB, "authTask");

  // Binding the FirebaseApp for authentication handler.
  // To unbind, use Database.resetApp();
  FbApp.getApp<RealtimeDatabase>(Database);

  Database.url((const char *)getWmInputValue(wmInputs, wmInputsSize, "firebaseHost", STRING));
}

void loop()
{
  // The async task handler should run inside the main loop
  // without blocking delay or bypassing with millis code blocks.
  FbApp.loop(); // Firebase async task handler
  Database.loop();

  computeDecibels(analogRead(NOISE_SENSOR_PIN), *(int *)getWmInputValue(wmInputs, wmInputsSize, "noiseRefRead", INT), *(int *)getWmInputValue(wmInputs, wmInputsSize, "noiseRefDbDiff", INT));

  if (FbApp.ready() && millis() - tmo > dbTimeout)
  {
    newFirebasePushTask("/noise", &noiseLevel, FLOAT);
    tmo = millis();
  }

  u8g2.clearBuffer(); // clear buffer for storing display content in RAM

  pageMenu(); // draw the menu screen

  u8g2.sendBuffer(); // send buffer from RAM to display controller
}