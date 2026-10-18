// =====================================================================
// PROGRAM: SPAGHETTI — strands poured, tangled and stirred (by Dewey) — Set 5, slot 2
//
// Loose strands pour from a spout into a container — a bowl, a box, a
// funnel, a tray — and tangle into a colourful heap like the end of
// CHIPLEADER. A whirlpool stirs the heap round, then the floor drops away,
// everything drains out of the bottom of the screen, and the pour begins
// again. Same rope engine as CHIPLEADER (vg5s.h).
//
// Presets:
//   k0 Tangle       — test-card colours poured into a box
//   k1 Spaghetti    — pasta and tomato in a bowl
//   k2 Yarn Basket  — soft wool colours, stirred hard
//   k3 Neon Funnel  — neon strands sliding into a V
//   k4 Rainbow Pour — rainbow strands heaping on a flat floor
//   k5 Cable Tray   — black, grey and red cables, stiff, in a tray
//   k6 Worms        — pink strands that wriggle on their own
//   k7 Two Bowls    — two bowls side by side, strands spilling between
//   k8 Ball of Yarn — no gravity: strands wind themselves into a turning ball
//   k9 Whirlpool    — blue strands spun by a strong whirlpool
//   k10 Streamers   — short paper streamers fluttering down for ever
//   k11 Tour        — a new preset every time the container drains
//
//   k12 — DRAIN NOW (or, once drained, POUR NOW)
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = preset)   p1 Speed (far left = freeze)   p2 Gravity   p3 Stiffness
//   p4 Stir (the whirlpool; left = the other way)   p5 Crowding   p6 Pour Rate   p7 Strand Length
//   p8 Strands (how many)   p9 Container (centre = preset: box, bowl, funnel, flat, two bowls)
//   p10 Hold (how long before draining)   p11 Wiggle
//   p12-p15 are Set 5's live FX knobs (vg5fx.h), off in the centre band:
//   p12 Warp (◄ ripple | zoom ►)   p13 Mirror (◄ mirror | tiles ►)   p14 Colour (◄ cycle | flash ►)
//   p15 Glitch (◄ tear | pixels ►)
// =====================================================================

#include "vg5s.h"

enum { SG5_BOX, SG5_BOWL, SG5_FUNNEL, SG5_FLAT, SG5_TWO, SG5_NCONT };
struct Sg5Preset {
  uint8_t cols[8][3]; uint8_t nc;
  uint8_t cont, strands, per, segPx;
  int8_t grav, stir; uint8_t pour, iters, wiggle, bounce;
};
static const Sg5Preset sg5_pre[11] = {
  {{{220,30,30},{110,170,215},{215,175,60},{70,190,70},{60,30,200},{200,60,200},{230,170,50},{235,235,220}}, 8, SG5_BOX, 78, 18, 4, 10, 0, 40, 2, 0, 20},
  {{{250,220,130},{240,200,100},{255,235,170},{230,190,90},{200,30,20},{250,215,120}}, 6, SG5_BOWL, 70, 22, 4, 10, 12, 30, 2, 0, 10},
  {{{230,120,150},{120,170,220},{240,220,170},{150,200,140},{200,160,230},{240,170,110}}, 6, SG5_BOWL, 64, 24, 4, 10, 45, 30, 2, 0, 20},
  {{{255,0,200},{0,255,240},{170,255,0},{255,130,0},{120,60,255},{255,255,90}}, 6, SG5_FUNNEL, 74, 18, 4, 10, 0, 50, 2, 0, 30},
  {{{255,0,40},{255,140,0},{250,240,0},{0,220,60},{0,170,255},{90,40,255},{220,30,255}}, 7, SG5_FLAT, 80, 18, 4, 8, 0, 25, 2, 0, 20},
  {{{30,30,34},{90,90,96},{200,30,30},{30,60,190},{220,200,40},{50,50,56}}, 6, SG5_BOX, 46, 30, 5, 12, 0, 30, 4, 0, 0},
  {{{255,140,170},{240,110,140},{255,180,190},{220,90,120}}, 4, SG5_FLAT, 50, 22, 4, 6, 0, 40, 2, 150, 10},
  {{{255,90,60},{255,200,60},{60,200,255},{120,255,140},{230,120,255}}, 5, SG5_TWO, 74, 18, 4, 10, 10, 40, 2, 0, 20},
  {{{230,90,110},{240,200,120},{120,190,220},{190,140,230},{150,220,160}}, 5, SG5_FLAT, 60, 24, 4, 0, 30, 40, 2, 0, 0},
  {{{20,90,200},{40,160,240},{150,220,255},{0,60,140},{230,250,255}}, 5, SG5_BOWL, 70, 20, 4, 10, 90, 40, 2, 0, 20},
  {{{255,90,90},{255,220,80},{90,200,255},{140,255,140},{255,140,230},{255,255,255},{180,120,255}}, 7, SG5_FLAT, 120, 10, 4, 3, 0, 60, 1, 60, 0},
};
static const char* const sg5_names[12] = {
  "Tangle", "Spaghetti", "Yarn Basket", "Neon Funnel", "Rainbow Pour", "Cable Tray", "Worms", "Two Bowls", "Ball of Yarn", "Whirlpool", "Streamers", "Tour"
};
static const char* const sg5_labels[16] = {
  "Palette", "Speed", "Gravity", "Stiffness", "Stir", "Crowding", "Pour Rate", "Strand Length",
  "Strands", "Container", "Hold", "Wiggle", "Line Style", "Trails", "Colour Flow", "Bounce"
};
const char* prog_spag_name() { return "SPAGHETTI"; }
const char* prog_spag_character() { return "Loose strands poured into bowls and boxes, tangling into heaps, stirred by a whirlpool, then drained"; }
const char* prog_spag_presetName(int preset) { return (preset >= 0 && preset < 12) ? sg5_names[preset] : NULL; }
const char* prog_spag_potLabel(int preset, int pot) { (void)preset; if (pot >= 12 && pot < 16) return fx5_labels[pot - 12]; return (pot >= 0 && pot < 16) ? sg5_labels[pot] : ""; }
uint8_t prog_spag_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

enum { SG5_POUR, SG5_STIR, SG5_DRAIN };
static int sg5_preset = -1, sg5_look = 0, sg5_ns = 0, sg5_per = 0, sg5_seg = 0, sg5_phase = SG5_POUR, sg5_out = 0;
static float sg5_t = 0, sg5_flow = 0, sg5_poured = 0, sg5_spin = 0;
static bool sg5_k12 = false;
static uint8_t sg5_live[(S5_MAXP + 7) / 8];           // which strands are in play (1 bit each)
static inline bool sg5_isLive(int s) { return (sg5_live[s >> 3] >> (s & 7)) & 1; }
static inline void sg5_setLive(int s, bool on) { if (on) sg5_live[s >> 3] |= (uint8_t)(1 << (s & 7)); else sg5_live[s >> 3] &= (uint8_t)~(1 << (s & 7)); }

// the container's floor height (px) under x; a big number = no floor there
static int sg5_floor(int x, int cont) {
  int dx = x - 160;
  switch (cont) {
    case SG5_BOWL: { if (dx < -140 || dx > 140) return 1000; return H - 6 - dx * dx / 160; }
    case SG5_FUNNEL: { int a = dx < 0 ? -dx : dx; return a > 150 ? 1000 : H - 6 - a * 9 / 10; }
    case SG5_TWO: {                                   // two bowls; a ridge between them
      int c = dx < 0 ? dx + 80 : dx - 80;
      if (c < -78 || c > 78) return (dx > -6 && dx < 6) ? 146 : 1000;
      return H - 6 - c * c / 70;
    }
    default: return H - 6;
  }
}
static void sg5_hide(int s) {                          // park a strand above the screen, out of play
  for (int i = 0; i < s5_per; i++) { int k = s * s5_per + i; s5_place(k, (20 + s) * 16, -60 * 16); s5_setPin(k, true); }
  sg5_setLive(s, false);
}
static void sg5_drop(int s, float wig) {               // pour one strand from the spout
  int x0 = (160 + v3_rndr(-24, 24)) * 16, y0 = -4 * 16;
  if (sg5_look == 10 && v3_centre(9)) x0 = v3_rndr(10, W - 10) * 16;     // streamers fall everywhere
  int ang = v3_rndr(-120, 120);                                         // laid at a slant, upwards from the spout
  int dx = (v3_sin(ang) * sg5_seg * 16) >> 14, dy = -((v3_cos(ang) * sg5_seg * 16) >> 14);
  for (int i = 0; i < s5_per; i++) {
    int k = s * s5_per + i;
    s5_place(k, x0 + dx * i, y0 + dy * i); s5_setPin(k, false);
    s5_kick(k, v3_rndr(-8, 8), 30 + v3_rndr(0, 10));
  }
  sg5_setLive(s, true); (void)wig;
}
static void sg5_layout(int ns, int per, int seg) {
  sg5_ns = ns; sg5_per = per; sg5_seg = seg;
  s5_setup(ns, per, seg * 16);
  sg5_ns = s5_ns;
  for (int s = 0; s < s5_ns; s++) sg5_hide(s);
  sg5_phase = SG5_POUR; sg5_t = 0; sg5_poured = 0;
}
void prog_spag_init() { v3_reset(); sg5_preset = -1; sg5_ns = 0; sg5_flow = 0; }

static void spag5_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  v3_frame();
  fx5_take();                                                      // p12-p15: the live FX knobs
  if (preset < 0 || preset > 11) preset = 0;
  if (preset != sg5_preset) { sg5_preset = preset; sg5_look = preset == 11 ? 0 : preset; sg5_ns = 0; }
  const Sg5Preset& P = sg5_pre[sg5_look];
  const Sg5Preset& Q = sg5_pre[v3_centre(0) ? sg5_look : v3_clamp(v3_k(0) * 11 / 1024, 0, 10)];

  float speed = v3_speed(1, 3.0f);
  int grav = v3_centre(2) ? P.grav : (int)(v3_bi(2) * 24);
  int iters = v3_centre(3) ? P.iters : 1 + (int)(v3_u(3) * 4.99f);
  int stir = v3_centre(4) ? P.stir : (int)(v3_bi(4) * 100);
  int push = v3_centre(5) ? 9 : 3 + (int)(v3_u(5) * 15);
  float pour = v3_rel(6, P.pour / 10.0f, 4.0f);                     // strands per second
  int per = v3_centre(7) ? P.per : 6 + (int)(v3_u(7) * 30);
  int ns = v3_centre(8) ? P.strands : 10 + (int)(v3_u(8) * 140);
  int cont = v3_centre(9) ? P.cont : v3_clamp(v3_k(9) * SG5_NCONT / 1024, 0, SG5_NCONT - 1);
  float hold = v3_rel(10, 6.0f, 3.0f);
  int wig = v3_centre(11) ? P.wiggle : (int)(v3_u(11) * 255);
  int lineStyle = v3_pick(12, 1, 3);
  int trails = v3_centre(13) ? 0 : (int)(v3_u(13) * 14);
  sg5_flow += v3_dt * (v3_centre(14) ? 0.0f : v3_bi(14) * 1.5f);
  s5_bounce = v3_centre(15) ? P.bounce : (int)(v3_u(15) * 230);
  if (per * ns > S5_MAXP) ns = S5_MAXP / per;
  if (ns != sg5_ns || per != sg5_per || P.segPx != sg5_seg) sg5_layout(ns, per, P.segPx);
  bool forever = sg5_look == 10 && v3_centre(9);                     // streamers: a pour that never ends

  uint8_t cols[8][3];
  for (int h = 0; h < 8; h++) {
    float f = h + sg5_flow; f -= floorf(f / Q.nc) * Q.nc;
    int a = (int)f % Q.nc, b = (a + 1) % Q.nc, w = (int)((f - floorf(f)) * 256);
    for (int c = 0; c < 3; c++) cols[h][c] = v3_mix8(Q.cols[a][c], Q.cols[b][c], w);
  }
  v3_palLH(cols, 8, 1.4f, 20);
  for (int l = 0; l < 16; l++) display.setColor((uint8_t)((l << 4) | 15), l * 6, l * 6, l * 7);   // hue 15: the container, grey

  float dt = v3_dt * speed;
  sg5_t += dt;
  bool k12 = keysPressed[KEY_MOD_A], hit = k12 && !sg5_k12; sg5_k12 = k12;

  // ── pour / stir / drain ──
  if (sg5_phase == SG5_POUR || forever) {
    sg5_poured += pour * dt;
    while (sg5_poured >= 1.0f) {
      sg5_poured -= 1.0f;
      int s = -1;
      for (int k = 0; k < s5_ns; k++) if (!sg5_isLive(k)) { s = k; break; }
      if (s < 0) { if (!forever) { sg5_phase = SG5_STIR; sg5_t = 0; } break; }
      sg5_drop(s, wig);
    }
    if (hit && !forever) { sg5_phase = SG5_DRAIN; sg5_t = 0; hit = false; }
  }
  if (sg5_phase == SG5_STIR && (sg5_t > hold || hit)) { sg5_phase = SG5_DRAIN; sg5_t = 0; hit = false; }
  if (sg5_phase == SG5_DRAIN) {
    sg5_out = 0;
    for (int s = 0; s < s5_ns; s++) if (sg5_isLive(s)) { if (s5_y[s * s5_per] > (H + 30) * 16 && s5_y[s * s5_per + s5_per - 1] > (H + 30) * 16) sg5_hide(s); else sg5_out++; }
    if (sg5_out == 0 || sg5_t > 8.0f || hit) {
      for (int s = 0; s < s5_ns; s++) sg5_hide(s);
      sg5_phase = SG5_POUR; sg5_t = 0;
      if (sg5_preset == 11) { sg5_look = (sg5_look + 1) % 11; sg5_ns = 0; }
    }
  }
  if (forever) for (int s = 0; s < s5_ns; s++) if (sg5_isLive(s) && s5_y[s * s5_per] > (H + 20) * 16 && s5_y[s * s5_per + s5_per - 1] > (H + 20) * 16) sg5_hide(s);

  // ── physics ──
  if (speed > 0.0f) {
    bool zeroG = sg5_look == 8 && v3_centre(2);
    s5_move(0, (grav * 16) / 64, zeroG ? 244 : 250);
    sg5_spin += dt * stir * 0.02f;
    int cx = 160 * 16, cy = (zeroG ? 120 : (cont == SG5_FLAT || cont == SG5_BOX ? 200 : 205)) * 16;
    for (int s = 0; s < s5_ns; s++) {
      if (!sg5_isLive(s)) continue;
      int a = s * s5_per;
      if (wig > 0) {                                                 // worms: a travelling wave pushes each body sideways
        int ph = (int)(sg5_t * 600) + s * 97;
        for (int i = 0; i < s5_per; i++) {
          int dx = s5_x[a + s5_per - 1] - s5_x[a], dy = s5_y[a + s5_per - 1] - s5_y[a];
          int w = (v3_sin(ph + i * 90) * wig) >> 16;
          int len = v3_abs(dx) + v3_abs(dy) + 1;
          s5_x[a + i] += (int16_t)(-dy * w / len); s5_y[a + i] += (int16_t)(dx * w / len);
        }
      }
      if (stir != 0 || zeroG) {
        for (int i = 0; i < s5_per; i++) {
          int k = a + i, ex = s5_x[k] - cx, ey = s5_y[k] - cy;
          if (ex > 2400 || ex < -2400 || ey > 2400 || ey < -2400) continue;
          s5_x[k] += (int16_t)(-ey * stir / 40000 - ex * v3_abs(stir) / 90000); s5_y[k] += (int16_t)(ex * stir / 40000);   // round, and a little inwards
          if (zeroG) { s5_x[k] -= (int16_t)(ex / 160); s5_y[k] -= (int16_t)(ey / 160); }   // drawn into a ball
        }
      }
    }
    s5_ropes(iters);
    if (sg5_phase != SG5_DRAIN) s5_crowd(2, push);                 // (not while draining: the crowd would hold them up)
    // the container
    bool open = sg5_phase == SG5_DRAIN;
    for (int k = 0; k < s5_n; k++) {
      if (s5_pinned(k)) continue;
      int x = s5_x[k] >> 4;
      int fy = open ? 2000 : sg5_floor(x, cont);
      if (zeroG) fy = H - 2;
      if (s5_y[k] > fy * 16 && s5_y[k] < (fy + 30) * 16) {
        int vx = s5_x[k] - s5_ox[k], vy = s5_y[k] - s5_oy[k];
        s5_y[k] = (int16_t)(fy * 16); s5_oy[k] = (int16_t)(fy * 16 + vy * s5_bounce / 256); s5_ox[k] = (int16_t)(s5_x[k] - vx * 150 / 256);
      }
      if (!open && cont == SG5_BOX && s5_y[k] > 80 * 16) {            // the box's walls
        int ox = s5_ox[k] >> 4;
        if (x < 70 && ox >= 70) { s5_x[k] = 70 * 16; s5_ox[k] = s5_x[k]; }
        if (x > 250 && ox <= 250) { s5_x[k] = 250 * 16; s5_ox[k] = s5_x[k]; }
        if (x >= 60 && x < 70 && ox < 60) { s5_x[k] = 59 * 16; s5_ox[k] = s5_x[k]; }
        if (x <= 260 && x > 250 && ox > 260) { s5_x[k] = 261 * 16; s5_ox[k] = s5_x[k]; }
      }
      if (grav < 0 && s5_y[k] < 2 * 16) { s5_y[k] = 2 * 16; s5_oy[k] = s5_y[k]; }   // falling upwards: a ceiling
      if (s5_x[k] < 0) { s5_x[k] = 0; s5_ox[k] = 0; } else if (s5_x[k] > (W - 1) * 16) { s5_x[k] = (W - 1) * 16; s5_ox[k] = s5_x[k]; }
    }
  }

  // ── draw ──
  if (trails > 0) v3_fadeLev(16 - trails); else s5_clear(0);
  if (sg5_phase != SG5_DRAIN && !(sg5_look == 8 && v3_centre(2))) {    // the container, as an outline
    uint8_t cc = (uint8_t)((7 << 4) | 15);
    if (cont == SG5_BOX) { v3_rect(56, 80, 59, H - 6, cc); v3_rect(261, 80, 264, H - 6, cc); v3_hspan(56, 264, H - 5, cc); v3_hspan(56, 264, H - 4, cc); }
    else {
      int py = sg5_floor(0, cont);
      for (int x = 1; x < W; x++) {
        int y = sg5_floor(x, cont);
        if (y < H && py < H) { v3_line(x - 1, py + 1, x, y + 1, cc); v3_line(x - 1, py + 2, x, y + 2, cc); }
        py = y;
      }
    }
  }
  for (int s = 0; s < s5_ns; s++) {
    if (!sg5_isLive(s)) continue;
    int hue = (int)(v3_hash((uint32_t)s * 2654435761u) % (uint32_t)Q.nc) & 7;
    int lev = (s & 1) ? 15 : 12;
    if (lineStyle == 2) s5_drawStrandAA(s, lev, hue);
    else s5_drawStrand(s, (uint8_t)((lev << 4) | hue), lineStyle == 1 ? (uint8_t)((5 << 4) | hue) : 0);
  }
}
void prog_spag_draw(int preset) { spag5_draw(preset); fx5_apply(); }
