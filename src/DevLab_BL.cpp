#include "DevLab_BL.h"

// BLE service UUIDs
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// BLE server callbacks
class DevLabBLServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    (void)pServer;
    DevLabBL._setConnected(true);
    Serial.println("BLE client connected");
  }

  void onDisconnect(BLEServer* pServer) {
    (void)pServer;
    DevLabBL._setConnected(false);
    Serial.println("BLE client disconnected");
    BLEDevice::startAdvertising();
    Serial.println("BLE advertising restarted");
  }
};

// Characteristic callbacks (command reception)
class DevLabBLCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    String value = String(pChar->getValue().c_str());
    if (value.length() > 0) {
      DevLabBL._handleCommand(value);
    }
  }
};

// Constructor
DevLabBLClass::DevLabBLClass() 
  : _bleServer(nullptr)
  , _bleCharacteristic(nullptr)
  , _bleConnected(false)
  , _commandCallback(nullptr)
  , _messageCallback(nullptr) {
}

// Generate unique BLE name based on chip ID
String DevLabBLClass::_generateBLEName() {
  uint64_t chipid = ESP.getEfuseMac();
  uint16_t id = chipid & 0xFFFF;
  char name[20];
  sprintf(name, "KUBI_%04X", id);
  return String(name);
}

// Initialize BLE
bool DevLabBLClass::begin(const String& customName) {
  // Determine BLE name
  if (customName.length() > 0) {
    _bleName = customName;
  } else {
    _bleName = _generateBLEName();
  }

  Serial.print("Initializing BLE: ");
  Serial.println(_bleName);

  // Initialize BLE device
  BLEDevice::init(_bleName.c_str());

  // Create BLE server
  _bleServer = BLEDevice::createServer();
  _bleServer->setCallbacks(new DevLabBLServerCallbacks());

  // Create BLE service
  BLEService* service = _bleServer->createService(SERVICE_UUID);

  // Create BLE characteristic
  _bleCharacteristic = service->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  _bleCharacteristic->addDescriptor(new BLE2902());
  _bleCharacteristic->setCallbacks(new DevLabBLCharacteristicCallbacks());
  _bleCharacteristic->setValue("ready");

  // Start service
  service->start();

  // Configure and start advertising
  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  adv->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE ready");
  Serial.print("Service UUID: ");
  Serial.println(SERVICE_UUID);
  Serial.print("Characteristic UUID: ");
  Serial.println(CHARACTERISTIC_UUID);

  return true;
}

// Set callback for commands
void DevLabBLClass::setCommandCallback(CommandCallback callback) {
  _commandCallback = callback;
}

// Set callback for text messages
void DevLabBLClass::setMessageCallback(MessageCallback callback) {
  _messageCallback = callback;
}

// Send message via BLE
void DevLabBLClass::send(const String& message) {
  if (!_bleConnected || _bleCharacteristic == nullptr) {
    return;
  }
  _bleCharacteristic->setValue(message.c_str());
  _bleCharacteristic->notify();
}

// Send text message with prefix
void DevLabBLClass::sendMessage(const String& message) {
  send("msg:" + message);
}

// Check if connected
bool DevLabBLClass::isConnected() const {
  return _bleConnected;
}

// Get BLE name
String DevLabBLClass::getName() const {
  return _bleName;
}

// Handle received command (called internally by callback)
void DevLabBLClass::_handleCommand(const String& command) {
  // Check if it's a text message (prefixed with "msg:")
  if (command.startsWith("msg:")) {
    String message = command.substring(4);  // Remove "msg:" prefix
    Serial.print("BLE MSG: ");
    Serial.println(message);
    
    if (_messageCallback != nullptr) {
      _messageCallback(message);
    }
  } else {
    // It's a command
    Serial.print("BLE CMD: ");
    Serial.println(command);
    
    if (_commandCallback != nullptr) {
      _commandCallback(command);
    }
  }
}

// Set connection state (called internally by callback)
void DevLabBLClass::_setConnected(bool connected) {
  _bleConnected = connected;
}

// Create global instance
DevLabBLClass DevLabBL;
