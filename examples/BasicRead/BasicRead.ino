/*
 * BasicRead - BLE command reading example
 * Demonstrates how to receive and process commands via BLE
 * using the DevLab_BL library
 */

#include <DevLab_BL.h>

// Callback to process received commands
void onBLECommand(const String& cmd) {
  Serial.print("BLE command received: ");
  Serial.println(cmd);
  
  // Process different commands
  String response;
  
  if (cmd.equalsIgnoreCase("hello")) {
    response = "Hello! BLE working correctly";
  } 
  else if (cmd.equalsIgnoreCase("status")) {
    response = "Status: OK";
  }
  else if (cmd.equalsIgnoreCase("time")) {
    response = "Uptime: " + String(millis() / 1000) + "s";
  }
  else {
    response = "Unknown command: " + cmd;
  }
  
  // Send response
  DevLabBL.send(response);
  Serial.print("  -> Response sent: ");
  Serial.println(response);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  
  Serial.println("\n========================================");
  Serial.println("BasicRead - BLE reading example");
  Serial.println("========================================");

  // Initialize BLE
  if (!DevLabBL.begin()) {
    Serial.println("Failed to initialize BLE!");
    while (1) {
      delay(1000);
    }
  }

  // Set callback for commands
  DevLabBL.setCommandCallback(onBLECommand);

  Serial.println("BLE initialized successfully");
  Serial.println("Device name: " + DevLabBL.getName());
  Serial.println("\nWaiting for BLE connection...");
  Serial.println("Available commands: hello, status, time");
  Serial.println("========================================\n");
}

void loop() {
  // Indicate connection status
  static bool lastConnected = false;
  bool currentlyConnected = DevLabBL.isConnected();
  
  if (currentlyConnected != lastConnected) {
    if (currentlyConnected) {
      Serial.println("\n✓ BLE client connected");
    } else {
      Serial.println("\n✗ BLE client disconnected");
    }
    lastConnected = currentlyConnected;
  }
  
  delay(100);
}
