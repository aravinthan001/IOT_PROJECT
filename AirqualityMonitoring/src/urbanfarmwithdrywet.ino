#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#define DHTPIN 4              // Pin connected to the DHT11 sensor
#define DHTTYPE DHT11         // Define DHT11 sensor

#define SOIL_SENSOR_PIN 34    // Pin connected to the soil moisture sensor (Updated)
#define RAIN_SENSOR_PIN 33    // Pin connected to the rain sensor (Updated)

DHT dht(DHTPIN, DHTTYPE);   // Initialize DHT sensor
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD address (0x27) and size (16x2)

const char* ssid = "farm";  // Your WiFi SSID
const char* password = "12345678";  // Your WiFi password

AsyncWebServer server(80);   // Create an asynchronous web server on port 80

// Sensor Data Variables
float temperature = 0.0;
float humidity = 0.0;
int soilMoistureValue = 0;
int rainSensorValue = 0;

// String to store multiple sensor readings
String sensorReadings = "";

// Soil moisture threshold for "Wet" and "Dry"
const int soilMoistureThreshold = 3000; // Adjust this threshold based on your sensor calibration

String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Urban Farming Sensor Data</title>
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css" rel="stylesheet">
  <style>
    body {
      background: url('https://media.assettype.com/indiawaterportal%2Fimport%2Fsites%2Fdefault%2Ffiles%2Fstyles%2Fimage_1200x675%2Fpublic%2F2023-05%2Fai-generated-7573620.jpg') no-repeat center center fixed;
      background-size: cover;
      color: white;
    }
    .card {
      border: none;
    }
    .card-body h5 {
      color: black;
    }
  </style>
</head>
<body>
  <div class="container py-5">
    <h1 style="color: black;" class="text-center mb-4">Agriculture monitoring system</h1>
    <div class="row g-4">
      <div class="col-md-4">
        <div class="card text-center p-3">
          <div class="card-header">Temperature (°C)</div>
          <div class="card-body">
            <h5 id="temperature">--</h5>
          </div>
        </div>
      </div>
      <div class="col-md-4">
        <div class="card text-center p-3">
          <div class="card-header">Humidity (%)</div>
          <div class="card-body">
            <h5 id="humidity">--</h5>
          </div>
        </div>
      </div>
      <div class="col-md-4">
        <div class="card text-center p-3">
          <div class="card-header">Soil Moisture</div>
          <div class="card-body">
            <h5 id="soilMoisture">--</h5>
          </div>
        </div>
      </div>
      <div class="col-md-4">
        <div class="card text-center p-3">
          <div class="card-header">Rain Sensor</div>
          <div class="card-body">
            <h5 id="rainSensor">--</h5>
            <small id="rainValue">--</small>
          </div>
        </div>
      </div>
    </div>
    <div class="text-center mt-4">
      <a href="/download" class="btn btn-primary">Download CSV</a>
    </div>
  </div>
  <script>
    setInterval(() => {
      fetch('/data')
      .then(response => response.json())
      .then(data => {
        document.getElementById('temperature').innerText = data.temperature + ' °C';
        document.getElementById('humidity').innerText = data.humidity + ' %';
        document.getElementById('soilMoisture').innerText = data.soilMoisture;
        document.getElementById('rainSensor').innerText = data.rainSensor ? 'Wet' : 'Dry';
        document.getElementById('rainValue').innerText = 'Value: ' + data.rainValue;
      });
    }, 2000);
  </script>
</body>
</html>
)rawliteral";

void setup() {
  // Start Serial Monitor for debugging
  Serial.begin(115200);

  // Initialize the LCD
  lcd.begin();
  lcd.backlight();

  // Initialize DHT11 sensor
  dht.begin();

  // Initialize Soil Moisture Sensor
  pinMode(SOIL_SENSOR_PIN, INPUT);

  // Initialize Rain Sensor
  pinMode(RAIN_SENSOR_PIN, INPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Display IP address on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP Address: ");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());

  // Serve the main page with sensor data
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", htmlPage);
  });

  // Endpoint to send sensor data in JSON format
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    soilMoistureValue = analogRead(SOIL_SENSOR_PIN);
    rainSensorValue = digitalRead(RAIN_SENSOR_PIN);

    // Determine Soil Moisture status (Dry/Wet)
    String soilStatus = (soilMoistureValue > soilMoistureThreshold) ? "Wet" : "Dry";

    // Add data to the sensorReadings string (this is a temporary storage for CSV)
    String data = String(temperature) + "," + String(humidity) + "," + 
                  soilStatus + "," + (rainSensorValue == LOW ? "Wet" : "Dry") + "\n";
    sensorReadings += data;  // Append to existing data

    // Send data as JSON
    String jsonResponse = "{\"temperature\":" + String(temperature) + "," 
                          "\"humidity\":" + String(humidity) + "," 
                          "\"soilMoisture\":\"" + soilStatus + "\"," 
                          "\"rainSensor\":" + String(rainSensorValue == LOW ? "true" : "false") + "," 
                          "\"rainValue\":" + String(rainSensorValue) + "}";

    request->send(200, "application/json", jsonResponse);
  });

  // Endpoint to generate and download the CSV file
  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
    // Generate CSV content with previous readings
    String csvContent = "Temperature,Humidity,SoilMoisture,RainSensor\n";
    csvContent += sensorReadings; // Append all stored readings

    // Serve the CSV content as a downloadable file
    request->send(200, "text/csv", csvContent);
  });

  // Start the web server
  server.begin();
}

void loop() {
  // The web server will automatically handle requests and updates
}
