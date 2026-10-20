// ============================================================
//  IC Tester v3.0
//  Reescrita da navegacao/teste sobre o ICTester v2.6 de
//  Frank Hellmann (GPL v3) - https://github.com/gotohellmann/ICTester
//  baseado no Smart-IC-Tester de Akshay Baweja.
//
//  Mesmo hardware: Arduino Mega 2560 + shield TFT ILI9341 touch
//  + SD + PCB do IC Tester (ZIF 20 pinos). Somente CIs de 5V!
//
//  Novidades:
//   - toque so dispara ao soltar (fim de toque duplo/repetido)
//   - telas sem lacos bloqueantes; Auto mostra lista para escolher
//   - teclado com Apaga/Cancela
//   - indice do banco em RAM (Auto e Manual muito mais rapidos)
//   - CIs de 8 pinos (encostados no topo do ZIF: pinos 1-4 e 17-20)
//   - CI desligado (pinos soltos) apos cada teste
//   - contador de bons/ruins no Retestar (lotes)
//   - Pinos so oferece os tamanhos que existem no banco
//   - codigo '-' nos vetores = pino solto e nao conferido
//  v3.1: teste de DRAM 4164 / 41256 / 44256 (March C-), ver dram.h
//  v3.2: + DRAM 4464 / 41464 (64K x4, 18 pinos)
//
//  Bibliotecas: MCUFRIEND_kbv, Adafruit GFX, Adafruit TouchScreen
//  e a SD_patched do repositorio original.
//  O chipdb.txt vai na raiz do cartao SD (FAT/FAT32, ate 4 GB).
// ============================================================
#define VERSION "v3.2"

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#define MEGA_SOFT_SPI 1
#define SOFTWARE_SPI 1
#include <SPI.h>
#include <SD_patched.h>

#include "config.h"
#include "zif.h"
#include "db.h"
#include "tester.h"
#include "dram.h"
#include "ui.h"

// ---------------- estado ----------------
enum Screen { SCR_SPLASH, SCR_MAIN, SCR_KEYPAD, SCR_RESULTS };
Screen screen = SCR_SPLASH;

uint8_t  pinCount = 14;
int16_t  curIdx = -1;               // CI selecionado (indice) ou -1
uint16_t batchOk = 0, batchBad = 0; // contador do lote no Retestar

uint16_t matches[MAX_MATCH];
uint8_t  matchCount = 0, matchTotal = 0, matchPage = 0;

char keyBuf[8] = ""; uint8_t keyLen = 0;

// ---------------- botoes ----------------
const Btn bPins   = {  5,  25, 80, 50, "Pinos",  DKGRAY };
const Btn bAuto   = {  5,  80, 80, 50, "Auto",   TTGREEN };
const Btn bManual = {  5, 135, 80, 50, "Manual", TTGREEN };
const Btn bRetest = {  5, 190, 80, 50, "Repete", 0x7BE0 };

const char *keyLabels[10] = {"1","2","3","4","5","6","7","8","9","0"};
Btn keys[10];
const Btn bCancel = {   4, 185, 96, 46, "Canc",  RED };
const Btn bDel    = { 112, 185, 96, 46, "Apaga", DKGRAY };
const Btn bOK     = { 220, 185, 96, 46, "OK",    TTGREEN };

const Btn bPrev   = {   4, 196, 96, 40, "<",    DKGRAY };
const Btn bBack   = { 112, 196, 96, 40, "Sair", RED };
const Btn bNext   = { 220, 196, 96, 40, ">",    DKGRAY };
#define ROWS_PER_PAGE 4
#define ROW_Y0 24
#define ROW_H  42

// ---------------- utilitarios ----------------
void numStr(uint32_t v, char *b) { ultoa(v, b, 10); }

void showSelectedNames() {           // recarrega nomes de pinos do CI atual
  if (curIdx >= 0) { dbOpenChip(curIdx); dbClose(); }
}

// ---------------- telas ----------------
void drawSplash() {
  tft.fillScreen(BLACK);
  textAt(25, 30, 5, CYAN, "IC TESTER");
  textAt(130, 80, 3, WHITE, VERSION);
  textAt(40, 115, 1, GRAY, "base: ICTester v2.6 de Frank Hellmann");
  char b[32];
  textAt(40, 140, 2, MAGENTA, dbVersion);
  sprintf(b, "%u CIs no banco", chipCount);
  textAt(40, 162, 2, TTGREEN, b);
  if (dbOverflow) textAt(40, 182, 1, RED, "AVISO: aumente MAX_CHIPS em config.h");
  textAt(40, 215, 2, CYAN, "Toque para iniciar");
}

void drawMain() {
  tft.fillScreen(BLACK);
  bool sel = curIdx >= 0;
  drawTop(sel ? curName : "IC Tester");
  drawIC(pinCount, sel ? curNumStr() : "", sel);
  drawBtn(bPins); drawBtn(bAuto); drawBtn(bManual);
  if (sel) drawBtn(bRetest);
}

char numTmp[11];
const char *curNumStr() { if (curIdx < 0) return ""; numStr(chips[curIdx].num, numTmp); return numTmp; }

void statusPins() {
  char b[20];
  if (pinCount == 8) strcpy(b, "8 pinos: topo ZIF");
  else sprintf(b, "%u pinos", pinCount);
  drawStatus(b, TTGREEN);
}

void drawKeypad() {
  tft.fillScreen(BLACK);
  drawTop("Numero do CI");
  for (uint8_t r = 0; r < 2; r++)
    for (uint8_t c = 0; c < 5; c++) {
      uint8_t k = r * 5 + c;
      keys[k] = { (int16_t)(9 + c * 63), (int16_t)(67 + r * 59), 50, 46, keyLabels[k], TTGREEN };
      drawBtn(keys[k]);
    }
  drawBtn(bCancel); drawBtn(bDel); drawBtn(bOK);
  tft.drawRect(90, 24, 140, 36, WHITE);
  drawKeyField();
}
void drawKeyField() {
  tft.fillRect(91, 25, 138, 34, BLACK);
  textAt(98, 32, 3, TTGREEN, keyBuf);
}

void drawResults() {
  tft.fillScreen(BLACK);
  char b[28];
  if (matchTotal > matchCount) sprintf(b, "%u achados (%u)", matchTotal, matchCount);
  else sprintf(b, "%u achados - toque um", matchCount);
  drawTop(b);
  uint8_t first = matchPage * ROWS_PER_PAGE;
  for (uint8_t r = 0; r < ROWS_PER_PAGE && first + r < matchCount; r++) {
    uint16_t idx = matches[first + r];
    int y = ROW_Y0 + r * ROW_H;
    tft.drawRoundRect(2, y, 316, ROW_H - 4, 5, GRAY);
    numStr(chips[idx].num, b);
    textAt(10, y + 11, 2, WHITE, b);
    dbOpenChip(idx); dbClose();
    char nm[18]; strncpy(nm, curName, 17); nm[17] = 0;
    textAt(104, y + 11, 2, CYAN, nm);
  }
  uint8_t pages = (matchCount + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
  if (matchPage > 0) drawBtn(bPrev);
  drawBtn(bBack);
  if (matchPage + 1 < pages) drawBtn(bNext);
  // curName foi sobrescrito ao listar: restaura o selecionado
  showSelectedNames();
}

// ---------------- acoes ----------------
void selectChip(int16_t idx) {
  if (idx != curIdx) { batchOk = batchBad = 0; }
  curIdx = idx;
  if (idx >= 0) { pinCount = entPins(idx); showSelectedNames(); }
}

void dramProgress(uint8_t k, uint8_t total) {
  char b[20]; sprintf(b, "DRAM passo %u/%u", k, total);
  drawStatus(b, YELLOW);
}

void doManualTest(bool counting) {
  drawStatus("Testando...", YELLOW); leds(1, 1);
  TestResult r = testChip(curIdx, false);
  char b[24], n[11]; numStr(chips[curIdx].num, n);
  if (r == T_DRAM) { doDramTest(counting, n); return; }
  drawTop(curName);
  drawIC(curPins, n, true);
  if (r == T_GOOD)       { if (counting) batchOk++;  leds(0, 1); }
  else if (r == T_BAD)   { if (counting) batchBad++; leds(1, 0); }
  else                   { leds(0, 0); }
  switch (r) {
    case T_GOOD:  sprintf(b, "%s OK", n); break;
    case T_BAD:   sprintf(b, "%s FALHA", n); break;
    case T_NOVEC: sprintf(b, "%s sem testes", n); break;
    default:      strcpy(b, "Erro no SD"); break;
  }
  if (counting && (r == T_GOOD || r == T_BAD) && (batchOk + batchBad) > 1) {
    char c[12]; sprintf(c, " %u/%u", batchOk, batchBad); strcat(b, c);
  }
  drawStatus(b, r == T_GOOD ? GREEN : r == T_BAD ? RED : MAGENTA);
}

void doDramTest(bool counting, const char *n) {
  char b[24];
  int8_t t = dramFind(dramId);
  if (t < 0) { sprintf(b, "DRAM%s?", dramId); drawStatus(b, MAGENTA); leds(0, 0); return; }
  if (!dramCheckPorts()) { drawStatus("Mapa de portas?", RED); leds(1, 0); return; }
  unsigned long t0 = millis();
  DramResult r = dramTest(t, dramProgress);
  unsigned long secs = (millis() - t0 + 500) / 1000;
  if (r == D_GOOD) { dramMarkErrPins(); if (counting) batchOk++; leds(0, 1); }
  else if (r == D_BAD) { dramMarkErrPins(); if (counting) batchBad++; leds(1, 0); }
  else { for (uint8_t i = 0; i < 20; i++) errPin[i] = false; leds(1, 0); }
  drawTop(curName);
  drawIC(curPins, n, true);
  if (r == D_NORESP) {
    drawStatus("Sem resposta", RED);
    // dica na area livre a esquerda (abaixo dos botoes nao ha espaco: usa o topo)
    drawTop("Sem resposta: VCC/posicao?");
    Serial.println(F("DRAM sem resposta: chip ausente, invertido, morto ou VCC insuficiente"));
    return;
  }
  if (r == D_GOOD) sprintf(b, "%s OK %lus", n, secs);
  else             sprintf(b, "%s FALHA", n);
  if (counting && (batchOk + batchBad) > 1) { char c[12]; sprintf(c, " %u/%u", batchOk, batchBad); strcat(b, c); }
  drawStatus(b, r == D_GOOD ? GREEN : RED);
  if (r == D_BAD) {
    Serial.print(F("DRAM falhas: ")); Serial.print(dramFailCount);
    Serial.print(F("  primeira em linha ")); Serial.print(dramFailRow);
    Serial.print(F(" coluna ")); Serial.print(dramFailCol);
    Serial.print(F("  bits DQ ")); Serial.println(dramFailBits, BIN);
    char tb[28]; sprintf(tb, "%lu erros r%u c%u", (unsigned long)dramFailCount, dramFailRow, dramFailCol);
    drawTop(tb);
  }
}

void doAuto() {
  // Auto procura um CI novo: desfaz a selecao anterior (os testes
  // dos candidatos sobrescrevem nome e pinos carregados)
  curIdx = -1; batchOk = batchBad = 0;
  for (uint8_t i = 0; i < 20; i++) errPin[i] = false;
  drawMain();
  drawStatus("Procurando...", YELLOW); leds(1, 1);
  matchCount = matchTotal = 0;
  uint16_t tested = 0;
  for (uint16_t i = 0; i < chipCount; i++) {
    if (entPins(i) != pinCount) continue;
    TestResult r = testChip(i, true);
    tested++;
    if ((tested & 7) == 0) { char b[20]; sprintf(b, "Procurando %u", tested); drawStatus(b, YELLOW); }
    if (r == T_GOOD) {
      matchTotal++;
      if (matchCount < MAX_MATCH) matches[matchCount++] = i;
    }
  }
  if (matchCount == 0) {
    leds(0, 0);
    drawStatus("Nada encontrado", MAGENTA);
    return;
  }
  leds(0, 1);
  if (matchCount == 1) {                  // um so: seleciona direto
    selectChip(matches[0]);
    drawMain();
    char b[20]; sprintf(b, "Achado: %s", curNumStr());
    drawStatus(b, GREEN);
    return;
  }
  matchPage = 0;
  screen = SCR_RESULTS;
  drawResults();
}

void nextPinCount() {
  // proximo tamanho que existe no banco
  for (uint8_t k = 0; k < 21; k++) {
    pinCount++;
    if (pinCount > 20) pinCount = 8;
    if (sizesMask & (1UL << pinCount)) break;
  }
}

// ---------------- toques ----------------
void onTapMain(int x, int y) {
  if (hit(bPins, x, y)) {
    flash(bPins);
    nextPinCount();
    curIdx = -1; batchOk = batchBad = 0;
    for (uint8_t i = 0; i < 20; i++) errPin[i] = false;
    drawMain(); statusPins(); leds(0, 0);
  } else if (hit(bAuto, x, y)) {
    flash(bAuto);
    doAuto();
  } else if (hit(bManual, x, y)) {
    flash(bManual);
    keyLen = 0; keyBuf[0] = 0;
    screen = SCR_KEYPAD; drawKeypad();
  } else if (curIdx >= 0 && hit(bRetest, x, y)) {
    flash(bRetest);
    doManualTest(true);
  }
}

void onTapKeypad(int x, int y) {
  for (uint8_t k = 0; k < 10; k++)
    if (hit(keys[k], x, y)) {
      flash(keys[k]);
      if (keyLen < 7) { keyBuf[keyLen++] = keyLabels[k][0]; keyBuf[keyLen] = 0; drawKeyField(); }
      return;
    }
  if (hit(bDel, x, y)) {
    flash(bDel);
    if (keyLen) { keyBuf[--keyLen] = 0; drawKeyField(); }
  } else if (hit(bCancel, x, y)) {
    flash(bCancel);
    screen = SCR_MAIN; drawMain(); statusPins();
  } else if (hit(bOK, x, y)) {
    flash(bOK);
    if (!keyLen) { screen = SCR_MAIN; drawMain(); statusPins(); return; }
    uint32_t num = strtoul(keyBuf, NULL, 10);
    int16_t idx = dbFind(num);
    screen = SCR_MAIN;
    if (idx < 0) {
      drawMain();
      char b[24]; sprintf(b, "%s nao achado", keyBuf);
      drawStatus(b, MAGENTA); leds(0, 0);
      return;
    }
    selectChip(idx);
    for (uint8_t i = 0; i < 20; i++) errPin[i] = false;
    drawMain();
    doManualTest(true);
  }
}

void onTapResults(int x, int y) {
  uint8_t pages = (matchCount + ROWS_PER_PAGE - 1) / ROWS_PER_PAGE;
  if (matchPage > 0 && hit(bPrev, x, y)) { flash(bPrev); matchPage--; drawResults(); return; }
  if (matchPage + 1 < pages && hit(bNext, x, y)) { flash(bNext); matchPage++; drawResults(); return; }
  if (hit(bBack, x, y)) { flash(bBack); screen = SCR_MAIN; drawMain(); statusPins(); return; }
  if (y >= ROW_Y0 && y < ROW_Y0 + ROWS_PER_PAGE * ROW_H) {
    uint8_t r = (y - ROW_Y0) / ROW_H;
    uint8_t m = matchPage * ROWS_PER_PAGE + r;
    if (m < matchCount) {
      selectChip(matches[m]);
      for (uint8_t i = 0; i < 20; i++) errPin[i] = false;
      screen = SCR_MAIN; drawMain();
      char b[24]; sprintf(b, "%s escolhido", curNumStr());
      drawStatus(b, GREEN);
    }
  }
}

// ---------------- SD ----------------
void sdError(const char *msg) {
  leds(1, 0);
  tft.fillScreen(BLACK);
  textAt(10, 90, 2, RED, "ERRO:");
  textAt(80, 90, 2, MAGENTA, msg);
  textAt(10, 130, 1, GRAY, "Cartao FAT/FAT32 ate 4GB, chipdb.txt na raiz.");
  textAt(30, 210, 2, CYAN, "Toque para tentar de novo");
  waitTap();
}

void initDB() {
  pinMode(SD_CS, OUTPUT);
  while (true) {
    if (!SD.begin(SD_CS))          { sdError("SD nao encontrado"); continue; }
    if (!dbBuildIndex())           { sdError("sem chipdb.txt");   continue; }
    if (chipCount == 0)            { sdError("banco vazio");      continue; }
    break;
  }
  if (!(sizesMask & (1UL << pinCount))) nextPinCount();
}

// ---------------- setup/loop ----------------
void setup() {
  Serial.begin(115200);
  zifRelease();
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_W, OUTPUT);
  leds(0, 0); digitalWrite(LED_W, HIGH);

  tft.reset();
  uint16_t id = tft.readID();
  if (id == 0x0101) id = 0x9341;        // correcao de alguns shields mcufriend
  tft.begin(id);
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  textAt(60, 110, 2, CYAN, "Lendo banco...");

  initDB();
  Serial.print(F("CIs no indice: ")); Serial.println(chipCount);
  drawSplash();
  screen = SCR_SPLASH;
}

void loop() {
  int x, y;
  if (getTap(x, y)) {
    switch (screen) {
      case SCR_SPLASH:  screen = SCR_MAIN; drawMain(); statusPins(); break;
      case SCR_MAIN:    onTapMain(x, y);    break;
      case SCR_KEYPAD:  onTapKeypad(x, y);  break;
      case SCR_RESULTS: onTapResults(x, y); break;
    }
  }
  delay(10);
}
