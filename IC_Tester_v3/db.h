// ============================================================
//  db.h  -  leitura do chipdb.txt com indice em RAM
//  Formato do arquivo igual ao da v2.6 (compativel)
// ============================================================
#pragma once

struct ChipEntry { uint32_t num; uint32_t posPins; };   // pos: 24 bits | pinos: 8 bits
ChipEntry chips[MAX_CHIPS];
uint16_t  chipCount = 0;
bool      dbOverflow = false;
char      dbVersion[24] = "";
uint32_t  sizesMask = 0;          // bit n = existe CI de n pinos

inline uint32_t entPos(uint16_t i)  { return chips[i].posPins & 0xFFFFFFUL; }
inline uint8_t  entPins(uint16_t i) { return chips[i].posPins >> 24; }

File dbFile;

// Le uma linha (sem \r\n e espacos nas pontas). Retorna comprimento, -1 em EOF.
int readLine(File &f, char *buf, int sz) {
  if (!f.available()) return -1;
  int n = 0, c;
  while ((c = f.read()) >= 0 && c != '\n') {
    if (c == '\r') continue;
    if (n < sz - 1) buf[n++] = (char)c;
  }
  buf[n] = 0;
  // trim
  int s = 0; while (buf[s] == ' ' || buf[s] == '\t') s++;
  while (n > s && (buf[n-1] == ' ' || buf[n-1] == '\t')) buf[--n] = 0;
  if (s) memmove(buf, buf + s, n - s + 1);
  return n - s;
}

bool isNumber(const char *s) {
  if (!*s) return false;
  for (; *s; s++) if (*s < '0' || *s > '9') return false;
  return true;
}

// Constroi o indice. Retorna false se nao abriu o arquivo.
bool dbBuildIndex() {
  chipCount = 0; dbOverflow = false; sizesMask = 0;
  File f = SD.open(DB_FILE);
  if (!f) return false;
  char line[32];
  // versao: primeira linha, so caracteres ASCII imprimiveis
  if (readLine(f, line, sizeof(line)) >= 0) {
    uint8_t j = 0;
    for (uint8_t i = 0; line[i] && j < sizeof(dbVersion) - 1; i++)
      if (line[i] >= 32 && line[i] < 127) dbVersion[j++] = line[i];
    dbVersion[j] = 0;
  }
  while (true) {
    int n = readLine(f, line, sizeof(line));
    if (n < 0) break;
    if (line[0] != '$' || !isNumber(line + 1)) continue;
    uint32_t num = strtoul(line + 1, NULL, 10);
    uint32_t pos = f.position();                 // inicio da linha do nome
    if (readLine(f, line, sizeof(line)) < 0) break;   // nome
    if (line[0] == '$') continue;
    if (readLine(f, line, sizeof(line)) < 0) break;   // pinos
    int pins = atoi(line);
    if (!(pins == 8 || pins == 14 || pins == 16 || pins == 18 || pins == 20)) continue;  // entrada malformada: ignora
    if (chipCount >= MAX_CHIPS) { dbOverflow = true; break; }
    chips[chipCount].num = num;
    chips[chipCount].posPins = (pos & 0xFFFFFFUL) | ((uint32_t)pins << 24);
    chipCount++;
    sizesMask |= (1UL << pins);
  }
  f.close();
  return true;
}

int16_t dbFind(uint32_t num) {
  for (uint16_t i = 0; i < chipCount; i++) if (chips[i].num == num) return i;
  return -1;
}

// ---- dados do CI carregado ----
char    curName[28] = "";
char    pinName[20][6];
uint8_t curPins = 0;

// Abre dbFile posicionado no primeiro vetor do CI 'idx' e carrega nome/pinos
bool dbOpenChip(uint16_t idx) {
  if (dbFile) dbFile.close();
  dbFile = SD.open(DB_FILE);
  if (!dbFile) return false;
  dbFile.seek(entPos(idx));
  char line[32];
  readLine(dbFile, line, sizeof(line));
  strncpy(curName, line, sizeof(curName) - 1); curName[sizeof(curName) - 1] = 0;
  readLine(dbFile, line, sizeof(line));
  curPins = entPins(idx);
  for (uint8_t i = 0; i < 20; i++) pinName[i][0] = 0;
  uint8_t i = 0;
  while (dbFile.available() && dbFile.peek() != '#' && i < curPins) {
    readLine(dbFile, line, sizeof(line));
    strncpy(pinName[i], line, 5); pinName[i][5] = 0;
    i++;
  }
  // pula ate a linha do '#'
  while (dbFile.available()) { readLine(dbFile, line, sizeof(line)); if (line[0] == '#') break; }
  return true;
}
void dbClose() { if (dbFile) dbFile.close(); }
