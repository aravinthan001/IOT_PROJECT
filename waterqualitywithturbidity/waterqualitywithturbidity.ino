#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HardwareSerial.h>

// Define UART for pH sensor (RX and TX)
HardwareSerial mySerial(1); // Use UART1 (You can also use UART0 or UART2 depending on your setup)
const int pHSensorRxPin = 3;  // RX pin for pH sensor (change as needed)
const int pHSensorTxPin = 1;  // TX pin for pH sensor (change as needed)

// DS18B20 setup
#define ONE_WIRE_BUS 4  // GPIO4 for DS18B20
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// LCD setup (I2C address 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Wi-Fi credentials
const char* ssid = "water";    // Replace with your SSID
const char* password = "12345678"; // Replace with your Wi-Fi password

// Initialize web server on port 80
AsyncWebServer server(80);

// Function to read pH, temperature, and turbidity data
String readSensorData() {
  // Generate random pH value between 6.0 and 8.5
  float pH = random(60, 86) / 10.0; // Random value between 6.0 and 8.5
  
  // Get temperature from DS18B20
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  // Generate random turbidity value between 0 and 100
  int turbidity = random(0, 101); // Random value between 0 and 100

  // Categorize turbidity and display it on the LCD
  String turbidityStatus = "Clear";
  if (turbidity < 20) {
    turbidityStatus = "Clear";
    lcd.setCursor(0, 1);
    lcd.print("It's CLEAR");
    Serial.println("It's CLEAR");
  }
  else if (turbidity >= 20 && turbidity < 50) {
    turbidityStatus = "Cloudy";
    lcd.setCursor(0, 1);
    lcd.print("It's CLOUDY");
    Serial.println("It's CLOUDY");
  }
  else if (turbidity >= 50) {
    turbidityStatus = "Dirty";
    lcd.setCursor(0, 1);
    lcd.print("It's DIRTY");
    Serial.println("It's DIRTY");
  }

  // Determine water status based on pH
  String waterStatus = "Neutral";
  if (pH < 6.5) {
    waterStatus = "Acidic";
  } else if (pH > 7.5) {
    waterStatus = "Alkaline";
  }

  // Return JSON data
  String data = "{";
  data += "\"pH\": " + String(pH) + ",";  // pH value
  data += "\"temperature\": " + String(temperature) + ",";  // Temperature value
  data += "\"waterStatus\": \"" + waterStatus + "\",";  // Water status (Acidic, Neutral, Alkaline)
  data += "\"turbidity\": \"" + turbidityStatus + "\"";  // Turbidity status
  data += "}";

  return data;
}

void setup() {
  // Start Serial Monitor
  Serial.begin(115200);

  // Initialize DS18B20 sensor
  sensors.begin();

  // Initialize LCD
  lcd.begin();
  lcd.setBacklight(HIGH);  // Turn on the LCD backlight
  
  // Initialize serial for pH sensor communication
  mySerial.begin(9600, SERIAL_8N1, pHSensorRxPin, pHSensorTxPin); // Set baud rate and RX/TX pins

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Print IP address after Wi-Fi connection
  Serial.println();
  Serial.print("Connected to Wi-Fi. IP Address: ");
  Serial.println(WiFi.localIP());

  // Display IP address on the LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP: ");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  // Serve the index.html page when the user accesses the root
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String htmlContent = "<html>";
    htmlContent += "<head>";
    htmlContent += "<style>";
    htmlContent += "body { background-image: url('https://wallpapers.com/images/hd/crystal-clear-4k-water-quality-mttwtw1p3k4qag1n.jpg'); background-size: cover; color: white; font-family: Arial, sans-serif; text-align: center; padding: 20px; }";
    htmlContent += "h1 { font-size: 36px; }";
    htmlContent += "p { font-size: 24px; }";
    htmlContent += ".status { font-size: 24px; font-weight: bold; margin-top: 20px; }";
    htmlContent += ".acidic { color: red; }";
    htmlContent += ".neutral { color: green; }";
    htmlContent += ".alkaline { color: blue; }";
    htmlContent += "</style>";
    htmlContent += "</head>";
    htmlContent += "<body>";
    htmlContent += "<h1>Water Quality Monitor</h1>";
    htmlContent += "<p>pH: <span id='pH'></span></p>";
    htmlContent += "<p>Temperature: <span id='temperature'></span> °C</p>";
    htmlContent += "<p class='status'>Water Status: <span id='waterStatus'></span></p>";
    htmlContent += "<p class='status'>Turbidity: <span id='turbidity'></span></p>";
    htmlContent += "<script>";
    htmlContent += "setInterval(function(){ fetch('/data').then(response => response.json()).then(data => { document.getElementById('pH').textContent = data.pH; document.getElementById('temperature').textContent = data.temperature; document.getElementById('waterStatus').textContent = data.waterStatus; document.getElementById('waterStatus').className = data.waterStatus.toLowerCase(); document.getElementById('turbidity').textContent = data.turbidity; }); }, 1000);";
    htmlContent += "</script>";
    htmlContent += "</body>";
    htmlContent += "</html>";
    request->send(200, "text/html", htmlContent);
  });

  // Serve data in JSON format at '/data' endpoint
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String sensorData = readSensorData();
    request->send(200, "application/json", sensorData);
  });

  // Start server
  server.begin();
}

void loop() {
  // Nothing to do in the loop, everything is handled by the web server
}
