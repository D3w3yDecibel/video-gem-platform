// =====================================================================
// PROGRAM: SPROUT — growing plants (by Dewey) — Set 3
// Seeds put up stems that reach for the light, wander, branch and branch
// again. As a plant grows, the old stems near its base thicken (the more
// growth a stem carries, the thicker it gets), leaves unfurl along the
// way, and glowing nodes in contrasting colours open at every branch end
// and fork. Pulses ripple out through the branches and the nodes beat and
// swap colours, so a grown garden keeps moving. When a garden has grown it
// slowly fades while the next one sprouts through it.
//
// Inspired by Laurent Malys' "Pousse" (https://www.instagram.com/p/DeZb_AbD48V/):
// thin white stems on black, branching up from a base, thickening below.
// The code here is an original take on that look.
//
// Presets:
//   k0 Pousse    — one seed, white on black, reaching for a wandering light (closest to the original)
//   k1 Meadow    — a row of seeds: grasses and flowers
//   k2 Tree      — one trunk that thickens and spreads into a crown
//   k3 Tendrils  — coiling tendrils that tighten into spirals
//   k4 Roots     — roots pushing down from the surface
//   k5 Coral     — forking growth, each generation its own colour
//   k6 Neural    — dendrites spreading from the centre, glowing synapses
//   k7 Snowflake — six-fold crystal growing from the centre
//   k8 Mandala   — eight-fold leafy growth from the centre
//   k9 Mycelium  — a fine white web from scattered spores
//   k10 Bloom    — flowering plants with big contrasting blossoms
//   k11 Garden   — a different kind of garden every time it regrows
//
//   k12 — REGROW: fade this garden and sow the next one now
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette       p1 Speed (far left = freeze)   p2 Branching   p3 Branch Angle
//   p4 Curl (how much stems wander)   p5 Tropism (how hard they reach)
//   p6 Light Motion (how fast the light they reach for moves)
//   p7 Thickening    p8 Leaves   p9 Nodes (glowing nodes at branch ends and forks)
//   p10 Seeds   p11 Reach (how far stems grow)
//   p12 Density (how many growing tips at once)   p13 Regrow Time
//   p14 Symmetry (centre = preset; left 1-3 fold, right 4-8 fold kaleidoscope)
//   p15 Pulse (energy pulses rippling out through the branches, and the nodes' beat)
// =====================================================================

#include "vg_sin.h"
static inline int ps_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline float ps_clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static uint32_t ps_rng = 31337u;
static inline uint32_t ps_rand() { ps_rng = ps_rng * 1664525u + 1013904223u; return ps_rng >> 8; }
static inline int ps_rnd(int n) { return n > 0 ? (int)(ps_rand() % (uint32_t)n) : 0; }
static inline int32_t ps_cos(int a16) { return vg_sin1024[((a16 >> 6) + 256) & 1023]; }   // angle 0 … 65535 = one turn; Q14
static inline int32_t ps_sin(int a16) { return vg_sin1024[(a16 >> 6) & 1023]; }
static inline int ps_atan16(int32_t dy, int32_t dx) {             // angle 0 … 65535, integers only
  int32_t ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
  if (ax == 0 && ay == 0) return 0;
  int32_t z = ax > ay ? (ay << 12) / ax : (ax << 12) / ay;
  int32_t t = 8192 + ((2848 * (4096 - z)) >> 12);
  int a = (int)((z * t) >> 18);                                   // 1/1024 turns
  if (ay > ax) a = 256 - a;
  if (dx < 0) a = 512 - a;
  if (dy < 0) a = 1024 - a;
  return (a & 1023) << 6;
}

// ─── Colours ──────────────────────────────────────────────────────────
// Two banks of 8 hues: the garden growing now uses one, the one fading away the other —
// so the old one can fade (and change colour) while the new one grows through it.
// Within a bank: 1 … 4 stems by generation, 5 leaves, 6 and 7 the glowing nodes at branch ends
// and forks — in colours that contrast with the stems, and animated through the palette (so they
// pulse and swap colours without redrawing a pixel). Pulses also ripple out through the stems,
// generation by generation, the same way.
#define PS_NPAL 8
static const uint8_t ps_pal[PS_NPAL][8][3] = {
  {{0,0,0},{255,255,255},{225,225,235},{200,200,215},{175,175,195},{190,190,190},{0,230,255},{255,40,190}},       // 0 Mono: white stems, cyan & magenta nodes
  {{0,0,0},{120,220,60},{90,200,70},{150,240,60},{60,180,90},{40,200,60},{255,220,0},{255,50,150}},             // 1 Spring: greens, yellow & pink
  {{0,0,0},{230,200,160},{255,170,60},{255,120,30},{230,70,30},{255,150,0},{0,210,255},{255,255,140}},           // 2 Autumn: bark & orange, cyan & cream
  {{0,0,0},{200,140,90},{180,110,60},{150,90,50},{120,70,40},{120,90,50},{0,255,220},{255,80,40}},               // 3 Earth: roots, aqua & ember
  {{0,0,0},{255,90,160},{0,220,200},{255,200,60},{140,90,255},{0,255,170},{255,255,255},{255,230,0}},            // 4 Reef: pink, teal, gold, violet; white & yellow
  {{0,0,0},{0,230,255},{255,40,220},{0,160,255},{200,60,255},{0,255,160},{255,255,0},{255,90,0}},                // 5 Neon: cyan & magenta, yellow & orange
  {{0,0,0},{200,240,255},{150,210,255},{110,180,255},{230,250,255},{170,200,255},{255,40,120},{255,255,255}},    // 6 Ice: blues, crimson & white
  {{0,0,0},{60,170,60},{40,150,70},{80,190,50},{50,140,90},{30,180,60},{255,30,150},{255,220,0}},                // 7 Bloom: greens, magenta & gold
};
static int16_t ps_levK[16];
static int ps_palA = 0, ps_palB = 0;                               // palettes of bank A (hues 0-7) and bank B (8-15)
static float ps_pulseAmt = 0.4f, ps_pulseT = 0, ps_nodeT = 0;
static void ps_buildPalette() {
  // stem pulses: a bright band runs out through the generations, root to tips
  int stemK[5];
  for (int g = 1; g <= 4; g++) {
    float u = ps_pulseT - g * 0.22f;
    u -= floorf(u);
    float c = 0.5f + 0.5f * cosf(u * 6.2831853f);
    c *= c; c *= c;                                                 // a narrow bump
    stemK[g] = (int)(256.0f * (1.0f - ps_pulseAmt * 0.55f + ps_pulseAmt * 0.55f * c * 1.6f));
  }
  // nodes: the two node colours drift into each other and back, and beat in opposite phase
  float m = 0.5f + 0.5f * sinf(ps_nodeT * 0.7f);
  float beat = 0.5f + 0.5f * sinf(ps_nodeT * 3.1f);
  int bA = (int)(256.0f * (0.62f + 0.38f * beat)), bB = (int)(256.0f * (0.62f + 0.38f * (1.0f - beat)));
  int mi = (int)(m * 256.0f);
  for (int h = 0; h < 16; h++) {
    const uint8_t (*pal)[3] = ps_pal[h < 8 ? ps_palA : ps_palB];
    int hh = h & 7;
    int r = pal[hh][0], g = pal[hh][1], bl = pal[hh][2], k = 256;
    if (hh >= 1 && hh <= 4) k = stemK[hh];
    else if (hh == 6 || hh == 7) {
      const uint8_t* o = pal[hh == 6 ? 7 : 6];
      int w = hh == 6 ? mi : 256 - mi;                                // 6 drifts toward 7's colour while 7 drifts toward 6's
      w = w * 2 / 5;                                                  // (never far: they stay distinct and contrasting)
      r = (r * (256 - w) + o[0] * w) >> 8; g = (g * (256 - w) + o[1] * w) >> 8; bl = (bl * (256 - w) + o[2] * w) >> 8;
      k = hh == 6 ? bA : bB;
    }
    display.setColor(h, 0, 0, 0);
    for (int l = 1; l < 16; l++) {
      int kk = ps_levK[l] * k >> 8;
      display.setColor((l << 4) | h, ps_clamp(r * kk >> 8, 0, 255), ps_clamp(g * kk >> 8, 0, 255), ps_clamp(bl * kk >> 8, 0, 255));
    }
  }
}

// ─── Drawing ──────────────────────────────────────────────────────────
static uint8_t* ps_buf;
static uint8_t ps_bank = 0;                                       // 0 or 8: which hue bank the growing garden uses
// the new garden draws over the old one; within a garden, brightest wins
static inline void ps_put(int x, int y, int lev, int hue) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H || lev <= 0) return;
  if (lev > 15) lev = 15;
  uint8_t* p = ps_buf + y * W + x;
  uint8_t v = *p;
  if ((v & 8) != ps_bank || lev > (v >> 4)) *p = (uint8_t)((lev << 4) | ps_bank | hue);
}
// an anti-aliased hairline between points in 1/8 px (Wu)
static void ps_lineAA1(int x0, int y0, int x1, int y1, int lev, int hue) {
  if (lev <= 0) return;
  int dx = x1 - x0, dy = y1 - y0;
  int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
  if (adx > 8 * 400 || ady > 8 * 400) return;
  if (adx < 8 && ady < 8) { ps_put((x0 + 4) >> 3, (y0 + 4) >> 3, lev, hue); return; }
  if (adx >= ady) {
    if (x1 < x0) { int t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; dy = -dy; }
    int32_t grad = (int32_t)dy * 65536 / (x1 - x0);
    int xs = (x0 + 7) >> 3, xe = x1 >> 3;
    int32_t y = y0 * 65536 + (int32_t)(xs * 8 - x0) * grad, step = grad * 8;
    for (int x = xs; x <= xe; x++, y += step) {
      int yy = (int)(y >> 16), yi = yy >> 3, f = yy & 7;
      ps_put(x, yi, (lev * (8 - f) + 4) >> 3, hue);
      if (f) ps_put(x, yi + 1, (lev * f + 4) >> 3, hue);
    }
  } else {
    if (y1 < y0) { int t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; dx = -dx; }
    int32_t grad = (int32_t)dx * 65536 / (y1 - y0);
    int ys = (y0 + 7) >> 3, ye = y1 >> 3;
    int32_t x = x0 * 65536 + (int32_t)(ys * 8 - y0) * grad, step = grad * 8;
    for (int y = ys; y <= ye; y++, x += step) {
      int xx = (int)(x >> 16), xi = xx >> 3, f = xx & 7;
      ps_put(xi, y, (lev * (8 - f) + 4) >> 3, hue);
      if (f) ps_put(xi + 1, y, (lev * f + 4) >> 3, hue);
    }
  }
}
// a filled disc, radius in 1/8 px, centre in 1/8 px
static void ps_disc1(int cx8, int cy8, int r8, int lev, int hue) {
  int cx = (cx8 + 4) >> 3, cy = (cy8 + 4) >> 3;
  if (r8 < 6) { ps_put(cx, cy, lev, hue); return; }
  int R = (r8 + 7) >> 3;
  int32_t r2 = r8 * r8, in = r8 - 5, in2 = in > 0 ? in * in : 0;
  for (int dy = -R; dy <= R; dy++) for (int dx = -R; dx <= R; dx++) {
    int32_t ex = dx * 8 - (cx8 - cx * 8), ey = dy * 8 - (cy8 - cy * 8);
    int32_t d2 = ex * ex + ey * ey;
    if (d2 > r2) continue;
    ps_put(cx + dx, cy + dy, d2 <= in2 ? lev : lev - 2, hue);
  }
}
// symmetry: every line and disc is also drawn rotated (and optionally mirrored) round the centre
static int ps_symN = 1;
static bool ps_symM = false;
static int32_t ps_symC[8], ps_symS[8];
static void ps_setSym(int n, bool mirror) {
  ps_symN = n < 1 ? 1 : (n > 8 ? 8 : n); ps_symM = mirror;
  for (int i = 0; i < ps_symN; i++) { int a = i * 1024 / ps_symN; ps_symC[i] = vg_sin1024[(a + 256) & 1023]; ps_symS[i] = vg_sin1024[a & 1023]; }
}
static inline void ps_symPt(int i, bool mir, int x8, int y8, int* ox, int* oy) {
  int32_t dx = x8 - W * 4, dy = y8 - H * 4;
  if (mir) dx = -dx;
  *ox = W * 4 + (int)((dx * ps_symC[i] - dy * ps_symS[i]) >> 14);
  *oy = H * 4 + (int)((dx * ps_symS[i] + dy * ps_symC[i]) >> 14);
}
static void ps_lineAA(int x0, int y0, int x1, int y1, int lev, int hue) {
  if (ps_symN <= 1 && !ps_symM) { ps_lineAA1(x0, y0, x1, y1, lev, hue); return; }
  for (int m = 0; m <= (ps_symM ? 1 : 0); m++) for (int i = 0; i < ps_symN; i++) {
    int a, b, c, d; ps_symPt(i, m, x0, y0, &a, &b); ps_symPt(i, m, x1, y1, &c, &d);
    ps_lineAA1(a, b, c, d, lev, hue);
  }
}
static void ps_disc(int cx8, int cy8, int r8, int lev, int hue) {
  if (ps_symN <= 1 && !ps_symM) { ps_disc1(cx8, cy8, r8, lev, hue); return; }
  for (int m = 0; m <= (ps_symM ? 1 : 0); m++) for (int i = 0; i < ps_symN; i++) {
    int a, b; ps_symPt(i, m, cx8, cy8, &a, &b);
    ps_disc1(a, b, r8, lev, hue);
  }
}
// a stem segment of a given radius (1/8 px): a hairline when thin, a row of discs when thick
static void ps_stem(int x0, int y0, int x1, int y1, int r8, int lev, int hue) {
  if (r8 <= 6) { ps_lineAA(x0, y0, x1, y1, lev, hue); return; }
  int dx = x1 - x0, dy = y1 - y0;
  int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
  int n = (adx > ady ? adx : ady) / 4 + 1;                        // a disc every ½ px: a smooth, solid stem
  for (int i = 0; i <= n; i++) ps_disc(x0 + dx * i / n, y0 + dy * i / n, r8, lev, hue);
}

// ─── The plant: nodes (for thickening) and growing tips ───────────────
#define PS_NODES 800
#define PS_TIPS 96
static int16_t ps_nx[PS_NODES], ps_ny[PS_NODES];                  // node position, 1/8 px
static uint16_t ps_npar[PS_NODES];                                 // parent node (low 11 bits) | generation (top 5 bits)
static uint16_t ps_ndesc[PS_NODES];                                // how much growth this stem carries
static int ps_nn = 0;
#define PS_NOPAR 0x7FF
static int32_t ps_tx[PS_TIPS], ps_ty[PS_TIPS];                    // tip position, Q8 px
static uint16_t ps_tth[PS_TIPS];                                   // heading (0 … 65535)
static int16_t ps_tcurl[PS_TIPS];                                  // current bend
static int16_t ps_tbias[PS_TIPS];                                  // each tip leans its own way, so branches fan out
static int8_t ps_tspin[PS_TIPS];                                    // which way a tendril coils
static uint16_t ps_tnode[PS_TIPS];                                 // the node the tip grows from
static int16_t ps_tsx[PS_TIPS], ps_tsy[PS_TIPS];                  // where the current bit of stem started (1/8 px)
static int16_t ps_tlife[PS_TIPS];                                  // segments left to grow
static uint8_t ps_tgen[PS_TIPS], ps_tside[PS_TIPS];
static int32_t ps_tseg[PS_TIPS];                                   // distance grown since the last node (Q8)
static int ps_nt = 0;

// growth settings (from preset + knobs)
#define TR_NONE 0
#define TR_LIGHT 1                                                // toward a (moving) light
#define TR_UP 2
#define TR_DOWN 3
#define TR_OUT 4                                                  // outward, away from the centre
#define TR_IN 5                                                   // toward the centre
static int ps_trop = TR_LIGHT, ps_tropK = 30;                    // tropism and its strength (/256 per node)
static int ps_branchP = 30;                                      // chance of a branch at each node (/256)
static int ps_branchA = 5461;                                    // branch angle (1/65536 turns)
static int ps_curl = 300;                                        // bend wander
static int ps_segL = 4 * 256;                                    // segment length, Q8 px
static int ps_maxGen = 6, ps_life0 = 90, ps_maxTips = 64;
static int ps_thickK = 100, ps_leafP = 0, ps_budP = 60, ps_budR = 14;
static bool ps_fork = false;                                     // Coral: tips split in two instead of side-branching
static bool ps_straight = false;                                 // Snowflake: no wander
static int ps_spiral = 0;                                        // Tendrils: a steady coil that tightens toward the tip
static int ps_joints = 1;                                        // nodes at forks
static int32_t ps_lx = 280 * 256, ps_ly = 20 * 256;              // the light (Q8 px)

static int ps_genHue(int gen) { return 1 + gen % 4; }
static int ps_radius8(int desc) {                                // stem radius (1/8 px) from the growth it carries
  int lg = 0; int d = desc; while (d > 1) { d >>= 1; lg++; }     // ≈ log2
  int r = 3 + (lg * ps_thickK) / 45;
  return r > 22 ? 22 : r;
}
static int ps_stemLev(int r8, int gen) { int l = 10 + r8 / 6 - gen / 3; return ps_clamp(l, 7, 15); }

static int ps_addNode(int32_t x8, int32_t y8, int parent, int gen) {
  if (ps_nn >= PS_NODES) return -1;
  int k = ps_nn++;
  ps_nx[k] = (int16_t)ps_clamp(x8, -30000, 30000); ps_ny[k] = (int16_t)ps_clamp(y8, -30000, 30000);
  ps_npar[k] = (uint16_t)((parent & 0x7FF) | ((gen & 31) << 11));
  ps_ndesc[k] = 1;
  return k;
}
// a node was added under `k`: every ancestor carries a little more; restamp those that thicken
static void ps_thicken(int k) {
  int guard = 0;
  while (k >= 0 && guard++ < 600) {
    int par = ps_npar[k] & 0x7FF;
    if (par == PS_NOPAR) break;
    int before = ps_radius8(ps_ndesc[par]);
    if (ps_ndesc[par] < 65000) ps_ndesc[par]++;
    int after = ps_radius8(ps_ndesc[par]);
    if (after > before) {                                         // this stem just got thicker: draw it again, fatter
      int gp = ps_npar[par] & 0x7FF;
      int gen = ps_npar[par] >> 11;
      if (gp != PS_NOPAR) ps_stem(ps_nx[gp], ps_ny[gp], ps_nx[par], ps_ny[par], after, ps_stemLev(after, gen), ps_genHue(gen));
    }
    k = par;
  }
}
static void ps_addTip(int32_t x, int32_t y, int th, int node, int gen, int life) {
  if (ps_nt >= PS_TIPS || ps_nt >= ps_maxTips || life <= 2) return;
  int t = ps_nt++;
  ps_tx[t] = x; ps_ty[t] = y; ps_tth[t] = (uint16_t)th; ps_tcurl[t] = 0;
  ps_tbias[t] = (int16_t)(gen == 0 ? 0 : (ps_rnd(2) ? 1 : -1) * (ps_branchA / 3 + ps_rnd(ps_branchA / 2 + 1)));
  ps_tnode[t] = (uint16_t)node; ps_tlife[t] = (int16_t)life;
  ps_tspin[t] = (int8_t)(ps_rnd(2) ? 1 : -1);
  ps_tsx[t] = (int16_t)ps_clamp(x >> 5, -30000, 30000); ps_tsy[t] = (int16_t)ps_clamp(y >> 5, -30000, 30000); ps_tgen[t] = (uint8_t)gen; ps_tside[t] = (uint8_t)(ps_rand() & 1);
  ps_tseg[t] = 0;
}
static void ps_removeTip(int t) {
  int j = --ps_nt;
  if (t != j) {
    ps_tsx[t] = ps_tsx[j]; ps_tsy[t] = ps_tsy[j]; ps_tspin[t] = ps_tspin[j];
    ps_tx[t] = ps_tx[j]; ps_ty[t] = ps_ty[j]; ps_tth[t] = ps_tth[j]; ps_tcurl[t] = ps_tcurl[j]; ps_tbias[t] = ps_tbias[j]; ps_tnode[t] = ps_tnode[j];
    ps_tlife[t] = ps_tlife[j]; ps_tgen[t] = ps_tgen[j]; ps_tside[t] = ps_tside[j]; ps_tseg[t] = ps_tseg[j];
  }
}
// a leaf: a little pointed oval off to one side of the stem
static void ps_leaf(int32_t x8, int32_t y8, int th, int side, int gen) {
  int a = th + (side ? 14000 : -14000);
  int L = 30 + ps_rnd(30);                                        // length, 1/8 px
  int32_t c = ps_cos(a), s = ps_sin(a);
  for (int i = 1; i <= 5; i++) {
    int r = (i < 3 ? i : 6 - i) * 6 + 4;                          // swell then taper
    ps_disc(x8 + ((c * L * i / 6) >> 14), y8 + ((s * L * i / 6) >> 14), r, 11, 5);
  }
  ps_lineAA(x8, y8, x8 + ((c * L) >> 14), y8 + ((s * L) >> 14), 13, 5);
}
// a glowing node at the end of a branch: a ball in one node colour, a halo in the other
static void ps_bud(int32_t x8, int32_t y8, int big) {
  int r = ps_budR * (big ? 2 : 1);
  ps_disc(x8, y8, r + 10, 8, 7);
  ps_disc(x8, y8, r, 15, 6);
}
// a small node where a branch forks off
static void ps_joint(int32_t x8, int32_t y8) { ps_disc(x8, y8, 10, 13, 7); }

// one tip grows a little; returns false if it is finished
static int ps_lightMode = 0;
static bool ps_grow(int t, int32_t step) {
  // bend: a wandering curl plus the pull of the tropism
  int th = ps_tth[t];
  if (!ps_straight) {
    ps_tcurl[t] = (int16_t)((ps_tcurl[t] * 15) / 16 + (ps_rnd(2 * ps_curl + 1) - ps_curl));
    th += ps_tcurl[t] / 8;
  }
  int target = -1;
  switch (ps_trop) {
    case TR_LIGHT: {
      int32_t ddx = (ps_lx - ps_tx[t]) >> 8, ddy = (ps_ly - ps_ty[t]) >> 8;
      if ((ddx < 0 ? -ddx : ddx) + (ddy < 0 ? -ddy : ddy) > 45) target = ps_atan16(ddy, ddx);   // close to the light: stop reaching (no tangles round it)
    } break;
    case TR_UP: target = 49152; break;
    case TR_DOWN: target = 16384; break;
    case TR_IN: target = ps_atan16((H * 128 - ps_ty[t]) >> 4, (W * 128 - ps_tx[t]) >> 4); break;
    case TR_OUT: target = ps_atan16((ps_ty[t] - H * 128) >> 4, (ps_tx[t] - W * 128) >> 4); break;
  }
  if (target >= 0) {                                              // younger branches feel the pull less, and each keeps its own lean
    int k = ps_tropK * (8 - (ps_tgen[t] > 6 ? 6 : ps_tgen[t])) / 8;
    th += ((int16_t)(target + ps_tbias[t] - th) * k) >> 10;
  }
  if (ps_spiral) th += ps_tspin[t] * ps_spiral * (ps_life0 + 8 - ps_tlife[t]) / (ps_life0 + 8) * 2;   // tendrils coil, tighter toward the tip
  ps_tth[t] = (uint16_t)th;
  // move
  int32_t nx = ps_tx[t] + ((ps_cos(th) * step) >> 14), ny = ps_ty[t] + ((ps_sin(th) * step) >> 14);
  ps_tx[t] = nx; ps_ty[t] = ny;
  // the growing tip: draw the bit of new stem since the last node
  int node = ps_tnode[t];
  int gen = ps_tgen[t];
  ps_lineAA(ps_tsx[t], ps_tsy[t], nx >> 5, ny >> 5, ps_stemLev(4, gen), ps_genHue(gen));
  ps_tseg[t] += step;
  if (nx < -20 * 256 || nx > (W + 20) * 256 || ny < -20 * 256 || ny > (H + 20) * 256) return false;   // left the screen
  if (ps_tseg[t] < ps_segL) return true;
  // a new node
  ps_tseg[t] = 0;
  ps_tsx[t] = (int16_t)ps_clamp(nx >> 5, -30000, 30000); ps_tsy[t] = (int16_t)ps_clamp(ny >> 5, -30000, 30000);
  int k = ps_addNode(nx >> 5, ny >> 5, node, gen);
  if (k >= 0) { ps_tnode[t] = (uint16_t)k; ps_thicken(k); }       // (once the node store is full, stems keep growing; they just stop thickening the ones below)
  else k = node;
  if (--ps_tlife[t] <= 0) {                                       // done: a bud at the tip
    if (ps_rnd(256) < ps_budP) ps_bud(nx >> 5, ny >> 5, gen <= 1);
    return false;
  }
  if (ps_leafP > 0 && ps_rnd(256) < ps_leafP) { ps_leaf(nx >> 5, ny >> 5, th, ps_tside[t], gen); ps_tside[t] ^= 1; }
  // branch
  if (gen < ps_maxGen && ps_rnd(256) < ps_branchP) {
    int life = ps_tlife[t] * (150 + ps_rnd(80)) / 256;
    if (ps_joints) ps_joint(nx >> 5, ny >> 5);
    if (ps_fork) {                                                // split in two
      ps_tth[t] = (uint16_t)(th - ps_branchA / 2);
      ps_tgen[t] = (uint8_t)(gen + 1);
      ps_addTip(nx, ny, th + ps_branchA / 2, k, gen + 1, ps_tlife[t]);
    } else {
      int side = ps_tside[t]; ps_tside[t] ^= 1;
      ps_addTip(nx, ny, th + (side ? ps_branchA : -ps_branchA), k, gen + 1, life);
    }
  }
  return true;
}

// ─── Gardens ──────────────────────────────────────────────────────────
const char* prog_sprout_name() { return "SPROUT"; }
const char* prog_sprout_character() { return "Growing plants: stems reach for the light, branch, thicken and bloom, then a new garden grows through the old"; }
static const char* const ps_presetNames[] = {
  "Pousse", "Meadow", "Tree", "Tendrils", "Roots", "Coral",
  "Neural", "Snowflake", "Mandala", "Mycelium", "Bloom", "Garden"
};
#define PS_NUM_PRESETS 12
//                                                Pou  Mea  Tre  Ten  Roo  Cor  Neu  Sno  Man  Myc  Blo  Gar
static const uint8_t ps_pTrop[PS_NUM_PRESETS]  = {TR_LIGHT, TR_UP, TR_UP, TR_UP, TR_DOWN, TR_UP, TR_OUT, TR_OUT, TR_OUT, TR_NONE, TR_UP, TR_LIGHT};
static const uint8_t ps_pTropK[PS_NUM_PRESETS] = { 18,  40,  24,  20,  30,  14,  22,   0,  14,   0,  32,  20};
static const uint8_t ps_pBranch[PS_NUM_PRESETS]= { 34,  14,  48,  22,  56,  44,  46,  70,  40,  52,  30,  34};   // /256 per node
static const uint8_t ps_pAngle[PS_NUM_PRESETS] = { 34,  22,  34,  40,  34,  36,  45,  60,  40,  50,  32,  34};   // degrees
static const uint8_t ps_pCurl[PS_NUM_PRESETS]  = { 60,  45,  40,  30,  70,  50,  95,   0,  60,  90,  45,  60};
static const uint8_t ps_pSpiral[PS_NUM_PRESETS]= {  0,   0,   0,  90,   0,   0,   0,   0,  30,   0,   0,   0};   // tendril coil
static const uint8_t ps_pSeeds[PS_NUM_PRESETS] = {  1,  12,   1,   5,   5,   3,   2,   1,   1,   6,   3,   2};
static const uint8_t ps_pLife[PS_NUM_PRESETS]  = { 95,  45,  80,  70, 110,  50,  60,  28,  34,  45,  55,  80};   // segments
static const uint8_t ps_pGen[PS_NUM_PRESETS]   = {  6,   2,   7,   4,   6,   6,   5,   4,   5,   7,   4,   6};
static const uint8_t ps_pThick[PS_NUM_PRESETS] = { 70,  35, 120,  40,  50,  80,  40,  25,  40,  10,  45,  70};
static const uint8_t ps_pLeaf[PS_NUM_PRESETS]  = {  0,  20,   0,  30,   0,   0,   0,   0,  60,   0,  50,   0};   // /256 per node
static const uint8_t ps_pBud[PS_NUM_PRESETS]   = {200, 170, 130, 220,  90, 200, 230, 220, 200,  60, 255, 200};   // /256 per finished tip
static const uint8_t ps_pBudR[PS_NUM_PRESETS]  = { 19,  16,  15,  18,  11,  18,  17,  13,  15,  11,  24,  18};   // 1/8 px
static const uint8_t ps_pPal[PS_NUM_PRESETS]   = {  0,   1,   2,   5,   3,   4,   5,   6,   4,   0,   7,   0};
static const uint8_t ps_pTips[PS_NUM_PRESETS]  = { 48,  60,  64,  60,  80,  64,  80,  40,  40,  96,  60,  64};
static const uint8_t ps_pSym[PS_NUM_PRESETS]   = {  1,   1,   1,   1,   1,   1,   1,   6,   8,   1,   1,   1};   // fold symmetry round the centre
static const uint8_t ps_pMirror[PS_NUM_PRESETS]= {  0,   0,   0,   0,   0,   0,   0,   1,   0,   0,   0,   0};
static const uint8_t ps_pPulse[PS_NUM_PRESETS] = { 50,  40,  45,  70,  40,  60,  90,  55,  70,  35,  50,  55};   // %

const char* prog_sprout_presetName(int preset) {
  if (preset >= 0 && preset < PS_NUM_PRESETS) return ps_presetNames[preset];
  return NULL;
}
static const char* const ps_labels[16] = {
  "Palette", "Speed", "Branching", "Branch Angle", "Curl", "Tropism", "Light Motion", "Thickening",
  "Leaves", "Nodes", "Seeds", "Reach", "Density", "Regrow Time", "Symmetry", "Pulse"
};
const char* prog_sprout_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 16) return ps_labels[pot];
  return "";
}
uint8_t prog_sprout_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // the picture builds up: we keep it between frames
}

// ─── Knobs ────────────────────────────────────────────────────────────
static float ps_sm[16];
static bool ps_smReady = false;
static inline int ps_k(int i) { int v = (int)ps_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }
static inline bool ps_centre(int i) { int k = ps_k(i); return k >= 440 && k < 600; }
static inline float ps_bi(int i) { return (ps_k(i) - 512) / 512.0f; }
static inline float ps_rel(int i, float own, float range) { return own * powf(range, ps_bi(i)); }

// sow a new garden of kind `look`
static int ps_look = 0;
static void ps_sow(int look, int seeds, int life) {
  ps_nn = 0; ps_nt = 0;
  seeds = ps_clamp(seeds, 1, 8);
  for (int s = 0; s < seeds; s++) {
    int32_t x = 0, y = 0; int th = 49152;                          // default: from the bottom, growing up
    switch (look) {
      case 0: x = (60 + ps_rnd(60)) * 256; y = (H + 2) * 256; th = 49152 + 3000; break;
      case 1: case 10: x = ((s + 1) * W / (seeds + 1) + ps_rnd(16) - 8) * 256; y = (H + 2) * 256; th = 49152 + ps_rnd(4000) - 2000; break;
      case 2: x = (W / 2 + ps_rnd(30) - 15) * 256; y = (H + 2) * 256; break;
      case 3: x = ((s + 1) * W / (seeds + 1) + ps_rnd(20) - 10) * 256; y = (H + 2) * 256; th = 49152 + ps_rnd(6000) - 3000; break;
      case 4: x = ((s + 1) * W / (seeds + 1)) * 256; y = -2 * 256; th = 16384; break;
      case 5: x = ((s + 1) * W / (seeds + 1)) * 256; y = (H + 2) * 256; break;
      case 6: x = (W / 2 + (s ? 40 : -40)) * 256; y = (H / 2 + ps_rnd(30) - 15) * 256; th = ps_rnd(65536); break;
      case 7: case 8: x = W * 128 + ps_rnd(512) - 256; y = H * 128 + ps_rnd(512) - 256; th = ps_rnd(65536); break;   // from the centre (drawn with symmetry)
      case 9: x = (30 + ps_rnd(W - 60)) * 256; y = (30 + ps_rnd(H - 60)) * 256; th = ps_rnd(65536); break;
      default: x = (40 + ps_rnd(W - 80)) * 256; y = (H + 2) * 256; break;
    }
    int k = ps_addNode(x >> 5, y >> 5, PS_NOPAR, 0);
    // first tips: some kinds start with several shoots from the same seed
    int shoots = (look == 6) ? 4 : (look == 9 ? 3 : (look == 7 ? 1 : (look == 8 ? 2 : 1)));
    for (int i = 0; i < shoots; i++) {
      int a = th + (shoots > 1 ? i * 65536 / shoots : 0);
      ps_addTip(x, y, a, k, 0, life);
    }
  }
}

void prog_sprout_init() {
  static bool ready = false;
  if (!ready) {
    ready = true;
    ps_levK[0] = 0;
    for (int l = 1; l < 16; l++) ps_levK[l] = (int16_t)(256.0f * (0.04f + 0.96f * powf(l / 15.0f, 1.3f)));
  }
  if (display.getBuffer()) memset(display.getBuffer(), 0, W * H);
}

static int ps_lastPreset = -1;
static float ps_genT = 0, ps_idleT = 0;
static int ps_fadeRow = 0, ps_fadeLeft = 0;                      // fading the old garden: rows done this pass, passes left
static float ps_fadeAcc = 0;
static float ps_lightT = 0;

// fade the old garden (the bank not in use) a little: a slice of rows per call
static void ps_fadeOld(int rows) {
  uint8_t oldBank = ps_bank ^ 8;
  for (int r = 0; r < rows && ps_fadeLeft > 0; r++) {
    uint8_t* p = ps_buf + ps_fadeRow * W;
    for (int x = 0; x < W; x++) {
      uint8_t v = p[x];
      if (v >= 16 && (v & 8) == oldBank) p[x] = (uint8_t)(v - 16);
    }
    if (++ps_fadeRow >= H) { ps_fadeRow = 0; ps_fadeLeft--; }
  }
}

void prog_sprout_draw(int preset) {
  ps_buf = display.getBuffer();
  if (!ps_buf) return;
  if (preset < 0 || preset >= PS_NUM_PRESETS) preset = 0;
  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  {
    float a = dt / 0.12f; if (a > 1.0f) a = 1.0f;
    for (int i = 0; i < 16; i++) {
      if (!ps_smReady) ps_sm[i] = pots[i];
      else ps_sm[i] += (pots[i] - ps_sm[i]) * a;
    }
    ps_smReady = true;
  }

  // ── knobs ──
  float sk = ps_sm[1] / 1023.0f;
  float speed = sk < 0.03f ? 0.0f : 0.25f * powf(14.0f, (sk - 0.03f) / 0.97f);   // centre ≈ 0.9 … right 3.5
  int look = ps_look;
  ps_trop = ps_pTrop[look];
  ps_tropK = (int)(ps_pTropK[look] * powf(3.0f, ps_bi(5)));
  if (ps_k(5) < 30) ps_tropK = 0;
  ps_branchP = ps_clamp((int)(ps_pBranch[look] * powf(3.0f, ps_bi(2))), 0, 200);
  ps_branchA = (int)(ps_clampf(ps_pAngle[look] * powf(2.0f, ps_bi(3)), 5, 90) * 182.04f);
  ps_curl = (int)(ps_pCurl[look] * 8 * powf(3.0f, ps_bi(4)));
  if (ps_k(4) < 30) ps_curl = 0;
  ps_straight = look == 7 && ps_centre(4);
  ps_spiral = (int)(ps_pSpiral[look] * 6 * powf(2.0f, ps_bi(4)));
  ps_fork = look == 5;
  float lightSpeed = ps_k(6) < 30 ? 0.0f : 0.08f * powf(5.0f, ps_bi(6));
  ps_thickK = (int)(ps_pThick[look] * powf(3.0f, ps_bi(7)));
  if (ps_k(7) < 30) ps_thickK = 0;
  ps_leafP = ps_clamp((int)(ps_pLeaf[look] + ps_bi(8) * 120), 0, 200);
  ps_budP = ps_clamp((int)(ps_pBud[look] + ps_bi(9) * 300), 0, 255);    // Nodes: how many branch ends get a glowing node, and how big
  ps_budR = ps_clamp((int)(ps_pBudR[look] * powf(1.8f, ps_bi(9))), 5, 36);
  ps_joints = ps_k(9) > 120;
  int seeds = ps_centre(10) ? ps_pSeeds[look] : ps_clamp(1 + ps_k(10) * 8 / 1024, 1, 8);
  int life = ps_clamp((int)(ps_pLife[look] * powf(2.0f, ps_bi(11))), 10, 250);
  ps_maxTips = ps_clamp((int)(ps_pTips[look] * powf(2.0f, ps_bi(12))), 8, PS_TIPS);
  ps_maxGen = ps_pGen[look];
  ps_life0 = life;
  float regrowT = 25.0f * powf(3.0f, ps_bi(13));                  // seconds per garden (8 … 75, centre 25)
  const int fadeRows = 12;                                        // rows of the old garden faded per frame
  static const uint8_t symChoices[7] = {1, 2, 3, 4, 5, 6, 8};
  static int gardenSym = 1; static bool gardenMir = false;
  {
    int k = ps_k(14), n, mir;
    if (k >= 440 && k < 600) { n = gardenSym; mir = gardenMir; }
    else if (k < 440) { n = symChoices[k * 3 / 440]; mir = 0; }  // left: 1, 2, 3
    else { int i = 3 + (k - 600) * 4 / 424; if (i > 6) i = 6; n = symChoices[i]; mir = i >= 5; }   // right: 4, 5, 6 (mirrored), 8 (mirrored)
    if (n != ps_symN || (mir != 0) != ps_symM) ps_setSym(n, mir != 0);
  }
  ps_pulseAmt = ps_clampf(ps_pPulse[look] / 100.0f + ps_bi(15) * 0.8f, 0.0f, 1.0f);
  float pulseSpeed = 0.35f + ps_k(15) * 0.9f / 1023.0f;

  // ── a new garden: on a new preset, REGROW, when it has finished, or when its time is up ──
  bool k12 = keysPressed[KEY_MOD_A];
  static bool k12Was = false;
  bool regrow = (k12 && !k12Was);
  k12Was = k12;
  bool first = preset != ps_lastPreset;
  if (first) {
    ps_lastPreset = preset;
    ps_rng = 31337u + (uint32_t)preset * 7919u + (uint32_t)now;
    memset(ps_buf, 0, W * H);
    ps_fadeLeft = 0;
  }
  if (speed > 0) { ps_genT += dt; if (ps_nt == 0) ps_idleT += dt; else ps_idleT = 0; }
  if (first || regrow || ps_idleT > 2.5f || ps_genT > regrowT) {
    int newLook = preset == 11 ? ps_rnd(11) : preset;
    int pal = ps_pPal[newLook];
    if (preset == 11) pal = ps_rnd(PS_NPAL);
    if (!ps_centre(0)) pal = ps_clamp(ps_k(0) * PS_NPAL / 1024, 0, PS_NPAL - 1);
    if (!first) { ps_bank ^= 8; ps_fadeLeft = 16; ps_fadeRow = 0; }   // the old garden starts fading
    if (ps_bank) ps_palB = pal; else ps_palA = pal;
    if (first) { ps_palA = ps_palB = pal; }
    ps_look = newLook; look = newLook;
    gardenSym = ps_pSym[look]; gardenMir = ps_pMirror[look] != 0;
    if (preset == 11 && ps_rnd(3) == 0) { static const uint8_t gs[4] = {4, 5, 6, 8}; gardenSym = gs[ps_rnd(4)]; gardenMir = ps_rnd(2); }   // Garden: sometimes a kaleidoscope
    if (ps_centre(14)) ps_setSym(gardenSym, gardenMir);
    ps_sow(look, ps_centre(10) ? ps_pSeeds[look] : seeds, ps_clamp((int)(ps_pLife[look] * powf(2.0f, ps_bi(11))), 10, 250));
    ps_genT = 0; ps_idleT = 0;
    ps_lightT = ps_rnd(1000) * 0.01f;
    if (first) {                                                  // never start on an empty screen: grow a little straight away
      // (bounded: at most 120 ms by the board's clock and 2500 tip-steps, feeding the 500 ms watchdog as it goes —
      //  with symmetry, density and nodes all turned up, an unbounded head start would take seconds)
      int32_t st = (int32_t)(1.6f * 256);
      unsigned long t0 = micros();
      int work = 0;
      for (int i = 0; i < 160 && ps_nt > 0 && work < 2500; i++) {
        for (int t = 0; t < ps_nt; t++) if (!ps_grow(t, st)) { ps_removeTip(t); t--; }
        work += ps_nt * ps_symN * (ps_symM ? 2 : 1);
        watchdog_update();
        if (micros() - t0 > 120000UL) break;
      }
    }
  }
  // palette knob: recolour the growing garden live
  if (!ps_centre(0)) { int pal = ps_clamp(ps_k(0) * PS_NPAL / 1024, 0, PS_NPAL - 1); if (ps_bank) ps_palB = pal; else ps_palA = pal; }
  ps_pulseT += dt * pulseSpeed; if (ps_pulseT > 1000.0f) ps_pulseT -= 1000.0f;
  ps_nodeT += dt; if (ps_nodeT > 1000.0f) ps_nodeT -= 1000.0f;
  ps_buildPalette();

  // ── the light wanders (stems that reach for it follow) ──
  ps_lightT += dt * lightSpeed;
  ps_lx = (int32_t)((W * 0.5f + W * 0.42f * sinf(ps_lightT * 1.3f + 0.6f)) * 256);
  ps_ly = (int32_t)((H * 0.18f + H * 0.16f * sinf(ps_lightT * 0.9f)) * 256);
  if (look == 0 && lightSpeed < 0.0001f) { ps_lx = (W - 30) * 256; ps_ly = 20 * 256; }

  // ── fade the old garden ──
  if (ps_fadeLeft > 0) ps_fadeOld(fadeRows);

  // ── grow ──
  if (speed > 0) {
    static float acc = 0;
    acc += dt * speed * 60.0f;                                    // growth steps this frame
    int steps = (int)acc; acc -= steps;
    if (steps > 4) steps = 4;
    int32_t st = (int32_t)(1.1f * 256);                           // each step: 1.1 px
    for (int s = 0; s < steps; s++) {
      for (int t = 0; t < ps_nt; t++) {
        if (!ps_grow(t, st)) { ps_removeTip(t); t--; }
      }
    }
  }
}
