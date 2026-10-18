// =====================================================================
// PROGRAM: HAIR — strands in the wind (by Dewey) — Set 5, slot 1
//
// Long strands held at one end and left to swing: a curtain of coloured
// threads, kelp swaying under water, a wheat field, a jellyfish trailing
// tentacles, a comet's tail, a fibre-optic lamp. Wind is a slowly moving
// "noise" field, so gusts roll across the strands the way they do across a
// real field. Same rope engine as CHIPLEADER (vg5s.h).
//
// Presets:
//   k0 Curtain     — a curtain of colour-bar threads in a breeze
//   k1 Kelp        — held at the sea floor, floating upwards, rocked by currents
//   k2 Wheat       — short stiff stalks, waves of wind rolling over them
//   k3 Spinner     — threads on a spinning ring, flung outwards
//   k4 Jellyfish   — a pulsing bell trailing long tentacles
//   k5 Willow      — long green strands hanging from an arching branch
//   k6 Comet       — every strand from one point flying a figure-of-eight, tails streaming
//   k7 Bead Curtain — a ball keeps walking through a curtain of beads
//   k8 Fibre Optic — a fountain of fibres from one point, glowing tips, colours cycling
//   k9 Static      — frizz standing out from a dome, crackling
//   k10 Swing      — the curtain rail swings from side to side
//   k11 Tour       — a new preset every 20 seconds
//
//   k12 — GUST: a strong blast of wind
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = freeze)   p2 Gravity (left = float up)
//   p3 Stiffness   p4 Wind   p5 Gust Size (small flutters … big slow waves)
//   p6 Length   p7 Strands (how many)   p8 Anchor Motion   p9 Ball (centre = preset; size of a ball pushing through)
//   p10 Ball Speed   p11 Glowing Tips
//   p12-p15 are Set 5's live FX knobs (vg5fx.h), off in the centre band:
//   p12 Warp (◄ ripple | zoom ►)   p13 Mirror (◄ mirror | tiles ►)   p14 Colour (◄ cycle | flash ►)
//   p15 Glitch (◄ tear | pixels ►)
// =====================================================================

#include "vg5s.h"

enum { HR5_RAIL, HR5_FLOOR, HR5_RING, HR5_POINT, HR5_ARCH, HR5_DOME, HR5_NSHAPE };
struct Hr5Preset {
  uint8_t cols[8][3]; uint8_t nc;
  uint8_t shape, strands, per, segPx;                // anchor shape, strands, points per strand, segment length (px)
  int8_t grav; uint8_t wind, gust, iters, motion, ball, tips, spring;   // spring: pull back towards the rest shape
};
static const Hr5Preset hr5_pre[11] = {
  {{{220,30,30},{110,170,215},{215,175,60},{70,190,70},{60,30,200},{200,60,200},{230,170,50},{235,235,220}}, 8, HR5_RAIL, 64, 22, 8, 12, 90, 120, 2, 0, 0, 0, 0},
  {{{20,120,60},{40,170,90},{120,200,60},{20,90,90},{60,200,160},{160,220,90}}, 6, HR5_FLOOR, 44, 30, 6, -10, 110, 200, 2, 0, 0, 0, 2},
  {{{230,180,60},{250,210,100},{200,140,40},{255,230,150},{180,120,30}}, 5, HR5_FLOOR, 110, 10, 7, 4, 120, 220, 3, 0, 0, 1, 26},
  {{{255,0,140},{0,220,255},{255,220,0},{140,255,60},{180,90,255}}, 5, HR5_RING, 48, 24, 4, 4, 40, 120, 2, 255, 0, 1, 6},
  {{{255,120,220},{150,120,255},{110,220,255},{255,190,240}}, 4, HR5_RING, 40, 32, 5, 10, 50, 140, 2, 200, 0, 1, 0},
  {{{40,110,30},{90,160,40},{150,200,70},{60,140,60},{200,220,120}}, 5, HR5_ARCH, 54, 26, 7, 12, 70, 160, 2, 0, 0, 0, 0},
  {{{255,255,255},{120,200,255},{255,160,60},{255,80,160}}, 4, HR5_POINT, 40, 34, 5, 0, 30, 100, 2, 255, 0, 1, 0},
  {{{255,200,60},{255,110,40},{200,40,80},{120,40,140},{255,230,180}}, 5, HR5_RAIL, 46, 30, 6, 12, 20, 100, 2, 0, 26, 2, 0},
  {{{255,0,80},{255,140,0},{255,240,0},{0,255,120},{0,160,255},{170,0,255}}, 6, HR5_POINT, 60, 24, 6, 6, 40, 80, 3, 60, 0, 2, 14},
  {{{180,200,255},{255,255,255},{140,160,255},{220,180,255}}, 4, HR5_DOME, 70, 18, 5, 2, 60, 40, 1, 40, 0, 1, 10},
  {{{255,60,60},{255,200,60},{60,200,255},{255,255,255},{120,255,120},{255,120,220}}, 6, HR5_RAIL, 60, 22, 8, 12, 40, 120, 2, 255, 0, 0, 0},
};
static const char* const hr5_names[12] = {
  "Curtain", "Kelp", "Wheat", "Spinner", "Jellyfish", "Willow", "Comet", "Bead Curtain", "Fibre Optic", "Static", "Swing", "Tour"
};
static const char* const hr5_labels[16] = {
  "Palette", "Speed", "Gravity", "Stiffness", "Wind", "Gust Size", "Length", "Strands",
  "Anchor Motion", "Ball", "Ball Speed", "Glowing Tips", "Line Style", "Trails", "Colour Flow", "Shape"
};
const char* prog_hair_name() { return "HAIR"; }
const char* prog_hair_character() { return "Strands held at one end swinging in rolling gusts: curtains, kelp, wheat, jellyfish, comets"; }
const char* prog_hair_presetName(int preset) { return (preset >= 0 && preset < 12) ? hr5_names[preset] : NULL; }
const char* prog_hair_potLabel(int preset, int pot) { (void)preset; if (pot >= 12 && pot < 16) return fx5_labels[pot - 12]; return (pot >= 0 && pot < 16) ? hr5_labels[pot] : ""; }
uint8_t prog_hair_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

static int hr5_preset = -1, hr5_look = 0, hr5_shape = -1, hr5_ns = 0, hr5_per = 0, hr5_seg = 0;
static float hr5_t = 0, hr5_tour = 0, hr5_flow = 0, hr5_gust = 0, hr5_ballA = 0;
static bool hr5_k12 = false;

// where strand s is held, and which way it hangs (dx, dy, 1/16 px per segment)
static void hr5_anchor(int s, int shape, float mot, int* ax, int* ay, int* dx, int* dy) {
  float u = hr5_ns > 1 ? (float)s / (hr5_ns - 1) : 0.5f, t = hr5_t;
  float x = 0, y = 0, hx = 0, hy = 1;
  switch (shape) {
    case HR5_RAIL: x = 8 + u * (W - 16) + mot * 40 * sinf(t * 0.9f); y = 4; break;
    case HR5_FLOOR: x = 6 + u * (W - 12) + mot * 10 * sinf(t * 0.7f + u * 3); y = H - 3; hy = -1; break;
    case HR5_RING: case HR5_DOME: {
      bool dome = shape == HR5_DOME;
      float a = dome ? 3.14159f + u * 3.14159f : u * 6.2832f + t * mot * 2.2f;
      float r = dome ? 46 : 26 + (hr5_look == 4 ? 6 * sinf(t * 2.5f) : 0);
      float cx = 160 + (dome ? 0 : mot * 50 * sinf(t * 0.5f)), cy = dome ? 150 : 70 + (hr5_look == 4 ? 14 * sinf(t * 2.5f - 1) : 30 * mot * sinf(t * 0.37f));
      x = cx + cosf(a) * r; y = cy + sinf(a) * r * (dome ? 1.0f : 0.45f);
      hx = cosf(a); hy = dome ? sinf(a) : 0.6f + 0.4f * sinf(a);
      if (hr5_look == 4 && !dome) { hx *= 0.25f; hy = 1; }                   // jellyfish tentacles hang down
      break;
    }
    case HR5_POINT: {
      float cx = 160 + mot * 110 * sinf(t * 0.8f), cy = (hr5_look == 8 ? 200 : 110) + mot * 60 * sinf(t * 1.6f);
      x = cx; y = cy;
      float a = (hr5_look == 8 ? -1.5708f : 1.5708f) + (u - 0.5f) * (hr5_look == 8 ? 2.4f : 1.0f);
      hx = cosf(a); hy = sinf(a); break;
    }
    case HR5_ARCH: x = 10 + u * (W - 20); y = 10 + 40 * (2 * u - 1) * (2 * u - 1) + mot * 6 * sinf(t + u * 6); break;
  }
  *ax = (int)(x * 16); *ay = (int)(y * 16); *dx = (int)(hx * 16); *dy = (int)(hy * 16);
}
static void hr5_layout(int ns, int per, int seg, int shape) {
  hr5_ns = ns; hr5_per = per; hr5_seg = seg; hr5_shape = shape;
  s5_setup(ns, per, seg * 16);
  hr5_ns = s5_ns;
  for (int s = 0; s < s5_ns; s++) {
    int ax, ay, dx, dy; hr5_anchor(s, shape, 0, &ax, &ay, &dx, &dy);
    for (int i = 0; i < per; i++) s5_place(s * per + i, ax + dx * seg * i, ay + dy * seg * i);
    s5_setPin(s * per, true);
  }
}
void prog_hair_init() { v3_reset(); hr5_preset = -1; hr5_ns = 0; hr5_t = 0; hr5_flow = 0; }

static void hair5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame();
  fx5_take();                                                      // p12-p15: the live FX knobs
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != hr5_preset) { hr5_preset = preset; hr5_look = preset == 11 ? 0 : preset; hr5_tour = 0; hr5_ns = 0; }
  if (preset == 11) { hr5_tour += v3_dt; if (hr5_tour > 20.0f) { hr5_tour = 0; hr5_look = (hr5_look + 1) % 11; hr5_ns = 0; } }
  const Hr5Preset& P = hr5_pre[hr5_look];
  const Hr5Preset& Q = hr5_pre[v3_centre(0) ? hr5_look : v3_clamp(v3_k(0) * 11 / 1024, 0, 10)];

  float speed = v3_speed(1, 3.0f);
  int grav = v3_centre(2) ? P.grav : (int)(v3_bi(2) * 24);
  int iters = v3_centre(3) ? P.iters : 1 + (int)(v3_u(3) * 4.99f);
  float wind = v3_centre(4) ? P.wind / 255.0f : v3_u(4) * 1.4f;
  float gustSize = v3_centre(5) ? P.gust / 255.0f : v3_u(5);
  int per = v3_centre(6) ? P.per : 4 + (int)(v3_u(6) * 34);
  int ns = v3_centre(7) ? P.strands : 8 + (int)(v3_u(7) * 110);
  float mot = v3_centre(8) ? P.motion / 255.0f : v3_u(8);
  int ball = v3_centre(9) ? P.ball : (int)(v3_u(9) * 50);
  float ballSp = v3_rel(10, 1.0f, 3.0f);
  int tips = v3_centre(11) ? P.tips : v3_clamp(v3_k(11) * 3 / 1024, 0, 2);
  int lineStyle = v3_pick(12, 0, 3);
  int trails = v3_centre(13) ? (hr5_look == 6 ? 9 : 0) : (int)(v3_u(13) * 14);
  hr5_flow += v3_dt * (v3_centre(14) ? (hr5_look == 8 ? 0.4f : 0.0f) : v3_bi(14) * 1.5f);
  int spring = P.spring;
  int shape = v3_centre(15) ? P.shape : v3_clamp(v3_k(15) * HR5_NSHAPE / 1024, 0, HR5_NSHAPE - 1);
  if (per * ns > S5_MAXP) ns = S5_MAXP / per;
  if (ns != hr5_ns || per != hr5_per || shape != hr5_shape || P.segPx != hr5_seg) hr5_layout(ns, per, P.segPx, shape);

  // colours: 8 hues; strands take their colour by position, levels brighten towards the tip
  uint8_t cols[8][3];
  for (int h = 0; h < 8; h++) {
    float f = h + hr5_flow; f -= floorf(f / Q.nc) * Q.nc;
    int a = (int)f % Q.nc, b = (a + 1) % Q.nc, w = (int)((f - floorf(f)) * 256);
    for (int c = 0; c < 3; c++) cols[h][c] = v3_mix8(Q.cols[a][c], Q.cols[b][c], w);
  }
  v3_palLH(cols, 8, 1.3f, 16);
  for (int l = 0; l < 16; l++) display.setColor((uint8_t)((l << 4) | 8), l * 4, l * 4, l * 5 + 10);   // hue 8: the ball, soft grey-blue

  float dt = v3_dt * speed;
  hr5_t += dt;
  bool k12 = keysPressed[KEY_MOD_A]; if (k12 && !hr5_k12) hr5_gust = 1.0f; hr5_k12 = k12;
  hr5_gust -= dt * 0.6f; if (hr5_gust < 0) hr5_gust = 0;

  // anchors follow their shape (and its motion)
  for (int s = 0; s < s5_ns; s++) {
    int ax, ay, dx, dy; hr5_anchor(s, shape, mot, &ax, &ay, &dx, &dy);
    s5_place(s * s5_per, ax, ay);
    if (shape == HR5_RING || shape == HR5_POINT || shape == HR5_DOME) {   // the second point is held too: strands leave in their direction
      s5_place(s * s5_per + 1, ax + dx * hr5_seg, ay + dy * hr5_seg); s5_setPin(s * s5_per + 1, true);
    }
  }
  // the ball walks through
  hr5_ballA += dt * 0.35f * ballSp;
  int bx = (int)((160 + 150 * sinf(hr5_ballA)) * 16), by = (int)((shape == HR5_FLOOR ? 175 : 120 + 40 * sinf(hr5_ballA * 1.7f)) * 16), br = ball * 16;

  if (speed > 0.0f) {
    s5_move(0, (grav * 16) / 64, 248);
    // wind: a noise field rolling across the screen
    float wk = wind * (1.0f + 3.0f * hr5_gust);
    if (wk > 0.01f) {
      uint32_t t = (uint32_t)(hr5_t * 120);
      int sc = 1 + (int)(gustSize * 4);                              // bigger gusts = noise read at a coarser scale
      int kx = (int)(wk * 6), ky = (int)(wk * 3);
      for (int k = 0; k < s5_n; k++) {
        if (s5_pinned(k)) continue;
        int x = s5_x[k] >> 4, y = s5_y[k] >> 4;
        int32_t n = s5_wind(x / sc, y / sc, t, 3);
        int32_t m = v3_noise1((uint32_t)(x * 2 / sc) + t * 2, 77);
        s5_x[k] += (int16_t)(((n + 2500) * kx) / 16384);            // the wind leans a little left → right
        s5_y[k] += (int16_t)((m * ky) / 16384);
      }
    }
    if (hr5_look == 9 && v3_centre(15)) {                             // static: every free point is pushed out from the dome
      for (int k = 0; k < s5_n; k++) if (!s5_pinned(k)) { int ex = s5_x[k] - 160 * 16, ey = s5_y[k] - 150 * 16; s5_x[k] += (int16_t)(ex / 90); s5_y[k] += (int16_t)(ey / 90); }
    }
    if (br > 0) {                                                    // the ball pushes points out of its way
      for (int k = 0; k < s5_n; k++) {
        int ex = s5_x[k] - bx, ey = s5_y[k] - by;
        if (ex > br || ex < -br || ey > br || ey < -br) continue;
        int d2 = ex * ex + ey * ey; if (d2 >= br * br || s5_pinned(k)) continue;
        int d = (int)v3_isqrt((uint32_t)d2) + 1;
        s5_x[k] = (int16_t)(bx + ex * br / d); s5_y[k] = (int16_t)(by + ey * br / d);
      }
    }
    if (spring > 0) {                                                // stalks and fibres spring back towards their rest shape
      for (int s = 0; s < s5_ns; s++) {
        int ax, ay, dx, dy; hr5_anchor(s, shape, mot, &ax, &ay, &dx, &dy);
        for (int i = 2; i < s5_per; i++) {
          int k = s * s5_per + i;
          int tx = ax + dx * hr5_seg * i, ty = ay + dy * hr5_seg * i;
          s5_x[k] += (int16_t)((tx - s5_x[k]) * spring / 256); s5_y[k] += (int16_t)((ty - s5_y[k]) * spring / 256);
        }
      }
    }
    s5_ropes(iters);
    s5_walls();
  }

  // ── draw ──
  if (trails > 0) v3_fadeLev(16 - trails); else s5_clear(0);
  if (br > 0) v3_disc(bx >> 4, by >> 4, ball, (uint8_t)((6 << 4) | 8));
  for (int s = 0; s < s5_ns; s++) {
    int hue = (s * 8 / (s5_ns > 0 ? s5_ns : 1)) & 7;
    if (shape == HR5_RING || shape == HR5_POINT || shape == HR5_DOME) hue = s % Q.nc % 8;
    int a = s * s5_per;
    if (lineStyle == 2) s5_drawStrandAA(s, 14, hue);
    else {
      int px = s5_x[a] >> 4, py = s5_y[a] >> 4;
      for (int i = 1; i < s5_per; i++) {
        int x = s5_x[a + i] >> 4, y = s5_y[a + i] >> 4;
        int lev = 6 + (i * 9) / s5_per;
        if (lineStyle == 1) v3_line(px + 1, py, x + 1, y, (uint8_t)(((lev - 4) << 4) | hue));
        v3_line(px, py, x, y, (uint8_t)((lev << 4) | hue));
        if (tips == 2 && hr5_look == 7 && (i & 3) == 0) v3_disc(x, y, 1, (uint8_t)((15 << 4) | hue));   // beads
        px = x; py = y;
      }
    }
    if (tips == 1) v3_put(s5_x[a + s5_per - 1] >> 4, s5_y[a + s5_per - 1] >> 4, (uint8_t)((15 << 4) | hue));
    else if (tips == 2 && hr5_look != 7) v3_disc(s5_x[a + s5_per - 1] >> 4, s5_y[a + s5_per - 1] >> 4, 1, (uint8_t)((15 << 4) | hue));
  }
  if (hr5_look == 4 && v3_centre(15)) {                               // the jellyfish's bell
    int ax, ay, dx, dy; hr5_anchor(0, HR5_RING, mot, &ax, &ay, &dx, &dy);
    float puls = 1.0f + 0.15f * sinf(hr5_t * 2.5f);
    int cx = (int)(160 + mot * 50 * sinf(hr5_t * 0.5f)), cy = (int)(70 + 14 * sinf(hr5_t * 2.5f - 1));
    for (int r = (int)(34 * puls); r > 0; r -= 2) {
      int lev = 3 + (34 - r) / 4; if (lev > 12) lev = 12;
      for (int a = 512; a < 1024; a += 4) v3_put(cx + ((v3_cos(a) * r) >> 14), cy + 2 + ((v3_sin(a) * r * 7 / 10) >> 14), (uint8_t)((lev << 4) | 0));
    }
  }
}
void prog_hair_draw(int preset) { hair5_draw(preset); fx5_apply(); }
