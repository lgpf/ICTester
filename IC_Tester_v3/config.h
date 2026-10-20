// ============================================================
//  config.h  -  tudo que e especifico do SEU hardware fica aqui
// ============================================================
#pragma once

// ---- Touch (valores do TouchScreen_Calibr_native.ino) ----
// ALL Touch panels and wiring is DIFFERENT
const int XP = 6, XM = A2, YP = A1, YM = 7;   // ID=0x9341
//const int TS_MINX = 100, TS_MAXX = 952, TS_MINY = 170, TS_MAXY = 939;
const int TS_MINX = 100, TS_MAXX = 952, TS_MINY = 939, TS_MAXY = 170;
#define MINPRESSURE 150
#define MAXPRESSURE 1000
#define TS_RX       300     // resistencia X do painel (ohms), como no original
// Ajuste fino opcional, somado a coordenada final (pixels)
#define TOUCH_OFS_X 0
#define TOUCH_OFS_Y 0

// ---- LEDs (opcionais na PCB) ----
#define LED_R 50
#define LED_G 52
#define LED_W 22

// ---- SD (shield TFT, SPI por software, biblioteca SD_patched) ----
#define SD_CS 10
#define DB_FILE "chipdb.txt"

// ---- Limites ----
#define MAX_CHIPS   480     // entradas no indice em RAM (8 bytes cada)
#define MAX_MATCH   24      // achados guardados no modo Auto
