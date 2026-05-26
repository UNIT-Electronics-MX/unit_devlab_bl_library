#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Class to handle BLE functionality in an encapsulated way
class DevLabBLClass {
public:
  // Callback function type for received commands
  typedef void (*CommandCallback)(const String& command);
  // Callback function type for received text messages
  typedef void (*MessageCallback)(const String& message);

  DevLabBLClass();

  // Initialize BLE with a custom name (optional)
  // If no name is provided, generates an automatic one based on chip ID
  bool begin(const String& customName = "");

  // Set callback for when a command is received
  void setCommandCallback(CommandCallback callback);
  
  // Set callback for when a text message is received (prefixed with "msg:")
  void setMessageCallback(MessageCallback callback);

  // Send a message via BLE (notify)
  void send(const String& message);
  
  // Send a text message to be displayed (automatically prefixed with "msg:")
  void sendMessage(const String& message);

  // Check if a client is connected
  bool isConnected() const;

  // Get the current BLE name
  String getName() const;

  // Internal method used by BLE callbacks
  void _handleCommand(const String& command);
  void _setConnected(bool connected);

private:
  BLEServer* _bleServer;
  BLECharacteristic* _bleCharacteristic;
  bool _bleConnected;
  String _bleName;
  CommandCallback _commandCallback;
  MessageCallback _messageCallback;

  String _generateBLEName();
  void _setupBLEServer();
};

// Global instance
extern DevLabBLClass DevLabBL;

