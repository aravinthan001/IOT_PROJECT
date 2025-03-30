#include <Wire.h>
#include <LoRa.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Define the LoRa pins
#define LORA_CS     5    // Chip Select (CS)
#define LORA_RST    15   // Reset (RST)
#define LORA_DIO0   2    // Digital IO (DIO0)

// Pin definitions
#define TRIG_PIN        25   // Ultrasonic sensor trigger pin
#define ECHO_PIN        14   // Ultrasonic sensor echo pin
#define RAIN_SENSOR_PIN 32   // Rain sensor pin
#define TEMP_SENSOR_PIN 13   // DS18B20 temperature sensor pin

// Relay control pins
#define RELAY_PIN_1 27    // Relay 1 control pin (Temperature-based)
#define RELAY_PIN_2 26    // Relay 2 control pin (Oil level-based)

// Current sensor pin
#define CURRENT_SENSOR_PIN 34  // Current sensor pin (GPIO 34)

// Thresholds
#define TEMP_THRESHOLD 38  // Temperature threshold in °C
#define OIL_LEVEL_THRESHOLD 10  // Oil level threshold (in cm)
#define RAIN_THRESHOLD 1  // When rain sensor detects high (raining)

// DS18B20 setup
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

// Ultrasonic sensor function to measure distance
long readUltrasonic() {
  // Send a short pulse to trigger the ultrasonic sensor
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure the duration of the echo pulse
  long duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate the distance in cm (sound travels at 343m/s or 0.0343 cm/µs)
  long distance = duration * 0.0343 / 2;
  
  return distance;  // Return the distance in cm
}

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);
  
  // Initialize LoRa
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Initialization Failed!");
    while (1);
  }

  // Set up relay pins
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);
  
  // Set up rain sensor pin
  pinMode(RAIN_SENSOR_PIN, INPUT);
  
  // Set initial relay states (Assuming active-low relays, relay is ON when HIGH)
  digitalWrite(RELAY_PIN_1, HIGH);  // Initially ON
  digitalWrite(RELAY_PIN_2, HIGH);  // Initially ON
  
  // Initialize temperature sensor
  sensors.begin();

  // Set Trig pin for ultrasonic sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("System Initialized.");
}

void loop() {
  // Read sensor values
  int rainStatus = digitalRead(RAIN_SENSOR_PIN);  // High means raining
  float oilLevel = readUltrasonic();  // Ultrasonic sensor for oil level monitoring
  sensors.requestTemperatures();  // Request temperature readings
  float temperature = sensors.getTempCByIndex(0);  // Get temperature in °C
  int current = analogRead(CURRENT_SENSOR_PIN);  // Current sensor reading

  // Process rain sensor data
  Serial.print("Rain Status: ");
  Serial.println(rainStatus == LOW ? "Raining" : "Not Raining");

  // Process ultrasonic sensor data for transformer oil level
  Serial.print("Oil Level: ");
  Serial.println(oilLevel);

  // Process temperature sensor data
  Serial.print("Temperature: ");
  Serial.println(temperature);

  // Process current sensor data
  Serial.print("Current: ");
  Serial.println(current);

  // Relay Control Logic
  if (temperature > TEMP_THRESHOLD) {
    digitalWrite(RELAY_PIN_1, LOW);  // Turn OFF relay 1 if temperature is too high
  } else {
    digitalWrite(RELAY_PIN_1, HIGH);  // Turn ON relay 1 if temperature is normal
  }

  // Control relay based on oil level
  if (oilLevel > OIL_LEVEL_THRESHOLD) {
    digitalWrite(RELAY_PIN_2, HIGH);  // Turn ON relay 2 if oil level is above threshold
  } else {
    digitalWrite(RELAY_PIN_2, LOW);   // Turn OFF relay 2 if oil level is below threshold
  }

  // Send data via LoRa
  String dataToSend = "Rain:" + String(rainStatus == LOW ? "Yes" : "No") + 
                      ",Temp:" + String(temperature) + 
                      ",OilLevel:" + String(oilLevel) + 
                      ",Current:" + String(current) +
                      ",Relay1:" + (digitalRead(RELAY_PIN_1) == HIGH ? "Off" : "On") + 
                      ",Relay2:" + (digitalRead(RELAY_PIN_2) == HIGH ? "Off" : "On");
  
  LoRa.beginPacket();
  LoRa.print(dataToSend);
  LoRa.endPacket();
  Serial.println("Sent LoRa Data: " + dataToSend);

  delay(2000);  // Send data every 2 seconds
}
