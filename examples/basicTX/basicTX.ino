#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DevLabEyes.h>
#include <DevLab_BL.h>

static const int WIDTH = 128;
static const int HEIGHT = 64;
static const int OLED_RESET = -1;
static const uint8_t OLED_ADDR = 0x3C;

// Auto-detect I2C pins based on chip type
#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(ARDUINO_ESP32C6_DEV)
  static const int I2C_SDA_PIN = 6;
  static const int I2C_SCL_PIN = 7;
  #define CHIP_NAME "ESP32-C6"
#elif defined(CONFIG_IDF_TARGET_ESP32H2) || defined(ARDUINO_ESP32H2_DEV)
  static const int I2C_SDA_PIN = 12;
  static const int I2C_SCL_PIN = 22;
  #define CHIP_NAME "ESP32-H2"
#else
  // Default pins for other ESP32 variants
  static const int I2C_SDA_PIN = 21;
  static const int I2C_SCL_PIN = 22;
  #define CHIP_NAME "ESP32"
#endif

// OLED
Adafruit_SSD1306 oled(WIDTH, HEIGHT, &Wire, OLED_RESET);
DevLabEyes eyes(oled);

String currentEmotion = "happy";
String currentMarch = "stop";
int marchStep = 0;
const unsigned long MARCH_MS = 220;

// Message display
String receivedMessage = "";
bool showingMessage = false;
unsigned long messageStartTime = 0;
unsigned long messageDisplayTime = 5000;  // Dynamic display time
int messageScrollOffset = 0;
unsigned long lastScrollTime = 0;
const unsigned long SCROLL_DELAY = 4000;  // Wait 4s before scrolling
const unsigned long SCROLL_SPEED = 800;   // Scroll every 800ms (slower for reading)


void showEmotion() {
  eyes.draw();
}

void showMessage(const String& message) {
  oled.clearDisplay();
  
  // Draw message frame/header with icon
  oled.drawRect(0, 0, WIDTH, HEIGHT, SSD1306_WHITE);
  oled.fillRect(0, 0, WIDTH, 12, SSD1306_WHITE);
  
  // Draw speaker/message icon
  oled.fillTriangle(3, 3, 3, 9, 8, 6, SSD1306_BLACK);
  oled.drawRect(7, 4, 3, 4, SSD1306_BLACK);
  
  // Header text
  oled.setTextColor(SSD1306_BLACK);
  oled.setTextSize(1);
  oled.setCursor(14, 2);
  oled.print("MENSAJE");
  
  // Draw separator line
  oled.drawLine(0, 12, WIDTH, 12, SSD1306_WHITE);
  
  // Content area
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  
  const int MAX_CHARS_PER_LINE = 19;  // Characters that fit (with padding)
  const int LINE_HEIGHT = 10;          // More spacing between lines
  const int START_Y = 16;              // Start below header and separator
  const int MAX_LINES = 5;             // Maximum lines that fit
  
  // Build array of lines
  String lines[20];  // Support up to 20 lines
  int totalLines = 0;
  String currentWord = "";
  String currentLine = "";
  
  for (unsigned int i = 0; i <= message.length(); i++) {
    char c = (i < message.length()) ? message.charAt(i) : ' ';  // Add space at end to flush last word
    
    // Check for word boundaries and line breaks
    if (c == ' ' || c == '\n' || c == ',' || c == '.' || c == '!' || c == '?' || i == message.length()) {
      // Add punctuation to word
      if ((c == ',' || c == '.' || c == '!' || c == '?') && currentWord.length() > 0) {
        currentWord += c;
      }
      
      // Try to add word to current line
      if (currentWord.length() > 0) {
        String testLine = currentLine;
        if (testLine.length() > 0) testLine += " ";
        testLine += currentWord;
        
        // Check if word fits on current line
        if (testLine.length() <= MAX_CHARS_PER_LINE && c != '\n') {
          currentLine = testLine;
        } else {
          // Save current line
          if (currentLine.length() > 0 && totalLines < 20) {
            lines[totalLines++] = currentLine;
          }
          
          // Start new line with current word
          currentLine = currentWord;
          if (c == '\n') {
            // Force line break
            if (totalLines < 20) {
              lines[totalLines++] = currentLine;
            }
            currentLine = "";
          }
        }
        currentWord = "";
      }
    } else {
      // Build current word
      currentWord += c;
    }
  }
  
  // Save last line
  if (currentLine.length() > 0 && totalLines < 20) {
    lines[totalLines++] = currentLine;
  }
  
  // Display lines with scroll offset
  int startLine = messageScrollOffset;
  int displayedLines = 0;
  
  for (int i = startLine; i < totalLines && displayedLines < MAX_LINES; i++) {
    int y = START_Y + (displayedLines * LINE_HEIGHT);
    oled.setCursor(4, y);  // Left padding
    
    // Add quote mark on first line for emphasis
    if (i == 0 && startLine == 0) {
      oled.print("> ");
      oled.print(lines[i]);
    } else {
      oled.print("  ");
      oled.print(lines[i]);
    }
    displayedLines++;
  }
  
  // Show scroll indicator if there are more lines
  if (totalLines > MAX_LINES) {
    // Down arrow indicator
    if (startLine + MAX_LINES < totalLines) {
      oled.fillTriangle(WIDTH - 7, HEIGHT - 8, WIDTH - 4, HEIGHT - 4, WIDTH - 10, HEIGHT - 4, SSD1306_WHITE);
    }
    // Up arrow indicator  
    if (startLine > 0) {
      oled.fillTriangle(WIDTH - 7, 14, WIDTH - 4, 18, WIDTH - 10, 18, SSD1306_WHITE);
    }
    // Page indicator in header
    int currentPage = (startLine / MAX_LINES) + 1;
    int totalPages = (totalLines + MAX_LINES - 1) / MAX_LINES;
    oled.setCursor(WIDTH - 28, 2);
    oled.setTextColor(SSD1306_BLACK);
    oled.print(String(currentPage) + "/" + String(totalPages));
  }
  
  oled.display();
}

String applyCommand(const String& cmdIn) {
  String cmd = cmdIn;
  cmd.toLowerCase();

  DevLabEyes::Emotion emotion;
  if (DevLabEyes::parseEmotion(cmd, emotion)) {
    eyes.setEmotion(emotion);
    currentEmotion = DevLabEyes::emotionName(emotion);
    return currentEmotion;
  }
  if (cmd == "standup" || cmd == "stand") {
    // neutralPose();
    return "standup";
  }
  if (cmd == "sit") {
    // sitDown();
    return "sit";
  }
  if (cmd == "sleep") {
    // sleep();
    return "sleep";
  }
  if (cmd == "wave") {
    // wave();
    return "wave";
  }
  if (cmd == "walk") {
    currentMarch = "walk";
    marchStep = 0;
    return "walk";
  }
  if (cmd == "stop") {
    // neutralPose();
    return "stop";
  }

  return "unknown:" + cmd;
}

// Callback to handle BLE commands
void onBLECommand(const String& cmd) {
  String result = applyCommand(cmd);
  DevLabBL.send("ok:" + result);
}

// Callback to handle BLE text messages
void onBLEMessage(const String& msg) {
  receivedMessage = msg;
  showingMessage = true;
  messageStartTime = millis();
  messageScrollOffset = 0;
  lastScrollTime = millis();
  
  // Calculate display time based on message length (2 seconds per 20 chars, min 8s, max 60s)
  messageDisplayTime = max(8000UL, min(60000UL, (msg.length() / 10) * 1000UL + 5000UL));
  
  showMessage(msg);
  
  // Send acknowledgment
  DevLabBL.sendMessage("Message received");
}

void setupRobot() {
// Initialize I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);

  Serial.print("Detected chip: ");
  Serial.println(CHIP_NAME);
  Serial.print("I2C pins - SDA: ");
  Serial.print(I2C_SDA_PIN);
  Serial.print(", SCL: ");
  Serial.println(I2C_SCL_PIN);

  // Initialize OLED
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Error initializing OLED");
    while (true) delay(1000);
  }

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 10);
  oled.print("Chip: ");
  oled.print(CHIP_NAME);
  oled.setCursor(0, 30);
  oled.print("Starting...");
  oled.display();
  delay(1500);
  

}

void setup() {
  Serial.begin(115200);
  delay(600);

  setupRobot();

  // Initialize BLE with DevLab_BL library
  DevLabBL.begin();
  DevLabBL.setCommandCallback(onBLECommand);
  DevLabBL.setMessageCallback(onBLEMessage);

  // Show BLE device name on OLED
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 10);
  oled.print("BLE Device:");
  oled.setCursor(0, 30);
  oled.setTextSize(2);
  oled.print(DevLabBL.getName());
  oled.display();
  delay(3000);  // Show for 3 seconds
  
  // Clear and show ready message
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 20);
  oled.print("Ready!");
  oled.display();

  Serial.println("----------------------------");
  Serial.println("Pulsar C6 BLE Only");
  Serial.println("Connect from docs page using Web Bluetooth");
  Serial.println("Commands: happy sleepy surprised wink angry standup sit sleep wave walk stop");
  Serial.println("Messages: Send 'msg:your text here' to display on OLED");
  Serial.println("----------------------------");
}

void loop() {
  static String serialBuffer = "";
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      serialBuffer.trim();
      if (serialBuffer.length() > 0) {
        String result = applyCommand(serialBuffer);
        Serial.println("SERIAL CMD => " + result);
        DevLabBL.send("serial:" + result);
      }
      serialBuffer = "";
      continue;
    }

    serialBuffer += c;
  }

  // Check if message display time has expired
  if (showingMessage && (millis() - messageStartTime > messageDisplayTime)) {
    showingMessage = false;
    messageScrollOffset = 0;
  }
  
  // Auto-scroll long messages
  if (showingMessage && millis() - lastScrollTime > SCROLL_SPEED) {
    if (millis() - messageStartTime > SCROLL_DELAY) {
      // Calculate total lines for current message (more accurate)
      int totalLines = 0;
      String tempWord = "";
      String tempLine = "";
      for (unsigned int i = 0; i <= receivedMessage.length(); i++) {
        char c = (i < receivedMessage.length()) ? receivedMessage.charAt(i) : ' ';
        if (c == ' ' || c == '\n' || c == ',' || c == '.' || c == '!' || c == '?' || i == receivedMessage.length()) {
          if ((c == ',' || c == '.' || c == '!' || c == '?') && tempWord.length() > 0) tempWord += c;
          if (tempWord.length() > 0) {
            String test = tempLine;
            if (test.length() > 0) test += " ";
            test += tempWord;
            if (test.length() > 19 || c == '\n') {
              if (tempLine.length() > 0) totalLines++;
              tempLine = tempWord;
              if (c == '\n') { totalLines++; tempLine = ""; }
            } else {
              tempLine = test;
            }
            tempWord = "";
          }
        } else {
          tempWord += c;
        }
      }
      if (tempLine.length() > 0) totalLines++;
      
      // Scroll one line at a time
      if (totalLines > 5) {
        messageScrollOffset++;
        if (messageScrollOffset > totalLines - 5) {
          messageScrollOffset = totalLines - 5;  // Stay at end, don't loop
        }
      }
    }
    lastScrollTime = millis();
  }

  // Update display
  static unsigned long lastDraw = 0;
  if (millis() - lastDraw > 80) {
    if (showingMessage) {
      showMessage(receivedMessage);  // Keep showing message
    } else {
      showEmotion();  // Show eyes/emotion
    }
    lastDraw = millis();
  }
}
