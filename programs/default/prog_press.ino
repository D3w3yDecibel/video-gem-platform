// =====================================================================
// PROGRAM: PRESS — ripples through a field of clay pins (by Dewey) — Set 5, slot 11
//
// The clay field as a surface you can poke: raindrops, a finger dragged
// across, a heartbeat, an earthquake. Each poke pushes blocks down and
// sends real ripples out through the field — a little wave simulation
// runs under the blocks, so rings spread, cross and bounce. Other modes
// press blocks like keys on a keyboard, or push shapes up from underneath
// like a pinscreen toy.
//
// Presets:
//   k0 Raindrops   — red and white, drops falling all over
//   k1 Finger      — a finger drawing loops across the field
//   k2 Heartbeat   — a double pulse from the middle
//   k3 Earthquake  — shocks rolling in from one side
//   k4 Typing      — blocks pressed like keys, at random
//   k5 Pinscreen   — shapes pushing the pins up from below
//   k6 Drum Skin   — beats around a ring
//   k7 Pastel Rain — soft colours, light rain
//   k8 Plaster     — all white
//   k9 Tsunami     — a long wave now and then
//   k10 Lava       — black and orange, slow blobs rising
//   k11 Tour       — a new preset every 20 seconds
//
//   k12 — SPLASH: one big drop in the middle
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = freeze)   p2 Mode (centre = preset)   p3 Strength
//   p4 Rate   p5 Damping (how long ripples last)   p6 Wave Speed   p7 Base Height   p8 Depth
//   p9 Cell Size   p10 Holes   p11 Speckles
//   p12-p15 are Set 5's live FX knobs (vg5fx.h), off in the centre band:
//   p12 Warp (◄ ripple | zoom ►)   p13 Mirror (◄ mirror | tiles ►)   p14 Colour (◄ cycle | flash ►)
//   p15 Glitch (◄ tear | pixels ►)
// =====================================================================

#include "vg5c.h"

enum { PR5_RAIN, PR5_FINGER, PR5_HEART, PR5_QUAKE, PR5_TYPING, PR5_PINS, PR5_DRUM, PR5_TSUNAMI, PR5_NMODE };
struct Pr5Preset { C5Look look; uint8_t mode, strength, rate, damp, base, depth, cell, holes, mixB, speck, depthColour; };
static const Pr5Preset pr5_pre[11] = {
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, PR5_RAIN, 120, 60, 5, 40, 100, 24, 60, 110, 40, 0},
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, PR5_FINGER, 110, 60, 4, 40, 100, 24, 60, 110, 40, 120},
  {{{150,140,150},{255,245,248},{110,0,20},{240,40,70},{230,40,50},{24,6,10}}, PR5_HEART, 200, 60, 5, 40, 60, 24, 60, 90, 40, 140},
  {{{150,150,160},{250,248,246},{40,40,46},{120,120,130},{230,40,50},{14,14,16}}, PR5_QUAKE, 160, 30, 6, 40, 60, 24, 40, 110, 40, 0},
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, PR5_TYPING, 100, 160, 5, 30, 60, 26, 0, 60, 40, 255},
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, PR5_PINS, 100, 60, 5, 20, 60, 20, 0, 0, 30, 160},
  {{{140,150,180},{245,248,255},{10,30,110},{60,120,255},{40,90,230},{10,14,30}}, PR5_DRUM, 160, 70, 5, 40, 90, 24, 60, 110, 40, 0},
  {{{120,160,150},{220,255,240},{140,80,60},{255,200,170},{255,140,120},{30,40,40}}, PR5_RAIN, 90, 40, 4, 40, 90, 24, 60, 110, 30, 0},
  {{{140,140,140},{252,252,250},{140,140,140},{252,252,250},{200,200,200},{20,20,24}}, PR5_RAIN, 120, 60, 5, 40, 100, 24, 60, 0, 0, 0},
  {{{140,150,180},{245,248,255},{0,60,80},{40,200,220},{40,90,230},{6,14,20}}, PR5_TSUNAMI, 200, 20, 7, 40, 70, 22, 40, 110, 30, 120},
  {{{20,16,14},{70,60,56},{130,30,0},{255,140,30},{255,200,80},{4,2,2}}, PR5_PINS, 110, 40, 5, 20, 70, 22, 40, 60, 10, 200},
};
static const char* const pr5_names[12] = {
  "Raindrops", "Finger", "Heartbeat", "Earthquake", "Typing", "Pinscreen", "Drum Skin", "Pastel Rain", "Plaster", "Tsunami", "Lava", "Tour"
};
static const char* const pr5_labels[16] = {
  "Palette", "Speed", "Mode", "Strength", "Rate", "Damping", "Wave Speed", "Base Height",
  "Depth", "Cell Size", "Holes", "Speckles", "Light Side", "Colour Mix", "Roundness", "Colour by Depth"
};
const char* prog_press_name() { return "PRESS"; }
const char* prog_press_character() { return "Ripples running through a field of clay pins: raindrops, a finger, a heartbeat, keys and a pinscreen"; }
const char* prog_press_presetName(int preset) { return (preset >= 0 && preset < 12) ? pr5_names[preset] : NULL; }
const char* prog_press_potLabel(int preset, int pot) { (void)preset; if (pot >= 12 && pot < 16) return fx5_labels[pot - 12]; return (pot >= 0 && pot < 16) ? pr5_labels[pot] : ""; }
uint8_t prog_press_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

#define PR5_N 26                                       // the simulated patch: cells −13 … 12 each way
#define PR5_O 13
static int16_t *pr5_u, *pr5_v;                          // the surface now and a step ago (in the shared memory)
static int pr5_preset = -1, pr5_look = 0, pr5_mode = 0, pr5_base = 40, pr5_depth = 60, pr5_holes = 60, pr5_mixB = 110, pr5_round = 255, pr5_dcol = 0;
static float pr5_t = 0, pr5_tour = 0, pr5_acc = 0, pr5_beat = 0;
static bool pr5_k12 = false;

static inline int16_t& pr5_at(int16_t* a, int i, int j) { return a[(i + PR5_O) * PR5_N + (j + PR5_O)]; }
static void pr5_poke(int i, int j, int amt, int rad) {              // push the surface down around (i, j)
  for (int a = -rad; a <= rad; a++) for (int b = -rad; b <= rad; b++) {
    int ii = i + a, jj = j + b;
    if (ii < -PR5_O + 1 || ii > PR5_N - PR5_O - 2 || jj < -PR5_O + 1 || jj > PR5_N - PR5_O - 2) continue;
    int w = rad == 0 ? 1 : (rad + 1 - (v3_abs(a) > v3_abs(b) ? v3_abs(a) : v3_abs(b)));
    pr5_at(pr5_u, ii, jj) = (int16_t)v3_clamp(pr5_at(pr5_u, ii, jj) - amt * w / (rad + 1), -12000, 12000);
  }
}
static void pr5_step(int damp) {                                   // one step of the ripple: new = (sum of 4 neighbours)/2 − old
  for (int i = 1; i < PR5_N - 1; i++) for (int j = 1; j < PR5_N - 1; j++) {
    int k = i * PR5_N + j;
    int n = ((pr5_u[k - 1] + pr5_u[k + 1] + pr5_u[k - PR5_N] + pr5_u[k + PR5_N]) >> 1) - pr5_v[k];
    n -= n >> damp;
    pr5_v[k] = (int16_t)v3_clamp(n, -12000, 12000);
  }
  int16_t* t = pr5_u; pr5_u = pr5_v; pr5_v = t;
}
static int pr5_height(int i, int j) {
  if (i < -PR5_O + 1 || i > PR5_N - PR5_O - 2 || j < -PR5_O + 1 || j > PR5_N - PR5_O - 2) return pr5_base;
  int u = pr5_at(pr5_u, i, j);
  return pr5_base + (u * pr5_depth >> 11);
}
static void pr5_cell(int i, int j, void* cv) {
  C5Cell* c = (C5Cell*)cv;
  uint32_t h = v3_hash(((uint32_t)(i) * 73856093u) ^ ((uint32_t)(j) * 19349663u) ^ 0x77u);
  c->seed = h;
  int z = pr5_height(i, j);
  c->h = z < 1 ? 1 : z;
  c->L = pr5_round == 255 ? ((h & 1) ? 0 : 7) : pr5_round;
  if (pr5_mode == PR5_TYPING && v3_centre(14)) c->L = 7;
  c->hole = (int)((h >> 4) & 255) < pr5_holes ? 150 : 0;
  int clay = (int)((h >> 12) & 255) < pr5_mixB ? 1 : 0;
  if (pr5_dcol > 0) { int d = pr5_base - z; if (d * pr5_dcol > 2600 || -d * pr5_dcol > 5200) clay ^= 1; }   // pressed deep (or pushed high): the other clay
  c->clay = clay;
  c->scale = 232;
}
void prog_press_init() {
  v3_reset(); pr5_preset = -1; c5_S = 0;
  pr5_u = (int16_t*)v3_scratch; pr5_v = pr5_u + PR5_N * PR5_N;
  memset(v3_scratch, 0, PR5_N * PR5_N * 4);
}

static void press5_draw(int preset) {
  v3_buf = c5_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame();
  fx5_take();                                                      // p12-p15: the live FX knobs
  if (!pr5_u) { pr5_u = (int16_t*)v3_scratch; pr5_v = pr5_u + PR5_N * PR5_N; }
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != pr5_preset) { pr5_preset = preset; pr5_look = preset == 11 ? 0 : preset; pr5_tour = 0; memset(v3_scratch, 0, PR5_N * PR5_N * 4); pr5_u = (int16_t*)v3_scratch; pr5_v = pr5_u + PR5_N * PR5_N; }
  if (preset == 11) { pr5_tour += v3_dt; if (pr5_tour > 20.0f) { pr5_tour = 0; pr5_look = (pr5_look + 1) % 11; } }
  const Pr5Preset& P = pr5_pre[pr5_look];
  const Pr5Preset& Q = pr5_pre[v3_centre(0) ? pr5_look : v3_clamp(v3_k(0) * 11 / 1024, 0, 10)];

  float speed = v3_speed(1, 3.0f);
  pr5_mode = v3_centre(2) ? P.mode : v3_clamp(v3_k(2) * PR5_NMODE / 1024, 0, PR5_NMODE - 1);
  int strength = (int)v3_rel(3, P.strength, 2.5f) * 16;
  float rate = v3_rel(4, P.rate / 20.0f, 3.0f);
  int damp = v3_centre(5) ? P.damp : 2 + (int)(v3_u(5) * 7);
  int wsteps = v3_centre(6) ? 1 : 1 + (int)(v3_u(6) * 2.99f);
  pr5_base = (int)v3_rel(7, P.base, 2.5f);
  pr5_depth = (int)v3_rel(8, P.depth, 2.5f);
  int S = v3_centre(9) ? P.cell : 12 + (int)(v3_u(9) * 24); S &= ~1;
  pr5_holes = v3_centre(10) ? P.holes : (int)(v3_u(10) * 255);
  c5_speckles = v3_centre(11) ? Q.speck : (int)(v3_u(11) * 200);
  c5_light = v3_centre(12) ? 1 : (v3_k(12) < 512 ? -1 : 1);
  pr5_mixB = v3_centre(13) ? Q.mixB : (int)(v3_u(13) * 255);
  pr5_round = v3_centre(14) ? 255 : v3_clamp((int)(v3_u(14) * 8), 0, 7);
  pr5_dcol = v3_centre(15) ? P.depthColour : (int)(v3_u(15) * 255);
  c5_profiles(S);
  c5_palette(Q.look);

  bool k12 = keysPressed[KEY_MOD_A]; if (k12 && !pr5_k12) pr5_poke(0, 0, strength * 3, 2); pr5_k12 = k12;
  float dt = v3_dt * speed;
  pr5_t += dt;

  // ── what pokes the surface ──
  if (speed > 0.0f) {
    pr5_acc += rate * dt;
    switch (pr5_mode) {
      case PR5_RAIN: case PR5_TYPING:
        while (pr5_acc >= 1.0f) { pr5_acc -= 1.0f; pr5_poke(v3_rndr(-9, 9), v3_rndr(-9, 9), strength, pr5_mode == PR5_TYPING ? 0 : 2); }
        break;
      case PR5_FINGER: {
        float a = pr5_t * 0.9f; int fi = (int)(7.0f * sinf(a)), fj = (int)(6.0f * sinf(a * 1.7f + 1.0f));
        pr5_poke(fi, fj, strength / 3, 1); pr5_acc = 0; break;
      }
      case PR5_HEART: {
        float ph = fmodf(pr5_t * 1.1f, 1.0f);
        if (pr5_beat < 0.05f && ph >= 0.05f) pr5_poke(0, 0, strength, 2);
        if (pr5_beat < 0.25f && ph >= 0.25f) pr5_poke(0, 0, strength * 2 / 3, 2);
        pr5_beat = ph; pr5_acc = 0; break;
      }
      case PR5_QUAKE: case PR5_TSUNAMI:
        while (pr5_acc >= 1.0f) { pr5_acc -= 1.0f; int j0 = pr5_mode == PR5_QUAKE ? v3_rndr(-9, 9) : 0; for (int j = -11; j <= 11; j++) if (pr5_mode == PR5_TSUNAMI || v3_abs(j - j0) < 4) pr5_poke(-11, j, strength, 0); }
        break;
      case PR5_DRUM:
        while (pr5_acc >= 1.0f) { pr5_acc -= 1.0f; float a = v3_rndf() * 6.2832f; pr5_poke((int)(7 * cosf(a)), (int)(7 * sinf(a)), strength, 1); }
        break;
      case PR5_PINS: break;
      default: break;
    }
    if (pr5_mode == PR5_TYPING) {                                    // keys spring back on their own: no ripples
      for (int k = 0; k < PR5_N * PR5_N; k++) { pr5_u[k] = (int16_t)(pr5_u[k] - pr5_u[k] / 6); pr5_v[k] = pr5_u[k]; }
    } else if (pr5_mode == PR5_PINS) {                               // shapes moving under the pins push them up
      int bx[3], by[3];                                            // where the three shapes are (1/16 cell), worked out once
      for (int b = 0; b < 3; b++) {
        bx[b] = (int)(7.0f * 16 * sinf(pr5_t * (0.31f + b * 0.17f) + b * 2.1f));
        by[b] = (int)(7.0f * 16 * sinf(pr5_t * (0.23f + b * 0.11f) + b * 1.3f + 0.7f));
        if (pr5_look == 10 && v3_centre(2)) by[b] = (int)((fmodf(pr5_t * (0.6f + b * 0.2f) + b * 5.0f, 26.0f) - 13.0f) * 16);   // lava: blobs rising through
      }
      for (int i = 0; i < PR5_N; i++) for (int j = 0; j < PR5_N; j++) {
        int best = 0;
        for (int b = 0; b < 3; b++) {
          int dx = (i - PR5_O) * 16 - bx[b], dy = (j - PR5_O) * 16 - by[b], d2 = (dx * dx + dy * dy) >> 8;   // in cells²
          int v = d2 < 16 ? (16 - d2) * strength / 16 : 0;
          if (v > best) best = v;
        }
        pr5_u[i * PR5_N + j] = (int16_t)v3_clamp(best, -12000, 12000); pr5_v[i * PR5_N + j] = pr5_u[i * PR5_N + j];
      }
    } else for (int s = 0; s < wsteps; s++) pr5_step(damp);
  }

  // ── draw ──
  c5_cx = 160; c5_top = 112;
  memset(v3_buf, 196, W * H);
  c5_field(pr5_base + pr5_depth * 6 + 30, pr5_cell, 0);
}
void prog_press_draw(int preset) { press5_draw(preset); fx5_apply(); }
