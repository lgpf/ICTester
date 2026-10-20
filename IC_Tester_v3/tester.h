// ============================================================
//  tester.h  -  execucao dos vetores (mesma semantica da v2.6)
//
//  V/G  alimentacao       0/1  entrada forcada baixo/alto
//  L/H  saida esperada    X    "nao importa" em entrada (forca baixo)
//  Z    saida esperada alta (igual H, pullup)
//  C    pulso de clock via pullup (como na v2.6, para os vetores antigos)
//  -    pino solto com pullup, NAO conferido (novo na v3)
//  Linha 'DRAMxxxx' no lugar dos vetores = teste de DRAM (dram.h)
// ============================================================
#pragma once

bool errPin[20];

enum TestResult { T_GOOD, T_BAD, T_NOVEC, T_ERROR, T_DRAM };
char dramId[8] = "";      // preenchido quando o CI usa o teste de DRAM

static bool runVector(const char *t, uint8_t n, bool collectErr) {
  bool ok = true;
  uint8_t clockPins[20], clocks = 0;
  // fase 1: alimentacao e solta as saidas
  for (uint8_t i = 0; i < n; i++) {
    uint8_t p = megaPin(n, i);
    switch (t[i]) {
      case 'V': pinMode(p, OUTPUT); digitalWrite(p, HIGH); break;
      case 'G': pinMode(p, OUTPUT); digitalWrite(p, LOW);  break;
      case 'L': case 'H': case 'Z': case '-':
        digitalWrite(p, LOW); pinMode(p, INPUT_PULLUP); break;
    }
  }
  delay(5);
  // fase 2: entradas
  for (uint8_t i = 0; i < n; i++) {
    uint8_t p = megaPin(n, i);
    switch (t[i]) {
      case 'X': case '0': pinMode(p, OUTPUT); digitalWrite(p, LOW);  break;
      case '1':           pinMode(p, OUTPUT); digitalWrite(p, HIGH); break;
      case 'C': clockPins[clocks++] = p; pinMode(p, OUTPUT); digitalWrite(p, LOW); break;
    }
  }
  // fase 3: pulsos de clock
  for (uint8_t i = 0; i < clocks; i++) {
    pinMode(clockPins[i], INPUT_PULLUP); delay(10);
    pinMode(clockPins[i], OUTPUT); digitalWrite(clockPins[i], LOW);
  }
  delay(5);
  // fase 4: leitura
  for (uint8_t i = 0; i < n; i++) {
    char c = t[i];
    if (c != 'L' && c != 'H' && c != 'Z') continue;
    bool hi = digitalRead(megaPin(n, i));
    bool bad = (c == 'L') ? hi : !hi;
    if (bad) { ok = false; if (collectErr) errPin[i] = true; }
  }
  return ok;
}

// Testa o CI 'idx'. stopOnFail=true no modo Auto (mais rapido).
TestResult testChip(uint16_t idx, bool stopOnFail) {
  for (uint8_t i = 0; i < 20; i++) errPin[i] = false;
  if (!dbOpenChip(idx)) return T_ERROR;
  char line[24];
  bool ok = true, any = false;
  uint8_t n = curPins;
  zifRelease(); delay(5);
  while (true) {
    int len = readLine(dbFile, line, sizeof(line));
    if (len < 0 || line[0] == '$') break;
    if (len == 0) continue;
    if (strcmp(line, "NOVECTORS") == 0) { dbClose(); zifRelease(); return T_NOVEC; }
    if (strncmp(line, "DRAM", 4) == 0) {          // teste especial (dram.h)
      strncpy(dramId, line + 4, sizeof(dramId) - 1); dramId[sizeof(dramId) - 1] = 0;
      dbClose(); zifRelease();
      return stopOnFail ? T_NOVEC : T_DRAM;       // no Auto, DRAM e pulada
    }
    if (len != n) continue;                      // linha malformada: ignora
    any = true;
    if (!runVector(line, n, !stopOnFail)) { ok = false; if (stopOnFail) break; }
  }
  dbClose();
  zifRelease();                                  // desliga o CI ao final
  if (!any) return T_NOVEC;
  return ok ? T_GOOD : T_BAD;
}
