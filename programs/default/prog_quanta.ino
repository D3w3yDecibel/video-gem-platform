// =====================================================================
// PROGRAM: QUANTA — quantized multi-scale grid (by Dewey) — Set 3
// The screen is cut into a grid of blocks at several scales at once: big
// blocks split into four, and again, and again, down to tiny cells. An
// unseen field flows across the screen and decides how finely each region
// is cut; as the "quantization blend" animates, cells merge and split in
// waves — fading in and out rather than popping. Cells are drawn as dotted
// point-cloud outlines, lattices of points, coloured clusters, bevelled
// tiles, rings, bars or triangles; lit cells shimmer, colour clusters
// drift, grids slide and bursts of detail ring out.
// Inspired by polyhop's "Illuminated Array X — animating quantization
// blends" (https://www.instagram.com/p/DeXh-35Eiw5/). The code is original.
//
// Presets:
//   k0 Quantize    — big outlined blocks, mid-size colour clusters, tiny points
//   k1 Sweep       — fine on one side, coarse on the other, the boundary sweeping
//   k2 Point Cloud — every cell a lattice of points, the grid sliding
//   k3 Ripple      — rings of fine detail spreading out, again and again
//   k4 Mondrian    — bold primary-colour blocks, slowly re-cut
//   k5 Rain        — symbols falling, the cutting raining down the screen
//   k6 Circuit     — chips joined by traces, signals pulsing
//   k7 Heatmap     — every cell filled by its value, colours drifting
//   k8 Bevel       — raised 3D tiles that rise and sink as the grid breathes
//   k9 Rings       — every cell a ring, rippling
//   k10 Kaleido    — mirrored four ways, colours cycling
//   k11 Equalizer  — cells as level bars, bouncing like a spectrum
//
//   k12 — QUANTIZE: a burst of fine detail rings out from the centre
//
// Knobs (this program owns the global knobs and makes its own colours):
//   p0 Palette (centre = the preset's own colours)   p1 Speed (far left = freeze)
//   p2 Block Size (40 / 60 / 80 / 120 px)   p3 Depth (how many times blocks may split)
//   p4 Detail (coarse … fine)   p5 Blend (how strongly the quantization animates)
//   p6 Trails   p7 Fill (a few … many cells light up)   p8 Dots (sparse … dense points)
//   p9 Field Scale (big slow regions … small busy ones)   p10 Colour Mix (one colour … many)
//   p11 Glow   p12 Flow X   p13 Flow Y (which way the field drifts)
//   p14 Outlines (dim … bright; past ~80 % adds a faint background grid)
//   p15 Events (far left = only k12 … frequent automatic bursts)
//
// How it works: the field is worked out once a frame on a coarse 8-px grid
// with whole-number noise, and every cell just looks it up (blended). The
// quadtree itself is rebuilt from the field every frame — nothing is stored.
// Pixels are (brightness << 4) | hue; "brightest wins"; a quick fade of the
// brightness gives the trails.
// =====================================================================

#include "vg_sin.h"
static inline float qa_sin(float r) {             // radians → −1..1 from the shared table
  int a = (int)(r * (1024.0f / 6.2831853f));
  return vg_sin1024[a & 1023] * (1.0f / 16384.0f);
}
static inline int qa_isin(int a) { return vg_sin1024[a & 1023]; }   // 1024 steps / turn → Q14
static inline int qa_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline uint32_t qa_hash(uint32_t a, uint32_t b) {
  uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
  h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12;
  return h;
}
static inline int qa_dist(int dx, int dy) {         // roughly the distance, no square root
  dx = abs(dx); dy = abs(dy);
  return dx > dy ? dx + (dy * 3 >> 3) : dy + (dx * 3 >> 3);
}

// ─── Colours ──────────────────────────────────────────────────────────
// hues 0 white, 1-6 colours, 7 grid
#define QA_NSCHEME 8
static const uint8_t qa_cols[QA_NSCHEME][8][3] = {
  {{235,250,255},{120,220,255},{60,140,255},{180,240,255},{90,255,230},{150,170,255},{220,235,255},{20,40,70}},   // 0 Ice
  {{255,255,255},{0,255,220},{255,40,200},{255,230,0},{120,90,255},{0,180,255},{255,120,40},{40,20,70}},        // 1 Neon
  {{255,240,210},{255,180,60},{255,120,20},{255,220,120},{200,80,20},{255,200,140},{255,150,90},{60,35,10}},     // 2 Amber
  {{210,255,210},{40,255,110},{0,180,70},{150,255,120},{0,255,190},{90,200,90},{200,255,80},{8,45,20}},          // 3 Matrix
  {{240,245,255},{60,230,255},{255,60,170},{255,50,60},{70,120,255},{170,90,255},{0,200,200},{35,40,70}},        // 4 Polyhop
  {{250,250,245},{230,30,30},{250,210,20},{30,70,200},{250,250,245},{20,20,20},{230,30,30},{60,60,60}},          // 5 Primary
  {{255,230,220},{255,40,40},{255,140,0},{255,220,60},{255,90,140},{200,20,60},{255,180,120},{60,10,10}},       // 6 Infrared
  {{255,250,250},{255,170,200},{170,220,255},{200,255,210},{255,230,170},{220,190,255},{170,255,240},{50,45,70}},// 7 Pastel
};
static int16_t qa_levK[16];
static void qa_buildPalette(int scheme) {
  const uint8_t (*c)[3] = qa_cols[scheme];
  for (int h = 0; h < 16; h++) {
    int r = 0, g = 0, b = 0;
    if (h < 8) { r = c[h][0]; g = c[h][1]; b = c[h][2]; }
    display.setColor(h, 0, 0, 0);
    for (int l = 1; l < 16; l++) {
      int k = qa_levK[l];
      int rr = r * k >> 8, gg = g * k >> 8, bb = b * k >> 8;
      if (l >= 14 && h != 7) { rr += (255 - rr) >> 2; gg += (255 - gg) >> 2; bb += (255 - bb) >> 2; }
      display.setColor((l << 4) | h, rr, gg, bb);
    }
  }
}

// ─── Drawing ──────────────────────────────────────────────────────────
static uint8_t* qa_buf;
static inline void qa_put(int x, int y, int hue, int lev) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H || lev <= 0) return;
  uint8_t* p = qa_buf + y * W + x;
  if (lev > 15) lev = 15;
  if ((*p >> 4) <= lev) *p = (uint8_t)((lev << 4) | hue);
}
static void qa_hline(int x0, int x1, int y, int step, int hue, int lev) {
  if ((unsigned)y >= (unsigned)H || lev <= 0) return;
  if (lev > 15) lev = 15;
  if (x0 < 0) x0 += ((-x0 + step - 1) / step) * step;
  if (x1 > W - 1) x1 = W - 1;
  uint8_t c = (uint8_t)((lev << 4) | hue);
  uint8_t* p = qa_buf + y * W;
  for (int x = x0; x <= x1; x += step) if ((p[x] >> 4) <= lev) p[x] = c;
}
static void qa_vline(int x, int y0, int y1, int step, int hue, int lev) {
  if ((unsigned)x >= (unsigned)W || lev <= 0) return;
  if (lev > 15) lev = 15;
  if (y0 < 0) y0 += ((-y0 + step - 1) / step) * step;
  if (y1 > H - 1) y1 = H - 1;
  uint8_t c = (uint8_t)((lev << 4) | hue);
  for (int y = y0; y <= y1; y += step) { uint8_t* p = qa_buf + y * W + x; if ((*p >> 4) <= lev) *p = c; }
}
static void qa_fill(int x0, int y0, int x1, int y1, int hue, int lev, int pat) {   // pat 0 solid, 1 checker, 2 sparse
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > W - 1) x1 = W - 1;
  if (y1 > H - 1) y1 = H - 1;
  if (lev <= 0) return;
  if (lev > 15) lev = 15;
  uint8_t c = (uint8_t)((lev << 4) | hue);
  for (int y = y0; y <= y1; y++) {
    uint8_t* p = qa_buf + y * W;
    if (pat == 0) { for (int x = x0; x <= x1; x++) if ((p[x] >> 4) <= lev) p[x] = c; }
    else if (pat == 1) { for (int x = x0 + ((x0 + y) & 1); x <= x1; x += 2) if ((p[x] >> 4) <= lev) p[x] = c; }
    else if ((y & 1) == 0) { for (int x = x0 + (y & 2); x <= x1; x += 4) if ((p[x] >> 4) <= lev) p[x] = c; }
  }
}
static void qa_line(int x0, int y0, int x1, int y1, int hue, int lev) {
  int dx = abs(x1 - x0), dy = abs(y1 - y0), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx - dy, n = 0;
  for (;;) {
    qa_put(x0, y0, hue, lev);
    if ((x0 == x1 && y0 == y1) || ++n > 400) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x0 += sx; }
    if (e2 < dx) { err += dx; y0 += sy; }
  }
}
static inline uint32_t qa_fade4(uint32_t x, uint32_t d4) {
  uint32_t h = (x >> 4) & 0x0F0F0F0Fu;
  uint32_t t = (h | 0x80808080u) - d4;
  uint32_t m = ((t & 0x80808080u) >> 7) * 0xFFu;
  return (((t & 0x7F7F7F7Fu) & m) << 4) | (x & 0x0F0F0F0Fu);
}
static void qa_fade(int dec) {
  uint32_t* b = (uint32_t*)qa_buf;
  uint32_t d4 = (uint32_t)dec * 0x01010101u;
  for (int i = 0; i < W * H / 4; i++) b[i] = qa_fade4(b[i], d4);
}

// ─── The field: worked out once a frame on an 8-px grid ───────────────
#define QA_G 8
#define QA_GW (W / QA_G + 2)
#define QA_GH (H / QA_G + 2)
static uint8_t qa_fg[QA_GH][QA_GW];               // the cutting field, 0..255
static uint8_t qa_cg[QA_GH][QA_GW];               // the colour field, 0..255
// whole-number value noise: coordinates in 1/256 lattice steps → 0..255
static inline int qa_vnI(int x, int y, uint32_t salt) {
  int ix = x >> 8, iy = y >> 8, fx = x & 255, fy = y & 255;
  fx = fx * fx * (768 - 2 * fx) >> 16; fy = fy * fy * (768 - 2 * fy) >> 16;   // smoothstep
  uint32_t hx0 = (uint32_t)ix * 73856093u ^ salt, hx1 = (uint32_t)(ix + 1) * 73856093u ^ salt;
  int a = qa_hash(hx0, (uint32_t)iy) & 255, b = qa_hash(hx1, (uint32_t)iy) & 255;
  int c = qa_hash(hx0, (uint32_t)(iy + 1)) & 255, d = qa_hash(hx1, (uint32_t)(iy + 1)) & 255;
  int top = a + ((b - a) * fx >> 8), bot = c + ((d - c) * fx >> 8);
  return top + ((bot - top) * fy >> 8);
}
static float qa_fx = 0, qa_fy = 0, qa_fz = 0, qa_scale = 60.0f, qa_cshift = 0;
static void qa_buildField() {
  int zi = (int)floorf(qa_fz);
  int zf = (int)((qa_fz - zi) * 256.0f);
  zf = zf * zf * (768 - 2 * zf) >> 16;
  uint32_t s0 = (uint32_t)zi * 977u, s1 = (uint32_t)(zi + 1) * 977u, s2 = 31337u + (uint32_t)zi;
  float inv = 256.0f / qa_scale;
  int ox = (int)((qa_fx - QA_G) * inv), oy = (int)((qa_fy - QA_G) * inv), step = (int)(QA_G * inv);
  int cox = (int)((qa_fx * 0.37f + 400.0f + qa_cshift) * inv * 0.6f), coy = (int)((qa_fy * 0.37f - 300.0f) * inv * 0.6f), cstep = (int)(QA_G * inv * 0.37f * 0.6f) + 1;
  for (int j = 0; j < QA_GH; j++) {
    int y = oy + j * step, cy = coy + j * cstep;
    for (int i = 0; i < QA_GW; i++) {
      int x = ox + i * step;
      int n0 = qa_vnI(x, y, s0), n1 = qa_vnI(x, y, s1);
      int n = n0 + ((n1 - n0) * zf >> 8);
      int m = qa_vnI(x * 23 / 10 + 1306, y * 23 / 10 - 435, s2);
      qa_fg[j][i] = (uint8_t)((n * 184 + m * 72) >> 8);
      qa_cg[j][i] = (uint8_t)qa_vnI(cox + i * cstep, cy, 4242u);
    }
  }
}
// look a field up at a screen position (blended between grid points) → 0..255
static inline int qa_look(uint8_t (*g)[QA_GW], int x, int y) {
  x += QA_G; y += QA_G;
  if (x < 0) x = 0; else if (x > (QA_GW - 1) * QA_G - 1) x = (QA_GW - 1) * QA_G - 1;
  if (y < 0) y = 0; else if (y > (QA_GH - 1) * QA_G - 1) y = (QA_GH - 1) * QA_G - 1;
  int i = x >> 3, j = y >> 3, fx = (x & 7) << 5, fy = (y & 7) << 5;
  int a = g[j][i], b = g[j][i + 1], c = g[j + 1][i], d = g[j + 1][i + 1];
  int top = a + ((b - a) * fx >> 8), bot = c + ((d - c) * fx >> 8);
  return top + ((bot - top) * fy >> 8);
}

// ─── Settings for this frame ──────────────────────────────────────────
#define QS_CLASSIC 0
#define QS_POINTS 1
#define QS_MONDRIAN 2
#define QS_HEAT 3
#define QS_GLYPH 4
#define QS_CIRCUIT 5
#define QS_MIXED 6
#define QS_BEVEL 7
#define QS_RINGS 8
#define QS_BARS 9
#define QF_TWINKLE 1
#define QF_CYCLE 2
#define QF_SCROLL 4
#define QF_PULSE 8
static int qa_style = 0, qa_ctrl = 0, qa_flags = 0, qa_depth = 4, qa_minS = 4;
static int qa_detail = 128, qa_blendQ = 0, qa_fillQ = 77, qa_mixQ = 150;   // 0..256
static float qa_t = 0.0f, qa_dots = 0.5f;
static int qa_glow = 12, qa_outline = 10, qa_bias = 0;
#define QA_NB 4
static float qa_bx[QA_NB], qa_by[QA_NB], qa_bt[QA_NB] = {9, 9, 9, 9};   // bursts: centre and age (s)
static int qa_tq = 0;                             // time in 1/1024 turns (for integer waves)

// how much the block at (x,y,size s, level L) wants to split: > 0 = split (×256 scale)
static int qa_splitM(int x, int y, int s, int L) {
  int cx = x + (s >> 1), cy = y + (s >> 1);
  int v = qa_look(qa_fg, cx, cy);
  int bias = qa_bias;
  switch (qa_ctrl) {
    case 1: bias += (128 - cx * 256 / W) * 280 / 256 * qa_isin(qa_tq * 2 / 5) >> 14; break;                  // sweep
    case 2: bias += 90 * qa_isin(qa_dist(cx - HALFW, cy - HALFH) * 8 - qa_tq * 2) >> 14; break;              // ripple
    case 3: bias += 128 * qa_isin(qa_tq * 7 / 10) >> 14; break;                                               // breath
    case 5: bias += (int)((qa_hash((uint32_t)(x * 31 + y * 7 + L), (uint32_t)(qa_tq >> 7)) & 255) - 128) * 150 >> 8; break;   // flicker
    default: break;
  }
  for (int b = 0; b < QA_NB; b++) {                              // bursts: a ring of fine detail spreading out
    if (qa_bt[b] > 2.5f) continue;
    int R = (int)(qa_bt[b] * 120.0f);
    int d = qa_dist(cx - (int)qa_bx[b], cy - (int)qa_by[b]) - R;
    if (d < 0) d = -d;
    if (d < 40) bias += (int)(230.0f * (1.0f - qa_bt[b] / 2.5f)) * (40 - d) / 40;
  }
  return v + bias - (108 + L * 28);                              // deeper levels need more push
}

// colour cluster: the colour field picks a hue band; Colour Mix widens it
static inline int qa_hueAt(int x, int y, uint32_t h) {
  int cv = qa_look(qa_cg, x, y) - 64;                            // stretch the field across the hues
  int k = cv * 2 * 6 * (90 + qa_mixQ * 166 / 256) >> 16;
  if (qa_mixQ > 205 && (h & 31) == 0) k += 3;                    // a few strays at high mix
  if (qa_mixQ < 13) k = 0;
  return 1 + ((k % 6) + 6) % 6;
}

// draw one leaf cell; fadeIn (0..256) softens cells that have only just appeared
static void qa_leaf(int x, int y, int s, int L, int fadeIn) {
  int cx = x + (s >> 1), cy = y + (s >> 1);
  int v = qa_look(qa_fg, cx, cy);
  uint32_t h = qa_hash((uint32_t)(x * 131 + y), (uint32_t)(s * 17 + L));
  int hue = qa_hueAt(cx, cy, h);
  int x1 = x + s - 1, y1 = y + s - 1;
  int dstep = qa_dots < 0.33f ? 3 : (qa_dots < 0.66f ? 2 : 1);
  // how lit (0..256): cells light up in clusters where the field is high, fading in smoothly
  int cl = (v - 97) * 3; if (cl < 0) cl = 0; else if (cl > 256) cl = 256;
  int want = qa_fillQ * 410 * cl >> 16;                          // 0..~410
  int r = (int)(h & 255);
  int lit = (want - r) * 6; if (lit < 0) lit = 0; else if (lit > 256) lit = 256;
  lit = lit * fadeIn >> 8;
  int glow = qa_glow;
  if ((qa_flags & QF_TWINKLE) && lit > 0) glow += (3 * qa_isin(qa_tq * 5 + (int)(h & 1023))) >> 14;
  if (qa_flags & QF_PULSE) {                                      // a signal pulse travelling across
    int ph = (cx + cy * 2 - (qa_tq >> 1)) & 255;
    if (ph < 18) { glow += 3; if (!lit) lit = 128; }
  }
  int litLev = glow * lit >> 8;                                   // fill brightness
  int oLev = qa_outline * fadeIn >> 8;
  switch (qa_style) {
    case QS_POINTS: {
      int sp = s >= 40 ? 4 : (s >= 20 ? 3 : 2);
      if (qa_dots > 0.66f && sp > 2) sp--;
      int lev = (glow - 3 - (s < 10 ? 1 : 0)) * fadeIn >> 8;
      if (litLev > lev) lev = litLev + 1;
      for (int yy = y + sp / 2; yy < y + s; yy += sp) qa_hline(x + sp / 2, x1, yy, sp, hue, lev);
      if (oLev > 0) { qa_put(x, y, 0, oLev); qa_put(x1, y, 0, oLev); qa_put(x, y1, 0, oLev); qa_put(x1, y1, 0, oLev); }
      break;
    }
    case QS_MONDRIAN: {
      int pick = (int)(h % 7u);
      int hc = pick == 0 ? 1 : (pick == 1 ? 2 : (pick == 2 ? 3 : 0));
      if (s < 10) hc = 0;
      qa_fill(x + 2, y + 2, x1, y1, hc, ((hc == 0 ? glow + 1 : glow) * fadeIn >> 8), 0);
      break;
    }
    case QS_HEAT: {
      int lev = (6 + v * (glow - 5) / 255) * fadeIn >> 8;
      int hh = 1 + qa_clamp(((qa_look(qa_cg, cx, cy) + v) >> 1) * 6 / 256, 0, 5);
      qa_fill(x + 1, y + 1, x1, y1, hh, lev, s >= 10 ? 0 : 1);
      if (oLev > 0 && s >= 10) { qa_hline(x, x1, y, 2, 0, oLev - 4); qa_vline(x, y, y1, 2, 0, oLev - 4); }
      break;
    }
    case QS_GLYPH: {
      int r2 = s / 2 - 2; if (r2 < 1) r2 = 1;
      int g = (int)((h >> 3) % 5u);
      int lev = (lit > 128 ? glow : glow - 4) * fadeIn >> 8, hh = lit > 128 ? hue : 0;
      if (g == 0) { qa_hline(cx - r2, cx + r2, cy, 1, hh, lev); qa_vline(cx, cy - r2, cy + r2, 1, hh, lev); }
      else if (g == 1) { qa_line(cx - r2, cy - r2, cx + r2, cy + r2, hh, lev); qa_line(cx - r2, cy + r2, cx + r2, cy - r2, hh, lev); }
      else if (g == 2) { qa_hline(cx - r2, cx + r2, cy - r2, 1, hh, lev); qa_hline(cx - r2, cx + r2, cy + r2, 1, hh, lev); qa_vline(cx - r2, cy - r2, cy + r2, 1, hh, lev); qa_vline(cx + r2, cy - r2, cy + r2, 1, hh, lev); }
      else if (g == 3) { for (int a = 0; a < 1024; a += 64) qa_put(cx + (r2 * qa_isin(a + 256) >> 14), cy + (r2 * qa_isin(a) >> 14), hh, lev); }
      else qa_fill(cx - r2 / 2, cy - r2 / 2, cx + r2 / 2, cy + r2 / 2, hh, lev, 0);
      if (oLev > 0) qa_put(x, y, 7, oLev);
      break;
    }
    case QS_CIRCUIT: {
      if (s >= 10) {
        int m = s / 5;
        int lev = (lit > 128 ? glow : glow - 3) * fadeIn >> 8, hh = lit > 128 ? hue : 0;
        qa_hline(x + m, x1 - m, y + m, 1, hh, lev); qa_hline(x + m, x1 - m, y1 - m, 1, hh, lev);
        qa_vline(x + m, y + m, y1 - m, 1, hh, lev); qa_vline(x1 - m, y + m, y1 - m, 1, hh, lev);
        if (litLev > 0) qa_fill(x + m + 2, y + m + 2, x1 - m - 2, y1 - m - 2, hue, litLev - 5, 1);
        for (int p = y + m + 2; p < y1 - m; p += 3) { qa_put(x + m - 1, p, 0, lev - 2); qa_put(x1 - m + 1, p, 0, lev - 2); }   // pins
        int dir = (int)((h >> 5) & 3);
        if (dir == 0) qa_vline(cx, y1 - m, y1, 1, 0, lev - 3);
        else if (dir == 1) qa_hline(x1 - m, x1, cy, 1, 0, lev - 3);
        else if (dir == 2) qa_vline(cx, y, y + m, 1, 0, lev - 3);
        else qa_hline(x, x + m, cy, 1, 0, lev - 3);
      } else qa_put(cx, cy, lit > 128 ? hue : 0, glow - 2);
      break;
    }
    case QS_BEVEL: {                                              // raised tiles: light top-left, dark bottom-right
      if (s < 6) { qa_put(cx, cy, hue, litLev > 0 ? litLev : glow - 6); break; }
      int body = litLev > 0 ? litLev - 1 : (glow - 11) * fadeIn >> 8;
      int hh = litLev > 0 ? hue : 0;
      qa_fill(x + 2, y + 2, x1 - 2, y1 - 2, hh, body, s >= 20 ? 0 : 1);
      int hi = (litLev > 0 ? 15 : glow - 6) * fadeIn >> 8, lo = (litLev > 0 ? 4 : 2) * fadeIn >> 8;
      qa_hline(x + 1, x1 - 1, y + 1, 1, hh, hi); qa_vline(x + 1, y + 1, y1 - 1, 1, hh, hi);
      qa_hline(x + 1, x1 - 1, y1 - 1, 1, hh, lo); qa_vline(x1 - 1, y + 1, y1 - 1, 1, hh, lo);
      break;
    }
    case QS_RINGS: {
      int r2 = (s >> 1) - 1; if (r2 < 1) r2 = 1;
      int lev = (litLev > 0 ? litLev + 1 : glow - 3) * fadeIn >> 8, hh = litLev > 0 ? hue : (qa_mixQ > 60 ? hue : 0);
      int n = r2 < 6 ? 16 : (r2 < 14 ? 32 : 64), stepA = 1024 / n;
      for (int a = 0; a < 1024; a += stepA) qa_put(cx + (r2 * qa_isin(a + 256) >> 14), cy + (r2 * qa_isin(a) >> 14), hh, lev);
      if (litLev > 0 && r2 > 3) { int r3 = r2 * lit >> 9; for (int a = 0; a < 1024; a += stepA * 2) qa_put(cx + (r3 * qa_isin(a + 256) >> 14), cy + (r3 * qa_isin(a) >> 14), hh, lev - 2); }
      qa_put(cx, cy, 0, lev);
      break;
    }
    case QS_BARS: {                                               // a level bar rising from the cell's floor
      if (s < 8) { qa_put(cx, cy, hue, glow - 4); break; }
      int lvl = (v * (s - 2) >> 8) + ((s >> 2) * qa_isin(qa_tq * 6 + (int)(h & 1023)) >> 14);
      lvl = qa_clamp(lvl, 1, s - 2);
      int top = y1 - lvl;
      int hh = 1 + qa_clamp((y1 - top) * 6 / s, 0, 5);
      qa_fill(x + 1, top, x1 - 1, y1 - 1, (qa_mixQ > 128 ? hh : hue), (glow - 2) * fadeIn >> 8, s >= 10 ? 0 : 1);
      qa_hline(x + 1, x1 - 1, top - 1, 1, 0, glow + 2);           // the peak cap
      if (oLev > 0) qa_hline(x, x1, y1, 2, 7, oLev - 2);
      break;
    }
    case QS_MIXED: {                                              // big: outlined + corner points; mid: clusters; small: points
      if (s >= 30) {
        if (oLev > 0) {
          qa_hline(x, x1, y, dstep, 0, oLev); qa_hline(x, x1, y1, dstep, 0, oLev - 2);
          qa_vline(x, y, y1, dstep, 0, oLev); qa_vline(x1, y, y1, dstep, 0, oLev - 2);
          qa_fill(x, y, x + 1, y + 1, 0, oLev + 3, 0); qa_fill(x1 - 1, y1 - 1, x1, y1, 0, oLev + 3, 0);
        }
        if (litLev > 0) qa_fill(x + 3, y + 3, x1 - 2, y1 - 2, hue, litLev - 2, 2);
      } else if (s >= 10) {
        if (litLev > 0) qa_fill(x + 1, y + 1, x1 - 1, y1 - 1, hue, litLev, 0);
        else if (oLev > 0) { qa_put(x, y, 0, oLev - 2); qa_put(x1, y1, 0, oLev - 4); }
      } else {
        qa_put(cx, cy, litLev > 0 ? hue : 0, litLev > 0 ? litLev + 1 : (glow - 5) * fadeIn >> 8);
      }
      break;
    }
    default: {                                                    // classic: dotted outlines + coloured clusters
      int big = s >= 20;
      if (oLev > 0) {
        int lev = oLev - (big ? 0 : 2);
        qa_hline(x, x1, y, dstep, 0, lev); qa_hline(x, x1, y1, dstep, 0, lev - 2);
        qa_vline(x, y, y1, dstep, 0, lev); qa_vline(x1, y, y1, dstep, 0, lev - 2);
      }
      if (litLev > 0) {
        int pat = big ? (qa_dots > 0.5f ? 1 : 2) : 0;
        qa_fill(x + 2, y + 2, x1 - 1, y1 - 1, hue, litLev - (big ? 1 : 0), pat);
        if (big) { qa_put(x + 2, y + 2, 0, 15); qa_put(x1 - 1, y1 - 1, 0, 15); }   // bright corner points
      }
      break;
    }
  }
}

static void qa_cell(int x, int y, int s, int L, int fadeIn) {
  if (x >= W || y >= H || x + s <= 0 || y + s <= 0) return;
  int m = (L >= qa_depth || s <= qa_minS) ? -1 : qa_splitM(x, y, s, L);
  if (m > 0) {
    int f = m * 24; if (f > 256) f = 256;                        // children fade in as the push grows
    int hs = s / 2;
    qa_cell(x, y, hs, L + 1, f); qa_cell(x + hs, y, s - hs, L + 1, f);
    qa_cell(x, y + hs, hs, L + 1, f); qa_cell(x + hs, y + hs, s - hs, L + 1, f);
    if (f < 256) qa_leaf(x, y, s, L, 256 - f);                   // and the parent fades out under them
  } else qa_leaf(x, y, s, L, fadeIn);
}

// ─── Knobs ────────────────────────────────────────────────────────────
static float qa_sm[16];
static bool qa_smReady = false;
static inline int qa_k(int i) { int v = (int)qa_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_quanta_name() { return "QUANTA"; }
const char* prog_quanta_character() { return "A multi-scale quantized grid that merges and splits in waves, drawn as point clouds, tiles, rings and bars"; }

static const char* const qa_presetNames[] = {
  "Quantize", "Sweep", "Point Cloud", "Ripple", "Mondrian", "Rain",
  "Circuit", "Heatmap", "Bevel", "Rings", "Kaleido", "Equalizer"
};
#define QA_NUM_PRESETS 12
// per preset: drawing style, cutting rule, extras, own palette, mirrored, scroll direction (x, y px/s)
static const uint8_t qa_pStyle[QA_NUM_PRESETS]  = {QS_MIXED, QS_CLASSIC, QS_POINTS, QS_CLASSIC, QS_MONDRIAN, QS_GLYPH,
                                                   QS_CIRCUIT, QS_HEAT, QS_BEVEL, QS_RINGS, QS_MIXED, QS_BARS};
static const uint8_t qa_pCtrl[QA_NUM_PRESETS]   = {0, 1, 0, 2, 0, 0, 0, 0, 3, 2, 0, 0};
static const uint8_t qa_pFlags[QA_NUM_PRESETS]  = {QF_TWINKLE, QF_CYCLE, QF_SCROLL | QF_TWINKLE, QF_TWINKLE, 0, QF_SCROLL,
                                                   QF_PULSE, QF_CYCLE, QF_TWINKLE, QF_CYCLE, QF_CYCLE | QF_TWINKLE, QF_CYCLE};
static const uint8_t qa_pPal[QA_NUM_PRESETS]    = {4, 1, 4, 4, 5, 3, 2, 1, 7, 4, 1, 1};
static const uint8_t qa_pMirror[QA_NUM_PRESETS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0};
// per preset: extra push toward coarse (−) or fine (+), and how many more cells light up (×1/64)
static const int8_t qa_pBias[QA_NUM_PRESETS]   = {0, 0, 0, 0, -45, 0, 0, 0, -20, -25, 0, -30};
static const uint8_t qa_pFillK[QA_NUM_PRESETS] = {64, 64, 64, 64, 64, 64, 64, 64, 120, 110, 64, 64};
static const int8_t qa_pScrollX[QA_NUM_PRESETS] = {0, 0, 14, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const int8_t qa_pScrollY[QA_NUM_PRESETS] = {0, 0, 6, 0, 0, 40, 0, 0, 0, 0, 0, 0};

const char* prog_quanta_presetName(int preset) {
  if (preset >= 0 && preset < QA_NUM_PRESETS) return qa_presetNames[preset];
  return NULL;
}
static const char* const qa_labels[16] = {
  "Palette", "Speed", "Block Size", "Depth", "Detail", "Blend", "Trails", "Fill",
  "Dots", "Field Scale", "Colour Mix", "Glow", "Flow X", "Flow Y", "Outlines", "Events"
};
const char* prog_quanta_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 16) return qa_labels[pot];
  return "";
}
uint8_t prog_quanta_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // we fade the last frame ourselves (trails)
}

void prog_quanta_init() {
  static bool ready = false;
  if (!ready) {
    ready = true;
    qa_levK[0] = 0;
    for (int l = 1; l < 16; l++) qa_levK[l] = (int16_t)(256.0f * (0.12f + 0.88f * powf(l / 15.0f, 0.9f)));
  }
  if (display.getBuffer()) memset(display.getBuffer(), 0, W * H);
}

static void qa_burst(float x, float y) {                         // start a ring in the oldest slot
  int o = 0;
  for (int b = 1; b < QA_NB; b++) if (qa_bt[b] > qa_bt[o]) o = b;
  qa_bx[o] = x; qa_by[o] = y; qa_bt[o] = 0;
}

void prog_quanta_draw(int preset) {
  qa_buf = display.getBuffer();
  if (!qa_buf) return;
  if (preset < 0 || preset >= QA_NUM_PRESETS) preset = 0;
  static int lastPreset = -1;
  if (preset != lastPreset) { memset(qa_buf, 0, W * H); lastPreset = preset; }

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  {
    float a = dt / 0.12f;
    if (a > 1.0f) a = 1.0f;
    for (int i = 0; i < 16; i++) {
      if (!qa_smReady) qa_sm[i] = pots[i];
      else qa_sm[i] += (pots[i] - qa_sm[i]) * a;
    }
    qa_smReady = true;
  }

  // ── knobs ──
  float sk = qa_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.15f * powf(14.0f, (sk - 0.03f) / 0.97f);   // centre ≈ 0.6×
  qa_style = qa_pStyle[preset]; qa_ctrl = qa_pCtrl[preset]; qa_flags = qa_pFlags[preset];
  int pal = (qa_pPal[preset] + qa_k(0) * 8 / 1024 - 4 + 8) % 8;   // centre = the preset's own
  static const int sizes[4] = {40, 60, 80, 120};
  int S0 = sizes[qa_clamp(qa_k(2) * 4 / 1024, 0, 3)];
  { int dk = qa_k(3); qa_depth = dk < 400 ? 2 : (dk < 700 ? 3 : (dk < 900 ? 4 : 5)); }   // at least two levels of cutting; centre = 3
  { int dk = qa_k(4); qa_detail = dk < 512 ? 60 + dk * 68 / 512 : 128 + (dk - 512) * 128 / 511; }   // far left still splits a little; centre as before
  qa_blendQ = qa_k(5) / 4;
  int trails = qa_k(6);
  { int fk = qa_k(7); qa_fillQ = fk < 512 ? 70 + fk * 58 / 512 : 128 + (fk - 512) * 128 / 511; }   // never fully dark; centre as before
  qa_dots = qa_k(8) / 1023.0f;
  qa_scale = 30.0f * powf(4.0f, (512 - qa_k(9)) / 512.0f);      // 7.5 … 120 px features
  qa_mixQ = 40 + qa_k(10) * 216 / 1023;                          // at least a little colour variety
  qa_glow = 8 + qa_k(11) * 7 / 1024;
  float flx = (qa_k(12) - 512) / 512.0f, fly = (qa_k(13) - 512) / 512.0f;
  if (flx > -0.04f && flx < 0.04f) flx = 0;
  if (fly > -0.04f && fly < 0.04f) fly = 0;
  int ok = qa_k(14);
  qa_outline = 6 + ok * 9 / 1023;                                // outlines always on (dim … bright)
  int ek = qa_k(15);
  float eventRate = ek < 60 ? 0.0f : 0.08f + (ek - 60) / 963.0f * 0.9f;   // automatic bursts per second
  qa_buildPalette(pal);

  // ── clocks ──
  qa_t += dt * speed;
  if (qa_t > 10000.0f) qa_t -= 10000.0f;
  qa_tq = (int)(qa_t * 1024.0f / 6.2831853f) & 0xFFFFF;
  qa_fx += dt * speed * (12.0f + flx * 60.0f);
  qa_fy += dt * speed * (fly * 60.0f - 4.0f - (qa_ctrl == 0 && preset == 5 ? 0.0f : 0.0f));
  if (preset == 5) qa_fy -= dt * speed * 40.0f;                   // Rain: the cutting falls
  qa_fz += dt * speed * 0.18f;
  if (qa_flags & QF_CYCLE) qa_cshift += dt * speed * 30.0f;      // colour clusters drift through the hues
  if (qa_fx > 1.0e5f || qa_fx < -1.0e5f) qa_fx = 0;
  if (qa_fy > 1.0e5f || qa_fy < -1.0e5f) qa_fy = 0;
  if (qa_fz > 1.0e4f) qa_fz -= 1.0e4f;
  if (qa_cshift > 1.0e5f) qa_cshift = 0;
  static float scx = 0, scy = 0;                                 // sliding grids
  scx += dt * speed * qa_pScrollX[preset]; scy += dt * speed * qa_pScrollY[preset];
  if (scx > 1.0e5f) scx = 0;
  if (scy > 1.0e5f) scy = 0;
  // the overall push: Detail + the quantization blend wobble
  qa_bias = (qa_detail - 128) + (qa_blendQ * qa_isin(qa_tq * 9 / 10) >> 15) + qa_pBias[preset];
  qa_fillQ = qa_clamp(qa_fillQ * qa_pFillK[preset] >> 6, 0, 400);

  // ── bursts: k12 from the centre; Events adds them at random ──
  static bool k12Was = false;
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !k12Was) qa_burst(HALFW, HALFH);
  k12Was = k12;
  static uint32_t rng = 99u;
  if (eventRate > 0 && speed > 0) {
    rng = rng * 1664525u + 1013904223u;
    if ((rng >> 8) * (1.0f / 16777216.0f) < eventRate * dt * (0.5f + speed)) {
      rng = rng * 1664525u + 1013904223u; float bx = (float)((rng >> 8) % W);
      rng = rng * 1664525u + 1013904223u; float by = (float)((rng >> 8) % H);
      qa_burst(bx, by);
    }
  }
  for (int b = 0; b < QA_NB; b++) if (qa_bt[b] < 9.0f) qa_bt[b] += dt * (0.4f + speed);

  // ── trails: fade (real-time rate) ──
  {
    static float fadeAcc = 0;
    float tr = 1.0f - trails / 1023.0f;
    fadeAcc += dt * (6.0f + tr * tr * 900.0f);
    int dec = (int)fadeAcc;
    if (dec > 15) dec = 15;
    fadeAcc -= (int)fadeAcc;
    if (dec > 0) qa_fade(dec);
  }

  // ── the field, once ──
  qa_buildField();

  // ── background grid (Outlines past ~80 %) ──
  if (ok > 820) {
    int lev = 1 + (ok - 820) * 3 / 203;
    int gs = S0 / 4 < 10 ? 10 : S0 / 4;
    int ox = (W % gs) / 2, oy = (H % gs) / 2;
    for (int y = oy; y < H; y += gs) qa_hline(0, W - 1, y, 4, 7, lev);
    for (int x = ox; x < W; x += gs) qa_vline(x, 0, H - 1, 4, 7, lev);
  }

  // ── the grid of blocks ──
  int mirror = qa_pMirror[preset];
  int Wd = mirror ? HALFW : W, Hd = mirror ? HALFH : H;
  int offx = ((int)scx) % S0, offy = ((int)scy) % S0;
  int nx = (Wd + S0 - 1) / S0 + 1, ny = (Hd + S0 - 1) / S0 + 1;
  int ox = (mirror ? HALFW - (nx - 1) * S0 : (W - (nx - 1) * S0) / 2) - S0 + offx;
  int oy = (mirror ? HALFH - (ny - 1) * S0 : (H - (ny - 1) * S0) / 2) - S0 + offy;
  if (!mirror && offx == 0 && offy == 0 && qa_pScrollX[preset] == 0 && qa_pScrollY[preset] == 0) { ox += S0; oy += S0; nx--; ny--; }
  for (int gy = 0; gy <= ny; gy++) for (int gx = 0; gx <= nx; gx++) {
    int bx = ox + gx * S0, by = oy + gy * S0;
    if (mirror && (bx >= HALFW || by >= HALFH)) continue;
    qa_cell(bx, by, S0, 0, 256);
  }
  if (mirror) {                                                  // copy the top-left quarter to the others
    for (int y = 0; y < HALFH; y++) {
      uint8_t* r = qa_buf + y * W;
      for (int x = 0; x < HALFW; x++) r[W - 1 - x] = r[x];
      memcpy(qa_buf + (H - 1 - y) * W, r, W);
    }
  }
}
