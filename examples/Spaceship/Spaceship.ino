#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SparkFun_BMI270_Arduino_Library.h>

constexpr uint8_t SDA_PIN = 6;
constexpr uint8_t SCL_PIN = 7;

constexpr uint32_t I2C_HZ = 400000;

constexpr uint8_t BMI270_ADDR = 0x69;
constexpr uint8_t OLED_ADDR = 0x3C;

constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;

Adafruit_SSD1306 display( SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1 );

BMI270 imu;

/* =========================================================
   Juego
   ========================================================= */

constexpr uint8_t MAX_METEORS = 6;

struct Meteor {
  float x;
  float y;
  float speed;
  uint8_t size;
  bool active;
};

Meteor meteors[MAX_METEORS];

/* =========================================================
   Nave
   ========================================================= */

float playerX = 64.0f;

constexpr uint8_t PLAYER_Y = 55;
constexpr uint8_t PLAYER_WIDTH = 9;
constexpr uint8_t PLAYER_HEIGHT = 7;

/* =========================================================
   Movimiento
   ========================================================= */

float velocityX = 0.0f;

constexpr float TILT_DEADZONE = 0.10f;
constexpr float TILT_ACCELERATION = 0.60f;
constexpr float MAX_VELOCITY = 3.2f;
constexpr float FRICTION = 0.9f;

/* =========================================================
   Sacudida
   ========================================================= */

constexpr float SHAKE_THRESHOLD = 1.15f;
constexpr uint32_t SHAKE_COOLDOWN = 1500;

uint32_t lastShake = 0;

bool blastActive = false;

uint32_t blastStart = 0;

constexpr uint32_t BLAST_DURATION = 500;

/* =========================================================
   Estado del juego
   ========================================================= */

uint32_t score = 0;

uint8_t lives = 3;

bool gameOver = false;

/* =========================================================
   Temporizadores
   ========================================================= */

uint32_t previousFrame = 0;
uint32_t previousSpawn = 0;

constexpr uint32_t FRAME_TIME = 30;

uint32_t spawnInterval = 900;

/* =========================================================
   Inicializar BMI270
   ========================================================= */

void iniciarBMI270() {

  int8_t resultado = imu.beginI2C( BMI270_ADDR, Wire );

  if ( resultado != BMI2_OK ) {

    Serial.print("ERROR BMI270: ");
    Serial.println( resultado );

    while (true) {
      delay(100);
    }
  }

  Serial.println("BMI270 OK");
}

/* =========================================================
   Inicializar OLED
   ========================================================= */

void iniciarOLED() {

  if (!display.begin( SSD1306_SWITCHCAPVCC, OLED_ADDR )) {

    Serial.println("ERROR SSD1306");

    while (true) {
      delay(100);
    }
  }

  display.clearDisplay();

  display.setTextColor( SSD1306_WHITE );

  display.setTextSize(1);

  display.display();
}

/* =========================================================
   Pantalla inicial
   ========================================================= */

void pantallaInicio() {

  display.clearDisplay();

  display.setTextSize(2);

  display.setCursor(10, 5);
  display.println("Spaceship");

  display.setTextSize(1);

  display.setCursor(50, 23);
  display.println("Kubi");

  display.display();

  delay(2500);
}

/* =========================================================
   Reiniciar
   ========================================================= */

void resetGame() {

  score = 0;

  lives = 3;

  playerX = 64;

  velocityX = 0;

  gameOver = false;

  blastActive = false;

  spawnInterval = 900;

  previousSpawn = millis();

  for ( uint8_t i = 0; i < MAX_METEORS; i++ ) {
    meteors[i].active = false;
  }
}

/* =========================================================
   Crear meteorito
   ========================================================= */

void spawnMeteor() {

  for ( uint8_t i = 0; i < MAX_METEORS; i++ ) {

    if (!meteors[i].active) {

      meteors[i].active = true;

      meteors[i].size = random(2, 5);

      meteors[i].x = random( meteors[i].size, SCREEN_WIDTH - meteors[i].size );

      meteors[i].y = -5;

      meteors[i].speed = random(12, 24) / 10.0f;

      return;
    }
  }
}

/* =========================================================
   Detectar sacudida
   ========================================================= */

bool detectShake( float ax, float ay, float az ) {

  float magnitude = sqrt( ( ax * ax ) + ( ay * ay ) + ( az * az ) );

  float movement = abs( magnitude - 1.0f );

  if ( movement > SHAKE_THRESHOLD && millis() - lastShake > SHAKE_COOLDOWN ) {

    lastShake = millis();

    return true;
  }

  return false;
}

/* =========================================================
   Control de la nave
   ========================================================= */

void updatePlayer(float ay) {

  float tilt = -ay;

  // Zona muerta para evitar movimiento por ruido del acelerómetro.

  if ( abs( tilt ) > TILT_DEADZONE ) {

    velocityX += tilt * TILT_ACCELERATION;

  } else {
    velocityX *= FRICTION;
  }

  velocityX = constrain( velocityX, -MAX_VELOCITY, MAX_VELOCITY );

  playerX += velocityX;

  // Limite izquierdo

  if ( playerX < 5 ) {

    playerX = 5;

    velocityX = 0;
  }

  // Limite derecho

  if ( playerX > 122 ) {

    playerX = 122;

    velocityX = 0;
  }
}

/* =========================================================
   Colision
   ========================================================= */

bool collision( const Meteor &meteor ) {

  float dx = abs( meteor.x - playerX );

  float dy = abs( meteor.y - PLAYER_Y );

  return (

    dx < meteor.size + PLAYER_WIDTH / 2 &&

    dy < meteor.size + PLAYER_HEIGHT / 2

  );
}

/* =========================================================
   Actualizar meteoritos
   ========================================================= */

void updateMeteors() {

  for ( uint8_t i = 0; i < MAX_METEORS; i++ ) {

    if ( !meteors[i].active ) continue;

    meteors[i].y += meteors[i].speed;

    /* Meteorito pasó */

    if ( meteors[i].y > SCREEN_HEIGHT ) {

      meteors[i].active = false;

      score++;

      continue;
    }

    // Colisión

    if ( collision( meteors[i] ) ) {

      meteors[i].active = false;

      if ( lives > 0 )
        lives--;

      velocityX = 0;

      if ( lives == 0 ) {
        gameOver = true;
      }
    }
  }
}

/* =========================================================
   Blast
   ========================================================= */

void activateBlast() {

  blastActive = true;

  blastStart = millis();

  for ( uint8_t i = 0; i < MAX_METEORS; i++ ) {

    if ( !meteors[i].active )
      continue;

    float dx = meteors[i].x - playerX;

    float dy = meteors[i].y - PLAYER_Y;

    float distance = sqrt( ( dx * dx ) + ( dy * dy ) );

    if ( distance < 43 ) {

      meteors[i].active = false;

      score += 5;
    }
  }
}

/* =========================================================
   Dibujar nave
   ========================================================= */

void drawShip() {

  int x = (int)playerX;

  int y = PLAYER_Y;

  display.drawPixel( x, y - 5, SSD1306_WHITE );

  display.drawLine( x, y - 4, x, y + 2, SSD1306_WHITE );

  display.drawLine( x, y - 1, x - 4, y + 3, SSD1306_WHITE );

  display.drawLine( x, y - 1, x + 4, y + 3, SSD1306_WHITE );

  display.drawLine( x - 4, y + 3, x + 4, y + 3, SSD1306_WHITE );

  // Fuego

  if ( ( millis() / 80 ) % 2 ) {

    display.drawLine( x, y + 4, x, y + 7, SSD1306_WHITE );
  }
}

/* =========================================================
   Meteorito
   ========================================================= */

void drawMeteor( const Meteor &meteor ) {

  display.drawCircle( (int)meteor.x, (int)meteor.y, meteor.size, SSD1306_WHITE );

  if ( meteor.size >= 3 ) {

    display.drawPixel( (int)meteor.x - 1, (int)meteor.y - 1, SSD1306_WHITE );

    display.drawPixel( (int)meteor.x + 1, (int)meteor.y + 1, SSD1306_WHITE );
  }
}

/* =========================================================
   Dibujar explosión
   ========================================================= */

void drawBlast() {

  if ( !blastActive )
    return;

  uint32_t elapsed = millis() - blastStart;

  if ( elapsed > BLAST_DURATION ) {

    blastActive = false;

    return;
  }

  float progress = (float)elapsed / BLAST_DURATION;

  uint8_t radius = progress * 42;

  display.drawCircle( (int)playerX, PLAYER_Y, radius, SSD1306_WHITE );

  if (radius > 4) {

    display.drawCircle( (int)playerX, PLAYER_Y, radius - 4, SSD1306_WHITE );
  }
}

/* =========================================================
   HUD
   ========================================================= */

void drawHUD() {

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.print("S:");
  display.print(score);

  display.setCursor(87, 0);

  display.print("HP:");

  for ( uint8_t i = 0; i < lives; i++ ) {
    display.print("*");
  }

  display.drawLine( 0, 9, 127, 9, SSD1306_WHITE );

  // Barra de carga del BLAST

  uint32_t elapsed = millis() - lastShake;

  int charge = constrain( map( elapsed, 0, SHAKE_COOLDOWN, 0, 30 ), 0, 30 );

  display.drawRect( 48, 2, 32, 5, SSD1306_WHITE );

  display.fillRect( 49, 3, charge, 3, SSD1306_WHITE );
}

/* =========================================================
   Render
   ========================================================= */

void drawGame() {

  display.clearDisplay();

  drawHUD();

  for ( uint8_t i = 0; i < MAX_METEORS; i++ ) {

    if ( meteors[i].active ) {
      drawMeteor(meteors[i]);
    }
  }

  drawShip();

  drawBlast();

  display.display();
}

/* =========================================================
   Game Over
   ========================================================= */

void drawGameOver() {

  display.clearDisplay();

  display.setTextSize(2);

  display.setCursor(12, 7);

  display.println("GAME OVER");

  display.setTextSize(1);

  display.setCursor(31, 32);

  display.print("SCORE: ");

  display.println(score);

  display.setCursor(11, 49);

  display.setTextSize(1);

  display.println("AGITA PARA REINICIAR");

  display.display();
}

/* =========================================================
   SETUP
   ========================================================= */

void setup() {

  Serial.begin(115200);

  delay(500);

  Wire.begin( SDA_PIN, SCL_PIN );

  Wire.setClock( I2C_HZ );

  iniciarBMI270();

  iniciarOLED();

  randomSeed(micros());

  pantallaInicio();

  resetGame();
}

/* =========================================================
   LOOP
   ========================================================= */

void loop() {

  if ( imu.getSensorData() != BMI2_OK ) {
    return;
  }

  float ax = imu.data.accelX;

  float ay = imu.data.accelY;

  float az = imu.data.accelZ;

  /* =======================================================
     Sacudida
     ======================================================= */

  bool shake = detectShake( ax, ay, az );

  /* =======================================================
     GAME OVER
     ======================================================= */

  if ( gameOver ) {

    drawGameOver();

    if ( shake ) {

      delay(200);

      resetGame();
    }

    delay(20);

    return;
  }

  /* =======================================================
     BLAST
     ======================================================= */

  if ( shake ) {

    activateBlast();
  }

  updatePlayer( -ay );

  /* =======================================================
     Spawn
     ======================================================= */

  uint32_t now = millis();

  if ( now - previousSpawn >= spawnInterval ) {

    previousSpawn = now;

    spawnMeteor();

    if ( score > 0 && score % 10 == 0 && spawnInterval > 380 ) {

      spawnInterval -= 25;
    }
  }

  /* =======================================================
     Actualización pantalla
     ======================================================= */

  if ( now - previousFrame >= FRAME_TIME ) {

    previousFrame = now;

    updateMeteors();

    drawGame();
  }

  delay(5);
}