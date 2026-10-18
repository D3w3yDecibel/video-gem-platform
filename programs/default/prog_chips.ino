// =====================================================================
// PROGRAM: CHIPLEADER — stacks of strands that collapse (by Dewey) — Set 5, slot 0
//
// After paketa12's "chipleader" (TouchDesigner, verlet + collision): columns
// of colour, each built from a stack of thin horizontal lines like a stack
// of poker chips or a test card's colour bars. The stacks shimmer, then
// crumble: each line becomes a loose strand that drops, bends and tangles
// into heaps on the floor. Then every strand flies back to its place, the
// columns rebuild themselves, and it all goes round again.
//
// Each line is a rope of points (vg5s.h). Strands pile up because crowded
// parts of the screen push their points into emptier parts.
//
// Presets:
//   k0 Chipleader — eight test-card colours, crumbling from the bottom up
//   k1 Poker      — red, black, white and green chips falling at random
//   k2 Spectrum   — twelve rainbow columns raining down from the top
//   k3 Moonlight  — silver and blue, coming apart from the middle outwards
//   k4 Neon Burst — the stacks blow apart at once
//   k5 Pastel     — soft colours, light gravity
//   k6 Twin Towers — two wide columns of long strands
//   k7 Downpour   — heavy gravity, gusty wind
//   k8 Zero-G     — no gravity: strands drift and knot in mid-air
//   k9 Dominoes   — the columns go one after another, left to right
//   k10 Embers    — strands fall *upwards* and heap on the ceiling, like flames
//   k11 Tour      — a different preset every time the stacks rebuild
//
//   k12 — DROP NOW (or, once it has fallen, REBUILD NOW)
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = freeze)   p2 Gravity
//   p3 Stiffness (floppy … stiff)   p4 Shimmer (waves through the stacks before they fall)
//   p5 Crowding (how hard strands push each other apart)   p6 Bounce
//   p7 Columns (centre = preset, else 1-12)   p8 Lines per Column (centre = preset)
//   p9 Hold (how long the stacks stand / how long the heap lies)
//   p10 Collapse Style (centre = preset: random, bottom-up, top-down, centre-out, burst, dominoes)
//   p11 Wind
//   p12-p15 are Set 5's live FX knobs (vg5fx.h), off in the centre band:
//   p12 Warp (◄ ripple | zoom ►)   p13 Mirror (◄ mirror | tiles ►)   p14 Colour (◄ cycle | flash ►)
//   p15 Glitch (◄ tear | pixels ►)
// =====================================================================

#include "vg5s.h"

struct Ch5Preset {
  uint8_t cols[12][3]; uint8_t nc;
  uint8_t C, S, style;                               // columns, lines per column, collapse style
  int8_t grav;                                       // gravity: 1/64 px per frame² (negative = up)
  uint8_t shimmer, bounce, wind, iters;
};
enum { CH5_RANDOM, CH5_BOTTOM, CH5_TOP, CH5_CENTRE, CH5_BURST, CH5_DOMINO, CH5_NSTYLE };
static const Ch5Preset ch5_pre[11] = {
  {{{220,30,30},{110,170,215},{215,175,60},{70,190,70},{60,30,200},{200,60,200},{230,170,50},{235,235,220}}, 8, 8, 36, CH5_BOTTOM, 10, 120, 20, 0, 2},
  {{{210,20,30},{40,40,48},{240,240,235},{20,150,70},{30,70,190}}, 5, 10, 36, CH5_RANDOM, 12, 60, 70, 0, 2},
  {{{255,0,40},{255,90,0},{255,170,0},{250,240,0},{150,240,0},{0,220,60},{0,220,180},{0,170,255},{0,80,255},{90,40,255},{180,30,255},{255,30,200}}, 12, 12, 30, CH5_TOP, 12, 60, 30, 0, 2},
  {{{220,225,240},{140,160,200},{80,100,170},{190,200,230},{110,120,140},{250,250,255}}, 6, 6, 40, CH5_CENTRE, 7, 200, 10, 30, 3},
  {{{255,0,200},{0,255,240},{170,255,0},{255,130,0},{120,60,255},{255,255,90}}, 6, 8, 36, CH5_BURST, 9, 40, 40, 0, 2},
  {{{255,180,200},{180,220,255},{200,255,210},{255,240,180},{220,190,255},{255,210,170},{190,250,240}}, 7, 7, 36, CH5_BOTTOM, 5, 160, 30, 20, 2},
  {{{255,60,40},{40,150,255}}, 2, 2, 40, CH5_BOTTOM, 10, 120, 20, 0, 3},
  {{{40,120,255},{0,200,255},{120,220,255},{230,240,255},{60,80,200},{0,160,200},{180,210,255},{20,60,160}}, 8, 8, 36, CH5_TOP, 20, 60, 10, 160, 2},
  {{{255,120,220},{120,255,200},{140,160,255},{255,255,255},{255,200,90},{90,220,255}}, 6, 8, 32, CH5_BURST, 0, 80, 0, 40, 2},
  {{{250,250,250},{30,30,30},{250,250,250},{30,30,30},{250,250,250},{30,30,30},{250,250,250},{30,30,30},{250,250,250}}, 9, 9, 34, CH5_DOMINO, 12, 40, 30, 0, 3},
  {{{255,40,0},{255,110,0},{255,180,20},{255,230,120},{200,20,40},{255,150,60},{255,80,30},{255,210,80},{180,40,10}}, 9, 9, 34, CH5_RANDOM, -8, 100, 0, 90, 2},
};
static const char* const ch5_names[12] = {
  "Chipleader", "Poker", "Spectrum", "Moonlight", "Neon Burst", "Pastel", "Twin Towers", "Downpour", "Zero-G", "Dominoes", "Embers", "Tour"
};
static const char* const ch5_labels[16] = {
  "Palette", "Speed", "Gravity", "Stiffness", "Shimmer", "Crowding", "Bounce", "Columns",
  "Lines per Column", "Hold", "Collapse Style", "Wind", "Line Style", "Trails", "Colour Flow", "Gap"
};
const char* prog_chips_name() { return "CHIPLEADER"; }
const char* prog_chips_character() { return "Columns of colour built from stacked line strands that crumble, tangle into heaps and rebuild"; }
const char* prog_chips_presetName(int preset) { return (preset >= 0 && preset < 12) ? ch5_names[preset] : NULL; }
const char* prog_chips_potLabel(int preset, int pot) { (void)preset; if (pot >= 12 && pot < 16) return fx5_labels[pot - 12]; return (pot >= 0 && pot < 16) ? ch5_labels[pot] : ""; }
uint8_t prog_chips_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

enum { CH5_STAND, CH5_FALL, CH5_HEAP, CH5_BUILD };
static int ch5_C = 0, ch5_S = 0, ch5_gap = -1, ch5_preset = -1, ch5_look = 0, ch5_phase = CH5_STAND, ch5_style = 0;
static float ch5_t = 0, ch5_flow = 0;
static bool ch5_k12 = false;
static int ch5_colW = 30, ch5_x0 = 10, ch5_sp16 = 100;  // column width, left margin, line spacing (1/16 px)

// where point i of strand s sits in the stack (1/16 px)
static inline void ch5_home(int s, int i, int* hx, int* hy) {
  int col = s / ch5_S, row = s % ch5_S;
  *hx = (ch5_x0 + col * (ch5_colW + ch5_gap)) * 16 + (i * ch5_colW * 16) / (s5_per - 1);
  *hy = (H - 6) * 16 - row * ch5_sp16;
}
// the order strands leave (or rejoin) the stack in: 0 … 1
static float ch5_order(int s, int style) {
  int col = s / ch5_S, row = s % ch5_S;
  float r = (v3_hash((uint32_t)s * 2654435761u + 99) & 1023) / 1023.0f;
  float rr = (float)row / (ch5_S > 1 ? ch5_S - 1 : 1), cc = (float)col / (ch5_C > 1 ? ch5_C - 1 : 1);
  switch (style) {
    case CH5_BOTTOM: return rr * 0.75f + r * 0.25f;
    case CH5_TOP: return (1.0f - rr) * 0.75f + r * 0.25f;
    case CH5_CENTRE: { float d = fabsf(cc - 0.5f) * 2.0f; return d * 0.6f + r * 0.4f; }
    case CH5_BURST: return r * 0.08f;
    case CH5_DOMINO: return cc * 0.85f + rr * 0.1f + r * 0.05f;
    default: return r;
  }
}
static void ch5_layout(int C, int S, int gap) {
  ch5_C = C; ch5_S = S; ch5_gap = gap;
  int usable = W - 16 - gap * (C - 1);
  ch5_colW = usable / C; if (ch5_colW < 4) ch5_colW = 4;
  ch5_x0 = (W - (ch5_colW * C + gap * (C - 1))) / 2;
  int per = v3_clamp(ch5_colW / 8 + 1, 3, 24);
  int S2 = S; if (C * S2 * per > S5_MAXP) S2 = S5_MAXP / (C * per);
  ch5_S = S2;
  s5_setup(C * ch5_S, per, (ch5_colW * 16) / (per - 1));
  ch5_sp16 = (180 * 16) / ch5_S;
  for (int s = 0; s < s5_ns; s++) for (int i = 0; i < per; i++) {
    int hx, hy; ch5_home(s, i, &hx, &hy);
    int k = s * per + i; s5_place(k, hx, hy); s5_setPin(k, true);
  }
  ch5_phase = CH5_STAND; ch5_t = 0;
}
void prog_chips_init() { v3_reset(); ch5_C = 0; ch5_preset = -1; ch5_look = 0; ch5_flow = 0; }

static void chips5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame();
  fx5_take();                                                      // p12-p15: the live FX knobs
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != ch5_preset) { ch5_preset = preset; ch5_look = preset == 11 ? 0 : preset; ch5_C = 0; }
  const Ch5Preset& P = ch5_pre[ch5_look];
  const Ch5Preset& Q = ch5_pre[v3_centre(0) ? ch5_look : v3_clamp(v3_k(0) * 11 / 1024, 0, 10)];   // colours

  // ── knobs ──
  float speed = v3_speed(1, 3.0f);
  int grav = v3_centre(2) ? P.grav : (int)(v3_bi(2) * 30);
  int iters = v3_centre(3) ? P.iters : 1 + (int)(v3_u(3) * 3.99f);
  float shimmer = v3_centre(4) ? P.shimmer / 255.0f : v3_u(4);
  int push = v3_centre(5) ? 9 : 3 + (int)(v3_u(5) * 15);
  s5_bounce = v3_centre(6) ? P.bounce : (int)(v3_u(6) * 230);
  int C = v3_centre(7) ? P.C : 1 + v3_clamp(v3_k(7) * 12 / 1024, 0, 11);
  int S = v3_centre(8) ? P.S : 8 + (int)(v3_u(8) * 44);
  float hold = v3_rel(9, 3.0f, 3.0f);
  ch5_style = v3_centre(10) ? P.style : v3_clamp(v3_k(10) * CH5_NSTYLE / 1024, 0, CH5_NSTYLE - 1);
  float wind = v3_centre(11) ? P.wind / 255.0f : v3_u(11);
  int lineStyle = v3_pick(12, 1, 3);
  int trails = v3_centre(13) ? 0 : (int)(v3_u(13) * 14);
  ch5_flow += v3_dt * (v3_centre(14) ? 0.0f : v3_bi(14) * 1.5f);
  int gap = v3_centre(15) ? (C > 6 ? 6 : 10) : (int)(v3_u(15) * 30);
  if (C != ch5_C || S != ch5_S || gap != ch5_gap) {
    if (C != ch5_C || gap != ch5_gap || ch5_phase == CH5_STAND) ch5_layout(C, S, gap);
  }
  s5_top = grav < 0 ? 2 * 16 : -2000;                               // falling upwards (Embers, or the knob): a ceiling

  // ── colours: hue h = column colour h, 15 brightness levels ──
  uint8_t cols[12][3];
  for (int h = 0; h < 12; h++) {
    float f = h + ch5_flow; f -= floorf(f / Q.nc) * Q.nc;
    int a = (int)f % Q.nc, b = (a + 1) % Q.nc, w = (int)((f - floorf(f)) * 256);
    for (int c = 0; c < 3; c++) cols[h][c] = v3_mix8(Q.cols[a][c], Q.cols[b][c], w);
  }
  v3_palLH(cols, 12, 1.4f, 20);
  display.setColor(0, 0, 0, 0);

  // ── the cycle: stand → fall → heap → build ──
  float dt = v3_dt * speed;
  ch5_t += dt;
  bool k12 = keysPressed[KEY_MOD_A], hit = k12 && !ch5_k12; ch5_k12 = k12;
  const float spread = ch5_style == CH5_BURST ? 0.4f : 3.0f, buildT = 2.4f;
  if (ch5_phase == CH5_STAND && (ch5_t > hold || hit)) { ch5_phase = CH5_FALL; ch5_t = 0; hit = false; }
  if (ch5_phase == CH5_FALL && ch5_t > spread + 0.2f) { ch5_phase = CH5_HEAP; ch5_t = 0; }
  if ((ch5_phase == CH5_HEAP && (ch5_t > hold * 2.0f || hit)) || (ch5_phase == CH5_FALL && hit)) {
    ch5_phase = CH5_BUILD; ch5_t = 0;
    for (int k = 0; k < s5_n; k++) s5_setPin(k, true);              // from here on each point is steered home
  }
  if (ch5_phase == CH5_BUILD && ch5_t > buildT + 0.8f) {
    ch5_phase = CH5_STAND; ch5_t = 0;
    if (ch5_preset == 11) { ch5_look = (ch5_look + 1) % 11; ch5_C = 0; }   // the tour: next preset
  }

  // ── each strand: held, released, or flying home ──
  int tw = (int)(globalTime * 220);
  for (int s = 0; s < s5_ns; s++) {
    int a = s * s5_per;
    if (ch5_phase == CH5_STAND || (ch5_phase == CH5_FALL && s5_pinned(a))) {
      float due = ch5_phase == CH5_FALL ? ch5_order(s, ch5_style) * spread : 99.0f;
      float near = ch5_phase == CH5_FALL ? v3_clampf(1.0f - (due - ch5_t) / 1.2f, 0.0f, 1.0f) : v3_clampf(ch5_t / hold, 0.0f, 1.0f) * 0.25f;
      int amp = (int)(shimmer * (8 + 120 * near * near));             // the stack ripples harder just before it goes
      int row = s % ch5_S, col = s / ch5_S;
      for (int i = 0; i < s5_per; i++) {
        int hx, hy; ch5_home(s, i, &hx, &hy);
        int wx = (v3_sin(row * 37 + col * 90 + tw + i * 12) * amp) >> 14;
        int wy = (v3_sin(i * 60 + row * 20 - tw * 2) * amp) >> 16;
        s5_place(a + i, hx + wx, hy + wy);
      }
      if (ch5_phase == CH5_FALL && ch5_t >= due) {                    // let go
        int kx = 0, ky = 0;
        if (ch5_style == CH5_BURST) { kx = ((s / ch5_S) * 2 - ch5_C + 1) * 10 + v3_rndr(-30, 30); ky = -v3_rndr(10, 60); }
        int spin = v3_rndr(-14, 14), side = v3_rndr(-16, 16);           // a sideways push and a twist, so they tumble and mix
        for (int i = 0; i < s5_per; i++) { s5_setPin(a + i, false); s5_kick(a + i, kx + side + v3_rndr(-4, 4), ky + spin * (2 * i - s5_per + 1) / s5_per + v3_rndr(-4, 4)); }
      }
    } else if (ch5_phase == CH5_BUILD) {
      float st = ch5_order(s, CH5_BOTTOM) * buildT * 0.7f;            // they rejoin bottom row first
      float u = v3_clampf((ch5_t - st) / (buildT * 0.3f), 0.0f, 1.0f);
      int k = (int)(u * u * 200) + 8;                               // how much of the way home this frame (/256)
      for (int i = 0; i < s5_per; i++) {
        int hx, hy; ch5_home(s, i, &hx, &hy);
        int x = s5_x[a + i], y = s5_y[a + i];
        x += ((hx - x) * k) >> 8; y += ((hy - y) * k) >> 8;
        if (u >= 1.0f) { x = hx; y = hy; }
        s5_place(a + i, x, y);
      }
    }
  }
  // ── physics (frozen when speed is 0) ──
  if (speed > 0.0f) {
    int steps = speed > 1.6f ? 2 : 1;
    for (int st = 0; st < steps; st++) {
      s5_move(0, (grav * 16) >> 6, 250);
      if (wind > 0.01f) {
        uint32_t t = (uint32_t)(globalTime * 90);
        int wk = (int)(wind * 7);
        for (int k = 0; k < s5_n; k += 1) if (!s5_pinned(k)) {
          int32_t n = s5_wind(s5_x[k] >> 4, s5_y[k] >> 4, t, 7);
          s5_x[k] += (int16_t)((n * wk) >> 14);
          if (ch5_look == 10) s5_y[k] -= (int16_t)(((n + 16384) * wk) >> 16);   // embers flicker upwards
        }
      }
      s5_ropes(iters);
      s5_crowd(2, push);
      s5_walls();
    }
  }

  // ── draw ──
  if (trails > 0) v3_fadeLev(16 - trails > 0 ? 16 - trails : 1); else s5_clear(0);
  for (int s = 0; s < s5_ns; s++) {
    int col = s / ch5_S, row = s % ch5_S, hue = col % 12;
    int lev = (row & 1) ? 15 : 12;
    if ((v3_hash((uint32_t)s * 31u) & 7) == 0) lev = 9;               // a few darker lines, like chip edges
    uint8_t c = (uint8_t)((lev << 4) | hue);
    if (lineStyle == 2) s5_drawStrandAA(s, lev, hue);
    else if (lineStyle == 1) s5_drawBand(s, c, (uint8_t)(((lev - 4) << 4) | hue), (uint8_t)((4 << 4) | hue));
    else s5_drawStrand(s, c, 0);
  }
}
void prog_chips_draw(int preset) { chips5_draw(preset); fx5_apply(); }
