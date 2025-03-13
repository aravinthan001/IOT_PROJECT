#include <TinyGPS++.h>
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// GPS and OLED Definitions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

TinyGPSPlus gps;
HardwareSerial mySerial(1); // Use HardwareSerial for GPS

// GPS Module TX/RX Pins
const int RXPin = 16;  // RX Pin connected to TX of GPS
const int TXPin = 17;  // TX Pin connected to RX of GPS

// WiFi credentials
const char* ssid = "Airtel_SMC_IOT";
const char* password = "12345678";

// GPIO Definitions
#define OBJECT_SENSOR_TRIGGER_PIN 33
#define OBJECT_SENSOR_ECHO_PIN 34
#define HOLE_SENSOR_TRIGGER_PIN 25
#define HOLE_SENSOR_ECHO_PIN 26
#define RAIN_SENSOR_PIN A0  // Analog pin for rain sensor
#define VIBRATION_SENSOR_PIN 32
#define BUTTON_PIN 14
#define BUZZER_PIN 13
#define VOICE_MODULE_PIN 12

// Variables for sensors and GPS
float Latitude, Longitude;
String LatitudeString, LongitudeString, currentTime;
bool objectDetected = false, holeDetected = false, rainDetected = false, fallDetected = false, helpNeeded = false;

WiFiServer server(80);

// Function to measure distance using ultrasonic sensor
long getDistance(int triggerPin, int echoPin) {
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  long distance = (duration / 2) / 29.1;  // Convert the time into distance (cm)
  return distance;
}

void setup() {
  // Initialize Serial for Debugging
  Serial.begin(115200);

  // Initialize the GPS serial communication
  mySerial.begin(9600, SERIAL_8N1, RXPin, TXPin); // RX = 16, TX = 17 for GPS module
  Serial.println("GPS Module Initialized");

  // Initialize OLED Display
  if (!display.begin(SSD1306_PAGEADDR, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.display();

  // WiFi Setup
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println("IP address: " + WiFi.localIP().toString());

  // Start Web Server
  server.begin();

  // Configure GPIO pins
  pinMode(OBJECT_SENSOR_TRIGGER_PIN, OUTPUT);
  pinMode(OBJECT_SENSOR_ECHO_PIN, INPUT);
  pinMode(HOLE_SENSOR_TRIGGER_PIN, OUTPUT);
  pinMode(HOLE_SENSOR_ECHO_PIN, INPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);  // Assuming digital for now
  pinMode(VIBRATION_SENSOR_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(VOICE_MODULE_PIN, OUTPUT);

  // Turn off buzzer and voice module initially
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(VOICE_MODULE_PIN, LOW);

  // Display IP Address on OLED
  updateOLED();
}

void loop() {
  while (mySerial.available() > 0) {
    gps.encode(mySerial.read());  // Encode GPS data

    if (gps.location.isUpdated()) {
      // Get Latitude and Longitude
      Latitude = gps.location.lat();
      Longitude = gps.location.lng();
      LatitudeString = String(Latitude, 6);  // 6 decimal places
      LongitudeString = String(Longitude, 6);

      // Get Date and Time from GPS
      currentTime = String(gps.date.year()) + "-" + String(gps.date.month()) + "-" + String(gps.date.day()) + " " +
                    String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second());
    }
  }

  // Measure distances with ultrasonic sensors
  long objectDistance = getDistance(OBJECT_SENSOR_TRIGGER_PIN, OBJECT_SENSOR_ECHO_PIN);
  long holeDistance = getDistance(HOLE_SENSOR_TRIGGER_PIN, HOLE_SENSOR_ECHO_PIN);

  objectDetected = (objectDistance < 30);  // Object detected if distance is less than 30 cm
  holeDetected = (holeDistance > 10);     // Hole detected if distance is less than 10 cm

  // Read other sensor states
  int rainValue = analogRead(RAIN_SENSOR_PIN);
  rainDetected = (rainValue < 500);  // Detect rain if analog value is less than 500
  fallDetected = digitalRead(VIBRATION_SENSOR_PIN) == HIGH;
  helpNeeded = digitalRead(BUTTON_PIN) == LOW;

  // Handle Buzzer and Voice Module
  if (objectDetected || rainDetected || fallDetected) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  if (holeDetected) {
    digitalWrite(VOICE_MODULE_PIN, HIGH);  // Play voice on hole detection
  } else {
    digitalWrite(VOICE_MODULE_PIN, LOW);
  }

  // Serve Web Page or JSON Data
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("GET /data") >= 0) {
      // Serve JSON data
      String json = "{";
      json += "\"latitude\":\"" + LatitudeString + "\",";
      json += "\"longitude\":\"" + LongitudeString + "\",";
      json += "\"time\":\"" + currentTime + "\",";
      json += "\"object\":" + String(objectDetected) + ",";
      json += "\"hole\":" + String(holeDetected) + ",";
      json += "\"rain\":" + String(rainDetected) + ",";
      json += "\"fall\":" + String(fallDetected) + ",";
      json += "\"help\":" + String(helpNeeded);
      json += "}";
      client.print("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
      client.print(json);
    } else {
      // Serve Web Page
      client.print(generateWebPage());
    }
    client.stop();
  }

  // Update OLED Display
  updateOLED();
}

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("GPS Info:");
  display.println("Lat: " + LatitudeString);
  display.println("Lng: " + LongitudeString);
  display.println("Time: " + currentTime);
  display.println("IP: " + WiFi.localIP().toString());

  display.display();
}

String generateWebPage() {
  String html = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
  html += "<!DOCTYPE html><html lang=\"en\"><head>";
  html += "<meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>Blind Stick Monitoring</title>";
  html += "<link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css\" rel=\"stylesheet\">";
  html += "<script>";
  html += "function fetchData() {";
  html += "  fetch('/data').then(response => response.json()).then(data => {";
  html += "    document.getElementById('latitude').innerText = data.latitude;";
  html += "    document.getElementById('longitude').innerText = data.longitude;";
  html += "    document.getElementById('time').innerText = data.time;";
  html += "    document.getElementById('object').innerText = data.object ? 'Yes' : 'No';";
  html += "    document.getElementById('hole').innerText = data.hole ? 'Yes' : 'No';";
  html += "    document.getElementById('rain').innerText = data.rain ? 'Yes' : 'No';";
  html += "    document.getElementById('fall').innerText = data.fall ? 'Yes' : 'No';";
  html += "    document.getElementById('help').innerText = data.help ? 'Yes' : 'No';";
  html += "  });";
  html += "}";
  html += "setInterval(fetchData, 1000);";
  html += "</script>";
  html += "</head><body onload=\"fetchData()\">";
  html += "<div class=\"container my-4\">";
  html += "<h1 class=\"text-center\">Blind Stick Monitoring</h1>";
  html += "<div class=\"card my-3\">";
  html += "<div class=\"card-body\">";
  html += "<h4>Location</h4>";
  html += "<p><strong>Latitude:</strong> <span id=\"latitude\">Loading...</span></p>";
  html += "<p><strong>Longitude:</strong> <span id=\"longitude\">Loading...</span></p>";
  html += "<p><strong>Time:</strong> <span id=\"time\">Loading...</span></p>";
  html += "</div></div>";

  html += "<div class=\"card my-3\">";
  html += "<div class=\"card-body\">";
  html += "<h4>Sensor Status</h4>";
  html += "<p><strong>Object Detected:</strong> <span id=\"object\">Loading...</span></p>";
  html += "<p><strong>Hole Detected:</strong> <span id=\"hole\">Loading...</span></p>";
  html += "<p><strong>Rain Detected:</strong> <span id=\"rain\">Loading...</span></p>";
  html += "<p><strong>Fall Detected:</strong> <span id=\"fall\">Loading...</span></p>";
  html += "<p><strong>Help Needed:</strong> <span id=\"help\">Loading...</span></p>";
  html += "</div></div>";

  html += "<div class=\"text-center\">";
  html += "<button class=\"btn btn-primary\" onclick=\"fetchData();\">Refresh Now</button>";
  html += "</div>";

  html += "</div>";
  html += "</body></html>";
  return html;
}
