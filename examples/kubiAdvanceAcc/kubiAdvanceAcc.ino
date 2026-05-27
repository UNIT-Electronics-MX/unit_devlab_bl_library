#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DevLabEyes.h>
#include <DevLab_BL.h>
#include <RTClib.h>
#include <SparkFun_BMI270_Arduino_Library.h>
#include <esp_sleep.h>
#include <math.h>

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

#ifndef D0
  #define D0 0
#endif
#ifndef D1
  #define D1 1
#endif

static const int IMU_SDO_PIN = D1;
static const int IMU_CS_PIN = D0;
static const uint8_t IMU_ADDR_PRIMARY = BMI2_I2C_PRIM_ADDR; // 0x68
static const uint8_t IMU_ADDR_SECONDARY = BMI2_I2C_SEC_ADDR; // 0x69
const unsigned long IMU_READ_MS = 20;
const unsigned long IMU_SLEEP_READ_MS = 250;
const unsigned long DISPLAY_SLEEP_MS = 5UL * 60UL * 1000UL;
const uint64_t LIGHT_SLEEP_US = 250000ULL;
const float IMU_WAKE_DELTA_G = 0.04;
const float IMU_WAKE_GYRO_DPS = 20.0;

// OLED
Adafruit_SSD1306 oled(WIDTH, HEIGHT, &Wire, OLED_RESET);
DevLabEyes eyes(oled);
RTC_DS3231 rtc;
BMI270 imu;

bool rtcAvailable = false;
bool rtcSynced = false;
unsigned long lastRtcSyncRequest = 0;
const unsigned long RTC_SYNC_REQUEST_MS = 5000;

bool imuAvailable = false;
float accelX = 0.0;
float accelY = 0.0;
float accelZ = 0.0;
float gyroX = 0.0;
float gyroY = 0.0;
float gyroZ = 0.0;
unsigned long lastImuRead = 0;
bool imuSampleReady = false;
bool imuMotionDetected = false;
float lastAccelX = 0.0;
float lastAccelY = 0.0;
float lastAccelZ = 0.0;
bool motionAutoEnabled = false;
DevLabEyes::Emotion lastMotionEmotion = DevLabEyes::HAPPY;
unsigned long lastMotionEmotionUpdate = 0;
const unsigned long MOTION_EMOTION_MS = 350;

bool displaySleeping = false;
unsigned long lastDisplayActivity = 0;

String currentEmotion = "happy";
String currentMarch = "stop";
int marchStep = 0;
const unsigned long MARCH_MS = 220;

enum DisplayMode {
  DISPLAY_EYES,
  DISPLAY_MESSAGE,
  DISPLAY_WEATHER,
  DISPLAY_POMODORO,
  DISPLAY_DATETIME,
  DISPLAY_IMU
};

DisplayMode activeDisplay = DISPLAY_EYES;
unsigned long displayStartTime = 0;
unsigned long displayDuration = 0;
String weatherPayload = "";
String pomodoroPayload = "";
String datetimePayload = "";

// Pomodoro timer runs on the ESP32; the web page only sends controls.
bool pomodoroRunning = false;
bool pomodoroSessionActive = false;
bool pomodoroWorkMode = true;
unsigned long pomodoroWorkSeconds = 25UL * 60UL;
unsigned long pomodoroBreakSeconds = 5UL * 60UL;
unsigned long pomodoroRemainingSeconds = 25UL * 60UL;
unsigned long lastPomodoroTick = 0;
unsigned long lastPomodoroNotify = 0;
unsigned long nextPomodoroDisplay = 0;
const unsigned long POMODORO_DISPLAY_MS = 5000;
const unsigned long POMODORO_REMINDER_MS = 30000;
const unsigned long POMODORO_NOTIFY_MS = 1000;

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

void wakeDisplay() {
  if (!displaySleeping) {
    return;
  }

  oled.ssd1306_command(SSD1306_DISPLAYON);
  displaySleeping = false;
  Serial.println("OLED awake");
}

void registerDisplayActivity() {
  lastDisplayActivity = millis();
  wakeDisplay();
}

bool canSleepCurrentDisplay() {
  return activeDisplay == DISPLAY_EYES ||
         activeDisplay == DISPLAY_MESSAGE ||
         activeDisplay == DISPLAY_WEATHER;
}

void sleepDisplayIfInactive() {
  if (displaySleeping || !canSleepCurrentDisplay() || millis() - lastDisplayActivity < DISPLAY_SLEEP_MS) {
    return;
  }

  oled.ssd1306_command(SSD1306_DISPLAYOFF);
  displaySleeping = true;
  Serial.println("OLED sleeping, waiting for IMU movement or command");
}

void handleDisplaySleep() {
  if (imuMotionDetected) {
    imuMotionDetected = false;
    registerDisplayActivity();
  }

  sleepDisplayIfInactive();
}

void enterIdleLowPower() {
  esp_sleep_enable_timer_wakeup(LIGHT_SLEEP_US);
  esp_light_sleep_start();
}

void notifyMotionAutoState() {
  DevLabBL.send(String("motion:auto:") + (motionAutoEnabled ? "on" : "off"));
}

void setMotionAuto(bool enabled) {
  motionAutoEnabled = enabled && imuAvailable;

  if (motionAutoEnabled) {
    showingMessage = false;
    activeDisplay = DISPLAY_EYES;
    lastMotionEmotionUpdate = 0;
    Serial.println("Motion auto mode: on");
  } else {
    Serial.println("Motion auto mode: off");
  }

  notifyMotionAutoState();
}

String twoDigits(int value) {
  if (value < 10) {
    return "0" + String(value);
  }
  return String(value);
}

String dayNameEs(uint8_t dayOfTheWeek) {
  static const char* names[] = {
    "domingo", "lunes", "martes", "miercoles", "jueves", "viernes", "sabado"
  };
  if (dayOfTheWeek > 6) {
    return "";
  }
  return String(names[dayOfTheWeek]);
}

String rtcDateTimePayload() {
  if (!rtcAvailable || !rtcSynced) {
    return "";
  }
  DateTime now = rtc.now();
  return twoDigits(now.hour()) + "|" + twoDigits(now.minute()) + "|" +
         twoDigits(now.day()) + "|" + twoDigits(now.month()) + "|" +
         String(now.year()) + "|" + dayNameEs(now.dayOfTheWeek());
}

bool parseDateTimePayload(const String& data, DateTime& outDateTime) {
  int idx1 = data.indexOf('|');
  int idx2 = data.indexOf('|', idx1 + 1);
  int idx3 = data.indexOf('|', idx2 + 1);
  int idx4 = data.indexOf('|', idx3 + 1);
  int idx5 = data.indexOf('|', idx4 + 1);

  if (idx1 == -1 || idx2 == -1 || idx3 == -1 || idx4 == -1 || idx5 == -1) {
    return false;
  }

  int hours = data.substring(0, idx1).toInt();
  int minutes = data.substring(idx1 + 1, idx2).toInt();
  int day = data.substring(idx2 + 1, idx3).toInt();
  int month = data.substring(idx3 + 1, idx4).toInt();
  int year = data.substring(idx4 + 1, idx5).toInt();

  if (year < 2024 || month < 1 || month > 12 || day < 1 || day > 31 ||
      hours < 0 || hours > 23 || minutes < 0 || minutes > 59) {
    return false;
  }

  outDateTime = DateTime(year, month, day, hours, minutes, 0);
  return true;
}

bool syncRtcFromPayload(const String& data) {
  if (!rtcAvailable) {
    return false;
  }

  DateTime syncedTime;
  if (!parseDateTimePayload(data, syncedTime)) {
    return false;
  }

  rtc.adjust(syncedTime);
  rtcSynced = true;
  Serial.print("RTC synchronized: ");
  Serial.print(syncedTime.year());
  Serial.print("-");
  Serial.print(twoDigits(syncedTime.month()));
  Serial.print("-");
  Serial.print(twoDigits(syncedTime.day()));
  Serial.print(" ");
  Serial.print(twoDigits(syncedTime.hour()));
  Serial.print(":");
  Serial.println(twoDigits(syncedTime.minute()));
  return true;
}

void requestRtcSyncIfNeeded() {
  if (!rtcAvailable || rtcSynced || !DevLabBL.isConnected()) {
    return;
  }

  unsigned long now = millis();
  if (now - lastRtcSyncRequest >= RTC_SYNC_REQUEST_MS) {
    lastRtcSyncRequest = now;
    DevLabBL.send("rtc:sync_needed");
  }
}

void printImuData() {
  if (!imuAvailable) {
    Serial.println("BMI270 not available");
    return;
  }

  Serial.print("Acceleration in g's\t");
  Serial.print("X: ");
  Serial.print(accelX, 3);
  Serial.print("\tY: ");
  Serial.print(accelY, 3);
  Serial.print("\tZ: ");
  Serial.print(accelZ, 3);
  Serial.print("\tRotation in deg/sec\t");
  Serial.print("X: ");
  Serial.print(gyroX, 3);
  Serial.print("\tY: ");
  Serial.print(gyroY, 3);
  Serial.print("\tZ: ");
  Serial.println(gyroZ, 3);
}

void updateImuData() {
  unsigned long readInterval = displaySleeping ? IMU_SLEEP_READ_MS : IMU_READ_MS;
  if (!imuAvailable || millis() - lastImuRead < readInterval) {
    return;
  }

  lastImuRead = millis();
  imu.getSensorData();

  float nextAccelX = imu.data.accelX;
  float nextAccelY = imu.data.accelY;
  float nextAccelZ = imu.data.accelZ;
  float nextGyroX = imu.data.gyroX;
  float nextGyroY = imu.data.gyroY;
  float nextGyroZ = imu.data.gyroZ;

  if (imuSampleReady) {
    float accelDelta = sqrt(((nextAccelX - lastAccelX) * (nextAccelX - lastAccelX)) +
                            ((nextAccelY - lastAccelY) * (nextAccelY - lastAccelY)) +
                            ((nextAccelZ - lastAccelZ) * (nextAccelZ - lastAccelZ)));
    if (accelDelta > IMU_WAKE_DELTA_G ||
        fabs(nextGyroX) > IMU_WAKE_GYRO_DPS ||
        fabs(nextGyroY) > IMU_WAKE_GYRO_DPS ||
        fabs(nextGyroZ) > IMU_WAKE_GYRO_DPS) {
      imuMotionDetected = true;
    }
  } else {
    imuSampleReady = true;
  }

  lastAccelX = nextAccelX;
  lastAccelY = nextAccelY;
  lastAccelZ = nextAccelZ;

  accelX = nextAccelX;
  accelY = nextAccelY;
  accelZ = nextAccelZ;
  gyroX = nextGyroX;
  gyroY = nextGyroY;
  gyroZ = nextGyroZ;
}

DevLabEyes::Emotion emotionFromMotion() {
  float absGyroX = fabs(gyroX);
  float absGyroY = fabs(gyroY);
  float absGyroZ = fabs(gyroZ);
  float accelMagnitude = sqrt((accelX * accelX) + (accelY * accelY) + (accelZ * accelZ));
  float motionDelta = fabs(accelMagnitude - 1.0);

  // Strong shake makes Kubi sick.
  if (motionDelta > 0.95 || absGyroX > 380.0 || absGyroY > 380.0 || absGyroZ > 380.0) {
    return DevLabEyes::VOMIT;
  }

  // Moderate movement/shake makes Kubi surprised.
  if (motionDelta > 0.45 || absGyroX > 200.0 || absGyroY > 200.0 || absGyroZ > 200.0) {
    return DevLabEyes::SURPRISED;
  }

  // In this board position, X ~= +1g means Kubi is upside down/head-down.
  if (accelX > 0.75) {
    return DevLabEyes::VOMIT;
  }
  if (accelX < -0.75) {
    return DevLabEyes::WINK;
  }
  if (accelY > 0.45) {
    return DevLabEyes::WINK;
  }
  if (accelY < -0.45) {
    return DevLabEyes::ANGRY;
  }
  if (accelZ < -0.45) {
    return DevLabEyes::SLEEPY;
  }
  if (accelZ > 0.45) {
    return DevLabEyes::HAPPY;
  }

  return DevLabEyes::HAPPY;
}

void updateMotionAuto() {
  if (!motionAutoEnabled || !imuAvailable || millis() - lastMotionEmotionUpdate < MOTION_EMOTION_MS) {
    return;
  }

  lastMotionEmotionUpdate = millis();
  DevLabEyes::Emotion nextEmotion = emotionFromMotion();
  if (nextEmotion != lastMotionEmotion) {
    lastMotionEmotion = nextEmotion;
    eyes.setEmotion(nextEmotion);
    currentEmotion = DevLabEyes::emotionName(nextEmotion);
    if (activeDisplay != DISPLAY_POMODORO && activeDisplay != DISPLAY_DATETIME) {
      activeDisplay = DISPLAY_EYES;
    }
    Serial.print("Motion emotion: ");
    Serial.println(currentEmotion);
    DevLabBL.send("motion:emotion:" + currentEmotion);
  }
}

void showImu() {
  oled.clearDisplay();

  oled.fillRect(0, 0, WIDTH, 14, SSD1306_WHITE);
  oled.setTextColor(SSD1306_BLACK);
  oled.setTextSize(1);
  oled.setCursor(4, 3);
  oled.print("BMI270");

  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);

  if (!imuAvailable) {
    oled.setCursor(4, 26);
    oled.print("IMU no detectado");
    oled.display();
    return;
  }

  oled.setCursor(4, 18);
  oled.print("ACC g");
  oled.setCursor(4, 28);
  oled.print("X:");
  oled.print(accelX, 2);
  oled.print(" Y:");
  oled.print(accelY, 2);
  oled.setCursor(4, 38);
  oled.print("Z:");
  oled.print(accelZ, 2);

  oled.setCursor(4, 50);
  oled.print("GYR Z:");
  oled.print(gyroZ, 1);
  oled.print(" d/s");

  oled.display();
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

void showWeather(const String& data) {
  // Parse: temp|desc|location|humidity
  int idx1 = data.indexOf('|');
  int idx2 = data.indexOf('|', idx1 + 1);
  int idx3 = data.indexOf('|', idx2 + 1);
  
  if (idx1 == -1 || idx2 == -1 || idx3 == -1) return;
  
  String temp = data.substring(0, idx1);
  String desc = data.substring(idx1 + 1, idx2);
  String location = data.substring(idx2 + 1, idx3);
  String humidity = data.substring(idx3 + 1);
  
  oled.clearDisplay();
  
  // Header with weather icon
  oled.fillRect(0, 0, WIDTH, 14, SSD1306_WHITE);
  oled.setTextColor(SSD1306_BLACK);
  oled.setTextSize(1);
  oled.setCursor(4, 3);
  oled.print("CLIMA");
  
  // Weather icon (sun)
  oled.fillCircle(WIDTH - 10, 7, 4, SSD1306_BLACK);
  
  oled.setTextColor(SSD1306_WHITE);
  
  // Temperature (large)
  oled.setTextSize(3);
  oled.setCursor(4, 18);
  oled.print(temp);
  oled.setTextSize(1);
  oled.setCursor(oled.getCursorX() + 2, 20);
  oled.print("C");
  
  // Description
  oled.setTextSize(1);
  oled.setCursor(4, 42);
  if (desc.length() > 19) {
    oled.print(desc.substring(0, 18) + ".");
  } else {
    oled.print(desc);
  }
  
  // Location and humidity
  oled.setCursor(4, 53);
  oled.print(location.substring(0, 10));
  oled.setCursor(WIDTH - 40, 53);
  oled.print(humidity);
  oled.print("%");
  
  oled.display();
}

String buildPomodoroPayload() {
  unsigned long minutes = pomodoroRemainingSeconds / 60UL;
  unsigned long seconds = pomodoroRemainingSeconds % 60UL;
  String mode = pomodoroWorkMode ? "Trabajo" : "Descanso";
  String status = pomodoroRunning ? "Activo" : "Pausado";
  return String(minutes) + "|" + String(seconds) + "|" + mode + "|" + status;
}

void sendPomodoroStatus() {
  String mode = pomodoroWorkMode ? "work" : "break";
  String status = pomodoroRunning ? "running" : "paused";
  DevLabBL.send("pomodoro:" + String(pomodoroRemainingSeconds) + "|" + mode + "|" + status);
}

void showPomodoro(const String& data) {
  // Parse: minutes|seconds|mode|status
  int idx1 = data.indexOf('|');
  int idx2 = data.indexOf('|', idx1 + 1);
  int idx3 = data.indexOf('|', idx2 + 1);
  
  if (idx1 == -1 || idx2 == -1 || idx3 == -1) return;
  
  String minutes = data.substring(0, idx1);
  String seconds = data.substring(idx1 + 1, idx2);
  String mode = data.substring(idx2 + 1, idx3);
  String status = data.substring(idx3 + 1);
  
  oled.clearDisplay();
  
  // Header
  oled.fillRect(0, 0, WIDTH, 14, SSD1306_WHITE);
  oled.setTextColor(SSD1306_BLACK);
  oled.setTextSize(1);
  oled.setCursor(4, 3);
  oled.print("POMODORO");
  
  // Tomato icon (simple circle)
  oled.fillCircle(WIDTH - 10, 7, 4, SSD1306_BLACK);
  
  oled.setTextColor(SSD1306_WHITE);
  
  // Timer (large)
  oled.setTextSize(3);
  String timeStr = minutes;
  if (minutes.length() == 1) timeStr = "0" + minutes;
  timeStr += ":";
  if (seconds.length() == 1) timeStr += "0";
  timeStr += seconds;
  
  int timeWidth = timeStr.length() * 18; // Approximate width
  int timeX = (WIDTH - timeWidth) / 2;
  oled.setCursor(timeX, 20);
  oled.print(timeStr);
  
  // Mode and status
  oled.setTextSize(1);
  oled.setCursor(4, 46);
  oled.print("Modo: ");
  oled.print(mode);
  
  oled.setCursor(4, 56);
  oled.print(status);
  
  oled.display();
}

void showPomodoroNow() {
  pomodoroPayload = buildPomodoroPayload();
  activeDisplay = DISPLAY_POMODORO;
  displayStartTime = millis();
  displayDuration = POMODORO_DISPLAY_MS;
  showingMessage = false;
  showPomodoro(pomodoroPayload);
  nextPomodoroDisplay = millis() + POMODORO_REMINDER_MS;
}

void showDateTime(const String& data) {
  // Parse: hours|minutes|day|month|year|dayName
  int idx1 = data.indexOf('|');
  int idx2 = data.indexOf('|', idx1 + 1);
  int idx3 = data.indexOf('|', idx2 + 1);
  int idx4 = data.indexOf('|', idx3 + 1);
  int idx5 = data.indexOf('|', idx4 + 1);
  
  if (idx1 == -1 || idx2 == -1 || idx3 == -1 || idx4 == -1 || idx5 == -1) return;
  
  String hours = data.substring(0, idx1);
  String minutes = data.substring(idx1 + 1, idx2);
  String day = data.substring(idx2 + 1, idx3);
  String month = data.substring(idx3 + 1, idx4);
  String year = data.substring(idx4 + 1, idx5);
  String dayName = data.substring(idx5 + 1);
  
  oled.clearDisplay();
  
  // Header
  oled.fillRect(0, 0, WIDTH, 14, SSD1306_WHITE);
  oled.setTextColor(SSD1306_BLACK);
  oled.setTextSize(1);
  oled.setCursor(4, 3);
  oled.print("FECHA Y HORA");
  
  // Clock icon
  oled.drawCircle(WIDTH - 10, 7, 4, SSD1306_BLACK);
  oled.drawLine(WIDTH - 10, 7, WIDTH - 10, 4, SSD1306_BLACK);
  oled.drawLine(WIDTH - 10, 7, WIDTH - 8, 7, SSD1306_BLACK);
  
  oled.setTextColor(SSD1306_WHITE);
  
  // Time (large)
  oled.setTextSize(3);
  String timeStr = hours + ":" + minutes;
  int timeWidth = timeStr.length() * 18;
  int timeX = (WIDTH - timeWidth) / 2;
  oled.setCursor(timeX, 18);
  oled.print(timeStr);
  
  // Date
  oled.setTextSize(1);
  String dateStr = day + "/" + month + "/" + year;
  int dateWidth = dateStr.length() * 6;
  int dateX = (WIDTH - dateWidth) / 2;
  oled.setCursor(dateX, 44);
  oled.print(dateStr);
  
  // Day name (truncate to fit)
  oled.setTextSize(1);
  String dayNameShort = dayName;
  if (dayNameShort.length() > 18) {
    dayNameShort = dayNameShort.substring(0, 17);
  }
  if (dayNameShort.length() > 0) {
    dayNameShort[0] = toupper(dayNameShort[0]); // Capitalize first letter
  }
  int dayWidth = dayNameShort.length() * 6;
  int dayX = (WIDTH - dayWidth) / 2;
  oled.setCursor(dayX, 54);
  oled.print(dayNameShort);
  
  oled.display();
}

void showRtcDateTime() {
  if (!rtcAvailable || !rtcSynced) {
    return;
  }

  DateTime now = rtc.now();

  oled.clearDisplay();

  // Header
  oled.fillRect(0, 0, WIDTH, 14, SSD1306_WHITE);
  oled.setTextColor(SSD1306_BLACK);
  oled.setTextSize(1);
  oled.setCursor(4, 3);
  oled.print("RTC");

  // Clock icon
  oled.drawCircle(WIDTH - 10, 7, 4, SSD1306_BLACK);
  oled.drawLine(WIDTH - 10, 7, WIDTH - 10, 4, SSD1306_BLACK);
  oled.drawLine(WIDTH - 10, 7, WIDTH - 8, 7, SSD1306_BLACK);

  oled.setTextColor(SSD1306_WHITE);

  // Time with seconds so it is visibly running.
  oled.setTextSize(2);
  String timeStr = twoDigits(now.hour()) + ":" + twoDigits(now.minute()) + ":" + twoDigits(now.second());
  int timeWidth = timeStr.length() * 12;
  int timeX = (WIDTH - timeWidth) / 2;
  oled.setCursor(timeX, 20);
  oled.print(timeStr);

  oled.setTextSize(1);
  String dateStr = twoDigits(now.day()) + "/" + twoDigits(now.month()) + "/" + String(now.year());
  int dateWidth = dateStr.length() * 6;
  oled.setCursor((WIDTH - dateWidth) / 2, 44);
  oled.print(dateStr);

  String dayName = dayNameEs(now.dayOfTheWeek());
  if (dayName.length() > 0) {
    dayName[0] = toupper(dayName[0]);
  }
  int dayWidth = dayName.length() * 6;
  oled.setCursor((WIDTH - dayWidth) / 2, 54);
  oled.print(dayName);

  oled.display();
}

bool hasSeparatorCount(const String& data, char separator, int expectedCount) {
  int count = 0;
  for (unsigned int i = 0; i < data.length(); i++) {
    if (data.charAt(i) == separator) {
      count++;
    }
  }
  return count >= expectedCount;
}

unsigned long parsePositiveMinutes(const String& value, unsigned long fallbackMinutes) {
  long minutes = value.toInt();
  if (minutes <= 0) {
    return fallbackMinutes * 60UL;
  }
  return (unsigned long)minutes * 60UL;
}

String handlePomodoroControl(const String& dataIn) {
  String data = dataIn;
  data.trim();
  String lowerData = data;
  lowerData.toLowerCase();

  int idx1 = data.indexOf('|');
  String action = (idx1 == -1) ? data : data.substring(0, idx1);
  String lowerAction = action;
  lowerAction.toLowerCase();

  if (lowerAction == "start") {
    int idx2 = data.indexOf('|', idx1 + 1);
    if (idx1 != -1 && idx2 != -1) {
      String workMinutes = data.substring(idx1 + 1, idx2);
      String breakMinutes = data.substring(idx2 + 1);
      pomodoroWorkSeconds = parsePositiveMinutes(workMinutes, 25);
      pomodoroBreakSeconds = parsePositiveMinutes(breakMinutes, 5);
    }
    if (!pomodoroSessionActive || pomodoroRemainingSeconds == 0) {
      pomodoroWorkMode = true;
      pomodoroRemainingSeconds = pomodoroWorkMode ? pomodoroWorkSeconds : pomodoroBreakSeconds;
    }
    pomodoroRunning = true;
    pomodoroSessionActive = true;
    lastPomodoroTick = millis();
    lastPomodoroNotify = millis();
    showPomodoroNow();
    sendPomodoroStatus();
    return "pomodoro_started";
  }

  if (lowerAction == "pause") {
    pomodoroRunning = false;
    showPomodoroNow();
    sendPomodoroStatus();
    return "pomodoro_paused";
  }

  if (lowerAction == "reset") {
    int idx2 = data.indexOf('|', idx1 + 1);
    if (idx1 != -1 && idx2 != -1) {
      String workMinutes = data.substring(idx1 + 1, idx2);
      String breakMinutes = data.substring(idx2 + 1);
      pomodoroWorkSeconds = parsePositiveMinutes(workMinutes, 25);
      pomodoroBreakSeconds = parsePositiveMinutes(breakMinutes, 5);
    }
    pomodoroRunning = false;
    pomodoroSessionActive = false;
    pomodoroWorkMode = true;
    pomodoroRemainingSeconds = pomodoroWorkSeconds;
    showPomodoroNow();
    sendPomodoroStatus();
    return "pomodoro_reset";
  }

  if (lowerAction == "status") {
    sendPomodoroStatus();
    return "pomodoro_status";
  }

  return "invalid_pomodoro";
}

void updatePomodoroTimer() {
  if (!pomodoroRunning) {
    return;
  }

  unsigned long now = millis();
  while (now - lastPomodoroTick >= 1000UL) {
    lastPomodoroTick += 1000UL;
    if (pomodoroRemainingSeconds > 0) {
      pomodoroRemainingSeconds--;
    }
    if (pomodoroRemainingSeconds == 0) {
      pomodoroWorkMode = !pomodoroWorkMode;
      pomodoroRemainingSeconds = pomodoroWorkMode ? pomodoroWorkSeconds : pomodoroBreakSeconds;
      lastPomodoroTick = now;
      eyes.setEmotion(pomodoroWorkMode ? DevLabEyes::HAPPY : DevLabEyes::SLEEPY);
      showPomodoroNow();
      sendPomodoroStatus();
      break;
    }
  }

  if (now - lastPomodoroNotify >= POMODORO_NOTIFY_MS) {
    lastPomodoroNotify = now;
    sendPomodoroStatus();
  }

  if (activeDisplay == DISPLAY_EYES && now >= nextPomodoroDisplay) {
    showPomodoroNow();
  }
}

String applyCommand(const String& cmdIn) {
  registerDisplayActivity();

  String cmd = cmdIn;
  cmd.trim();
  String lowerCmd = cmd;
  lowerCmd.toLowerCase();
  
  // Check for special commands with data (weather, pomodoro, datetime)
  if (lowerCmd.startsWith("weather:")) {
    String data = cmd.substring(8);
    if (!hasSeparatorCount(data, '|', 3)) {
      return "invalid_weather";
    }
    weatherPayload = data;
    activeDisplay = DISPLAY_WEATHER;
    displayStartTime = millis();
    displayDuration = 8000;
    showingMessage = false;
    showWeather(weatherPayload);
    return "weather_displayed";
  }
  if (lowerCmd.startsWith("pomodoro:")) {
    String data = cmd.substring(9);
    String lowerData = data;
    lowerData.trim();
    lowerData.toLowerCase();
    if (lowerData.startsWith("start") || lowerData.startsWith("pause") ||
        lowerData.startsWith("reset") || lowerData.startsWith("status")) {
      return handlePomodoroControl(data);
    }
    if (!hasSeparatorCount(data, '|', 3)) {
      return "invalid_pomodoro";
    }
    pomodoroPayload = data;
    activeDisplay = DISPLAY_POMODORO;
    displayStartTime = millis();
    displayDuration = 6000;
    showingMessage = false;
    showPomodoro(pomodoroPayload);
    return "pomodoro_displayed";
  }
  if (lowerCmd.startsWith("datetime:")) {
    if (!rtcAvailable) {
      return "rtc_ignored";
    }
    if (rtcSynced) {
      datetimePayload = rtcDateTimePayload();
      activeDisplay = DISPLAY_DATETIME;
      displayStartTime = millis();
      displayDuration = 6000;
      showingMessage = false;
      showRtcDateTime();
      return "rtc_already_synced";
    }
    String data = cmd.substring(9);
    if (!hasSeparatorCount(data, '|', 5)) {
      return "invalid_datetime";
    }
    syncRtcFromPayload(data);
    datetimePayload = data;
    activeDisplay = DISPLAY_DATETIME;
    displayStartTime = millis();
    displayDuration = 6000;
    showingMessage = false;
    showDateTime(datetimePayload);
    return rtcSynced ? "rtc_synced" : "datetime_displayed";
  }
  if (lowerCmd == "rtc:status") {
    if (!rtcAvailable) {
      return "rtc_ignored";
    }
    if (!rtcSynced) {
      DevLabBL.send("rtc:sync_needed");
      return "rtc_sync_needed";
    }
    String payload = rtcDateTimePayload();
    if (payload.length() > 0) {
      datetimePayload = payload;
      DevLabBL.send("rtc:ok");
    }
    return "rtc_ok";
  }
  if (cmd == "accel" || cmd == "imu") {
    updateImuData();
    activeDisplay = DISPLAY_IMU;
    displayStartTime = millis();
    displayDuration = 6000;
    showingMessage = false;
    showImu();
    printImuData();
    return imuAvailable ? "imu_displayed" : "imu_unavailable";
  }
  if (cmd == "motion:auto:on" || cmd == "auto:motion:on") {
    setMotionAuto(true);
    return motionAutoEnabled ? "motion_auto_on" : "imu_unavailable";
  }
  if (cmd == "motion:auto:off" || cmd == "auto:motion:off") {
    setMotionAuto(false);
    return "motion_auto_off";
  }
  if (cmd == "motion:auto:toggle" || cmd == "auto:motion") {
    setMotionAuto(!motionAutoEnabled);
    return motionAutoEnabled ? "motion_auto_on" : "motion_auto_off";
  }
  if (cmd == "motion:auto:status") {
    notifyMotionAutoState();
    return motionAutoEnabled ? "motion_auto_on" : "motion_auto_off";
  }
  
  cmd = lowerCmd;

  DevLabEyes::Emotion emotion;
  if (DevLabEyes::parseEmotion(cmd, emotion)) {
    if (motionAutoEnabled) {
      setMotionAuto(false);
    }
    eyes.setEmotion(emotion);
    currentEmotion = DevLabEyes::emotionName(emotion);
    showingMessage = false;
    activeDisplay = DISPLAY_EYES;
    showEmotion();
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
  registerDisplayActivity();

  receivedMessage = msg;
  showingMessage = true;
  activeDisplay = DISPLAY_MESSAGE;
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

  pinMode(IMU_CS_PIN, OUTPUT);
  pinMode(IMU_SDO_PIN, OUTPUT);
  digitalWrite(IMU_CS_PIN, HIGH);
  digitalWrite(IMU_SDO_PIN, HIGH);

  Serial.println("Initializing BMI270...");
  if (imu.beginI2C(IMU_ADDR_PRIMARY, Wire) == BMI2_OK) {
    imuAvailable = true;
    Serial.println("BMI270 detected at 0x68");
  } else if (imu.beginI2C(IMU_ADDR_SECONDARY, Wire) == BMI2_OK) {
    imuAvailable = true;
    Serial.println("BMI270 detected at 0x69");
  } else {
    imuAvailable = false;
    Serial.println("BMI270 not detected, accelerometer commands ignored");
  }

  rtcAvailable = rtc.begin();
  if (rtcAvailable) {
    rtcSynced = !rtc.lostPower();
    Serial.print("DS3231 RTC: ");
    if (rtcSynced) {
      Serial.print("OK ");
      DateTime now = rtc.now();
      Serial.print(now.year());
      Serial.print("-");
      Serial.print(twoDigits(now.month()));
      Serial.print("-");
      Serial.print(twoDigits(now.day()));
      Serial.print(" ");
      Serial.print(twoDigits(now.hour()));
      Serial.print(":");
      Serial.println(twoDigits(now.minute()));
    } else {
      Serial.println("needs Bluetooth sync");
    }
  } else {
    rtcSynced = false;
    Serial.println("DS3231 RTC not found, RTC commands ignored");
  }

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
  registerDisplayActivity();
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

  updatePomodoroTimer();
  requestRtcSyncIfNeeded();
  updateImuData();
  handleDisplaySleep();
  updateMotionAuto();

  if (displaySleeping) {
    enterIdleLowPower();
    return;
  }

  // Check if message display time has expired
  if (showingMessage && (millis() - messageStartTime > messageDisplayTime)) {
    showingMessage = false;
    messageScrollOffset = 0;
    activeDisplay = DISPLAY_EYES;
  }

  // Check if temporary data screen has expired
  if ((activeDisplay == DISPLAY_WEATHER ||
       (activeDisplay == DISPLAY_DATETIME && (!rtcAvailable || !rtcSynced)) ||
       (activeDisplay == DISPLAY_POMODORO && !pomodoroRunning) ||
       activeDisplay == DISPLAY_IMU) &&
      (millis() - displayStartTime > displayDuration)) {
    activeDisplay = DISPLAY_EYES;
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
    switch (activeDisplay) {
      case DISPLAY_MESSAGE:
        showMessage(receivedMessage);
        break;
      case DISPLAY_WEATHER:
        showWeather(weatherPayload);
        break;
      case DISPLAY_POMODORO:
        pomodoroPayload = buildPomodoroPayload();
        showPomodoro(pomodoroPayload);
        break;
      case DISPLAY_DATETIME:
        if (rtcAvailable && rtcSynced) {
          showRtcDateTime();
        } else {
          showDateTime(datetimePayload);
        }
        break;
      case DISPLAY_IMU:
        showImu();
        break;
      case DISPLAY_EYES:
      default:
        showEmotion();
        break;
    }
    lastDraw = millis();
  }
}
