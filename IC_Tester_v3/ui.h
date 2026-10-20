// ============================================================
//  ui.h  -  desenho de tela, botoes e touch
// ============================================================
#pragma once
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GRAY    0x8410
#define TTGREEN 0x03ED
#define DRKBLUE 0x0008
#define DKGRAY  0x2104

MCUFRIEND_kbv tft;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, TS_RX);

// ---------------- touch ----------------
// So devolve um "toque" quando o dedo SOLTA: nada de repeticao fantasma.
bool touchRaw(int &x, int &y) {
  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT); pinMode(YP, OUTPUT);
  digitalWrite(XM, LOW); digitalWrite(YP, HIGH);
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    int mx = map(p.x, TS_MINX, TS_MAXX, tft.height(), 0);
    int my = map(p.y, TS_MINY, TS_MAXY, 0, tft.width());
    x = my + TOUCH_OFS_X;          // mesma troca de eixos da v2.6 (rotacao 1)
    y = mx + TOUCH_OFS_Y;
    return true;
  }
  return false;
}

bool getTap(int &x, int &y) {
  static bool down = false;
  static int lx, ly;
  static uint8_t hits = 0, misses = 0;
  int tx, ty;
  if (touchRaw(tx, ty)) {
    lx = tx; ly = ty; down = true; misses = 0;
    if (hits < 255) hits++;
    return false;
  }
  if (down && ++misses >= 4) {            // ~40 ms sem pressao = soltou
    down = false; misses = 0;
    bool valid = hits >= 2;               // ignora ruido de uma leitura so
    hits = 0;
    if (valid) { x = lx; y = ly; return true; }
  }
  return false;
}

// espera soltar (usado apos telas bloqueantes de erro)
void waitTap() { int x, y; while (!getTap(x, y)) delay(10); }

// ---------------- botoes ----------------
struct Btn { int16_t x, y, w, h; const char *label; uint16_t fill; };

void drawBtn(const Btn &b, bool inv = false) {
  tft.fillRoundRect(b.x, b.y, b.w, b.h, 6, inv ? WHITE : b.fill);
  tft.drawRoundRect(b.x, b.y, b.w, b.h, 6, GRAY);
  tft.setTextSize(2); tft.setTextColor(inv ? BLACK : WHITE);
  int len = strlen(b.label);
  tft.setCursor(b.x + (b.w - len * 12) / 2 + 1, b.y + (b.h - 14) / 2);
  tft.print(b.label);
}
bool hit(const Btn &b, int x, int y) {
  return x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h;
}
void flash(const Btn &b) { drawBtn(b, true); delay(60); drawBtn(b); }

// ---------------- textos ----------------
void textAt(int x, int y, uint8_t sz, uint16_t col, const char *s) {
  tft.setCursor(x, y); tft.setTextSize(sz); tft.setTextColor(col); tft.print(s);
}
void drawTop(const char *title) {
  tft.fillRect(0, 0, 320, 19, BLACK);
  tft.drawFastHLine(0, 19, 320, GRAY);
  int len = strlen(title); if (len > 26) len = 26;
  tft.setTextWrap(false);
  tft.setCursor(160 - len * 6, 1); tft.setTextSize(2); tft.setTextColor(CYAN, BLACK);
  for (int i = 0; i < len; i++) tft.print(title[i]);
}
// Barra de status abaixo do CI (x 92..319)
void drawStatus(const char *text, uint16_t col) {
  tft.fillRect(92, 218, 228, 22, BLACK);
  int len = strlen(text); if (len > 18) len = 18;
  tft.setTextWrap(false);
  tft.setCursor(206 - len * 6, 220); tft.setTextSize(2); tft.setTextColor(col, BLACK);
  for (int i = 0; i < len; i++) tft.print(text[i]);
}

// ---------------- desenho do CI ----------------
uint16_t nameColor(const char *s) {
  if (strstr(s, "VCC") || strstr(s, "VEE") || strstr(s, "VDD") || strstr(s, "GND") || strstr(s, "VSS")) return WHITE;
  if (strstr(s, "NC") || strstr(s, "TST")) return DRKBLUE;
  return BLUE;
}

void drawIC(uint8_t n, const char *number, bool names) {
  const int sx = 166, sy = 26;
  char buf[4];
  bool errors = false;
  tft.fillRect(sx - 80, sy - 6, 234, 190, BLACK);
  tft.fillCircle(sx + 30, sy + 1, 10, GRAY);
  tft.fillRect(sx + 20, sy - 10, 21, 10, BLACK);
  tft.drawFastHLine(0, 19, 320, GRAY);
  tft.drawRect(sx, sy, 60, 184, TTGREEN);                        // contorno do ZIF
  tft.drawRect(sx + 1, sy + 1, 58, (n / 2) * 18 + 2, WHITE);     // corpo do CI
  for (uint8_t i = 0; i < n / 2; i++) {
    int y = sy + 4 + i * 18;
    tft.drawRect(sx - 15, y, 14, 14, WHITE);
    tft.drawRect(sx + 61, y, 14, 14, WHITE);
    sprintf(buf, "%2d", i + 1);  textAt(sx - 40, y, 2, GRAY, buf);
    sprintf(buf, "%d", n - i);   textAt(sx + 77, y, 2, GRAY, buf);
    if (names) {
      const char *l = pinName[i], *r = pinName[n - i - 1];
      textAt(sx - 52 - strlen(l) * 5, y, 2, nameColor(l), l);   // mesma posicao da v2.6
      textAt(sx + 128 - strlen(r) * 5, y, 2, nameColor(r), r);
      if (errPin[i])         { tft.fillRect(sx - 14, y + 1, 12, 11, RED); errors = true; }
      if (errPin[n - i - 1]) { tft.fillRect(sx + 62, y + 1, 12, 11, RED); errors = true; }
    }
  }
  if (number && *number) {
    tft.setRotation(2);
    textAt(sy + 30, sx - 52, 3, errors ? RED : WHITE, number);
    tft.setRotation(1);
  }
}

void leds(bool r, bool g) { digitalWrite(LED_R, r); digitalWrite(LED_G, g); }
