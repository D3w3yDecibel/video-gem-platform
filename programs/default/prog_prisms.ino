// =====================================================================
// PROGRAM: PRISMS — a field of hexagonal columns (by Dewey) — Set 6, slot 6
//
// A honeycomb of hexagonal pillars seen from above, like a pin-screen or
// the Giant's Causeway, rising and falling in patterns: ripples spreading
// from the centre, a breathing pyramid, waves crossing each other, rolling
// hills, a graphic equaliser, rain, a turning mandala … Each pillar is a
// solid six-sided prism with a lit top and shaded sides, and the whole
// field slowly turns.
//
// How it works: about 180 prisms, sorted far-to-near every frame and
// painted as filled polygons (a top hexagon and the two or three sides
// you can see).
//
// Presets: k0 Ripples · k1 Pyramid · k2 Interference · k3 Hills · k4 Equaliser ·
//          k5 Mandala · k6 Rain · k7 Swell · k8 Spiral · k9 City · k10 Bloom · k11 Tour
//   k12 — DROP: a stone falls into the field
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Height · p6 Waves (how many) · p7 Tilt (◄ flat on | from the side ►)
//   p8 Spin · p9 Gap (between the pillars) · p10 Shading · p11 Colour by (◄ height | ring ►)
//   p12-p15 live FX
// =====================================================================
#include "vg6.h"

struct PrLook { uint8_t pal, height, waves, tilt, spin, gap, shade, colBy, flow, spread; };
static const PrLook pr_looks[11] = {
  // pal ht waves tilt spin gap shade colBy flow spread
  {  0, 50, 50, 55, 10, 10, 50, 20, 5, 70 },   // Ripples
  {  9, 60, 30, 60, 15, 12, 60, 30, 4, 50 },   // Pyramid
  {  4, 45, 60, 55, 8,  10, 50, 20, 6, 80 },   // Interference
  {  6, 55, 40, 45, 6,  6,  60, 10, 4, 60 },   // Hills
  { 11, 70, 50, 50, 0,  18, 40, 10, 8,100 },   // Equaliser
  {  2, 45, 50, 62, 20, 10, 50, 80, 5, 80 },   // Mandala
  {  4, 40, 50, 55, 6,  10, 50, 20, 5, 70 },   // Rain
  {  3, 50, 35, 40, 10, 8,  60, 20, 5, 60 },   // Swell
  {  8, 50, 55, 58, 12, 10, 50, 50, 6, 90 },   // Spiral
  {  1, 75, 30, 50, 8,  22, 70, 80, 3, 60 },   // City
  { 10, 55, 50, 60, 10, 12, 45, 80, 7, 90 },   // Bloom
};
static const char* const pr_names[12] = { "Ripples", "Pyramid", "Interference", "Hills", "Equaliser", "Mandala", "Rain", "Swell", "Spiral", "City", "Bloom", "Tour" };
static const char* const pr_lbl[7] = { "Height", "Waves", "Tilt", "Spin", "Gap", "Shading", "Colour by" };
const char* prog_prisms_name() { return "PRISMS"; }
const char* prog_prisms_character() { return "A field of hexagonal pillars rising and falling in ripples, waves, pyramids and mandalas"; }
const char* prog_prisms_presetName(int p) { return (p >= 0 && p < 12) ? pr_names[p] : NULL; }
const char* prog_prisms_potLabel(int p, int pot) { (void)p; return v6_label(pot, pr_lbl); }
uint8_t prog_prisms_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// ─── The field ────────────────────────────────────────────────────────
#define PR_K 7                                   // hexagon of prisms, "radius" 7 cells → 169 prisms
#define PR_MAX 169
static int8_t pr_q[PR_MAX], pr_r[PR_MAX];
static float* pr_h;                              // heights (eased)
static float* pr_hv;                             // equaliser / city targets
static uint8_t* pr_ring;                         // hex distance from the centre
static int32_t* pr_depth; static uint8_t* pr_ord;
static int pr_n = 0;
static void pr_mem() {
  uint8_t* p = v6_mem;
  pr_h = (float*)p; p += PR_MAX * 4; pr_hv = (float*)p; p += PR_MAX * 4; pr_depth = (int32_t*)p; p += PR_MAX * 4;
  pr_ring = p; p += PR_MAX; pr_ord = p; p += PR_MAX;
}
// rain: a few drops spreading rings
#define PR_DROPS 6
static float pr_dx[PR_DROPS], pr_dy[PR_DROPS], pr_dt[PR_DROPS];

static float pr_height, pr_waves, pr_tilt, pr_spin, pr_gap, pr_shade, pr_colBy;
static float pr_ang = 0.3f, pr_tt = 0, pr_beatT = 0;
static int pr_lookNow = 0;

void prog_prisms_init() {
  pr_mem(); v6_init();
  pr_n = 0;
  for (int q = -PR_K; q <= PR_K; q++) for (int r = -PR_K; r <= PR_K; r++) {
    int s = -q - r; if (v3_abs(s) > PR_K) continue;
    int d = v6_max(v3_abs(q), v6_max(v3_abs(r), v3_abs(s)));
    pr_q[pr_n] = (int8_t)q; pr_r[pr_n] = (int8_t)r; pr_ring[pr_n] = (uint8_t)d; pr_h[pr_n] = 0; pr_hv[pr_n] = v3_rndf();
    pr_n++;
  }
  for (int i = 0; i < PR_DROPS; i++) pr_dt[i] = 99;
}

// the height (0 … 1) of the prism at world (x, y) (cell spacing 1), ring d, for look lk.
// Whole numbers throughout (positions in 1/256 cell, angles 1024 per turn): the RP2040 has no float unit.
static int32_t pr_T1, pr_T2, pr_K;                   // this frame's time phases and wave number
static inline int32_t pr_dist(int32_t x, int32_t y) { return (int32_t)v3_isqrt((uint32_t)(x * x + y * y)); }   // 1/256 cell
static float pr_field(int lk, int i, int32_t x, int32_t y, int d) {
  int32_t K = pr_K, h;                               // h: Q14 (16384 = 1)
  switch (lk) {
    case 0: h = 8192 + (v3_sin(((pr_dist(x, y) * K) >> 8) - pr_T1 * 2) >> 1); break;
    case 1: {
      int32_t b = 8192 + (v3_sin(pr_T1 * 9 / 10) >> 1);
      h = ((16384 - d * 16384 / PR_K) * (4915 + ((b * 11469) >> 14)) >> 14) + ((v3_sin(((d * K)) - pr_T1 * 3) * 2458 >> 14) * b >> 14);
      break;
    }
    case 2: {
      int32_t a = 0;
      for (int k = 0; k < 3; k++) {
        int ang = k * 341 + pr_T1 / 10;
        int32_t sx = (v3_cos(ang) * 5) >> 6, sy = (v3_sin(ang) * 5) >> 6;   // 5 cells out, in 1/256
        a += v3_sin(((pr_dist(x - sx, y - sy) * K) >> 8) - pr_T1 * 22 / 10);
      }
      h = 8192 + a / 6;
      break;
    }
    case 3: h = 8192 + (int32_t)(((int64_t)(v3_noise1((uint32_t)((x * 90 >> 8) + pr_T2 * 4), 11) + v3_noise1((uint32_t)((y * 90 >> 8) - pr_T2 * 3), 77)) * 16384) / 40000); break;
    case 4: return pr_hv[i];                                                     // equaliser: set by the beat
    case 5: { int th = v3_atan(y, x); h = 8192 + (v3_cos(6 * th + pr_T1 * 8 / 10) >> 2) * ((d & 1) ? 1 : -1) + (v3_sin(d * K * 8 / 10 - pr_T1 * 15 / 10) >> 2); break; }
    case 6: {
      h = 2458;
      for (int k = 0; k < PR_DROPS; k++) {
        if (pr_dt[k] > 6) continue;
        int32_t e = pr_dist(x - (int32_t)(pr_dx[k] * 256), y - (int32_t)(pr_dy[k] * 256)) - (int32_t)(pr_dt[k] * 1024);   // 1/256 cell
        if (e > -768 && e < 768) { int32_t f = (int32_t)((1.0f - pr_dt[k] / 6.0f) * 8192); h += ((v3_cos((e * 228) >> 8) * f) >> 14) * (768 - v3_abs(e)) / 768; }
      }
      break;
    }
    case 7: h = 8192 + (v3_sin(((x * K * 4 / 10) >> 8) + pr_T1 * 11 / 10) * 5734 >> 14) + (v3_sin(((y * K * 7 / 10) >> 8) - pr_T1 * 17 / 10) * 2458 >> 14); break;
    case 8: { int th = v3_atan(y, x); h = 8192 + (v3_sin(3 * th + ((pr_dist(x, y) * K * 8 / 10) >> 8) - pr_T1 * 2) >> 1); break; }
    case 9: return pr_hv[i] * (0.85f + 0.15f * v3_sin(pr_T1 / 2 + i * 163) / 16384.0f);   // city: towers, gently swaying
    default: h = 8192 + (int32_t)(((int64_t)v3_cos(d * K * 12 / 10 - pr_T1 * 25 / 10) * (9830 + (v3_sin(pr_T1 * 4 / 10) * 6554 >> 14))) >> 15); break;   // bloom: hex rings
  }
  return h / 16384.0f;
}

void prog_prisms_draw(int preset) {
  int look = v6_begin(preset, 11);
  if (!v3_buf) return;
  pr_mem();
  const PrLook& L = pr_looks[look];
  float dt = v6_f.dt, sdt = v6_f.sdt;
  float tH = v3_centre(5) ? L.height / 100.0f : v3_u(5);
  float tW = v3_centre(6) ? L.waves / 100.0f : v3_u(6);
  float tTilt = v3_centre(7) ? L.tilt / 100.0f : 0.1f + v3_u(7) * 0.85f;
  float tSpin = v3_centre(8) ? L.spin / 100.0f : v3_bi(8);
  float tGap = v3_centre(9) ? L.gap / 100.0f : v3_u(9) * 0.6f;
  float tShade = v3_centre(10) ? L.shade / 100.0f : v3_u(10);
  float tCol = v3_centre(11) ? L.colBy / 100.0f : v3_u(11);
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&pr_height, tH, rate); v6_ease(&pr_waves, tW, rate); v6_ease(&pr_tilt, tTilt, rate); v6_ease(&pr_spin, tSpin, rate);
  v6_ease(&pr_gap, tGap, rate); v6_ease(&pr_shade, tShade, rate); v6_ease(&pr_colBy, tCol, rate);
  if (v6_first || v6_fade > 0.5f) {
    if (L.colBy && pr_lookNow != look && (look == 9)) for (int i = 0; i < pr_n; i++) pr_hv[i] = 0.1f + 0.9f * v3_rndf() * v3_rndf();   // a new skyline
    pr_lookNow = look;
  }
  v6_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 100);
  pr_tt += sdt; pr_ang += sdt * pr_spin * 0.25f;

  // ── the equaliser's beat, the rain's drops, k12's stone ──
  pr_beatT += sdt;
  if (pr_lookNow == 4 && pr_beatT > 0.18f) { pr_beatT = 0; for (int i = 0; i < pr_n; i++) { float base = 1.0f - pr_ring[i] / (float)(PR_K + 1); pr_hv[i] = v3_clampf(base * (0.4f + 0.8f * v3_rndf()), 0, 1); } }
  for (int k = 0; k < PR_DROPS; k++) pr_dt[k] += sdt;
  if ((pr_lookNow == 6 && v3_rndf() < sdt * 1.5f) || v6_k12hit) {
    int k = 0; for (int j = 1; j < PR_DROPS; j++) if (pr_dt[j] > pr_dt[k]) k = j;
    float a = v3_rndf() * 6.28f, d = v6_k12hit ? 0.0f : v3_rndf() * 6.0f;
    pr_dx[k] = cosf(a) * d; pr_dy[k] = sinf(a) * d; pr_dt[k] = 0;
  }

  // ── heights, then the order to paint them in ──
  float ca = cosf(pr_ang), sa = sinf(pr_ang);
  pr_T1 = (int32_t)(pr_tt * 163.0f); pr_T2 = (int32_t)(pr_tt * 102.0f);   // 1024 per turn: t radians → t × 163
  pr_K = (int32_t)((0.5f + pr_waves * 1.2f) * 163.0f);                  // radians per cell → 1024ths
  for (int i = 0; i < pr_n; i++) {
    float x = pr_q[i] + pr_r[i] * 0.5f, y = pr_r[i] * 0.8660254f;
    int32_t xi = (pr_q[i] * 2 + pr_r[i]) * 128, yi = pr_r[i] * 222;      // 1/256 cell (222 = 256·√3/2)
    float h = pr_field(pr_lookNow, i, xi, yi, pr_ring[i]);
    if (pr_lookNow != 6) {                                          // a dropped stone ripples through any pattern
      for (int k = 0; k < PR_DROPS; k++) if (pr_dt[k] < 4) {
        int32_t e = pr_dist(xi - (int32_t)(pr_dx[k] * 256), yi - (int32_t)(pr_dy[k] * 256)) - (int32_t)(pr_dt[k] * 1280);
        if (e > -640 && e < 640) h += 0.45f * (1.0f - pr_dt[k] / 4.0f) * (640 - v3_abs(e)) / 640.0f * v3_cos((e * 212) >> 8) / 16384.0f;
      }
    }
    v6_ease(&pr_h[i], v3_clampf(h, -0.2f, 1.3f), 8.0f);
    pr_depth[i] = (int32_t)((x * sa + y * ca) * 256);               // screen-down after turning: bigger = nearer
    pr_ord[i] = (uint8_t)i;
  }
  for (int a = 1; a < pr_n; a++) {                                  // far to near (insertion sort: the order barely changes frame to frame)
    uint8_t v = pr_ord[a]; int32_t dv = pr_depth[v]; int b = a - 1;
    while (b >= 0 && pr_depth[pr_ord[b]] > dv) { pr_ord[b + 1] = pr_ord[b]; b--; }
    pr_ord[b + 1] = v;
  }

  // ── paint ──
  memset(v3_buf, 0, W * H);
  float zoom = v6_zoom(1.0f);
  float S = 16.5f * zoom;                                           // px per cell
  float ct = cosf(pr_tilt * 1.4f), st = sinf(pr_tilt * 1.4f);       // tilt: 0 = looking straight down
  float hs = pr_height * 9.0f;                                      // height in cells at h = 1
  float rho = (0.5773503f) * (1.0f - pr_gap);                       // corner distance of each hexagon (touching at gap 0)
  float yOff = 120.0f + st * hs * 0.5f * S * 0.6f;
  // the six corners, turned by the view, as screen offsets in 1/16 px (they're the same for every prism)
  int32_t ox[6], oy[6];
  float cyk[6];
  for (int k = 0; k < 6; k++) {
    float b = (k * 60 + 30) * 0.0174533f; float px = cosf(b) * rho, py = sinf(b) * rho;
    float cx = px * ca - py * sa; cyk[k] = px * sa + py * ca;
    ox[k] = (int32_t)(cx * S * 16); oy[k] = (int32_t)(cyk[k] * S * ct * 16);
  }
  // which sides face us (the same for every prism), and how they're lit
  int8_t sideLev[6];
  for (int k = 0; k < 6; k++) {
    int k2 = (k + 1) % 6;
    if (cyk[k] + cyk[k2] <= 0.02f) { sideLev[k] = -1; continue; }
    bool lit = (ox[k] + ox[k2]) < 0;                                // light from the upper left
    sideLev[k] = (int8_t)v3_clamp(10 - (int)(pr_shade * 8) + (lit ? 3 : 0) - (k & 1), 2, 13);
  }
  int32_t hk = (int32_t)(hs * S * st * 16);                         // screen height of h = 1, 1/16 px
  int32_t sx16 = (int32_t)(S * 16), syk = (int32_t)(S * ct * 16);
  int colA = (int)(pr_colBy * 512), colB = (int)((1.0f - pr_colBy) * 2560);
  bool rim = pr_shade > 0.15f;
  for (int oi = 0; oi < pr_n; oi++) {
    int i = pr_ord[oi];
    float x = pr_q[i] + pr_r[i] * 0.5f, y = pr_r[i] * 0.8660254f;
    int32_t bx0 = 160 * 16 + (int32_t)((x * ca - y * sa) * sx16), by0 = (int32_t)(yOff * 16) + (int32_t)((x * sa + y * ca) * syk);
    int32_t hp = (int32_t)(pr_h[i] * hk);
    int32_t tx[6], ty[6], by[6];
    for (int k = 0; k < 6; k++) { tx[k] = bx0 + ox[k]; by[k] = by0 + oy[k]; ty[k] = by[k] - hp; }
    int hue = ((pr_ring[i] * colA + (int)(pr_h[i] * colB)) >> 8) & 15;
    for (int k = 0; k < 6; k++) {
      if (sideLev[k] < 0) continue;
      int k2 = (k + 1) % 6;
      int32_t qx[4] = { tx[k], tx[k2], tx[k2], tx[k] }, qy[4] = { ty[k], ty[k2], by[k2], by[k] };
      v6_fill(qx, qy, 4, (uint8_t)((sideLev[k] << 4) | hue));
    }
    int lev = v3_clamp(11 + (int)(pr_h[i] * 4), 8, 15);
    v6_fill(tx, ty, 6, (uint8_t)((lev << 4) | hue));
    if (rim) for (int k = 0; k < 6; k++) { int k2 = (k + 1) % 6; v3_line(tx[k] >> 4, ty[k] >> 4, tx[k2] >> 4, ty[k2] >> 4, (uint8_t)(((lev - 6) << 4) | hue)); }
  }
  v6_end();
}
