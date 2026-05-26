/*
 * Basic BLE Test for ESP32-C3/S3
 * Minimal test to verify BLE works
 * Uses DevLab_BL library to simplify code
 */

#include <DevLab_BL.h>

// Callback to handle received commands
void onBLECommand(const String& cmd) {
  Serial.print("Command received: ");
  Serial.println(cmd);
  
  // Respond with command echo
  DevLabBL.send("Echo: " + cmd);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n========================================");
  Serial.println("BASIC BLE TEST - ESP32");
  Serial.println("========================================");
  
  // Print chip information
  Serial.print("Chip: ");
  Serial.println(ESP.getChipModel());
  Serial.print("Revision: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("Cores: ");
  Serial.println(ESP.getChipCores());
  
  Serial.println("\nInitializing BLE...");
  
  // Initialize BLE with DevLab_BL library
  // Can pass custom name or leave empty for auto-generation
  DevLabBL.begin("ESP32_TEST");
  DevLabBL.setCommandCallback(onBLECommand);
  
  Serial.println("\n========================================");
  Serial.println("BLE READY - Broadcasting as: " + DevLabBL.getName());
  Serial.println("========================================\n");
}

void loop() {
  if (DevLabBL.isConnected()) {
    // Send counter every second
    static unsigned long lastTime = 0;
    static int counter = 0;
    
    if (millis() - lastTime > 1000) {
      String msg = "Counter: " + String(counter++);
      DevLabBL.send(msg);
      Serial.println("✓ Sent: " + msg);
      lastTime = millis();
    }
  }
  
  delay(100);
}
