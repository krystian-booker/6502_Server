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

// User Config
const char *ssid = "(╯▔皿▔)╯_EXT";
const char *password = "kr1stis!";

ESP8266WebServer server(80);
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_SI5351 si5351;

bool clockHalted = false;
uint32_t lastFrequency = 1000;
String lastUnit = "kHz";
String ipAddress = "0.0.0.0";

static uint32_t oldFrequency = 0;
static bool oldHalted = true;
static String oldIP = "";

void setClockSpeed(uint32_t freq, const String &unit);
void haltClock();
void resumeClock();
void updateDisplay();
void drawHeader();
void drawFooter();

void updateFrequencyOnScreen(uint32_t newFrequency, uint32_t oldFrequency);
void updateHaltedStateOnScreen(bool halted, bool oldHalted);
void updateIPAddressOnScreen(String newIP, String oldIP);

void initializeDisplay()
{
  tft.init(240, 320, SPI_MODE2);
  tft.setRotation(0);

  tft.fillScreen(ST77XX_BLACK);

  drawHeader();
  drawFooter();

  tft.drawLine(0, 30, 240, 30, ST77XX_WHITE);
  tft.drawLine(0, 300, 240, 300, ST77XX_WHITE);

  tft.drawRoundRect(10, 40, 220, 60, 5, ST77XX_WHITE);
  tft.drawRoundRect(10, 110, 220, 40, 5, ST77XX_WHITE);
  tft.drawRoundRect(10, 160, 220, 40, 5, ST77XX_WHITE);
}

void drawHeader()
{
  tft.fillRect(0, 0, 240, 30, ST77XX_BLUE);
  tft.setCursor(10, 5);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("6502 Clock");
}

void drawFooter()
{
  tft.fillRect(0, 300, 240, 20, ST77XX_BLUE);
  tft.setCursor(10, 303);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Krystian Booker");
}

void updateDisplay()
{
  updateFrequencyOnScreen(lastFrequency, oldFrequency);
  updateHaltedStateOnScreen(clockHalted, oldHalted);
  updateIPAddressOnScreen(ipAddress, oldIP);

  oldFrequency = lastFrequency;
  oldHalted = clockHalted;
  oldIP = ipAddress;
}

void updateFrequencyOnScreen(uint32_t newFrequency, uint32_t oldFrequency)
{
  if (newFrequency != oldFrequency)
  {
    tft.fillRect(20, 50, 200, 40, ST77XX_BLACK);
    tft.setCursor(20, 50);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(3);

    if (newFrequency % 1000000 == 0)
    {
      tft.print(newFrequency / 1000000);
      tft.print(" MHz");
    }
    else if (newFrequency % 1000 == 0)
    {
      tft.print(newFrequency / 1000);
      tft.print(" kHz");
    }
    else
    {
      tft.print(newFrequency);
      tft.print(" Hz");
    }
  }
}

void updateHaltedStateOnScreen(bool halted, bool oldHalted)
{
  if (halted != oldHalted)
  {
    tft.fillRect(20, 120, 200, 20, ST77XX_BLACK);
    tft.setCursor(20, 120);
    tft.setTextSize(2);
    tft.setTextColor(halted ? ST77XX_RED : ST77XX_GREEN);
    tft.print("Halted: ");
    tft.print(halted ? "True" : "False");
  }
}

void updateIPAddressOnScreen(String newIP, String oldIP)
{
  if (newIP != oldIP)
  {
    tft.fillRect(20, 170, 200, 20, ST77XX_BLACK);
    tft.setCursor(20, 170);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("IP: ");
    tft.print(newIP);
  }
}

void setClockSpeed(uint32_t freq, const String &unit)
{
  if (unit == "kHz")
  {
    freq *= 1000;
  }
  else if (unit == "MHz")
  {
    freq *= 1000000;
  }
  else
  {
    return; // Invalid unit
  }

  if (freq < 8000 || freq > 14000000)
  {
    return;
  }

  lastFrequency = freq;
  lastUnit = unit;
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
  // si5351.enableOutputs(true);
  updateDisplay();
}

void handleClock()
{
  String action = server.arg("action");
  if (action == "set" && server.hasArg("frequency") && server.hasArg("unit"))
  {
    uint32_t frequency = server.arg("frequency").toInt();
    String unit = server.arg("unit");

    // Convert the frequency based on the unit
    if (unit == "kHz")
    {
      frequency *= 1000; // Convert kHz to Hz
    }
    else if (unit == "MHz")
    {
      frequency *= 1000000; // Convert MHz to Hz
    }
    else
    {
      server.send(400, "application/json", "{\"success\":false, \"error\":\"Invalid unit specified\"}");
      return;
    }

    // Validate frequency range
    if (frequency < 8000 || frequency > 14000000)
    {
      server.send(400, "application/json", "{\"success\":false, \"error\":\"Frequency out of range (8 kHz to 14 MHz)\"}");
      return;
    }

    // Set the clock speed if validation passes
    setClockSpeed(frequency / (unit == "kHz" ? 1000 : (unit == "MHz" ? 1000000 : 1)), unit);

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

 // Set the default clock speed to 1 MHz
  setClockSpeed(1, "MHz");

  server.onNotFound(handleNotFound);
  server.on("/", handleRoot);
  server.on("/clock", handleClock);

  server.begin();
}

void loop()
{
  server.handleClient();
}
