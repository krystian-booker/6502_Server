#include <Arduino.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ----------- USER CONFIG -----------
const char* ssid = "(╯▔皿▔)╯_EXT";
const char* password = "kr1stis!";
// -----------------------------------

// Pins for the ST7789
#define TFT_CS   D8   // GPIO15
#define TFT_DC   D1   // GPIO5
#define TFT_RST  D2   // GPIO4

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Create a web server object that listens on port 80
ESP8266WebServer server(80);

// We will store the last displayed message here
String displayedText = "Hello, ST7789!";

void handleRoot() {
  // HTML page with Bootstrap styling
  String html =  "<!DOCTYPE html>"
                 "<html lang='en'>"
                 "<head>"
                 "  <meta charset='utf-8'>"
                 "  <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                 "  <title>NodeMCU Text Input</title>"
                 "  <link href='https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css' rel='stylesheet'>"
                 "</head>"
                 "<body class='bg-light'>"
                 "  <div class='container mt-5'>"
                 "    <div class='card shadow-sm'>"
                 "      <div class='card-header bg-primary text-white'>"
                 "        <h1 class='h3 mb-0'>ST7789 Text Update</h1>"
                 "      </div>"
                 "      <div class='card-body'>"
                 "        <form action='/update' method='GET'>"
                 "          <div class='mb-3'>"
                 "            <label for='message' class='form-label'>Enter text to display:</label>"
                 "            <input type='text' id='message' name='message' class='form-control' placeholder='Type your text here' required>"
                 "          </div>"
                 "          <button type='submit' class='btn btn-primary'>Update Display</button>"
                 "        </form>"
                 "      </div>"
                 "    </div>"
                 "  </div>"
                 "</body>"
                 "</html>";

  server.send(200, "text/html", html);
}

void handleUpdate() {
  // If the client sent a "message" parameter, update the displayed text
  if (server.hasArg("message")) {
    displayedText = server.arg("message");

    // Print to Serial as well (for debugging)
    Serial.println("Received new text: " + displayedText);

    // Clear the screen and update the display immediately
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(10, 10);
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(2);
    tft.println(displayedText);
  }
  
  // Send the user back to the main page
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET )?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("NodeMCU IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize ST7789
  tft.init(240, 320, SPI_MODE2);
  tft.setRotation(1); // Adjust rotation as needed (0-3)
  tft.fillScreen(ST77XX_BLACK);

  // Draw an initial message on the display
  tft.setCursor(10, 10);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.println(displayedText);

  // Setup server handlers
  server.on("/", handleRoot);
  server.on("/update", handleUpdate);
  server.onNotFound(handleNotFound);

  // Start the server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Continuously handle incoming client requests
  server.handleClient();
}
