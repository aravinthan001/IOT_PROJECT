#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#define DHTPIN 4              // Pin connected to the DHT11 sensor
#define DHTTYPE DHT11         // Define DHT11 sensor

#define SOIL_SENSOR_PIN 34    // Pin connected to the soil moisture sensor
#define RAIN_SENSOR_PIN 33    // Pin connected to the rain sensor
#define RELAY_PIN 2           // Pin connected to the relay

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
String sensorReadings = "";  // For storing multiple readings in CSV format

// Soil moisture threshold for "Wet" and "Dry"
const int soilMoistureThreshold = 3000;

// Array to hold the sensor data for charting
const int maxDataPoints = 20;  // Number of data points to plot on the chart (this is just an example)
float tempData[maxDataPoints] = {0};
float humidityData[maxDataPoints] = {0};
float soilData[maxDataPoints] = {0};
int rainData[maxDataPoints] = {0};
int dataIndex = 0;

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
    .btn-primary {
      background-color: #ff6347; /* Tomato Red */
      border-color: #ff6347;
    }
    .btn-primary:hover {
      background-color: #ff4500; /* Darker red for hover effect */
      border-color: #ff4500;
    }
    .text-center {
      margin-top: 20px;
    }
  </style>
</head>
<body>
  <div class="container py-5">
    <h1 style="color: black;" class="text-center mb-4">Agriculture Monitoring System</h1>
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
      <!-- JavaScript Button for Redirection -->
      <button class="btn btn-primary btn-lg" onclick="window.location.href = '/chart';">View Chart</button>
      <a href="/download" class="btn btn-secondary btn-lg mt-3">Download CSV</a> <!-- Another large button for download -->
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

  // Initialize Soil Moisture Sensor and Relay
  pinMode(SOIL_SENSOR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

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

  // Serve the chart page
  server.on("/chart", HTTP_GET, [](AsyncWebServerRequest *request){
    String chartPage = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Sensor Data Chart</title>
      <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    </head>
    <body>
      <h1>Sensor Data Chart</h1>
      <canvas id="sensorChart" width="400" height="200"></canvas>
      <script>
        fetch('/data')
        .then(response => response.json())
        .then(data => {
          const ctx = document.getElementById('sensorChart').getContext('2d');
          const chart = new Chart(ctx, {
            type: 'line',
            data: {
              labels: ['Temperature', 'Humidity', 'Soil Moisture', 'Rain Sensor'],
              datasets: [{
                label: 'Temperature',
                data: [data.temperature, null, null, null],
                borderColor: 'rgba(75, 192, 192, 1)',
                fill: false
              }, {
                label: 'Humidity',
                data: [null, data.humidity, null, null],
                borderColor: 'rgba(153, 102, 255, 1)',
                fill: false
              }, {
                label: 'Soil Moisture',
                data: [null, null, data.soilMoisture, null],
                borderColor: 'rgba(255, 159, 64, 1)',
                fill: false
              }, {
                label: 'Rain Sensor',
                data: [null, null, null, data.rainValue],
                borderColor: 'rgba(255, 99, 132, 1)',
                fill: false
              }]
            },
            options: {
              responsive: true,
              scales: {
                y: {
                  beginAtZero: true
                }
              }
            }
          });
        });
      </script>
    </body>
    </html>
    )rawliteral";
    request->send(200, "text/html", chartPage);
  });

  // Endpoint to send sensor data in JSON format
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    soilMoistureValue = analogRead(SOIL_SENSOR_PIN);
    rainSensorValue = digitalRead(RAIN_SENSOR_PIN);

    // Control relay based on soil moisture (Dry = Relay ON)
    if (soilMoistureValue < soilMoistureThreshold) {
      digitalWrite(RELAY_PIN, HIGH);  // Turn on relay if soil is dry
    } else {
      digitalWrite(RELAY_PIN, LOW);   // Turn off relay if soil is wet
    }

    // Store data points for charting
    tempData[dataIndex] = temperature;
    humidityData[dataIndex] = humidity;
    soilData[dataIndex] = soilMoistureValue;
    rainData[dataIndex] = rainSensorValue;
    dataIndex = (dataIndex + 1) % maxDataPoints;  // Wrap around if we exceed max data points

    // Send data as JSON
    String jsonResponse = "{\"temperature\":" + String(temperature) + "," 
                          "\"humidity\":" + String(humidity) + "," 
                          "\"soilMoisture\":" + String(soilMoistureValue) + "," 
                          "\"rainSensor\":" + String(rainSensorValue) + "}";

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
