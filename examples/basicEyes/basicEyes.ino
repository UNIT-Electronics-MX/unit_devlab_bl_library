#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DevLabEyes.h>

static const int WIDTH = 128;
static const int HEIGHT = 64;
static const int OLED_RESET = -1;
static const uint8_t OLED_ADDR = 0x3C;

// Auto-detect I2C pins based on chip type.
#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(ARDUINO_ESP32C6_DEV)
  static const int I2C_SDA_PIN = 6;
  static const int I2C_SCL_PIN = 7;
  #define CHIP_NAME "ESP32-C6"
#elif defined(CONFIG_IDF_TARGET_ESP32H2) || defined(ARDUINO_ESP32H2_DEV)
  static const int I2C_SDA_PIN = 12;
  static const int I2C_SCL_PIN = 22;
  #define CHIP_NAME "ESP32-H2"
#else
  static const int I2C_SDA_PIN = 21;
  static const int I2C_SCL_PIN = 22;
  #define CHIP_NAME "ESP32"
#endif

Adafruit_SSD1306 oled(WIDTH, HEIGHT, &Wire, OLED_RESET);
DevLabEyes eyes(oled);

const unsigned long EYES_DRAW_MS = 80;

enum Face {
  FACE_HAPPY,
  FACE_SLEEPY,
  FACE_SURPRISED,
  FACE_WINK,
  FACE_ANGRY,
  FACE_VOMIT
};

void showFace(Face face) {
  switch (face) {
    case FACE_HAPPY:
      eyes.setEmotion(DevLabEyes::HAPPY);
      break;
    case FACE_SLEEPY:
      eyes.setEmotion(DevLabEyes::SLEEPY);
      break;
    case FACE_SURPRISED:
      eyes.setEmotion(DevLabEyes::SURPRISED);
      break;
    case FACE_WINK:
      eyes.setEmotion(DevLabEyes::WINK);
      break;
    case FACE_ANGRY:
      eyes.setEmotion(DevLabEyes::ANGRY);
      break;
    case FACE_VOMIT:
      eyes.setEmotion(DevLabEyes::VOMIT);
      break;
  }

  Serial.print("Emotion: ");
  Serial.println(eyes.getEmotionName());
}

void updateEyes() {
  static unsigned long lastDraw = 0;
  if (millis() - lastDraw >= EYES_DRAW_MS) {
    lastDraw = millis();
    eyes.draw();
  }
}

void setup() {
  Serial.begin(115200);
  delay(600);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Error initializing OLED");
    while (true) {
      delay(1000);
    }
  }

  showFace(FACE_HAPPY);
}

void loop() {
  updateEyes();
}
