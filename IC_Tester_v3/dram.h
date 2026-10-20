// ============================================================
//  dram.h  -  teste de DRAM dinamica: 4164, 41256 (16 pinos) e 44256 (20)
//
//  Os vetores do chipdb.txt sao lentos demais para DRAM (RAS baixo
//  no maximo ~10 us, refresh a cada 2-8 ms). Aqui os pinos do ZIF sao
//  acionados direto nos registradores das portas do ATmega2560.
//  O mapa ZIF -> porta e fixo pela PCB do IC Tester:
//    ZIF  1..8  = PC7 PC5 PC3 PC1 PD7 PG1 PL7 PL5
//    ZIF  9..10 = PL3 PL1          ZIF 11..12 = PL0 PL2
//    ZIF 13..16 = PL4 PL6 PG0 PG2  ZIF 17..20 = PC0 PC2 PC4 PC6
//  (dramCheckPorts() confere isso em tempo de execucao)
//
//  Os bits logicos de endereco sao ligados aos pinos A0..A8 em outra
//  ordem, escolhida para escrever as portas com poucas instrucoes.
//  Para o teste isso nao muda nada: todo pino de endereco e usado e
//  todas as celulas sao visitadas.
//
//  Ordem de acesso: a LINHA varia mais rapido; cada ciclo faz refresh
//  da sua linha, e todas sao revisitadas em poucos ms.
//
//  Algoritmo: March C-  ^w0 ^(r0,w1) ^(r1,w0) v(r0,w1) v(r1,w0) ^r0
//  No x4 (44256) com fundo 0000/1111 e mais uma passada 0101/1010.
//  Leitura seguida de escrita na mesma celula usa ciclo read-modify-write.
//
//  ATENCAO: o VCC sai de um pino do Mega (~25 ohms, 40 mA max) e DRAM
//  NMOS tem picos maiores. Chip bom falhando = desconfie da alimentacao.
// ============================================================
#pragma once

enum DramKind { DK16, DK18, DK20 };
struct DramDef { const char *id; DramKind kind; uint8_t n; uint16_t rows; uint8_t width;
                 uint8_t vcc, gnd, dout; uint8_t dq[4]; };
const DramDef DRAMS[] = {
  //  id       tipo  pinos linhas larg VCC GND DOUT  DQ1..DQ4
  { "4164",  DK16, 16, 256, 1,  8, 16, 14, {0} },
  { "41256", DK16, 16, 512, 1,  8, 16, 14, {0} },
  { "44256", DK20, 20, 512, 4, 10, 20,  0, {1, 2, 18, 19} },
  { "4464",  DK18, 18, 256, 4,  9, 18,  0, {2, 3, 15, 17} },
};
const uint8_t NUM_DRAMS = sizeof(DRAMS) / sizeof(DRAMS[0]);
static const DramDef *dd;

// ---------------- 16 pinos: 4164 / 41256 ----------------
// NC/A8=1 PC7  DIN=2 PC5  WE=3 PC3  RAS=4 PC1  5 PD7  6 PG1  7 PL7  VCC=8 PL5
// 9 PL4  10 PL6  11 PG0  12 PG2  13 PC0  DOUT=14 PC2  CAS=15 PC4  GND=16 PC6
#define S_RAS16 _BV(1)
#define S_CAS16 _BV(4)
#define S_WE16  _BV(3)
#define S_DIN16 _BV(5)
#define S_DO16  _BV(2)
#define AI __attribute__((always_inline)) static inline
#define WAIT3() __asm__ __volatile__("nop\n\tnop\n\tnop\n\t")
#define WAIT6() __asm__ __volatile__("nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t")

struct Ad { uint8_t g, l, d, c; };          // bytes de endereco ja espalhados por porta

// b0-2 -> PG0..2   b3 -> PL4   b4-5 -> PL6..7   b6 -> PD7   b7 -> PC0   b8 -> PC7
AI Ad prep16(uint16_t v) {
  Ad a; a.g = v & 0x07; a.l = ((v & 0x08) << 1) | ((v & 0x30) << 2);
  a.d = (v & 0x40) << 1; a.c = ((v >> 7) & 1) | ((v >> 1) & 0x80); return a;
}
AI void put16(const Ad &a) {
  PORTG = (PORTG & ~0x07) | a.g;
  PORTL = (PORTL & ~0xD0) | a.l;
  PORTD = (PORTD & 0x7F)  | a.d;
  PORTC = (PORTC & 0x7E)  | a.c;
}
AI uint8_t read16(const Ad &r, const Ad &c) {
  put16(r); PORTC &= ~S_RAS16;
  put16(c); PORTC &= ~S_CAS16;
  WAIT6();
  uint8_t d = (PINC & S_DO16) ? 1 : 0;
  PORTC |= S_CAS16; PORTC |= S_RAS16;
  return d;
}
AI void write16(const Ad &r, const Ad &c, uint8_t d) {
  put16(r); PORTC &= ~S_RAS16;
  put16(c);
  if (d) PORTC |= S_DIN16; else PORTC &= ~S_DIN16;
  PORTC &= ~S_WE16;                         // early write
  PORTC &= ~S_CAS16;
  WAIT3();
  PORTC |= S_CAS16; PORTC |= S_WE16; PORTC |= S_RAS16;
}
// leitura-modificacao-escrita num so ciclo de RAS (le, depois grava d)
AI uint8_t rmw16(const Ad &r, const Ad &c, uint8_t d) {
  put16(r); PORTC &= ~S_RAS16;
  put16(c); PORTC &= ~S_CAS16;
  WAIT6();
  uint8_t v = (PINC & S_DO16) ? 1 : 0;
  if (d) PORTC |= S_DIN16; else PORTC &= ~S_DIN16;
  PORTC &= ~S_WE16;                         // late write: grava na descida do WE
  WAIT3();
  PORTC |= S_WE16; PORTC |= S_CAS16; PORTC |= S_RAS16;
  return v;
}

// ---------------- 20 pinos: 44256 ----------------
// DQ1=1 PC7  DQ2=2 PC5  WE=3 PC3  RAS=4 PC1  NC=5 PD7  A=6 PG1  7 PL7  8 PL5
// 9 PL3  VCC=10 PL1  11 PL0  12 PL2  13 PL4  14 PL6  15 PG0  OE=16 PG2
// CAS=17 PC0  DQ3=18 PC2  DQ4=19 PC4  GND=20 PC6
#define S_RAS20 _BV(1)
#define S_CAS20 _BV(0)
#define S_WE20  _BV(3)
#define S_OE20  _BV(2)          // em PORTG
#define S_DQ20  0xB4            // PC7 PC5 PC2 PC4
// b0-1 -> PG0..1   b2 -> PL0   b3-8 -> PL2..7   (PL1 = VCC, preservado)
AI Ad prep20(uint16_t v) {
  Ad a; uint8_t x = v >> 2; a.g = v & 0x03; a.l = (x & 0x01) | ((x & 0x7E) << 1); a.d = 0; a.c = 0; return a;
}
AI void put20(const Ad &a) {
  PORTG = (PORTG & ~0x03) | a.g;
  PORTL = (PORTL & 0x02)  | a.l;
}
AI uint8_t dq2port(uint8_t d) { return ((d & 1) << 7) | ((d & 2) << 4) | (d & 4) | ((d & 8) << 1); }
AI uint8_t port2dq(uint8_t p) { return ((p >> 7) & 1) | ((p >> 4) & 2) | (p & 4) | ((p >> 1) & 8); }
AI void dq20out(bool out) {
  if (out) DDRC |= S_DQ20; else { DDRC &= ~S_DQ20; PORTC &= ~S_DQ20; }
}
AI uint8_t read20(const Ad &r, const Ad &c) {
  put20(r); PORTC &= ~S_RAS20;
  put20(c); PORTG &= ~S_OE20; PORTC &= ~S_CAS20;
  WAIT6();
  uint8_t d = port2dq(PINC);
  PORTC |= S_CAS20; PORTG |= S_OE20; PORTC |= S_RAS20;
  return d;
}
AI void write20(const Ad &r, const Ad &c, uint8_t d) {       // DQ ja como saida
  put20(r); PORTC &= ~S_RAS20;
  put20(c);
  PORTC = (PORTC & ~S_DQ20) | dq2port(d);
  PORTC &= ~S_WE20;
  PORTC &= ~S_CAS20;
  WAIT3();
  PORTC |= S_CAS20; PORTC |= S_WE20; PORTC |= S_RAS20;
}
AI uint8_t rmw20(const Ad &r, const Ad &c, uint8_t d) {     // entra e sai com DQ como entrada
  put20(r); PORTC &= ~S_RAS20;
  put20(c); PORTG &= ~S_OE20; PORTC &= ~S_CAS20;
  WAIT6();
  uint8_t v = port2dq(PINC);
  PORTG |= S_OE20;                          // solta o barramento antes de dirigir
  WAIT3();
  PORTC = (PORTC & ~S_DQ20) | dq2port(d);
  DDRC |= S_DQ20;
  PORTC &= ~S_WE20;                         // late write
  WAIT3();
  PORTC |= S_WE20; PORTC |= S_CAS20; PORTC |= S_RAS20;
  DDRC &= ~S_DQ20; PORTC &= ~S_DQ20;
  return v;
}

// ---------------- 18 pinos: 4464 / 41464 (64K x4) ----------------
// OE=1 PC7  DQ1=2 PC5  DQ2=3 PC3  WE=4 PC1  RAS=5 PD7  6 PG1  7 PL7  8 PL5  VCC=9 PL3
// 10 PL2  11 PL4  12 PL6  13 PG0  14 PG2  DQ3=15 PC0  CAS=16 PC2  DQ4=17 PC4  GND=18 PC6
#define S_RAS18 _BV(7)          // em PORTD
#define S_CAS18 _BV(2)
#define S_WE18  _BV(1)
#define S_OE18  _BV(7)          // em PORTC
#define S_DQ18  0x39            // PC5 PC3 PC0 PC4
// b0-2 -> PG0..2   b3 -> PL2   b4-7 -> PL4..7   (PL3 = VCC, preservado)
AI Ad prep18(uint16_t v) {
  Ad a; uint8_t x = v >> 3; a.g = v & 0x07; a.l = ((x & 0x01) << 2) | ((x & 0x1E) << 3); a.d = 0; a.c = 0; return a;
}
AI void put18(const Ad &a) {
  PORTG = (PORTG & ~0x07) | a.g;
  PORTL = (PORTL & ~0xF4) | a.l;
}
AI uint8_t dq2port18(uint8_t d) { return ((d & 1) << 5) | ((d & 2) << 2) | ((d & 4) >> 2) | ((d & 8) << 1); }
AI uint8_t port2dq18(uint8_t p) { return ((p >> 5) & 1) | ((p >> 2) & 2) | ((p & 1) << 2) | ((p >> 1) & 8); }
AI void dq18out(bool out) {
  if (out) DDRC |= S_DQ18; else { DDRC &= ~S_DQ18; PORTC &= ~S_DQ18; }
}
AI uint8_t read18(const Ad &r, const Ad &c) {
  put18(r); PORTD &= ~S_RAS18;
  put18(c); PORTC &= ~S_OE18; PORTC &= ~S_CAS18;
  WAIT6();
  uint8_t d = port2dq18(PINC);
  PORTC |= S_CAS18; PORTC |= S_OE18; PORTD |= S_RAS18;
  return d;
}
AI void write18(const Ad &r, const Ad &c, uint8_t d) {       // DQ ja como saida
  put18(r); PORTD &= ~S_RAS18;
  put18(c);
  PORTC = (PORTC & ~S_DQ18) | dq2port18(d);
  PORTC &= ~S_WE18;
  PORTC &= ~S_CAS18;
  WAIT3();
  PORTC |= S_CAS18; PORTC |= S_WE18; PORTD |= S_RAS18;
}
AI uint8_t rmw18(const Ad &r, const Ad &c, uint8_t d) {     // entra e sai com DQ como entrada
  put18(r); PORTD &= ~S_RAS18;
  put18(c); PORTC &= ~S_OE18; PORTC &= ~S_CAS18;
  WAIT6();
  uint8_t v = port2dq18(PINC);
  PORTC |= S_OE18;
  WAIT3();
  PORTC = (PORTC & ~S_DQ18) | dq2port18(d);
  DDRC |= S_DQ18;
  PORTC &= ~S_WE18;                         // late write
  WAIT3();
  PORTC |= S_WE18; PORTC |= S_CAS18; PORTD |= S_RAS18;
  DDRC &= ~S_DQ18; PORTC &= ~S_DQ18;
  return v;
}

// ---------------- despacho por tipo ----------------
template <DramKind K> AI Ad prepK(uint16_t v) { return K == DK16 ? prep16(v) : K == DK18 ? prep18(v) : prep20(v); }
template <DramKind K> AI uint8_t readK(const Ad &r, const Ad &c) {
  return K == DK16 ? read16(r, c) : K == DK18 ? read18(r, c) : read20(r, c); }
template <DramKind K> AI void writeK(const Ad &r, const Ad &c, uint8_t d) {
  if (K == DK16) write16(r, c, d); else if (K == DK18) write18(r, c, d); else write20(r, c, d); }
template <DramKind K> AI uint8_t rmwK(const Ad &r, const Ad &c, uint8_t d) {
  return K == DK16 ? rmw16(r, c, d) : K == DK18 ? rmw18(r, c, d) : rmw20(r, c, d); }
static void dqOut(bool out) {
  if (dd->kind == DK18) dq18out(out); else if (dd->kind == DK20) dq20out(out);
}

// ---------------- conferencia do mapa de portas ----------------
static bool samePort(uint8_t megaPinNo, volatile uint8_t *reg, uint8_t bit) {
  return portOutputRegister(digitalPinToPort(megaPinNo)) == reg && digitalPinToBitMask(megaPinNo) == _BV(bit);
}
bool dramCheckPorts() {
  struct { volatile uint8_t *r; uint8_t b; } m[20] = {
    {&PORTC,7},{&PORTC,5},{&PORTC,3},{&PORTC,1},{&PORTD,7},{&PORTG,1},{&PORTL,7},{&PORTL,5},{&PORTL,3},{&PORTL,1},
    {&PORTL,0},{&PORTL,2},{&PORTL,4},{&PORTL,6},{&PORTG,0},{&PORTG,2},{&PORTC,0},{&PORTC,2},{&PORTC,4},{&PORTC,6} };
  for (uint8_t i = 0; i < 20; i++) if (!samePort(ZIF[i], m[i].r, m[i].b)) return false;
  return true;
}

// ---------------- liga o CI ----------------
static void dramPowerUp() {
  zifRelease();
  for (uint8_t k = 0; k < dd->n; k++) {
    uint8_t mp = megaPin(dd->n, k);
    uint8_t chipPin = k + 1;
    bool isData = (dd->width == 1) ? (chipPin == dd->dout)
                                   : (chipPin == dd->dq[0] || chipPin == dd->dq[1] || chipPin == dd->dq[2] || chipPin == dd->dq[3]);
    if (isData) { pinMode(mp, INPUT); digitalWrite(mp, LOW); continue; }
    pinMode(mp, OUTPUT);
    digitalWrite(mp, chipPin == dd->gnd ? LOW : LOW);
  }
  // controles em repouso (alto); endereco e NC em 0
  if (dd->kind == DK16) PORTC |= S_RAS16 | S_CAS16 | S_WE16;
  else if (dd->kind == DK18) { PORTD |= S_RAS18; PORTC |= S_CAS18 | S_WE18 | S_OE18; }
  else { PORTC |= S_RAS20 | S_CAS20 | S_WE20; PORTG |= S_OE20; }
  digitalWrite(megaPin(dd->n, dd->vcc - 1), HIGH);     // VCC por ultimo
  delay(2);                                             // > 200 us
  for (uint8_t i = 0; i < 16; i++) {                    // ciclos de aquecimento
    if (dd->kind == DK16)      { put16(prep16(i)); PORTC &= ~S_RAS16; PORTC |= S_RAS16; }
    else if (dd->kind == DK18) { put18(prep18(i)); PORTD &= ~S_RAS18; PORTD |= S_RAS18; }
    else                       { put20(prep20(i)); PORTC &= ~S_RAS20; PORTC |= S_RAS20; }
  }
}

// ---------------- resultado ----------------
uint32_t dramFailCount;
uint16_t dramFailRow, dramFailCol;
uint8_t  dramFailBits;
enum DramResult { D_GOOD, D_BAD, D_NORESP };

static void markFail(uint16_t r, uint16_t c, uint8_t diff) {
  if (!dramFailCount) { dramFailRow = r; dramFailCol = c; }
  dramFailCount++;
  dramFailBits |= diff;
}

// ---------------- elemento March ----------------
// op1 'r' ou 'w' com d1; op2 'w' (com d2) ou 0.  r+w vira um ciclo RMW.
template <DramKind K>
static void marchT(bool up, char op1, uint8_t d1, char op2, uint8_t d2, uint16_t rows, uint16_t cols) {
  for (uint16_t ci = 0; ci < cols; ci++) {
    uint16_t c = up ? ci : cols - 1 - ci;
    Ad ca = prepK<K>(c);
    for (uint16_t ri = 0; ri < rows; ri++) {
      uint16_t r = up ? ri : rows - 1 - ri;           // linha varia mais rapido = refresh
      Ad ra = prepK<K>(r);
      uint8_t s = SREG; cli();
      if (op1 == 'r') {
        uint8_t v = op2 ? rmwK<K>(ra, ca, d2) : readK<K>(ra, ca);
        SREG = s;
        if (v != d1) markFail(r, c, v ^ d1);
      } else { writeK<K>(ra, ca, d1); SREG = s; }
    }
  }
}
static void march(bool up, char op1, uint8_t d1, char op2, uint8_t d2) {
  if (dd->width == 4) dqOut(op1 == 'w');
  if (dd->kind == DK16)      marchT<DK16>(up, op1, d1, op2, d2, dd->rows, dd->rows);
  else if (dd->kind == DK18) marchT<DK18>(up, op1, d1, op2, d2, dd->rows, dd->rows);
  else                       marchT<DK20>(up, op1, d1, op2, d2, dd->rows, dd->rows);
  if (dd->width == 4) dqOut(false);
}

// a memoria responde? (pega chip ausente, invertido, morto ou sem VCC)
static bool dramSanity() {
  uint8_t one = (dd->width == 1) ? 1 : 15, bad = 0;
  for (uint8_t k = 0; k < 16; k++) {
    uint16_t r = (k * 37) & (dd->rows - 1), c = (k * 91) & (dd->rows - 1);
    uint8_t d = (k & 1) ? one : 0;
    uint8_t v;
    if (dd->kind == DK16) { Ad ra = prep16(r), ca = prep16(c); cli(); write16(ra, ca, d); v = read16(ra, ca); sei(); }
    else if (dd->kind == DK18) { Ad ra = prep18(r), ca = prep18(c); dq18out(true); cli(); write18(ra, ca, d); sei(); dq18out(false); cli(); v = read18(ra, ca); sei(); }
    else { Ad ra = prep20(r), ca = prep20(c); dq20out(true); cli(); write20(ra, ca, d); sei(); dq20out(false); cli(); v = read20(ra, ca); sei(); }
    if (v != d) bad++;
  }
  return bad < 8;
}

int8_t dramFind(const char *id) {
  for (uint8_t i = 0; i < NUM_DRAMS; i++) if (strcmp(DRAMS[i].id, id) == 0) return i;
  return -1;
}

// progress(passo, total) e chamado entre os elementos
DramResult dramTest(uint8_t type, void (*progress)(uint8_t, uint8_t)) {
  dd = &DRAMS[type];
  dramFailCount = 0; dramFailBits = 0;
  dramPowerUp();
  if (!dramSanity()) { zifRelease(); return D_NORESP; }
  uint8_t one = (dd->width == 1) ? 1 : 15;
  uint8_t steps = (dd->width == 4) ? 8 : 6;
  progress(1, steps); march(true,  'w', 0,   0,   0);
  progress(2, steps); march(true,  'r', 0,   'w', one);
  progress(3, steps); march(true,  'r', one, 'w', 0);
  progress(4, steps); march(false, 'r', 0,   'w', one);
  progress(5, steps); march(false, 'r', one, 'w', 0);
  progress(6, steps); march(true,  'r', 0,   0,   0);
  if (dd->width == 4) {
    progress(7, steps); march(true, 'w', 0x5, 0, 0);
    progress(8, steps); march(true, 'r', 0x5, 'w', 0xA);
    march(true, 'r', 0xA, 0, 0);
  }
  zifRelease();
  return dramFailCount ? D_BAD : D_GOOD;
}

void dramMarkErrPins() {
  for (uint8_t i = 0; i < 20; i++) errPin[i] = false;
  if (dd->width == 1) { if (dramFailBits) errPin[dd->dout - 1] = true; }
  else for (uint8_t i = 0; i < 4; i++) if (dramFailBits & (1 << i)) errPin[dd->dq[i] - 1] = true;
}
