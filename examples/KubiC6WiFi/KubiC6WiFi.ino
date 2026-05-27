#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <time.h>
#include "html_page.h"
#include "SparkFun_BMI270_Arduino_Library.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

const char* AP_SSID = "Kubi-Control";
const char* AP_PASS = "12345678";
WebServer server(80);

BMI270 imu;
uint8_t i2cAddress = BMI2_I2C_SEC_ADDR;

String currentEmotion = "happy";
String pendingEmotion  = "happy";
String currentMessage  = "";

int animFrame    = 0;
int lookX        = 0;
int lookY        = 0;
int blinkState   = 0;
int blinkCounter = 0;
int textScrollX  = 128;

unsigned long lastScroll   = 0;
unsigned long emotionTimer = 0;
unsigned long lastHit      = 0;

bool autoMode   = true;
int  hitCounter = 0;

int32_t ax_zero = 0, ay_zero = 0, az_zero = 0;

const int eyeLX = 38, eyeRX = 90, eyeY = 32;
const int eyeW = 15, eyeH = 12, pupilR = 5;

// ---- Parpadeo ----

void actualizarParpadeo() {
  blinkCounter++;
  if      (blinkCounter > 60 && blinkCounter < 63) blinkState = 1;
  else if (blinkCounter < 66)                       blinkState = 2;
  else if (blinkCounter < 69)                       blinkState = 3;
  else                                              blinkState = 0;
  if (blinkCounter > 120) blinkCounter = 0;
}

int calcApertura(int base, bool parpadear) {
  if (!parpadear)        return base;
  if (blinkState == 1)   return base / 2;
  if (blinkState == 2)   return 1;
  if (blinkState == 3)   return base / 2;
  return base;
}

// ---- Ojo ----

void drawEye(int cx, int cy, int aperturaH, int px, int py) {
  if (aperturaH <= 1) {
    u8g2.drawLine(cx - eyeW, cy,     cx + eyeW, cy);
    u8g2.drawLine(cx - eyeW, cy + 1, cx + eyeW, cy + 1);
    return;
  }
  for (int dy = -aperturaH; dy <= aperturaH; dy++) {
    float ratio = 1.0f - (float)(dy * dy) / (float)(aperturaH * aperturaH);
    int hw = (int)(eyeW * sqrt(ratio));
    u8g2.drawLine(cx - hw, cy + dy, cx + hw, cy + dy);
  }
  u8g2.setDrawColor(0);
  int pcx = cx + px, pcy = cy + py;
  for (int dy = -pupilR; dy <= pupilR; dy++)
    for (int dx = -pupilR; dx <= pupilR; dx++)
      if (dx * dx + dy * dy <= pupilR * pupilR)
        u8g2.drawPixel(pcx + dx, pcy + dy);
  u8g2.setDrawColor(1);
  u8g2.drawPixel(pcx - 2, pcy - 2);
}

// ---- Emociones ----

void ojosHappy() {
  int ap = calcApertura(eyeH, true);
  u8g2.clearBuffer();
  drawEye(eyeLX, eyeY, ap, lookX, lookY);
  drawEye(eyeRX, eyeY, ap, lookX, lookY);
  u8g2.sendBuffer();
}

void ojosSad() {
  int ap = calcApertura(eyeH - 2, true);
  u8g2.clearBuffer();
  drawEye(eyeLX, eyeY + 2, ap, 0, 2);
  drawEye(eyeRX, eyeY + 2, ap, 0, 2);
  u8g2.drawLine(20, 18, 45, 10);
  u8g2.drawLine(83, 10, 108, 18);
  u8g2.sendBuffer();
}

void ojosAngry() {
  int ap = calcApertura(eyeH - 3, false);
  u8g2.clearBuffer();
  drawEye(eyeLX, eyeY, ap, lookX, lookY);
  drawEye(eyeRX, eyeY, ap, lookX, lookY);
  u8g2.drawLine(20, 10, 45, 20);
  u8g2.drawLine(83, 20, 108, 10);
  u8g2.sendBuffer();
}

void ojosSleepy() {
  u8g2.clearBuffer();
  drawEye(eyeLX, eyeY, 3, 0, 0);
  drawEye(eyeRX, eyeY, 3, 0, 0);
  u8g2.setFont(u8g2_font_6x10_tr);
  int z = animFrame % 18;
  u8g2.drawStr(100, 40 - z,     "z");
  u8g2.drawStr(107, 34 - z / 2, "Z");
  u8g2.drawStr(114, 28 - z / 3, "Z");
  u8g2.sendBuffer();
}

void ojosSurprised() {
  u8g2.clearBuffer();
  drawEye(eyeLX, eyeY, eyeH + 4, 0, 0);
  drawEye(eyeRX, eyeY, eyeH + 4, 0, 0);
  u8g2.sendBuffer();
}

void ojosVomit() {
  u8g2.clearBuffer();
  u8g2.drawLine(28, 22, 42, 36); u8g2.drawLine(42, 22, 28, 36);
  u8g2.drawLine(80, 22, 94, 36); u8g2.drawLine(94, 22, 80, 36);
  u8g2.drawBox(56, 46, 16, 8);
  u8g2.setDrawColor(0); u8g2.drawBox(59, 48, 10, 4); u8g2.setDrawColor(1);
  int vo = animFrame % 4;
  for (int y = 0; y < 18; y++) {
    int w = 6 - (y / 4);
    for (int x = -w; x < w; x++) u8g2.drawPixel(64 + x + vo, 54 + y);
  }
  int d = animFrame % 8;
  u8g2.drawBox(58, 62 - d, 2, 2);
  u8g2.drawBox(70, 66 - d * 2, 2, 2);
  u8g2.sendBuffer();
}

void mostrarEmocion() {
  actualizarParpadeo();
  if      (currentEmotion == "angry")     ojosAngry();
  else if (currentEmotion == "sad")       ojosSad();
  else if (currentEmotion == "sleepy")    ojosSleepy();
  else if (currentEmotion == "surprised") ojosSurprised();
  else if (currentEmotion == "vomit")     ojosVomit();
  else                                    ojosHappy();
}

// ---- Reloj ----

void drawClock() {
  u8g2.clearBuffer();
  struct tm t; time_t now = time(nullptr); localtime_r(&now, &t);
  char tb[16], db[32];
  snprintf(tb, sizeof(tb), "%02d:%02d", t.tm_hour, t.tm_min);
  snprintf(db, sizeof(db), "%02d/%02d/%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);

  // Marco exterior
  u8g2.drawFrame(0, 0, 122, 64);
  // Marco interior (área útil real: x=5..117, y=5..57)
  u8g2.drawFrame(3, 3, 116, 58);

  // Esquinas anguladas estilo terminal Pip-Boy
  u8g2.drawLine(0, 7,  7,  0);
  u8g2.drawLine(114, 0, 121, 7);
  u8g2.drawLine(0, 56, 7,  63);
  u8g2.drawLine(114, 63, 121, 56);

  // Etiqueta superior — fuente pequeña dentro del marco [5,5..11]
  u8g2.setFont(u8g2_font_04b_03_tr);
  u8g2.drawStr(6, 11, "Hora actual");

  // Indicador RTC alineado a la derecha
  u8g2.drawStr(101, 11, "RTC");

  // Línea separadora decorativa bajo la cabecera
  u8g2.drawHLine(5, 13, 112);

  // Hora grande — logisoso28, área [5,15]..[117,46]
  u8g2.setFont(u8g2_font_logisoso28_tf);
  u8g2.drawStr((128 - u8g2.getStrWidth(tb)) / 2, 44, tb);

  // Parpadeo de dos puntos: borra la columna cada segundo impar
  if ((t.tm_sec % 2) != 0) {
    u8g2.setDrawColor(0);
    u8g2.drawBox(56, 16, 14, 28);
    u8g2.setDrawColor(1);
  }

  // Línea separadora decorativa sobre la fecha
  u8g2.drawHLine(5, 47, 112);

  // Fecha pequeña centrada — área [5,48..57]
  u8g2.setFont(u8g2_font_04b_03_tr);
  u8g2.drawStr((128 - u8g2.getStrWidth(db)) / 2, 56, db);

  u8g2.sendBuffer();
}

// ---- Mensaje ----

void drawMessage() {
  u8g2.clearBuffer();

  // Marco + etiqueta estilo terminal
  u8g2.drawFrame(0, 0, 128, 64);
  u8g2.drawHLine(1, 12, 126);
  u8g2.setFont(u8g2_font_04b_03_tr);
  u8g2.drawStr(3, 9, "// INCOMING MSG //");

  // Cursor de bloque parpadeante (usa animFrame del loop)
  if ((animFrame / 12) % 2 == 0) u8g2.drawBox(113, 3, 5, 7);

  // Texto del mensaje — fuente 10x13, área útil [2,14]..[126,54]
  // Divide en dos líneas de máx 12 chars para que quepan con 10px de ancho (10*12=120 < 124)
  u8g2.setFont(u8g2_font_10x20_tr);
  int tw = currentMessage.length() * 10;

  if (tw <= 120) {
    // Cabe en una línea — centrar verticalmente en el área
    u8g2.drawStr(3, 38, currentMessage.c_str());
  } else {
    // Scroll horizontal con fuente grande
    u8g2.drawStr(textScrollX, 38, currentMessage.c_str());
    if (millis() - lastScroll > 45) {
      lastScroll = millis();
      if (--textScrollX < -tw) textScrollX = 128;
    }
  }

  // Línea de estado inferior
  u8g2.setFont(u8g2_font_04b_03_tr);
  u8g2.drawStr(3, 61, "> END OF MESSAGE_");

  u8g2.sendBuffer();
}

// ---- BMI270 ----

void initBMI() {
  while (imu.beginI2C(i2cAddress, Wire) != BMI2_OK) {
    Serial.println("BMI270 no encontrado, reintentando...");
    delay(1000);
  }
  Serial.println("BMI READY");
}

bool readMotion6(int16_t &ax, int16_t &ay, int16_t &az,
                 int16_t &gx, int16_t &gy, int16_t &gz) {
  imu.getSensorData();
  ax = (int16_t)(imu.data.accelX * 16384.0f);
  ay = (int16_t)(imu.data.accelY * 16384.0f);
  az = (int16_t)(imu.data.accelZ * 16384.0f);
  gx = (int16_t)(imu.data.gyroX  * 131.0f);
  gy = (int16_t)(imu.data.gyroY  * 131.0f);
  gz = (int16_t)(imu.data.gyroZ  * 131.0f);
  return true;
}

void calibrateBMI() {
  long sx = 0, sy = 0, sz = 0;
  for (int i = 0; i < 50; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    readMotion6(ax, ay, az, gx, gy, gz);
    sx += ax; sy += ay; sz += az;
    delay(20);
  }
  ax_zero = sx / 50; ay_zero = sy / 50; az_zero = sz / 50;
  Serial.println("BMI CALIBRATED");
}

// ---- Detección de emoción ----

unsigned long getEmotionDelay(const String &e) {
  if (e == "surprised" || e == "vomit") return 0;
  if (e == "angry")   return 300;
  if (e == "sad")     return 1000;
  if (e == "sleepy")  return 2000;
  return 1000;
}

String detectEmotion(int16_t ax, int16_t ay, int16_t az,
                     int16_t gx, int16_t gy, int16_t gz) {
  int32_t ax_rel = ax - ax_zero;
  int32_t ay_rel = ay - ay_zero;
  int32_t az_rel = az - az_zero;
  int motion = abs(gx) + abs(gy) + abs(gz);

  String next;
  if      (motion > 55000)                  next = "vomit";
  else if (az < -12000 && motion < 1000)    next = "sleepy";
  else if (motion > 30000)                  next = "angry";
  else if (motion > 20000)                  next = "surprised";
  else                                      next = "happy";

  if (next != pendingEmotion) { pendingEmotion = next; emotionTimer = millis(); }
  if (millis() - emotionTimer > getEmotionDelay(pendingEmotion)) currentEmotion = pendingEmotion;

  return currentEmotion;
}

// ---- Web handlers ----

void handleRoot()      { server.send(200, "text/html", HTML_PAGE); }

void handleEmotion() {
  if (server.hasArg("name")) {
    currentEmotion = server.arg("name");
    Serial.println("Emotion: " + currentEmotion);
    mostrarEmocion();
  }
  server.send(200, "text/plain", "OK");
}

void handleClock() {
  currentEmotion = "clock"; drawClock();
  server.send(200, "text/plain", "CLOCK");
}

void handleMessage() {
  if (server.hasArg("text")) {
    currentMessage = server.arg("text");
    currentEmotion = "message";
    textScrollX = 128;
    Serial.println("MESSAGE: " + currentMessage);
  }
  server.send(200, "text/plain", "MESSAGE OK");
}

void handleSetTime() {
  if (server.hasArg("epoch")) {
    struct timeval tv = { (time_t)server.arg("epoch").toInt(), 0 };
    settimeofday(&tv, NULL);
    Serial.println("TIME UPDATED");
  }
  server.send(200, "text/plain", "TIME OK");
}

void handleAutoMode() {
  if (server.hasArg("state")) {
    autoMode = (server.arg("state") == "on");
    Serial.println(autoMode ? "AUTO MODE ON" : "AUTO MODE OFF");
  }
  server.send(200, "text/plain", autoMode ? "AUTO ON" : "AUTO OFF");
}

// ---- Setup ----

void setup() {
  Serial.begin(115200);
  setenv("TZ", "CST6", 1); tzset();
  delay(1000);

  Wire.begin(6, 7);
  Wire.setClock(400000);
  u8g2.begin();
  u8g2.setContrast(255);
  currentEmotion = "sleepy";
  mostrarEmocion();

  initBMI();
  delay(500);
  calibrateBMI();

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("AP READY — IP: " + WiFi.softAPIP().toString());

  server.on("/",        handleRoot);
  server.on("/emotion", handleEmotion);
  server.on("/clock",   handleClock);
  server.on("/message", handleMessage);
  server.on("/setTime", handleSetTime);
  server.on("/auto",    handleAutoMode);
  server.begin();
  Serial.println("WEB SERVER READY");
}

// ---- Loop ----

void loop() {
  server.handleClient();

  static unsigned long lastFrame = 0;
  if (millis() - lastFrame <= 40) return;
  lastFrame = millis();
  animFrame++;

  if (animFrame % 80 == 0) {
    int r = random(0, 5);
    lookX = (r == 0) ? -3 : (r == 1) ? 3 : 0;
    lookY = (r == 2) ? -2 : (r == 3) ? 2 : 0;
  }

  if (autoMode) {
    int16_t ax, ay, az, gx, gy, gz;
    if (readMotion6(ax, ay, az, gx, gy, gz))
      currentEmotion = detectEmotion(ax, ay, az, gx, gy, gz);
  }

  if      (currentEmotion == "clock")   drawClock();
  else if (currentEmotion == "message") drawMessage();
  else                                  mostrarEmocion();
}
