#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>  

// Pin Setup for L298N
const int motor1IN1Pin = 32;  // Motor 1 IN1 (Forward)
const int motor1IN2Pin = 33;  // Motor 1 IN2 (Backward)
const int motor2IN3Pin = 35;  // Motor 2 IN3 (Forward)
const int motor2IN4Pin = 26;  // Motor 2 IN4 (Backward)

const int waterPumpPin = 14;  // Relay control pin for the water pump (connected to relay IN)
const int flameSensorPin = 34;  // Flame sensor pin
const int servoPin = 13;  // Pin for servo motor

LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD setup

const char* ssid = "fire";  // Replace with your Wi-Fi SSID
const char* password = "12345678";  // Replace with your Wi-Fi password

AsyncWebServer server(80);

Servo waterTubeServo;  // Create a Servo object to control the water tube's head

void setup() {
  Serial.begin(115200);

  // Set up motor pins
  pinMode(motor1IN1Pin, OUTPUT);
  pinMode(motor1IN2Pin, OUTPUT);
  pinMode(motor2IN3Pin, OUTPUT);
  pinMode(motor2IN4Pin, OUTPUT);
  pinMode(waterPumpPin, OUTPUT);  // Set relay control pin as output
  pinMode(flameSensorPin, INPUT);

  // Initialize LCD
  lcd.begin();
  lcd.backlight();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Display the IP address on the LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP Address:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  // Initialize Servo
  waterTubeServo.attach(servoPin);
  waterTubeServo.write(90);  // Initial position of servo (middle)

  // Serve the web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><head>";
    
    // Adding background image and button styles
    html += "<style>";
    html += "body {";
    html += "  background-image: url('https://th.bing.com/th/id/OIP.I_JXJESWUMKXmnFE0D9a-QHaE7?w=500&h=333&rs=1&pid=ImgDetMain');";
    html += "  background-size: cover;";
    html += "  background-position: center;";
    html += "  font-family: Arial, sans-serif;";
    html += "  color: white;";
    html += "  text-align: center;";
    html += "  padding: 50px 0;";
    html += "}" ;

    html += "h1 {";
    html += "  font-size: 36px;";
    html += "  margin-bottom: 30px;";
    html += "  text-shadow: 2px 2px 10px rgba(0,0,0,0.5);";
    html += "  animation: fadeIn 2s ease-out;";
    html += "}" ;

    html += "h2 {";
    html += "  font-size: 28px;";
    html += "  text-shadow: 2px 2px 5px rgba(0,0,0,0.3);";
    html += "  animation: fadeIn 3s ease-out;";
    html += "}" ;

    html += "p {";
    html += "  font-size: 20px;";
    html += "  text-shadow: 1px 1px 5px rgba(0,0,0,0.3);";
    html += "  animation: fadeIn 4s ease-out;";
    html += "}" ;

    html += "button {";
    html += "  background-color: #4CAF50;";
    html += "  border: none;";
    html += "  color: white;";
    html += "  padding: 15px 32px;";
    html += "  text-align: center;";
    html += "  text-decoration: none;";
    html += "  display: inline-block;";
    html += "  font-size: 16px;";
    html += "  margin: 4px 2px;";
    html += "  cursor: pointer;";
    html += "  border-radius: 8px;";
    html += "  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);";
    html += "  transition: all 0.3s ease;";
    html += "}" ;

    html += "button:hover {";
    html += "  background-color: #45a049;";
    html += "  box-shadow: 0 8px 16px rgba(0, 0, 0, 0.3);";
    html += "  transform: translateY(-2px);";
    html += "}" ;

    html += "@keyframes fadeIn {";
    html += "  0% { opacity: 0; }";
    html += "  100% { opacity: 1; }";
    html += "}";

    html += "</style>";
    html += "</head><body>";

    html += "<h1>Fire Fighting Robot</h1>";

    // Flame sensor status
    int flameSensorValue = digitalRead(flameSensorPin);
    html += "<h2>Flame Sensor Status: ";
    if (flameSensorValue == HIGH) {
      html += "Fire Detected</h2>";
    } else {
      html += "No Fire</h2>";
    }

    html += "<h2>Manual Mode</h2>";
    html += "<button onclick=\"location.href='/moveForward'\">Move Forward</button><br>";
    html += "<button onclick=\"location.href='/moveBackward'\">Move Backward</button><br>";
    html += "<button onclick=\"location.href='/moveLeft'\">Turn Left</button><br>";
    html += "<button onclick=\"location.href='/moveRight'\">Turn Right</button><br>";
    html += "<button onclick=\"location.href='/stop'\">Stop Movement</button><br><br>";
    html += "<button onclick=\"location.href='/activatePump'\">Activate Water Pump</button><br>";
    html += "<button onclick=\"location.href='/deactivatePump'\">Deactivate Water Pump</button><br>";
    html += "<button onclick=\"location.href='/servoLeft'\">Turn Servo Left</button><br>";
    html += "<button onclick=\"location.href='/servoRight'\">Turn Servo Right</button><br>";

    html += "</body></html>";

    request->send(200, "text/html", html);
  });

  // Manual control actions
  server.on("/moveForward", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(motor1IN1Pin, HIGH);
    digitalWrite(motor1IN2Pin, LOW);
    digitalWrite(motor2IN3Pin, HIGH);
    digitalWrite(motor2IN4Pin, LOW);
    request->redirect("/");  // Redirect back to the homepage
  });

  server.on("/moveBackward", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(motor1IN1Pin, LOW);
    digitalWrite(motor1IN2Pin, HIGH);
    digitalWrite(motor2IN3Pin, LOW);
    digitalWrite(motor2IN4Pin, HIGH);
    request->redirect("/");  // Redirect back to the homepage
  });

  server.on("/moveLeft", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(motor1IN1Pin, LOW);  // Left motor backward
    digitalWrite(motor1IN2Pin, HIGH);
    digitalWrite(motor2IN3Pin, HIGH);  // Right motor forward
    digitalWrite(motor2IN4Pin, LOW);
    request->redirect("/");  // Redirect back to the homepage
  });

  server.on("/moveRight", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(motor1IN1Pin, HIGH);  // Left motor forward
    digitalWrite(motor1IN2Pin, LOW);
    digitalWrite(motor2IN3Pin, LOW);  // Right motor backward
    digitalWrite(motor2IN4Pin, HIGH);
    request->redirect("/");  // Redirect back to the homepage
  });

  // Stop movement
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(motor1IN1Pin, LOW);
    digitalWrite(motor1IN2Pin, LOW);
    digitalWrite(motor2IN3Pin, LOW);
    digitalWrite(motor2IN4Pin, LOW);
    request->redirect("/");  // Redirect back to the homepage
  });

  // Water Pump control (manual)
  server.on("/activatePump", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(waterPumpPin, LOW); // Activate the relay (turn on pump)
    request->redirect("/");  // Redirect back to the homepage
  });

  server.on("/deactivatePump", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(waterPumpPin, HIGH);  // Deactivate the relay (turn off pump)
    request->redirect("/");  // Redirect back to the homepage
  });

  // Servo control (manual)
  server.on("/servoLeft", HTTP_GET, [](AsyncWebServerRequest *request) {
    waterTubeServo.write(45);  // Move servo to 45 degrees (left)
    request->redirect("/");  // Redirect back to the homepage
  });

  server.on("/servoRight", HTTP_GET, [](AsyncWebServerRequest *request) {
    waterTubeServo.write(135);  // Move servo to 135 degrees (right)
    request->redirect("/");  // Redirect back to the homepage
  });

  // Start server
  server.begin();
}

void loop() {
  // Nothing needed here for manual mode
}
