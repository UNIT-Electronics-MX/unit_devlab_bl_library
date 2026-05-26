#include "DevLabEyes.h"

DevLabEyes::DevLabEyes(Adafruit_SSD1306& display)
  : _display(&display),
    _emotion(HAPPY),
    _animFrame(0),
    _zOffset(0),
    _vomitOffset(0) {
}

void DevLabEyes::setEmotion(Emotion emotion) {
  _emotion = emotion;
}

bool DevLabEyes::setEmotion(const String& emotion) {
  Emotion parsed;
  if (!parseEmotion(emotion, parsed)) return false;
  setEmotion(parsed);
  return true;
}

DevLabEyes::Emotion DevLabEyes::getEmotion() const {
  return _emotion;
}

const char* DevLabEyes::getEmotionName() const {
  return emotionName(_emotion);
}

void DevLabEyes::draw() {
  switch (_emotion) {
    case HAPPY:
      drawHappy();
      break;
    case SLEEPY:
      drawSleepy();
      break;
    case SURPRISED:
      drawSurprised();
      break;
    case WINK:
      drawWink();
      break;
    case ANGRY:
      drawAngry();
      break;
    case VOMIT:
      drawVomit();
      break;
  }

  _animFrame++;
}

void DevLabEyes::draw(Emotion emotion) {
  setEmotion(emotion);
  draw();
}

bool DevLabEyes::draw(const String& emotion) {
  if (!setEmotion(emotion)) return false;
  draw();
  return true;
}

bool DevLabEyes::parseEmotion(const String& emotion, Emotion& outEmotion) {
  String value = emotion;
  value.toLowerCase();
  value.trim();

  if (value == "happy") {
    outEmotion = HAPPY;
    return true;
  }
  if (value == "sleepy") {
    outEmotion = SLEEPY;
    return true;
  }
  if (value == "surprised" || value == "wow") {
    outEmotion = SURPRISED;
    return true;
  }
  if (value == "wink") {
    outEmotion = WINK;
    return true;
  }
  if (value == "angry") {
    outEmotion = ANGRY;
    return true;
  }
  if (value == "vomit") {
    outEmotion = VOMIT;
    return true;
  }

  return false;
}

const char* DevLabEyes::emotionName(Emotion emotion) {
  switch (emotion) {
    case HAPPY:
      return "happy";
    case SLEEPY:
      return "sleepy";
    case SURPRISED:
      return "surprised";
    case WINK:
      return "wink";
    case ANGRY:
      return "angry";
    case VOMIT:
      return "vomit";
  }

  return "happy";
}

void DevLabEyes::fillCircle(int x0, int y0, int r, uint16_t color) {
  for (int y = -r; y < r; y++) {
    for (int x = -r; x < r; x++) {
      if ((x * x + y * y) <= (r * r)) {
        _display->drawPixel(x0 + x, y0 + y, color);
      }
    }
  }
}

void DevLabEyes::drawArc(int x0, int y0, int r, int startAngle, int endAngle, uint16_t color) {
  for (int a = startAngle; a < endAngle; a++) {
    float angle = a * DEG_TO_RAD;
    int x = (int)(x0 + r * cosf(angle));
    int y = (int)(y0 + r * sinf(angle));
    _display->drawPixel(x, y, color);
  }
}

void DevLabEyes::drawHappy() {
  _display->clearDisplay();

  fillCircle(40, 32, 15, SSD1306_WHITE);
  fillCircle(88, 32, 15, SSD1306_WHITE);
  fillCircle(40, 32, 5, SSD1306_BLACK);
  fillCircle(88, 32, 5, SSD1306_BLACK);
  drawArc(64, 48, 10, 0, 180, SSD1306_WHITE);

  _display->display();
}

void DevLabEyes::drawAngry() {
  _display->clearDisplay();

  int shake = ((_animFrame / 6) % 3) - 1;

  fillCircle(40 + shake, 32, 12, SSD1306_WHITE);
  fillCircle(88 - shake, 32, 12, SSD1306_WHITE);
  fillCircle(40 + shake, 28, 5, SSD1306_BLACK);
  fillCircle(88 - shake, 28, 5, SSD1306_BLACK);

  _display->drawLine(28, 20, 52, 24, SSD1306_WHITE);
  _display->drawLine(76, 24, 100, 20, SSD1306_WHITE);

  int mouth = 20 + ((_animFrame / 10) % 2);
  _display->drawFastHLine(64 - (mouth / 2), 50, mouth, SSD1306_WHITE);

  _display->display();
}

void DevLabEyes::drawSleepy() {
  _display->clearDisplay();

  _display->drawFastHLine(28, 32, 24, SSD1306_WHITE);
  _display->drawFastHLine(76, 32, 24, SSD1306_WHITE);
  _display->drawFastHLine(54, 50, 20, SSD1306_WHITE);

  _display->setTextSize(1);
  _display->setTextColor(SSD1306_WHITE);
  _display->setCursor(100, 20 - _zOffset);
  _display->print("Z");
  _display->setCursor(108, 10 - (_zOffset / 2));
  _display->print("z");

  _zOffset++;
  if (_zOffset > 30) _zOffset = 0;

  _display->display();
}

void DevLabEyes::drawSurprised() {
  _display->clearDisplay();

  int bounce = _animFrame % 4;
  int pupil = 5 + (_animFrame % 2);

  fillCircle(40, 32 - bounce, 18, SSD1306_WHITE);
  fillCircle(88, 32 - bounce, 18, SSD1306_WHITE);
  fillCircle(40, 32 - bounce, pupil, SSD1306_BLACK);
  fillCircle(88, 32 - bounce, pupil, SSD1306_BLACK);

  int mouthSize = 8 + (_animFrame % 4);
  _display->drawRoundRect(64 - (mouthSize / 2), 48, mouthSize, mouthSize + 4, 3, SSD1306_WHITE);

  _display->display();
}

void DevLabEyes::drawWink() {
  _display->clearDisplay();

  bool closed = (_animFrame % 20) < 10;

  if (closed) {
    _display->drawFastHLine(24, 32, 30, SSD1306_WHITE);
    _display->drawFastHLine(24, 33, 30, SSD1306_WHITE);
  } else {
    fillCircle(40, 32, 15, SSD1306_WHITE);
    fillCircle(40, 32, 5, SSD1306_BLACK);
  }

  fillCircle(88, 32, 15, SSD1306_WHITE);
  fillCircle(88, 32, 5, SSD1306_BLACK);

  drawArc(64, 48, 12, 10, 170, SSD1306_WHITE);

  _display->display();
}

void DevLabEyes::drawVomit() {
  _display->clearDisplay();

  fillCircle(40, 32, 14, SSD1306_WHITE);
  fillCircle(88, 32, 14, SSD1306_WHITE);

  _display->drawLine(34, 26, 46, 38, SSD1306_BLACK);
  _display->drawLine(46, 26, 34, 38, SSD1306_BLACK);
  _display->drawLine(82, 26, 94, 38, SSD1306_BLACK);
  _display->drawLine(94, 26, 82, 38, SSD1306_BLACK);

  _display->fillRect(56, 46, 16, 8, SSD1306_WHITE);
  _display->fillRect(59, 48, 10, 4, SSD1306_BLACK);

  for (int y = 0; y < 18; y++) {
    int width = 6 - (y / 4);
    for (int x = -width; x < width; x++) {
      _display->drawPixel(64 + x + ((_vomitOffset % 2) * 2), 54 + y, SSD1306_WHITE);
    }
  }

  _display->fillRect(58, 62 - _vomitOffset, 2, 2, SSD1306_WHITE);
  _display->fillRect(70, 66 - (_vomitOffset * 2), 2, 2, SSD1306_WHITE);

  _vomitOffset++;
  if (_vomitOffset > 8) _vomitOffset = 0;

  _display->display();
}
