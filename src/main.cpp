#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#define TFT_CS   D8   // GPIO15
#define TFT_DC   D1   // GPIO5
#define TFT_RST  D2   // GPIO4

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  Serial.begin(115200);
  // Wait a moment for Serial to initialize
  delay(1000);

  tft.init(240, 320, SPI_MODE2); 
  tft.setRotation(0); // Adjust rotation as needed (0-3)

  // Fill the screen with black
  tft.fillScreen(ST77XX_BLACK);

  // Draw some simple text:
  tft.setCursor(10, 10);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.println("Hello, ST7789!");

  // Draw a red rectangle
  tft.fillRect(50, 50, 100, 100, ST77XX_RED);

  // Draw a blue circle
  tft.fillCircle(120, 200, 30, ST77XX_BLUE);
}

void loop() {
  // Nothing to do here, everything is drawn once in setup().
}
