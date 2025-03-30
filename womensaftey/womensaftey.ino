#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <TinyGPS++.h>

// OLED I2C address, usually 0x3C or 0x3D
#define OLED_ADDRESS 0x3C  // I2C address of the OLED display
#define OLED_RESET    -1
#define SCREEN_WIDTH  128  // OLED display width in pixels
#define SCREEN_HEIGHT 64  // OLED display height in pixels

// Pin for GPS (Use HardwareSerial on ESP8266)
#define GPS_RX_PIN 3  // RX pin
#define GPS_TX_PIN 1  // TX pin (this is optional if you're not using TX from GPS)
#define BUTTON_PIN 12  // Pin D6 corresponds to GPIO12 on the ESP8266

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
TinyGPSPlus gps;  // Initialize GPS object

// Use hardware serial (Serial1) for GPS communication
HardwareSerial& ss = Serial1; // ESP8266 has Serial1 for hardware serial

const char* ssid = "women";  // Replace with your WiFi credentials
const char* password = "12345678";  // Replace with your WiFi password

AsyncWebServer server(80);

// Global variables to hold latitude, longitude, and help status
String lat = "";
String lon = "";
bool helpNeeded = false;

void setup() {
  Serial.begin(115200);  // Start serial monitor for debugging
  ss.begin(9600);        // Start GPS serial connection (on Serial1)

  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Set button pin as input with pull-up resistor

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {  // Check if OLED initialization is successful
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);  // Loop forever if initialization fails
  }

  display.clearDisplay();  // Clear the buffer
  display.setTextSize(1);  // Normal text size
  display.setTextColor(SSD1306_WHITE);  // Set text color to white

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Display WiFi IP address on OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Connected to WiFi");
  display.setCursor(0, 10);
  display.print("IP Address: ");
  display.println(WiFi.localIP());  // Display the IP address
  display.display();
  delay(2000);  // Wait for 2 seconds to let the user read the WiFi status
  
  // Web server to serve the GPS info and handle button press
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Fetch the latest GPS coordinates
    if (gps.location.isUpdated()) {
      lat = String(gps.location.lat(), 6);  // Get latitude (6 decimal places)
      lon = String(gps.location.lng(), 6);  // Get longitude (6 decimal places)
    }

    String webpage = R"rawliteral(
    <!DOCTYPE html>
    <html>
      <head>
        <title>Women Safety</title>
        <style>
          body {
            font-family: Arial, sans-serif;
            color: #333;
            text-align: center;
            margin: 0;
            padding: 0;
            height: 100vh; 
            background-image: url('https://www.tatahitachi.co.in/wp-content/uploads/2019/07/blog03-content-img-03.gif');
            background-size: cover;
            background-position: center;
            background-repeat: no-repeat;
            overflow: hidden;
          }

          .overlay {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background-color: rgba(0, 0, 0, 0.5);  
            z-index: -1;
          }

          h1 {
            color: #fff;
            font-size: 2em;
            margin-top: 20px;
          }

          .coordinates {
            font-size: 1.5em;
            margin-top: 20px;
            padding: 10px;
            background-color: rgba(224, 247, 250, 0.8);  
            border-radius: 5px;
            margin-bottom: 20px;
          }

          .button {
            background-color: #4CAF50;
            color: white;
            padding: 10px 20px;
            margin-top: 20px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            width: 90%;
            max-width: 250px;
          }

          .button:hover {
            background-color: #45a049;
          }

          .error {
            color: red;
            font-size: 18px;
          }

          .container {
            text-align: center;
            padding: 20px;
          }

          @media (max-width: 600px) {
            h1 {
              font-size: 1.5em;
            }

            .coordinates {
              font-size: 1.2em;
            }

            .button {
              font-size: 1.2em;
            }
          }
        </style>
      </head>
      <body>
        <div class="overlay"></div>
        <h1>Women Safety</h1>
        <div class="coordinates">
          <p>Latitude: %LAT%</p>
          <p>Longitude: %LON%</p>
          <p>Help Needed: %HELP_STATUS%</p>  
        </div>
        <div class="container">
          <a href="https://www.google.com/maps?q=%LAT%,%LON%" target="_blank">
            <button class="button">Open Map</button>
          </a>
          <a href="https://www.google.com/maps/dir/?api=1&destination=%LAT%,%LON%" target="_blank">
            <button class="button">Get Directions</button>
          </a>
          <a href="mailto:?subject=Location%20from%20ESP8266&body=Check%20out%20this%20location%20on%20Google%20Maps:%20https://www.google.com/maps?q=%LAT%,%LON%" target="_blank">
            <button class="button">Share Location</button>
          </a>
        </div>
        %ERROR_MSG%
        <script>
          // Function to toggle help status instantly
          function toggleHelp() {
            fetch('/toggleHelp')
              .then(response => response.text())
              .then(data => {
                // Update the coordinates and help status dynamically
                document.querySelector('.coordinates').innerHTML = data;
              });
          }

          // Attach the toggle function to button
          document.getElementById("helpButton").addEventListener("click", toggleHelp);
        </script>
      </body>
    </html>
    )rawliteral";

    // Replace placeholders with actual GPS data and help status
    webpage.replace("%LAT%", lat);
    webpage.replace("%LON%", lon);
    webpage.replace("%HELP_STATUS%", helpNeeded ? "Yes" : "No");

    request->send(200, "text/html", webpage);
  });

  // Web server endpoint to toggle the help status instantly when the button is pressed
  server.on("/toggleHelp", HTTP_GET, [](AsyncWebServerRequest *request) {
    // If button pressed, toggle the help status immediately
    if (digitalRead(BUTTON_PIN) == LOW) {  // Button pressed (LOW due to pull-up)
      helpNeeded = !helpNeeded;  // Toggle the help status
    }

    // Send updated help status and GPS coordinates to the page
    String response = "<p>Latitude: " + lat + "</p><p>Longitude: " + lon + "</p><p>Help Needed: " + (helpNeeded ? "Yes" : "No") + "</p>";
    request->send(200, "text/html", response);  // Send updated help status and coordinates to the page
  });

  server.begin();  // Start the web server
}

void loop() {
  // Read GPS data from the GPS serial port
  while (ss.available() > 0) {
    gps.encode(ss.read());
  }

  // Update OLED with current GPS coordinates and help status
  if (gps.location.isUpdated()) {
    lat = String(gps.location.lat(), 6);  // Get latitude (6 decimal places)
    lon = String(gps.location.lng(), 6);  // Get longitude (6 decimal places)
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(F("Current Location:"));
  display.print("Lat: ");
  display.println(lat);
  display.print("Lon: ");
  display.println(lon);

  // Display help status
  display.setCursor(0, 40);
  display.print("Help Needed: ");
  display.println(helpNeeded ? "Yes" : "No");

  // Display IP address on OLED
  display.setCursor(0, 50);
  display.print("IP: ");
  display.println(WiFi.localIP());

  display.display();  // Refresh the display
  delay(1000);  // Refresh every 1 second
}
