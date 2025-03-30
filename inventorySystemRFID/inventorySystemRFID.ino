#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

MFRC522DriverPinSimple ss_pin(15);  // SPI slave select pin
MFRC522DriverSPI driver{ss_pin};    // SPI driver for MFRC522
MFRC522 mfrc522{driver};            // MFRC522 instance

// Define RFID tags and their corresponding products.
String rfidTags[] = {
  "db1cb903",  // Biscuits tag
  "233c7b22",  // Cool Drinks tag
  "74de3004"   // Shampoo tag
};

// Define product names, quantities, and expiry dates.
struct Product {
  String name;
  int quantity;
  String expiryDate;
};

Product products[] = {
  {"Biscuits", 10, "2025-12-31"},
  {"Cool Drinks", 15, "2026-06-30"},
  {"Shampoo", 20, "2027-03-15"}
};

void setup() {
  Serial.begin(115200);    // Start serial communication
  while (!Serial);         // Wait for serial monitor to be ready
  mfrc522.PCD_Init();      // Initialize the MFRC522 RFID reader
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial); // Show MFRC522 version info
  Serial.println(F("Scan RFID tag to add to cart"));
}

void loop() {
  // Check if a new card is detected.
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;  // No new card present, exit loop
  }

  // Read the card serial number.
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;  // Error reading the card serial number, exit loop
  }

  // Get the UID as a string.
  String scannedUID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      scannedUID += "0";  // Add leading zeros for single digits
    }
    scannedUID += String(mfrc522.uid.uidByte[i], HEX);
  }
  
  scannedUID.toLowerCase();  // Convert UID to lowercase to match the format

  // Loop through all products and check if the scanned UID matches
  for (int i = 0; i < sizeof(rfidTags) / sizeof(rfidTags[0]); i++) {
    if (scannedUID == rfidTags[i]) {
      // Product matched, display the product info
      Product &product = products[i];  // Get the product reference

      if (product.quantity > 0) {
        // Decrease the quantity of the product
        product.quantity--;

        // Display the product info and balance
        Serial.print("Product: ");
        Serial.println(product.name);
        Serial.print("Quantity: ");
        Serial.println(product.quantity);
        Serial.print("Expiry Date: ");
        Serial.println(product.expiryDate);
        Serial.println("-----------");
      } else {
        // If no stock left
        Serial.println("Sorry, this product is out of stock.");
      }
      break;  // Exit the loop once the product is found
    }
  }

  delay(1000);  // Delay to prevent multiple reads from the same tag in a short period
}
