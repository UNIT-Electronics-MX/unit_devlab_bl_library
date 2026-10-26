# DevLab_BL Library

Connect your ESP32 to the DevLab_BL app and control it with a simple interface. This library provides an easy way to set up Bluetooth Low Energy (BLE) communication between your ESP32 and the DevLab_BL mobile app, allowing you to send commands and receive data seamlessly.

## Eyes-only display

`DevLabEyes` can draw Kubi's eyes on an Adafruit SSD1306 display without starting BLE. Its default mode still draws the complete face. Call `setEyesOnly(true)` to hide the mouth, sleep symbols, and other non-eye details while keeping the selected eye expression:

```cpp
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <DevLabEyes.h>

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
DevLabEyes eyes(oled);

void setup() {
  Wire.begin();
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) return;
  eyes.setEyesOnly(true);
  eyes.setEmotion(DevLabEyes::HAPPY);
  eyes.draw();
}

void loop() {}
```

Use `setEyesOnly(false)` to restore complete faces. The codec library's Pomodoro player uses the eyes-only mode; it does not call `DevLabBL.begin()`.

