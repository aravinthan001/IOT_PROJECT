#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid = "pet";
const char* password = "12345678";

ESP8266WebServer server(80);
bool intruderDetected = false;   // Sensor status from pin 12
bool servoState = false;         // false = OFF position, true = ON position

Servo servoMotor;  // Create servo object

// Ultrasonic sensor pins (Updated)
const int trigPin = 12;  // D6 (GPIO 12)
const int echoPin = 14;  // D5 (GPIO 14)

// Buzzer pin (Updated)
const int buzzerPin = 0; // D3 (GPIO 0)

// Servo pin (Updated)
const int servoPin = 2;  // D4 (GPIO 2)

void setup() {
  Serial.begin(115200);

  // Set up sensor input on pin 12 and output on pin 2 (for status indicator)
  pinMode(12, INPUT);  // Sensor pin 12 (D6)
  pinMode(2, OUTPUT);   // Indicator pin

  pinMode(trigPin, OUTPUT);  // Trig pin (D6)
  pinMode(echoPin, INPUT);   // Echo pin (D5)
  pinMode(buzzerPin, OUTPUT);  // Buzzer pin (D3)

  lcd.begin();
  lcd.backlight();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.print(WiFi.localIP());
  delay(5000);

  // Attach servo to pin 2 (D4) and set initial OFF position (0°)
  servoMotor.attach(servoPin);
  servoMotor.write(0);

  // --- Web Server Routes ---
  server.on("/", HTTP_GET, []() {
    String page = "<html><head>";
    page += "<title>pet monitoring</title>";
    page += "<style>";
    page += "body { background-image: url('https://th.bing.com/th/id/OIP.tP-mimVxyKa4XLVel1C8OQHaHF?w=201&h=192&c=7&r=0&o=5&dpr=1.1&pid=1.7');";
    page += "background-size: cover; background-position: center; background-repeat: no-repeat;";
    page += "text-align: center; color: white; font-family: Arial, sans-serif; }";
    page += "h1 { font-size: 60px; margin-top: 50px; text-shadow: 3px 3px 7px white; color: black; font-weight: bold; }";
    page += "h2 { font-size: 50px; font-weight: bold; text-shadow: 3px 3px 7px black; }";
    page += ".status { padding: 30px; display: inline-block; border-radius: 10px; font-size: 50px; font-weight: bold; }";
    page += ".active { background: rgba(0, 255, 0, 0.6); }";
    page += ".alert { background: rgba(255, 0, 0, 0.6); }";
    page += "button { font-size: 30px; padding: 15px 30px; margin: 20px; border-radius: 10px; cursor: pointer; }";
    page += ".on { background: green; color: white; }";
    page += ".off { background: red; color: white; }";
    page += "</style>";
    
    page += "<script>";
    page += "function updateStatus() {";
    page += "  var xhttp = new XMLHttpRequest();";
    page += "  xhttp.onreadystatechange = function() {";
    page += "    if (this.readyState == 4 && this.status == 200) {";
    page += "      document.getElementById('status').innerHTML = this.responseText;";
    page += "    }";
    page += "  };";
    page += "  xhttp.open('GET', '/status', true);";
    page += "  xhttp.send();";
    page += "}";

    page += "function toggleServo() {";
    page += "  var xhttp = new XMLHttpRequest();";
    page += "  xhttp.open('GET', '/toggleServo', true);";
    page += "  xhttp.send();";
    page += "}";

    page += "setInterval(updateStatus, 1000);"; // Update status every second
    page += "</script>";
    page += "</head><body>";
    page += "<h1>pet monitoring</h1>";
    page += "<h2 id='status' class='status active'>Checking...</h2>";
    page += "<button class='on' onclick='toggleServo()'>Servo Control</button>";
    page += "</body></html>";
    
    server.send(200, "text/html", page);
  });

  // AJAX route for status update based on sensor input (pin 12)
  server.on("/status", HTTP_GET, []() {
    String response = "<h2 class='status active'>SAFE</h2>";
    if (intruderDetected) {
      response = "<h2 class='status alert'>GO OUTSIDE</h2>";
    }
    server.send(200, "text/html", response);
  });

  // Route to toggle the servo motor on/off using Servo library commands only
  server.on("/toggleServo", HTTP_GET, []() {
    servoState = !servoState;
    if (servoState) {
      servoMotor.write(180);  // Move servo to ON position
    } else {
      servoMotor.write(0);  // Move servo to OFF position
    }
    server.send(200, "text/plain", "OK");
  });

  server.begin();
}

void loop() {
  server.handleClient();

  // Measure distance with ultrasonic sensor
  long duration, distance;
  
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  distance = (duration / 2) / 29.1; // Calculate distance in cm
  
  // If distance is less than 20 cm, consider it as an intruder detected
  if (distance < 20) {
    digitalWrite(2, HIGH);  // Turn on status indicator
    Serial.println("GO OUTSIDE");
    lcd.clear();
    lcd.print("GO OUTSIDE");
    intruderDetected = true;
    digitalWrite(buzzerPin, HIGH);  // Activate buzzer
  } else {
    digitalWrite(2, LOW);  // Turn off status indicator
    Serial.println("SAFE");
    lcd.clear();
    lcd.print("SAFE");
    intruderDetected = false;
    digitalWrite(buzzerPin, LOW);  // Deactivate buzzer
  }

  delay(1000);
}
