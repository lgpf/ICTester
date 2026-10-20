# IC Tester v3

Rewrite of the menu/test engine of IC Tester v2.6 (Frank Hellmann, GPL v3).
**Same hardware**: Arduino Mega 2560 + ILI9341 touch shield with SD + IC Tester PCB (20-pin ZIF).
5V parts only. Chips are always inserted **top-aligned** in the ZIF (pin 1 at ZIF pin 1).

## Install

1. Open `IC_Tester_v3/IC_Tester_v3.ino` in the Arduino IDE (board: Arduino Mega 2560).
2. Libraries (same as v2.6): MCUFRIEND_kbv, Adafruit GFX, Adafruit TouchScreen and
   `SD_patched` (from `SD_patched.zip` in the repository root).
3. Put your touch calibration in `config.h` (values from `TouchScreen_Calibr_native.ino`).
4. Copy `data/chipdb.txt` to the **root** of the SD card (FAT/FAT32, up to 4 GB).

The v3 database must be used with the v3 firmware (it contains 8-pin and DRAM entries
that v2.6 cannot handle).

## What changed

**User interface**
- Touch fires on release: no more repeated/ghost presses (Pins skipping, double tests).
- No blocking loops: Auto results are a list; tap the chip you want and Retest uses it.
- Keypad with Delete and Cancel.
- Pass/fail counter for batches when using Retest.
- Pins only offers sizes present in the database (8, 14, 16, 18, 20).

**Engine**
- Database index in RAM at boot: Auto and Manual are much faster.
- Socket is powered off (all pins released) after every test.
- 8-pin support (top of ZIF: positions 1-4 and 17-20).
- New vector code `-` = released with pull-up, not checked.
- SD errors show a retry screen instead of a jump to address 0.
- Auto keeps at most 24 matches (the v2.6 array could overflow).

**DRAM test** (`dram.h`, Manual mode only)
- 4164, 41256 (16 pins), 4464/41464 (18 pins), 44256 (20 pins).
- March C- over every cell, plus a 0101/1010 pass on x4 parts.
- Direct port access; row-fastest order keeps refresh within datasheet limits
  (row sweep 1.5 ms / 3.0 ms / 2.6 ms / 5.3 ms).
- Quick sanity check first: "No response" usually means wrong position or not enough VCC.
- VCC comes from a Mega I/O pin; NMOS DRAMs draw more than that pin is rated for.
  If a known-good chip fails, add a 5V wire + 100 nF to the VCC position of the ZIF.

## Database (`data/chipdb.txt`, version 3.1)

389 entries (v2.6: 324 entries, 317 distinct chips).

**Fixes to v2.6 entries**
- 4052, 7453: missing pin-count line. 7490: missing description.
- 4584, 7473: declared as 16 pins, they are 14.
- 7443: missing `#` separator. 74131: is a 16-pin 3-to-8 decoder, not a quad AND.
- 74378: had the 74377 header and pinout; rebuilt.
- 7431: inconsistent entry, disabled (NOVECTORS) until checked against a datasheet.
- Pin names fixed: 74122, 74152, 7418, 74293, 74589, 74597, 7495.
- Duplicates removed: 40161, 40162, 40174, 40175, 40192, 40193, 74246.
  The second 74425 is the 74426 (active-high enables) and was renamed.
- 25 chips used `X` (driven LOW) on output pins; changed to `-`.

**New chips with test vectors**
74169, 74377, 74379, 74594, 7483, 7491, 74121 (static), 4050, 4516, 2114 (SRAM),
LM393, LM339, LM358, LM324 (as comparators), NE555, NE556 (static), 75176,
MC1489, MAX232 (receivers), ULN2003, ULN2803, 75451-75454.
Enter numbers only in Manual mode: 393, 339, 358, 324, 555, 556, 1489, 232, 2003, 2803.

**Equivalents** (same pinout and vectors): 74HC4xxx (744017, 744040, 744051...),
2901-2904, 485, 75189, 202, 74596, 74247, 747266, 40194, 74221, 41464.

**Pinout only** (NOVECTORS): 4046, 4047, 4060, 4528, 4538 and their 74HC versions.

**DRAM**: 4164, 41256, 4464, 44256 use a `DRAMxxxx` line instead of vectors.

New entries were generated with the simulator in `tools/chipdb_generator` and have not
all been checked on real hardware yet. Test each one with a known-good chip first.

## Generating vectors

`tools/chipdb_generator` contains a Python simulator that reproduces `testCase()`
exactly (pin order, pull-up clock pulse). It was validated against existing vectors
(7400, 7474, 74160, 74163, 74174, 74175, 74245, 74273, 4049, 4510).
Write a logic model and a stimulus sequence in `newchips.py`, then run
`python3 newchips.py` to print database blocks ready to paste.
