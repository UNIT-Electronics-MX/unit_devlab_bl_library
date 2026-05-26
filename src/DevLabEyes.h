#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class DevLabEyes {
public:
  enum Emotion {
    HAPPY,
    SLEEPY,
    SURPRISED,
    WINK,
    ANGRY,
    VOMIT
  };

  explicit DevLabEyes(Adafruit_SSD1306& display);

  void setEmotion(Emotion emotion);
  bool setEmotion(const String& emotion);
  Emotion getEmotion() const;
  const char* getEmotionName() const;

  void draw();
  void draw(Emotion emotion);
  bool draw(const String& emotion);

  static bool parseEmotion(const String& emotion, Emotion& outEmotion);
  static const char* emotionName(Emotion emotion);

private:
  Adafruit_SSD1306* _display;
  Emotion _emotion;
  uint16_t _animFrame;
  uint8_t _zOffset;
  uint8_t _vomitOffset;

  void fillCircle(int x0, int y0, int r, uint16_t color);
  void drawArc(int x0, int y0, int r, int startAngle, int endAngle, uint16_t color);

  void drawHappy();
  void drawAngry();
  void drawSleepy();
  void drawSurprised();
  void drawWink();
  void drawVomit();
};
