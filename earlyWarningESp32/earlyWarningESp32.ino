#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <FS.h>
#include <SPIFFS.h>

// Wi-Fi credentials
const char* ssid = "early";
const char* password = "12345678";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// DHT11 setup
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adjust I2C address if necessary

// GPS setup
HardwareSerial GPS_Serial(1);
TinyGPSPlus gps;
#define GPS_RX 16
#define GPS_TX 17

// Ultrasonic sensor setup
#define TRIG_PIN 5
#define ECHO_PIN 18

// Rain sensor setup
#define RAIN_SENSOR_PIN 34

// Water Flow Sensor setup
#define FLOW_SENSOR_PIN 32
volatile int flowPulseCount = 0;
unsigned long lastFlowCheck = 0;
float flowRate = 0.0;

void IRAM_ATTR flowPulse() {
  flowPulseCount++;
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  lcd.begin();
  lcd.backlight();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), flowPulse, RISING);

  GPS_Serial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  WiFi.begin(ssid, password);
  lcd.print("Connecting WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  lcd.clear();
  lcd.print(WiFi.localIP());
  Serial.println(WiFi.localIP());

  // Initialize SPIFFS (to store CSV file)
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  // Create or open CSV file for appending and add headers if the file is empty
  File file = SPIFFS.open("/data.csv", FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  
  // Check if the file is empty, and add headers if necessary
  if (file.size() == 0) {
    file.println("Date,Time,Temperature,Humidity,Water Flow,Water Level");  // Add headers
  }
  file.close();  // Close the file

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    unsigned long currentTime = millis();
    if (currentTime - lastFlowCheck >= 1000) {
      flowRate = (flowPulseCount / 7.5);  // Adjust conversion factor as needed
      flowPulseCount = 0;
      lastFlowCheck = currentTime;
    }

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = duration * 0.034 / 2;

    // Get current date and time
    String dateTime = getDateTime();  // This function generates the current timestamp

    // Prepare CSV data
    String csvData = dateTime + "," + String(temp) + "," + String(hum) + "," + String(flowRate, 2) + "," +
                     String(distance, 2) + "," + (digitalRead(RAIN_SENSOR_PIN) == HIGH ? "Dry" : "Raining");

    // Append data to CSV file on SPIFFS
    File file = SPIFFS.open("/data.csv", FILE_APPEND);
    if (file) {
      file.println(csvData);  // Append the data
      file.close();  // Close the file
    } else {
      Serial.println("Failed to open file for writing");
    }

    // Return HTML page
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>ESP32 Early Warning System</title>";
    html += "<style>body { font-family: Arial, sans-serif; background-color: #f4f4f9; text-align: center; }";
    html += "button { background-color: #4CAF50; color: white; padding: 10px 20px; border: none;";
    html += "cursor: pointer; border-radius: 5px; } button:hover { background-color: #45a049; }";
    html += "</style></head><body>";
    html += "<h1>ESP32 Early Warning System</h1>";
    html += "<p><strong>Temperature:</strong> " + String(temp) + " &deg;C</p>";
    html += "<p><strong>Humidity:</strong> " + String(hum) + " %</p>";
    html += "<p><strong>Distance:</strong> " + String(distance) + " cm</p>";
    html += "<p><strong>Water Flow Rate:</strong> " + String(flowRate, 2) + " L/min</p>";
    if (gps.location.isValid()) {
      html += "<p><strong>GPS Location:</strong> " + String(gps.location.lat(), 6) + ", " + String(gps.location.lng(), 6) + "</p>";
      html += "<p><a href='https://www.openstreetmap.org/?mlat=" + String(gps.location.lat(), 6) +
              "&mlon=" + String(gps.location.lng(), 6) + "#map=15' target='_blank'><button>View on OpenStreetMap</button></a></p>";
    } else {
      html += "<p><strong>GPS:</strong> No Signal</p>";
    }
    html += "<p><strong>Rain Status:</strong> " + String(digitalRead(RAIN_SENSOR_PIN) == HIGH ? "Dry" : "Raining") + "</p>";
    html += "<p><a href='/download'><button>Download CSV Data</button></a></p>";
    html += "</body></html>";

    request->send(200, "text/html", html);
  });

  // CSV Download Handler
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    File file = SPIFFS.open("/data.csv", FILE_READ);
    if (!file) {
      request->send(500, "text/plain", "Failed to open file for reading");
      return;
    }
    String csv = file.readString();  // Read the whole file
    file.close();
    request->send(200, "text/csv", csv);
  });

  server.begin();
}

void loop() {
  while (GPS_Serial.available() > 0) {
    gps.encode(GPS_Serial.read());
  }
}

// Function to get the current date and time
String getDateTime() {
  unsigned long currentMillis = millis();
  unsigned long hours = (currentMillis / 3600000) % 24;
  unsigned long minutes = (currentMillis / 60000) % 60;
  unsigned long seconds = (currentMillis / 1000) % 60;
  
  unsigned long days = currentMillis / 86400000; // Calculate the days from millis
  int year = 2025; // Example year, adjust as needed
  int month = 3;   // Example month, adjust as needed
  int day = days % 31;  // Adjust based on the month
  
  // Format the date and time
  String dateTime = String(year) + "-" + String(month) + "-" + String(day) + " " + 
                    String(hours) + ":" + String(minutes) + ":" + String(seconds);
  return dateTime;
}
