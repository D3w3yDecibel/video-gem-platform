// =====================================================================
// PROGRAM: MORPH — clay shapes changing shape (by Dewey) — Set 5, slot 10
//
// The clay field again, but here it is the shapes themselves that cycle:
// every cube melts into a cylinder and back, tubes open and close, shapes
// swell and shrink, clay changes colour — in waves, ripples and spirals
// that roll across the field, or glide past on a conveyor. The renderer
// (vg5c.h) draws any shape between round and square, so the change is
// smooth.
//
// Presets:
//   k0 Morph Wave   — a diagonal wave turns squares round, red and white
//   k1 Breathing    — shapes swell and shrink from the middle out; holes open as they shrink
//   k2 Conveyor     — the field glides past, each shape changing as it goes
//   k3 Checker Flip — neighbours swap shape and colour, in a beat
//   k4 Ripple Holes — rings of tubes opening and closing
//   k5 Spiral       — a turning spiral of round and square
//   k6 Halftone     — flat round dots swelling in waves, like a printed screen
//   k7 Pastel Morph — soft colours, slow
//   k8 Plaster      — all white
//   k9 Flicker      — every shape on its own random timing
//   k10 Lava        — black and orange, slow noise in height and shape
//   k11 Tour        — a new preset every 20 seconds
//
//   k12 — FLIP: every shape snaps the other way and settles back
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = freeze)   p2 Pattern (centre = preset)
//   p3 Morph Depth   p4 Wavelength   p5 Height   p6 Height Wave   p7 Hole Wave   p8 Size Wave
//   p9 Cell Size   p10 Colour Flip   p11 Speckles
//   p12-p15 are Set 5's live FX knobs (vg5fx.h), off in the centre band:
//   p12 Warp (◄ ripple | zoom ►)   p13 Mirror (◄ mirror | tiles ►)   p14 Colour (◄ cycle | flash ►)
//   p15 Glitch (◄ tear | pixels ►)
// =====================================================================

#include "vg5c.h"

enum { MO5_DIAG, MO5_RADIAL, MO5_ROWS, MO5_CHECK, MO5_SPIRAL, MO5_NOISE, MO5_RANDOM, MO5_NPAT };
struct Mo5Preset { C5Look look; uint8_t pattern, depth, wlen, height, hWave, holeWave, sizeWave, cell, flip, mixB, drift, speck; };
static const Mo5Preset mo5_pre[11] = {
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, MO5_DIAG, 255, 60, 26, 30, 0, 0, 26, 0, 110, 0, 40},
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, MO5_RADIAL, 120, 50, 26, 0, 255, 255, 26, 0, 110, 0, 40},
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, MO5_ROWS, 255, 50, 30, 60, 120, 0, 26, 0, 110, 120, 40},
  {{{140,140,150},{250,250,250},{20,20,24},{90,90,100},{230,40,50},{6,6,8}}, MO5_CHECK, 255, 60, 26, 40, 0, 0, 26, 255, 128, 0, 30},
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, MO5_RADIAL, 60, 40, 26, 0, 255, 0, 26, 0, 110, 0, 40},
  {{{140,150,180},{245,248,255},{10,30,110},{60,120,255},{40,90,230},{10,14,30}}, MO5_SPIRAL, 255, 50, 26, 30, 80, 0, 26, 120, 110, 0, 40},
  {{{8,8,12},{60,60,70},{90,10,20},{230,50,60},{230,40,50},{246,242,232}}, MO5_DIAG, 0, 70, 2, 0, 0, 255, 22, 0, 50, 40, 0},
  {{{120,160,150},{220,255,240},{140,80,60},{255,200,170},{255,140,120},{30,40,40}}, MO5_NOISE, 255, 80, 30, 60, 100, 60, 26, 80, 110, 0, 30},
  {{{140,140,140},{252,252,250},{140,140,140},{252,252,250},{200,200,200},{20,20,24}}, MO5_DIAG, 255, 60, 26, 30, 100, 0, 26, 0, 0, 0, 0},
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, MO5_RANDOM, 255, 60, 26, 40, 120, 60, 26, 120, 110, 0, 40},
  {{{20,16,14},{70,60,56},{130,30,0},{255,140,30},{255,200,80},{4,2,2}}, MO5_NOISE, 255, 90, 40, 120, 60, 0, 24, 160, 110, 40, 20},
};
static const char* const mo5_names[12] = {
  "Morph Wave", "Breathing", "Conveyor", "Checker Flip", "Ripple Holes", "Spiral", "Halftone", "Pastel Morph", "Plaster", "Flicker", "Lava", "Tour"
};
static const char* const mo5_labels[16] = {
  "Palette", "Speed", "Pattern", "Morph Depth", "Wavelength", "Height", "Height Wave", "Hole Wave",
  "Size Wave", "Cell Size", "Colour Flip", "Speckles", "Light Side", "Drift", "Colour Mix", "Gap"
};
const char* prog_morph_name() { return "MORPH"; }
const char* prog_morph_character() { return "Clay cubes melting into cylinders, tubes opening and shapes swelling in waves across the field"; }
const char* prog_morph_presetName(int preset) { return (preset >= 0 && preset < 12) ? mo5_names[preset] : NULL; }
const char* prog_morph_potLabel(int preset, int pot) { (void)preset; if (pot >= 12 && pot < 16) return fx5_labels[pot - 12]; return (pot >= 0 && pot < 16) ? mo5_labels[pot] : ""; }
uint8_t prog_morph_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

static int mo5_preset = -1, mo5_look = 0;
static float mo5_t = 0, mo5_tour = 0, mo5_di = 0, mo5_flip = 0;
static bool mo5_k12 = false;
static int mo5_pat, mo5_depth, mo5_wl, mo5_height, mo5_hWave, mo5_holeWave, mo5_sizeWave, mo5_flipC, mo5_mixB, mo5_gap, mo5_ti, mo5_oi, mo5_flipAmt;

// the pattern's value at a cell: −16384 … 16384
static int mo5_phase(int i, int j, int extra) {
  int t = mo5_ti + extra, wl = mo5_wl > 2 ? mo5_wl : 2;
  switch (mo5_pat) {
    case MO5_DIAG: return v3_sin((i + j) * 1024 / wl - t);
    case MO5_RADIAL: { int r = (int)v3_isqrt((uint32_t)(i * i + j * j) * 256); return v3_sin(r * 64 / wl - t); }
    case MO5_ROWS: return v3_sin(i * 2048 / wl - t);
    case MO5_CHECK: { int s = v3_sin(t / 2); return ((i + j) & 1) ? s : -s; }
    case MO5_SPIRAL: { int r = (int)v3_isqrt((uint32_t)(i * i + j * j) * 256); return v3_sin(r * 64 / wl + v3_atan(j, i) * 2 - t); }
    case MO5_NOISE: return v3_noise1((uint32_t)((i * 3 + j * 5) * 256 / wl + t / 3), 7u + (uint32_t)(j * 131 + i * 977));
    default: { uint32_t h = v3_hash(((uint32_t)(i) * 73856093u) ^ ((uint32_t)(j) * 19349663u)); return v3_sin((int)(h & 1023) + t * (1 + (int)((h >> 10) & 3)) / 2); }
  }
}
static void mo5_cell(int i, int j, void* cv) {
  C5Cell* c = (C5Cell*)cv;
  int wi = i + mo5_oi, wj = j;
  uint32_t h = v3_hash(((uint32_t)(wi) * 73856093u) ^ ((uint32_t)(wj) * 19349663u) ^ 0x33u);
  c->seed = h;
  int a = mo5_phase(wi, wj, 0);                                       // shape
  int b = mo5_phase(wi, wj, 341);                                     // holes and size, a third of a turn behind
  int m = ((a + 16384) >> 1) * mo5_depth >> 8;                      // how far along its change this shape is: 0 … 16384
  bool startsRound = mo5_look == 6 ? true : (h & 1);                // halftone: all dots
  int sq = startsRound ? m : 16384 - m;                             // 0 … 16384: round … square
  sq += (mo5_flipAmt * (startsRound ? 1 : -1)) / 2;
  int L = v3_clamp(sq * 8 / 16384, 0, 7);
  c->L = L;
  c->h = mo5_height + ((v3_sin((wi + wj) * 120 - mo5_ti / 2) + a) * mo5_hWave >> 15);
  if (c->h < 1) c->h = 1;
  int hole = (b * mo5_holeWave >> 14) + 40; c->hole = hole > 60 ? (hole > 190 ? 190 : hole) : 0;
  int sz = 230 - mo5_gap + ((b * mo5_sizeWave >> 14) * 70 >> 8); c->scale = v3_clamp(sz, 70, 256);
  if (mo5_sizeWave > 0 && mo5_holeWave > 0 && c->scale < 170) c->hole = 120;   // breathing: small ones open up
  int clay = (int)((h >> 12) & 255) < mo5_mixB ? 1 : 0;
  if (mo5_flipC > 0 && a * mo5_flipC / 255 > 6000) clay ^= 1;           // colour flips with the wave
  c->clay = clay;
}
void prog_morph_init() { v3_reset(); mo5_preset = -1; c5_S = 0; }

static void morph5_draw(int preset) {
  v3_buf = c5_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame();
  fx5_take();                                                      // p12-p15: the live FX knobs
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != mo5_preset) { mo5_preset = preset; mo5_look = preset == 11 ? 0 : preset; mo5_tour = 0; }
  if (preset == 11) { mo5_tour += v3_dt; if (mo5_tour > 20.0f) { mo5_tour = 0; mo5_look = (mo5_look + 1) % 11; } }
  const Mo5Preset& P = mo5_pre[mo5_look];
  const Mo5Preset& Q = mo5_pre[v3_centre(0) ? mo5_look : v3_clamp(v3_k(0) * 11 / 1024, 0, 10)];

  float speed = v3_speed(1, 3.0f);
  mo5_pat = v3_centre(2) ? P.pattern : v3_clamp(v3_k(2) * MO5_NPAT / 1024, 0, MO5_NPAT - 1);
  mo5_depth = v3_centre(3) ? P.depth : (int)(v3_u(3) * 255);
  mo5_wl = (int)v3_rel(4, P.wlen / 10.0f, 3.0f);
  mo5_height = (int)v3_rel(5, P.height, 2.5f);
  mo5_hWave = v3_centre(6) ? P.hWave : (int)(v3_u(6) * 120);
  mo5_holeWave = v3_centre(7) ? P.holeWave : (int)(v3_u(7) * 255);
  mo5_sizeWave = v3_centre(8) ? P.sizeWave : (int)(v3_u(8) * 255);
  int S = v3_centre(9) ? P.cell : 12 + (int)(v3_u(9) * 24); S &= ~1;
  mo5_flipC = v3_centre(10) ? P.flip : (int)(v3_u(10) * 255);
  c5_speckles = v3_centre(11) ? Q.speck : (int)(v3_u(11) * 200);
  c5_light = v3_centre(12) ? 1 : (v3_k(12) < 512 ? -1 : 1);
  float drift = v3_centre(13) ? P.drift / 100.0f : v3_bi(13) * 1.5f;
  mo5_mixB = v3_centre(14) ? Q.mixB : (int)(v3_u(14) * 255);
  mo5_gap = v3_centre(15) ? 0 : (int)(v3_bi(15) * 80);
  c5_profiles(S);
  c5_palette(Q.look);

  bool k12 = keysPressed[KEY_MOD_A]; if (k12 && !mo5_k12) mo5_flip = 1.0f; mo5_k12 = k12;
  float dt = v3_dt * speed;
  mo5_t += dt; if (mo5_t > 10000) mo5_t = 0;
  mo5_ti = (int)(mo5_t * 260);
  mo5_flip -= v3_dt * 0.8f; if (mo5_flip < 0) mo5_flip = 0;
  mo5_flipAmt = (int)(mo5_flip * mo5_flip * 30000);
  mo5_di += drift * dt;
  mo5_oi = (int)floorf(mo5_di);
  int fi = (int)((mo5_di - mo5_oi) * 256);
  c5_cx = 160 - ((fi * c5_S) >> 8); c5_top = 110 - ((fi * (c5_S / 2)) >> 8);

  memset(v3_buf, mo5_look == 6 && v3_centre(0) ? 207 : 196, W * H);   // the gaps (halftone: the paper)
  c5_field(mo5_height + mo5_hWave + 20, mo5_cell, 0);
}
void prog_morph_draw(int preset) { morph5_draw(preset); fx5_apply(); }
