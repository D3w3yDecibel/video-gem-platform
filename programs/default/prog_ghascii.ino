// =====================================================================
// PROGRAM: ASCII — a little universe drawn in letters (by Dewey) — Set 8 slot 5
//
// Replaces RIBBONS. After the ASCII-art films Dewey sent (a rocket launch
// that keeps going — ignition, lift-off, second stage, orbit, the solar
// system, the Milky Way — an ASCII jellyfish, and a p5.js grid of neon
// glyph shapes). Each scene is lit and shaded as a field of light on a grid
// of character cells; each cell then becomes a letter picked by how much
// ink it has (" .:-i+=coxv%nO&zkhZX$dbB#@0M8"), outlines become / \ | _
// where the light changes sharply, and a status read-out counts the
// letters like the original.
//
// Presets: k0 Journey (pad → lift-off → second stage → orbit → the Moon → the solar system →
//          Saturn → a nebula → the Milky Way, ~6 s a stage) · k1 Launch · k2 Earth & Moon ·
//          k3 Planets · k4 Deep Space · k5 Jellyfish · k6 Jelly Bloom · k7 Aurora · k8 Glyph Grid ·
//          k9 Glyph Journey (the trip in neon shapes) · k10 Bit Universe (0s and 1s) · k11 Tour
//   k12 — NEXT stage · PULSE (jellyfish) · SURGE (aurora) · RIPPLE (glyph grid)
//
// Knobs: p0 Palette (centre = natural colours) · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Letter Size · p6 Charset (letters / classic / binary / shapes / blocks) · p7 Scene (hold one) ·
//   p8 Outlines · p9 Pace (seconds a stage) · p10 Read-out (off | on)
//   p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8AS_IGNITION, H8AS_LIFTOFF, H8AS_STAGE2, H8AS_ORBIT, H8AS_EARTHRISE, H8AS_SOLAR, H8AS_SATURN, H8AS_NEBULA, H8AS_GALAXY,
       H8AS_AURORA, H8AS_JELLY, H8AS_BLOOM, H8AS_GRID, H8AS_NSCENE };
enum { H8AS_LETTERS, H8AS_CLASSIC, H8AS_BINARY, H8AS_SHAPES, H8AS_BLOCKS, H8AS_NCHAR };
enum { H8AS_HOLD, H8AS_RUN };
// each look plays a run of scenes: first … first + count − 1 (one scene = held). slow: stages last 2.5 × Pace
struct H8AsLook { uint8_t first, count, slow, charset, size, outline, hud; };
static const H8AsLook h8as_looks[11] = {
  // first           count slow charset        size outl hud
  { H8AS_IGNITION,     9,  0, H8AS_LETTERS,  0,  55,  1 },   // Journey: pad → orbit → Moon → planets → nebula → Milky Way
  { H8AS_IGNITION,     3,  0, H8AS_LETTERS,  0,  55,  1 },   // Launch
  { H8AS_ORBIT,        2,  1, H8AS_LETTERS,  0,  50,  1 },   // Earth & Moon
  { H8AS_SOLAR,        2,  1, H8AS_LETTERS,  0,  55,  1 },   // Planets
  { H8AS_NEBULA,       2,  1, H8AS_LETTERS,  0,  35,  1 },   // Deep Space
  { H8AS_JELLY,        1,  0, H8AS_LETTERS,  0,  55,  0 },   // Jellyfish
  { H8AS_BLOOM,        1,  0, H8AS_LETTERS,  1,  50,  0 },   // Jelly Bloom
  { H8AS_AURORA,       1,  0, H8AS_LETTERS,  0,  40,  0 },   // Aurora
  { H8AS_GRID,         1,  0, H8AS_SHAPES,   1,   0,  0 },   // Glyph Grid
  { H8AS_IGNITION,     9,  0, H8AS_SHAPES,   0,  45,  1 },   // Glyph Journey
  { H8AS_IGNITION,     9,  0, H8AS_BINARY,   0,  40,  1 },   // Bit Universe
};
static const char* const h8as_names[12] = { "Journey", "Launch", "Earth & Moon", "Planets", "Deep Space", "Jellyfish", "Jelly Bloom", "Aurora", "Glyph Grid", "Glyph Journey", "Bit Universe", "Tour" };
static const char* const h8as_lbl[6] = { "Letter Size", "Charset", "Scene", "Outlines", "Pace", "Read-out" };
const char* prog_ghascii_name() { return "ASCII"; }
const char* prog_ghascii_character() { return "ASCII: a rocket launch to the Milky Way, the Moon, Saturn, a nebula, an aurora, jellyfish and neon glyphs, in letters"; }
const char* prog_ghascii_presetName(int p) { return (p >= 0 && p < 12) ? h8as_names[p] : NULL; }
const char* prog_ghascii_potLabel(int p, int pot) { (void)p; return v8_label(pot, h8as_lbl); }
uint8_t prog_ghascii_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// ─── glyphs: a 3×5 font, sorted by ink (codes 0-28), then outline and extra marks ───
static const char h8as_chars[] = " .:-i+=coxv%nO&zkhZX$dbB#@0M8/\\|_*()~1";
static const uint8_t h8as_g3[][5] = {
  {0,0,0,0,0},{0,0,0,0,2},{0,2,0,2,0},{0,0,7,0,0},{2,0,2,2,2},{0,2,7,2,0},{0,7,0,7,0},{0,3,4,4,3},{0,2,5,5,2},{0,5,2,2,5},
  {0,5,5,5,2},{5,1,2,4,5},{0,6,5,5,5},{2,5,5,5,2},{2,5,2,5,3},{0,7,1,2,7},{4,5,6,5,5},{4,4,7,5,5},{7,1,2,4,7},{5,5,2,5,5},
  {3,6,2,3,6},{1,1,7,5,7},{4,4,7,5,7},{6,5,6,5,6},{5,7,5,7,5},{7,5,7,4,7},{7,5,5,5,7},{5,7,7,5,5},{7,5,7,5,7},
  {1,1,2,4,4},{4,4,2,1,1},{2,2,2,2,2},{0,0,0,0,7},{5,2,7,2,5},{1,2,2,2,1},{4,2,2,2,4},{0,0,3,6,0},{2,6,2,2,7},
};
#define H8AS_RAMP 29
enum { H8AS_C_SP = 0, H8AS_C_DOT = 1, H8AS_C_DASH = 3, H8AS_C_ZERO = 26, H8AS_C_SLASH = 29, H8AS_C_BACK = 30, H8AS_C_BAR = 31,
       H8AS_C_UNDER = 32, H8AS_C_STAR = 33, H8AS_C_LP = 34, H8AS_C_RP = 35, H8AS_C_TILDE = 36, H8AS_C_ONE = 37 };
static const uint8_t h8as_classic[10] = { 0, 1, 2, 3, 6, 5, 33, 24, 11, 25 };          // " .:-=+*#%@"
// 5×7 forms for the marks the Set 8 font lacks (\ | ~ & $, and a real @ — Set 8's @ is a heart)
static const uint8_t h8as_g5x[6][7] = { {16,16,8,4,2,1,1}, {4,4,4,4,4,4,4}, {0,0,8,21,2,0,0}, {12,18,20,8,21,18,13}, {4,15,20,14,5,30,4}, {14,17,23,21,23,16,14} };
static const char h8as_x5[] = "\\|~&$@";

// ─── natural colours (p0 in the centre): the 16 hues the scenes paint with ───
enum { H8AS_WHITE, H8AS_CREAM, H8AS_YELLOW, H8AS_ORANGE, H8AS_RED, H8AS_MAROON, H8AS_BROWN, H8AS_OCEAN,
       H8AS_DEEP, H8AS_CYAN, H8AS_GREEN, H8AS_OLIVE, H8AS_MAGENTA, H8AS_PURPLE, H8AS_PINK, H8AS_GREY };
static const uint8_t h8as_nat[16][3] = {
  {200,225,255},{255,236,175},{255,228,  0},{255,118,  0},{255, 28,  8},{205, 18, 32},{195,100, 35},{  0,105,255},
  { 10, 40,215},{  0,230,255},{ 30,235, 40},{215,205, 30},{255, 25,220},{140, 45,255},{255,105,170},{150,168,190},
};
static void h8as_natural() {
  int crush = v8_key > 0 ? (int)(v8_key * 100) : 0, lift = v8_key < 0 ? (int)(-v8_key * 46) : 0;
  int kk[16];
  for (int l = 0; l < 16; l++) {
    int e = l * 16 - crush;
    if (e <= 0) { kk[l] = 0; continue; }
    e = e * 240 / (240 - crush);
    int i = e >> 4, f = e & 15;
    static int16_t br[16]; static bool brOK = false;                          // a brighter curve than the shared gamma: letters glow
    if (!brOK) { for (int q = 0; q < 16; q++) br[q] = (int16_t)(256.0f * powf(q / 15.0f, 0.75f)); brOK = true; }
    int g = i >= 15 ? br[15] : br[i] + (((br[i + 1] - br[i]) * f) >> 4);
    kk[l] = lift + (((256 - lift) * g) >> 8);
  }
  if (lift) kk[0] = lift / 2;
  int vv = (int)(v8_viv * 256), gain = 320 + (vv > 0 ? vv * 3 / 2 : vv);              // natural colours run extra vivid
  for (int h = 0; h < 16; h++) {
    int c[3]; for (int q = 0; q < 3; q++) c[q] = h8as_nat[h][q];
    int lum = (c[0] * 77 + c[1] * 150 + c[2] * 29) >> 8;
    for (int q = 0; q < 3; q++) c[q] = v3_clamp(lum + (((c[q] - lum) * gain) >> 8), 0, 255);
    for (int l = 0; l < 16; l++) {
      int k = kk[l], w = l == 15 ? 70 : 0;                                    // only the very brightest burn toward white
      int r = (c[0] * k) >> 8, g = (c[1] * k) >> 8, b = (c[2] * k) >> 8;
      if (w) { r += ((255 - r) * w) >> 8; g += ((255 - g) * w) >> 8; b += ((255 - b) * w) >> 8; }
      display.setColor((uint8_t)((l << 4) | h), (uint8_t)v3_clamp(r, 0, 255), (uint8_t)v3_clamp(g, 0, 255), (uint8_t)v3_clamp(b, 0, 255));
    }
  }
}

// ─── the cell grid ───
static int h8as_cw = 4, h8as_ch = 6, h8as_nx = 80, h8as_ny = 40, h8as_x0 = 0, h8as_y0 = 0;
#define H8AS_LEV V8_LEV                                   // light per cell 0 … 255
#define H8AS_HUE V8_HUE                                   // natural hue per cell 0 … 15
#define H8AS_GL  V8_EXTRA                                 // forced glyph: 0 = by light, 1 … 63 = a character code, 100 + n = shape n
#define H8AS_EL  (V8_EXTRA + V8_GN)                       // light for outline glyphs
static float h8as_t = 0, h8as_st = 0, h8as_ripT = -1, h8as_pulseT = -1;
static int h8as_scene = 0, h8as_hold = -1, h8as_lastLook = -1, h8as_letters = 0;

// value noise, coordinates in 1/256 of a lattice cell → 0 … 255
static inline int h8as_vn(int x, int y, uint32_t seed) {
  int xi = x >> 8, yi = y >> 8, fx = x & 255, fy = y & 255;
  fx = (fx * fx * (768 - 2 * fx)) >> 16; fy = (fy * fy * (768 - 2 * fy)) >> 16;
  uint32_t b = (uint32_t)xi * 374761393u + (uint32_t)yi * 668265263u + seed;
  int h00 = v3_hash(b) & 255, h10 = v3_hash(b + 374761393u) & 255, h01 = v3_hash(b + 668265263u) & 255, h11 = v3_hash(b + 374761393u + 668265263u) & 255;
  int a = h00 + (((h10 - h00) * fx) >> 8), c = h01 + (((h11 - h01) * fx) >> 8);
  return a + (((c - a) * fy) >> 8);
}
static inline int h8as_fbm(int x, int y, uint32_t seed) { return (h8as_vn(x, y, seed) * 2 + h8as_vn(x * 2 + 77, y * 2 + 31, seed + 9)) / 3; }
static inline void h8as_set(int c, int lev, int hue) { if (lev > H8AS_LEV[c]) { H8AS_LEV[c] = (uint8_t)v3_clamp(lev, 0, 255); H8AS_HUE[c] = (uint8_t)hue; } }
static inline void h8as_put(int c, int lev, int hue) { H8AS_LEV[c] = (uint8_t)v3_clamp(lev, 0, 255); H8AS_HUE[c] = (uint8_t)hue; }
static inline int h8as_cellAt(int px, int py) {
  int i = (px - h8as_x0) / h8as_cw, j = (py - h8as_y0) / h8as_ch;
  if (px < h8as_x0 || py < h8as_y0 || i >= h8as_nx || j >= h8as_ny) return -1;
  return j * h8as_nx + i;
}
// a mark (outline glyph) forced into the cell under (px, py)
static inline void h8as_mark(int px, int py, int code, int lev, int hue) {
  int c = h8as_cellAt(px, py); if (c < 0) return;
  H8AS_GL[c] = (uint8_t)code; H8AS_EL[c] = (uint8_t)v3_clamp(lev, 0, 255); H8AS_HUE[c] = (uint8_t)hue;
  if (H8AS_LEV[c] < lev) H8AS_LEV[c] = (uint8_t)v3_clamp(lev, 0, 255);
}
// slope → outline glyph: dx across per dy down (Q8)
static inline int h8as_slopeGlyph(int s) { return s > 110 ? H8AS_C_BACK : (s < -110 ? H8AS_C_SLASH : H8AS_C_BAR); }

// stars: some cells twinkle; a few big flares with rays
static void h8as_stars(int ox, int oy, int dens, int lev) {
  int ocx = ox / h8as_cw, ocy = oy / h8as_ch, tw = (int)(h8as_t * 3.0f);
  for (int j = 0; j < h8as_ny; j++) for (int i = 0; i < h8as_nx; i++) {
    uint32_t h = v3_hash((uint32_t)(i + ocx) * 2654435761u ^ (uint32_t)(j + ocy) * 40503u);
    if ((int)(h & 1023) < dens) {
      int l = lev / 3 + (int)((h >> 10) & 63) + ((v3_hash(h + tw) & 7) == 0 ? 50 : 0);
      h8as_set(j * h8as_nx + i, l, (h >> 20) & 3 ? H8AS_WHITE : H8AS_CYAN);
    }
  }
}
static void h8as_flare(int px, int py, int size, int lev) {
  h8as_mark(px, py, H8AS_C_STAR, lev, H8AS_WHITE);
  for (int k = 1; k <= size; k++) {
    int l = lev * (size + 1 - k) / (size + 1);
    h8as_mark(px + k * h8as_cw, py, H8AS_C_DASH, l, H8AS_WHITE); h8as_mark(px - k * h8as_cw, py, H8AS_C_DASH, l, H8AS_WHITE);
    h8as_mark(px, py + k * h8as_ch, H8AS_C_BAR, l, H8AS_WHITE);  h8as_mark(px, py - k * h8as_ch, H8AS_C_BAR, l, H8AS_WHITE);
    if (k < size) {
      h8as_mark(px + k * h8as_cw, py - k * h8as_ch, H8AS_C_SLASH, l * 2 / 3, H8AS_WHITE); h8as_mark(px - k * h8as_cw, py + k * h8as_ch, H8AS_C_SLASH, l * 2 / 3, H8AS_WHITE);
      h8as_mark(px + k * h8as_cw, py + k * h8as_ch, H8AS_C_BACK, l * 2 / 3, H8AS_WHITE);  h8as_mark(px - k * h8as_cw, py - k * h8as_ch, H8AS_C_BACK, l * 2 / 3, H8AS_WHITE);
    }
  }
}

// ─── the rocket, in its own frame: u across, v down from the nose (px at scale 1) ───
// returns light 0 … 255 or −1 (not rocket); hue out. stage2: only the upper stage
static inline int h8as_rocketAt(int u, int v, int stage2, int* hue) {
  int au = u < 0 ? -u : u;
  int tail = stage2 ? 120 : 186;
  int hw = 0, cu = 0, bodyHue = H8AS_CREAM;
  if (v >= 0 && v < 36) { if (au * au * 36 < 256 * v) hw = 16; }                            // the nose
  else if (v >= 36 && v < tail && au < 16) hw = 16;
  if (hw) cu = 0;
  else if (!stage2) {                                                                       // side boosters
    int bu = au - 25, abu = bu < 0 ? -bu : bu;
    if ((v >= 70 && v < 196 && abu < 7) || (v >= 56 && v < 70 && abu * abu * 14 < 49 * (v - 56))) { hw = 7; cu = u < 0 ? -25 : 25; bodyHue = H8AS_WHITE; }
    else if (v >= 196 && v < 204 && abu < 5) { *hue = H8AS_GREY; return 70; }              // booster nozzles
  }
  if (!hw) {
    if (v >= tail && v < tail + 10 && au < 10 + (v - tail) / 3) { *hue = H8AS_GREY; return 75; }   // engine bell
    return -1;
  }
  int n = ((u - cu) * 256) / hw; if (n > 255) n = 255; if (n < -255) n = -255;
  int sq = (int)v3_isqrt((uint32_t)(65536 - n * n));
  int d = (218 * sq - 128 * n) >> 8; if (d < 0) d = 0;                                       // lit from the upper left
  int lev = 30 + d * 215 / 256;
  if ((v >= 70 && v < 74) || (v >= 124 && v < 128) || (stage2 && v >= 100 && v < 104)) lev = lev * 2 / 5;   // bands
  if (!stage2 && v >= 116 && v < 124 && cu == 0) { lev = lev / 3; }                         // interstage
  *hue = bodyHue;
  return lev;
}
// the flame below the engine: dv = px below the nozzle, len = flame length
static inline int h8as_flameAt(int u, int dv, int len, int t8, int* hue) {
  if (dv < 0 || dv >= len) return -1;
  int hwf = 9 + dv / 8, au = u < 0 ? -u : u;
  if (au >= hwf) return -1;
  int f = dv * 256 / len, core = 256 - au * 256 / hwf;
  int fl = h8as_vn(u * 60 + 5000, dv * 40 - t8 * 30, 99) - 128;
  int lev = ((256 - f) * (140 + core / 2)) >> 8; lev += fl / 3;
  *hue = f < 50 && core > 140 ? H8AS_WHITE : (f < 100 ? H8AS_YELLOW : (f < 170 ? H8AS_ORANGE : H8AS_RED));
  return lev + 40;
}
// billowing smoke at the pad: lit red from the fire beneath
static inline int h8as_smokeAt(int px, int py, int groundY, int spread256, int t8, int* hue) {
  int dx = px - 160, dy = groundY - py;
  if (dy < -30) return -1;
  int reach = 60 + spread256 * 170 / 256, adx = dx < 0 ? -dx : dx;
  if (adx > reach) return -1;
  int r = adx * 256 / reach, env = (26 + spread256 * 64 / 256) * (65536 - r * r) >> 16;
  int sx = (dx * (256 - spread256 / 3)) >> 8;
  int n = h8as_fbm(sx * 7 + 9000, (py + t8 / 3) * 7 + 4000, 1234);
  int val = env * (150 + n) / 256 - dy;
  if (val <= 0) return -1;
  int lev = (val > 12 ? 255 : val * 21) * (90 + n) / 345 + 30;
  bool hot = adx < 40 + spread256 / 8 && dy < 45;
  *hue = hot ? H8AS_ORANGE : (n > 150 ? H8AS_RED : H8AS_MAROON);
  return lev;
}

// draw the rocket (and its flame) into the field. (rx, ry) = nose on screen, z256 = scale, ang = tilt (1024 = turn)
static void h8as_rocket(int rx, int ry, int z256, int ang, int stage2, int flameLen, int t8) {
  int cs = v3_cos(ang), sn = v3_sin(ang);
  int len = (stage2 ? 130 : 205) + flameLen;
  // bounding box on screen
  int ex = rx - (len * sn >> 14) * z256 / 256, ey = ry + (len * cs >> 14) * z256 / 256;
  int pad = 40 * z256 / 256 + 30;
  int xa = (rx < ex ? rx : ex) - pad, xb = (rx > ex ? rx : ex) + pad, ya = (ry < ey ? ry : ey) - pad, yb = (ry > ey ? ry : ey) + pad;
  int i0 = v3_clamp((xa - h8as_x0) / h8as_cw, 0, h8as_nx - 1), i1 = v3_clamp((xb - h8as_x0) / h8as_cw, 0, h8as_nx - 1);
  int j0 = v3_clamp((ya - h8as_y0) / h8as_ch, 0, h8as_ny - 1), j1 = v3_clamp((yb - h8as_y0) / h8as_ch, 0, h8as_ny - 1);
  int iz = 65536 / (z256 > 8 ? z256 : 8);
  int tail = stage2 ? 130 : 196;
  for (int j = j0; j <= j1; j++) for (int i = i0; i <= i1; i++) {
    int dx = h8as_x0 + i * h8as_cw + h8as_cw / 2 - rx, dy = h8as_y0 + j * h8as_ch + h8as_ch / 2 - ry;
    int u = (((dx * cs + dy * sn) >> 14) * iz) >> 8, v = (((-dx * sn + dy * cs) >> 14) * iz) >> 8;
    int hue, c = j * h8as_nx + i;
    int lev = h8as_rocketAt(u, v, stage2, &hue);
    if (lev >= 0) { h8as_put(c, lev, hue); continue; }
    if (flameLen > 0) {
      lev = h8as_flameAt(u, v - tail, flameLen, t8, &hue);
      if (lev < 0 && !stage2) { int bu = u < 0 ? u + 25 : u - 25; if (u > 12 || u < -12) lev = h8as_flameAt(bu * 2, (v - 204) * 2, flameLen, t8, &hue); }
      if (lev > 0) h8as_set(c, lev, hue);
    }
  }
}

// ─── the scenes ───
#define H8AS_CELLS for (int j = 0; j < h8as_ny; j++) for (int i = 0, px = h8as_x0 + h8as_cw / 2, py = h8as_y0 + j * h8as_ch + h8as_ch / 2, c = j * h8as_nx; i < h8as_nx; i++, px += h8as_cw, c++)

static void h8as_launchTower(int groundY, int t8) {
  int x0 = 92, x1 = 112, top = groundY - 215;
  for (int py = top; py < groundY; py += h8as_ch) {
    int glow = 60 + v3_clamp((py - groundY + 120), 0, 120);
    int hue = py > groundY - 70 ? H8AS_ORANGE : H8AS_GREY;
    h8as_mark(x0, py, H8AS_C_BAR, glow + 30, hue); h8as_mark(x1, py, H8AS_C_BAR, glow + 30, hue);
    int row = (py - top) / h8as_ch;
    for (int px = x0 + h8as_cw; px < x1; px += h8as_cw) h8as_mark(px, py, ((row + (px - x0) / h8as_cw) & 1) ? H8AS_C_SLASH : H8AS_C_BACK, glow, hue);
    if (row % 9 == 4) for (int px = x1 + h8as_cw; px < 140; px += h8as_cw) h8as_mark(px, py, H8AS_C_DASH, glow, hue);   // swing arms
  }
  (void)t8;
}
static void h8as_ignition(float st, float s) {
  int t8 = (int)(st * 256), sp = (int)(v3_clampf(s * 1.2f, 0, 1) * 256);
  h8as_stars(0, 0, 14, 120);
  int groundY = 222;
  h8as_launchTower(groundY, t8);
  h8as_rocket(160, 6, 256, 0, 0, s > 0.25f ? 12 + (int)(s * 30) : 0, t8);
  H8AS_CELLS {
    int hue, lev = h8as_smokeAt(px, py, groundY, sp, t8, &hue);
    if (lev > 0) { if (py > 196 || H8AS_LEV[c] < 40) h8as_put(c, lev, hue); }
    else if (py > groundY) h8as_set(c, 25 + (h8as_vn(px * 20, py * 20, 7) >> 3), H8AS_BROWN);
  }
  h8as_flare(40, 40, 3, 230); h8as_flare(268, 72, 2, 200);
}
static void h8as_liftoff(float st, float s) {
  int t8 = (int)(st * 256);
  float alt = st * st * 14.0f;                                                   // px climbed
  float ss = v3_clampf(s, 0, 1);
  float z = 0.8f - 0.48f * ss;                                                   // the camera pulls back as it climbs
  int rocketY = (int)(14 + 10 * ss);
  int groundY = 222 + (int)(alt * z) + (int)(ss * 60);
  h8as_stars(0, -(int)(alt * 0.2f), 14, 120);
  if (groundY < 420) { h8as_launchTower(groundY, t8);
    H8AS_CELLS { int hue, lev = h8as_smokeAt(px, py, groundY, 256, t8, &hue); if (lev > 0) h8as_set(c, lev, hue); } }
  int flame = (int)((215 - rocketY) / z) - 200;                                  // the flame reaches the bottom of the screen
  flame = (int)(flame * v3_clampf(0.35f + ss * 1.6f, 0, 1)); if (flame < 40) flame = 40;
  h8as_rocket(160, rocketY, (int)(z * 256), 0, 0, flame, t8);
  // the smoke trail below the flame
  H8AS_CELLS {
    int dy = py - (rocketY + (int)((204 + flame) * z)), dx = px - 160;
    if (dy > 0) { int w = 8 + dy / 6, adx = dx < 0 ? -dx : dx; if (adx < w) h8as_set(c, (w - adx) * 90 / w + (h8as_vn(dx * 30, (py + t8) * 20, 5) >> 3), H8AS_MAROON); }
  }
  h8as_flare(52, 60, 3, 220); h8as_flare(270, 120, 2, 190);
}
static void h8as_stage2(float st, float s) {
  int t8 = (int)(st * 256);
  h8as_stars((int)(st * 6), 0, 18, 140);
  int hy0 = 150 + (int)(s * 25);                                                 // the horizon sinks as we climb
  H8AS_CELLS {
    int dx = px - 160, hy = hy0 + dx * dx / 1400;
    int d = py - hy;
    if (d >= -8 && d < 0) {                                                       // the thin bright atmosphere
      int k = (d + 8) * 32;
      int sun = dx > 40 ? 1 : 0;
      h8as_set(c, 120 + k / 2 + (h8as_vn(px * 40 + t8 * 4, py * 30, 3) >> 2), sun && d > -4 ? H8AS_ORANGE : (d > -3 ? H8AS_YELLOW : H8AS_CYAN));
    } else if (d >= 0) {                                                          // the night side below
      int n = h8as_fbm(px * 6 + t8 * 2, py * 9, 55);
      h8as_set(c, d < 4 ? 160 : 20 + (n > 150 ? (n - 150) : 0), d < 4 ? H8AS_CYAN : (n > 170 ? H8AS_WHITE : H8AS_DEEP));
    }
  }
  int ang = 120;                                                                  // tilted over toward orbit
  int rx = 180 + (int)(s * 30), ry = 62 - (int)(s * 20);
  int z = 80;
  // the long exhaust trail behind
  int cs = v3_cos(ang), sn = v3_sin(ang);
  int tx = rx - ((136 * z / 256) * sn >> 14), ty = ry + ((136 * z / 256) * cs >> 14);
  H8AS_CELLS {
    int dx = px - tx, dy = py - ty;
    int along = (-dx * sn + dy * cs) >> 14, perp = (dx * cs + dy * sn) >> 14;
    if (along > 0 && along < 260) {
      int ap = perp < 0 ? -perp : perp, w = 2 + along / 18;
      if (ap < w) { int l = (260 - along) * (w - ap) / w * 220 / 260; h8as_set(c, l + 20, along < 30 ? H8AS_YELLOW : (along < 90 ? H8AS_ORANGE : H8AS_RED)); }
    }
  }
  h8as_rocket(rx, ry, z, ang, 1, 40, t8);
  h8as_flare(60, 40, 3, 230); h8as_flare(250, 22, 2, 200);
}
// the Earth as an ASCII globe: (cx, cy) centre, R radius, rot = turn, lit from the right
static void h8as_earth(int cx, int cy, int R, int rot, float st) {
  int R2 = R * R, Ra = R + 7, Ra2 = Ra * Ra;
  H8AS_CELLS {
    int dx = px - cx, dy = py - cy, r2 = dx * dx + dy * dy;
    if (r2 >= Ra2) continue;
    if (r2 >= R2) {                                                               // the air glowing round the edge (lit side)
      int r = (int)v3_isqrt((uint32_t)r2), k = (Ra - r) * 255 / 7;
      int lit = dx * 3 - dy > 0 ? 255 : 60;
      h8as_set(c, (k * lit) >> 9, H8AS_CYAN);
      continue;
    }
    int nx = dx * 256 / R, ny = dy * 256 / R, nz = (int)v3_isqrt((uint32_t)v3_clamp(65536 - nx * nx - ny * ny, 0, 65536));
    int d = (180 * nx - 70 * ny + 120 * nz) >> 8;                                 // sunlight from the right
    int lon = (v3_atan(nx, nz) + rot) & 1023, lat = ny;
    int land = h8as_fbm(lon * 14, (lat + 300) * 7, 4242);
    int cloud = h8as_vn(lon * 30 + (int)(st * 30), (lat + 300) * 16, 777);
    int lev, hue;
    if (land > 140) { lev = 170 + (land - 140) / 2; hue = (lat < -170 || lat > 190) ? H8AS_WHITE : (h8as_vn(lon * 20, lat * 20 + 9000, 31) > 150 ? H8AS_OLIVE : H8AS_GREEN); }
    else { lev = 140 + land / 3; hue = land < 100 ? H8AS_DEEP : H8AS_OCEAN; }
    if (cloud > 175) { lev = 200 + (cloud - 175); hue = H8AS_WHITE; }
    int shade = 50 + (d < 0 ? 0 : d) * 230 / 256;                              // a little earthshine on the night side
    lev = lev * shade >> 8;
    if (d < 10) { int rim = 256 - nz; if (rim > 170) { lev = 40 + (rim - 170); hue = H8AS_DEEP; } }   // the dark limb still shows
    h8as_put(c, lev, hue);
  }
}
static void h8as_orbit(float st, float s) {
  h8as_stars((int)(st * 3), 0, 18, 130);
  float zm = v8_zoom(1.0f);
  int R = (int)((128 - 40 * s) * zm); if (R < 20) R = 20;
  h8as_earth(186, 128, R, (int)(st * 40), st);
  h8as_flare(40, 50, 3, 220);
}
// EARTHRISE: the grey cratered Moon in front, the Earth climbing over its horizon
static void h8as_earthrise(float st, float s) {
  h8as_stars((int)(st * 2), 0, 22, 140);
  float zm = v8_zoom(1.0f);
  int R = (int)(52 * zm); if (R < 10) R = 10;
  int ey = (int)(205 - 120 * v3_clampf(s * 1.4f, 0, 1));
  h8as_earth(210, ey, R, (int)(st * 30), st);
  int fwd = (int)(st * 40);
  H8AS_CELLS {
    int dx = px - 160, yh = 150 + dx * dx / 1700, d = py - yh;
    if (d < 0) continue;
    int u = dx * 256 / (d + 10), v = 40000 / (d + 10) + fwd;                       // the ground in perspective
    int n = h8as_fbm(u * 3 + 20000, v * 3, 4040);
    int lev = 110 + (n >> 1) + d;                                                   // nearer = brighter
    int hue = n > 175 ? H8AS_WHITE : H8AS_GREY;
    int gu = (u + 20000) >> 7, gv = v >> 7;                                         // craters, one per patch of ground (some)
    for (int a = -1; a <= 1; a++) for (int b = -1; b <= 0; b++) {
      int cu = gu + a, cv = gv + b;
      uint32_t h = v3_hash((uint32_t)cu * 73856093u ^ (uint32_t)cv * 19349663u);
      if ((h & 3) == 0) continue;
      int ccu = (cu << 7) + (int)((h >> 4) & 127) - 20000, ccv = (cv << 7) + (int)((h >> 12) & 127);
      int rr = 20 + (int)((h >> 20) & 31);
      int du = u - ccu, dv = (v - ccv) * 2, dd = du * du + dv * dv;
      if (dd < rr * rr) {
        int dist = (int)v3_isqrt((uint32_t)dd);
        if (dist > rr * 3 / 4) { lev = du < 0 ? 240 : 60; hue = du < 0 ? H8AS_WHITE : H8AS_GREY; }   // lit rim on the sunward side
        else lev = lev * 2 / 5;
      }
    }
    if (d < 2) { lev = 230; hue = H8AS_WHITE; }                                     // the bright horizon line
    h8as_put(c, lev, hue);
  }
  h8as_flare(48, 40, 3, 220);
}
// SATURN: a banded globe and its tilted rings (the near half crosses in front)
static void h8as_saturn(float st, float s) {
  h8as_stars((int)(st * 2), 0, 22, 140);
  float zm = v8_zoom(1.0f) * (0.9f + 0.2f * s);
  int R = (int)(52 * zm); if (R < 8) R = 8;
  int cx = 160, cy = 118, roll = -40, cs = v3_cos(roll), sn = v3_sin(roll), t8 = (int)(st * 256);
  static const uint8_t bandHue[8] = { H8AS_BROWN, H8AS_ORANGE, H8AS_YELLOW, H8AS_ORANGE, H8AS_YELLOW, H8AS_ORANGE, H8AS_RED, H8AS_BROWN };
  int R2 = R * R, rIn = R * 125 / 100, rOut = R * 230 / 100, cas0 = R * 186 / 100, cas1 = R * 196 / 100;
  H8AS_CELLS {
    int dx = px - cx, dy = py - cy;
    int rx = (dx * cs + dy * sn) >> 14, ry = (-dx * sn + dy * cs) >> 14;
    int r2 = dx * dx + dy * dy, inPlanet = r2 < R2;
    int ryp = ry * 256 / 112, rr = (int)v3_isqrt((uint32_t)(rx * rx + ryp * ryp));  // the ring plane, seen at a slant
    bool ring = rr >= rIn && rr < rOut && !(rr >= cas0 && rr < cas1);
    if (ring && (ry > 0 || !inPlanet)) {
      int f = (rr - rIn) * 256 / (rOut - rIn);
      int lev = (f < 60 ? 90 : (f < 170 ? 215 : 160)) + ((h8as_vn(rr * 60, 7, 3) - 128) >> 1);
      int hue = f < 60 ? H8AS_GREY : (f < 170 ? H8AS_WHITE : H8AS_CREAM);              // pale rings against the golden globe
      if (ry < 0 && rx > 0 && rx < R * 11 / 10) lev /= 4;                          // the planet's shadow across the far ring
      h8as_put(c, lev, hue);
      continue;
    }
    if (inPlanet) {
      int nx = dx * 256 / R, ny = dy * 256 / R, nz = (int)v3_isqrt((uint32_t)v3_clamp(65536 - nx * nx - ny * ny, 0, 65536));
      int d = (-150 * nx - 90 * ny + 190 * nz) >> 8; if (d < 0) d = 0;            // the sun from the upper left
      int bv = ry * 256 / R + ((h8as_vn(rx * 10 + t8, ry * 60, 81) - 128) >> 3);
      int bi = ((bv + 256) * 8 / 512) & 7;
      int lev = 40 + d * 215 / 256;
      if (ry > 0 && ry < R / 4 && rx > -R && rx < R) lev = lev * 2 / 3;            // the rings' shadow on the globe
      h8as_put(c, lev, bandHue[bi]);
    }
  }
  for (int k = 0; k < 3; k++) {                                                    // three moons
    float a = st * (0.5f - k * 0.12f) + k * 2.1f;
    int mr = (int)((290 + k * 40) * zm);
    int mx = cx + (int)(mr * cosf(a)), my = cy + (int)(mr * sinf(a) * 0.28f) - (int)(mr * cosf(a) * 0.1f);
    int cc = h8as_cellAt(mx, my); if (cc >= 0) { H8AS_GL[cc] = 0; h8as_put(cc, 240, k == 1 ? H8AS_ORANGE : H8AS_WHITE); }
  }
  h8as_flare(40, 34, 3, 220); h8as_flare(290, 200, 2, 180);
}
// NEBULA: glowing gas — magenta, cyan and orange clouds, dark dust, newborn stars
static void h8as_nebula(float st, float s) {
  h8as_stars((int)(st * 4), 0, 26, 150);
  float zm = v8_zoom(1.0f) * (1.0f + 0.25f * s);
  int iz = (int)(256.0f / zm), t8 = (int)(st * 256);
  H8AS_CELLS {
    int qx = ((px - 160) * iz >> 8) + 160 + t8 / 40, qy = ((py - 120) * iz >> 8) + 120;
    int w = h8as_vn(qx * 4, qy * 4, 11) - 128;                                      // warp, so the clouds swirl
    int wx = qx + (w >> 2), wy = qy + (w >> 3);
    int a = h8as_fbm(wx * 6, wy * 6 + t8 / 8, 101), b = h8as_vn(wx * 5 + 7000, wy * 5, 202), o = h8as_vn(wx * 7, wy * 7 + 5000 - t8 / 10, 303);
    int m = a, hue = a > 175 ? H8AS_PINK : H8AS_MAGENTA;
    if (b > m) { m = b; hue = b > 175 ? H8AS_CYAN : H8AS_OCEAN; }
    if (o > m) { m = o; hue = o > 175 ? H8AS_YELLOW : H8AS_ORANGE; }
    int lev = (m - 112) * 9 / 4;
    if (lev <= 0) continue;
    if (h8as_vn(wx * 10, wy * 10, 404) > 195) { lev /= 4; hue = H8AS_BROWN; }        // dark dust
    if (lev > 230 && (v3_hash((uint32_t)c) & 3) == 0) hue = H8AS_PURPLE;
    h8as_set(c, lev, hue);
  }
  static const int16_t sx[4] = { 92, 214, 150, 262 }, sy[4] = { 84, 150, 196, 58 };
  for (int k = 0; k < 4; k++) {                                                    // young hot stars with glowing halos
    int x = 160 + (int)((sx[k] - 160) * zm), y = 120 + (int)((sy[k] - 120) * zm);
    for (int yy = y - 16; yy <= y + 16; yy += h8as_ch) for (int xx = x - 22; xx <= x + 22; xx += h8as_cw) {
      int cc = h8as_cellAt(xx, yy); if (cc < 0) continue;
      int d2 = (xx - x) * (xx - x) + (yy - y) * (yy - y) * 2;
      if (d2 < 480) h8as_set(cc, 255 - d2 / 3, H8AS_WHITE);
    }
    h8as_flare(x, y, 2 + (k & 1), 255);
  }
}
// AURORA: green-to-purple curtains rippling over snowy mountains, mirrored in a lake
static int h8as_auroraAt(int px, int yy, int t8, int surge) {
  int b1 = 128 + (24 * v3_sin(px * 3 + t8 / 4) >> 14) + (12 * v3_sin(px * 8 - t8 / 2) >> 14);
  int b2 = b1 - 26 + (10 * v3_sin(px * 5 + t8 / 3 + 300) >> 14);
  int best = 0, hue = 0;
  for (int layer = 0; layer < 2; layer++) {
    int base = layer ? b2 : b1, h = base - yy;
    if (h < -6) continue;
    int ray = h8as_vn(px * 14 + t8 * (layer ? 3 : 2), layer * 5000, 77 + layer);   // vertical rays, drifting
    ray = (ray * ray >> 8) * ray >> 8;
    int hh2 = h * (layer ? 2 : 1), e = h < 0 ? (6 + h) * 40 : 255 * 24 / (24 + hh2 + hh2 * hh2 / 45);   // bright lower edge, fading out above
    int lev = (e * (40 + ray * 2) >> 8) * (256 + surge) >> 8;
    int hh = h < 0 ? H8AS_PINK : (h < 22 ? H8AS_GREEN : (h < 48 ? (layer ? H8AS_PURPLE : H8AS_CYAN) : (h < 85 ? H8AS_PURPLE : H8AS_MAGENTA)));
    if (lev > best) { best = lev; hue = hh; }
  }
  return best * 16 + hue;                                                           // packed: light × 16 + hue
}
static void h8as_aurora(float st, float s) {
  (void)s;
  h8as_stars(0, 0, 24, 150);
  int t8 = (int)(st * 256), surge = h8as_pulseT >= 0 ? (int)(200 * (1.0f - h8as_pulseT / 3.0f)) : 0;
  int lake = 200;
  H8AS_CELLS {
    int side = px < 160 ? 160 - px : px - 160;
    int ym = 198 - ((40 + (h8as_vn(px * 2, 50, 11) >> 2) + (h8as_vn(px * 9, 99, 12) >> 4)) * side) / 160;   // mountains on both sides, a shore in the middle
    if (py < ym) {
      int a = h8as_auroraAt(px, py, t8, surge);
      if ((a >> 4) > 20) h8as_set(c, a >> 4, a & 15);
    } else if (py < lake) {
      int snow = py - ym < 7 + ((px * 7) & 3);
      h8as_put(c, snow ? 150 : 22 + (h8as_vn(px * 30, py * 30, 5) >> 4), snow ? H8AS_WHITE : H8AS_DEEP);
    } else {
      int my = lake - (py - lake) * 3 - 30, rip = (3 * v3_sin(py * 40 - t8 * 3)) >> 14;   // the lake mirrors the lights, rippling
      int sr = (px + rip) < 160 ? 160 - (px + rip) : (px + rip) - 160;
      int ymr = 198 - ((40 + (h8as_vn((px + rip) * 2, 50, 11) >> 2) + (h8as_vn((px + rip) * 9, 99, 12) >> 4)) * sr) / 160;
      if (my >= ymr && sr > 40) { h8as_put(c, 30, H8AS_DEEP); continue; }
      int a = h8as_auroraAt(px + rip, my, t8, surge);
      if ((a >> 4) > 20) h8as_set(c, (a >> 4) * 11 / 20, a & 15);
    }
  }
}
static void h8as_solar(float st, float s) {
  h8as_stars((int)(st * 2), 0, 22, 140);
  float zm = v8_zoom(1.0f);
  int sx = 300, sy = 238, R = (int)((95 - 10 * s) * zm), Rc = R + 26;
  static const int orbR[5] = { 128, 162, 200, 245, 300 };
  static const uint8_t pHue[5] = { H8AS_GREY, H8AS_YELLOW, H8AS_OCEAN, H8AS_RED, H8AS_ORANGE };
  static const uint8_t pSz[5] = { 3, 5, 6, 4, 11 };
  int t8 = (int)(st * 256);
  H8AS_CELLS {
    int dx = px - sx, dy = py - sy, r2 = dx * dx + dy * dy;
    int r = (int)v3_isqrt((uint32_t)r2);
    if (r < R) {                                                                  // the sun: boiling granules, darker at the edge
      int n = h8as_fbm(px * 9 + t8 * 2, py * 9 - t8, 5151);
      int limb = 256 - r * 256 / R;
      int lev = 120 + (n >> 1) + (limb >> 2);
      h8as_put(c, lev, limb > 120 ? (n > 150 ? H8AS_WHITE : H8AS_YELLOW) : (n > 140 ? H8AS_YELLOW : H8AS_ORANGE));
    } else if (r < Rc) {
      h8as_set(c, (Rc - r) * 200 / 26 + (h8as_vn(px * 30, py * 30 + t8 * 3, 9) >> 3), (Rc - r) > 14 ? H8AS_ORANGE : H8AS_RED);
    } else {
      float zz = zm;
      for (int k = 0; k < 5; k++) {                                               // orbit paths, drawn as / \ | -
        int rr = (int)(orbR[k] * zz), d = r - rr; if (d < 0) d = -d;
        if (d * 2 < h8as_cw) { int g = dx * 2 < -dy ? H8AS_C_SLASH : H8AS_C_BAR;
          int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
          g = ady > adx * 2 ? H8AS_C_DASH : (adx > ady * 2 ? H8AS_C_BAR : ((dx < 0) == (dy < 0) ? H8AS_C_BACK : H8AS_C_SLASH));
          (void)g; H8AS_GL[c] = (uint8_t)g; H8AS_EL[c] = 70; if (H8AS_LEV[c] < 70) { H8AS_LEV[c] = 70; H8AS_HUE[c] = H8AS_GREY; } }
      }
    }
  }
  for (int k = 0; k < 5; k++) {                                                   // planets on their orbits (inner ones faster)
    float w = 0.9f / powf(orbR[k] / 128.0f, 1.5f);
    float a = 3.4f + k * 0.37f + st * w * 0.25f;
    int rr = (int)(orbR[k] * zm);
    int ppx = sx + (int)(rr * cosf(a)), ppy = sy + (int)(rr * sinf(a));
    int pr = pSz[k] + 2;
    for (int y = ppy - pr; y <= ppy + pr; y += h8as_ch / 2 + 1) for (int x = ppx - pr; x <= ppx + pr; x += h8as_cw / 2 + 1) {
      int ddx = x - ppx, ddy = y - ppy, d2 = ddx * ddx + ddy * ddy;
      if (d2 > pr * pr) continue;
      int cc = h8as_cellAt(x, y); if (cc < 0) continue;
      int lit = ((ddx * (sx - ppx) + ddy * (sy - ppy)) > 0) ? 255 : 90;          // the side facing the sun is lit
      int hue = pHue[k];
      if (k == 2 && h8as_vn(x * 60, y * 60, 3) > 150) hue = H8AS_GREEN;
      if (k == 4 && ((y - ppy + 20) / 3) & 1) hue = H8AS_BROWN;
      H8AS_GL[cc] = 0; h8as_put(cc, lit, hue);
    }
  }
  h8as_flare(70, 60, 3, 230); h8as_flare(150, 30, 2, 190);
}
static void h8as_galaxy(float st, float s) {
  h8as_stars((int)(st * 1.5f), 0, 20, 140);
  float zm = v8_zoom(1.0f) * (0.85f + 0.35f * s);
  int cx = 160, cy = 120, tilt = 100, rot = (int)(st * 18);
  int cs = v3_cos(tilt), sn = v3_sin(tilt), iz = (int)(256.0f / zm);
  H8AS_CELLS {
    int dx = px - cx, dy = py - cy;
    int gx = (dx * cs + dy * sn) >> 14, gy = (-dx * sn + dy * cs) >> 14;
    gx = gx * iz >> 8; gy = (gy * iz >> 8) * 256 / 105;                           // seen at a slant
    int r2 = gx * gx + gy * gy, r = (int)v3_isqrt((uint32_t)r2);
    if (r > 240) continue;
    int a = v3_atan(gy, gx);
    int ph = (a * 2 - r * 5 + rot) & 1023;                                         // two arms winding out
    int arm = v3_cos(ph); arm = arm > 0 ? (arm >> 6) : 0;                          // 0 … 256
    arm = arm * arm >> 8;
    int core = 255 * 900 / (r2 + 900);
    int disc = 256 - r * 256 / 240;
    int clump = h8as_vn(gx * 24 + 3000, gy * 24, 61);
    int lev = core + ((arm * disc >> 8) * (170 + clump) >> 8) + (disc * disc >> 11);
    int dust = v3_cos((ph - 140) & 1023); dust = dust > 9000 && r > 20 ? 1 : 0;     // a dark lane along each arm
    int hue = r < 26 ? H8AS_CREAM : (core > 120 ? H8AS_YELLOW : (clump > 200 && arm > 120 ? H8AS_PINK : H8AS_WHITE));
    if (r > 26 && lev < 140 && hue == H8AS_WHITE) hue = (clump & 1) ? H8AS_CYAN : H8AS_OCEAN;
    if (dust) { lev = lev / 2; hue = H8AS_BROWN; }
    if (lev > 20) h8as_set(c, lev + (h8as_vn(px * 64, py * 64, 2) >> 4), hue);
  }
}

// a jellyfish: (jx, jy) = middle of the rim; R = bell radius; P = pulse phase (1024 = one beat); hs = hue step
static const uint8_t h8as_jhues[10] = { H8AS_OCEAN, H8AS_CYAN, H8AS_PURPLE, H8AS_MAGENTA, H8AS_PINK, H8AS_YELLOW, H8AS_ORANGE, H8AS_RED, H8AS_MAGENTA, H8AS_PURPLE };
static void h8as_jelly(int jx, int jy, int R, int P, int hs, int detail) {
  int pc = v3_sin(P), t8 = (int)(h8as_t * 256);
  int Rx = R + (R * pc >> 14) * 12 / 100, Hb = (R * 88 / 100) - ((R * pc >> 14) * 9 / 100), rimH = R / 6 + 2;
  int nT = detail ? 12 : 7, tl = detail ? R * 27 / 10 : R * 2;
  int bx0 = jx - Rx - R, bx1 = jx + Rx + R, by0 = jy - Hb - 8, by1 = jy + tl + 10;
  int i0 = v3_clamp((bx0 - h8as_x0) / h8as_cw, 0, h8as_nx - 1), i1 = v3_clamp((bx1 - h8as_x0) / h8as_cw, 0, h8as_nx - 1);
  int j0 = v3_clamp((by0 - h8as_y0) / h8as_ch, 0, h8as_ny - 1), j1 = v3_clamp((by1 - h8as_y0) / h8as_ch, 0, h8as_ny - 1);
  if (by1 < 0 || by0 >= H) return;
  int sparkT = (int)(h8as_t * 9.0f);
  for (int j = j0; j <= j1; j++) {
    int py = h8as_y0 + j * h8as_ch + h8as_ch / 2, dy = py - jy;
    int und = (R * 8 / 100 * v3_sin(dy * 9 - t8 * 3 + (P >> 1))) >> 14;           // a ripple running down the bell: its sides undulate
    for (int i = i0; i <= i1; i++) {
      int px = h8as_x0 + i * h8as_cw + h8as_cw / 2, dx = px - jx, c = j * h8as_nx + i;
      int dxw = dx - und, adx = dxw < 0 ? -dxw : dxw;
      // the bell
      int inBell = 0;
      if (dy < 0 && dy > -Hb) { int nx = dxw * 256 / Rx, ny = dy * 256 / Hb; if (nx * nx + ny * ny < 65536) inBell = 1; }
      else if (dy >= 0 && adx < Rx) { int scal = (rimH * (16384 + v3_sin(dxw * 1024 / (R / 3 + 1) + t8 * 2))) >> 15; if (dy < scal + 1) inBell = 1; }
      if (inBell) {
        int nx = v3_clamp(dxw * 256 / Rx, -255, 255), ny = dy < 0 ? dy * 256 / Hb : 0;
        int nz = (int)v3_isqrt((uint32_t)v3_clamp(65536 - nx * nx - ny * ny, 0, 65536));
        int d = (-102 * nx - 154 * ny + 179 * nz) >> 8; if (d < 0) d = 0;
        int rim = 256 - nz;
        int lev = 90 + (d * 140 >> 8) + (rim * 130 >> 8) + ((h8as_vn(px * 40, py * 40 - t8 * 8, 21) - 128) >> 2);
        int band = v3_sin(dy * 14 + dx * 4 - t8 * 5) >> 13;                         // rainbow bands sweeping down the bell
        int hi = (hs + ((ny + 256) >> 6) + band + 20 + (int)((v3_hash((uint32_t)c) & 1))) % 10;
        int hue = h8as_jhues[hi];
        if (rim > 150) { lev += 70; hue = h8as_jhues[(hs + (dx * 12) / (R + 1) + 30 + (t8 >> 7)) % 10]; }   // glowing rim lights
        uint32_t hz = v3_hash((uint32_t)c * 2654435761u + (uint32_t)sparkT);
        if ((hz & 255) < 6) { lev = 255; hue = (hz >> 8) & 1 ? H8AS_WHITE : H8AS_CYAN; }   // bioluminescent sparkles
        h8as_put(c, lev, hue);
        continue;
      }
      // the glowing frilly arms inside, below the bell
      if (detail && dy > -Hb / 3 && dy < R * 15 / 10) {
        for (int k = 0; k < 4; k++) {
          int ax = jx + (k * 2 - 3) * R * 9 / 100 + ((R * 18 / 100) * v3_sin(dy * 7 - t8 * 2 + k * 280) >> 14) * (dy + R / 3) / R;
          int w = R * 9 / 100 + 1 - dy / 32 + (v3_sin(dy * 22 - t8 * 5 + k * 100) >> 13), ad = px - ax; if (ad < 0) ad = -ad;   // frilled edges
          if (w > 0 && ad < w) { h8as_set(c, 245 - dy * 60 / R, h8as_jhues[(k * 3 + dy / 10 + (t8 >> 7) + 40) % 10]); break; }
        }
      }
      // tentacles: long strands with travelling waves, colour running along them
      if (dy > 0 && dy < tl) {
        for (int k = 0; k < nT; k++) {
          int base = jx + Rx * 92 / 100 * (2 * k - (nT - 1)) / (nT - 1);
          int ph = dy * 6 - t8 * 4 + k * 170, fan = (base - jx) * dy / (2 * tl);      // waves run down them; they fan out lower down
          int A = R * 45 / 100 * dy / tl;
          int x = base + fan + ((A * v3_sin(ph) + (A * 2 / 5) * v3_sin(ph * 2 + 300 - k * 90)) >> 14);
          int dy2 = dy + h8as_ch, ph2 = ph + 6 * h8as_ch, fan2 = (base - jx) * dy2 / (2 * tl), A2 = R * 45 / 100 * dy2 / tl;
          int x2 = base + fan2 + ((A2 * v3_sin(ph2) + (A2 * 2 / 5) * v3_sin(ph2 * 2 + 300 - k * 90)) >> 14);
          int ad = px - x; if (ad < 0) ad = -ad;
          if (ad * 2 <= h8as_cw) {
            int sl = (x2 - x) * 256 / h8as_ch;
            int l = 235 - dy * 140 / tl;
            H8AS_GL[c] = (uint8_t)h8as_slopeGlyph(sl); H8AS_EL[c] = (uint8_t)l;
            if (H8AS_LEV[c] < l) { H8AS_LEV[c] = (uint8_t)l; H8AS_HUE[c] = h8as_jhues[(hs + k + dy / 14 + 50 - (t8 >> 6)) % 10]; }
            break;
          }
        }
      }
    }
  }
}
// little outline jellies, drawn as ASCII art: " .-~-. " / "(     )" / " ) ( ) "
static const char* const h8as_tiny[4] = { " .-~-. ", "(     )", " ) ( ) ", " ( ) ( " };
static void h8as_tinyJelly(int px, int py, int hue, int lev, int ph) {
  for (int r = 0; r < 4; r++) {
    const char* s = h8as_tiny[r == 3 ? (ph & 1 ? 2 : 3) : (r == 2 ? (ph & 1 ? 3 : 2) : r)];
    for (int k = 0; s[k]; k++) {
      if (s[k] == ' ') continue;
      const char* f = strchr(h8as_chars, s[k]); if (!f) continue;
      h8as_mark(px + k * h8as_cw, py + r * h8as_ch, (int)(f - h8as_chars), lev - r * 15, hue);
    }
  }
}
static void h8as_deep(float st, int bloom) {
  int t8 = (int)(st * 256);
  H8AS_CELLS {                                                                     // marine snow, sinking, and a faint blue glow from above
    uint32_t h = v3_hash((uint32_t)i * 7919u ^ (uint32_t)(j + (int)(st * 2.0f * 6 / h8as_ch)) * 104729u);
    if ((h & 1023) < 16) h8as_set(c, 60 + (h >> 24) / 4, H8AS_CYAN);
    else if (py < 70 && (h & 1023) < 60) h8as_set(c, 30 + (70 - py), H8AS_OCEAN);
  }
  static const uint8_t th[4] = { H8AS_MAGENTA, H8AS_CYAN, H8AS_PURPLE, H8AS_ORANGE };
  for (int k = 0; k < 4; k++) {                                                   // small jellies drifting up behind
    int yy = 260 - (int)((st * (6 + k * 2) + k * 70)) % 300;
    int xx = 30 + k * 80 + (k & 1) * 20 + (int)(10 * sinf(st * 0.3f + k));
    h8as_tinyJelly(xx, yy, th[k], 140, (int)(st * 1.3f + k));
  }
  float zm = v8_zoom(1.0f);
  float pulse = h8as_pulseT >= 0 ? 2.5f : 1.0f;
  if (!bloom) {
    int R = (int)(78 * zm);
    int P = (int)(st * 280 * pulse);
    int swim = (int)(10 * sinf(st * 0.45f)) - ((v3_sin(P) * 7) >> 14);         // it rises a little on each stroke
    int sway = (int)(16 * sinf(st * 0.23f));
    h8as_jelly(160 + sway, 96 + swim, R, P, (int)(st * 0.8f) % 10, 1);
  } else {
    for (int k = 0; k < 6; k++) {
      int R = (int)((20 + (k * 13) % 34) * zm);
      int yy = 300 - ((int)(st * (10 + k * 3)) + k * 64) % 400;
      int xx = 30 + k * 52 + (int)(18 * sinf(st * 0.2f + k * 2));
      h8as_jelly(xx, yy, R, (int)(st * (230 + k * 35) * pulse) + k * 300, ((int)(st * 0.6f) + k * 3) % 10, (k % 3) == 1);
    }
  }
  (void)t8;
}
// the neon glyph grid: a field of outline shapes, colour drifting through it
static const uint8_t h8as_neon[10] = { H8AS_RED, H8AS_ORANGE, H8AS_YELLOW, H8AS_GREEN, H8AS_CYAN, H8AS_OCEAN, H8AS_PURPLE, H8AS_MAGENTA, H8AS_PINK, H8AS_WHITE };
static void h8as_grid(float st) {
  int t8 = (int)(st * 256), slow = (int)(st * 2.0f);
  int rip = h8as_ripT >= 0 ? (int)(h8as_ripT * 220) : -1000;
  H8AS_CELLS {
    int f = h8as_fbm(px * 5 + t8 / 2, py * 5 - t8 / 3, 808);
    int g = h8as_vn(px * 3 - t8, py * 3 + 4000, 909);
    uint32_t h = v3_hash((uint32_t)c * 2654435761u + (uint32_t)(slow + (c % 7)) / 3);
    static const uint8_t pick[16] = { 3, 3, 3, 5, 5, 5, 4, 4, 4, 6, 6, 2, 2, 1, 7, 8 };   // mostly outlines, now and then filled
    int type = pick[(h >> 3) & 15];
    if (f > 175) type = 3 + ((f >> 3) % 3);                                        // shapes clump where the field is high
    if ((h & 31) == 0) type = 0;
    int hue = h8as_neon[((g * 10) >> 8) + ((h >> 12) & 1) > 9 ? 9 : ((g * 10) >> 8) + ((h >> 12) & 1)];
    int lev = 150 + (f >> 1) + ((h >> 16) & 31);
    int dx = px - 160, dy = py - 120, r = (int)v3_isqrt((uint32_t)(dx * dx + dy * dy)), dr = r - rip;
    if (dr > -14 && dr < 14) { lev = 255; type = 8; hue = H8AS_WHITE; }
    h8as_put(c, lev, hue);
    H8AS_GL[c] = (uint8_t)(100 + type);
  }
}

// ─── drawing the glyphs ───
static inline void h8as_g3draw(int x, int y, int code, int sc, uint8_t col) {
  const uint8_t* g = h8as_g3[code];
  for (int r = 0; r < 5; r++) {
    int b = g[r]; if (!b) continue;
    uint8_t* row = v3_buf + (y + r * sc) * W + x;
    if (sc == 1) { if (b & 4) row[0] = col; if (b & 2) row[1] = col; if (b & 1) row[2] = col; }
    else for (int q = 0; q < 2; q++) { uint8_t* rr = row + q * W; if (b & 4) { rr[0] = col; rr[1] = col; } if (b & 2) { rr[2] = col; rr[3] = col; } if (b & 1) { rr[4] = col; rr[5] = col; } }
  }
}
static inline void h8as_g5draw(int x, int y, int code, uint8_t col) {
  char ch = h8as_chars[code];
  const uint8_t* g = NULL;
  const char* xp = strchr(h8as_x5, ch);
  if (ch && xp) g = h8as_g5x[xp - h8as_x5]; else g = v8_font[v8_glyph(ch)];
  for (int r = 0; r < 7; r++) { int b = g[r]; if (!b) continue; uint8_t* row = v3_buf + (y + r) * W + x; for (int k = 0; k < 5; k++) if (b & (16 >> k)) row[k] = col; }
}
// neon shapes in an s × s cell: 1 dot · 2 small plus · 3 ring · 4 diamond · 5 square · 6 big plus · 7 filled square · 8 filled disc · 9-12 / \ | -
static void h8as_shape(int x, int y, int s, int type, uint8_t col) {
  int cx = x + s / 2, cy = y + s / 2, r = s / 2 - 1; if (r < 1) r = 1;
  switch (type) {
    case 1: v3_rect(cx - (s > 7), cy - (s > 7), cx, cy, col); break;
    case 2: { int q = r / 2 > 0 ? r / 2 : 1; v3_hspan(cx - q, cx + q, cy, col); for (int k = -q; k <= q; k++) v3_put(cx, cy + k, col); } break;
    case 3: { int a = 0, b = r, e = 1 - r;                                         // midpoint circle
      while (a <= b) { v3_put(cx + a, cy + b, col); v3_put(cx - a, cy + b, col); v3_put(cx + a, cy - b, col); v3_put(cx - a, cy - b, col);
        v3_put(cx + b, cy + a, col); v3_put(cx - b, cy + a, col); v3_put(cx + b, cy - a, col); v3_put(cx - b, cy - a, col);
        if (e < 0) e += 2 * a + 3; else { e += 2 * (a - b) + 5; b--; } a++; } } break;
    case 4: for (int k = 0; k <= r; k++) { v3_put(cx - r + k, cy - k, col); v3_put(cx + r - k, cy - k, col); v3_put(cx - r + k, cy + k, col); v3_put(cx + r - k, cy + k, col); } break;
    case 5: v3_hspan(cx - r, cx + r, cy - r, col); v3_hspan(cx - r, cx + r, cy + r, col); for (int k = -r; k <= r; k++) { v3_put(cx - r, cy + k, col); v3_put(cx + r, cy + k, col); } break;
    case 6: v3_hspan(cx - r, cx + r, cy, col); for (int k = -r; k <= r; k++) v3_put(cx, cy + k, col); break;
    case 7: v3_rect(cx - r + 1, cy - r + 1, cx + r - 1, cy + r - 1, col); break;
    case 8: v3_disc(cx, cy, r, col); break;
    case 9:  for (int k = 0; k < s; k++) v3_put(x + k, y + s - 1 - k, col); break;
    case 10: for (int k = 0; k < s; k++) v3_put(x + k, y + k, col); break;
    case 11: for (int k = 0; k < s; k++) v3_put(cx, y + k, col); break;
    case 12: v3_hspan(x, x + s - 1, cy, col); break;
  }
}
static const uint8_t h8as_shapeRamp[9] = { 0, 1, 2, 3, 4, 5, 6, 8, 7 };
static inline int h8as_codeToShape(int code) {
  switch (code) {
    case H8AS_C_SLASH: return 9; case H8AS_C_BACK: return 10; case H8AS_C_BAR: return 11;
    case H8AS_C_DASH: case H8AS_C_UNDER: case H8AS_C_TILDE: return 12; case H8AS_C_STAR: return 6;
    case H8AS_C_LP: case H8AS_C_RP: return 3; case H8AS_C_DOT: return 1;
  }
  return -1;
}

// ─── read-out: stage, numbers, letter count ───
static void h8as_num(char* o, long v) {                                          // 1234567 → "1,234,567"
  char tmp[24]; int n = 0; if (v < 0) v = 0;
  do { if (n && n % 4 == 3) tmp[n++] = ','; tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v && n < 22);
  for (int i = 0; i < n; i++) o[i] = tmp[n - 1 - i]; o[n] = 0;
}
static void h8as_hud(int scene, float st, float total) {
  char a[48], b[96], c[48], n1[28], n2[28];
  uint8_t dim = (uint8_t)(0x80 | H8AS_GREY), lit = (uint8_t)(0xC0 | H8AS_GREY);
  a[0] = b[0] = 0;
  switch (scene) {
    case H8AS_IGNITION: strcpy(a, "IGNITION"); strcpy(b, "ALTITUDE 0 M     DOWNRANGE 0 KM"); break;
    case H8AS_LIFTOFF:  strcpy(a, "LIFT OFF"); h8as_num(n1, (long)(st * st * 30)); snprintf(b, sizeof b, "ALTITUDE %s M", n1); break;
    case H8AS_STAGE2:   strcpy(a, "SECOND STAGE"); h8as_num(n1, (long)(120 + st * 25)); h8as_num(n2, (long)(200 + st * 60)); snprintf(b, sizeof b, "ALTITUDE %s KM  DOWNRANGE %s KM", n1, n2); break;
    case H8AS_ORBIT:    strcpy(a, "IN ORBIT"); h8as_num(n2, (long)(1050 + st * 470)); snprintf(b, sizeof b, "ALTITUDE 400 KM  DOWNRANGE %s KM", n2); break;
    case H8AS_SOLAR:    strcpy(a, "THE SOLAR SYSTEM"); h8as_num(n1, (long)(2395678 + st * 91000)); snprintf(b, sizeof b, "%s KM FROM HOME", n1); break;
    case H8AS_EARTHRISE: strcpy(a, "THE MOON"); h8as_num(n1, (long)(384400 + st * 30)); snprintf(b, sizeof b, "%s KM FROM HOME", n1); break;
    case H8AS_SATURN:   strcpy(a, "SATURN"); h8as_num(n1, (long)(1272000000L + (long)(st * 400000))); snprintf(b, sizeof b, "%s KM FROM HOME", n1); break;
    case H8AS_NEBULA:   strcpy(a, "A STELLAR NURSERY"); h8as_num(n1, (long)(7500 + st * 40)); snprintf(b, sizeof b, "%s LIGHT YEARS FROM HOME", n1); break;
    case H8AS_AURORA:   strcpy(a, "AURORA BOREALIS"); strcpy(b, "LATITUDE 69 N   KP INDEX 7"); break;
    case H8AS_GALAXY:   strcpy(a, "THE MILKY WAY"); h8as_num(n1, (long)(140452 + st * 2300)); snprintf(b, sizeof b, "%s LIGHT YEARS FROM HOME", n1); break;
    case H8AS_JELLY: case H8AS_BLOOM: strcpy(a, "THE DEEP"); h8as_num(n1, (long)(1240 + st * 3)); snprintf(b, sizeof b, "DEPTH %s M", n1); break;
    default: strcpy(a, "GLYPH FIELD"); break;
  }
  h8as_num(n1, h8as_letters); snprintf(c, sizeof c, "LETTERS %s", n1);
  v8_text(4, 206, "VIDEO GEM ASCII", 1, dim);
  v8_text(4, 216, a, 1, lit);
  if (b[0]) v8_text(4, 226, b, 1, dim);
  v8_text(228, 216, c, 1, dim);
  int ts = (int)(total * 100);                                                     // "T 05.33S", top right
  snprintf(c, sizeof c, "T %02d.%02dS", (ts / 100) % 100, ts % 100);
  v8_text(268, 4, c, 1, (uint8_t)(0x90 | H8AS_ORANGE));
}

static float h8as_total = 0;
void prog_ghascii_init() { v8_init(); h8as_scene = 0; h8as_st = 0; h8as_hold = -1; h8as_lastLook = -1; h8as_total = 0; }

void prog_ghascii_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  const H8AsLook& L = h8as_looks[look];
  v8_colours(V8P_RAINBOW, 0.02f, 0.9f, 140, 0.0f);
  if (v3_centre(0)) h8as_natural();
  float dt = v3_clampf(v6_f.sdt / V8_CHILL, 0.0f, 0.1f);                             // the trip runs at its own (real) pace; p1 scales it
  h8as_t += dt; if (h8as_t > 100000.0f) h8as_t -= 100000.0f;

  // ── which scene ──
  int charset = v3_pick(6, L.charset, H8AS_NCHAR);
  int size = v3_centre(5) ? L.size : v3_clamp((int)(v3_u(5) * 3.0f), 0, 2);
  float dur = v3_centre(9) ? 6.0f : 3.0f + v3_u(9) * 17.0f;
  int manual = v3_centre(7) ? -1 : v3_clamp((int)(v3_u(7) * H8AS_NSCENE), 0, H8AS_NSCENE - 1);
  if (v6_f.fresh || v6_first || look != h8as_lastLook) { h8as_scene = L.first; h8as_st = 0; h8as_hold = -1; h8as_total = 0; h8as_lastLook = look; }
  int mode = (manual >= 0 || L.count == 1) ? H8AS_HOLD : H8AS_RUN;
  if (L.slow) dur *= 2.5f;
  int first = L.first, last = L.first + L.count - 1;
  bool k12 = v6_k12hit;
  if (mode == H8AS_HOLD) {
    int want = manual >= 0 ? manual : (h8as_hold >= 0 ? h8as_hold : L.first);
    if (k12) {
      if (want == H8AS_JELLY || want == H8AS_BLOOM || want == H8AS_AURORA) h8as_pulseT = 0;   // pulse / surge
      else if (want == H8AS_GRID) h8as_ripT = 0;
      else { h8as_hold = (want + 1) % H8AS_AURORA; want = h8as_hold; }
    }
    if (want != h8as_scene) { h8as_scene = want; h8as_st = 0; }
  } else {
    if (h8as_scene < first || h8as_scene > last) { h8as_scene = first; h8as_st = 0; }
    if (k12 || h8as_st > dur) {
      h8as_scene = h8as_scene >= last ? first : h8as_scene + 1; h8as_st = 0;
      if (h8as_scene == first) h8as_total = 0;
    }
  }
  h8as_st += dt; h8as_total += dt;
  if (h8as_ripT >= 0) { h8as_ripT += dt; if (h8as_ripT > 2.5f) h8as_ripT = -1; }
  if (h8as_pulseT >= 0) { h8as_pulseT += dt; if (h8as_pulseT > 3.0f) h8as_pulseT = -1; }
  float st = h8as_st;
  float s = mode == H8AS_HOLD ? 0.5f + 0.5f * sinf(st * 0.12f) : v3_clampf(st / dur, 0.0f, 1.0f);
  if (mode == H8AS_HOLD && (h8as_scene == H8AS_IGNITION || h8as_scene == H8AS_LIFTOFF || h8as_scene == H8AS_STAGE2)) { float p = fmodf(st, 8.0f); s = p / 8.0f; st = p; }

  // ── the grid of cells ──
  static const uint8_t lw[3] = { 4, 6, 8 }, lh[3] = { 6, 8, 12 }, sw[3] = { 6, 8, 12 };
  if (charset == H8AS_SHAPES || charset == H8AS_BLOCKS) { h8as_cw = h8as_ch = sw[size]; }
  else { h8as_cw = lw[size]; h8as_ch = lh[size]; }
  h8as_nx = W / h8as_cw; h8as_ny = H / h8as_ch;
  h8as_x0 = (W - h8as_nx * h8as_cw) / 2; h8as_y0 = (H - h8as_ny * h8as_ch) / 2;
  int ncell = h8as_nx * h8as_ny;
  memset(H8AS_LEV, 0, ncell); memset(H8AS_HUE, 0, ncell); memset(H8AS_GL, 0, ncell); memset(H8AS_EL, 0, ncell);

  switch (h8as_scene) {
    case H8AS_IGNITION: h8as_ignition(st, s); break;
    case H8AS_LIFTOFF:  h8as_liftoff(st, s); break;
    case H8AS_STAGE2:   h8as_stage2(st, s); break;
    case H8AS_ORBIT:    h8as_orbit(st, s); break;
    case H8AS_EARTHRISE: h8as_earthrise(st, s); break;
    case H8AS_SOLAR:    h8as_solar(st, s); break;
    case H8AS_SATURN:   h8as_saturn(st, s); break;
    case H8AS_NEBULA:   h8as_nebula(st, s); break;
    case H8AS_AURORA:   h8as_aurora(st, s); break;
    case H8AS_GALAXY:   h8as_galaxy(st, s); break;
    case H8AS_JELLY:    h8as_deep(st, 0); break;
    case H8AS_BLOOM:    h8as_deep(st, 1); break;
    default:            h8as_grid(st); break;
  }

  // ── fades: between stages, and on a look change ──
  // stage changes don't go through black: the letters scramble out of one stage and into the next
  int fade = (int)(v8_fade * 256), scr = 0;
  { float a = 1.0f - v3_clampf(h8as_st / 0.4f, 0, 1), b = mode != H8AS_HOLD ? 1.0f - v3_clampf((dur - h8as_st) / 0.3f, 0, 1) : 0.0f;
    scr = (int)((a > b ? a : b) * 256); }
  if (fade < 256) for (int c = 0; c < ncell; c++) { H8AS_LEV[c] = (uint8_t)((H8AS_LEV[c] * fade) >> 8); H8AS_EL[c] = (uint8_t)((H8AS_EL[c] * fade) >> 8); }

  // ── outlines: where the light changes sharply, a / \ | _ along the edge ──
  float outl = v8_kn(8, L.outline / 100.0f);
  if (outl > 0.03f && h8as_scene != H8AS_GRID) {
    int thr = (int)(330 - outl * 250);
    uint8_t* Lv = H8AS_LEV;
    for (int j = 1; j < h8as_ny - 1; j++) for (int i = 1; i < h8as_nx - 1; i++) {
      int c = j * h8as_nx + i;
      if (H8AS_GL[c]) continue;
      { uint8_t* G = H8AS_GL; int m1 = G[c - 1], m2 = G[c + 1], m3 = G[c - h8as_nx], m4 = G[c + h8as_nx];   // not beside drawn marks (no doubled lines)
        if ((m1 && m1 < 100) || (m2 && m2 < 100) || (m3 && m3 < 100) || (m4 && m4 < 100)) continue; }
      int gx = Lv[c + 1] - Lv[c - 1], gy = Lv[c + h8as_nx] - Lv[c - h8as_nx];
      int ax = gx < 0 ? -gx : gx, ay = gy < 0 ? -gy : gy;
      if (ax + ay < thr) continue;
      int code = ay > ax * 2 ? (gy < 0 ? H8AS_C_UNDER : H8AS_C_DASH) : (ax > ay * 2 ? H8AS_C_BAR : ((gx > 0) == (gy > 0) ? H8AS_C_SLASH : H8AS_C_BACK));
      H8AS_GL[c] = (uint8_t)code;
      int m = Lv[c - 1]; if (Lv[c + 1] > m) m = Lv[c + 1]; if (Lv[c - h8as_nx] > m) m = Lv[c - h8as_nx]; if (Lv[c + h8as_nx] > m) m = Lv[c + h8as_nx];
      H8AS_EL[c] = (uint8_t)(m * 3 / 4);
      if (Lv[c] < 40) H8AS_HUE[c] = H8AS_HUE[gx > 0 ? c + 1 : c - 1];
    }
  }
  bool hud = v3_centre(10) ? L.hud != 0 : v3_u(10) > 0.5f;
  if (hud) {                                                                      // dim the cells under the read-out
    for (int c = 0; c < ncell; c++) { int py = h8as_y0 + (c / h8as_nx) * h8as_ch, px = h8as_x0 + (c % h8as_nx) * h8as_cw;
      if ((py >= 202 && (px < 200 || px > 222)) || (py < 14 && px > 260)) { H8AS_LEV[c] >>= 2; H8AS_EL[c] >>= 2; } }
  }

  // ── letters ──
  memset(v3_buf, 0, W * H);
  int letters = 0, shimmer = (int)(h8as_t * 7.0f);
  for (int j = 0; j < h8as_ny; j++) {
    int y = h8as_y0 + j * h8as_ch;
    for (int i = 0; i < h8as_nx; i++) {
      int c = j * h8as_nx + i, lev = H8AS_LEV[c], gl = H8AS_GL[c], hue = H8AS_HUE[c];
      int x = h8as_x0 + i * h8as_cw;
      int code;
      if (gl && gl < 100) { code = gl; lev = H8AS_EL[c] > 0 ? H8AS_EL[c] : lev; if (lev < 30) continue; }
      else {
        if (lev < 18 && gl < 100) {
          if (!scr) continue;
          uint32_t hs = v3_hash((uint32_t)c * 2654435761u + (uint32_t)v8_frameNo * 40503u);
          if ((int)(hs & 1023) >= scr / 3) continue;
          lev = 70 + (int)((hs >> 10) & 63); hue = (hs >> 20) & 1 ? H8AS_CYAN : H8AS_GREEN;
        }
        uint32_t hh = v3_hash((uint32_t)c * 2246822519u + (uint32_t)((shimmer + c) / 5));
        int jit = (hh & 7) == 0 ? 1 : ((hh & 7) == 1 ? -1 : 0);                      // letters swap now and then, like the original
        code = v3_clamp(((lev - 18) * H8AS_RAMP) / 238 + jit, 1, H8AS_RAMP - 1);
      }
      if (scr) {                                                                  // the scramble
        uint32_t hs = v3_hash((uint32_t)c * 747796405u + (uint32_t)v8_frameNo * 2891336453u);
        if ((int)(hs & 255) < scr) { code = 1 + (int)((hs >> 8) % (H8AS_RAMP - 1)); if (gl >= 100) gl = 100 + 1 + (int)((hs >> 16) % 8); }
      }
      int lvl = 4 + lev * 11 / 255; if (lvl > 15) lvl = 15;
      uint8_t col = (uint8_t)((lvl << 4) | hue);
      letters++;
      switch (charset) {
        case H8AS_SHAPES: {
          int sh = gl >= 100 ? gl - 100 : h8as_codeToShape(code);
          if (sh < 0) sh = h8as_shapeRamp[v3_clamp(lev * 9 / 256, 1, 8)];
          if (sh == 0) { letters--; break; }
          if (h8as_cw >= 8) { uint8_t g = (uint8_t)(((lvl / 3) << 4) | hue);  // a soft neon glow round each shape
            h8as_shape(x - 1, y, h8as_cw, sh, g); h8as_shape(x + 1, y, h8as_cw, sh, g); h8as_shape(x, y - 1, h8as_cw, sh, g); h8as_shape(x, y + 1, h8as_cw, sh, g); }
          h8as_shape(x, y, h8as_cw, sh, col);
        } break;
        case H8AS_BLOCKS: { int s2 = (lev * h8as_cw) >> 9; if (s2 < 1) s2 = 1; int cx = x + h8as_cw / 2, cy = y + h8as_ch / 2; v3_rect(cx - s2, cy - s2, cx + s2 - 1, cy + s2 - 1, col); } break;
        default: {
          if (charset == H8AS_CLASSIC && gl == 0) code = h8as_classic[v3_clamp(code * 10 / H8AS_RAMP, 1, 9)];
          if (charset == H8AS_BINARY && (gl == 0 || gl >= 100)) code = (v3_hash((uint32_t)c * 31u + (uint32_t)(shimmer / 3)) & 1) ? H8AS_C_ONE : H8AS_C_ZERO;
          if (gl >= 100 && charset != H8AS_BINARY) code = v3_clamp(lev * H8AS_RAMP / 256, 1, H8AS_RAMP - 1);
          if (h8as_cw == 6) h8as_g5draw(x, y, code, col);
          else if (h8as_cw == 8) h8as_g3draw(x + 1, y + 1, code, 2, col);
          else h8as_g3draw(x, y, code, 1, col);
        }
      }
    }
  }
  h8as_letters = letters;
  if (hud) h8as_hud(h8as_scene, h8as_st, h8as_total);
  v8_end();
}
