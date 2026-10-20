// ============================================================
//  zif.h  -  mapeamento do soquete ZIF para os pinos do Mega
//  Todos os CIs ficam encostados no TOPO do ZIF (pino 1 no pino 1).
//  ZIF 1..10 = 30,32..48   ZIF 11..20 = 49,47..31
// ============================================================
#pragma once
const uint8_t ZIF[20] = {30,32,34,36,38,40,42,44,46,48,49,47,45,43,41,39,37,35,33,31};

// Pino 'k' (0-based) de um CI de 'n' pinos -> pino do ZIF (0-based)
inline uint8_t zifIndex(uint8_t n, uint8_t k) {
  uint8_t half = n / 2;
  return (k < half) ? k : (20 - n + k);
}
inline uint8_t megaPin(uint8_t n, uint8_t k) { return ZIF[zifIndex(n, k)]; }

// Solta todos os pinos (sem pullup): CI sem alimentacao, seguro para trocar
void zifRelease() {
  for (uint8_t i = 0; i < 20; i++) { pinMode(ZIF[i], INPUT); digitalWrite(ZIF[i], LOW); }
}
