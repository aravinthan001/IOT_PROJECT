#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define pins
#define DHTPIN 15           // DHT11 data pin
#define DHTTYPE DHT11       // DHT11 sensor
#define SOIL_PIN 34         // Soil moisture sensor analog pin
#define RELAY1_PIN 13       // Relay 1 for temperature control
#define RELAY2_PIN 12       // Relay 2 for humidity and soil moisture control
#define RELAY3_PIN 14       // Relay 3 for soil moisture control
#define RELAY4_PIN 27       // Relay 4 for manual control

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Address of the LCD (0x27 for 16x2)

// DHT sensor setup
DHT dht(DHTPIN, DHTTYPE);

// WiFi credentials
const char* ssid = "agri";
const char* password = "12345678";

// Web server setup
AsyncWebServer server(80);

// Thresholds for automatic relay control
const float tempThreshold = 30.0; // Temperature threshold (in °C)
const float humidityThreshold = 60.0; // Humidity threshold (in %)
const int soilMoistureThreshold = 400; // Soil moisture threshold (dry soil)

void setup() {
  // Start Serial for debugging
  Serial.begin(115200);

  // Initialize DHT sensor
  dht.begin();

  // Initialize relays
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);

  // Initialize LCD
  lcd.begin();
  lcd.setBacklight(true);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  String ipAddress = WiFi.localIP().toString();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" " + ipAddress);
  delay(2000); // Display IP for 2 seconds

  // Serve web page with data and Relay 4 control
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    // Read sensor data
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int soilMoisture = analogRead(SOIL_PIN);  // Read soil moisture

    // Check for NaN readings
    if (isnan(temperature) || isnan(humidity)) {
      temperature = 0;
      humidity = 0;
    }

    // Relay control logic
    bool relay1Status = digitalRead(RELAY1_PIN);
    bool relay2Status = digitalRead(RELAY2_PIN);
    bool relay3Status = digitalRead(RELAY3_PIN);

    // Webpage HTML with background image and styling
    String html = "<html><head>";
    html += "<style>";
    html += "body { background-image: url('https://cdn.pixabay.com/photo/2022/06/22/09/29/agricultural-7277520_960_720.jpg'); background-size: cover; color: white; font-family: Arial, sans-serif; padding: 20px; text-align: center; }";
    html += "h1 { font-size: 32px; margin-bottom: 20px; }";
    html += "h2 { font-size: 24px; margin-top: 30px; }";
    html += "p { font-size: 18px; }";
    html += "button { padding: 10px 20px; font-size: 16px; margin: 10px; background-color: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; }";
    html += "button:hover { background-color: #45a049; }";
    html += "</style>";
    html += "</head><body>";
    html += "<h1>Sensor Data</h1>";
    html += "<p>Temperature: " + String(temperature) + " °C</p>";
    html += "<p>Humidity: " + String(humidity) + " %</p>";
    html += "<p>Soil Moisture: " + String(soilMoisture) + "</p>";

    // Display relay status
    html += "<h2>Relay Status</h2>";
    html += "<p>Relay 1: " + String(relay1Status == HIGH ? "ON" : "OFF") + "</p>";
    html += "<p>Relay 2: " + String(relay2Status == HIGH ? "ON" : "OFF") + "</p>";
    html += "<p>Relay 3: " + String(relay3Status == HIGH ? "ON" : "OFF") + "</p>";

    // Relay 4 control buttons
    html += "<h2>Relay 4 Control</h2>";
    html += "<button onclick=\"location.href='/relay4/on'\">Relay 4 OFF</button>";
    html += "<button onclick=\"location.href='/relay4/off'\">Relay 4 ON</button>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });

  // Relay control endpoint for Relay 4
  server.on("/relay4/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(RELAY4_PIN, HIGH);  // Turn on relay 4
    request->redirect("/");  // Redirect back to the main page
  });
  
  server.on("/relay4/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(RELAY4_PIN, LOW);   // Turn off relay 4
    request->redirect("/");  // Redirect back to the main page
  });

  // Start server
  server.begin();
}

void loop() {
  // Read sensor data
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int soilMoisture = analogRead(SOIL_PIN);  // Read soil moisture

  // Control relay 1 (temperature > threshold)
  if (temperature > tempThreshold) {
    digitalWrite(RELAY1_PIN, HIGH);  // Turn on relay 1 if temperature > threshold
  } else {
    digitalWrite(RELAY1_PIN, LOW);   // Turn off relay 1 if temperature <= threshold
  }

  // Control relay 2 (humidity > threshold and soil is dry)
  if (humidity > humidityThreshold && soilMoisture < soilMoistureThreshold) {
    digitalWrite(RELAY2_PIN, HIGH);  // Turn on relay 2 if humidity > threshold and soil is dry
  } else {
    digitalWrite(RELAY2_PIN, LOW);   // Turn off relay 2 otherwise
  }

  // Control relay 3 (soil moisture < threshold)
  if (soilMoisture < soilMoistureThreshold) {
    digitalWrite(RELAY3_PIN, HIGH);  // Turn on relay 3 if soil moisture is low
  } else {
    digitalWrite(RELAY3_PIN, LOW);   // Turn off relay 3 if soil moisture is adequate
  }

  delay(2000);  // Wait 2 seconds before checking again
}
