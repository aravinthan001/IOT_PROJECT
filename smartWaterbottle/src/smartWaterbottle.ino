#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pin configuration using GPIO numbers directly
#define TEMP_PIN 0      // GPIO0 (instead of D3)
#define TRIG_PIN 13     // GPIO13 (D7)
#define ECHO_PIN 12     // GPIO12 (D6)
#define SWITCH_PIN 14   // GPIO14 (instead of D5)

const char* ssid = "water";
const char* password = "12345678";

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
ESP8266WebServer server(80);
Adafruit_SSD1306 display(128, 64, &Wire, -1);

long duration;
int distance;

void setup() {
  Serial.begin(115200);

  // WiFi connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // Initialize sensors
  sensors.begin();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  // OLED Display setup
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Web server routes
  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();

  // Measure distance with ultrasonic sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);  // Ensure there is no residual pulse
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);  // Trigger pulse for 10 microseconds
  digitalWrite(TRIG_PIN, LOW);

  // Read the duration of the pulse reflected back
  duration = pulseIn(ECHO_PIN, HIGH);  // Time in microseconds

  // Calculate the distance (in cm)
  distance = duration * 0.0344 / 2;

  // Debugging output to the Serial Monitor
  Serial.print("Duration: ");
  Serial.println(duration);
  Serial.print("Distance: ");
  Serial.println(distance);

  // Read temperature from DS18B20 sensor
  sensors.requestTemperatures();
  float waterTemp = sensors.getTempCByIndex(0);

  // Display on OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Water Temp: ");
  display.print(waterTemp);
  display.print(" C");

  display.setCursor(0, 20);
  display.print("Water Level: ");
  display.print(distance);
  display.print(" cm");

  // Display IP Address
  IPAddress ip = WiFi.localIP();
  display.setCursor(0, 40);
  display.print("IP Address: ");
  display.print(ip);

  display.display();
}

void handleRoot() {
  float waterTemp = sensors.getTempCByIndex(0);
  String html = "<html><head><style>";

  html += "body {";
  html += "  background-image: url('https://pics.craiyon.com/2023-11-18/bCGKKFolTFCVa2LRNyNYmQ.webp');";
  html += "  background-size: cover;";
  html += "  background-position: center;";
  html += "  color: white;";
  html += "  text-align: center;";
  html += "  font-family: 'Arial', sans-serif;";
  html += "  height: 100vh;";
  html += "  margin: 0;";
  html += "  display: flex;";
  html += "  flex-direction: column;";
  html += "  justify-content: center;";
  html += "  align-items: center;";
  html += "}";

  html += "h1 {";
  html += "  font-size: 3em;";
  html += "  text-shadow: 2px 2px 5px rgba(0, 0, 0, 0.7);";
  html += "  margin-bottom: 20px;";
  html += "}";

  html += "p {";
  html += "  font-size: 1.5em;";
  html += "  margin: 10px 0;";
  html += "  text-shadow: 1px 1px 4px rgba(0, 0, 0, 0.5);";
  html += "}";

  html += "</style></head><body>";

  html += "<h1>Smart Water Bottle</h1>";
  html += "<p>Water Temperature: " + String(waterTemp) + " &#8451;</p>";
  html += "<p>Water Level: " + String(distance) + " cm</p>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}
///d1  for scl d2 for sda d3 for temp d6 echo d7 trig 
