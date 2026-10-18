// =====================================================================
// PROGRAM: BLOCKS — a field of clay shapes in waves (by Dewey) — Set 5, slot 8
//
// After ducky3d's "Cycles": a packed isometric field of soft clay shapes —
// cubes, cylinders, square tubes and round tubes, white flecked with red
// and plain red — rising and falling in slow waves that roll across the
// field. A knob morphs every shape between round and square.
//
// Presets:
//   k0 Cycles       — red and white, a diagonal wave
//   k1 Ripple       — rings spreading from the middle
//   k2 Mint & Peach — soft pastels, drifting noise
//   k3 Plaster      — all white, a chequerboard pulse
//   k4 Licorice     — charcoal and red, rolling rows
//   k5 Blueberry    — blue and white, a spiral
//   k6 Lemon        — yellow and white, a long swell
//   k7 Pistons      — orange and teal, every block on its own beat
//   k8 Terrazzo     — white flecked with black, stepped heights
//   k9 Midnight     — navy and gold, slow ripples
//   k10 Cubes       — only cubes, moving in steps
//   k11 Tour        — a new preset every 20 seconds
//
//   k12 — DROP: a splash in the middle sends a ring out through the field
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = freeze)   p2 Wave (centre = preset)
//   p3 Amplitude   p4 Wavelength   p5 Base Height   p6 Roundness (centre = mixed shapes; else all round … all square)
//   p7 Holes (how many are tubes)   p8 Colour Mix (how many are clay B)   p9 Cell Size   p10 Steps
//   p11 Speckles
//   p12-p15 are Set 5's live FX knobs (vg5fx.h), off in the centre band:
//   p12 Warp (◄ ripple | zoom ►)   p13 Mirror (◄ mirror | tiles ►)   p14 Colour (◄ cycle | flash ►)
//   p15 Glitch (◄ tear | pixels ►)
// =====================================================================

#include "vg5c.h"

enum { BK5_DIAG, BK5_RIPPLE, BK5_NOISE, BK5_CHECK, BK5_ROWS, BK5_SPIRAL, BK5_SWELL, BK5_PISTON, BK5_NWAVE };
struct Bk5Preset { C5Look look; uint8_t wave, amp, wlen, base, holes, mixB, cell, steps, speck, roundness; };   // roundness 255 = mixed
static const Bk5Preset bk5_pre[11] = {
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, BK5_DIAG, 40, 60, 30, 90, 110, 26, 0, 40, 255},
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, BK5_RIPPLE, 36, 50, 30, 90, 110, 26, 0, 40, 255},
  {{{120,160,150},{220,255,240},{140,80,60},{255,200,170},{255,140,120},{30,40,40}}, BK5_NOISE, 40, 70, 30, 90, 110, 26, 0, 30, 255},
  {{{140,140,140},{252,252,250},{140,140,140},{252,252,250},{200,200,200},{20,20,24}}, BK5_CHECK, 26, 60, 30, 60, 0, 26, 0, 0, 255},
  {{{30,30,34},{110,110,118},{90,10,20},{250,70,80},{250,200,200},{6,6,8}}, BK5_ROWS, 36, 60, 30, 90, 110, 26, 0, 20, 255},
  {{{140,150,180},{245,248,255},{10,30,110},{60,120,255},{40,90,230},{10,14,30}}, BK5_SPIRAL, 36, 60, 30, 90, 110, 26, 0, 40, 255},
  {{{160,150,120},{255,252,235},{130,100,0},{255,220,40},{240,190,20},{30,26,10}}, BK5_SWELL, 50, 120, 30, 90, 110, 26, 0, 30, 255},
  {{{20,90,90},{90,220,210},{140,50,10},{255,150,60},{255,255,255},{10,20,20}}, BK5_PISTON, 40, 60, 30, 90, 128, 26, 0, 0, 255},
  {{{150,150,150},{250,250,248},{80,80,80},{180,180,180},{20,20,20},{20,20,20}}, BK5_DIAG, 40, 60, 30, 60, 40, 24, 6, 140, 255},
  {{{20,30,70},{60,90,170},{90,60,0},{255,200,80},{255,220,120},{4,6,14}}, BK5_RIPPLE, 30, 80, 30, 90, 110, 26, 0, 20, 255},
  {{{150,150,160},{250,248,246},{90,10,20},{250,70,80},{230,40,50},{20,10,14}}, BK5_DIAG, 40, 60, 30, 0, 110, 26, 5, 40, 7},
};
static const char* const bk5_names[12] = {
  "Cycles", "Ripple", "Mint & Peach", "Plaster", "Licorice", "Blueberry", "Lemon", "Pistons", "Terrazzo", "Midnight", "Cubes", "Tour"
};
static const char* const bk5_labels[16] = {
  "Palette", "Speed", "Wave", "Amplitude", "Wavelength", "Base Height", "Roundness", "Holes",
  "Colour Mix", "Cell Size", "Steps", "Speckles", "Light Side", "Gap", "Drift", "Balls"
};
const char* prog_blocks_name() { return "BLOCKS"; }
const char* prog_blocks_character() { return "A packed field of clay cubes, cylinders and tubes rising and falling in waves"; }
const char* prog_blocks_presetName(int preset) { return (preset >= 0 && preset < 12) ? bk5_names[preset] : NULL; }
const char* prog_blocks_potLabel(int preset, int pot) { (void)preset; if (pot >= 12 && pot < 16) return fx5_labels[pot - 12]; return (pot >= 0 && pot < 16) ? bk5_labels[pot] : ""; }
uint8_t prog_blocks_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

static int bk5_preset = -1, bk5_look = 0;
static float bk5_t = 0, bk5_tour = 0, bk5_drop = -1, bk5_di = 0, bk5_dj = 0;
static bool bk5_k12 = false;
// per frame
static int bk5_wave, bk5_amp, bk5_wlen, bk5_base, bk5_holes, bk5_mixB, bk5_steps, bk5_round, bk5_scale, bk5_ti, bk5_oi, bk5_oj;
static int bk5_dropR = -1;

static int bk5_height(int i, int j) {
  int a = 0, t = bk5_ti;                                            // a: −16384 … 16384
  int wl = bk5_wlen > 4 ? bk5_wlen : 4;
  switch (bk5_wave) {
    case BK5_DIAG: a = v3_sin((i + j) * 1024 / wl * 4 / 4 * 2 - t); break;
    case BK5_RIPPLE: case BK5_SPIRAL: {
      int r = (int)v3_isqrt((uint32_t)(i * i + j * j) * 256);       // ×16
      int ang = bk5_wave == BK5_SPIRAL ? v3_atan(j, i) * 3 : 0;
      a = v3_sin(r * 64 * 2 / wl - t + ang); break;
    }
    case BK5_NOISE: a = (v3_noise1((uint32_t)(i * 400 / wl * 4 + t / 4), 11u + (uint32_t)j * 7919u) + v3_noise1((uint32_t)(j * 400 / wl * 4 + t / 5 + 9000), 23u + (uint32_t)i * 104729u)) >> 1; break;
    case BK5_CHECK: a = ((i + j) & 1) ? v3_sin(t) : -v3_sin(t); break;
    case BK5_ROWS: a = v3_sin(i * 1024 * 2 / wl - t); break;
    case BK5_SWELL: a = (v3_sin((i * 3 + j) * 1024 / (wl * 2) - t) + v3_sin((j * 2 - i) * 1024 / (wl * 3) - t * 2 / 3)) >> 1; break;
    case BK5_PISTON: { uint32_t h = v3_hash(((uint32_t)(i) * 73856093u) ^ ((uint32_t)(j) * 19349663u)); a = v3_sin((int)(h & 1023) + t * (int)(1 + ((h >> 10) & 3)) / 2); break; }
  }
  if (bk5_dropR >= 0) {                                             // the splash ring
    int r = (int)v3_isqrt((uint32_t)((i - bk5_oi) * (i - bk5_oi) + (j - bk5_oj) * (j - bk5_oj)) * 256) * 16;   // 1/256 cell … ×16
    int d = r / 16 - bk5_dropR; if (d < 0) d = -d;
    if (d < 512) a += (512 - d) * 24;
  }
  int h = bk5_base + ((a * bk5_amp) >> 14);
  if (bk5_steps > 1) h = (h / bk5_steps) * bk5_steps;
  return h < 1 ? 1 : h;
}
static void bk5_cell(int i, int j, void* cv) {
  C5Cell* c = (C5Cell*)cv;
  int wi = i + bk5_oi, wj = j + bk5_oj;
  uint32_t h = v3_hash(((uint32_t)(wi) * 73856093u) ^ ((uint32_t)(wj) * 19349663u) ^ 0xA5u);
  c->seed = h;
  c->h = bk5_height(wi, wj);
  int kind = (int)(h & 1);                                         // 0 square, 1 round
  c->L = bk5_round == 255 ? (kind ? 0 : 7) : bk5_round;
  c->hole = (int)((h >> 4) & 255) < bk5_holes ? 150 : 0;
  c->clay = (int)((h >> 12) & 255) < bk5_mixB ? 1 : 0;
  c->scale = bk5_scale;
}
// balls riding the waves
#define BK5_BALLS 10
static int bk5_nb = 0;
static void bk5_between(int d) {
  for (int b = 0; b < bk5_nb; b++) {
    int bi = (b * 7) % 9 - 4, bj = (b * 5) % 9 - 4;               // a few fixed cells near the middle
    if (bi + bj != d) continue;
    int h = bk5_height(bi + bk5_oi, bj + bk5_oj);
    int sx, sy; c5_toScreen(bi * 256, bj * 256, h, &sx, &sy);
    int r = c5_S * 5 / 12;
    c5_ballShadow(sx + 2, sy + 1, r);
    c5_ball(sx, sy - r + 1, r);
  }
}
void prog_blocks_init() { v3_reset(); bk5_preset = -1; c5_S = 0; }

static void blocks5_draw(int preset) {
  v3_buf = c5_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame();
  fx5_take();                                                      // p12-p15: the live FX knobs
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != bk5_preset) { bk5_preset = preset; bk5_look = preset == 11 ? 0 : preset; bk5_tour = 0; }
  if (preset == 11) { bk5_tour += v3_dt; if (bk5_tour > 20.0f) { bk5_tour = 0; bk5_look = (bk5_look + 1) % 11; } }
  const Bk5Preset& P = bk5_pre[bk5_look];
  const Bk5Preset& Q = bk5_pre[v3_centre(0) ? bk5_look : v3_clamp(v3_k(0) * 11 / 1024, 0, 10)];

  float speed = v3_speed(1, 3.0f);
  bk5_wave = v3_centre(2) ? P.wave : v3_clamp(v3_k(2) * BK5_NWAVE / 1024, 0, BK5_NWAVE - 1);
  bk5_amp = (int)v3_rel(3, P.amp, 2.5f);
  bk5_wlen = (int)v3_rel(4, P.wlen / 10.0f, 3.0f);
  bk5_base = (int)v3_rel(5, P.base, 2.5f);
  bk5_round = v3_centre(6) ? P.roundness : v3_clamp((int)(v3_u(6) * 8), 0, 7);
  bk5_holes = v3_centre(7) ? P.holes : (int)(v3_u(7) * 255);
  bk5_mixB = v3_centre(8) ? P.mixB : (int)(v3_u(8) * 255);
  int S = v3_centre(9) ? P.cell : 10 + (int)(v3_u(9) * 26);
  S &= ~1;
  bk5_steps = v3_centre(10) ? P.steps : (int)(v3_u(10) * 16);
  c5_speckles = v3_centre(11) ? Q.speck : (int)(v3_u(11) * 200);
  c5_light = v3_centre(12) ? 1 : (v3_k(12) < 512 ? -1 : 1);
  bk5_scale = v3_centre(13) ? 228 : 160 + (int)(v3_u(13) * 96);
  float drift = v3_centre(14) ? 0.0f : v3_bi(14) * 1.5f;
  bk5_nb = v3_centre(15) ? 0 : v3_clamp(v3_k(15) * (BK5_BALLS + 1) / 1024, 0, BK5_BALLS);
  c5_profiles(S);
  c5_palette(Q.look);

  bool k12 = keysPressed[KEY_MOD_A]; if (k12 && !bk5_k12) bk5_drop = 0; bk5_k12 = k12;
  float dt = v3_dt * speed;
  bk5_t += dt; if (bk5_t > 10000) bk5_t = 0;
  bk5_ti = (int)(bk5_t * 300);
  if (bk5_drop >= 0) { bk5_drop += dt; bk5_dropR = (int)(bk5_drop * 1500); if (bk5_drop > 4) { bk5_drop = -1; bk5_dropR = -1; } }
  bk5_di += drift * v3_dt; bk5_dj += drift * 0.6f * v3_dt;
  bk5_oi = (int)floorf(bk5_di); bk5_oj = (int)floorf(bk5_dj);
  // the field: a little sub-cell glide for smooth drifting
  int fi = (int)((bk5_di - bk5_oi) * 256), fj = (int)((bk5_dj - bk5_oj) * 256);
  c5_cx = 160 - (((fi - fj) * c5_S) >> 8); c5_top = 120 - (((fi + fj) * (c5_S / 2)) >> 8) - 10;

  memset(v3_buf, 196, W * H);                                       // the gaps between shapes: dark
  c5_field(bk5_base + bk5_amp + 30, bk5_cell, bk5_nb ? bk5_between : 0);
}
void prog_blocks_draw(int preset) { blocks5_draw(preset); fx5_apply(); }
