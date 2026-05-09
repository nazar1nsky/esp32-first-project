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
#define LDR_PIN 34
#define TEMP_PIN 33

// ---------- LIGHT SENSOR SETTINGS ----------
int lightThreshold = 2000;   // базовий рівень (калібрується)
int darkMargin = 600;        // "ще темніше"
int lightMargin = 400;       // "ще світліше"

bool lightTriggerDark = true;
bool lightAlarmEnabled = false;

int indicatorSubMenu = 0; // 0 main, 1 light
int indicatorMenuIndex = 0;

// ---------- GAME ----------
int gameState = -1; // -1 MAIN MENU
int gameMode = 0;
int menuIndex = 0;
int mainMenuIndex = 0;
int musicMenuIndex = 0;

int level = 1;
int score = 0;
int highScore = 0;
int lives = 3;

bool lastBtn = HIGH;

// ---------- FLAGS ----------
bool winPlayed = false;
bool gameOverPlayed = false;

// ---------- MUSIC ----------
bool musicPlaying = false;
int musicStep = 0;
unsigned long musicTimer = 0;
int selectedMusic = 0; // 0 = victory, 2 = spider-man

int melody[] = {1000, 1400, 1800, 1400, 1800, 2200};
int duration[] = {150, 150, 200, 150, 200, 300};
int melodySize = 6;

// ---------- SPIDER-MAN THEME ----------
int spiderMelody[] = {
  659, 784, 880, 784, 659, 523, 587,
  659, 784, 880, 988, 880, 784, 659,
  523, 587, 659, 784, 659, 523
};

int spiderDuration[] = {
  120,120,120,120,140,140,160,
  120,120,120,140,120,120,160,
  140,140,140,140,160,200
};

int spiderSize = 20;

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

// ---------- TIME ----------
int timeMenuIndex = 0;
unsigned long clockTimer = 0;

bool showTimerEndScreen = false;

// CLOCK
int hours = 19;
int minutes = 35;
int seconds = 0;

// TIMER
bool timerRunning = false;
bool timerFinished = false;

int timerMinutes = 1;
int timerSeconds = 0;

unsigned long timerMillis = 0;

bool timerScreenDrawn = false;

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

void saveTime() {
  prefs.putInt("h", hours);
  prefs.putInt("m", minutes);
  prefs.putInt("s", seconds);
}

void saveTimer() {
  prefs.putInt("tm", timerMinutes);
  prefs.putInt("ts", timerSeconds);
}

// ================= MAIN MENU =================
void drawMainMenu() {

  int p = analogRead(POT_PIN);

  mainMenuIndex = map(p, 0, 4095, 0, 4);

  if(mainMenuIndex > 3) mainMenuIndex = 3;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(24,0);
  display.println("MAIN MENU");

  display.setCursor(0,14);
  display.println(mainMenuIndex==0 ? "> ARCADE GAME" : "  ARCADE GAME");

  display.setCursor(0,28);
  display.println(mainMenuIndex==1 ? "> MUSIC" : "  MUSIC");

  display.setCursor(0,42);
  display.println(mainMenuIndex==2 ? "> INDICATORS" : "  INDICATORS");

  display.setCursor(0,54);
  display.println(mainMenuIndex==3 ? "> TIME" : "  TIME");

  display.display();
}

// ================= MUSIC MENU =================
void drawMusicMenu() {

  int p = analogRead(POT_PIN);
  musicMenuIndex = map(p, 0, 4095, 0, 2);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // TITLE
  display.setCursor(25, 10);
  display.println("MUSIC");

  // ITEM 1
  display.setCursor(10, 25);
  display.println(musicMenuIndex == 0 ? "> VICTORY TUNE" : "  VICTORY TUNE");

  // ITEM 2
  display.setCursor(10, 40);
  display.println(musicMenuIndex == 1 ? "> SPIDER-MAN" : "  SPIDER-MAN");

  // EXIT ITEM (логічний, але не в списку)
  display.setCursor(115, 0);
  display.println(musicMenuIndex == 2 ? "X" : "x");

  display.display();
}


// ================= MUSIC UPDATE =================
void updateMusic() {

  if (!musicPlaying) return;

  int freq;
  int dur;

  if (selectedMusic == 2) {
    freq = spiderMelody[musicStep];
    dur = spiderDuration[musicStep];
  } else {
    freq = melody[musicStep];
    dur = duration[musicStep];
  }

  if (millis() - musicTimer > (unsigned long)dur) {

    musicTimer = millis();
    tone(BUZZER, freq, dur);

    musicStep++;

    int size = (selectedMusic == 2) ? spiderSize : melodySize;

    if (musicStep >= size) musicStep = 0;
  }
}

float readTemperature() {

  int adc = analogRead(TEMP_PIN);

  if(adc <= 0) return 0;

  float resistance =
    (4095.0 / adc - 1.0) * 10000.0;

  float tempK =
    1.0 / (
      (1.0 / 298.15) +
      (1.0 / 3950.0) *
      log(resistance / 10000.0)
    );

  float tempC = tempK - 273.15;

  return tempC;
}

void drawTimeMenu() {

  int p = analogRead(POT_PIN);

  timeMenuIndex = map(p, 0, 4095, 0, 3);
  if(timeMenuIndex > 3) timeMenuIndex = 3;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(50, 0);
  display.print("TIME");

  display.setCursor(10, 12);
  display.print("C ");
  if(hours < 10) display.print("0");
  display.print(hours);
  display.print(":");
  if(minutes < 10) display.print("0");
  display.print(minutes);
  display.print(":");
  if(seconds < 10) display.print("0");
  display.print(seconds);

  display.setCursor(10, 24);
  display.print("T ");
  if(timerMinutes < 10) display.print("0");
  display.print(timerMinutes);
  display.print(":");
  if(timerSeconds < 10) display.print("0");
  display.print(timerSeconds);

  display.setCursor(0, 40);
  display.println(timeMenuIndex==0 ? "> TIMER +" : "  TIMER +");

  display.setCursor(0, 50);
  display.println(timeMenuIndex==1 ? "> TIMER -" : "  TIMER -");

  display.setCursor(80, 40);
  display.println(timeMenuIndex==2 ? "> START" : "  START");

  display.setCursor(118, 0);
  display.println(timeMenuIndex==3 ? "X" : "x");

  display.display();
}

void drawExit(bool selected){
  display.setCursor(118,0);
  display.print(selected ? "X" : "x");
}

void updateClock() {

  static unsigned long lastMillis = 0;

  unsigned long now = millis();

  // накопичуємо реальний час
  while (now - lastMillis >= 1000) {

    lastMillis += 1000;

    seconds++;

    if (seconds >= 60) {
      seconds = 0;
      minutes++;
    }

    if (minutes >= 60) {
      minutes = 0;
      hours++;
    }

    if (hours >= 24) {
      hours = 0;
    }
  }
}

void updateTimer() {

  if (!timerRunning) return;

  if (millis() - timerMillis >= 1000) {

    timerMillis += 1000;

    if (timerSeconds > 0) {
      timerSeconds--;
    }
    else {
      if (timerMinutes > 0) {
        timerMinutes--;
        timerSeconds = 59;
      }
      else {
        timerRunning = false;
        timerFinished = true;
      }
    }

    saveTimer();
  }
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

  if (l == 1) { tx[0]=35; ty[0]=35; tx[1]=64; ty[1]=30; tx[2]=95; ty[2]=35; }
  if (l == 2) { tx[0]=20; ty[0]=10; tx[1]=64; ty[1]=8; tx[2]=110; ty[2]=10; }

  if (l == 3) {
    tx[0]=0; ty[0]=15;
    alive[1]=false; alive[2]=false;
    moveX=0; dir=true;
  }

  if (l == 4) {
    tx[0]=64; ty[0]=20;
    alive[1]=false; alive[2]=false;
    moveX=64; dir=true;
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

  for (int i=0;i<3;i++) {
    if (alive[i]) {
      float d = sqrt(pow(bx-tx[i],2)+pow(by-ty[i],2));
      if (d < 5) {
        alive[i]=false;
        score++;
        hitS();
        hit=true;

        explosion=true;
        ex=tx[i]; ey=ty[i]; expFrame=6;

        if (score > highScore) {
          highScore = score;
          prefs.putInt("high", highScore);
        }
      }
    }
  }

  if (bx<0||bx>128||by<0||by>64) {
    if (!hit) {
      missS();
      if (gameMode==0) lives--;
      if (gameMode==1) gameState=4;
    }
    bullet=false;
  }

  if (hit) bullet=false;
}

// ================= DRAW =================
void drawPlayer() {

  float dx = cos(angle);
  float dy = sin(angle);
  float pxv = -dy;
  float pyv = dx;

  display.fillCircle(px,py,3,WHITE);
  display.drawCircle(px,py-1,4,WHITE);

  float gx=px+dx*2;
  float gy=py+dy*2;

  display.drawLine(gx,gy,gx+dx*12+pxv,gy+dy*12+pyv,WHITE);
  display.drawLine(gx,gy,gx+dx*12-pxv,gy+dy*12-pyv,WHITE);
}

void drawExplosion() {
  if (!explosion) return;
  display.drawCircle(ex,ey,expFrame,WHITE);
  display.drawCircle(ex,ey,expFrame/2,WHITE);
  expFrame--;
  if (expFrame<=0) explosion=false;
}

void drawTargets() {
  for (int i=0;i<3;i++)
    if (alive[i]) display.drawCircle(tx[i],ty[i],3,WHITE);
}

void drawBullet() {
  if (bullet) display.fillCircle((int)bx,(int)by,2,WHITE);
}


void updateLightAlarm() {

  if(!lightAlarmEnabled || indicatorSubMenu != 1) return;

  int light = analogRead(LDR_PIN);

  bool trigger = false;

  // ================= DARK MODE =================
  if(lightTriggerDark){

    // реагує тільки коли СУТТЄВО темніше
    if(light < (lightThreshold - darkMargin)){
      trigger = true;
    }
  }

  // ================= LIGHT MODE =================
  else{

    // реагує тільки коли СУТТЄВО світліше
    if(light > (lightThreshold + lightMargin)){
      trigger = true;
    }
  }

  if(trigger){
    tone(BUZZER, 1200, 80);
  }
}

void drawIndicatorsMenu() {

  int p = analogRead(POT_PIN);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(18,0);
  display.println("INDICATORS");

  // ===== MAIN =====
  if(indicatorSubMenu == 0){

    indicatorMenuIndex = map(p, 0, 4095, 0, 2);

    if(indicatorMenuIndex > 1) indicatorMenuIndex = 1;

    float t = readTemperature();

    display.setCursor(0,16);
    display.print("Temp: ");
    display.print(t);
    display.println(" C");

    display.setCursor(0,36);
    display.println(indicatorMenuIndex==0 ? "> LIGHT" : "  LIGHT");

    // X справа зверху
    display.setCursor(118,0);
    display.println(indicatorMenuIndex==1 ? "X" : "x");
  }

  // ===== LIGHT MENU =====
  else {

    indicatorMenuIndex = map(p, 0, 4095, 0, 4);

    if(indicatorMenuIndex > 3) indicatorMenuIndex = 3;

    display.setCursor(0,14);

    // кружечки
    display.print(lightTriggerDark ? "(o) " : "( ) ");
    display.println("DARK");

    display.setCursor(0,28);

    display.print(!lightTriggerDark ? "(o) " : "( ) ");
    display.println("LIGHT");

    display.setCursor(0,44);

    display.print(lightAlarmEnabled ? "ALARM: ON" : "ALARM: OFF");

    // X
    display.setCursor(118,0);
    display.println(indicatorMenuIndex==3 ? "X" : "x");

// стрілка справа
if(indicatorMenuIndex == 0){
  display.setCursor(120,14);
  display.print("<");
}

if(indicatorMenuIndex == 1){
  display.setCursor(120,28);
  display.print("<");
}

if(indicatorMenuIndex == 2){
  display.setCursor(120,44);
  display.print("<");
}

  }

  display.display();
}

void setTime(int h, int m, int s){
  hours = h;
  minutes = m;
  seconds = s;

  clockTimer = millis();

  saveTime(); // 🔥 щоб не скидалось після перезапуску
}

  // ================= SETUP =================
void setup() {

  Wire.begin(21,22);
  display.begin(SSD1306_SWITCHCAPVCC,0x3C);

  pinMode(BUTTON, INPUT_PULLUP);

  analogReadResolution(12);
  analogSetPinAttenuation(TEMP_PIN, ADC_11db);

  prefs.begin("game", false);

  highScore = prefs.getInt("high", 0);

  // ================= CLOCK LOAD =================
  hours = prefs.getInt("h", 19);
  minutes = prefs.getInt("m", 35);
  seconds = prefs.getInt("s", 0);

  // 🔥 FIX: компенсація відставання (7 хв)
  minutes += 7;

  if (minutes >= 60) {
    minutes -= 60;
    hours++;
  }

  if (hours >= 24) {
    hours = 0;
  }

  // 🔥 записуємо виправлений час назад у пам’ять
  prefs.putInt("h", hours);
  prefs.putInt("m", minutes);
  prefs.putInt("s", seconds);

  // ================= TIMER LOAD =================
  timerMinutes = prefs.getInt("tm", 1);
  timerSeconds = prefs.getInt("ts", 0);

  // ================= TIMERS INIT =================
  clockTimer = millis();
  timerMillis = millis();
}

// ================= LOOP =================
void loop() {

  bool btn = digitalRead(BUTTON);

    updateClock();   // завжди
  updateTimer();   // завжди
  updateMusic();   // якщо треба
  updateLightAlarm();

  // ===== GLOBAL TIMER ALERT (працює завжди) =====
if(timerFinished){

  tone(BUZZER, 1500);

  // НЕ даємо мигати
  if(!timerScreenDrawn){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(25,20);
    display.println("TIME IS UP!");

    display.setCursor(10,40);
    display.println("PRESS BUTTON");

    display.display();

    timerScreenDrawn = true;
  }

  // кнопка скидання
  if(btn == LOW && lastBtn == HIGH){

    noTone(BUZZER);

    timerFinished = false;
    timerScreenDrawn = false;

    timerMinutes = 1;
    timerSeconds = 0;
  }

  lastBtn = btn;
  delay(20);
  return;   // 🔥 блокує інші режими
}

  switch(gameState){
    case -1: drawMainMenu(); break;
    case 0: drawMenu(); break;
    case 5: drawMusicMenu(); break;
    case 6: drawIndicatorsMenu(); break;
    case 7: drawTimeMenu(); break;
  }
  // ================= MAIN MENU =================
  if(gameState == -1){

    drawMainMenu();

    if(btn == LOW && lastBtn == HIGH){

      if(mainMenuIndex == 0){
        gameState = 0;
      }

      else if(mainMenuIndex == 1){
        gameState = 5;
      }

      else if(mainMenuIndex == 2){
        gameState = 6;
      }

      else if(mainMenuIndex == 3){
        gameState = 7;
}
    }
  }

  // ================= MUSIC =================
  else if(gameState == 5){

    drawMusicMenu();
    updateMusic();

    if(btn == LOW && lastBtn == HIGH){

      if(musicMenuIndex == 0){

        selectedMusic = 0;
        musicPlaying = !musicPlaying;

        if(!musicPlaying) noTone(BUZZER);

        musicStep = 0;
        musicTimer = millis();
      }

      else if(musicMenuIndex == 1){

        selectedMusic = 2;
        musicPlaying = !musicPlaying;

        if(!musicPlaying) noTone(BUZZER);

        musicStep = 0;
        musicTimer = millis();
      }

      else if(musicMenuIndex == 2){

        musicPlaying = false;
        noTone(BUZZER);
        gameState = -1;
      }
    }
  }

  // ================= INDICATORS =================
  else if(gameState == 6){

    drawIndicatorsMenu();
    updateLightAlarm();

    if(btn == LOW && lastBtn == HIGH){

      // MAIN
      if(indicatorSubMenu == 0){

        if(indicatorMenuIndex == 0){
          indicatorSubMenu = 1;
        }

        else if(indicatorMenuIndex == 1){
          gameState = -1;
        }
      }
      // LIGHT MENU
      else if(indicatorSubMenu == 1){

        if(indicatorMenuIndex == 0){
          lightTriggerDark = true;
        }

        else if(indicatorMenuIndex == 1){
          lightTriggerDark = false;
        }

        else if(indicatorMenuIndex == 2){
          lightAlarmEnabled = !lightAlarmEnabled;
        }

        else if(indicatorMenuIndex == 3){
          indicatorSubMenu = 0;
        }
      }
    }
  }

// ================= TIME =================
else if(gameState == 7){

  bool btnLocal = digitalRead(BUTTON);

  // ====== ЕКРАН "TIME IS UP" ======
  if(showTimerEndScreen){

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(25,20);
    display.println("TIME IS UP!");

    display.setCursor(10,40);
    display.println("PRESS BUTTON");

    display.display();

    if(btn == LOW && lastBtn == HIGH){

      noTone(BUZZER);

      showTimerEndScreen = false;
      timerRunning = false;
      timerFinished = false;

      timerMinutes = 1;
      timerSeconds = 0;
    }

    lastBtn = btn;
    delay(20);
    return; // 🔥 ВАЖЛИВО: стопаємо звичайний рендер
  }

  // ====== ЗВИЧАЙНИЙ ЕКРАН TIME ======
  drawTimeMenu();

  updateClock();
  updateTimer();

  // ====== КНОПКА УПРАВЛІННЯ ======
 if(btn == LOW && lastBtn == HIGH){

  if(timeMenuIndex == 0){
    timerMinutes++;
    if(timerMinutes > 99) timerMinutes = 99;
  }

  else if(timeMenuIndex == 1){
    if(timerMinutes > 0) timerMinutes--;
  }

  else if(timeMenuIndex == 2){
    timerRunning = true;
    timerFinished = false;
    timerScreenDrawn = false;
    timerMillis = millis();
  }

  else if(timeMenuIndex == 3){
    gameState = -1;
  }
}
}

  // ================= ARCADE MENU =================
  else if(gameState == 0){

    drawMenu();

    if(btn == LOW && lastBtn == HIGH){

      if(menuIndex == 2){
        gameState = -1;
      }

      else{
        gameMode = menuIndex;
        score = 0;
        lives = 3;
        level = 1;
        gameState = 1;
      }
    }
  }

  // ================= START =================
  else if(gameState == 1){

    display.clearDisplay();

    display.setCursor(40,25);

    if(gameMode == 0){
      display.print("LEVEL ");
      display.print(level);
    }
    else{
      display.print("ENDLESS");
    }

    display.display();

    delay(900);

    loadLevel(level);

    gameState = 2;
  }

  // ================= GAME =================
  else if(gameState == 2){

    int pot = analogRead(POT_PIN);

    angle = map(pot,0,4095,-314,0)/100.0;

    if(btn == LOW && lastBtn == HIGH && !bullet){
      shoot();
    }

    updateBullet();

  // ===== MOVING LEVELS =====

if(gameMode == 0){

  // LEVEL 3
  if(level == 3 && alive[0]){

    moveX += dir ? 1 : -1;

    if(moveX > 120) dir = false;
    if(moveX < 5) dir = true;

    tx[0] = moveX;
  }

  // LEVEL 4
  if(level == 4 && alive[0]){

    moveX += dir ? 2 : -2;

    if(moveX > 120) dir = false;
    if(moveX < 5) dir = true;

    tx[0] = moveX;

    ty[0] = 20 + sin(millis()*0.01) * 10;
  }

// LEVEL 5
if(level == 5){

  static int vx[3] = {1,-1,1};

  for(int i=0;i<3;i++){

    if(alive[i]){

      tx[i] += vx[i];

      if(tx[i] > 120) vx[i] = -1;
      if(tx[i] < 5) vx[i] = 1;

      // плавний рух Y
      ty[i] = 20 + sin(millis()*0.005 + i*2) * 10;
    }
  }
}
}

    // LEVEL MODE
    if(gameMode == 0){

      bool done = true;

      for(int i=0;i<3;i++){
        if(alive[i]) done = false;
      }

      if(done){

        level++;

        if(level > 5){
          gameState = 3;
        }
        else{
          gameState = 1;
        }
      }

      if(lives <= 0){
        gameState = 4;
      }
    }

    // ENDLESS
    if(gameMode == 1){

      for(int i=0;i<3;i++){

        if(!alive[i]){

          tx[i] = random(10,118);
          ty[i] = random(10,50);

          alive[i] = true;
        }
      }
    }

    display.clearDisplay();

    display.setCursor(0,0);
    display.print("S:");
    display.print(score);

    if(gameMode == 0){

      display.setCursor(90,0);
      display.print("HP:");
      display.print(lives);
    }

    if(gameMode == 1){

      display.setCursor(90,0);
      display.print("H:");
      display.print(highScore);
    }

    drawPlayer();
    drawTargets();
    drawBullet();
    drawExplosion();

    display.display();
  }

  // ================= WIN =================
else if(gameState == 3){

  if(!winPlayed){
    winS();
    winPlayed = true;
  }

  display.clearDisplay();

  display.setCursor(35,20);
  display.println("YOU WIN!");

  display.setCursor(10,40);
  display.println("PRESS BUTTON");

  display.display();

  if(btn == LOW && lastBtn == HIGH){

    winPlayed = false;
    gameState = 0;
  }
}

  // ================= GAME OVER =================
else if(gameState == 4){

  if(!gameOverPlayed){
    gameOverS();
    gameOverPlayed = true;
  }

  display.clearDisplay();

  display.setCursor(25,20);
  display.println("GAME OVER");

  display.setCursor(10,40);
  display.println("PRESS BUTTON");

  display.display();

  if(btn == LOW && lastBtn == HIGH){

    gameOverPlayed = false;
    gameState = 0;
  }
}

lastBtn = btn;
delay(20);
}