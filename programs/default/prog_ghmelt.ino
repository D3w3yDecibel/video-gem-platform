// =====================================================================
// PROGRAM: MELT — melting cellular automata (by Dewey) — Set 8 slot 8
//
// Greatest hit from Set 1 (MIND MELT, "after slowing") and Set 3
// (AUTOMATA): cellular automata that organise themselves out of noise into
// turning spirals and travelling waves. Here they step only a few times a
// second and every cell CROSS-FADES from one state to the next, and the
// 80 × 60 grid is spread smoothly over the screen — so instead of
// flickering pixels you get colour slowly melting round in spirals.
//
// Two rules:
//   CYCLIC — every cell has a colour 0 … n−1 and takes the next colour when
//     enough neighbours already have it. Out of random noise grow rainbow
//     spirals that eat the screen ("tie-dye").
//   EXCITE — Greenberg-Hastings excitable medium: resting cells fire when
//     a neighbour fires, then rest a while. Waves and spirals of light run
//     over a dark sea (keys beautifully).
//
// Presets: k0 Melt · k1 Spiral Waves · k2 Tie-Dye · k3 Ripples · k4 Marble ·
//          k5 Embers · k6 Lava Melt · k7 Neon Spirals · k8 Forest Rings ·
//          k9 Aurora Melt · k10 Velvet · k11 Tour
//   k12 — SPLASH: drop a splash of fresh noise in somewhere
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 States · p6 Rule · p7 Threshold · p8 Steps/s · p9 Wave Glow · p10 Sparks
//   p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8ME_CYCLIC, H8ME_EXCITE };
struct H8MeLook { uint8_t rule, states, thr, range, steps, glow, sparks, pal, flow, spread; int8_t key; };
static const H8MeLook h8me_looks[11] = {
  // range: 1/2 = Moore neighbourhood of radius 1/2, 11/12 = von Neumann (diamond) radius 1/2
  // rule        n  thr range steps glow sparks pal          flow spr key
  { H8ME_CYCLIC, 14, 1, 11,   6,  55,  20, V8P_LOUNGE,   2, 100,  0 },   // Melt         (Griffeath's CCA: R1/T1/C14/vN)
  { H8ME_EXCITE,  9, 1, 11,   6,  60,  25, V8P_VAPOR,    3,  90,  0 },   // Spiral Waves
  { H8ME_CYCLIC,  4, 3,  1,   5,  45,  20, V8P_RAINBOW,  3, 100,  0 },   // Tie-Dye      ("perfect spirals" R1/T3/C4/Moore)
  { H8ME_EXCITE, 14, 1, 11,   6,  60,  45, V8P_LAGOON,   2,  70,  0 },   // Ripples
  { H8ME_CYCLIC,  3, 10, 2,   4,  40,  20, V8P_PEACH,    2,  80,  0 },   // Marble       ("lava lamp" R2/T10/C3/Moore)
  { H8ME_EXCITE, 10, 1,  1,   5,  65,  35, V8P_FIREICE,    2,  50,  0 },   // Embers
  { H8ME_CYCLIC,  8, 5,  2,   4,  50,  20, V8P_LAVA,     2,  90,  0 },   // Lava Melt    ("turbulent phase" R2/T5/C8/Moore)
  { H8ME_EXCITE,  8, 1,  1,   7,  70,  25, V8P_LOUNGE,   3, 100,  0 },   // Neon Spirals
  { H8ME_CYCLIC,  3, 3,  1,   4,  45,  20, V8P_ACID,   2,  80,  0 },   // Forest Rings ("313" R1/T3/C3/Moore)
  { H8ME_CYCLIC,  6, 2, 12,   5,  55,  20, V8P_AURORA,   3, 100,  0 },   // Aurora Melt  ("squarish spirals" R2/T2/C6/vN)
  { H8ME_EXCITE, 12, 2,  2,   5,  60,  30, V8P_VELVET,   2,  70,  0 },   // Velvet
};
static const char* const h8me_names[12] = { "Melt", "Spiral Waves", "Tie-Dye", "Ripples", "Marble", "Embers", "Lava Melt", "Neon Spirals", "Forest Rings", "Aurora Melt", "Velvet", "Tour" };
static const char* const h8me_lbl[6] = { "States", "Rule", "Threshold", "Steps/s", "Wave Glow", "Sparks" };
const char* prog_ghmelt_name() { return "MELT"; }
const char* prog_ghmelt_character() { return "Melting cellular automata: rainbow spirals and waves that cross-fade instead of flicker"; }
const char* prog_ghmelt_presetName(int p) { return (p >= 0 && p < 12) ? h8me_names[p] : NULL; }
const char* prog_ghmelt_potLabel(int p, int pot) { (void)p; return v8_label(pot, h8me_lbl); }
uint8_t prog_ghmelt_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

#define H8ME_OLD (V8_EXTRA)                       // the state before the last step
#define H8ME_NEW (V8_EXTRA + V8_GN)               // the state now
#define H8ME_AGE (V8_EXTRA + 2 * V8_GN)           // steps since the cell last changed (0 … 15)
static float h8me_acc = 0, h8me_glow = 0.5f, h8me_sparks = 0.2f;
static int h8me_n = 12, h8me_rule = 0, h8me_thr = 1, h8me_range = 1, h8me_changes = 0;
static bool h8me_ready = false;
void prog_ghmelt_init() { v8_init(); h8me_ready = false; }

static void h8me_splash(int cx, int cy, int r, bool all) {
  uint8_t* S = H8ME_NEW;
  for (int j = 0; j < V8_GH; j++) for (int i = 0; i < V8_GW; i++) {
    int dx = i - cx, dy = j - cy;
    if (!all && dx * dx + dy * dy > r * r) continue;
    int k = j * V8_GW + i;
    S[k] = h8me_rule == H8ME_EXCITE ? (uint8_t)(v3_rnd(100) < 12 ? 1 : (v3_rnd(100) < 50 ? 0 : v3_rnd(h8me_n))) : (uint8_t)v3_rnd(h8me_n);
    H8ME_AGE[k] = 15;
  }
}
// one step of the automaton (torus), NEW → OLD copy first
__attribute__((optimize("O2"))) static void h8me_step() {
  uint8_t* O = H8ME_OLD; uint8_t* S = H8ME_NEW; uint8_t* A = H8ME_AGE;
  memcpy(O, S, V8_GN);
  int n = h8me_n, thr = h8me_thr, rg = h8me_range % 10, changes = 0;
  bool vn = h8me_range > 10;                                          // von Neumann: a diamond of neighbours
  bool ex = h8me_rule == H8ME_EXCITE;
  uint32_t spark = (uint32_t)(h8me_sparks * h8me_sparks * 600);       // chance per cell per step, /2^20
  for (int j = 0; j < V8_GH; j++) {
    const uint8_t* ra = O + (j ? j - 1 : V8_GH - 1) * V8_GW;            // the rows above, here and below (wrapping)
    const uint8_t* rb = O + j * V8_GW;
    const uint8_t* rc = O + (j + 1 < V8_GH ? j + 1 : 0) * V8_GW;
    for (int i = 0; i < V8_GW; i++) {
      int k = j * V8_GW + i, s = rb[i];
      int want = ex ? 1 : (s + 1 == n ? 0 : s + 1), cnt = 0;
      if (ex && s != 0) { S[k] = (uint8_t)(s + 1 >= n ? 0 : s + 1); A[k] = 0; changes++; continue; }   // firing / resting cells move on
      int im = i ? i - 1 : V8_GW - 1, ip = i + 1 < V8_GW ? i + 1 : 0;
      if (rg == 1) {                                                   // the common cases, unrolled
        cnt = (ra[i] == want) + (rb[im] == want) + (rb[ip] == want) + (rc[i] == want);
        if (!vn) cnt += (ra[im] == want) + (ra[ip] == want) + (rc[im] == want) + (rc[ip] == want);
      } else {
        for (int dy = -rg; dy <= rg; dy++) {
          int jj = j + dy; if (jj < 0) jj += V8_GH; else if (jj >= V8_GH) jj -= V8_GH;
          const uint8_t* row = O + jj * V8_GW;
          for (int dx = -rg; dx <= rg; dx++) {
            if (!dx && !dy) continue;
            if (vn && v3_abs(dx) + v3_abs(dy) > rg) continue;
            int ii = i + dx; if (ii < 0) ii += V8_GW; else if (ii >= V8_GW) ii -= V8_GW;
            cnt += row[ii] == want;
          }
        }
      }
      if (cnt >= thr || (ex && (v3_rand() & 0xFFFFF) < spark)) { S[k] = (uint8_t)want; A[k] = 0; changes++; }
      else if (A[k] < 15) A[k]++;
    }
  }
  h8me_changes = changes;
  if (ex) {                                                             // two drifting pacemakers keep the waves coming
    static uint16_t tick = 0; tick++;
    if (tick % (unsigned)(n + 3) == 0) for (int q = 0; q < 2; q++) {
      int px = (int)(V8_GW / 2 + (V8_GW / 3) * sinf(tick * 0.013f + q * 3.1f)), py = (int)(V8_GH / 2 + (V8_GH / 3) * cosf(tick * 0.009f + q * 2.0f));
      int k = v3_clamp(py, 0, V8_GH - 1) * V8_GW + v3_clamp(px, 0, V8_GW - 1);
      if (S[k] == 0) { S[k] = 1; A[k] = 0; }
    }
  }
}

void prog_ghmelt_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  const H8MeLook& L = h8me_looks[look];
  int n = v3_centre(5) ? L.states : 3 + v3_k(5) * 22 / 1024;
  int rule = v3_pick(6, L.rule, 2);
  int thr = v3_centre(7) ? L.thr : 1 + v3_k(7) * 4 / 1024;
  float stepsPerS = v3_centre(8) ? (float)L.steps : 0.5f + v3_u(8) * v3_u(8) * 20.0f;
  float rate = v8_rate();
  v6_ease(&h8me_glow, v8_kn(9, L.glow / 100.0f), rate);
  v6_ease(&h8me_sparks, v8_kn(10, L.sparks / 100.0f), rate);
  v8_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 90, L.key / 100.0f);
  bool reseed = !h8me_ready || v6_f.fresh || n != h8me_n || rule != h8me_rule;
  int nbs = L.range == 1 ? 8 : L.range == 2 ? 24 : L.range == 11 ? 4 : 12;   // neighbours in the look's neighbourhood
  thr = v3_clamp(thr, 1, rule == H8ME_EXCITE ? v6_max(1, nbs / 4) : nbs / 2 + 1);   // past this, nothing can ever change
  h8me_n = n; h8me_rule = rule; h8me_thr = thr; h8me_range = L.range;
  if (reseed) {                                                      // fresh noise, then a head start so it has organised itself
    h8me_splash(0, 0, 0, true);
    int pre = h8me_range % 10 == 2 ? 16 : (h8me_range > 10 ? 80 : 36);   // radius-2 rules cost 3× per step; the 4-neighbour ones are cheap
    for (int s = 0; s < pre; s++) { h8me_step(); watchdog_update(); }
    h8me_ready = true; h8me_acc = 0;
  }
  if (v6_k12hit) h8me_splash(v3_rnd(V8_GW), v3_rnd(V8_GH), 6 + v3_rnd(8), false);
  h8me_acc += v6_f.sdt * stepsPerS;
  if (h8me_acc >= 1.0f) {
    h8me_acc -= 1.0f; if (h8me_acc > 1.0f) h8me_acc = 0;
    h8me_step();
    if (h8me_changes < V8_GN / 300) h8me_splash(v3_rnd(V8_GW), v3_rnd(V8_GH), 10, false);   // gone still: stir it
  }
  // the picture: each cell cross-fades from its old state to its new one
  int f = (int)(h8me_acc * 256);
  float z = v8_zoom(1.0f);
  int base = rule == H8ME_EXCITE ? 0 : 110 + (int)(z * 10);
  int wave = 20 + (int)(h8me_glow * 180);
  const uint8_t* O = H8ME_OLD; const uint8_t* S = H8ME_NEW; const uint8_t* A = H8ME_AGE;
  uint8_t* LEV = V8_LEV; uint8_t* HUE = V8_HUE;
  int hstep = 256 / n;
  for (int k = 0; k < V8_GN; k++) {
    int so = O[k], sn = S[k], lev, hue;
    if (rule == H8ME_EXCITE) {
      // firing (1) is brightest, then the light dies away through the resting states
      int lo = so == 0 ? 0 : 226 - (so - 1) * 226 / n, ln = sn == 0 ? 0 : 226 - (sn - 1) * 226 / n;
      lo = lo * wave / 160; ln = ln * wave / 160;
      lev = lo + (((ln - lo) * f) >> 8);
      hue = (so * hstep * 3 + ((((sn - so) * hstep * 3) * f) >> 8)) >> 1;
    } else {
      int ho = so * hstep, hn = sn * hstep;
      int dh = v8_hwrap((hn - ho) & 255);
      hue = ho + ((dh * f) >> 8);
      int age = A[k] == 0 ? (256 - f) >> 4 : 0;                       // a just-changed cell glows as the wave front passes
      lev = base + ((age * wave) >> 4) - (A[k] > 6 ? 20 : 0);
    }
    LEV[k] = (uint8_t)v3_clamp(lev, 0, 226); HUE[k] = (uint8_t)(hue & 255);
  }
  v8_upscale(6);
  v8_end();
}
