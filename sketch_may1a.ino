#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include <Preferences.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Preferences prefs;

// ---------- PINS ----------
#define POT_PIN 35
#define BUTTON 4
#define BUZZER 14

// ---------- GAME ----------
int gameState = -1; // -1 MAIN MENU
int gameMode = 0;
int menuIndex = 0;

int level = 1;
int score = 0;
int highScore = 0;
int lives = 3;

bool lastBtn = HIGH;

// ---------- PLAYER ----------
float angle = 0;
int px = 64, py = 60;

// ---------- BULLET ----------
bool bullet = false;
float bx, by, bdx, bdy;

// ---------- EXPLOSION ----------
bool explosion = false;
int ex = 0, ey = 0;
int expFrame = 0;

// ---------- TARGETS ----------
int tx[3], ty[3];
bool alive[3];

int moveX = 0;
bool dir = true;

// ---------- SOUND ----------
void shootS() { tone(BUZZER, 1200, 40); }
void hitS()   { tone(BUZZER, 1800, 60); }
void missS()  { tone(BUZZER, 300, 80); }

void winS() {
  tone(BUZZER, 1000, 150); delay(160);
  tone(BUZZER, 1400, 150); delay(160);
  tone(BUZZER, 1800, 250);
}

void gameOverS() {
  tone(BUZZER, 400, 200); delay(220);
  tone(BUZZER, 300, 200); delay(220);
  tone(BUZZER, 200, 300);
}

// ================= MAIN MENU =================
void drawMainMenu() {

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(20,10);
  display.println("MAIN MENU");

  display.setCursor(20,35);
  display.println("> ARCADE GAME");

  display.display();
}

// ================= ARCADE MENU =================
void drawMenu() {

  int p = analogRead(POT_PIN);
  menuIndex = map(p, 0, 4095, 0, 2);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(25,10);
  display.println("ARCADE GAME");

  display.setCursor(20,30);
  display.println(menuIndex==0 ? "> LEVELS" : "  LEVELS");

  display.setCursor(20,45);
  display.println(menuIndex==1 ? "> ENDLESS" : "  ENDLESS");

  display.setCursor(115,0);
  display.print(menuIndex==2 ? "X" : "x");

  display.display();
}

// ================= LEVEL =================
void loadLevel(int l) {

  for (int i = 0; i < 3; i++) alive[i] = true;

  if (l == 1) {
    tx[0]=35; ty[0]=35;
    tx[1]=64; ty[1]=30;
    tx[2]=95; ty[2]=35;
  }

  if (l == 2) {
    tx[0]=20; ty[0]=10;
    tx[1]=64; ty[1]=8;
    tx[2]=110; ty[2]=10;
  }

  if (l == 3) {
    tx[0]=0; ty[0]=15;
    alive[1]=false;
    alive[2]=false;
    moveX = 0;
    dir = true;
  }

  if (l == 4) {
    tx[0]=64; ty[0]=20;
    alive[1]=false;
    alive[2]=false;
    moveX = 64;
    dir = true;
  }

  if (l == 5) {
    tx[0]=20; ty[0]=20;
    tx[1]=64; ty[1]=25;
    tx[2]=100; ty[2]=20;
  }
}

// ================= SHOOT =================
void shoot() {
  bullet = true;
  bx = px;
  by = py;
  bdx = cos(angle) * 3;
  bdy = sin(angle) * 3;
  shootS();
}

// ================= BULLET =================
void updateBullet() {

  if (!bullet) return;

  bx += bdx;
  by += bdy;

  bool hit = false;

  for (int i = 0; i < 3; i++) {
    if (alive[i]) {
      float d = sqrt(pow(bx - tx[i],2) + pow(by - ty[i],2));
      if (d < 5) {

        alive[i] = false;
        score++;
        hitS();
        hit = true;

        explosion = true;
        ex = tx[i];
        ey = ty[i];
        expFrame = 6;

        if (score > highScore) {
          highScore = score;
          prefs.putInt("high", highScore);
        }
      }
    }
  }

  if (bx < 0 || bx > 128 || by < 0 || by > 64) {

    if (!hit) {
      missS();
      if (gameMode == 0) lives--;
      if (gameMode == 1) gameState = 4;
    }

    bullet = false;
  }

  if (hit) bullet = false;
}

// ================= DRAW =================
void drawPlayer() {

  float dx = cos(angle);
  float dy = sin(angle);
  float pxv = -dy;
  float pyv = dx;

  display.fillCircle(px, py, 3, WHITE);
  display.drawCircle(px, py-1, 4, WHITE);

  float gunX = px + dx*2;
  float gunY = py + dy*2;

  display.drawLine(gunX, gunY,
                   gunX + dx*12 + pxv, gunY + dy*12 + pyv, WHITE);

  display.drawLine(gunX, gunY,
                   gunX + dx*12 - pxv, gunY + dy*12 - pyv, WHITE);
}

void drawExplosion() {
  if (!explosion) return;

  display.drawCircle(ex, ey, expFrame, WHITE);
  display.drawCircle(ex, ey, expFrame/2, WHITE);

  expFrame--;
  if (expFrame <= 0) explosion = false;
}

void drawTargets() {
  for (int i = 0; i < 3; i++) {
    if (alive[i])
      display.drawCircle(tx[i], ty[i], 3, WHITE);
  }
}

void drawBullet() {
  if (bullet)
    display.fillCircle((int)bx, (int)by, 2, WHITE);
}

// ================= SETUP =================
void setup() {

  Wire.begin(21,22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(BUTTON, INPUT_PULLUP);

  prefs.begin("game", false);
  highScore = prefs.getInt("high", 0);
}

// ================= LOOP =================
void loop() {

  bool btn = digitalRead(BUTTON);

  // MAIN MENU
  if (gameState == -1) {

    drawMainMenu();

    if (btn == LOW && lastBtn == HIGH)
      gameState = 0;
  }

  // ARCADE MENU
  else if (gameState == 0) {

    drawMenu();

    if (btn == LOW && lastBtn == HIGH) {

      if (menuIndex == 2) {
        gameState = -1;
      } else {
        gameMode = menuIndex;
        score = 0;
        lives = 3;
        level = 1;
        gameState = 1;
      }
    }
  }

  // START
  else if (gameState == 1) {

    display.clearDisplay();
    display.setCursor(40,25);

    if (gameMode == 0) {
      display.print("LEVEL ");
      display.print(level);
    } else {
      display.print("ENDLESS");
    }

    display.display();
    delay(900);

    loadLevel(level);
    gameState = 2;
  }

  // GAME
  else if (gameState == 2) {

    int pot = analogRead(POT_PIN);
    angle = map(pot,0,4095,-314,0)/100.0;

    if (btn == LOW && lastBtn == HIGH && !bullet)
      shoot();

    updateBullet();

    if (gameMode == 0) {

      if (level == 3 && alive[0]) {
        moveX += dir ? 1 : -1;
        if (moveX > 120) dir = false;
        if (moveX < 0) dir = true;
        tx[0] = moveX;
      }

      if (level == 4 && alive[0]) {
        moveX += dir ? 1 : -1;
        if (moveX > 120) dir = false;
        if (moveX < 0) dir = true;

        tx[0] = moveX;
        ty[0] = 20 + sin(millis()*0.007)*9;
      }

      if (level == 5) {
        int baseY[3] = {20, 25, 20};
        static int vx[3] = {1, -1, 1};

        for (int i=0;i<3;i++) {
          if (alive[i]) {
            tx[i] += vx[i];
            if (tx[i] > 120) vx[i] = -1;
            if (tx[i] < 0) vx[i] = 1;
            ty[i] = baseY[i] + sin(millis()*0.007 + i) * 5;
          }
        }
      }
    }

    if (gameMode == 1) {
      for (int i = 0; i < 3; i++) {
        if (!alive[i]) {
          tx[i] = random(10,118);
          ty[i] = random(10,40);
          alive[i] = true;
        }
      }
    }

    display.clearDisplay();

    display.setCursor(0,0);
    display.print("S:");
    display.print(score);

    if (gameMode == 1) {
      display.setCursor(95,0);
      display.print("H:");
      display.print(highScore);
    }

    if (gameMode == 0) {
      display.setCursor(90,0);
      display.print("HP:");
      display.print(lives);
    }

    drawPlayer();
    drawTargets();
    drawBullet();
    drawExplosion();

    display.display();

    if (gameMode == 0) {

      bool done = true;
      for (int i=0;i<3;i++)
        if (alive[i]) done=false;

      if (done) {
        level++;
        if (level > 5) gameState = 3;
        else gameState = 1;
      }

      if (lives <= 0) gameState = 4;
    }
  }

  // WIN
  else if (gameState == 3) {

    display.clearDisplay();
    display.setCursor(35,20);
    display.println("YOU WIN!");

    display.setCursor(10,40);
    display.println("PRESS TO MENU");

    display.display();

    if (btn == LOW && lastBtn == HIGH)
      gameState = 0;
  }

  // GAME OVER
  else if (gameState == 4) {

    display.clearDisplay();
    display.setCursor(30,20);
    display.println("GAME OVER");

    display.setCursor(10,40);
    display.println("PRESS TO MENU");

    display.display();

    if (btn == LOW && lastBtn == HIGH)
      gameState = 0;
  }

  lastBtn = btn;
  delay(20);
}