// =====================================================================
// PROGRAM: RILEY — Bridget Riley's wave paintings (by Dewey) — Set 7, slot 0
//
// In the 1960s Bridget Riley painted fields of black and white lines that
// bend in waves — "Current", "Fall", "Cataract", "Arrest" — so precisely
// that the canvas seems to ripple and shimmer while you look. Later she
// turned to colour: "Ra", "Late Morning", stripes of a few colours side by
// side that mix and glow. And "Movement in Squares" (1961): a chequerboard
// whose columns squeeze together until it seems to fold away.
//
// How it works: every stripe field here is one sum per pixel. The position
// across the stripes grows by a fixed step along each row; a wave (one
// table lookup) bends it; one more table turns the result into a shade and
// a hue, with soft anti-aliased edges so fine lines stay clean.
//
// Presets: k0 Current · k1 Fall · k2 Cataract · k3 Ra · k4 Arrest · k5 Chevron ·
//          k6 Movement in Squares · k7 Breathe · k8 Late Morning · k9 Shiver ·
//          k10 Rhythm · k11 Tour
//   k12 — SHIVER: a shudder runs through the lines
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Stripes · p6 Wave · p7 Wavelength · p8 Thickness · p9 Angle · p10 Edge (sharp | soft)
//   p11 Colour (B&W | full) · p12-p15 live FX
// =====================================================================
#include "vg7op.h"

enum { RY_STRIPES, RY_SQUARES };
enum { RY_SINE, RY_ZIGZAG };
// freq: stripes across · amp: wave height ×10 stripes · wlen: waves down the screen ×10 · duty % paper ·
// edge: soft (0-100) · angle (deg) · drift: wave speed · slide: stripes sliding · chirp: stripes closer
// toward the bottom (%) · grow: wave grows toward the bottom (%) · pulse: thickness breathes (%)
struct RyLook { uint8_t mode, shape, freq, amp, wlen, duty, edge, angle; int8_t drift, slide; uint8_t chirp, grow, pulse, hstep, pal, flow, spread, sat; };
static const RyLook ry_looks[11] = {
  // mode        shape      freq amp wlen duty edg ang drift slide chirp grow pulse hst pal          flow spr sat
  { RY_STRIPES, RY_SINE  , 44,  6,  25, 50,  0,  0, 25,   0,   0,   0,  0, 0, O7P_BW      , 0,  50,   0 },   // Current
  { RY_STRIPES, RY_SINE  , 26, 10,  14, 50,  0,  0, 15,   0, 130,   0,  0, 1, O7P_RA      , 3,  90, 100 },   // Fall
  { RY_STRIPES, RY_SINE  , 22, 12,  12, 50, 10, 90, 20,   0,   0,   0,  0, 1, O7P_RA      , 2,  30,  95 },   // Cataract
  { RY_STRIPES, RY_SINE  , 24,  0,  10, 50,  5, 60,  0,  25,   0,   0,  0, 1, O7P_RA      , 3,  80, 100 },   // Ra
  { RY_STRIPES, RY_SINE  , 34, 16,  18, 50,  0,  0, 20,   0,   0, 100,  0, 1, O7P_VEGA    , 3,  90, 100 },   // Arrest
  { RY_STRIPES, RY_ZIGZAG, 30,  8,  60, 50,  0,  0, 35,   0,   0,   0,  0, 1, O7P_NEON    , 3,  90, 100 },   // Chevron
  { RY_SQUARES, RY_SINE  , 16, 10,  60, 50,  0,  0, 20,   0,   0,   0,  0, 1, O7P_KITAOKA , 3,  90, 100 },   // Movement in Squares
  { RY_STRIPES, RY_SINE  , 30,  7,  20, 50, 15,  0, 15,   0,   0,   0, 70, 1, O7P_VEGA    , 3,  60,  90 },   // Breathe
  { RY_STRIPES, RY_SINE  , 20,  0,  10, 50,  0,  0,  0, -18,   0,   0,  0, 4, O7P_SPECTRUM, 2, 100, 100 },   // Late Morning
  { RY_STRIPES, RY_SINE  , 46,  3, 100, 50,  0,  0, 45,   0,   0,   0,  0, 1, O7P_SPECTRUM, 3,  90, 100 },   // Shiver
  { RY_STRIPES, RY_SINE  , 30,  9,  22, 50,  5, 45, 20,  10,   0,  60,  0, 1, O7P_VEGA    , 4,  70, 100 },   // Rhythm
};
static const char* const ry_names[12] = { "Current", "Fall", "Cataract", "Ra", "Arrest", "Chevron", "Movement in Squares", "Breathe", "Late Morning", "Shiver", "Rhythm", "Tour" };
static const char* const ry_lbl[6] = { "Stripes", "Wave", "Wavelength", "Thickness", "Angle", "Edge: Sharp|Soft" };
const char* prog_riley_name() { return "RILEY"; }
const char* prog_riley_character() { return "Bridget Riley: black-and-white and colour stripes bending into waves"; }
const char* prog_riley_presetName(int p) { return (p >= 0 && p < 12) ? ry_names[p] : NULL; }
const char* prog_riley_potLabel(int p, int pot) { (void)p; return o7_label(pot, ry_lbl); }
uint8_t prog_riley_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// working memory: the stripe field (4096), the wave (256 × 4), the band (256), squares' columns (320)
static uint8_t *ry_comb, *ry_band, *ry_col; static int32_t* ry_wave;
static void ry_mem() { ry_comb = v6_mem; ry_wave = (int32_t*)(v6_mem + 4096); ry_band = v6_mem + 5120; ry_col = v6_mem + 5376; }

static float ry_freq, ry_amp, ry_wlen, ry_duty, ry_ang, ry_soft, ry_wph = 0, ry_slide = 0, ry_kick = 0, ry_cph = 0;
void prog_riley_init() { ry_mem(); v6_init(); ry_kick = 0; }

// one row of a stripe field: phase P (65536 per stripe) steps by kx; the wave index w (2^32 per wave) by dw
__attribute__((optimize("O2"))) static void ry_row(uint8_t* out, uint32_t P, int32_t kx, uint32_t w, int32_t dw) {
  const uint8_t* comb = ry_comb; const int32_t* wv = ry_wave;
  for (int x = 0; x < W; x += 4) {
    uint32_t a = P + (uint32_t)wv[w >> 24]; P += kx; w += dw;
    uint32_t b = P + (uint32_t)wv[w >> 24]; P += kx; w += dw;
    uint32_t c = P + (uint32_t)wv[w >> 24]; P += kx; w += dw;
    uint32_t d = P + (uint32_t)wv[w >> 24]; P += kx; w += dw;
    O7_PUT4(out + x, comb[(a >> 8) & 4095], comb[(b >> 8) & 4095], comb[(c >> 8) & 4095], comb[(d >> 8) & 4095]);
  }
}
// … with the wave scaled for this row (Arrest: the wave grows down the screen)
__attribute__((optimize("O2"))) static void ry_rowGrow(uint8_t* out, uint32_t P, int32_t kx, uint32_t w, int32_t dw, int32_t g) {
  const uint8_t* comb = ry_comb; const int32_t* wv = ry_wave;
  for (int x = 0; x < W; x++, P += kx, w += dw) out[x] = comb[((P + (uint32_t)((wv[w >> 24] * g) >> 8)) >> 8) & 4095];
}

void prog_riley_draw(int preset) {
  int look = o7_begin(preset);
  if (!v3_buf) return;
  ry_mem();
  const RyLook& L = ry_looks[look];
  float rate = v6_fade < 1.0f ? 1.5f : 4.0f;
  v6_ease(&ry_freq, v3_centre(5) ? o7_vary(5, (float)L.freq) : 6.0f + v3_u(5) * 90.0f, rate);
  v6_ease(&ry_amp, v3_centre(6) ? o7_vary(6, L.amp / 10.0f) : v3_u(6) * 3.0f, rate);
  v6_ease(&ry_wlen, v3_centre(7) ? o7_vary(7, L.wlen / 10.0f) : 0.3f + v3_u(7) * v3_u(7) * 18.0f, rate);
  v6_ease(&ry_duty, v3_centre(8) ? o7_vary(8, L.duty / 100.0f) : 0.05f + v3_u(8) * 0.9f, rate);
  v6_ease(&ry_ang, v3_centre(9) ? o7_vary(9, L.angle * 0.01745f) : v3_u(9) * 3.1416f, rate);
  v6_ease(&ry_soft, o7_kn(10, L.edge / 100.0f), rate);
  o7_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, L.sat / 100.0f);

  float sdt = v6_f.sdt, t = v6_t;
  ry_wph += sdt * L.drift * 0.01f; ry_wph -= floorf(ry_wph);
  ry_slide += sdt * L.slide * 0.04f; if (ry_slide > 4096.0f || ry_slide < -4096.0f) ry_slide = 0;
  if (v6_k12hit) ry_kick = 1.0f;
  if (ry_kick > 0) { ry_kick -= v6_f.dt * 0.7f; if (ry_kick < 0) ry_kick = 0; }
  float z = v6_zoom(1.0f);
  float periodPx = 320.0f / ry_freq * z;                               // one stripe, in pixels
  float duty = ry_duty;
  if (L.pulse) duty = v3_clampf(duty + L.pulse * 0.004f * o7_nsin(t * 1.3f), 0.08f, 0.92f);

  if (L.mode == RY_SQUARES) {
    // ── Movement in Squares: the columns squeeze together toward a line that drifts across ──
    o7_band(ry_band, 256, 128, o7_edge(periodPx, 256, ry_soft));
    float cx = 160.0f + 90.0f * o7_nsin(t * 0.21f), squeeze = 0.95f * (0.75f + 0.25f * o7_nsin(t * 0.33f)) * v3_clampf(ry_amp, 0.0f, 1.0f);
    float wpx = 40.0f + ry_wlen * 10.0f;
    // the squeeze: each column's width shrinks near cx; the widths add up to the phase
    int32_t F[W + 1]; int32_t acc = 0;
    for (int x = 0; x <= W; x++) {
      F[x] = acc;
      float u = (x - cx) / wpx, b = u * u < 1.0f ? (1.0f - u * u) * (1.0f - u * u) : 0.0f;
      acc += (int32_t)((1.0f + squeeze * 6.0f * b) * 4096.0f);          // narrower near the fold
    }
    // phase (65536 a stripe, ×65536) per unit of F (4096 a pixel): away from the fold the squares are square
    int32_t sk = (int32_t)(65536.0f * 16.0f / periodPx);
    int32_t off = (int32_t)(-(F[(int)cx] * (int64_t)sk >> 16)) + (int32_t)(65536 * 64);
    for (int x = 0; x < W; x++) { int32_t P = (int32_t)(((int64_t)F[x] * sk) >> 16) + off; ry_col[x] = ry_band[(P >> 8) & 255]; }
    (void)acc;
    int32_t ky = (int32_t)(65536.0f / periodPx), Py = -ky * HALFH + 65536 * 64;
    for (int y = 0; y < H; y++, Py += ky) {
      int rs = ry_band[(Py >> 8) & 255];
      uint8_t xr[16]; const uint8_t* op = o7_op[O7_XOR] + rs * 16;
      int hu = L.hstep ? o7_mod15[((uint32_t)Py >> 16) % 15 * L.hstep % 60] : O7_GREY;   // each row of squares its own hue
      for (int s = 0; s < 16; s++) xr[s] = (uint8_t)((op[s] << 4) | hu);
      uint8_t* o = v3_buf + y * W;
      for (int x = 0; x < W; x += 4) O7_PUT4(o + x, xr[ry_col[x]], xr[ry_col[x + 1]], xr[ry_col[x + 2]], xr[ry_col[x + 3]]);
    }
    o7_end();
    return;
  }
  // ── stripes: the band, the hues, the wave ──
  o7_band(ry_band, 256, (int)(duty * 256), o7_edge(periodPx, 256, ry_soft));
  ry_cph += sdt * 0.25f * L.flow; ry_cph -= floorf(ry_cph);
  o7_comb(ry_comb, ry_band, 0, L.hstep);
  float amp = ry_amp + ry_kick * ry_kick * 1.2f;
  int32_t A = (int32_t)(amp * 65536.0f);
  for (int i = 0; i < 256; i++) {
    int s;
    if (L.shape == RY_ZIGZAG) { s = i < 128 ? i * 256 - 16384 : (255 - i) * 256 - 16384; }   // −16384 … 16384
    else s = v3_sin(i * 4);
    if (ry_kick > 0) s += (int)(ry_kick * ry_kick * v3_sin(i * 28 + o7_frame * 90) * 0.35f);
    ry_wave[i] = (int32_t)(((int64_t)A * s) >> 14);
  }
  // per row: where the stripes start and how fast they go, where the wave starts and its step
  float ca = cosf(ry_ang), sa = sinf(ry_ang);
  float Kpx = 65536.0f / periodPx;                                        // phase per pixel across the stripes
  float fW = ry_wlen / 240.0f / z * 4294967296.0f;                         // wave phase per pixel along the stripes
  float gW = fW * 0.15f;                                                   // … and a little across them (the waves lean)
  int32_t dw = (int32_t)(-sa * fW + ca * gW);
  float wph = ry_wph * 4294967296.0f;
  uint32_t slide = (uint32_t)(int32_t)(ry_slide * 65536.0f);
  for (int y = 0; y < H; y++) {
    float yc = (float)(y - HALFH);
    float c = 1.0f + L.chirp * 0.01f * yc / 240.0f;                         // Fall: the stripes crowd toward the bottom
    if (c < 0.2f) c = 0.2f;
    int32_t kx = (int32_t)(c * Kpx * ca);
    uint32_t P0 = (uint32_t)(int32_t)(c * Kpx * (-HALFW * ca + yc * sa)) + slide + (uint32_t)(65536 * 1024);
    uint32_t w0 = (uint32_t)(int64_t)(fW * (HALFW * sa + yc * ca) + gW * (-HALFW * ca + yc * sa) + wph);
    uint8_t* out = v3_buf + y * W;
    if (L.grow) {
      int g = (int)(256.0f * (1.0f + L.grow * 0.01f * yc / 120.0f));
      ry_rowGrow(out, P0, kx, w0, dw, g < 0 ? 0 : g);
    } else ry_row(out, P0, kx, w0, dw);
  }
  o7_end();
}
