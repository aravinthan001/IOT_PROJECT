#include <SoftwareSerial.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>  // LCD library
#include <FS.h>  // For file system (to store CSV file)

// Pin definitions for the sensor's RX and TX
#define RE 2   // Transmit enable
#define DE 0   // Receive enable

// NPK sensor request commands
const byte nitro[] = {0x01, 0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c}; // Nitrogen request
const byte phos[] = {0x01, 0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc}; // Phosphorus request
const byte pota[] = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0}; // Potassium request

// Store values read from the sensor
byte values[11];

// SoftwareSerial for communication with the sensor
SoftwareSerial mod(14, 12); // RX = D5 (GPIO14), TX = D6 (GPIO12)

// DHT11 sensor setup
#define DHTPIN 13  // Pin connected to DHT11 (GPIO13, corresponding to D7)
DHT dht(DHTPIN, DHT11);

// LDR sensor setup
#define LDRPIN A0  // Pin connected to LDR sensor

// Wi-Fi credentials
const char* ssid = "npk";
const char* password = "12345678";

// Create web server on port 80
ESP8266WebServer server(80);

// LCD display setup (16x2 I2C)
LiquidCrystal_I2C lcd(0x27, 16, 2); // 0x3F is the I2C address for the LCD

void setup() {
  // Start serial communication for debugging
  Serial.begin(115200);

  // Initialize Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  // Wait for Wi-Fi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Initialize LCD
  lcd.begin();     // Set up the LCD's number of columns and rows
  lcd.clear();          // Clear the display
  lcd.setCursor(0, 0);  // Set the cursor to the top-left corner

  // Show the IP address on LCD
  String ipAddress = WiFi.localIP().toString();
  lcd.print("");
  lcd.println(ipAddress);

  // Initialize SoftwareSerial for NPK sensor
  mod.begin(4800);  // Set baud rate for communication with NPK sensor

  // Initialize DHT sensor
  dht.begin();

  // Set RE and DE pins as output
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);

  // Initialize SPIFFS file system
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialization failed!");
    return;
  }

  // Serve the webpage
  server.on("/", HTTP_GET, handleRoot);
  server.on("/download", HTTP_GET, handleDownload);

  // Start the server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // Handle client requests
  server.handleClient();
}

// Webpage handler
void handleRoot() {
  // Get sensor readings
  int ldrValue = analogRead(LDRPIN);
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Request nitrogen, phosphorus, and potassium values
  byte nitrogenVal = nitrogen();
  byte phosphorusVal = phosphorous();
  byte potassiumVal = potassium();

  // Build the HTML response
  String html = "<html><head>";
  html += "<style>";
  html += "body {";
  html += "background-image: url('https://th.bing.com/th/id/OIP.Kvdo_oxoNBptTTR8rAiYiwHaEv?w=259&h=180&c=7&r=0&o=5&dpr=1.3&pid=1.7');";
  html += "background-size: cover;";
  html += "color: white;";
  html += "font-family: Arial, sans-serif;";
  html += "text-align: center;";
  html += "padding: 20px;";
  html += "}";

  html += "h1 { font-size: 32px; }";
  html += "p { font-size: 20px; margin: 10px 0; }";
  html += "a { color: #00BFFF; text-decoration: none; font-size: 18px; padding: 10px; border: 2px solid #00BFFF; border-radius: 5px; }";
  html += "a:hover { background-color: #00BFFF; color: white; }";
  html += "</style>";
  html += "</head><body>";

  // Add sensor readings to the page
  html += "<h1>Sensor Data</h1>";
  html += "<p>LDR Value: " + String(ldrValue) + "</p>";
  html += "<p>Temperature: " + String(temperature) + "°C</p>";
  html += "<p>Humidity: " + String(humidity) + "%</p>";
  html += "<p>Nitrogen: " + String(nitrogenVal) + " mg/kg</p>";
  html += "<p>Phosphorus: " + String(phosphorusVal) + " mg/kg</p>";
  html += "<p>Potassium: " + String(potassiumVal) + " mg/kg</p>";
  html += "<a href='/download'>Download CSV</a>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

// Download CSV file handler with appended data
void handleDownload() {
  // Open the CSV file in append mode
  File dataFile = SPIFFS.open("/sensor_data.csv", "a");

  // If the file is available, append data
  if (dataFile) {
    int ldrValue = analogRead(LDRPIN);
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    byte nitrogenVal = nitrogen();
    byte phosphorusVal = phosphorous();
    byte potassiumVal = potassium();
    
    // Get current time for timestamp
    String timestamp = String(millis() / 1000);  // Time in seconds since the start of the program

    // Append the new data
    dataFile.print(timestamp);  // Add the timestamp
    dataFile.print(",");
    dataFile.print(ldrValue);   // LDR value
    dataFile.print(",");
    dataFile.print(temperature);  // Temperature
    dataFile.print(",");
    dataFile.print(humidity);  // Humidity
    dataFile.print(",");
    dataFile.print(nitrogenVal);  // Nitrogen
    dataFile.print(",");
    dataFile.print(phosphorusVal);  // Phosphorus
    dataFile.print(",");
    dataFile.println(potassiumVal);  // Potassium

    // Close the file after writing
    dataFile.close();
    
    // Send the CSV file to the client
    File file = SPIFFS.open("/sensor_data.csv", "r");
    if (file) {
      server.streamFile(file, "text/csv");
      file.close();
    }
  } else {
    Serial.println("Error opening file for writing.");
  }
}

// Function to get nitrogen value from the sensor
byte nitrogen() {
  digitalWrite(DE, HIGH);  // Set to Transmit mode
  digitalWrite(RE, HIGH);  // Set to Transmit mode
  delay(10);

  if (mod.write(nitro, sizeof(nitro)) == 8) {
    digitalWrite(DE, LOW);  // Set to Receive mode
    digitalWrite(RE, LOW);
    delay(100);

    for (byte i = 0; i < 7; i++) {
      values[i] = mod.read();
    }
  }

  return values[4];
}

// Function to get phosphorus value from the sensor
byte phosphorous() {
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);

  if (mod.write(phos, sizeof(phos)) == 8) {
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
    delay(100);

    for (byte i = 0; i < 7; i++) {
      values[i] = mod.read();
    }
  }

  return values[4];
}

// Function to get potassium value from the sensor
byte potassium() {
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);

  if (mod.write(pota, sizeof(pota)) == 8) {
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
    delay(100);

    for (byte i = 0; i < 7; i++) {
      values[i] = mod.read();
    }
  }

  return values[4];
}
