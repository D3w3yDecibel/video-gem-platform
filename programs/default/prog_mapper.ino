// =====================================================================
// PROGRAM: MAPPER — a projection-mapping kit (by Dewey) — Set 3, slot 9
//
// For projecting onto real things. Point the projector at a box, a corner
// of a room or a few panels, line the surfaces up with the geometry knobs,
// and every surface fills with light that follows its edges and its
// perspective: chases running round the edges, wipes, grids, stripes,
// checkers, rain, equalizers. Everything outside the surfaces stays black
// (no light), so only the object glows. Effects run to a beat, and each
// face can follow the one before it.
//
// How it works: each surface is a quad mapped from a unit square by a
// perspective transform, so effects are drawn in the surface's own (u, v)
// coordinates — lines stay straight and stripes converge like the real
// surface does.
//
// Layouts (p14; centre = Box):
//   Full    — the whole picture is one surface
//   Quad    — one surface, corner-pinned: p4-p11 move the four corners
//   Box     — the three visible faces of a box: position, size, turn, tilt, height, width, depth
//   Corner  — a corner of a room: two walls and the floor
//   Panels  — three free-standing panels
//
// Presets:
//   k0 Calibrate  — outlines, corner handles, grid and face numbers: use this to line up
//   k1 Edge Chase — light running round every edge
//   k2 Wipes      — colour wipes across the faces, one after another
//   k3 Grid Pulse — a grid with pulses running out from the middle
//   k4 Stripes    — stripes scrolling across the faces
//   k5 Checker    — a checkerboard flipping in waves
//   k6 Zoom       — squares zooming into each face
//   k7 Rain       — light falling down the faces
//   k8 Equalizer  — bars dancing on every face
//   k9 Ripple     — a grid rippling like water
//   k10 Build     — tiles filling the faces one by one, then clearing
//   k11 Show      — runs through all the effects, changing every 4 bars
//
//   k12 — CUE: next effect in the Show, or a wipe of light on the others
//
// Knobs:
//   p0 Palette (centre = preset)   p1 Speed (tempo; centre = 120 BPM, far left = freeze)
//   p2 Density (lines, stripes, cells)   p3 Colour Mode (centre = preset: per face, one colour, change on the beat, rainbow)
//   p4-p11 Geometry — what each one does depends on the layout (the info screen shows it)
//   p12 Edge Glow (outline brightness)   p13 Guides (outlines and handles over any effect)
//   p14 Layout (centre = Box)   p15 Follow (left: faces together, right: each face a beat behind the last)
// =====================================================================

#include "vg3.h"

// ─── Surfaces ─────────────────────────────────────────────────────────
#define MP_MAXF 4
static float mp_q[MP_MAXF][8];                     // corners: (0,0) (1,0) (1,1) (0,1) in screen px
static float mp_hm[MP_MAXF][8];                    // perspective transforms a b c d e f g h
static int mp_nf = 0;
static int mp_layout = 2;

static void mp_homog(int f) {
  const float* q = mp_q[f]; float* m = mp_hm[f];
  float x0 = q[0], y0 = q[1], x1 = q[2], y1 = q[3], x2 = q[4], y2 = q[5], x3 = q[6], y3 = q[7];
  float sx = x0 - x1 + x2 - x3, sy = y0 - y1 + y2 - y3;
  float g = 0, h = 0;
  if (fabsf(sx) > 0.01f || fabsf(sy) > 0.01f) {
    float dx1 = x1 - x2, dx2 = x3 - x2, dy1 = y1 - y2, dy2 = y3 - y2;
    float den = dx1 * dy2 - dx2 * dy1;
    if (fabsf(den) > 1e-4f) { g = (sx * dy2 - dx2 * sy) / den; h = (dx1 * sy - sx * dy1) / den; }
  }
  m[0] = x1 - x0 + g * x1; m[1] = x3 - x0 + h * x3; m[2] = x0;
  m[3] = y1 - y0 + g * y1; m[4] = y3 - y0 + h * y3; m[5] = y0;
  m[6] = g; m[7] = h;
}
// surface point (u, v) → screen, in 1/16 px
static inline void mp_map(int f, float u, float v, int32_t* x, int32_t* y) {
  const float* m = mp_hm[f];
  float w = m[6] * u + m[7] * v + 1.0f;
  if (w < 0.05f) w = 0.05f;
  float iw = 16.0f / w;
  *x = (int32_t)((m[0] * u + m[1] * v + m[2]) * iw);
  *y = (int32_t)((m[3] * u + m[4] * v + m[5]) * iw);
}
static void mp_fill(int f, float u0, float v0, float u1, float v1, uint8_t c) {
  int32_t xs[4], ys[4];
  mp_map(f, u0, v0, &xs[0], &ys[0]); mp_map(f, u1, v0, &xs[1], &ys[1]);
  mp_map(f, u1, v1, &xs[2], &ys[2]); mp_map(f, u0, v1, &xs[3], &ys[3]);
  v3_poly(xs, ys, 4, c);
}
static void mp_line(int f, float u0, float v0, float u1, float v1, int lev, int hue, int thick) {
  int32_t x0, y0, x1, y1;
  mp_map(f, u0, v0, &x0, &y0); mp_map(f, u1, v1, &x1, &y1);
  x0 >>= 1; y0 >>= 1; x1 >>= 1; y1 >>= 1;                          // 1/16 → 1/8 px
  v3_lineAA(x0, y0, x1, y1, lev, hue);
  if (thick > 1) {                                                  // thicker: two more lines either side, a little dimmer
    int32_t dx = x1 - x0, dy = y1 - y0;
    bool steep = v3_abs(dy) > v3_abs(dx);
    for (int k = 1; k < thick; k++) {
      int o = ((k + 1) / 2) * 8 * ((k & 1) ? 1 : -1);
      if (steep) v3_lineAA(x0 + o, y0, x1 + o, y1, lev - k, hue); else v3_lineAA(x0, y0 + o, x1, y1 + o, lev - k, hue);
    }
  }
}
static void mp_outline(int f, int lev, int hue, int thick) {
  mp_line(f, 0, 0, 1, 0, lev, hue, thick); mp_line(f, 1, 0, 1, 1, lev, hue, thick);
  mp_line(f, 1, 1, 0, 1, lev, hue, thick); mp_line(f, 0, 1, 0, 0, lev, hue, thick);
}
// a point along the outline, s in 0 … 4 (one unit per edge)
static void mp_perim(float s, float* u, float* v) {
  s = s - floorf(s / 4.0f) * 4.0f;
  int e = (int)s; float t = s - e;
  switch (e) { case 0: *u = t; *v = 0; break; case 1: *u = 1; *v = t; break; case 2: *u = 1 - t; *v = 1; break; default: *u = 0; *v = 1 - t; }
}

// ─── Layout geometry from the knobs ───────────────────────────────────
static inline float mp_kb(int i) { return v3_bi(i); }              // −1 … 1, centre 0
static void mp_setQuad(int f, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3) {
  float* q = mp_q[f]; q[0] = x0; q[1] = y0; q[2] = x1; q[3] = y1; q[4] = x2; q[5] = y2; q[6] = x3; q[7] = y3;
}
static void mp_buildLayout(int layout) {
  switch (layout) {
    case 0:                                                         // Full
      mp_nf = 1; mp_setQuad(0, 0, 0, W, 0, W, H, 0, H); break;
    case 1:                                                         // Quad: corner pin
      mp_nf = 1;
      mp_setQuad(0, 40 + mp_kb(4) * 160, 30 + mp_kb(5) * 120, 280 + mp_kb(6) * 160, 30 + mp_kb(7) * 120,
                    280 + mp_kb(8) * 160, 210 + mp_kb(9) * 120, 40 + mp_kb(10) * 160, 210 + mp_kb(11) * 120);
      break;
    case 3: {                                                       // Corner of a room: left wall, right wall, floor
      mp_nf = 3;
      float cx = 160 + mp_kb(4) * 120, top = 20 + mp_kb(5) * 60, bot = 170 + mp_kb(6) * 60;
      float lw = 150 * (1 + mp_kb(7) * 0.8f), rw = 150 * (1 + mp_kb(8) * 0.8f);
      float lp = 0.35f + mp_kb(9) * 0.3f, rp = 0.35f + mp_kb(10) * 0.3f, fl = 60 + mp_kb(11) * 50;
      mp_setQuad(0, cx - lw, top - lw * lp * 0.4f, cx, top, cx, bot, cx - lw, bot + lw * lp);
      mp_setQuad(1, cx, top, cx + rw, top - rw * rp * 0.4f, cx + rw, bot + rw * rp, cx, bot);
      mp_setQuad(2, cx - lw, bot + lw * lp, cx, bot, cx + rw, bot + rw * rp, cx, bot + fl + (lw + rw) * 0.3f);
      break;
    }
    case 4: {                                                       // Panels: three free-standing flats
      mp_nf = 3;
      float cx = 160 + mp_kb(4) * 100, cy = 120 + mp_kb(5) * 80, gap = 100 * (1 + mp_kb(6) * 0.6f), sz = 70 * (1 + mp_kb(7) * 0.6f);
      float turn = mp_kb(8) * 0.9f, stag = mp_kb(9) * 40, hgt = 1.4f * (1 + mp_kb(10) * 0.5f), lean = mp_kb(11) * 20;
      for (int f = 0; f < 3; f++) {
        float px = cx + (f - 1) * gap, py = cy + (f == 1 ? -stag : stag * 0.5f);
        float a = turn * (f - 1);
        float hw = sz * 0.5f * cosf(a), dz = sz * 0.5f * sinf(a) * 0.25f, hh = sz * 0.5f * hgt;
        mp_setQuad(f, px - hw + lean, py - hh * (1 + dz / 60), px + hw + lean, py - hh * (1 - dz / 60), px + hw, py + hh * (1 - dz / 60), px - hw, py + hh * (1 + dz / 60));
      }
      break;
    }
    default: {                                                      // Box: the top and two sides of a box
      mp_nf = 3;
      float X = 160 + mp_kb(4) * 120, Y = 150 + mp_kb(5) * 80, S = 90 * powf(2.0f, mp_kb(6));
      float yaw = 0.785f + mp_kb(7) * 0.7f, pitch = 0.5f + mp_kb(8) * 0.45f;
      float hgt = 1.0f * powf(1.8f, mp_kb(9)), wid = 1.0f * powf(1.8f, mp_kb(10)), dep = 1.0f * powf(1.8f, mp_kb(11));
      float cyw = cosf(yaw), syw = sinf(yaw), cp = cosf(pitch), sp = sinf(pitch);
      float px[8], py[8];
      for (int i = 0; i < 8; i++) {                                 // corners: x ±w, y 0/h, z ±d
        float x = (i & 1 ? 0.5f : -0.5f) * wid, y = (i & 2 ? hgt : 0.0f), z = (i & 4 ? 0.5f : -0.5f) * dep;
        float rx = x * cyw - z * syw, rz = x * syw + z * cyw;     // turn
        float ry = y * cp + rz * sp;                              // tilt: seen from a little above
        px[i] = X + rx * S; py[i] = Y - ry * S;
      }
      // faces facing the viewer: top (y = h), front (z = −d/2), side (x = −w/2)
      mp_setQuad(0, px[2 + 4], py[2 + 4], px[3 + 4], py[3 + 4], px[3], py[3], px[2], py[2]);          // top: back edge → front edge
      mp_setQuad(1, px[2], py[2], px[3], py[3], px[1], py[1], px[0], py[0]);                          // front
      mp_setQuad(2, px[6], py[6], px[2], py[2], px[0], py[0], px[4], py[4]);                          // side (x = −w/2: back edge on the left)
      break;
    }
  }
  for (int f = 0; f < mp_nf; f++) mp_homog(f);
}

// ─── Presets ──────────────────────────────────────────────────────────
enum { ME_CAL, ME_CHASE, ME_WIPE, ME_GRID, ME_STRIPE, ME_CHECK, ME_ZOOM, ME_RAIN, ME_EQ, ME_RIPPLE, ME_BUILD, ME_N };
static const uint8_t mp_pPal[11] = {0, 0, 1, 2, 3, 4, 5, 2, 6, 7, 1};
static const uint8_t mp_pCol[11] = {0, 0, 2, 0, 0, 0, 3, 0, 3, 1, 2};   // colour mode
static const uint8_t mp_pDen[11] = {4, 3, 1, 6, 5, 6, 5, 9, 8, 7, 6};
static const char* const mp_presetNames[12] = {
  "Calibrate", "Edge Chase", "Wipes", "Grid Pulse", "Stripes", "Checker",
  "Zoom", "Rain", "Equalizer", "Ripple", "Build", "Show"
};
static const char* const mp_labelsBase[16] = {
  "Palette", "Speed", "Density", "Colour Mode", "", "", "", "", "", "", "", "",
  "Edge Glow", "Guides", "Layout", "Follow"
};
static const char* const mp_geo[5][8] = {
  {"(unused)", "(unused)", "(unused)", "(unused)", "(unused)", "(unused)", "(unused)", "(unused)"},
  {"Top Left X", "Top Left Y", "Top Right X", "Top Right Y", "Bottom Right X", "Bottom Right Y", "Bottom Left X", "Bottom Left Y"},
  {"Box X", "Box Y", "Box Size", "Box Turn", "Box Tilt", "Box Height", "Box Width", "Box Depth"},
  {"Corner X", "Ceiling", "Floor Line", "Left Wall", "Right Wall", "Left Angle", "Right Angle", "Floor Depth"},
  {"Panels X", "Panels Y", "Spacing", "Panel Size", "Turn", "Stagger", "Height", "Lean"},
};
const char* prog_mapper_name() { return "MAPPER"; }
const char* prog_mapper_character() { return "Projection-mapping kit: line up a box, a room corner or panels, and fill every surface with light"; }
const char* prog_mapper_presetName(int preset) { return (preset >= 0 && preset < 12) ? mp_presetNames[preset] : NULL; }
const char* prog_mapper_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 4 && pot <= 11) return mp_geo[mp_layout < 0 || mp_layout > 4 ? 2 : mp_layout][pot - 4];
  return (pot >= 0 && pot < 16) ? mp_labelsBase[pot] : "";
}
uint8_t prog_mapper_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

#define MP_NPAL 8
static const uint8_t mp_pals[MP_NPAL][4][3] = {
  {{0,230,255},{255,0,170},{255,230,0},{255,255,255}},      // 0 Neon
  {{255,60,0},{255,170,0},{255,240,120},{255,20,60}},       // 1 Fire
  {{60,160,255},{0,255,220},{200,240,255},{120,80,255}},    // 2 Ice
  {{255,255,255},{200,220,255},{255,230,200},{160,160,170}},// 3 White Light
  {{255,0,0},{0,255,0},{0,80,255},{255,255,255}},           // 4 RGB
  {{255,120,40},{255,40,120},{150,40,255},{255,220,80}},    // 5 Sunset
  {{120,255,0},{180,0,255},{0,255,160},{255,255,0}},        // 6 Toxic
  {{255,60,60},{60,255,120},{80,120,255},{255,220,60}},     // 7 Primary
};

// ─── State ────────────────────────────────────────────────────────────
static int mp_preset = -1, mp_fx = 1, mp_pal = 0;
static float mp_beat = 0, mp_cueT = -10, mp_showBar = 0;
static bool mp_k12was = false;

void prog_mapper_init() { v3_reset(); mp_preset = -1; }

static void mp_palette(int pal, float beat, int colMode) {
  (void)beat; (void)colMode;
  uint8_t c[8][3];
  for (int h = 0; h < 4; h++) for (int q = 0; q < 3; q++) c[h][q] = mp_pals[pal][h][q];
  for (int h = 0; h < 4; h++) for (int q = 0; q < 3; q++) c[4 + h][q] = v3_mix8(mp_pals[pal][h][q], mp_pals[pal][(h + 1) & 3][q], 128);
  for (int h = 0; h < 8; h++) v3_setLH(1 + h, c[h][0], c[h][1], c[h][2], 1.2f, 0);
  v3_setLH(9, 255, 255, 255, 1.0f, 0);                              // white
  v3_setLH(10, 255, 60, 40, 1.0f, 0);                               // guide red
  v3_setLH(11, 40, 255, 120, 1.0f, 0);                              // guide green
}
static inline int mp_hue(int colMode, int face, int item, float beat) {
  switch (colMode) {
    case 1: return 1;
    case 2: return 1 + (((int)beat + face) & 3);
    case 3: return 1 + ((item + face) & 7);
    default: return 1 + (face & 3);
  }
}
static inline float mp_ease(float t) { t = v3_clampf(t, 0, 1); return t * t * (3 - 2 * t); }

static void mp_effect(int fx, int f, float ph, int den, int colMode, float beat, float glow) {
  int hue = mp_hue(colMode, f, 0, beat);
  int eg = (int)(glow * 15);
  switch (fx) {
    case ME_CHASE: {
      mp_outline(f, 3 + eg / 4, hue, 1);
      int segs = 2 + den / 2;
      for (int k = 0; k < segs; k++) {
        float s0 = ph * 1.0f + k * 4.0f / segs, len = 0.6f;
        for (int part = 0; part < 6; part++) {
          float a = s0 + part * len / 6, b = s0 + (part + 1) * len / 6;
          float u0, v0, u1, v1; mp_perim(a, &u0, &v0); mp_perim(b, &u1, &v1);
          if (fabsf(u0 - u1) + fabsf(v0 - v1) > 0.5f) continue;
          mp_line(f, u0, v0, u1, v1, 6 + part * 2, mp_hue(colMode, f, k, beat), 3);
        }
      }
      break;
    }
    case ME_WIPE: {
      int b = (int)floorf(ph); float t = mp_ease((ph - b) * 1.6f);
      int prevHue = mp_hue(colMode, f, b - 1, (float)(b - 1)), curHue = mp_hue(colMode, f, b, (float)b);
      if (colMode == 0) { prevHue = 1 + ((b + f + 3) & 3); curHue = 1 + ((b + f) & 3); }
      mp_fill(f, 0, 0, 1, 1, (uint8_t)(0x60 | prevHue));
      int dir = (b * 7 + f) & 3;
      uint8_t c = (uint8_t)(0xF0 | curHue);
      switch (dir) { case 0: mp_fill(f, 0, 0, t, 1, c); break; case 1: mp_fill(f, 1 - t, 0, 1, 1, c); break; case 2: mp_fill(f, 0, 0, 1, t, c); break;
                     default: mp_fill(f, 0.5f - t * 0.5f, 0.5f - t * 0.5f, 0.5f + t * 0.5f, 0.5f + t * 0.5f, c); }
      mp_outline(f, 6 + eg / 2, 9, 1);
      break;
    }
    case ME_GRID: {
      int n = 2 + den;
      for (int i = 0; i <= n; i++) {
        float u = (float)i / n, d = fabsf(u - 0.5f) * 2;
        float w = ph * 0.5f - d; w -= floorf(w); int lev = 3 + (int)(12 * powf(1 - w, 6));
        mp_line(f, u, 0, u, 1, lev, mp_hue(colMode, f, i, beat), 2);
        mp_line(f, 0, u, 1, u, lev, mp_hue(colMode, f, i + 1, beat), 2);
      }
      break;
    }
    case ME_STRIPE: {
      int n = 2 + den;
      float off = ph * 0.25f; off -= floorf(off);
      bool vert = (f & 1) == 0;
      for (int i = -1; i <= n; i++) {
        float a = (i + off) / n, b = a + 0.5f / n;
        a = v3_clampf(a, 0, 1); b = v3_clampf(b, 0, 1);
        if (b <= a) continue;
        uint8_t c = (uint8_t)(0xF0 | mp_hue(colMode, f, i, beat));
        if (vert) mp_fill(f, a, 0, b, 1, c); else mp_fill(f, 0, a, 1, b, c);
      }
      mp_outline(f, 4 + eg / 2, 9, 1);
      break;
    }
    case ME_CHECK: {
      int n = 2 + den / 2;
      for (int j = 0; j < n; j++) for (int i = 0; i < n; i++) {
        float w = ph * 0.5f - (float)(i + j) / (2 * n);
        int st = ((int)floorf(w) + i + j) & 1;
        float fr = w - floorf(w);
        int lev = st ? 15 : 2 + (int)(5 * (1 - fr));
        mp_fill(f, (float)i / n, (float)j / n, (float)(i + 1) / n, (float)(j + 1) / n, (uint8_t)(lev << 4 | mp_hue(colMode, f, st, beat)));
      }
      break;
    }
    case ME_ZOOM: {
      int n = 2 + den / 2;
      for (int k = 0; k < n; k++) {
        float r = (ph * 0.5f + (float)k / n); r -= floorf(r);
        float a = 0.5f - r * 0.5f, b = 0.5f + r * 0.5f;
        int lev = (int)(4 + 11 * r);
        int h = mp_hue(colMode, f, k + (int)(ph * 0.5f), beat);
        mp_line(f, a, a, b, a, lev, h, 2); mp_line(f, b, a, b, b, lev, h, 2); mp_line(f, b, b, a, b, lev, h, 2); mp_line(f, a, b, a, a, lev, h, 2);
      }
      break;
    }
    case ME_RAIN: {
      int n = 3 + den;
      for (int i = 0; i < n; i++) {
        uint32_t hsh = v3_hash(i * 977u + f * 131u);
        float u = (float)((hsh & 1023) / 1023.0) * 0.92f + 0.04f, sp = 0.5f + ((hsh >> 10) & 255) / 255.0f;
        float y = ph * 0.35f * sp + ((hsh >> 18) & 255) / 255.0f; y -= floorf(y); y = y * 1.4f - 0.2f;
        float w = 0.012f + 0.02f * ((hsh >> 26) & 3) / 3.0f;
        int h = mp_hue(colMode, f, i, beat);
        for (int k = 0; k < 5; k++) {
          float a = y - 0.06f * (k + 1), b = y - 0.06f * k;
          if (b < 0 || a > 1) continue;
          mp_fill(f, u - w, v3_clampf(a, 0, 1), u + w, v3_clampf(b, 0, 1), (uint8_t)((15 - k * 3) << 4 | h));
        }
      }
      mp_outline(f, 2 + eg / 3, hue, 1);
      break;
    }
    case ME_EQ: {
      int n = 3 + den / 2;
      for (int i = 0; i < n; i++) {
        float v = 0.5f + 0.5f * (v3_noise1((uint32_t)(ph * 180) + i * 997u + f * 55u, 77u + i) / 16384.0f);
        v = v3_clampf(v * 0.9f + 0.1f * (0.5f + 0.5f * sinf(ph * 3.14159f)), 0.05f, 1);
        float a = (float)i / n + 0.08f / n, b = (float)(i + 1) / n - 0.08f / n;
        int segs = 8;
        for (int s = 0; s < segs; s++) {
          float top = 1 - (float)(s + 1) / segs, bot = 1 - (float)s / segs + 0.0f;
          if (1 - top > v + 0.001f) break;
          int h = s < 5 ? mp_hue(colMode, f, i, beat) : (s < 7 ? 1 + ((mp_hue(colMode, f, i, beat)) & 3) % 4 + 0 : 9);
          mp_fill(f, a, top + 0.015f, b, bot - 0.015f, (uint8_t)(0xF0 | h));
        }
      }
      break;
    }
    case ME_RIPPLE: {
      int n = 3 + den / 2;
      for (int i = 0; i <= n; i++) {
        float base = (float)i / n;
        int h = mp_hue(colMode, f, i, beat), h2 = mp_hue(colMode, f, i + 2, beat);
        int32_t ax = 0, ay = 0, bx = 0, by = 0;                    // previous points of the two lines
        for (int s = 0; s <= 10; s++) {
          float u = s / 10.0f;
          float o = 0.035f * sinf((u * 2.5f - ph * 0.5f) * 6.2831853f + base * 4);
          int32_t x1, y1, x2, y2;
          mp_map(f, u, base + o, &x1, &y1); mp_map(f, base + o, u, &x2, &y2);
          if (s) { v3_lineAA(ax >> 1, ay >> 1, x1 >> 1, y1 >> 1, 14, h); v3_lineAA(bx >> 1, by >> 1, x2 >> 1, y2 >> 1, 9, h2); }
          ax = x1; ay = y1; bx = x2; by = y2;
        }
      }
      break;
    }
    case ME_BUILD: {
      int n = 3 + den / 2, tot = n * n;
      float cyc = ph / 8.0f; int ci = (int)floorf(cyc); float t = cyc - ci;   // 8 beats: build up, hold, clear
      int shown = t < 0.7f ? (int)(tot * t / 0.6f) : tot;
      int fade = t < 0.7f ? 15 : (int)(15 * (1 - (t - 0.7f) / 0.3f));
      for (int k = 0; k < tot && k < shown; k++) {
        uint32_t hsh = v3_hash(k * 2654435761u + ci * 97u + f * 13u);
        int idx = (int)((k * 37u + (hsh & 7)) % (uint32_t)tot);
        int i = idx % n, j = idx / n;
        int lev = k == shown - 1 && t < 0.7f ? 15 : fade;
        if (lev <= 0) continue;
        mp_fill(f, (float)i / n + 0.01f, (float)j / n + 0.01f, (float)(i + 1) / n - 0.01f, (float)(j + 1) / n - 0.01f, (uint8_t)(lev << 4 | mp_hue(colMode, f, idx, beat)));
      }
      mp_outline(f, 3 + eg / 2, 9, 1);
      break;
    }
    default: break;
  }
}

static void mp_guides(int strength) {
  for (int f = 0; f < mp_nf; f++) {
    mp_outline(f, strength, 9, 1);
    mp_line(f, 0, 0, 1, 1, strength / 3, 10, 1); mp_line(f, 1, 0, 0, 1, strength / 3, 10, 1);
    for (int i = 1; i < 4; i++) { mp_line(f, i / 4.0f, 0, i / 4.0f, 1, strength / 4, 11, 1); mp_line(f, 0, i / 4.0f, 1, i / 4.0f, strength / 4, 11, 1); }
    // corner handles and the face number
    for (int c = 0; c < 4; c++) {
      int x = (int)mp_q[f][c * 2], y = (int)mp_q[f][c * 2 + 1];
      for (int k = -3; k <= 3; k++) { v3_put(x + k, y - 3, 0xF0 | 10); v3_put(x + k, y + 3, 0xF0 | 10); v3_put(x - 3, y + k, 0xF0 | 10); v3_put(x + 3, y + k, 0xF0 | 10); }
    }
    int32_t cx, cy; mp_map(f, 0.5f, 0.5f, &cx, &cy);
    char s[2] = {(char)('1' + f), 0};
    v3_text((cx >> 4) - 5, (cy >> 4) - 7, s, 2, 0xF0 | 9);
  }
}

void prog_mapper_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  if (preset < 0 || preset >= 12) preset = 0;
  v3_frame();
  float dt = v3_dt;
  bool show = preset == 11;
  if (preset != mp_preset) { mp_preset = preset; mp_fx = show ? ME_CHASE : preset; mp_beat = 0; mp_showBar = 0; }

  // ── knobs ──
  float tempo = v3_k(1) < 30 ? 0.0f : 2.0f * powf(2.0f, v3_bi(1) * 1.3f);   // beats per second: 120 BPM at the centre
  int look = show ? mp_fx : preset;
  int den = v3_centre(2) ? mp_pDen[look] : v3_k(2) * 12 / 1024;
  int colMode = v3_pick(3, mp_pCol[look], 4);
  float glow = v3_u(12);
  float guides = v3_k(13) > 560 ? (v3_k(13) - 560) / 463.0f : 0.0f;   // off unless turned up past the centre
  mp_layout = v3_pick(14, 2, 5);
  float follow = v3_centre(15) ? 0.25f : v3_u(15);
  int pal = v3_centre(0) ? mp_pPal[look] : v3_clamp(v3_k(0) * MP_NPAL / 1024, 0, MP_NPAL - 1);

  // ── the beat ──
  mp_beat += dt * tempo;
  if (mp_beat > 100000.0f) mp_beat -= 99968.0f;
  bool k12 = keysPressed[KEY_MOD_A];
  bool cue = k12 && !mp_k12was;
  mp_k12was = k12;
  if (show) {
    float bar = floorf(mp_beat / 4.0f);
    if (bar >= mp_showBar + 4 || cue) { mp_showBar = bar; mp_fx = 1 + (mp_fx % (ME_N - 1)); }
  } else if (cue) mp_cueT = mp_beat;

  // ── geometry and colours ──
  mp_buildLayout(mp_layout);
  mp_palette(pal, mp_beat, colMode);
  memset(v3_buf, 0, W * H);                                          // black: no light outside the surfaces

  // ── effects, face by face ──
  for (int f = 0; f < mp_nf; f++) {
    float ph = mp_beat - f * follow * 2.0f;
    if (look == ME_CAL) {
      mp_fill(f, 0, 0, 1, 1, (uint8_t)(0x20 | (1 + f)));             // dim face colour so the surfaces show
    } else mp_effect(look, f, ph, den, colMode, mp_beat, glow);
    if (glow > 0.05f && look != ME_CAL) mp_outline(f, (int)(glow * 15), 9, glow > 0.6f ? 2 : 1);
    // a cue: a sweep of white light across every face
    float ct = (mp_beat - mp_cueT) - f * 0.25f;
    if (!show && ct >= 0 && ct < 1.0f) { float a = mp_ease(ct); mp_fill(f, a * 1.2f - 0.2f, 0, a * 1.2f, 1, 0xF9); }
  }
  if (look == ME_CAL) mp_guides(15);
  else if (guides > 0.05f) mp_guides((int)(guides * 15));
}
