// =====================================================================
// PROGRAM: KALEIDO — a liquid-light kaleidoscope (by Dewey) — Set 6, slot 4
//
// A kaleidoscope's mirrors (4, 6, 8 or 12 of them) turn whatever flows
// behind the glass into a mandala. Behind this glass: liquid light (warped
// waves of colour, like an oil-and-water light show), beads of coloured
// oil drifting and smearing, shards of glass tumbling, or rings of
// ripples. The mirrors stay still like a real kaleidoscope's tube; the
// world behind them turns and flows.
//
// How it works: the screen's four quarters mirror each other, so only one
// quarter is worked out. Each pixel is folded into the kaleidoscope's
// wedge with two to four reflections, and the wedge looks up its colour.
//
// Presets: k0 Oil Lamp · k1 Rose Glass · k2 Bead Garden · k3 Shard Temple ·
//          k4 Ripple Lotus · k5 Peacock Eye · k6 Ember Wheel · k7 Ice Palace ·
//          k8 Opal Drift · k9 Nebula Mandala · k10 Spectrum Bloom · k11 Tour
//   k12 — DROP: a splash of new colour falls into the light
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Mirrors (4 · 6 · 8 · 12; centre = preset) · p6 Flow · p7 Warp
//   p8 Turn · p9 Pulse · p10 Detail · p11 Trails (beads and shards)
//   p12-p15 live FX
// =====================================================================
#include "vg6.h"

enum { KL_LIQUID, KL_BEADS, KL_SHARDS, KL_RIPPLE };
// mir: 0 = 4, 1 = 6, 2 = 8, 3 = 12 mirrors · src: what's behind the glass
struct KlLook { uint8_t mir, src, pal, flow, warp, turn, pulse, detail, trail, cflow, spread; };
static const KlLook kl_looks[11] = {
  // mir src         pal flow warp turn pulse det trail cflow spread
  { 1, KL_LIQUID,  0, 40, 60, 15, 20, 45, 0,  6, 70 },   // Oil Lamp
  { 3, KL_LIQUID,  3, 30, 40, 10, 30, 60, 0,  5, 60 },   // Rose Glass
  { 1, KL_BEADS,   2, 45, 50, 20, 10, 50, 85, 4, 80 },   // Bead Garden
  { 2, KL_SHARDS,  1, 35, 40, 25, 15, 50, 70, 4, 60 },   // Shard Temple
  { 3, KL_RIPPLE,  6, 40, 30, 10, 40, 50, 0,  5, 60 },   // Ripple Lotus
  { 1, KL_RIPPLE,  2, 35, 60, 15, 30, 40, 0,  6, 90 },   // Peacock Eye
  { 2, KL_LIQUID,  5, 60, 70, 30, 20, 50, 0,  8, 50 },   // Ember Wheel
  { 3, KL_SHARDS,  4, 25, 30, 15, 10, 60, 80, 4, 50 },   // Ice Palace
  { 0, KL_BEADS,   7, 30, 40, 10, 20, 45, 90, 3, 60 },   // Opal Drift
  { 1, KL_LIQUID,  8, 35, 80, 20, 25, 55, 0,  7, 90 },   // Nebula Mandala
  { 2, KL_BEADS,  11, 50, 60, 25, 20, 50, 80, 9,100 },   // Spectrum Bloom
};
static const char* const kl_names[12] = { "Oil Lamp", "Rose Glass", "Bead Garden", "Shard Temple", "Ripple Lotus", "Peacock Eye", "Ember Wheel", "Ice Palace", "Opal Drift", "Nebula Mandala", "Spectrum Bloom", "Tour" };
static const char* const kl_lbl[7] = { "Mirrors", "Flow", "Warp", "Turn", "Pulse", "Detail", "Trails" };
const char* prog_kaleido_name() { return "KALEIDO"; }
const char* prog_kaleido_character() { return "A liquid-light kaleidoscope: oil, beads and glass shards folded by 4 to 12 mirrors into a mandala"; }
const char* prog_kaleido_presetName(int p) { return (p >= 0 && p < 12) ? kl_names[p] : NULL; }
const char* prog_kaleido_potLabel(int p, int pot) { (void)p; return v6_label(pot, kl_lbl); }
uint8_t prog_kaleido_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// ─── Memory ───────────────────────────────────────────────────────────
#define KL_T 128                                   // the bead/shard picture behind the glass: 128 × 128
static uint8_t* kl_tex;                            // level << 4 | hue
static uint8_t* kl_lut;                            // liquid/ripple value → colour (rebuilt each frame: colour cycling)
static int8_t* kl_sin;                             // a 256-step sine, ±127
static void kl_mem() { kl_tex = v6_mem; kl_lut = v6_mem + KL_T * KL_T; kl_sin = (int8_t*)(kl_lut + 1024); }

// ─── Beads and shards: things drifting behind the glass ───────────────
#define KL_NB 26
static float kl_bx[KL_NB], kl_by[KL_NB], kl_bvx[KL_NB], kl_bvy[KL_NB], kl_ba[KL_NB];
static uint8_t kl_bh[KL_NB], kl_bs[KL_NB];
static void kl_seed() {
  for (int i = 0; i < KL_NB; i++) {
    kl_bx[i] = v3_rnd(KL_T); kl_by[i] = v3_rnd(KL_T); kl_ba[i] = v3_rndf() * 6.28f;
    float a = v3_rndf() * 6.28f, s = 6 + v3_rndf() * 14;
    kl_bvx[i] = cosf(a) * s; kl_bvy[i] = sinf(a) * s;
    kl_bh[i] = (uint8_t)v3_rnd(16); kl_bs[i] = (uint8_t)(6 + v3_rnd(11));
  }
}
// a soft round bead, wrapping round the edges of the texture (brightest wins)
static void kl_bead(int cx, int cy, int r, int hue) {
  int r2 = r * r;
  for (int dy = -r; dy <= r; dy++) for (int dx = -r; dx <= r; dx++) {
    int d2 = dx * dx + dy * dy; if (d2 > r2) continue;
    int lev = 15 - (d2 * 11) / (r2 + 1);
    uint8_t* p = kl_tex + ((cy + dy) & (KL_T - 1)) * KL_T + ((cx + dx) & (KL_T - 1));
    if ((*p >> 4) < lev) *p = (uint8_t)((lev << 4) | hue);
  }
}
// a shard: a triangle of glass, filled flat, bright edge
static void kl_shard(float cx, float cy, float r, float a, int hue) {
  int xs[3], ys[3];
  for (int k = 0; k < 3; k++) { float b = a + k * 2.0944f + (k == 1 ? 0.5f : 0.0f); xs[k] = (int)(cx + cosf(b) * r); ys[k] = (int)(cy + sinf(b) * r * 0.8f); }
  int y0 = v6_min(ys[0], v6_min(ys[1], ys[2])), y1 = v6_max(ys[0], v6_max(ys[1], ys[2]));
  for (int y = y0; y <= y1; y++) {
    int xl = 9999, xr = -9999;
    for (int k = 0; k < 3; k++) {
      int j = (k + 1) % 3, ya = ys[k], yb = ys[j];
      if ((ya <= y && yb > y) || (yb <= y && ya > y)) { int x = xs[k] + (y - ya) * (xs[j] - xs[k]) / (yb - ya); if (x < xl) xl = x; if (x > xr) xr = x; }
    }
    for (int x = xl; x <= xr; x++) {
      uint8_t* p = kl_tex + (y & (KL_T - 1)) * KL_T + (x & (KL_T - 1));
      int lev = (x == xl || x == xr) ? 15 : 9;
      if ((*p >> 4) < lev) *p = (uint8_t)((lev << 4) | hue);
    }
  }
}
// fade the texture (trails), all four pixels of a word at once
static void kl_fade(int amt) {
  uint32_t* p = (uint32_t*)kl_tex; uint32_t sub = (uint32_t)amt * 0x01010101u;
  for (int i = 0; i < KL_T * KL_T / 4; i++) {
    uint32_t v = p[i], lev = (v >> 4) & 0x0F0F0F0Fu, hue = v & 0x0F0F0F0Fu;
    uint32_t d = (lev | 0x10101010u) - sub, keep = ((d >> 4) & 0x01010101u) * 0x0Fu;
    p[i] = ((d & keep) << 4) | hue;
  }
}

// ─── The mirrors: fold a point into the wedge ─────────────────────────
// Each fold reflects across a line through the centre at angle φ (Q14 sin, cos).
static int16_t kl_fs[3], kl_fc[3]; static int kl_nf = 0;
static void kl_setMirrors(int m) {
  static const float ang[4][3] = { {45, 0, 0}, {60, 30, 0}, {45, 22.5f, 0}, {60, 30, 15} };   // after the quarter fold
  static const int8_t n[4] = { 1, 2, 2, 3 };
  kl_nf = n[m];
  for (int i = 0; i < kl_nf; i++) { float a = ang[m][i] * 0.0174533f; kl_fs[i] = (int16_t)(sinf(a) * 16384); kl_fc[i] = (int16_t)(cosf(a) * 16384); }
}

// ─── The hot loop: one quarter of the screen ──────────────────────────
// src: what's behind the glass. ku: half-pixels → content units (×256). t*: time offsets.
__attribute__((optimize("O2"))) static void kl_quarter(int src, int ku, int ca, int sa, int t1, int t2, int t3, int warp, int lutOff) {
  const int8_t* S = kl_sin; const uint8_t* lut = kl_lut; const uint8_t* tex = kl_tex;
  int nf = kl_nf;
  int fs0 = kl_fs[0], fc0 = kl_fc[0], fs1 = kl_fs[1], fc1 = kl_fc[1], fs2 = kl_fs[2], fc2 = kl_fc[2];
  for (int y = 0; y < H / 2; y++) {
    uint8_t* row = v3_buf + y * W;
    int Y0 = 239 - 2 * y;                                         // distance from the centre, half-pixels
    for (int x = 0; x < W / 2; x++) {
      int X = 319 - 2 * x, Y = Y0;
      // the quarter fold is free (we are in one quarter); then the mirror lines
      int d = (-X * fs0 + Y * fc0) >> 14;
      if (d > 0) { X += (2 * d * fs0) >> 14; Y -= (2 * d * fc0) >> 14; }
      if (nf > 1) { d = (-X * fs1 + Y * fc1) >> 14; if (d > 0) { X += (2 * d * fs1) >> 14; Y -= (2 * d * fc1) >> 14; } }
      if (nf > 2) { d = (-X * fs2 + Y * fc2) >> 14; if (d > 0) { X += (2 * d * fs2) >> 14; Y -= (2 * d * fc2) >> 14; } }
      // turn the world behind the glass, and scale it
      int u = ((X * ca - Y * sa) >> 14) * ku >> 8, v = ((X * sa + Y * ca) >> 14) * ku >> 8;
      uint8_t c;
      if (src == KL_LIQUID) {                                     // warped waves (domain-warped plasma)
        int w1 = S[(v + t1) & 255], w2 = S[(u - t2) & 255];
        int a = S[(u + ((w1 * warp) >> 7) + t3) & 255] + S[(v + ((w2 * warp) >> 7) - t1) & 255] + S[(((u + v) >> 1) + t2) & 255];
        c = lut[(a + 384 + lutOff) & 1023];
      } else if (src == KL_RIPPLE) {                              // rings round the centre, bent by the wedge
        int r2 = (u * u + v * v) >> 6;
        int a = S[(r2 - t1) & 255] + S[(u + (S[(v + t2) & 255] * warp >> 7)) & 255] + (S[(v * 3 - t3) & 255] >> 1);
        c = lut[(a + 384 + lutOff) & 1023];
      } else if (src == KL_BEADS) {                               // beads: the picture behind the glass, as seen
        c = tex[((v + t1) & (KL_T - 1)) * KL_T + ((u - t2) & (KL_T - 1))];
      } else {                                                     // shards: the picture behind the glass, wrapped
        // round the wedge: across = the angle within the wedge (Y / X), along = the distance out
        int tv = ((Y << 7) / (X + 1)) * t3 >> 8;                  // (t3 here: the wedge's angle → 0 … 127)
        c = tex[((tv + t1) & (KL_T - 1)) * KL_T + ((((X * ku) >> 9) - t2) & (KL_T - 1))];
      }
      row[x] = c;
    }
  }
}

static float kl_flow, kl_warp, kl_turn, kl_pulse, kl_detail, kl_trail;
static float kl_ang = 0, kl_t1 = 0, kl_t2 = 0, kl_t3 = 0, kl_drop = 0;
static int kl_mirNow = 1, kl_srcNow = 0;

void prog_kaleido_init() {
  kl_mem(); v6_init(); kl_seed();
  for (int i = 0; i < 256; i++) kl_sin[i] = (int8_t)((v3_sin(i * 4) * 127) / 16384);
  memset(kl_tex, 0, KL_T * KL_T);
  kl_setMirrors(1);
}

void prog_kaleido_draw(int preset) {
  int look = v6_begin(preset, 11);
  if (!v3_buf) return;
  kl_mem();
  const KlLook& L = kl_looks[look];
  float dt = v6_f.dt, sdt = v6_f.sdt;
  int mir = v3_centre(5) ? L.mir : v3_clamp(v3_k(5) * 4 / 1024, 0, 3);
  float tFlow = v3_centre(6) ? L.flow / 100.0f : v3_u(6) * 1.2f;
  float tWarp = v3_centre(7) ? L.warp / 100.0f : v3_u(7) * 1.3f;
  float tTurn = v3_centre(8) ? L.turn / 100.0f : v3_bi(8);
  float tPulse = v3_centre(9) ? L.pulse / 100.0f : v3_u(9);
  float tDet = v3_centre(10) ? L.detail / 100.0f : v3_u(10);
  float tTrail = v3_centre(11) ? L.trail / 100.0f : v3_u(11);
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&kl_flow, tFlow, rate); v6_ease(&kl_warp, tWarp, rate); v6_ease(&kl_turn, tTurn, rate);
  v6_ease(&kl_pulse, tPulse, rate); v6_ease(&kl_detail, tDet, rate); v6_ease(&kl_trail, tTrail, rate);
  if (v6_first || v6_fade > 0.5f) { kl_srcNow = L.src; }
  if (mir != kl_mirNow || v6_first) { kl_mirNow = mir; kl_setMirrors(mir); }
  v6_colours(L.pal, L.cflow / 100.0f, L.spread / 100.0f, 120);

  // ── time ──
  float fl = sdt * kl_flow;
  kl_t1 += fl * 70; kl_t2 += fl * 53; kl_t3 += fl * 37;
  if (kl_t1 > 25600) kl_t1 -= 25600; if (kl_t2 > 25600) kl_t2 -= 25600; if (kl_t3 > 25600) kl_t3 -= 25600;
  kl_ang += sdt * kl_turn * 0.35f;
  if (v6_k12hit) kl_drop = 1.0f;
  kl_drop -= dt * 0.8f; if (kl_drop < 0) kl_drop = 0;

  // ── the colour table for liquid and ripples: value → level and hue, cycling ──
  for (int i = 0; i < 1024; i++) {
    int k = i - 384;                                              // −384 … 384 for the three waves summed (a little past either end)
    int band = (k + 768) & 127;                                   // bright bands where the waves meet
    int lev = band < 64 ? band / 4 : (127 - band) / 4;
    lev = v3_clamp(lev + 1, 1, 15);
    int hue = ((k + 768) >> 5) & 15;
    kl_lut[i] = (uint8_t)((lev << 4) | hue);
  }
  int lutOff = (int)(v6_t * 40 * kl_flow);

  // ── beads and shards ──
  if (kl_srcNow == KL_BEADS || kl_srcNow == KL_SHARDS) {
    int fd = 15 - (int)(kl_trail * 14.5f);
    if (kl_drop > 0.95f) fd = 6;
    kl_fade(v3_clamp(fd, 1, 15));
    int nb = 12 + (int)(kl_warp * 14);
    for (int i = 0; i < nb && i < KL_NB; i++) {
      kl_bx[i] += kl_bvx[i] * fl * 1.5f; kl_by[i] += kl_bvy[i] * fl * 1.5f; kl_ba[i] += fl * (i & 1 ? 1.3f : -0.9f);
      if (kl_bx[i] < 0) kl_bx[i] += KL_T; if (kl_bx[i] >= KL_T) kl_bx[i] -= KL_T;
      if (kl_by[i] < 0) kl_by[i] += KL_T; if (kl_by[i] >= KL_T) kl_by[i] -= KL_T;
      int hue = (kl_bh[i] + (int)(v6_t * 0.5f)) & 15;
      if (kl_srcNow == KL_BEADS) kl_bead((int)kl_bx[i], (int)kl_by[i], kl_bs[i] * (kl_drop > 0 ? 1 : 1) + (int)(kl_drop * 6), hue);
      else kl_shard(kl_bx[i], kl_by[i], kl_bs[i] * 1.6f, kl_ba[i], hue);
    }
    if (kl_drop > 0.9f) {                                          // a splash of new colour
      int h = v3_rnd(16);
      for (int k = 0; k < 6; k++) kl_bead(v3_rnd(KL_T), v3_rnd(KL_T), 6 + v3_rnd(10), (h + k) & 15);
    }
  }

  // ── draw one quarter, then mirror it ──
  float zoom = v6_zoom(1.0f) * (1.0f + kl_pulse * 0.25f * sinf(v6_t * 0.8f));
  int ku = (int)(256.0f * (0.25f + kl_detail * 1.1f) / zoom * (kl_srcNow >= KL_BEADS ? (kl_srcNow == KL_BEADS ? 0.6f : 0.6f) : 1.0f));
  if (kl_drop > 0 && kl_srcNow != KL_BEADS && kl_srcNow != KL_SHARDS) lutOff += (int)(kl_drop * kl_drop * 400);
  int ca = (int)(cosf(kl_ang) * 16384), sa = (int)(sinf(kl_ang) * 16384);
  if (kl_srcNow == KL_BEADS) {
    kl_quarter(kl_srcNow, (int)(ku * 1.2f), ca, sa, (int)(v6_t * 7 * kl_flow) & 127, (int)(v6_t * 11 * kl_flow) & 127, 0, 0, 0);
  } else if (kl_srcNow == KL_SHARDS) {
    static const float wedgeTan[4] = { 1.0f, 0.57735f, 0.41421f, 0.26795f };   // tan of each wedge's angle (45°, 30°, 22.5°, 15°)
    int angK = (int)(256.0f / (wedgeTan[kl_mirNow] * 1.0f));
    kl_quarter(kl_srcNow, ku, 16384, 0, (int)(kl_ang * 40) & 127, (int)(v6_t * 9 * kl_flow) & 127, angK, 0, 0);
  } else kl_quarter(kl_srcNow, ku, ca, sa, (int)kl_t1, (int)kl_t2, (int)kl_t3, (int)(kl_warp * 160), lutOff);
  for (int y = 0; y < H / 2; y++) {
    uint8_t* r = v3_buf + y * W;
    for (int x = 0; x < W / 2; x++) r[W - 1 - x] = r[x];
    memcpy(v3_buf + (H - 1 - y) * W, r, W);
  }
  v6_end();
}
