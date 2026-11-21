// =====================================================================
// vg7.h — the Set 7 "Analog Ghosts" engine (by Dewey)
//
// Set 7 is twelve programs that bring back the ghosts of analog video:
// the instruments and tricks of early video art, from Dan Sandin's Image
// Processor and the Rutt/Etra scan processor to Nam June Paik's magnet TV,
// camera feedback, Scanimate titles, vision-mixer wipes, slow-scan TV and
// the 2001 Stargate. Built on the Set 6 engine (vg6.h): same colours,
// same tour, same knobs — plus what the analog set needs:
//
//   • TWELVE ANALOG PALETTES — phosphor green, amber, the Chicago colourizer,
//     Vasulka blue, Paik primaries, Scanimate, Kodachrome, Lumia, blacklight,
//     studio bars, sunset and ghost grey.
//   • COMMON KNOBS (same as Set 6): p0 Palette (centre = the preset's own),
//     p1 Speed (far left = freeze), p2 Zoom, p3 Colour Flow, p4 Hue Spread,
//     p12-p15 live FX (vg5fx.h) — and on every Set 7 program
//     p11 SCANLINES (far left = off, centre = the preset's own, right = deep).
//   • OSCILLATORS — sine, triangle, saw, square and pulse, the five shapes
//     every analog video synth had.
//   • THE LAST FRAME — PicoDVI keeps two pictures: the one being drawn and
//     the one on screen. v7_front points at the one on screen (read only),
//     so FEEDBACK can film its own monitor.
//   • PHOSPHOR — v7_decay fades the picture by fractions of a level a frame,
//     a few rows at a time, like interlace, so afterglows can be long.
// =====================================================================
#ifndef VG7_H
#define VG7_H
#include "vg6.h"

// ─── Palettes: twelve gradients of six colours (they wrap round) ───────
#define V7_NPAL 12
static const uint8_t v7_pals[V7_NPAL][6][3] = {
  {{  0,  40,  10}, { 20, 140,  40}, { 80, 255, 120}, {190, 255, 200}, { 40, 200,  80}, {  0,  90,  30}},   //  0 Phosphor: P31 green
  {{ 60,  20,   0}, {180,  90,   0}, {255, 170,  30}, {255, 220, 140}, {220, 120,  10}, {100,  40,   0}},   //  1 Amber: P3 terminal
  {{255,  40,   0}, {255, 220,   0}, {  0, 230, 120}, {  0, 120, 255}, {200,   0, 255}, {255,   0, 120}},   //  2 Chicago: the Sandin colourizer
  {{  0,  10,  60}, { 40,  60, 200}, {120, 160, 255}, {230, 240, 255}, { 90,  40, 200}, { 10,  20, 120}},   //  3 Vasulka: cold violet and white
  {{255,  30,  40}, {  0, 220, 255}, {255, 240,   0}, { 40,  40, 255}, {255,   0, 180}, {  0, 255, 120}},   //  4 Paik: TV primaries
  {{255, 110,   0}, {255,   0, 140}, {120,   0, 255}, {  0, 200, 255}, {  0, 255, 160}, {255, 220,   0}},   //  5 Scanimate: 70s titles
  {{120,  30,  10}, {220, 120,  40}, {255, 220, 150}, { 70, 130, 140}, { 30,  70,  90}, {200,  60,  40}},   //  6 Kodachrome
  {{ 20,  10,  60}, {120,  60, 200}, {255, 120, 170}, {255, 200, 120}, { 80, 200, 220}, { 40,  60, 160}},   //  7 Lumia: veils of light
  {{ 30,   0,  90}, {160,   0, 255}, {255,   0, 230}, {  0, 255, 200}, {240, 255,   0}, { 80,   0, 200}},   //  8 Blacklight
  {{200, 200, 200}, {200, 200,   0}, {  0, 200, 200}, {  0, 200,   0}, {200,   0, 200}, {  0,   0, 200}},   //  9 Studio: colour bars
  {{ 40,   0,  40}, {180,  20,  60}, {255, 100,  30}, {255, 200,  80}, {120,  40, 120}, { 20,  10,  80}},   // 10 Sunset
  {{ 10,  12,  20}, { 70,  80, 100}, {170, 180, 200}, {240, 245, 255}, {120, 130, 150}, { 40,  45,  60}},   // 11 Ghost: grey-blue monochrome
};
static const char* const v7_palNames[V7_NPAL] = { "Phosphor", "Amber", "Chicago", "Vasulka", "Paik", "Scanimate", "Kodachrome", "Lumia", "Blacklight", "Studio", "Sunset", "Ghost" };
enum { V7P_PHOSPHOR, V7P_AMBER, V7P_CHICAGO, V7P_VASULKA, V7P_PAIK, V7P_SCANIMATE, V7P_KODACHROME, V7P_LUMIA, V7P_BLACKLIGHT, V7P_STUDIO, V7P_SUNSET, V7P_GHOST };

// ─── Palette: the Set 6 machinery (fades, flow, sheen) on Set 7's gradients ─
static inline void v7_grad(int pal, int p, int* o) {
  p %= 1536; if (p < 0) p += 1536;
  int i = p >> 8, f = p & 255, j = i == 5 ? 0 : i + 1;
  const uint8_t* a = v7_pals[pal][i]; const uint8_t* b = v7_pals[pal][j];
  for (int c = 0; c < 3; c++) o[c] = a[c] + (((b[c] - a[c]) * f) >> 8);
}
static void v7_palette(int spread256, int sheen) {
  if (!v6_gammaOK) { for (int l = 0; l < 16; l++) v6_gamma[l] = (int)(256.0f * powf(l / 15.0f, 1.25f)); v6_gammaOK = true; }
  int base = (int)(v6_flow * 1536.0f);
  int xf = (int)(v6_palX * 256);
  for (int h = 0; h < 16; h++) {
    int p = base + ((h * spread256 * 6) >> 4);
    int ca[3], cb[3], c[3];
    v7_grad(v6_palA, p, ca); v7_grad(v6_palB, p, cb);
    for (int q = 0; q < 3; q++) c[q] = ca[q] + (((cb[q] - ca[q]) * xf) >> 8);
    display.setColor((uint8_t)h, 0, 0, 0);
    for (int l = 1; l < 16; l++) {
      int k = v6_gamma[l], w = l > 11 ? ((l - 11) * sheen) >> 2 : 0;
      int r = (c[0] * k) >> 8, g = (c[1] * k) >> 8, b = (c[2] * k) >> 8;
      r += ((255 - r) * w) >> 8; g += ((255 - g) * w) >> 8; b += ((255 - b) * w) >> 8;
      display.setColor((uint8_t)((l << 4) | h), (uint8_t)v3_clamp(r, 0, 255), (uint8_t)v3_clamp(g, 0, 255), (uint8_t)v3_clamp(b, 0, 255));
    }
  }
}
// palette, colour flow and hue spread from the common knobs, the look's own in the centre bands
static void v7_colours(int ownPal, float ownFlow, float ownSpread, int sheen) {
  int pal = v3_centre(0) ? ownPal : v3_clamp(v3_k(0) * V7_NPAL / 1024, 0, V7_NPAL - 1);
  if (v6_first) { v6_palA = v6_palB = pal; v6_palX = 1.0f; }
  if (pal != v6_palB) { v6_palA = v6_palB; v6_palB = pal; v6_palX = 0; }
  if (v6_palX < 1.0f) { v6_palX += v3_dt / 1.6f; if (v6_palX >= 1.0f) { v6_palX = 1.0f; v6_palA = v6_palB; } }
  float fl = v3_centre(3) ? ownFlow : v3_bi(3) * fabsf(v3_bi(3)) * 0.6f;
  v6_f.flowRate = fl;
  v6_flow += fl * v3_dt * (v6_f.speed > 0 ? 1.0f : 0.0f);
  v6_flow -= floorf(v6_flow);
  float sp = v3_centre(4) ? ownSpread : 0.08f + v3_u(4) * 1.4f;
  v6_f.spread = (int)(sp * 256);
  v7_palette(v6_f.spread, sheen);
}

// ─── The picture on screen (last frame) ───────────────────────────────
// PicoDVI swaps between two buffers; once both have been seen, the one not
// being drawn is the one on screen. NULL until then (and in the simulator
// if it only has one buffer): programs then read their own buffer instead.
static uint8_t* v7_bufA = 0; static uint8_t* v7_bufB = 0;
static const uint8_t* v7_front = 0;
static void v7_findFront() {
  uint8_t* b = display.getBuffer();
  if (!v7_bufA) v7_bufA = b;
  else if (b != v7_bufA && !v7_bufB) v7_bufB = b;
  v7_front = v7_bufB ? (b == v7_bufA ? v7_bufB : v7_bufA) : 0;
}

// ─── Every frame ──────────────────────────────────────────────────────
static int v7_frameNo = 0;
static int v7_begin(int preset, int nLooks) {
  int look = v6_begin(preset, nLooks);
  v7_findFront();
  v7_frameNo++;
  return look;
}
// darken every other row by n levels (0 … 15): scanlines. Buffer rows are word-aligned.
__attribute__((optimize("O2"))) static void v7_scanRows(int n, int firstRow) {
  if (n <= 0) return;
  if (n > 15) n = 15;
  uint32_t sub = (uint32_t)n * 0x01010101u;
  for (int y = firstRow; y < H; y += 2) {
    uint32_t* p = (uint32_t*)(v3_buf + y * W);
    for (int i = 0; i < W / 4; i++) {
      uint32_t v = p[i];
      uint32_t lev = (v >> 4) & 0x0F0F0F0Fu, hue = v & 0x0F0F0F0Fu;
      uint32_t d = (lev | 0x10101010u) - sub;
      uint32_t keep = ((d >> 4) & 0x01010101u) * 0x0Fu;
      p[i] = ((d & keep) << 4) | hue;
    }
  }
}
// phosphor afterglow: fade by `rate` sixteenths of a level per frame. Whole levels go off
// every row; the fraction is spread over rows in turn (interlaced), so trails can last seconds.
__attribute__((optimize("O2"))) static void v7_decay(int rate16) {
  if (rate16 <= 0) return;
  int whole = rate16 >> 4, frac = rate16 & 15;
  for (int y = 0; y < H; y++) {
    int n = whole + ((((y * 7) + v7_frameNo * 5) & 15) < frac ? 1 : 0);
    if (n <= 0) continue;
    if (n > 15) n = 15;
    uint32_t sub = (uint32_t)n * 0x01010101u;
    uint32_t* p = (uint32_t*)(v3_buf + y * W);
    for (int i = 0; i < W / 4; i++) {
      uint32_t v = p[i];
      if (!(v & 0xF0F0F0F0u)) continue;                              // all black already
      uint32_t lev = (v >> 4) & 0x0F0F0F0Fu, hue = v & 0x0F0F0F0Fu;
      uint32_t d = (lev | 0x10101010u) - sub;
      uint32_t keep = ((d >> 4) & 0x01010101u) * 0x0Fu;
      p[i] = ((d & keep) << 4) | hue;
    }
  }
}
// at the very end of the draw: the live FX, then the scanlines (ownScan = the look's own, 0 … 8)
static int v7_scan = 0;
static void v7_end(int ownScan) {
  fx5_apply();
  v7_scan = v3_centre(11) ? ownScan : (v3_k(11) < 60 ? 0 : 1 + v3_k(11) * 8 / 1024);
  if (v7_scan > 0) v7_scanRows(v7_scan, 1);
}

// ─── Oscillators: phase 0 … 65535 → −127 … 127 ─────────────────────────
enum { V7W_SINE, V7W_TRI, V7W_SAW, V7W_SQUARE, V7W_PULSE, V7W_N };
static const char* const v7_waveNames[V7W_N] = { "Sine", "Triangle", "Saw", "Square", "Pulse" };
static inline int v7_wave(int shape, uint32_t ph) {
  ph &= 0xFFFF;
  switch (shape) {
    case V7W_SINE: { int s = vg_sin1024[ph >> 6] >> 7; return s > 127 ? 127 : s; }
    case V7W_TRI: { int p = (int)(ph >> 8); return p < 128 ? 2 * p - 127 : 383 - 2 * p; }
    case V7W_SAW: return (int)(ph >> 8) - 128;
    case V7W_SQUARE: return ph < 32768 ? 127 : -127;
    default: return ph < 16384 ? 127 : -127;
  }
}

// ─── Labels: common knobs, the program's own p5-p10, p11 scanlines ─────
static const char* v7_label(int pot, const char* const* own) {
  if (pot >= 0 && pot <= 4) return v6_commonLbl[pot];
  if (pot >= 12 && pot <= 15) return fx5_labels[pot - 12];
  if (pot >= 5 && pot <= 10) return own[pot - 5];
  if (pot == 11) return "Scanlines";
  return "";
}
// a knob in its centre band gives the look's own value, otherwise 0 … 1 across the dial
static inline float v7_kn(int pot, float own) { return v3_centre(pot) ? own : v3_u(pot); }
// …or −1 … 1
static inline float v7_kb(int pot, float own) { return v3_centre(pot) ? own : v3_bi(pot); }
#endif
