#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HardwareSerial.h>
#include <ESP32Servo.h>  // Correct ESP32 Servo library

// Pin Definitions for ESP32
#define GSM_RX_PIN 16  // Define the RX pin for GSM (D16)
#define GSM_TX_PIN 17  // Define the TX pin for GSM (D17)

// Ultrasonic Sensor Pins
#define TRIG_PIN 12
#define ECHO_PIN 13

// IR Sensor Pin
#define IR_SENSOR_PIN 14

// Servo Motor Pin (Normal Servo)
#define SERVO_PIN 15

// ISD1820 Play Pin
#define ISD_PLAY_PIN 4  // Pin connected to Play pin of ISD1820

HardwareSerial gsmSerial(1);  // Create HardwareSerial instance for UART1
Servo dustbinServo;  // Servo instance

const String phoneNumber = "+919344189244";  // Replace with the receiver's phone number

// Initialize the LCD display (16x2, I2C address is typically 0x3F, but it could be 0x27)
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Address 0x3F for a common 16x2 I2C LCD

// Ultrasonic sensor threshold (in cm)
const float FULL_BIN_THRESHOLD = 3; // 3 cm threshold for full bin
bool smsSent = false;  // Flag to prevent multiple SMS sending

void setup() {
  Serial.begin(115200);  // Start serial communication for debugging
  gsmSerial.begin(9600, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);  // GSM serial communication at 9600 baud
  
  // Initialize IR sensor pin with pull-up resistor
  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
  
  // Initialize servo
  dustbinServo.attach(SERVO_PIN);
  dustbinServo.write(0);  // Initially close the lid (0° corresponds to fully closed)
  
  // Initialize ISD1820 Play Pin
  pinMode(ISD_PLAY_PIN, OUTPUT);
  digitalWrite(ISD_PLAY_PIN, LOW);  // Ensure the ISD1820 is not playing initially
  
  // Initialize ultrasonic sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Initialize LCD display
  lcd.begin();  // Set the LCD to 16x2
  lcd.backlight();  // Turn on the backlight
  lcd.clear();  // Clear the LCD screen

  // Test communication with GSM module
  delay(1000);
  sendATCommand("AT", 2000);  // Send AT to check if GSM module is responding
  sendATCommand("AT+CMGF=1", 2000);  // Set SMS to text mode
}

void loop() {
  // Read IR sensor status
  bool personDetected = digitalRead(IR_SENSOR_PIN) == LOW;  // Assuming LOW means detection

  // Print IR sensor status to Serial Monitor
  Serial.print("IR Sensor Status: ");
  if (personDetected) {
    Serial.println("Person Detected!");
  } else {
    Serial.println("No Person Detected");
  }

  // Control the servo based on IR sensor status
  if (personDetected) {
    // Open the lid with a larger angle (e.g., 120 degrees or 150 degrees)
    dustbinServo.write(180);  // Open the lid more than 90° for wider opening
    
    // Play voice message when a person is detected
    digitalWrite(ISD_PLAY_PIN, HIGH);  // Trigger the ISD1820 to play the message
    delay(500);  // Keep the Play pin HIGH for a brief period (adjust as necessary)
    digitalWrite(ISD_PLAY_PIN, LOW);   // Stop playing the message
  } else {
    dustbinServo.write(0);  // Close the lid (0° corresponds to fully closed)
  }

  // Read Ultrasonic Sensor and calculate distance
  long totalDuration = 0;
  int numReadings = 5;
  
  // Take multiple readings to average the result
  for (int i = 0; i < numReadings; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH);
    totalDuration += duration;
  }
  
  // Average the duration readings
  long averageDuration = totalDuration / numReadings;
  long distance = (averageDuration / 2) / 29.1;  // Calculate distance in cm

  // Print Ultrasonic Sensor data to Serial Monitor
  Serial.print("Ultrasonic Sensor Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Display Ultrasonic Sensor data on LCD
  lcd.setCursor(0, 0);  // Set cursor to the first line
  lcd.print("Garbage Level:");
  lcd.setCursor(0, 1);  // Set cursor to the second line
  lcd.print("Dist: ");
  lcd.print(distance);
  lcd.print(" cm");

  // Check if the garbage bin is full
  if (distance <= FULL_BIN_THRESHOLD && !smsSent) {
    Serial.println("Garbage Bin Full!");
    sendSMS(phoneNumber, "Garbage bin is full!");  // Send SMS alert
    smsSent = true;  // Set flag to true so that SMS is not sent again
    delay(10000);  // Wait before checking again (to prevent multiple SMS)
  }

  // If garbage bin is not full, reset the SMS flag
  if (distance > FULL_BIN_THRESHOLD) {
    smsSent = false;  // Reset the flag when the bin is not full
  }

  delay(1000);  // Small delay for readability and better performance
}

// Send AT command to GSM module
void sendATCommand(String command, int timeout) {
  gsmSerial.println(command);  // Send the command to the GSM module
  unsigned long startMillis = millis();
  while (millis() - startMillis < timeout) {
    if (gsmSerial.available()) {
      String response = gsmSerial.readString();
      Serial.println("Response: " + response);  // Print the response from the GSM module
      return;
    }
  }
  Serial.println("No response received!");
}

// Function to send an SMS
void sendSMS(String phoneNumber, String message) {
  Serial.println("Sending SMS...");
  
  // Send the SMS command and recipient's phone number
  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(phoneNumber);
  gsmSerial.println("\"");  // Ending the phone number part
  
  delay(1000);  // Wait for the module to be ready
  
  // Send the message
  gsmSerial.println(message);
  
  // Send the CTRL+Z (ASCII code 26) to send the message
  gsmSerial.write(26);  // ASCII code for CTRL+Z (end of message)
  delay(2000);  // Wait for the SMS to be sent
}
