#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SI5351.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <FS.h>
#include <LittleFS.h>

// Pins for the ST7789
#define TFT_CS D8  // GPIO15
#define TFT_DC D1  // GPIO5
#define TFT_RST D2 // GPIO4

// ----------- USER CONFIG -----------
const char *ssid = "(╯▔皿▔)╯_EXT";
const char *password = "kr1stis!";
// -----------------------------------

ESP8266WebServer server(80);
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_SI5351 si5351;

bool clockHalted = false;
uint32_t lastFrequency = 1000;
String ipAddress = "0.0.0.0";

// Function prototypes
void setClockSpeed(uint32_t freq);
void haltClock();
void resumeClock();
void updateDisplay();

void initializeDisplay()
{
  tft.init(240, 320, SPI_MODE2);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
}

void updateDisplay()
{
  tft.fillScreen(ST77XX_BLACK);

  // Clock Speed Display
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(4);
  tft.setCursor(20, 50);
  tft.print(lastFrequency);
  tft.print(" Hz");

  // Halted State Display
  tft.setTextSize(2);
  tft.setCursor(10, 150);
  tft.setTextColor(clockHalted ? ST77XX_RED : ST77XX_GREEN);
  tft.print("Halted: ");
  tft.print(clockHalted ? "True" : "False");

  // IP Address Display
  tft.setCursor(10, 200);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("IP: ");
  tft.print(ipAddress);
}

void setClockSpeed(uint32_t freq)
{
    lastFrequency = freq;
    clockHalted = false;
    // uint32_t pll_freq = freq * 8;
    // si5351.setupPLL(SI5351_PLL_A, pll_freq / 25000000, 0, 1);
    // si5351.setupMultisynth(0, SI5351_PLL_A, 8, 0, 1);
    // si5351.enableOutputs(true);
    updateDisplay();
}

void haltClock()
{
    clockHalted = true;
    // si5351.enableOutputs(false);
    updateDisplay();
}

void resumeClock()
{
    clockHalted = false;
    setClockSpeed(lastFrequency);
}

void handleClock()
{
    String action = server.arg("action");
    if (action == "set" && server.hasArg("frequency"))
    {
        uint32_t frequency = server.arg("frequency").toInt();
        setClockSpeed(frequency);
        server.send(200, "application/json", "{\"success\":true}");
    }
    else if (action == "halt")
    {
        haltClock();
        server.send(200, "application/json", "{\"success\":true}");
    }
    else if (action == "resume")
    {
        resumeClock();
        server.send(200, "application/json", "{\"success\":true}");
    }
    else if (action == "state")
    {
        String state = clockHalted ? "halted" : "running";
        server.send(200, "application/json", "{\"state\":\"" + state + "\"}");
    }
    else
    {
        server.send(400, "application/json", "{\"success\":false}");
    }
}

void handleRoot()
{
  File file = LittleFS.open("/index.html", "r");
  if (!file)
  {
    server.send(404, "text/plain", "index.html not found on LittleFS.");
    return;
  }

  server.streamFile(file, "text/html");
  file.close();
}

void handleNotFound()
{
  server.send(404, "text/plain", "File Not Found");
}

void setup()
{
  initializeDisplay();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  ipAddress = WiFi.localIP().toString();
  updateDisplay();

  if (!LittleFS.begin())
  {
    tft.setCursor(10, 10);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.println("LittleFS mount failed!");
  }

  // if (si5351.begin() != 0)
  // {
  //   tft.setCursor(10, 30);
  //   tft.setTextColor(ST77XX_RED);
  //   tft.setTextSize(2);
  //   tft.println("SI5351 init failed!");
  // }

  server.onNotFound(handleNotFound);
  server.on("/", handleRoot);
  server.on("/clock", handleClock);

  server.begin();
}

void loop()
{
  server.handleClient();
}