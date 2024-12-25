
/*
    QuieTrack - A Noise Monitoring System

    Jerrald J. Guiriba
    https://github.com/Jed556
*/

#define ESP_DRD_USE_SPIFFS true

#include <WiFi.h>        // WiFi Library
#include <FS.h>          // File System Library
#include <SPIFFS.h>      // SPI Flash Syetem Library
#include <WiFiManager.h> // WiFiManager Library
#include <ArduinoJson.h> // Arduino JSON library
#include <U8g2lib.h>

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

// ***** Configuration ***** //

// Development mode
bool DEVELOPMENT_MODE = false;

// Button configuration
int BUTTON_PIN = 19;
int BUTTON_DEBOUNCE = 10;
int BUTTON_TIMEOUT = 300;

// Noise sensor pin
int NOISE_SENSOR_PIN = 12;

// JSON configuration file
#define JSON_CONFIG_FILE "/config.json"

// OLED Configuration
#define OLED_RESET -1       // Reset pin
#define SCREEN_ADDRESS 0x3C // 0x3C or 0x3D
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels

// Flag for saving data to json
bool shouldSaveConfig = true;

// Variables to hold data from custom textboxes
char testString[50] = "test value";
int testNumber = 1234;

// WiFiManager AP configuration
char AP_SSID[50] = "QuieTrack";
char AP_Pass[50] = "Jed55611";

// Menu items configuration
const int NUM_ITEMS = 3;        // number of items in the list
const int MAX_ITEM_LENGTH = 20; // maximum characters for the item name

// 'icon_config', 16x16px
const unsigned char bitmap_icon_config[] PROGMEM = {0xc0, 0x03, 0x48, 0x12, 0x34, 0x2c, 0x02, 0x40, 0xc4, 0x23, 0x24, 0x24, 0x13, 0xc8, 0x11, 0x88, 0x11, 0x88, 0x13, 0xc8, 0x24, 0x24, 0xc4, 0x23, 0x02, 0x40, 0x34, 0x2c, 0x48, 0x12, 0xc0, 0x03};

// 'icon_measure', 16x16px
const unsigned char bitmap_icon_measure[] PROGMEM = {
    0xE0,
    0x07,
    0x18,
    0x18,
    0x84,
    0x24,
    0x0A,
    0x40,
    0x12,
    0x50,
    0x21,
    0x80,
    0xC1,
    0x81,
    0x45,
    0xA2,
    0x41,
    0x82,
    0x81,
    0x81,
    0x05,
    0xA0,
    0x02,
    0x40,
    0xD2,
    0x4B,
    0xC4,
    0x23,
    0x18,
    0x18,
    0xE0,
    0x07,
};

// 'icon_reboot', 16x16px
const unsigned char bitmap_icon_reboot[] PROGMEM = {0x80, 0x00, 0x80, 0x00, 0x98, 0x0c, 0xa4, 0x12, 0x92, 0x24, 0x8a, 0x28, 0x85, 0x50, 0x05, 0x50, 0x05, 0x50, 0x05, 0x50, 0x05, 0x50, 0x0a, 0x28, 0x12, 0x24, 0xe4, 0x13, 0x18, 0x0c, 0xe0, 0x03};

// Menu Item Icons
const unsigned char *bitmap_icons[8] = {
    bitmap_icon_config,
    bitmap_icon_measure,
    bitmap_icon_reboot,
};

// Menu Item Names
char menu_items[NUM_ITEMS][MAX_ITEM_LENGTH] = {
    {"Config"},
    {"Measure"},
    {"Reboot"},
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
        u8g2.drawStr(0, 30, AP_SSID);
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
    float noiseLevel = analogRead(NOISE_SENSOR_PIN);
    char noiseStr[10];
    sprintf(noiseStr, "%f", (float)noiseLevel);
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 10, "Measure");
    u8g2.drawStr(0, 30, "Noise Level: ");
    u8g2.drawStr(90, 30, noiseStr);
    drawLoudnessBar(noiseLevel);
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

// Pages
void (*displayPages[8])() = {
    page_config,
    page_measure,
    page_reboot,
};

// ***** Internal Variables ***** //

// Define WiFiManager Object
WiFiManager wm;

// Define Variables for Double Click Detection
byte buttonLast;
enum
{
    None,
    SingleClick,
    DoubleClick
};

// Define Menu Options and Variables
int item_selected = 0; // which item in the menu is selected

int item_sel_previous; // previous item - used in the menu screen to draw the item before the selected one
int item_sel_next;     // next item - used in the menu screen to draw next item after the selected one

int current_page = 0; // 0 = menu, 1 = sub-page

// Bitmaps
const unsigned char bitmap_logo[] PROGMEM = {
    0x04, 0x74, 0x67, 0x24, 0x16, 0x11, 0x54, 0x35, 0x33, 0x35, 0x45, 0x54, 0x62, 0x36, 0x23};

// 'item_sel_outline', 128x21px
const unsigned char bitmap_item_sel_outline[] PROGMEM = {
    0xF8,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0x03,
    0x04,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x04,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x0C,
    0xFC,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0x07,
    0xF8,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0x03,
};

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

void handleClick()
// Function to handle button clicks
{
    int buttonState = checkButton();
    if (buttonState == SingleClick)
    {
        if (current_page == 0)
        {
            item_selected = item_selected + 1; // select next item
            if (item_selected >= NUM_ITEMS)
            { // last item was selected, jump to first menu item
                item_selected = 0;
            }
        }
    }
    else if (buttonState == DoubleClick)
    {
        if (current_page == 0)
        {
            current_page = 1;
        } // menu items screen --> sub-page screen
        else if (current_page == 1)
        {
            current_page = 0;
        } // sub-page screen --> menu items screen
    }
}

void page_menu()
{
    if (current_page == 0) // MENU SCREEN
    {
        // selected item background
        u8g2.drawXBMP(0, 22, 128, 21, bitmap_item_sel_outline);

        // draw previous item as icon + label
        u8g2.setFont(u8g_font_7x14);
        u8g2.drawStr(25, 15, menu_items[item_sel_previous]);
        u8g2.drawXBMP(4, 2, 16, 16, bitmap_icons[item_sel_previous]);

        // draw selected item as icon + label in bold font
        u8g2.setFont(u8g_font_7x14B);
        u8g2.drawStr(25, 15 + 20 + 2, menu_items[item_selected]);
        u8g2.drawXBMP(4, 24, 16, 16, bitmap_icons[item_selected]);

        // draw next item as icon + label
        u8g2.setFont(u8g_font_7x14);
        u8g2.drawStr(25, 15 + 20 + 20 + 2 + 2, menu_items[item_sel_next]);
        u8g2.drawXBMP(4, 46, 16, 16, bitmap_icons[item_sel_next]);

        // draw scrollbar background
        for (int i = 0; i < SCREEN_HEIGHT; i += 2)
        {
            u8g2.drawPixel(126, i);
        }

        // draw scrollbar handle
        int scrollbarHeight = SCREEN_HEIGHT / NUM_ITEMS;
        u8g2.drawBox(125, scrollbarHeight * item_selected, 3, scrollbarHeight);

        // draw logo
        u8g2.drawXBMP(128 - 16 - 4, 64 - 4, 16, 4, bitmap_logo);
    }
    else if (current_page == 1) // DISPLAY SUB-PAGE
    {
        displayPages[item_selected](); // run the function corresponding to the selected item
    }
}

void saveConfigFile()
// Save Config in JSON format
{
    Serial.println(F("Saving configuration..."));

    // Create a JSON document
    StaticJsonDocument<512> json;
    json["testString"] = testString;
    json["testNumber"] = testNumber;

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

bool loadConfigFile()
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

                    strcpy(testString, json["testString"]);
                    testNumber = json["testNumber"].as<int>();

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
    Serial.println("Should save config");
    shouldSaveConfig = true;
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
    bool spiffsSetup = loadConfigFile();
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

    // Set config save notify callback
    wm.setSaveConfigCallback(saveConfigCallback);

    // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wm.setAPCallback(configModeCallback);

    // Custom elements

    // Text box (String) - 50 characters maximum
    WiFiManagerParameter custom_text_box("key_text", "Enter your string here", testString, 50);

    // Need to convert numerical input to string to display the default value.
    char convertedValue[6];
    sprintf(convertedValue, "%d", testNumber);

    // Text box (Number) - 7 characters maximum
    WiFiManagerParameter custom_text_box_num("key_num", "Enter your number here", convertedValue, 7);

    // Add all defined parameters
    wm.addParameter(&custom_text_box);
    wm.addParameter(&custom_text_box_num);

    if (forceConfig) // Run configuration is needed
    {
        if (!wm.startConfigPortal(AP_SSID, AP_Pass))
        {
            connectCallback(1);
        }
    }
    else
    {
        if (!wm.autoConnect(AP_SSID, AP_Pass))
        {
            connectCallback(1);
        }
    }

    connectCallback();

    // User config values

    // Copy the string value
    strncpy(testString, custom_text_box.getValue(), sizeof(testString));
    Serial.print("testString: ");
    Serial.println(testString);

    // Convert the number value
    testNumber = atoi(custom_text_box_num.getValue());
    Serial.print("testNumber: ");
    Serial.println(testNumber);

    // Save the custom parameters to FS
    if (shouldSaveConfig)
    {
        saveConfigFile();
    }
}

void loop()
{
    handleClick(); // check how the button is pressed and handle the action

    // set correct values for the previous and next items
    item_sel_previous = item_selected - 1;
    if (item_sel_previous < 0)
    {
        item_sel_previous = NUM_ITEMS - 1;
    } // previous item would be below first = make it the last
    item_sel_next = item_selected + 1;
    if (item_sel_next >= NUM_ITEMS)
    {
        item_sel_next = 0;
    } // next item would be after last = make it the first

    u8g2.clearBuffer(); // clear buffer for storing display content in RAM

    page_menu(); // draw the menu screen

    u8g2.sendBuffer(); // send buffer from RAM to display controller
}