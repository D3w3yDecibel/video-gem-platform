// =====================================================================
// PROGRAM: CHAOS CROWN (by Dewey)
// A ring of double pendulums hanging from one pivot, each swinging in its
// own vertical plane, fanned all the way round like a crown and seen in
// 3D. They start almost identical (a tiny angle apart), so at first the
// crown moves as one perfect shape, then the differences grow and it
// shatters into chaos, leaving rainbow light trails. k12 relaunches it.
// Inspired by mathswithmuza's "Chaos Theory in 3D — 50 pendulums, 0.1°
// apart" (https://www.instagram.com/p/Dbq7yDXskGg/). The code is original.
//
// Presets:
//   k0 Crown       — the classic: a ring of pendulums seen from above at an angle
//   k1 Fan         — all in one plane, facing you: the 2D chaos fan
//   k2 Top Bloom   — the crown seen from straight above: a flower
//   k3 Carousel    — pivots round a ring, swinging sideways: a whirl
//   k4 Wave Bar    — a row of pendulums hanging from a bar
//   k5 Light Paint — no rods, just bobs painting long trails
//   k6 Solo        — one big double pendulum with a long colour-changing trail
//   k7 Chandelier  — three crowns stacked, different sizes
//
//   It LOOPS: the crown falls into chaos, eases to a stop, then time runs
//   backwards — every pendulum retraces its path exactly and the crown
//   re-forms, at rest — and it falls again. Seamless, forever.
//   k12 — REVERSE TIME now (eases to a stop first; press again to go forward)
//   Shift + k12 — floor reflection on / off (a glossy black floor under the crown)
//
// Knobs (this program owns the global knobs and makes its own colours):
//   p0 Palette (8 schemes, centre = Rainbow)    p1 Speed (far left = freeze)
//   p2 Count (8 … 64, centre 50)   p3 Spread (start difference, centre 0.1°)
//   p4 Start Angle   p5 Arm Ratio (2nd arm length)   p6 Trails
//   p7 Rods (brightness, far left = hidden)   p8 Spin (centre = still)
//   p9 Tilt   p10 Zoom (0.35× … 4×, zooms round the panned spot)
//   p11 Loop Length (4 … 60 simulated seconds of falling before it rewinds,
//       centre ≈ 16 s; far right = never rewind). Start Angle, Spread and Arm
//       Ratio changes take effect at the next crown (the loop's seam).
//   p12 Pan X (right = look right)   p14 Pan Y (right = look up)
//   p13 Color: left of centre = DRIFT colour (pendulums stay white while in
//       step and light up in colour as they drift apart — the chaos spreading
//       round the ring); centre = plain rainbow; right of centre = colour cycle
//   p15 Bobs (size and glow together)
//
// How it works: each pendulum is two bobs on stiff rods, stepped with an
// integer Verlet rule that is exactly reversible (see Physics below).
// Positions are turned into 3D (each pendulum's plane rotated round the
// vertical axis), spun and tilted by the camera, and drawn in perspective.
// Every pixel stores (brightness << 4) | hue; brighter wins, and a quick
// fade of the brightness leaves the trails. Sines come from a table.
// =====================================================================

// ─── Maths ────────────────────────────────────────────────────────────
#include "vg_sin.h"
#define cb_sinT vg_sin512                        // shared sine table (in flash: no RAM)
#define CB_TWO_PI 6.28318531f
#define CB_PI     3.14159265f

// fast sine/cosine for angles in about −4π … 4π (table + straight-line blend)
static inline float cb_sin(float a) {
  float fi = a * (512.0f / CB_TWO_PI);
  int i = (int)(fi + 8192.0f) - 8192;            // floor
  float fr = fi - (float)i;
  int s0 = cb_sinT[i & 511], s1 = cb_sinT[(i + 1) & 511];
  return ((float)s0 + (float)(s1 - s0) * fr) * (1.0f / 16384.0f);
}
static inline int cb_min(int a, int b) { return a < b ? a : b; }
static inline int cb_c16(int v) { return v < -4000 ? -4000 : (v > 4000 ? 4000 : v); }
static inline float cb_cos(float a) { return cb_sin(a + CB_PI * 0.5f); }
static inline float cb_wrap(float a) {           // keep angles in −π … π
  while (a > CB_PI) a -= CB_TWO_PI;
  while (a < -CB_PI) a += CB_TWO_PI;
  return a;
}

// ─── Colours ──────────────────────────────────────────────────────────
// palette index = (brightness << 4) | hue;  hues 0-14 = the scheme's
// colour wheel, hue 15 = the scheme's "white" (rods, inner bobs)
#define CB_WHITE 15
#define CB_NSCHEME 8
static const char* const cb_schemeNames[CB_NSCHEME] = {
  "Mono", "Ice", "Ocean", "Pastel", "Rainbow", "Neon", "Sunset", "Fire"
};
// six colour stops round the wheel, then the scheme's white
static const uint8_t cb_stops[CB_NSCHEME][7][3] = {
  {{255,255,255},{200,205,215},{150,160,180},{220,225,235},{180,185,200},{240,240,250},{235,235,240}},  // Mono
  {{0,90,255},{0,200,255},{150,240,255},{255,255,255},{120,150,255},{60,40,255},{210,235,255}},        // Ice
  {{0,60,200},{0,170,255},{0,255,200},{40,255,120},{0,200,170},{0,110,255},{200,255,240}},             // Ocean
  {{255,150,170},{255,210,150},{255,255,170},{160,255,190},{160,200,255},{220,160,255},{255,245,250}}, // Pastel
  {{255,30,30},{255,170,0},{230,255,0},{0,230,90},{0,140,255},{200,0,255},{235,235,245}},             // Rainbow
  {{255,0,160},{255,0,255},{120,0,255},{0,120,255},{0,255,255},{0,255,120},{240,220,255}},            // Neon
  {{255,40,90},{255,90,40},{255,170,40},{255,220,90},{255,110,170},{170,60,220},{255,235,215}},        // Sunset
  {{255,0,0},{255,70,0},{255,140,0},{255,220,40},{255,120,0},{220,20,0},{255,240,200}},               // Fire
};
static int cb_flash = 0;

static void cb_buildPalette(int scheme, int cyc) {
  static const uint8_t gain[12] = {0, 30, 54, 78, 102, 126, 150, 174, 198, 220, 240, 255};
  for (int h = 0; h < 16; h++) {
    int r, g, b;
    if (h == CB_WHITE) {
      r = cb_stops[scheme][6][0]; g = cb_stops[scheme][6][1]; b = cb_stops[scheme][6][2];
    } else {
      int pos = ((h * 256 / 15) + cyc) & 255;
      int p6 = pos * 6, s0 = p6 >> 8, f = p6 & 255, s1 = (s0 + 1) % 6;
      r = (cb_stops[scheme][s0][0] * (256 - f) + cb_stops[scheme][s1][0] * f) >> 8;
      g = (cb_stops[scheme][s0][1] * (256 - f) + cb_stops[scheme][s1][1] * f) >> 8;
      b = (cb_stops[scheme][s0][2] * (256 - f) + cb_stops[scheme][s1][2] * f) >> 8;
    }
    display.setColor(h, 0, 0, 0);
    for (int l = 1; l < 16; l++) {
      int rr, gg, bb;
      if (l <= 11) { int m = gain[l]; rr = r * m >> 8; gg = g * m >> 8; bb = b * m >> 8; }
      else { int wv = (l - 11) * 36; rr = r + ((255 - r) * wv >> 8); gg = g + ((255 - g) * wv >> 8); bb = b + ((255 - b) * wv >> 8); }
      if (cb_flash) { rr = (rr + 255) >> 1; gg = (gg + 255) >> 1; bb = (bb + 255) >> 1; }
      display.setColor((l << 4) | h, rr, gg, bb);
    }
  }
}

// ─── Drawing ──────────────────────────────────────────────────────────
static uint8_t* cb_buf;

// Live parts (rods, bobs) are erased again next frame so they never smear;
// in erase mode a pixel is cleared only if it still holds exactly what we
// drew there (anything brighter drawn on top — a trail — is left alone).
static bool cb_erase = false;
static inline void cb_put(int x, int y, int hue, int l) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H || l <= 0) return;
  if (l > 15) l = 15;
  uint8_t* p = cb_buf + y * W + x;
  uint8_t v = (uint8_t)((l << 4) | hue);
  if (cb_erase) { if (*p == v) *p = 0; return; }
  if ((*p >> 4) < l) *p = v;
}

static void cb_line(int x0, int y0, int x1, int y1, int hue, int l) {
  if (l <= 0) return;
  if ((x0 < 0 && x1 < 0) || (x0 >= W && x1 >= W) || (y0 < 0 && y1 < 0) || (y0 >= H && y1 >= H)) return;
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, n = 0;
  for (;;) {
    cb_put(x0, y0, hue, l);
    if ((x0 == x1 && y0 == y1) || ++n > 900) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

// a round bob: bright core, coloured body, optional glow ring
static void cb_bob(int cx, int cy, int r, int hue, int l, int glow) {
  if (cx < -8 || cx >= W + 8 || cy < -8 || cy >= H + 8) return;
  int rg = r + (glow > 0 ? 1 + (glow >> 2) : 0);
  for (int dy = -rg; dy <= rg; dy++)
    for (int dx = -rg; dx <= rg; dx++) {
      int d2 = dx * dx + dy * dy;
      if (d2 <= r * r) cb_put(cx + dx, cy + dy, hue, d2 * 3 <= r * r ? 15 : l);
      else if (glow > 0 && d2 <= rg * rg + rg) cb_put(cx + dx, cy + dy, hue, glow);
    }
}

// Fade every pixel's brightness by dec, four pixels at a time.
static inline uint32_t cb_fade4(uint32_t x, uint32_t d4) {
  uint32_t h = (x >> 4) & 0x0F0F0F0Fu;
  uint32_t t = (h | 0x80808080u) - d4;
  uint32_t m = ((t & 0x80808080u) >> 7) * 0xFFu;
  return (((t & 0x7F7F7F7Fu) & m) << 4) | (x & 0x0F0F0F0Fu);
}
static void cb_fade(int dec) {
  uint32_t* b = (uint32_t*)cb_buf;
  uint32_t d4 = (uint32_t)dec * 0x01010101u;
  for (int i = 0; i < W * H / 4; i++) b[i] = cb_fade4(b[i], d4);
}

// ─── Physics: exactly reversible, so the animation can loop ───────────
// Each pendulum lives in its own vertical plane: two bobs (mass 1) on two
// very stiff rods (springs), pulled by gravity. Positions are whole numbers
// (Q24: 16777216 = one arm length) and are stepped with the integer
// Verlet rule   next = 2·now − before + round(h²·force(now)).
// That rule is perfectly symmetric in time: swap "before" and "now" and it
// retraces its path back to the start, bit for bit — the chaos un-mixes and
// the crown re-forms exactly. Starting from rest, t = 0 is a natural mirror,
// so forward → rewind → forward is one seamless loop.
#define CB_MAXN 64
#define CB_Q    16777216                         // 1.0 in Q24
#define CB_HZ   512                              // physics steps per simulated second
#define CB_G    628                              // h²·g in Q24 (9.81 / 512²)
#define CB_KS   0.1717f                          // h²·k/2 (rod stiffness k = 90000)
static int32_t cb_x1[CB_MAXN], cb_y1[CB_MAXN], cb_x2[CB_MAXN], cb_y2[CB_MAXN];   // now
static int32_t cb_vx1[CB_MAXN], cb_vy1[CB_MAXN], cb_vx2[CB_MAXN], cb_vy2[CB_MAXN]; // now − before
static uint8_t cb_drift[CB_MAXN];                // how far each one has drifted from its neighbour (0..255)
#define CB_HIST 4
static int16_t cb_hx[CB_MAXN][CB_HIST], cb_hy[CB_MAXN][CB_HIST];   // last few outer-bob positions (newest first)
static int cb_n = 50;
static float cb_L1 = 1.0f, cb_L2 = 1.0f;         // arm lengths this loop was launched with
static int64_t cb_LL1 = (int64_t)CB_Q * CB_Q, cb_LL2 = (int64_t)CB_Q * CB_Q;   // L² in Q48
static int32_t cb_c1 = 0, cb_c2 = 0;             // h²·k/(2L²) in Q24
static bool cb_fresh = true;                     // no previous trail point yet

static inline int32_t cb_rq(int64_t v) { return (int32_t)((v + (1LL << 23)) >> 24); }   // Q48 → Q24, rounded

// one Verlet step for every pendulum
static void cb_step() {
  for (int i = 0; i < cb_n; i++) {
    int32_t x1 = cb_x1[i], y1 = cb_y1[i], x2 = cb_x2[i], y2 = cb_y2[i];
    int32_t dx = x2 - x1, dy = y2 - y1;
    // rod 1 (pivot → bob 1) and rod 2 (bob 1 → bob 2): pull = c·(len² − L²)·direction
    int32_t e1 = cb_rq((int64_t)x1 * x1 + (int64_t)y1 * y1 - cb_LL1);
    int32_t e2 = cb_rq((int64_t)dx * dx + (int64_t)dy * dy - cb_LL2);
    int32_t f1 = cb_rq((int64_t)e1 * cb_c1), f2 = cb_rq((int64_t)e2 * cb_c2);
    int32_t ax2 = -cb_rq((int64_t)f2 * dx), ay2 = -cb_rq((int64_t)f2 * dy);
    int32_t ax1 = -cb_rq((int64_t)f1 * x1) - ax2, ay1 = -cb_rq((int64_t)f1 * y1) - ay2;
    cb_vx1[i] += ax1; cb_vy1[i] += ay1 - CB_G;
    cb_vx2[i] += ax2; cb_vy2[i] += ay2 - CB_G;
    cb_x1[i] = x1 + cb_vx1[i]; cb_y1[i] = y1 + cb_vy1[i];
    cb_x2[i] = x2 + cb_vx2[i]; cb_y2[i] = y2 + cb_vy2[i];
  }
}
// turn time round: "before" and "now" swap places
static void cb_reverse() {
  for (int i = 0; i < cb_n; i++) {
    cb_x1[i] -= cb_vx1[i]; cb_y1[i] -= cb_vy1[i]; cb_x2[i] -= cb_vx2[i]; cb_y2[i] -= cb_vy2[i];
    cb_vx1[i] = -cb_vx1[i]; cb_vy1[i] = -cb_vy1[i]; cb_vx2[i] = -cb_vx2[i]; cb_vy2[i] = -cb_vy2[i];
  }
}

// how far apart each pendulum is from its neighbour, smoothed (for Drift colour)
static void cb_measureDrift() {
  for (int i = 0; i < cb_n; i++) {
    int j = i > 0 ? i - 1 : cb_n - 1;
    uint32_t d = (uint32_t)abs(cb_x1[i] - cb_x1[j]) + (uint32_t)abs(cb_y1[i] - cb_y1[j])
               + (uint32_t)abs(cb_x2[i] - cb_x2[j]) + (uint32_t)abs(cb_y2[i] - cb_y2[j]);
    int v = (int)(d >> 18);                      // ~0.5 rad apart → 64
    if (v > 255) v = 255;
    cb_drift[i] = (uint8_t)((cb_drift[i] * 7 + v) >> 3);
  }
}

// everyone in step and at rest: start angle a0, each one `spread` radians apart
static void cb_launch(int n, float a0, float spread, float L2) {
  cb_n = n;
  cb_L1 = 1.0f; cb_L2 = L2;
  int32_t q1 = CB_Q, q2 = (int32_t)(L2 * CB_Q);
  cb_LL1 = (int64_t)q1 * q1; cb_LL2 = (int64_t)q2 * q2;
  cb_c1 = (int32_t)(CB_KS * CB_Q); cb_c2 = (int32_t)(CB_KS / (L2 * L2) * CB_Q);
  for (int i = 0; i < n; i++) {
    float a = a0 + spread * (float)i;
    float sa = sinf(a), ca = cosf(a);
    cb_x1[i] = (int32_t)lroundf(sa * CB_Q);  cb_y1[i] = (int32_t)lroundf(-ca * CB_Q);
    cb_x2[i] = cb_x1[i] + (int32_t)lroundf(sa * q2); cb_y2[i] = cb_y1[i] - (int32_t)lroundf(ca * q2);
    cb_vx1[i] = cb_vy1[i] = cb_vx2[i] = cb_vy2[i] = 0;
    cb_drift[i] = 0;
  }
  cb_fresh = true;
}

// ─── Camera ───────────────────────────────────────────────────────────
static float cb_cy = 1, cb_sy = 0, cb_cp = 1, cb_sp = 0;   // yaw / pitch cos & sin
static float cb_scale = 60.0f, cb_dist = 6.0f;
static int cb_ox = HALFW, cb_oy = 90;
static float cb_depth;                           // depth of the last projected point (+ = far)

static inline void cb_proj(float x, float y, float z, int* sx, int* sy) {
  float xr = x * cb_cy - z * cb_sy, zr = x * cb_sy + z * cb_cy;
  float yr = y * cb_cp - zr * cb_sp, zz = y * cb_sp + zr * cb_cp;
  float k = cb_scale * cb_dist / (cb_dist + zz);
  cb_depth = zz;
  *sx = cb_ox + (int)(xr * k);
  *sy = cb_oy - (int)(yr * k);
}
// brightness factor from depth: near = brighter (0.55 … 1.25, as Q8)
static inline int cb_dq() {
  int q = (int)(256.0f * (1.0f - cb_depth / (cb_dist * 1.6f)));
  return q < 140 ? 140 : (q > 320 ? 320 : q);
}

// ─── Live layer: what was drawn last frame, so it can be erased ───────
static int16_t cb_lx[CB_MAXN][6];               // pivot, inner bob, outer bob (screen)
static uint8_t cb_lh[CB_MAXN], cb_lc[CB_MAXN];   // outer hue, inner (contrast) hue
static uint8_t cb_ll[CB_MAXN][4];               // rod 1, rod 2, inner bob, outer bob levels
static int cb_liveN = 0, cb_lBob = 2, cb_lGlow = 0, cb_lPreset = 0;
static int16_t cb_lExtra[4];                    // pivot / bar
static bool cb_lRods = true;
static int16_t cb_lr[CB_MAXN][6];               // the same three points mirrored in the floor
static bool cb_lRefl = false;
static bool cb_reflOn = true;                    // Shift + k12 toggles
static int16_t cb_rpx[CB_MAXN], cb_rpy[CB_MAXN]; // last reflected outer-bob position

static void cb_live(int i) {
  const int16_t* q = cb_lx[i];
  if (cb_lRods) {
    cb_line(q[0], q[1], q[2], q[3], CB_WHITE, cb_ll[i][0]);
    cb_line(q[2], q[3], q[4], q[5], cb_lh[i], cb_ll[i][1]);
  }
  if (cb_lPreset != 5) cb_bob(q[2], q[3], cb_lBob > 1 ? cb_lBob - 1 : 1, cb_lc[i], cb_ll[i][2], cb_lGlow >> 1);
  cb_bob(q[4], q[5], cb_lBob, cb_lh[i], cb_ll[i][3], cb_lGlow);
  if (cb_lPreset == 6 || cb_lPreset == 4 || cb_lPreset == 3) cb_bob(q[0], q[1], 1, CB_WHITE, 10, 0);
  if (cb_lRefl && cb_lRods) {                                     // reflected rods, faint
    const int16_t* r = cb_lr[i];
    int l0 = cb_ll[i][0] / 3, l1 = cb_ll[i][1] / 3;
    cb_line(r[0], r[1], r[2], r[3], CB_WHITE, l0 > 0 ? l0 : 1);
    cb_line(r[2], r[3], r[4], r[5], cb_lh[i], l1 > 0 ? l1 : 1);
  }
}
static void cb_liveExtras() {
  if (cb_lPreset == 4) cb_line(cb_lExtra[0], cb_lExtra[1], cb_lExtra[2], cb_lExtra[3], CB_WHITE, 9);
  else if (cb_lPreset != 3 && cb_lPreset != 6) cb_bob(cb_lExtra[0], cb_lExtra[1], 1, CB_WHITE, 14, cb_lGlow);
}

// ─── Knobs ────────────────────────────────────────────────────────────
static float cb_sm[16];
static bool cb_smReady = false;
static inline int cb_k(int i) { int v = (int)cb_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_chaos_name() { return "CHAOS CROWN"; }
const char* prog_chaos_character() { return "A crown of double pendulums that start in step and shatter into rainbow chaos"; }

static const char* const cb_presetNames[] = {
  "Crown", "Fan", "Top Bloom", "Carousel", "Wave Bar", "Light Paint", "Solo", "Chandelier"
};
#define CB_NUM_PRESETS 8

const char* prog_chaos_presetName(int preset) {
  if (preset >= 0 && preset < CB_NUM_PRESETS) return cb_presetNames[preset];
  return NULL;
}

static const char* const cb_labels[16] = {
  "Palette", "Speed", "Count", "Spread", "Start Angle", "Arm Ratio", "Trails", "Rods",
  "Spin", "Tilt", "Zoom", "Loop Length", "Pan X", "Color", "Pan Y", "Bobs"
};
const char* prog_chaos_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 16) return cb_labels[pot];
  return "";
}

uint8_t prog_chaos_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // we fade the last frame ourselves (trails)
}

static int cb_lastPreset = -1;

void prog_chaos_init() {
  static bool ready = false;
  if (!ready) {
    ready = true;
  }
  cb_lastPreset = -1;                            // relaunch on the first frame
  cb_liveN = 0;
  if (display.getBuffer()) memset(display.getBuffer(), 0, W * H);
}

void prog_chaos_draw(int preset) {
  cb_buf = display.getBuffer();
  if (!cb_buf) return;
  if (preset < 0 || preset >= CB_NUM_PRESETS) preset = 0;

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  {
    float a = dt / 0.12f;
    if (a > 1.0f) a = 1.0f;
    for (int i = 0; i < 16; i++) {
      if (!cb_smReady) cb_sm[i] = pots[i];
      else cb_sm[i] += (pots[i] - cb_sm[i]) * a;
    }
    cb_smReady = true;
  }

  // ── knobs ──
  float sk = cb_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.15f * powf(13.0f, (sk - 0.03f) / 0.97f);   // centre ≈ 0.55×
  int ck = cb_k(2);
  int count = ck < 512 ? 8 + ck * 42 / 512 : 50 + (ck - 512) * 14 / 511;          // centre 50
  if (count > CB_MAXN) count = CB_MAXN;
  float spreadDeg = 0.1f * powf(10.0f, (cb_k(3) - 512) / 256.0f);                // 0.001° … 10°, centre 0.1°
  float a0 = (20.0f + cb_k(4) * 160.0f / 1023.0f) * (CB_PI / 180.0f);           // 20° … 180°, centre 100°
  float armK = 0.4f + cb_k(5) * 1.2f / 1023.0f;                                  // 0.4 … 1.6, centre 1.0
  int trails = cb_k(6);
  int rods = cb_k(7) * 9 / 1023;                                                 // 0 = hidden … 9
  int bk = cb_k(15);                                                             // Bobs: size, then glow
  int bobR = 1 + bk * 3 / 1024;                                                  // 1 … 3
  int glow = bk < 300 ? 0 : (bk - 300) * 7 / 724;                                // glow comes in past ~30 %
  // Loop: how long it falls apart before rewinding (simulated seconds), far right = never rewind
  float loopSec = cb_k(11) > 990 ? 0.0f : 4.0f * powf(15.0f, cb_k(11) / 990.0f);   // 4 … 60 s, centre ≈ 16 s

  // ── per-preset layout ──
  if (preset == 6) count = 1;                                    // Solo
  if (preset == 7) count = (count / 3) * 3 < 6 ? 6 : (count / 3) * 3;   // Chandelier: 3 rings
  float pitchBase = 28.0f, spinBase = 6.0f, zoomBase = 1.0f, yawBase = 0.0f;
  switch (preset) {
    case 1: pitchBase = 0.0f;  spinBase = 0.0f; break;                   // Fan
    case 2: pitchBase = 82.0f; spinBase = 4.0f; break;                   // Top Bloom
    case 3: pitchBase = 32.0f; spinBase = 8.0f; zoomBase = 0.8f; break;  // Carousel
    case 4: pitchBase = 18.0f; spinBase = 0.0f; zoomBase = 0.85f; yawBase = 0.65f; break; // Wave Bar
    case 5: pitchBase = 35.0f; spinBase = 3.0f; break;                   // Light Paint
    case 6: pitchBase = 0.0f;  spinBase = 0.0f; zoomBase = 1.25f; break; // Solo
    case 7: pitchBase = 24.0f; spinBase = 7.0f; zoomBase = 0.8f; break;  // Chandelier
    default: break;
  }

  // ── the loop ──
  // idx counts physics steps from the crown (t = 0). Forward: idx grows until the
  // turn point, where time eases to a halt and swaps direction; the rewind then
  // runs back to t = 0 (the crown, at rest) and carries straight on forward again.
  static int32_t idx = 0, flipAt = -1;
  static int dir = 1;
  static float stepAcc = 0;
  static float lA0 = -1, lSpread = -1, lArm = -1;
  bool changed = fabsf(a0 - lA0) > 0.004f || fabsf(spreadDeg - lSpread) > lSpread * 0.03f || fabsf(armK - lArm) > 0.015f;
  bool relaunch = (preset != cb_lastPreset) || (count != cb_n);
  if (changed && (loopSec <= 0 || lA0 < 0)) relaunch = true;     // free-running: apply now; looping: at the seam
  if (relaunch) {
    cb_launch(count, a0, spreadDeg * (CB_PI / 180.0f), armK);
    lA0 = a0; lSpread = spreadDeg; lArm = armK;
    idx = 0; dir = 1; flipAt = -1; stepAcc = 0;
    if (preset != cb_lastPreset) { memset(cb_buf, 0, W * H); cb_liveN = 0; }
    cb_lastPreset = preset;
  }
  int32_t loopN = loopSec > 0 ? (int32_t)(loopSec * CB_HZ) : 0x7FFFFFF0;
  int32_t easeN = (int32_t)(CB_HZ * 1.1f * (speed > 0.2f ? speed : 0.2f));   // ~1 s of slow-motion at the turn
  // k12: reverse time now (smoothly: it slows to a stop first)
  static bool k12Was = false;
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !k12Was && !keysPressed[KEY_SHIFT]) {
    int32_t f = idx + dir * easeN;
    if (dir > 0 || f > 0) flipAt = f;
  }
  k12Was = k12;

  // ── simulate: fixed whole-number steps (that's what makes it exactly reversible) ──
  float simT = dt * speed;
  int32_t target = dir > 0 ? (flipAt >= 0 && flipAt < loopN ? flipAt : loopN) : flipAt;   // reverse: only a k12 flip
  float ease = 1.0f;
  if (target >= 0) {
    int32_t dist = dir > 0 ? target - idx : idx - target;
    if (dist < easeN) { float e = dist > 0 ? (float)dist / easeN : 0.0f; ease = 0.05f + 0.95f * e * e * (3.0f - 2.0f * e); }
  }
  {   // just after a turn, ease back up to speed
    static int32_t lastTurn = -1000000;
    int32_t since = abs(idx - lastTurn);
    if (since < easeN) { float e = (float)since / easeN; float u = 0.05f + 0.95f * e * e * (3.0f - 2.0f * e); if (u < ease) ease = u; }
    stepAcc += simT * CB_HZ * ease;
    int steps = (int)stepAcc;
    stepAcc -= steps;
    if (steps > 48) steps = 48;
    for (int s = 0; s < steps; s++) {
      if (dir < 0 && idx == 0) {
        // back at the crown: one more step lands exactly on the launch state (at rest)
        cb_step(); dir = 1; flipAt = -1;
        if (changed) {                                              // knob changes take effect at the seam
          cb_launch(count, a0, spreadDeg * (CB_PI / 180.0f), armK);
          lA0 = a0; lSpread = spreadDeg; lArm = armK;
        }
        continue;
      }
      if (target >= 0 && (dir > 0 ? idx >= target : idx <= target)) {   // turn time round
        cb_reverse(); dir = -dir; flipAt = -1; lastTurn = idx;
        target = dir > 0 ? loopN : -1;
        idx += dir;                                                // the swap itself is one step
        continue;
      }
      cb_step();
      idx += dir;
    }
  }
  float since = idx * (1.0f / CB_HZ);                            // loop time (runs backwards on the rewind)
  cb_measureDrift();

  // Shift + k12: floor reflection on/off (never in Top Bloom: we look straight down)
  {
    static bool skWas = false;
    bool sk = keysPressed[KEY_MOD_A] && keysPressed[KEY_SHIFT];
    if (sk && !skWas) { cb_reflOn = !cb_reflOn; memset(cb_buf, 0, W * H); cb_liveN = 0; cb_fresh = true; }
    skWas = sk;
  }
  bool refl = cb_reflOn && preset != 2;
  // the floor sits just below the lowest any bob can reach
  float lsMax = (preset == 3) ? 0.65f : (preset == 4 ? 0.6f : 1.0f);
  float floorY = -(cb_L1 + cb_L2) * lsMax - 0.12f;
  if (preset == 7) floorY = -1.1f - (cb_L1 + cb_L2) * 0.39f - 0.12f;

  // ── colours ──
  static const uint8_t order[CB_NSCHEME] = {0, 1, 2, 3, 4, 5, 6, 7};
  int sc = cb_k(0) * CB_NSCHEME / 1024;
  if (sc > CB_NSCHEME - 1) sc = CB_NSCHEME - 1;
  static float cycF = 0;
  float cyk = (cb_sm[13] - 512.0f) / 511.0f;                    // right half: colour cycle speed
  if (cyk > 0.05f) cycF += dt * cyk * cyk * 160.0f;
  int driftAmt = cb_k(13) < 490 ? (490 - cb_k(13)) * 256 / 490 : 0;   // left half: drift colour (0..256)
  if (preset == 6) driftAmt = 0;
  if (cycF > 1.0e6f) cycF = 0;
  int cyc = ((int)cycF) & 255;
  if (preset == 6) cyc = (cyc + (int)(since * 20.0f)) & 255;     // Solo: the trail changes colour along the loop
  cb_buildPalette(order[sc], cyc);
  if (cb_flash > 0) cb_flash--;

  // ── take away last frame's rods and bobs (only trails persist) ──
  cb_erase = true;
  for (int i = 0; i < cb_liveN; i++) cb_live(i);
  if (cb_liveN > 0) cb_liveExtras();
  cb_erase = false;

  // ── trails: fade (real-time rate); Light Paint keeps them long ──
  {
    static float fadeAcc = 0;
    float tk = 1.0f - trails / 1023.0f;
    if (preset == 5 || preset == 6) tk *= 0.35f;
    fadeAcc += dt * (6.0f + tk * tk * 420.0f);
    int dec = (int)fadeAcc;
    if (dec > 15) dec = 15;
    fadeAcc -= (int)fadeAcc;
    if (dec > 0) cb_fade(dec);
  }

  // ── camera ──
  static float yaw = 0;
  static int32_t idxWas = 0;
  float spk = (cb_sm[8] - 512.0f) / 512.0f;
  if (spk > -0.05f && spk < 0.05f) spk = 0;
  {
    // the built-in slow spin moves with the loop's own steps (so it slows into each turn), and
    // turns a whole number of times per loop, so a full loop lands exactly where it began
    int32_t dsteps = abs(idx - idxWas);
    idxWas = idx;
    if (dsteps > 64) dsteps = 0;                                  // relaunch
    float perStep = spinBase * (CB_PI / 180.0f) / CB_HZ;
    if (loopSec > 0 && spinBase > 0) {
      float turns = roundf(spinBase * 2.0f * loopSec / 360.0f);
      if (turns < 1) turns = 1;
      perStep = turns * CB_TWO_PI / (2.0f * loopN);
    }
    yaw += dsteps * perStep + dt * spk * fabsf(spk) * 90.0f * (CB_PI / 180.0f);
  }
  if (yaw > CB_TWO_PI) yaw -= CB_TWO_PI;
  if (yaw < 0) yaw += CB_TWO_PI;
  float pitch = (pitchBase + (cb_k(9) - 512) * 45.0f / 512.0f) * (CB_PI / 180.0f);
  if (pitch > 1.53f) pitch = 1.53f;
  if (pitch < -0.8f) pitch = -0.8f;
  cb_cy = cb_cos(yaw + yawBase); cb_sy = cb_sin(yaw + yawBase);
  cb_cp = cb_cos(pitch); cb_sp = cb_sin(pitch);
  float zk = (cb_k(10) - 512) / 512.0f;
  float zoom = zoomBase * powf(zk < 0 ? 2.85f : 4.0f, zk);                       // 0.35× … 4×
  float reach = cb_L1 + cb_L2;
  cb_scale = 57.0f * zoom * 2.0f / reach;
  if (refl) cb_scale *= 0.72f;                                  // step back a little so the floor fits
  cb_dist = 6.0f;
  // pan: move the view across the scene (in pendulum lengths, so it tracks zoom);
  // a small dead zone at the centre of each knob holds it still
  float pxk = (cb_k(12) - 512) / 512.0f, pyk = (cb_k(14) - 512) / 512.0f;
  if (pxk > -0.04f && pxk < 0.04f) pxk = 0;
  if (pyk > -0.04f && pyk < 0.04f) pyk = 0;
  cb_ox = HALFW - (int)(pxk * 1.0f * reach * cb_scale);
  cb_oy = HALFH - 4 + (int)(pyk * 0.9f * reach * cb_scale);     // pivot near the centre: bobs can swing above and below
  if (refl) cb_oy += (int)(0.60f * H) - (HALFH - 4) - (int)(-floorY * cb_scale * cb_cp);   // floor line at ~60 % down the screen
  if (cb_ox < -3000) cb_ox = -3000; else if (cb_ox > 3000) cb_ox = 3000;
  if (cb_oy < -3000) cb_oy = -3000; else if (cb_oy > 3000) cb_oy = 3000;

  // ── draw every pendulum ──
  for (int i = 0; i < cb_n; i++) {
    const float iq = 1.0f / CB_Q;
    // pivot and swing plane for this pendulum
    float pxw = 0, pyw = 0, pzw = 0, phi = 0, Ls = 1.0f;
    int hue = (cb_n > 1) ? (i * 15 / cb_n) % 15 : 0;
    switch (preset) {
      case 1: phi = 0; pzw = ((float)i / cb_n - 0.5f) * 0.02f; break;               // Fan: one plane
      case 3: {                                                                    // Carousel
        float a = CB_TWO_PI * i / cb_n;
        pxw = 1.3f * cb_cos(a); pzw = 1.3f * cb_sin(a); phi = a + CB_PI * 0.5f; Ls = 0.65f; break;
      }
      case 4: {                                                                    // Wave Bar
        pxw = ((float)i / (cb_n - 1 > 0 ? cb_n - 1 : 1) - 0.5f) * 4.2f; phi = CB_PI * 0.5f; Ls = 0.6f; break;
      }
      case 6: phi = 0; hue = 0; break;                                             // Solo
      case 7: {                                                                    // Chandelier: 3 rings
        int per = cb_n / 3, ring = i / per;
        if (ring > 2) ring = 2;
        int k = i - ring * per;
        phi = CB_TWO_PI * k / per + ring * 0.3f;
        pyw = -0.55f * ring; Ls = 0.75f - 0.18f * ring;
        hue = (k * 15 / per + ring * 5) % 15; break;
      }
      default: phi = CB_TWO_PI * i / cb_n; break;                                  // Crown, Top Bloom, Light Paint
    }
    // Drift colour: white while still in step with its neighbour, its own colour once it has drifted
    bool calm = false;
    if (driftAmt > 0) {
      int t = cb_drift[i] * 6; if (t > 256) t = 256;
      int teff = 256 - (driftAmt * (256 - t) >> 8);
      calm = teff < 110;
    }
    float cph = cb_cos(phi), sph = cb_sin(phi);
    float u1 = Ls * cb_x1[i] * iq, v1 = Ls * cb_y1[i] * iq;
    float u2 = Ls * cb_x2[i] * iq, v2 = Ls * cb_y2[i] * iq;
    int x0, y0, x1, y1, x2, y2;
    cb_proj(pxw, pyw, pzw, &x0, &y0);
    cb_proj(pxw + u1 * cph, pyw + v1, pzw + u1 * sph, &x1, &y1);
    int dq1 = cb_dq();
    cb_proj(pxw + u2 * cph, pyw + v2, pzw + u2 * sph, &x2, &y2);
    int dq2 = cb_dq();
    // trail: a crisp line from the outer bob's last position; its colour
    // runs ahead round the wheel the faster the bob is moving
    int16_t* hx = cb_hx[i];
    int16_t* hy = cb_hy[i];
    if (cb_fresh) for (int k = 0; k < CB_HIST; k++) { hx[k] = (int16_t)cb_c16(x2); hy[k] = (int16_t)cb_c16(y2); }
    else {
      int sp = abs(hx[0] - x2) + abs(hy[0] - y2);
      int th = calm ? CB_WHITE : (hue + (sp >> 2 > 6 ? 6 : sp >> 2)) % 15;
      int tl = (sp > 6 ? 13 : 12) * dq2 >> 8;
      // redo the last few segments too: last frame's bob covered them and was just erased
      for (int k = CB_HIST - 1; k > 0; k--)
        if (abs(hx[k] - hx[k - 1]) + abs(hy[k] - hy[k - 1]) < 160) cb_line(hx[k], hy[k], hx[k - 1], hy[k - 1], th, tl - 1);
      if (sp < 160) cb_line(hx[0], hy[0], x2, y2, th, tl);
      for (int k = CB_HIST - 1; k > 0; k--) { hx[k] = hx[k - 1]; hy[k] = hy[k - 1]; }
      hx[0] = (int16_t)cb_c16(x2); hy[0] = (int16_t)cb_c16(y2);
    }
    // live parts: remembered so they can be erased next frame
    int16_t* q = cb_lx[i];
    q[0] = (int16_t)cb_c16(x0); q[1] = (int16_t)cb_c16(y0); q[2] = (int16_t)cb_c16(x1);
    q[3] = (int16_t)cb_c16(y1); q[4] = (int16_t)cb_c16(x2); q[5] = (int16_t)cb_c16(y2);
    cb_lh[i] = (uint8_t)(calm ? CB_WHITE : hue);
    cb_lc[i] = (uint8_t)(calm ? CB_WHITE : (hue + 7) % 15);       // inner bob: the opposite colour
    // reflection: the same points mirrored in the floor (y → 2·floor − y)
    if (refl) {
      int rx0, ry0, rx1, ry1, rx2, ry2;
      float fy2 = 2.0f * floorY;
      cb_proj(pxw, fy2 - pyw, pzw, &rx0, &ry0);
      cb_proj(pxw + u1 * cph, fy2 - (pyw + v1), pzw + u1 * sph, &rx1, &ry1);
      cb_proj(pxw + u2 * cph, fy2 - (pyw + v2), pzw + u2 * sph, &rx2, &ry2);
      int wob = ((ry2 + (int)(since * 24.0f)) % 8) < 4 ? 0 : 1;     // a little shimmer, like a wet floor
      rx2 += wob;
      int16_t* r = cb_lr[i];
      r[0] = (int16_t)cb_c16(rx0); r[1] = (int16_t)cb_c16(ry0); r[2] = (int16_t)cb_c16(rx1);
      r[3] = (int16_t)cb_c16(ry1); r[4] = (int16_t)cb_c16(rx2); r[5] = (int16_t)cb_c16(ry2);
      int rh = calm ? CB_WHITE : hue;
      if (!cb_fresh && abs(cb_rpx[i] - rx2) + abs(cb_rpy[i] - ry2) < 160)
        cb_line(cb_rpx[i], cb_rpy[i], rx2, ry2, rh, 6);           // reflected trail (dim, persists)
      cb_put(rx2, ry2, rh, 8); cb_put(rx1, ry1, calm ? CB_WHITE : (hue + 7) % 15, 6);
      cb_rpx[i] = r[4]; cb_rpy[i] = r[5];
    }
    int r1 = (rods * 3 / 2 + 1) * dq1 >> 8, r2 = (rods * 3 / 2 + 2) * dq2 >> 8;
    cb_ll[i][0] = (uint8_t)(r1 > 11 ? 11 : r1);
    cb_ll[i][1] = (uint8_t)(r2 > 12 ? 12 : r2);
    cb_ll[i][2] = (uint8_t)cb_min(12 * dq1 >> 8, 15);
    cb_ll[i][3] = (uint8_t)cb_min(13 * dq2 >> 8, 15);
  }
  cb_liveN = cb_n;
  cb_lBob = bobR; cb_lGlow = glow; cb_lPreset = preset; cb_lRods = rods > 0 && preset != 5; cb_lRefl = refl;
  if (preset == 4) {                                                 // the bar
    int bx0, by0, bx1, by1;
    cb_proj(-2.25f, 0, 0, &bx0, &by0); cb_proj(2.25f, 0, 0, &bx1, &by1);
    cb_lExtra[0] = (int16_t)cb_c16(bx0); cb_lExtra[1] = (int16_t)cb_c16(by0);
    cb_lExtra[2] = (int16_t)cb_c16(bx1); cb_lExtra[3] = (int16_t)cb_c16(by1);
  } else { cb_lExtra[0] = (int16_t)cb_ox; cb_lExtra[1] = (int16_t)cb_oy; }
  // draw the live layer on top of the trails
  for (int i = 0; i < cb_liveN; i++) cb_live(i);
  cb_liveExtras();
  cb_fresh = false;
}
