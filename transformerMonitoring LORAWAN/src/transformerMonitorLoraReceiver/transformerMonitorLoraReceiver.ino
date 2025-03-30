#include <Wire.h>
#include <LoRa.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Define LoRa pins
#define LORA_CS     5    // Chip Select (CS)
#define LORA_RST    15   // Reset (RST)
#define LORA_DIO0   2    // Digital IO (DIO0)

// OLED Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Web Server setup
const char* ssid = "transformer";      // Your WiFi SSID
const char* password = "12345678"; // Your WiFi password
AsyncWebServer server(80);  // Web server runs on port 80

// Variables for displaying data
String receivedData = "No Data"; // Store received data to display

// Initialize LoRa
void setup() {
  Serial.begin(115200);

  // Initialize LoRa module
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initialized.");

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address of the display
    Serial.println(F("SSD1306 allocation failed"));
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Display IP address on OLED
  String ipAddress = WiFi.localIP().toString();
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("IP: ");
  display.println(ipAddress);
  display.display();

  // Start web server to display data
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = "<html><head><style>";
    html += "body { font-family: Arial, sans-serif; background-color: #f0f0f0; margin: 0; padding: 20px; }";
    html += "h1 { color: #333; text-align: center; font-size: 40px; }";
    html += "p { font-size: 18px; text-align: center; font-weight: bold; color: #333; }";
    html += "</style></head><body>";
    html += "<h1>Receiver Side Data</h1>";
    html += "<p>" + receivedData + "</p>";  // Show received data on the webpage
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Start server
  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  // Check if data is available from LoRa
  if (LoRa.parsePacket()) {
    receivedData = "";  // Clear previous data

    // Read the incoming LoRa data
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }

    // Debug: Print the received data
    Serial.println("Received LoRa Data: " + receivedData);

    // Display received data on OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("Received Data: ");
    display.println(receivedData);
    display.display();
  }

  delay(1000);  // Wait 1 second before checking again
}
