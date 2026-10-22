// =====================================================================
// PROGRAM: FLOWER — compass drawings of sacred circles (by Dewey) — Set 6, slot 1
//
// Circles drawn the old way, one sweep of the compass at a time: the Seed of
// Life growing into the Flower of Life, the Torus, the Fruit of Life, a
// lotus, a sunflower's spiral, ripples on a pond … Each figure is drawn in
// glowing light — where circles cross the light adds up, so the petals and
// vesicas between them shine — then it turns, breathes and sends out echo
// rings, fades, and the next drawing begins.
//
// Presets: k0 Seed of Life (grows to the Flower, then the Fruit) · k1 Torus ·
//          k2 Lotus · k3 Fruit of Life · k4 Sunflower · k5 Ripples ·
//          k6 Flower Field · k7 Egg of Life (turning in 3-D) · k8 Mandala ·
//          k9 Golden Spiral · k10 Rosette (2 to 12 petals) · k11 Tour
//   k12 — STRIKE: every circle rings out an echo
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Drawing Speed · p6 Glow · p7 Trails · p8 Spin · p9 Breathing
//   p10 Echoes · p11 Line Brightness · p12-p15 live FX
// =====================================================================
#include "vg6.h"

struct FlLook { uint8_t pal, size, spin, breath, echo, glow, trail, flow, spread; };
static const FlLook fw_looks[11] = {
  // pal size spin breath echo glow trail flow spread
  {  0, 28,  10, 30, 30, 2, 55,  6, 60 },   // Seed of Life
  {  4, 55,  25, 30, 20, 1, 70,  8, 80 },   // Torus
  {  3, 40,  15, 50, 30, 2, 60,  5, 60 },   // Lotus
  {  9, 38,  8,  20, 40, 2, 55,  4, 40 },   // Fruit of Life
  {  6, 64,  20, 30, 10, 1, 65,  6, 70 },   // Sunflower
  {  2, 50,  0,  20, 0,  1, 75,  6, 70 },   // Ripples
  {  8, 42,  6,  30,  0, 0, 50,  7, 90 },   // Flower Field
  { 10, 70,  0,  20, 20, 2, 60,  6, 70 },   // Egg of Life
  {  1, 45,  0,  40, 30, 2, 55,  5, 60 },   // Mandala
  {  5, 40,  0,  20, 20, 1, 70,  8, 60 },   // Golden Spiral
  { 11, 52,  10, 40, 30, 2, 60,  8,100 },   // Rosette
};
static const char* const fw_names[12] = { "Seed of Life", "Torus", "Lotus", "Fruit of Life", "Sunflower", "Ripples", "Flower Field", "Egg of Life", "Mandala", "Golden Spiral", "Rosette", "Tour" };
static const char* const fw_lbl[7] = { "Drawing Speed", "Glow", "Trails", "Spin", "Breathing", "Echoes", "Line Brightness" };
const char* prog_flower_name() { return "FLOWER"; }
const char* prog_flower_character() { return "Compass drawings of sacred circles: Seed and Flower of Life, Torus, Lotus, drawn in glowing light"; }
const char* prog_flower_presetName(int p) { return (p >= 0 && p < 12) ? fw_names[p] : NULL; }
const char* prog_flower_potLabel(int p, int pot) { (void)p; return v6_label(pot, fw_lbl); }
uint8_t prog_flower_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// ─── The figure: circles in figure units (R = 256) ────────────────────
#define FL_MAX 140
static int16_t* fw_x; static int16_t* fw_y; static int16_t* fw_r; static uint8_t* fw_h; static int8_t* fw_z;
static int fw_n = 0;
static void fw_mem() {
  fw_x = (int16_t*)v6_mem; fw_y = fw_x + FL_MAX; fw_r = fw_y + FL_MAX;
  fw_h = (uint8_t*)(fw_r + FL_MAX); fw_z = (int8_t*)(fw_h + FL_MAX);
}
static inline void fw_add(int x, int y, int r, int h, int z) {
  if (fw_n >= FL_MAX) return;
  fw_x[fw_n] = (int16_t)v3_clamp(x, -32000, 32000); fw_y[fw_n] = (int16_t)v3_clamp(y, -32000, 32000);
  fw_r[fw_n] = (int16_t)v3_clamp(r, 0, 32000); fw_h[fw_n] = (uint8_t)h; fw_z[fw_n] = (int8_t)z; fw_n++;
}
// the hexagonal lattice points within hex distance k of the centre, ring by ring
static void fw_hexRings(int k, int R, bool outer) {
  for (int ring = 0; ring <= k; ring++) {
    if (ring == 0) { fw_add(0, 0, R, 0, 0); continue; }
    // walk the ring: start at (ring, 0) in axial coordinates, six sides of `ring` steps
    static const int8_t dq[6] = { -1, -1, 0, 1, 1, 0 }, dr[6] = { 1, 0, -1, -1, 0, 1 };
    int q = ring, r = 0;
    for (int side = 0; side < 6; side++) for (int s = 0; s < ring; s++) {
      fw_add((R * (2 * q + r)) / 2, (R * r * 443) / 512, R, ring * 3, 0);   // 443/512 ≈ √3/2
      q += dq[side]; r += dr[side];
    }
  }
  if (outer) fw_add(0, 0, R * (k + 1), 9, 0);
}

static int fw_cycle = 0;           // how many drawings so far (Seed of Life grows with it)
static float fw_spinA = 0, fw_sz, fw_spin, fw_breath, fw_echo, fw_glow, fw_trail, fw_strike = 0, fw_ph = 0, fw_lastPh = 0;

// build the figure for look `lk` at time t (seconds of this drawing)
static void fw_build(int lk, float t) {
  fw_n = 0;
  const int R = 256;
  switch (lk) {
    case 0: fw_hexRings(1 + (fw_cycle % 3), R, true); break;                       // Seed → Flower → bigger Flower
    case 1: {                                                                       // Torus: circles round a ring
      int n = 18 + 6 * (fw_cycle % 3);
      for (int i = 0; i < n; i++) { int a = i * 1024 / n; fw_add((v3_cos(a) * R) >> 14, (v3_sin(a) * R) >> 14, R, (i * 16) / n, 0); }
      break;
    }
    case 2: {                                                                       // Lotus: layers of petals turning both ways
      for (int L = 1; L <= 4; L++) {
        int n = 6 * L, d = L * 180, r = 120 + L * 40, off = (int)(t * 18 * ((L & 1) ? 1 : -1)) + (L & 1) * (512 / n);
        for (int i = 0; i < n; i++) { int a = i * 1024 / n + off; fw_add((v3_cos(a) * d) >> 14, (v3_sin(a) * d) >> 14, r, L * 3, 0); }
      }
      fw_add(0, 0, 150, 0, 0);
      break;
    }
    case 3: {                                                                       // Fruit of Life: 13 touching circles
      int r = 110;
      fw_add(0, 0, r, 0, 0);
      for (int k = 1; k <= 2; k++) for (int i = 0; i < 6; i++) { int a = i * 1024 / 6; fw_add((v3_cos(a) * r * 2 * k) >> 14, (v3_sin(a) * r * 2 * k) >> 14, r, k * 4, 0); }
      if (fw_cycle & 1) for (int i = 0; i < 6; i++) { int a = i * 1024 / 6 + 85; fw_add((v3_cos(a) * r * 3) >> 14, (v3_sin(a) * r * 3) >> 14, r / 2, 12, 0); }
      break;
    }
    case 4: {                                                                       // Sunflower: the golden angle
      int n = 90 + 20 * (fw_cycle % 3);
      for (int i = 1; i <= n; i++) {
        int a = (i * 391) & 1023, d = (int)(36.0f * sqrtf((float)i));               // 391/1024 of a turn ≈ 137.5°
        fw_add((v3_cos(a) * d) >> 14, (v3_sin(a) * d) >> 14, 16 + i / 3, (i * 3) >> 3, 0);
      }
      break;
    }
    case 5: {                                                                       // Ripples: seven stones in a pond
      for (int c = 0; c < 7; c++) {
        int cx = 0, cy = 0;
        if (c) { int a = (c - 1) * 1024 / 6; cx = (v3_cos(a) * 300) >> 14; cy = (v3_sin(a) * 300) >> 14; }
        for (int w = 0; w < 4; w++) {
          float e = t * 0.35f + w * 0.25f + c * 0.13f; e -= floorf(e);
          fw_add(cx, cy, 10 + (int)(e * 700), c * 2, (int)((1.0f - e) * 100));        // z = brightness for ripples
        }
      }
      break;
    }
    case 6: {                                                                       // Flower Field: a lattice drifting past
      int sp = R, ox = (int)(t * 40) % sp, oy = (int)(t * 23) % (sp * 2);
      for (int j = -4; j <= 4; j++) for (int i = -5; i <= 5; i++) {
        int x = i * sp + ((j & 1) ? sp / 2 : 0) - ox, y = j * (sp * 443 / 512) - oy + sp;
        fw_add(x, y, R, (unsigned)(i * 3 + j * 5) & 15, 0);
      }
      break;
    }
    case 7: {                                                                       // Egg of Life: 8 circles turning in 3-D
      float a = t * 0.5f, b = t * 0.31f;
      float ca = cosf(a), sa = sinf(a), cb = cosf(b), sb = sinf(b);
      for (int i = 0; i < 8; i++) {
        float x, y, z;
        if (i < 6) { float th = i * 1.0471976f; x = cosf(th) * 1.15f; y = sinf(th) * 1.15f; z = (i & 1) ? 0.55f : -0.55f; }
        else { x = 0; y = 0; z = i == 6 ? 1.1f : -1.1f; }
        float x2 = x * ca - z * sa, z2 = x * sa + z * ca;
        float y2 = y * cb - z2 * sb, z3 = y * sb + z2 * cb;
        float p = 3.0f / (3.0f + z3);
        fw_add((int)(x2 * p * 180), (int)(y2 * p * 180), (int)(150 * p), i * 2, (int)(z3 * 40));
      }
      break;
    }
    case 8: {                                                                       // Mandala: rings of circles, counter-turning
      for (int k = 1; k <= 4; k++) {
        int n = 6 * k, d = k * 150, r = 70 + 22 * k, off = (int)(t * 12 * ((k & 1) ? 1 : -1) * (5 - k));
        for (int i = 0; i < n; i++) { int a = i * 1024 / n + off; fw_add((v3_cos(a) * d) >> 14, (v3_sin(a) * d) >> 14, r, k * 4, 0); }
      }
      for (int k = 1; k <= 4; k++) fw_add(0, 0, k * 150 + 60, 2 + k * 4, 0);
      break;
    }
    case 9: {                                                                       // Golden Spiral: circles growing by φ, endlessly
      float z = t * 0.35f; z -= floorf(z);                                          // the zoom loops every quarter turn of growth
      for (int i = 0; i < 40; i++) {
        float k = i - 12 + z * 4;
        float d = 30.0f * powf(1.618034f, k / 4.0f);
        if (d > 3000) break;
        int a = (int)(k * 256) & 1023;
        fw_add((int)(v3_cos(a) * d / 16384), (int)(v3_sin(a) * d / 16384), (int)(d * 0.62f), i & 15, 0);
      }
      break;
    }
    default: {                                                                      // Rosette: n circles through the centre
      int n = 2 + (fw_cycle % 11);
      for (int i = 0; i < n; i++) { int a = i * 1024 / n; fw_add((v3_cos(a) * 200) >> 14, (v3_sin(a) * 200) >> 14, 200, (i * 16) / n, 0); }
      fw_add(0, 0, 400, 8, 0);
      break;
    }
  }
}

void prog_flower_init() {
  fw_mem(); v6_init();
  fw_cycle = 0; fw_ph = 0; fw_lastPh = 0; fw_spinA = 0;
  const FlLook& L = fw_looks[0];
  fw_sz = L.size; fw_spin = L.spin / 100.0f; fw_breath = L.breath / 100.0f; fw_echo = L.echo / 100.0f; fw_glow = L.glow; fw_trail = L.trail / 100.0f;
  if (display.getBuffer()) memset(display.getBuffer(), 0, W * H);
}

void prog_flower_draw(int preset) {
  int look = v6_begin(preset, 11);
  if (!v3_buf) return;
  fw_mem();
  const FlLook& L = fw_looks[look];
  float dt = v6_f.dt, sdt = v6_f.sdt;
  if (v6_f.fresh) { fw_ph = 0; fw_cycle = 0; }

  // ── knobs ──
  float drawSp = v3_centre(5) ? 1.0f : powf(4.0f, v3_bi(5));
  float tGlow = v3_centre(6) ? L.glow : v3_u(6) * 3.0f;
  float tTrail = v3_centre(7) ? L.trail / 100.0f : v3_u(7) * 0.95f;
  float tSpin = v3_centre(8) ? L.spin / 100.0f : v3_bi(8);
  float tBreath = v3_centre(9) ? L.breath / 100.0f : v3_u(9);
  float tEcho = v3_centre(10) ? L.echo / 100.0f : v3_u(10);
  int bright = v3_centre(11) ? 12 : 4 + (int)(v3_u(11) * 11);
  v6_ease(&fw_sz, (float)L.size, 2.0f); v6_ease(&fw_glow, tGlow, 3.0f); v6_ease(&fw_trail, tTrail, 3.0f);
  v6_ease(&fw_spin, tSpin, 2.0f); v6_ease(&fw_breath, tBreath, 2.0f); v6_ease(&fw_echo, tEcho, 2.0f);
  v6_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 90);

  // ── the drawing's life: draw (0-0.4), dance (0.4-0.85), fade (0.85-1) ──
  const float CYC = 22.0f;
  fw_ph += sdt * drawSp / CYC;
  if (fw_ph >= 1.0f) { fw_ph -= 1.0f; fw_cycle++; }
  float ph = fw_ph, t = fw_ph * CYC;
  fw_spinA += sdt * fw_spin * 0.4f;
  if (v6_k12hit) fw_strike = 1.0f;
  fw_strike -= dt * 1.2f; if (fw_strike < 0) fw_strike = 0;

  // ── fade the last frame (trails) ──
  int fade = 15 - (int)(fw_trail * 14.0f + 0.5f);
  if (ph > 0.85f) fade = v6_max(fade, 2 + (int)((ph - 0.85f) * 40));                // clear away at the end of a drawing
  if (fade >= 15) memset(v3_buf, 0, W * H); else v3_fadeLev(v6_max(fade, 1));

  fw_build(look, t);
  float build = v3_clampf(ph / 0.4f, 0.0f, 1.0f);
  float life = ph > 0.85f ? 1.0f - (ph - 0.85f) / 0.15f : 1.0f;
  float S = v6_zoom(1.0f) * fw_sz * 8.0f / 256.0f * (1.0f + fw_breath * 0.08f * sinf(v6_t * 0.9f));   // figure units → 1/8 px (R = fw_sz px)
  float ca = cosf(fw_spinA), sa = sinf(fw_spinA);
  int halo = (int)(fw_glow + 0.5f);
  int n = fw_n;
  for (int i = 0; i < n; i++) {
    // reveal: one circle after another, each swept round by the compass
    float rv = build * (n + 2) - i * 1.0f;
    if (look == 5 || look == 9 || look == 6) rv = 2.0f;                              // ripples, spiral and field are always whole
    if (rv <= 0) continue;
    int len = rv >= 1.0f ? 1024 : (int)(rv * 1024);
    float br = 1.0f + fw_breath * 0.12f * sinf(v6_t * 1.3f + i * 0.45f);
    float x = fw_x[i], y = fw_y[i];
    int cx = 160 * 8 + (int)((x * ca - y * sa) * S), cy = 120 * 8 + (int)((x * sa + y * ca) * S);
    int r = (int)(fw_r[i] * S * br);
    int lev = (int)(bright * life * (0.85f + 0.4f * fw_strike));
    if (look == 5) lev = (lev * fw_z[i]) / 100;                                       // ripples fade as they spread
    if (look == 7) lev = (lev * (60 - fw_z[i])) / 70;                                 // the egg: nearer is brighter
    int hue = (fw_h[i] + (int)(fw_spinA * 3)) & 15;
    int a0 = v3_atan(-(cy - 120 * 8), -(cx - 160 * 8)) - 256;                         // the compass starts on the side facing the centre
    v6_ring(cx, cy, r, a0, len, lev, hue, len < 1024 ? 0 : halo);
    if (len < 1024 && r > 8) {                                                        // the compass point: a bright spark where the line is being drawn
      int a = a0 + len;
      v6_dot(cx + ((v3_cos(a) * r) >> 14), cy + ((v3_sin(a) * r) >> 14), 2, 12, hue);
    }
    // echoes: rings that leave each circle and spread out, fading
    float ek = fw_echo + fw_strike;
    if (ek > 0.02f && len >= 1024 && r < 8 * 400) {
      float e = v6_t * 0.45f + i * 0.17f; e -= floorf(e);
      if (fw_strike > 0.01f) e = 1.0f - fw_strike;
      int el = (int)(lev * (1.0f - e) * v3_clampf(ek, 0, 1) * 0.6f);
      if (el > 0) v6_arc(cx, cy, (int)(r * (1.0f + e * 0.7f)), 0, 1024, el, (hue + 4) & 15);
    }
  }
  v6_end();
}
