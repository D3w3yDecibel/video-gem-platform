// =====================================================================
// VIDEO GEM — Main sketch
// Multi-program visualizer platform
//
// Structure:
//   types.h           — shared defines, structs, pin assignments
//   core_input.ino    — pot scanning, keypad, program/preset switching
//   core_math.ino     — sin table, fast angle
//   core_time.ino     — normalizedTime, pingPongTime, LFO pool
//   core_draw.ino     — drawPoly, drawStarShape, fadeScreen
//   core_info.ino     — info overlay, contextual hints, program cards
//   core_fx.ino       — global FX layer (palette/pre-draw/post-draw effects)
//   core_palette.ino  — palette build/apply/cycle
//   core_registry.ino — PROG_ENTRY macro, progForSlot(), g_numPrograms
//   prog_registry.ino — program table: g_programs[] (stub; overlaid by merge)
//   prog_*.ino        — one file per program (from programs/ when merged)
//   VideoGem.ino      — this file: setup(), loop(), program router
//
// Control model:
//   k0–k11            preset select (0–11, or 12–23 with Shift)
//   k12               modifier A (program-defined); with Control held: FX mode toggle
//   k13               Info — hold=full overlay, double-tap=toggle hints
//   k14               Control — with Shift + k0..k11: program switch
//   k15               Shift — with k0–k11: presets 12–23
//   p0–p3             global: palette, speed, trail, intensity
//   p4–p15            per-preset (program-defined)
// =====================================================================

#include "types.h"
#ifndef UNIT_TEST
#include "hardware/watchdog.h"
#endif

DVIGFX8 display(DVI_RES_320x240p60, true, adafruit_feather_dvi_cfg);

// =====================================================================
// SETUP
// =====================================================================

void setup() {
  if (!display.begin()) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  }

  keypad.begin();
  pinMode(MUX_CTRL_0, OUTPUT);
  pinMode(MUX_CTRL_1, OUTPUT);

  initSinTable();
  buildPalette(0);
  applyPalette(0);
  display.swap(false, true);

#ifndef UNIT_TEST
  watchdog_enable(500, true);
#endif
}

// =====================================================================
// MAIN LOOP
// =====================================================================

void loop() {
  // ─── Input ──────────────────────────────────────────────────────
  keypad.tick();
  handleKeypad();
  updateK13Gesture();

  if (millis() > nextPotUpdateTime) {
    updatePots();
    nextPotUpdateTime = millis() + potIntervalMS;
  }

  // ─── Program lookup + init on switch ─────────────────────────────
  ProgEntry* prog = progForSlot(activeProgram);
  static int prevActiveProgram = -1;
  if (activeProgram != prevActiveProgram) {
    // Restore the platform palette: a program that sets its own colors
    // (PROG_FLAG_OWNS_GLOBALS) would otherwise leave them behind.
    buildPalette(currentPalette);
    if (prog && prog->init) prog->init();
    prevActiveProgram = activeProgram;
  }

  // ─── Globals from p0–p3 (skipped when program owns globals) ────
  int trailAmt = 20;
  if (!prog || !(prog->flags & PROG_FLAG_OWNS_GLOBALS)) {
    int newPal = potMap(0, 0, NUM_PALETTES - 1);
    if (newPal != currentPalette) {
      currentPalette = newPal;
      buildPalette(currentPalette);
    }
    float timeScale = potMap(1, 2, 30) / 10.0f;
    globalTime = millis() / 1000.0f * timeScale;
    trailAmt = potMap(2, 2, 50);
    int palSpeed = potMap(3, 0, 12);
    paletteOffset = (paletteOffset + palSpeed) % PAL_ENTRIES;
    applyPalette(paletteOffset);
  }

  applyPaletteFX();

  updateLFOs();

  // ─── k13 hold → full info overlay (replaces visualization) ─────
  if (k13Holding) {
    drawInfoOverlay();
    display.swap(true, true);
#ifndef UNIT_TEST
    watchdog_update();
#endif
    return;
  }

  // ─── Hints: update change detection ─────────────────────────────
  if (hintsEnabled) {
    updateHints();
    if (hintsWantFullScreen()) {
      drawHints();
      display.swap(true, true);
#ifndef UNIT_TEST
      watchdog_update();
#endif
      return;
    }
  }

  // ─── Frame preparation and draw (table-driven) ──────────────────
  uint8_t hint = RENDER_CLEAR;

  if (prog) {
    hint = prog->renderHint(activePreset);
  }

  if (hint == RENDER_TRAIL) {
    fadeScreen(trailAmt);
  } else if (hint == RENDER_CLEAR) {
    display.fillScreen(0);
  }
  // RENDER_PERPIXEL: no clear — program overwrites every pixel

  applyPreDrawFX();

  if (prog) {
    prog->draw(activePreset);
  } else {
    display.fillScreen(0);
    display.setCursor(40, 100);
    display.setTextColor(255);
    display.setTextSize(2);
    display.print("NO PROGRAM ");
    display.print(activeProgram);
  }

  applyPostDrawFX();

  // ─── Hints: draw bottom bar / indicator over the visualization ──
  if (hintsEnabled) {
    drawHints();
  }

  // ─── Swap ───────────────────────────────────────────────────────
  display.swap(true, true);
#ifndef UNIT_TEST
  watchdog_update();
#endif
}
