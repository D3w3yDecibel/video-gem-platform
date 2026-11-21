// =====================================================================
// PROGRAM: ZONE — zone plates and their ghosts (by Dewey) — Set 7, slot 7
//
// Broadcast engineers tested cameras and monitors with the zone plate:
// rings that get closer and closer together toward the edge, sweeping
// every frequency the system can carry. Past the limit the rings don't
// just blur — they come back as new phantom rings, whole ghost zone plates
// growing out of the sides. Those aliases are the ghosts of the analog
// age: moiré, beat patterns, rings within rings. Here they are the show.
//
// How it works: a zone plate's phase is k·(x² + y²), which splits into a
// table across and a table down — so each pixel is one addition and one
// lookup (32-bit phases that wrap round on their own). Hyperbolas use
// x² − y², the saddle x·y; twin, beat and quad plates add two or four
// plates' brightness together, and a slower plate paints the hues.
//
// Presets: k0 Zone Plate · k1 Hyperbola · k2 Saddle · k3 Twin · k4 Chirp ·
//          k5 Beat · k6 Drift · k7 Quad · k8 Op Art · k9 Breath · k10 Ellipse · k11 Tour
//   k12 — SWEEP: hold to push the frequency up into a storm of aliases
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Frequency · p6 Flow (◄ in | out ►) · p7 Separation · p8 Shape
//   p9 Bands (◄ soft | hard ►) · p10 Drift · p11 Scanlines · p12-p15 live FX
// =====================================================================
#include "vg7.h"

enum { ZN_CIRCLE, ZN_HYPER, ZN_SADDLE, ZN_TWIN, ZN_CHIRP, ZN_BEAT, ZN_QUAD, ZN_NSHAPE };
struct ZnLook { uint8_t shape, freq; int8_t flow; uint8_t sep, bands, drift, breath, aspect, pal, cflow, spread, scan; };
static const ZnLook zn_looks[11] = {
  // shape       freq flow sep bands drift brth asp pal              cflow spr scan
  { ZN_CIRCLE,   40,  30,  0,  40,   0,   0, 100, V7P_GHOST,         2,  60, 2 },   // Zone Plate
  { ZN_HYPER,    35,  25,  0,  50,  10,   0, 100, V7P_PAIK,          4,  90, 2 },   // Hyperbola
  { ZN_SADDLE,   35, -30,  0,  50,   0,   0, 100, V7P_BLACKLIGHT,    5,  90, 2 },   // Saddle
  { ZN_TWIN,     30,  35, 50,  60,  20,   0, 100, V7P_CHICAGO,       5, 100, 2 },   // Twin
  { ZN_CHIRP,    45,  40,  0,  45,   0,   0, 100, V7P_VASULKA,       3,  70, 2 },   // Chirp
  { ZN_BEAT,     55,  20, 40,  50,   0,   0, 100, V7P_SUNSET,        4,  90, 2 },   // Beat
  { ZN_CIRCLE,   45,  30,  0,  50,  80,   0, 100, V7P_LUMIA,         5,  90, 2 },   // Drift
  { ZN_QUAD,     30,  25, 60,  55,  30,   0, 100, V7P_SCANIMATE,     6, 100, 2 },   // Quad
  { ZN_CIRCLE,   50,  25,  0, 100,   0,   0, 100, V7P_GHOST,         0,   8, 0 },   // Op Art
  { ZN_CIRCLE,   45,  15,  0,  50,   0, 100, 100, V7P_KODACHROME,    5,  90, 2 },   // Breath
  { ZN_CIRCLE,   40,  30,  0,  50,  20,   0,  35, V7P_STUDIO,        4, 100, 2 },   // Ellipse
};
static const char* const zn_names[12] = { "Zone Plate", "Hyperbola", "Saddle", "Twin", "Chirp", "Beat", "Drift", "Quad", "Op Art", "Breath", "Ellipse", "Tour" };
static const char* const zn_lbl[6] = { "Frequency", "Flow: In|Out", "Separation", "Shape", "Bands: Soft|Hard", "Drift" };
const char* prog_zone_name() { return "ZONE"; }
const char* prog_zone_character() { return "Zone-plate test signals and the ghost rings of their aliasing"; }
const char* prog_zone_presetName(int p) { return (p >= 0 && p < 12) ? zn_names[p] : NULL; }
const char* prog_zone_potLabel(int p, int pot) { (void)p; return v7_label(pot, zn_lbl); }
uint8_t prog_zone_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: per-plate tables across (W) and down (H), the hue plate, colour tables
static uint32_t *zn_X, *zn_Y, *zn_hX, *zn_hY; static uint8_t* zn_lut; static int8_t* zn_cos;
static void zn_mem() {
  zn_X = (uint32_t*)v6_mem; zn_Y = zn_X + 4 * (W + H);                    // up to 4 plates (saddle: 2 diagonal tables of W+H)
  zn_hX = zn_X + 4 * (W + H) + 4 * H; zn_hY = zn_hX + W;
  zn_lut = (uint8_t*)(zn_hY + H); zn_cos = (int8_t*)(zn_lut + 4096);
}
static float zn_k, zn_flowR, zn_sep, zn_bands, zn_drift, zn_ph = 0, zn_dph = 0, zn_sweep = 1.0f, zn_hph = 0;

void prog_zone_init() { zn_mem(); v6_init(); zn_sweep = 1.0f; for (int i = 0; i < 256; i++) zn_cos[i] = (int8_t)v3_clamp(v3_cos(i * 4) >> 7, -127, 127); }

// a rate in cycles → a 32-bit phase step (only the fraction of a cycle matters)
static inline uint32_t zn_q(float k) { double f = (double)k - floor((double)k); return (uint32_t)(uint64_t)(f * 4294967296.0); }
// a plate's tables: phase (2^32 per cycle) = kx·(x − cx)² + ky·(y − cy)² + ph
static void zn_plate(uint32_t* X, uint32_t* Y, float k, float cx, float cy, float aspect, int sgnY, uint32_t ph) {
  uint32_t kq = zn_q(k), kyq = zn_q(k * aspect);
  int icx = (int)cx, icy = (int)cy;
  for (int x = 0; x < W; x++) { int32_t d = x - icx; X[x] = kq * (uint32_t)(d * d) + ph; }
  for (int y = 0; y < H; y++) { int32_t d = y - icy; uint32_t v = kyq * (uint32_t)(d * d); Y[y] = sgnY < 0 ? (uint32_t)0 - v : v; }
}
__attribute__((optimize("O2"))) static void zn_rowOne(uint8_t* out, const uint32_t* X, uint32_t y0, const uint32_t* hX, uint32_t hy) {
  const uint8_t* lut = zn_lut;
  for (int x = 0; x < W; x++) out[x] = lut[(((hX[x] + hy) >> 20) & 0xF00) | ((X[x] + y0) >> 24)];
}
__attribute__((optimize("O2"))) static void zn_rowSum(uint8_t* out, const uint32_t* X, const uint32_t* Y, int y, int n, const uint32_t* hX, uint32_t hy) {
  const uint8_t* lut = zn_lut; const int8_t* cs = zn_cos;
  uint32_t y0 = Y[y], y1 = Y[H + y], y2 = Y[2 * H + y], y3 = Y[3 * H + y];
  const uint32_t *X0 = X, *X1 = X + W, *X2 = X + 2 * W, *X3 = X + 3 * W;
  for (int x = 0; x < W; x++) {
    int s = cs[(X0[x] + y0) >> 24] + cs[(X1[x] + y1) >> 24];
    if (n > 2) s += cs[(X2[x] + y2) >> 24] + cs[(X3[x] + y3) >> 24];
    if (n > 2) s >>= 1;
    out[x] = lut[(((hX[x] + hy) >> 20) & 0xF00) | (uint8_t)((s + 256) >> 1)];
  }
}
__attribute__((optimize("O2"))) static void zn_rowSaddle(uint8_t* out, const uint32_t* D1, const uint32_t* D2, int y, uint32_t ph, const uint32_t* hX, uint32_t hy) {
  const uint8_t* lut = zn_lut;
  const uint32_t* a = D1 + y; const uint32_t* b = D2 + (H - 1 - y);
  for (int x = 0; x < W; x++) out[x] = lut[(((hX[x] + hy) >> 20) & 0xF00) | ((a[x] - b[x] + ph) >> 24)];
}

void prog_zone_draw(int preset) {
  int look = v7_begin(preset, 11);
  if (!v3_buf) return;
  zn_mem();
  const ZnLook& L = zn_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&zn_k, v3_centre(5) ? L.freq / 100.0f : v3_u(5), rate);
  v6_ease(&zn_flowR, v7_kb(6, L.flow / 100.0f), rate);
  v6_ease(&zn_sep, v7_kn(7, L.sep / 100.0f), rate);
  v6_ease(&zn_bands, v7_kn(9, L.bands / 100.0f), rate);
  v6_ease(&zn_drift, v7_kn(10, L.drift / 100.0f), rate);
  int shape = v3_centre(8) ? L.shape : v3_clamp(v3_k(8) * ZN_NSHAPE / 1024, 0, ZN_NSHAPE - 1);
  v7_colours(L.pal, L.cflow / 100.0f, L.spread / 100.0f, 110);

  float sdt = v6_f.sdt, t = v6_t;
  zn_ph += sdt * zn_flowR * 1.5f; zn_ph -= floorf(zn_ph);
  zn_dph += sdt * (0.1f + zn_drift * 0.6f);
  zn_hph += sdt * 0.05f; zn_hph -= floorf(zn_hph);
  float sw = v6_k12 ? 4.0f : 1.0f;
  zn_sweep += (sw - zn_sweep) * v3_clampf(v3_dt * (v6_k12 ? 0.8f : 2.0f), 0, 1);
  float br = L.breath ? 0.55f + 0.45f * sinf(t * 0.35f) * (L.breath / 100.0f) * 1.6f : 1.0f;
  if (br < 0.15f) br = 0.15f;
  // k: cycles per px² — aliasing starts where 2·k·r passes half a cycle per pixel
  float k = (0.0004f + zn_k * zn_k * 0.012f) * zn_sweep * br / (v6_zoom(1.0f) * v6_zoom(1.0f));
  if (k > 0.25f) k = 0.25f;                                                  // (beyond this it is all aliases anyway)
  uint32_t ph = zn_q(zn_ph);

  // ── the colour table: 16 hue rings × 256 steps of brightness round a cycle ──
  int gain = 256 + (int)(zn_bands * zn_bands * 3600);
  bool sum = shape == ZN_TWIN || shape == ZN_BEAT || shape == ZN_QUAD;
  for (int i = 0; i < 256; i++) {
    int c = sum ? (i - 128) * 128 : v3_cos(i * 4);                        // brightness: −16384 … 16384
    int lev = 8 + (int)(((int64_t)c * 7 * gain) >> 22);
    lev = v3_clamp(lev, 0, 15);
    if (zn_bands > 0.95f) lev = lev >= 8 ? 15 : 0;
    for (int h = 0; h < 16; h++) zn_lut[(h << 8) | i] = V6C(lev, h);
  }
  // ── the plates ──
  float dx = zn_drift * 70.0f, sep = zn_sep * 120.0f;
  float cx = HALFW + dx * sinf(zn_dph * 1.3f), cy = HALFH + dx * 0.7f * sinf(zn_dph * 0.9f + 1.0f);
  float asp = L.aspect / 100.0f;
  zn_plate(zn_hX, zn_hY, k / 9.0f, cx, cy, asp, 1, zn_q(zn_hph));   // the slow plate that paints the hues
  if (shape == ZN_SADDLE) {
    // x·y = ((x+y)² − (x−y)²) / 4: two diagonal tables, indexed by x + y and x − y
    uint32_t kq = zn_q(k * 0.25f);
    uint32_t* D1 = zn_X; uint32_t* D2 = zn_X + (W + H);
    int icx = (int)cx, icy = (int)cy;
    for (int s = 0; s < W + H; s++) { int32_t a = s - icx - icy, b = s - (H - 1) - icx + icy; D1[s] = kq * (uint32_t)(a * a); D2[s] = kq * (uint32_t)(b * b); }
    for (int y = 0; y < H; y++) zn_rowSaddle(v3_buf + y * W, D1, D2, y, ph, zn_hX, zn_hY[y]);
  } else if (sum) {
    int n = shape == ZN_QUAD ? 4 : 2;
    for (int p = 0; p < n; p++) {
      float a = p * 6.2832f / n + zn_dph * 0.4f;
      float px = cx + (shape == ZN_BEAT ? 0 : sep * cosf(a)), py = cy + (shape == ZN_BEAT ? 0 : sep * 0.75f * sinf(a));
      float kk = shape == ZN_BEAT ? k * (1.0f + (p ? zn_sep * 0.3f : 0)) : k;
      zn_plate(zn_X + p * W, zn_Y + p * H, kk, px, py, asp, 1, p ? (uint32_t)0 - ph : ph);
    }
    for (int y = 0; y < H; y++) zn_rowSum(v3_buf + y * W, zn_X, zn_Y, y, n, zn_hX, zn_hY[y]);
  } else {
    if (shape == ZN_CHIRP) {                                              // across: k·x² · down: straight bands
      zn_plate(zn_X, zn_Y, k, cx, cy, asp, 1, ph);
      uint32_t lq = zn_q(k * 40.0f);
      for (int y = 0; y < H; y++) zn_Y[y] = lq * (uint32_t)y;
    } else zn_plate(zn_X, zn_Y, k, cx, cy, asp, shape == ZN_HYPER ? -1 : 1, ph);
    for (int y = 0; y < H; y++) zn_rowOne(v3_buf + y * W, zn_X, zn_Y[y], zn_hX, zn_hY[y]);
  }
  v7_end(L.scan);
}
