// =====================================================================
// PROGRAM: RIBBONS — flowing silk ribbons (by Dewey) — Set 8 slot 5
//
// Greatest hit from Set 1 (WEAVE) and Set 2 (SPECTRUM FLOW): bundles of
// fine glowing threads that wave across the screen and twist over and over
// like ribbons in slow water, leaving silky afterglow trails.
//
// How it works: each ribbon follows two slow sine waves; its threads sit
// side by side, spread by the cosine of a twist that travels along the
// ribbon — where the cosine passes zero the threads cross, which reads as
// the ribbon turning over. Threads are anti-aliased lines that ADD light,
// so crossings glow. The last frame fades a little each frame (trails).
// Vortex mode bends the ribbons into rings round the middle; Cascade hangs
// them from the top.
//
// Presets: k0 Silk Road · k1 Spectrum · k2 Vortex · k3 Cascade · k4 Velvet Rope ·
//          k5 Double Helix · k6 Halo · k7 Weave · k8 Kelp · k9 Ember · k10 Sherbet ·
//          k11 Tour
//   k12 — PLUCK: the ribbons are plucked like strings and ring out
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Ribbons · p6 Threads · p7 Wave · p8 Twist · p9 Trails · p10 Glow
//   p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8RI_SILK, H8RI_VORTEX, H8RI_CASCADE };
struct H8RiLook { uint8_t mode, nr, ns, wave, twist, trails, glow, width, pal, flow, spread; int8_t key; };
static const H8RiLook h8ri_looks[11] = {
  // mode          nr ns wave twist trail glow width pal         flow spr key
  { H8RI_SILK,     3, 10,  50,  45,  60,  50,  55, V8P_VAPOR,    3,  90,  0 },   // Silk Road
  { H8RI_SILK,     1, 16,  55,  40,  65,  55,  80, V8P_LOUNGE,   3, 100,  0 },   // Spectrum
  { H8RI_VORTEX,   3,  8,  45,  50,  60,  50,  45, V8P_ELECTRIC,   3,  90,  0 },   // Vortex
  { H8RI_CASCADE,  4,  8,  50,  45,  60,  50,  45, V8P_LAGOON,   2,  70,  0 },   // Cascade
  { H8RI_SILK,     2, 12,  40,  35,  70,  50,  60, V8P_VELVET,   2,  60,  0 },   // Velvet Rope
  { H8RI_SILK,     1,  2,  45,  80,  55,  70,  60, V8P_CANDY,  3,  80,  0 },   // Double Helix
  { H8RI_VORTEX,   1, 14,  40,  40,  70,  55,  70, V8P_SUNSET,   2,  90,  0 },   // Halo
  { H8RI_SILK,     5,  6,  55,  50,  55,  50,  40, V8P_DUSK,     3,  90,  0 },   // Weave
  { H8RI_CASCADE,  3,  6,  45,  35,  65,  50,  40, V8P_TROPIC,   2,  70,  0 },   // Kelp
  { H8RI_SILK,     3,  8,  50,  45,  65,  60,  50, V8P_EMBER,    2,  50,  0 },   // Ember
  { H8RI_VORTEX,   2, 10,  50,  45,  60,  50,  55, V8P_SHERBET,  3, 100,  0 },   // Sherbet
};
static const char* const h8ri_names[12] = { "Silk Road", "Spectrum", "Vortex", "Cascade", "Velvet Rope", "Double Helix", "Halo", "Weave", "Kelp", "Ember", "Sherbet", "Tour" };
static const char* const h8ri_lbl[6] = { "Ribbons", "Threads", "Wave", "Twist", "Trails", "Glow" };
const char* prog_ghribbons_name() { return "RIBBONS"; }
const char* prog_ghribbons_character() { return "Flowing silk ribbons of glowing threads that twist and leave soft trails"; }
const char* prog_ghribbons_presetName(int p) { return (p >= 0 && p < 12) ? h8ri_names[p] : NULL; }
const char* prog_ghribbons_potLabel(int p, int pot) { (void)p; return v8_label(pot, h8ri_lbl); }
uint8_t prog_ghribbons_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

#define H8RI_MAXR 6
#define H8RI_MAXS 16
#define H8RI_SEG 48
static float h8ri_nr = 3, h8ri_ns = 8, h8ri_wave = 0.5f, h8ri_twist = 0.5f, h8ri_trails = 0.5f, h8ri_glow = 0.5f;
static float h8ri_t = 0, h8ri_pluck = 0;
static uint32_t h8ri_seed = 5;
// a segment that stops 1/8 px short, so joined segments don't light their shared pixel twice
static inline void h8ri_seg(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int lev, int hue) {
  int32_t dx = x1 - x0, dy = y1 - y0;
  if (v3_abs(dx) >= v3_abs(dy)) x1 -= dx > 0 ? 1 : (dx < 0 ? -1 : 0); else y1 -= dy > 0 ? 1 : -1;
  v6_lineAdd(x0, y0, x1, y1, lev, hue);
}
void prog_ghribbons_init() { v8_init(); h8ri_pluck = 0; h8ri_seed = 1 + v3_rnd(100000); }

void prog_ghribbons_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  const H8RiLook& L = h8ri_looks[look];
  float rate = v8_rate();
  v6_ease(&h8ri_nr, v3_centre(5) ? (float)L.nr : 1.0f + v3_u(5) * (H8RI_MAXR - 1), rate);
  v6_ease(&h8ri_ns, v3_centre(6) ? (float)L.ns : 2.0f + v3_u(6) * (H8RI_MAXS - 2), rate);
  v6_ease(&h8ri_wave, v8_kn(7, L.wave / 100.0f), rate);
  v6_ease(&h8ri_twist, v8_kn(8, L.twist / 100.0f), rate);
  v6_ease(&h8ri_trails, v8_kn(9, L.trails / 100.0f), rate);
  v6_ease(&h8ri_glow, v8_kn(10, L.glow / 100.0f), rate);
  if (v6_f.fresh) h8ri_seed = 1 + v3_rnd(100000);
  v8_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 110, L.key / 100.0f);
  if (v6_first || v6_f.fresh) memset(v3_buf, 0, W * H);
  else v8_decay(v6_fade < 1.0f ? 40 : 1 + (int)((1.0f - h8ri_trails) * (1.0f - h8ri_trails) * 70));   // trails (clears faster during a change)

  h8ri_t += v6_f.sdt * 1.5f;                                          // (v3: livelier)
  static float nextPluck = 6.0f;                                     // ribbons pluck themselves now and then
  if (h8ri_t > nextPluck || h8ri_t + 30.0f < nextPluck) { h8ri_pluck = v3_clampf(h8ri_pluck + 0.6f, 0.0f, 1.0f); nextPluck = h8ri_t + 6.0f + v3_rndf() * 9.0f; }
  if (h8ri_t > 20000.0f) h8ri_t -= 20000.0f;
  if (v6_k12hit) h8ri_pluck = 1.0f;
  h8ri_pluck *= 1.0f - v3_clampf(v6_f.dt * 0.6f, 0.0f, 1.0f);
  int nr = v3_clamp((int)(h8ri_nr + 0.5f), 1, H8RI_MAXR), ns = v3_clamp((int)(h8ri_ns + 0.5f), 1, H8RI_MAXS);
  if (nr * ns > 48) ns = v6_max(2, 48 / nr);                          // at most 48 threads: keeps the frame rate up
  float z = v8_zoom(1.0f);
  int lev = 2 + (int)(h8ri_glow * 11);
  int vortex = L.mode == H8RI_VORTEX, casc = L.mode == H8RI_CASCADE;
  int span = casc ? H : W;                                           // ribbons run across (or down)
  int32_t px[H8RI_SEG + 1], py[H8RI_SEG + 1];
  for (int k = 0; k < nr; k++) {
    uint32_t h = v3_hash(h8ri_seed * 17u + k * 4057u);
    float tk = h8ri_t * (0.6f + (h & 255) / 512.0f);
    float base = (k + 0.5f) / nr + 0.16f * sinf(h8ri_t * 0.11f + k * 1.9f) / (nr > 2 ? 2.0f : 1.0f);   // ribbons wander
    int f1 = (int)((350 + (h >> 8) % 300) * (0.4f + h8ri_wave) / z), f2 = (int)((900 + (h >> 16) % 500) * (0.4f + h8ri_wave) / z);
    int ph1 = (int)(tk * 140) + (int)(h % 1024), ph2 = (int)(tk * 90) + (int)((h >> 5) % 1024);
    float amp = (0.02f + 0.45f * h8ri_wave) * (0.6f + 0.4f * sinf(h8ri_t * 0.19f + k * 2.3f)) * (1.0f + 1.5f * h8ri_pluck * sinf(h8ri_t * 25.0f + k)) * z;
    int ft = (int)((300 + (h >> 20) % 400) * (0.05f + 3.5f * h8ri_twist * h8ri_twist)), Tt = (int)(tk * 160) + (int)((h >> 11) % 1024);
    if (vortex) {                                                    // whole waves per turn, so each ring closes
      f1 = 1024 * (2 + (int)((h >> 8) % 3)); f2 = 1024 * (3 + (int)((h >> 16) % 2)); ft = 1024 * (2 + (int)(h8ri_twist * 6));
      base = 0.3f + 0.7f * base;
    }
    int hk = (int)((k * 5 + 1) & 15);
    float wid = (0.05f + 0.08f * L.width / 100.0f) * z * (vortex ? 0.8f : 1.0f) * (0.6f + 0.5f * sinf(h8ri_t * 0.27f + k * 1.3f));   // and breathe
    // the ribbon's middle line and its twist, per segment (fixed point: position in 1/1024 of the span)
    int32_t mid[H8RI_SEG + 1], cs[H8RI_SEG + 1], sn[H8RI_SEG + 1];
    int32_t A = (int32_t)(amp * 1024), Wd = (int32_t)(wid * 1024);
    for (int q = 0; q <= H8RI_SEG; q++) {
      int xq = q * 1024 / H8RI_SEG;                                   // along, 0 … 1024
      mid[q] = (int32_t)(base * 1024) + ((A * v3_sin(((xq * f1) >> 10) + ph1) + (A >> 2) * v3_sin(((xq * f2) >> 10) - ph2)) >> 14);
      int tp = ((xq * ft) >> 10) + Tt;
      cs[q] = v3_cos(tp); sn[q] = v3_sin(tp);
    }
    for (int s = 0; s < ns; s++) {
      int32_t off2 = ns > 1 ? (2 * s - (ns - 1)) * Wd / (ns - 1) / 2 : 0;   // the thread's place across the ribbon
      int hue = (hk + (s * 8) / (ns > 1 ? ns : 1)) & 15;
      for (int q = 0; q <= H8RI_SEG; q++) {
        int32_t across = mid[q] + ((off2 * cs[q]) >> 14);             // 1/1024 of the screen
        int32_t along = q * 1024 / H8RI_SEG;
        if (vortex) {                                                 // along = angle, across = radius
          int ang = (int)(along + (int)(h8ri_t * 20) * (k & 1 ? -1 : 1));
          int32_t rr = across * 150 >> 10;                            // px
          px[q] = (W / 2) * 8 + ((rr * v3_cos(ang)) >> 11); py[q] = (H / 2) * 8 + ((rr * v3_sin(ang)) >> 11);
        } else if (casc) { px[q] = across * W * 8 >> 10; py[q] = -64 + (along * (span + 16) * 8 >> 10); }
        else { px[q] = -64 + (along * (span + 16) * 8 >> 10); py[q] = across * H * 8 >> 10; }
      }
      for (int q = 0; q < H8RI_SEG; q++) {
        int front = (sn[q] + 16384) >> 12;                            // 0 … 8: the near side of the twist is brighter
        int pp = (q * 3072 / H8RI_SEG - (int)(h8ri_t * 500) + k * 300) & 1023;   // pulses of light running along
        int bump = pp < 180 ? (180 - pp) / 45 : 0;
        int hq = (hue + (q * 3) / H8RI_SEG + (int)(h8ri_t * 0.8f)) & 15;    // colour runs along the ribbon
        h8ri_seg(px[q], py[q], px[q + 1], py[q + 1], lev - 2 + (front >> 2) + bump + (ns <= 2 ? 3 : 0), hq);
      }
    }
    if (ns == 2) {                                                    // a double helix gets its rungs
      for (int q = 0; q <= H8RI_SEG; q += 2) {
        int32_t a1 = mid[q] + ((-Wd / 2 * cs[q]) >> 14), a2 = mid[q] + ((Wd / 2 * cs[q]) >> 14);
        int32_t x = -64 + (q * 1024 / H8RI_SEG * (W + 16) * 8 >> 10);
        if (!vortex && !casc) v6_lineAdd(x, a1 * H * 8 >> 10, x, a2 * H * 8 >> 10, 2, (hk + 8) & 15);
      }
    }
  }
  v8_end();
}
