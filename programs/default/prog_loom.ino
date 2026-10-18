// =====================================================================
// PROGRAM: LOOM — strings held at both ends (by Dewey) — Set 5, slot 3
//
// Strings stretched across the screen and held at both ends: plucked,
// strummed, blown, combed, sagging under their own weight, or woven into a
// net that a ball bounces on. Each string is a rope of points (vg5s.h), so
// a pluck sends real waves running along it and bouncing off the ends.
// Colour can follow the string, run in test-card bars across all the
// strings, or light up only where a string is moving.
//
// Presets:
//   k0 Test Card  — a test card made of strings: plucks ripple through the colour bars
//   k1 Harp       — vertical rainbow strings, swept in glissandos
//   k2 Net        — woven horizontal and vertical strings, a ball pushing through
//   k3 Hammock    — slack strings sagging into curves, swaying
//   k4 Ocean      — blue strings rolling with waves of wind
//   k5 Comb       — a comb dragged through the strings
//   k6 Trampoline — a ball bouncing on a net
//   k7 Guitar     — six thick strings strummed in rhythm, glowing where they move
//   k8 Standing Waves — strings shaken at their middles, each at a different speed
//   k9 Rain on Wires — drops hit the wires at random; only the trembling parts glow
//   k10 Cat's Cradle — the ends of the strings slowly circle, crossing and uncrossing
//   k11 Tour      — a new preset every 20 seconds
//
//   k12 — STRUM: pluck every string, one after another
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = freeze)   p2 Gravity (sag)   p3 Tension
//   p4 Wind   p5 Pluck Rate   p6 Pluck Strength   p7 Strings (how many)   p8 Ball (centre = preset; size)
//   p9 Ball Speed   p10 Sustain (how long strings ring)   p11 Direction (centre = preset: across, down, both)
//   p12-p15 are Set 5's live FX knobs (vg5fx.h), off in the centre band:
//   p12 Warp (◄ ripple | zoom ►)   p13 Mirror (◄ mirror | tiles ►)   p14 Colour (◄ cycle | flash ►)
//   p15 Glitch (◄ tear | pixels ►)
// =====================================================================

#include "vg5s.h"

enum { LM5_ACROSS, LM5_DOWN, LM5_BOTH };
enum { LM5_BYSTRING, LM5_BARS, LM5_MOTION };
struct Lm5Preset {
  uint8_t cols[8][3]; uint8_t nc;
  uint8_t dir, strands, per, colour;
  int8_t grav; uint8_t wind, pluck, power, ball, sustain, slack, mode;   // slack: extra length, /64 of the span; mode: special behaviour
};
enum { LMM_NONE, LMM_GLISS, LMM_COMB, LMM_BOUNCE, LMM_STRUM, LMM_SHAKE, LMM_RAIN, LMM_CRADLE };
static const Lm5Preset lm5_pre[11] = {
  {{{235,235,220},{230,200,40},{60,200,200},{60,190,60},{200,60,200},{220,40,40},{40,40,210},{20,20,20}}, 8, LM5_ACROSS, 56, 26, LM5_BARS, 0, 30, 60, 90, 0, 250, 0, LMM_NONE},
  {{{255,0,40},{255,140,0},{250,240,0},{0,220,60},{0,170,255},{90,40,255},{220,30,255}}, 7, LM5_DOWN, 40, 30, LM5_BYSTRING, 0, 0, 0, 80, 0, 252, 0, LMM_GLISS},
  {{{220,220,230},{120,180,255},{255,190,90},{180,120,255}}, 4, LM5_BOTH, 52, 26, LM5_BYSTRING, 4, 20, 0, 60, 30, 246, 0, LMM_NONE},
  {{{255,150,90},{255,210,120},{200,90,140},{120,80,160},{255,120,120}}, 5, LM5_ACROSS, 30, 34, LM5_BYSTRING, 14, 60, 0, 60, 0, 250, 14, LMM_NONE},
  {{{10,60,160},{30,120,220},{90,190,250},{200,240,255},{0,90,150}}, 5, LM5_ACROSS, 50, 28, LM5_BYSTRING, 0, 140, 0, 60, 0, 248, 1, LMM_NONE},
  {{{230,60,60},{240,170,60},{240,240,120},{90,200,120},{90,140,240},{200,110,230}}, 6, LM5_ACROSS, 50, 28, LM5_BYSTRING, 0, 10, 0, 60, 0, 246, 0, LMM_COMB},
  {{{255,80,120},{255,200,60},{80,220,255},{160,255,120}}, 4, LM5_BOTH, 50, 28, LM5_BYSTRING, 4, 0, 0, 60, 26, 244, 0, LMM_BOUNCE},
  {{{255,210,120},{255,170,80},{230,230,240},{200,200,210},{255,190,100},{240,240,255}}, 6, LM5_ACROSS, 6, 40, LM5_MOTION, 0, 0, 0, 150, 0, 253, 0, LMM_STRUM},
  {{{255,0,200},{0,255,240},{170,255,0},{255,130,0},{120,60,255},{255,255,90}}, 6, LM5_ACROSS, 24, 36, LM5_BYSTRING, 0, 0, 0, 60, 0, 250, 0, LMM_SHAKE},
  {{{150,200,255},{200,230,255},{110,160,240},{255,255,255}}, 4, LM5_ACROSS, 30, 40, LM5_MOTION, 2, 0, 0, 60, 0, 252, 0, LMM_RAIN},
  {{{255,60,60},{255,200,60},{60,200,255},{255,255,255},{120,255,120},{255,120,220}}, 6, LM5_ACROSS, 24, 30, LM5_BYSTRING, 0, 0, 0, 60, 0, 248, 2, LMM_CRADLE},
};
static const char* const lm5_names[12] = {
  "Test Card", "Harp", "Net", "Hammock", "Ocean", "Comb", "Trampoline", "Guitar", "Standing Waves", "Rain on Wires", "Cat's Cradle", "Tour"
};
static const char* const lm5_labels[16] = {
  "Palette", "Speed", "Gravity", "Tension", "Wind", "Pluck Rate", "Pluck Strength", "Strings",
  "Ball", "Ball Speed", "Sustain", "Direction", "Line Style", "Trails", "Colour Flow", "Colour Mode"
};
const char* prog_loom_name() { return "LOOM"; }
const char* prog_loom_character() { return "Strings held at both ends: plucked, strummed, combed, sagging, woven into a net with a bouncing ball"; }
const char* prog_loom_presetName(int preset) { return (preset >= 0 && preset < 12) ? lm5_names[preset] : NULL; }
const char* prog_loom_potLabel(int preset, int pot) { (void)preset; if (pot >= 12 && pot < 16) return fx5_labels[pot - 12]; return (pot >= 0 && pot < 16) ? lm5_labels[pot] : ""; }
uint8_t prog_loom_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

static int lm5_preset = -1, lm5_look = 0, lm5_ns = 0, lm5_per = 0, lm5_dir = -1, lm5_slack = -1, lm5_nAcross = 0;
static float lm5_t = 0, lm5_tour = 0, lm5_flow = 0, lm5_pluckAcc = 0, lm5_strum = -1, lm5_ballA = 0;
static bool lm5_k12 = false;

// the two ends of string s (1/16 px)
static void lm5_ends(int s, int* x0, int* y0, int* x1, int* y1) {
  bool across = s < lm5_nAcross;
  int n = across ? lm5_nAcross : lm5_ns - lm5_nAcross, j = across ? s : s - lm5_nAcross;
  if (across) {
    int y = (int)((16 + (float)(j + 0.5f) * (H - 32) / n) * 16);
    *x0 = 6 * 16; *x1 = (W - 7) * 16; *y0 = *y1 = y;
    if (lm5_look == 7) { *y0 = *y1 = (60 + j * 24) * 16; *x0 = 20 * 16; *x1 = 300 * 16; }
  } else {
    int x = (int)((10 + (float)(j + 0.5f) * (W - 20) / n) * 16);
    *y0 = 6 * 16; *y1 = (H - 7) * 16; *x0 = *x1 = x;
  }
  if (lm5_look == 10 && v3_centre(11)) {                              // cat's cradle: the ends circle
    float a = lm5_t * 0.3f + j * 0.35f;
    *y0 += (int)(sinf(a) * 70 * 16); *y1 += (int)(sinf(a + 2.2f + lm5_t * 0.13f) * 70 * 16);
  }
}
static void lm5_layout(int ns, int per, int dir, int slack) {
  lm5_dir = dir; lm5_slack = slack;
  if (per * ns > S5_MAXP) ns = S5_MAXP / per;
  lm5_ns = ns; lm5_per = per;
  lm5_nAcross = dir == LM5_ACROSS ? ns : (dir == LM5_DOWN ? 0 : ns / 2);
  int span = lm5_nAcross > 0 ? (W - 13) : (H - 13);
  s5_setup(ns, per, (span * 16 * (slack ? 61 + slack : 52)) / (64 * (per - 1)));   // slack 0 = well short of the span: taut, they snap straight
  lm5_ns = s5_ns;
  for (int s = 0; s < s5_ns; s++) {
    int x0, y0, x1, y1; lm5_ends(s, &x0, &y0, &x1, &y1);
    for (int i = 0; i < per; i++) s5_place(s * per + i, x0 + (x1 - x0) * i / (per - 1), y0 + (y1 - y0) * i / (per - 1));
    s5_setPin(s * per, true); s5_setPin(s * per + per - 1, true);
  }
}
static void lm5_pluck(int s, int power) {
  if (s < 0 || s >= s5_ns) return;
  int a = s * s5_per, at = 2 + v3_rnd(s5_per - 4 > 1 ? s5_per - 4 : 1);
  bool across = s < lm5_nAcross;
  int kick = (v3_rnd(2) ? 1 : -1) * power;
  for (int i = 1; i < s5_per - 1; i++) {
    int d = v3_abs(i - at), w = d < 4 ? 4 - d : 0; if (!w) continue;
    if (across) s5_y[a + i] += (int16_t)(kick * w / 4); else s5_x[a + i] += (int16_t)(kick * w / 4);
  }
}
void prog_loom_init() { v3_reset(); lm5_preset = -1; lm5_ns = 0; lm5_t = 0; lm5_flow = 0; }

static void loom5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame();
  fx5_take();                                                      // p12-p15: the live FX knobs
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != lm5_preset) { lm5_preset = preset; lm5_look = preset == 11 ? 0 : preset; lm5_tour = 0; lm5_ns = 0; }
  if (preset == 11) { lm5_tour += v3_dt; if (lm5_tour > 20.0f) { lm5_tour = 0; lm5_look = (lm5_look + 1) % 11; lm5_ns = 0; } }
  const Lm5Preset& P = lm5_pre[lm5_look];
  const Lm5Preset& Q = lm5_pre[v3_centre(0) ? lm5_look : v3_clamp(v3_k(0) * 11 / 1024, 0, 10)];

  float speed = v3_speed(1, 3.0f);
  int grav = v3_centre(2) ? P.grav : (int)(v3_u(2) * 30);
  int iters = v3_centre(3) ? (P.slack > 4 ? 3 : 6) : 1 + (int)(v3_u(3) * 6.99f);
  int slack = v3_centre(3) ? P.slack : (int)((1.0f - v3_u(3)) * 16);
  float wind = v3_centre(4) ? P.wind / 255.0f : v3_u(4) * 1.3f;
  float pluckRate = v3_centre(5) ? P.pluck / 40.0f : v3_u(5) * 8.0f;  // plucks per second
  int power = (int)(v3_rel(6, P.power, 3.0f) * 2);
  int ns = v3_centre(7) ? P.strands : 2 + (int)(v3_u(7) * 70);
  int ball = v3_centre(8) ? P.ball : (int)(v3_u(8) * 50);
  float ballSp = v3_rel(9, 1.0f, 3.0f);
  int keep = v3_centre(10) ? P.sustain : 236 + (int)(v3_u(10) * 19);
  int dir = v3_centre(11) ? P.dir : v3_clamp(v3_k(11) * 3 / 1024, 0, 2);
  int lineStyle = v3_pick(12, P.colour == LM5_MOTION ? 1 : 0, 3);
  int trails = v3_centre(13) ? (P.colour == LM5_MOTION ? 10 : 0) : (int)(v3_u(13) * 14);
  lm5_flow += v3_dt * (v3_centre(14) ? 0.0f : v3_bi(14) * 1.5f);
  int cmode = v3_pick(15, P.colour, 3);
  int per = P.per;
  if (ns != lm5_ns || dir != lm5_dir || slack != lm5_slack || per != lm5_per) lm5_layout(ns, per, dir, slack);

  uint8_t cols[8][3];
  for (int h = 0; h < 8; h++) {
    float f = h + lm5_flow; f -= floorf(f / Q.nc) * Q.nc;
    int a = (int)f % Q.nc, b = (a + 1) % Q.nc, w = (int)((f - floorf(f)) * 256);
    for (int c = 0; c < 3; c++) cols[h][c] = v3_mix8(Q.cols[a][c], Q.cols[b][c], w);
  }
  v3_palLH(cols, 8, 1.3f, 16);
  for (int l = 0; l < 16; l++) display.setColor((uint8_t)((l << 4) | 8), l * 9, l * 7, l * 5);   // hue 8: ball / comb, warm grey

  float dt = v3_dt * speed;
  lm5_t += dt;
  bool k12 = keysPressed[KEY_MOD_A]; if (k12 && !lm5_k12) lm5_strum = 0; lm5_k12 = k12;

  // ── the ends (moving, for cat's cradle) ──
  for (int s = 0; s < s5_ns; s++) {
    int x0, y0, x1, y1; lm5_ends(s, &x0, &y0, &x1, &y1);
    s5_place(s * s5_per, x0, y0); s5_place(s * s5_per + s5_per - 1, x1, y1);
  }
  // ── plucks ──
  if (speed > 0.0f) {
    lm5_pluckAcc += pluckRate * dt;
    while (lm5_pluckAcc >= 1.0f) { lm5_pluckAcc -= 1.0f; lm5_pluck(v3_rnd(s5_ns), power); }
    if (P.mode == LMM_GLISS || lm5_strum >= 0) {                      // a sweep across the strings
      float rate = P.mode == LMM_GLISS && lm5_strum < 0 ? 14.0f : 60.0f;
      static float gl = 0; int before = (int)gl;
      if (lm5_strum >= 0) { int b = (int)lm5_strum; lm5_strum += dt * rate; for (int s = b; s < (int)lm5_strum && s < s5_ns; s++) lm5_pluck(s, power); if (lm5_strum >= s5_ns) lm5_strum = -1; }
      else { gl += dt * rate; if ((int)gl != before) lm5_pluck(((int)gl) % (s5_ns * 2) < s5_ns ? ((int)gl) % s5_ns : s5_ns - 1 - ((int)gl) % s5_ns, power); if (gl > 100000) gl = 0; }
    }
    if (P.mode == LMM_STRUM && v3_centre(5)) {                       // a strumming rhythm
      static int lastBeat = -1; int beat = (int)(lm5_t * 2.4f);
      if (beat != lastBeat) { lastBeat = beat; if ((beat & 3) != 3) lm5_strum = 0; }
    }
    if (P.mode == LMM_RAIN && v3_centre(5)) {                        // drops
      if (v3_rndf() < dt * 12) lm5_pluck(v3_rnd(s5_ns), power / 2);
    }
    if (P.mode == LMM_SHAKE) {                                       // each string's middle shaken at its own speed
      for (int s = 0; s < s5_ns; s++) {
        int m = s * s5_per + s5_per / 2, x0, y0, x1, y1; lm5_ends(s, &x0, &y0, &x1, &y1);
        int amp = power * 3;
        s5_place(m, (x0 + x1) / 2, (y0 + y1) / 2 + (int)(sinf(lm5_t * (1.5f + s * 0.45f)) * amp)); s5_setPin(m, true);
      }
    }
  }
  // ── the ball or the comb ──
  lm5_ballA += dt * 0.4f * ballSp;
  int bx = 0, by = 0, br = ball * 16;
  if (P.mode == LMM_BOUNCE && v3_centre(8)) { bx = (int)((160 + 90 * sinf(lm5_ballA * 0.7f)) * 16); by = (int)((120 - 80 * fabsf(sinf(lm5_ballA * 3.0f)) + 30) * 16); }
  else { bx = (int)((160 + 140 * sinf(lm5_ballA)) * 16); by = (int)((120 + 70 * sinf(lm5_ballA * 1.3f)) * 16); }

  if (speed > 0.0f) {
    s5_move(0, (grav * 16) / 64, keep);
    if (wind > 0.01f) {
      uint32_t t = (uint32_t)(lm5_t * 150);
      int kx = (int)(wind * 8);
      for (int k = 0; k < s5_n; k++) {
        if (s5_pinned(k)) continue;
        bool across = (k / s5_per) < lm5_nAcross;
        int32_t n = s5_wind(s5_x[k] >> 4, s5_y[k] >> 4, t, 11);
        if (across) s5_y[k] += (int16_t)((n * kx) / 16384); else s5_x[k] += (int16_t)((n * kx) / 16384);
      }
    }
    if (br > 0) {
      for (int k = 0; k < s5_n; k++) {
        int ex = s5_x[k] - bx, ey = s5_y[k] - by;
        if (ex > br || ex < -br || ey > br || ey < -br || s5_pinned(k)) continue;
        int d2 = ex * ex + ey * ey; if (d2 >= br * br) continue;
        int d = (int)v3_isqrt((uint32_t)d2) + 1;
        s5_x[k] = (int16_t)(bx + ex * br / d); s5_y[k] = (int16_t)(by + ey * br / d);
      }
    }
    if (P.mode == LMM_COMB && v3_centre(8)) {                        // the comb: teeth dragging the strings sideways
      int cx = (int)((160 + 150 * sinf(lm5_t * 0.5f)) * 16);
      for (int k = 0; k < s5_n; k++) {
        if (s5_pinned(k)) continue;
        int d = s5_x[k] - cx; if (d > 160 || d < -160) continue;
        int tooth = ((s5_y[k] >> 4) % 12) < 2;
        if (tooth) s5_x[k] += (int16_t)((cx - s5_x[k]) / 3);
      }
    }
    s5_ropes(iters);
    s5_walls();
  }

  // ── draw ──
  if (trails > 0) v3_fadeLev(16 - trails); else s5_clear(0);
  if (br > 0) { v3_disc(bx >> 4, by >> 4, ball, (uint8_t)((7 << 4) | 8)); v3_disc((bx >> 4) - ball / 3, (by >> 4) - ball / 3, ball / 3, (uint8_t)((12 << 4) | 8)); }
  if (P.mode == LMM_COMB && v3_centre(8)) { int cx = 160 + (int)(150 * sinf(lm5_t * 0.5f)); v3_rect(cx - 2, 0, cx + 2, H - 1, (uint8_t)((5 << 4) | 8)); }
  for (int s = 0; s < s5_ns; s++) {
    int a = s * s5_per, hue = (s * Q.nc / (s5_ns > 0 ? s5_ns : 1)) % 8;
    if (lm5_look == 7) hue = s % 8;
    int px = s5_x[a] >> 4, py = s5_y[a] >> 4;
    for (int i = 1; i < s5_per; i++) {
      int x = s5_x[a + i] >> 4, y = s5_y[a + i] >> 4, h = hue, lev = 14;
      if (cmode == LM5_BARS) h = (((s < lm5_nAcross ? x : y) * Q.nc) / (s < lm5_nAcross ? W : H)) % 8;
      else if (cmode == LM5_MOTION) {
        int v = v3_abs(s5_x[a + i] - s5_ox[a + i]) + v3_abs(s5_y[a + i] - s5_oy[a + i]);
        lev = 3 + v / 3; if (lev > 15) lev = 15;
      }
      if (lineStyle == 2) v3_lineAA(s5_x[a + i - 1] >> 1, s5_y[a + i - 1] >> 1, s5_x[a + i] >> 1, s5_y[a + i] >> 1, lev, h);
      else {
        if (lineStyle == 1 || lm5_look == 7) {
          bool across = s < lm5_nAcross;
          v3_line(px + !across, py + across, x + !across, y + across, (uint8_t)(((lev > 5 ? lev - 5 : 1) << 4) | h));
        }
        v3_line(px, py, x, y, (uint8_t)((lev << 4) | h));
      }
      px = x; py = y;
    }
  }
}
void prog_loom_draw(int preset) { loom5_draw(preset); fx5_apply(); }
