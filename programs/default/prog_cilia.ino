// =====================================================================
// PROGRAM: CILIA — cosine creatures (by Dewey)
// Living-looking creatures made from nothing but two cosine waves and a
// phase delay. A slow wave bends the body; a fast wave beats the ribs (legs,
// bristles, cilia), each one a little later than the one before, so the
// beat ripples down the body — a "metachronal wave", the motion of
// centipedes, bristle worms and the cilia in your airways. Each rib tip
// flares when its beat peaks.
// Inspired by ekspertodniczego's "Cosine Creature" post
// (https://www.instagram.com/p/DcV9iXMt-jV/), itself after a sketch by
// yuruyurau. The code here is original.
//
// Everything moves on whole-number multiples of one loop clock, so with
// Swim steady the animation loops seamlessly.
//
// Presets:
//   k0 Centipede    — rib pairs rippling down an S-curved body
//   k1 Bristleworm  — a swimming worm, each paddle a fan of bristles
//   k2 Cilia Field  — three rows of cilia beating in travelling waves
//   k3 Medusa       — a pulsing ring with tentacles; the wave runs round it
//   k4 Fern         — one-sided fronds on a curling, spiralling body
//   k5 Twin Helix   — two creatures entwined, bodies waving in opposition
//   k6 Shoal        — four little ones swimming round each other
//   k7 Ghost        — a dense, misty cloud of dots
//   k8 Feather Star — five feathery arms curling from a centre
//   k9 Radiolarian  — three rings of spines, waves running opposite ways
//   k10 Kelp        — fronds swaying up from the sea floor
//   k11 Morph       — one creature dissolving into the next, each loop
//
//   k12 — STARTLE: the beat quickens and the ribs flare, then settle
//
// Knobs (this program owns the global knobs and makes its own colours):
//   p0 Palette (8; centre = Specimen: white body, red tips)
//   p1 Speed (far left = freeze)      p2 Segments (12 … 80, centre 32)
//   p3 Wave Delay (how far each rib lags its neighbour; far left = all in step)
//   p4 Beat (how far the ribs swing)  p5 Body Wave (how much the body bends)
//   p6 Trails   p7 Rib Length   p8 Size   p9 Swim (left = stay, right = roam)
//   p10 Curl (left = curl one way, right = the other)
//   p11 Lines: left = dotted, centre = crisp 1-px lines, right = bold 2-px
//   p12 Pan X   p14 Pan Y
//   p13 Tips (left = none … right = big flares)   p15 Glow (line brightness)
//
// Pixels are (brightness << 4) | hue: hues 0-13 run along the body
// (the palette's gradient), 14 = tip flare, 15 = spine. Lines are drawn
// "brightest wins", so crossings stay crisp instead of blooming; a quick
// fade of the brightness leaves the trails. The per-pixel work is
// whole-number maths, and the beat's sines are stepped by rotation, so
// there are only a few table look-ups per rib.
// =====================================================================

// ─── Maths ────────────────────────────────────────────────────────────
#define CC_TWO_PI 6.28318531f
#define CC_PI     3.14159265f
#include "vg_sin.h"
#define cc_sinT vg_sin512                        // shared sine table (in flash: no RAM)
static inline float cc_sin(float a) {            // table sine with a linear blend
  float f = a * (512.0f / CC_TWO_PI);
  int i = (int)f;
  if (f < 0) i--;
  float fr = f - (float)i;
  int s0 = cc_sinT[i & 511], s1 = cc_sinT[(i + 1) & 511];
  return (s0 + (s1 - s0) * fr) * (1.0f / 16384.0f);
}
static inline float cc_cos(float a) { return cc_sin(a + CC_PI * 0.5f); }
static inline int cc_clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline uint32_t cc_hash(uint32_t a, uint32_t b) {
  uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
  h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12;
  return h;
}

// ─── Colours ──────────────────────────────────────────────────────────
#define CC_TIP 14
#define CC_SPINE 15
#define CC_NSCHEME 8
// per scheme: body head colour, body tail colour, tip flare, spine
static const uint8_t cc_cols[CC_NSCHEME][4][3] = {
  {{190,235,255},{90,140,255},{255,40,190},{220,245,255}},   // Ice: pale blue → blue, magenta tips
  {{190,255,40},{60,255,170},{170,60,255},{230,255,190}},    // Acid: lime → mint, violet tips
  {{255,190,70},{255,110,30},{0,220,255},{255,230,180}},     // Ember: amber → orange, cyan tips
  {{40,230,210},{40,90,255},{255,140,20},{170,240,255}},     // Deep Sea: teal → blue, orange tips
  {{235,235,230},{190,190,195},{255,30,40},{255,255,255}},   // Specimen: white body, red tips
  {{0,255,230},{160,70,255},{255,240,40},{200,255,250}},     // Biolume: cyan → violet, yellow tips
  {{255,60,60},{60,90,255},{255,255,255},{255,255,255}},     // Rainbow: (body runs the wheel), white tips
  {{255,40,30},{255,170,20},{0,255,220},{255,220,200}},      // Infrared: red → amber, cyan tips
};
static int16_t cc_levK[16];                      // brightness ramp (0..256), worked out once
static int cc_lastScheme = -1;
static uint8_t cc_rgb[16][3];                    // each hue's full colour (rebuilt only when the palette changes)

static void cc_schemeColours(int scheme) {
  const uint8_t (*c)[3] = cc_cols[scheme];
  for (int h = 0; h < 16; h++) {
    int r, g, b;
    if (h == CC_TIP) { r = c[2][0]; g = c[2][1]; b = c[2][2]; }
    else if (h == CC_SPINE) { r = c[3][0]; g = c[3][1]; b = c[3][2]; }
    else if (scheme == 6) {                                       // rainbow round the wheel
      float a = h * (CC_TWO_PI / 14.0f);
      r = (int)(150 + 105 * cc_cos(a)); g = (int)(150 + 105 * cc_cos(a - 2.094f)); b = (int)(150 + 105 * cc_cos(a + 2.094f));
    } else {
      r = c[0][0] + (c[1][0] - c[0][0]) * h / 13;
      g = c[0][1] + (c[1][1] - c[0][1]) * h / 13;
      b = c[0][2] + (c[1][2] - c[0][2]) * h / 13;
    }
    cc_rgb[h][0] = (uint8_t)r; cc_rgb[h][1] = (uint8_t)g; cc_rgb[h][2] = (uint8_t)b;
  }
}
static void cc_buildPalette(int scheme) {
  if (scheme != cc_lastScheme) { cc_schemeColours(scheme); cc_lastScheme = scheme; }
  for (int h = 0; h < 16; h++) {
    int r = cc_rgb[h][0], g = cc_rgb[h][1], b = cc_rgb[h][2];
    display.setColor(h, 0, 0, 0);
    for (int l = 1; l < 16; l++) {
      int k = cc_levK[l];
      int rr = r * k >> 8, gg = g * k >> 8, bb = b * k >> 8;
      if (l >= 13 && h != CC_TIP) { int w = (l - 12) * 30; rr += (255 - rr) * w >> 8; gg += (255 - gg) * w >> 8; bb += (255 - bb) * w >> 8; }
      display.setColor((l << 4) | h, rr, gg, bb);
    }
  }
}

// ─── Drawing ──────────────────────────────────────────────────────────
static uint8_t* cc_buf;
static int cc_mul = 256;                          // overall brightness (Q8): Morph's cross-fade
static int cc_dash = 0;                           // 0 = solid, else draw every n-th pixel
static int cc_thick = 0;                          // 1 = a second pixel beside each line pixel
static int cc_dashC = 0;

// brightest wins (crisp crossings)
static inline void cc_putP(uint8_t* p, int hue, int lev) {
  if ((*p >> 4) <= lev) *p = (uint8_t)((lev << 4) | hue);
}
static inline void cc_put(int x, int y, int hue, int lev) {
  if ((unsigned)x < (unsigned)W && (unsigned)y < (unsigned)H) cc_putP(cc_buf + y * W + x, hue, lev);
}
// light adds up (Ghost's misty dots)
static inline void cc_add(int x, int y, int hue, int a) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H) return;
  uint8_t* p = cc_buf + y * W + x;
  int l = (*p >> 4) + a;
  if (l > 12) l = 12;
  if ((*p & 15) == CC_TIP && (*p >> 4) > l) return;
  *p = (uint8_t)((l << 4) | hue);
}

static void cc_line(int x0, int y0, int x1, int y1, int hue, int lev) {
  if (lev <= 0) return;
  if ((x0 < -2 && x1 < -2) || (x0 > W + 1 && x1 > W + 1) || (y0 < -2 && y1 < -2) || (y0 > H + 1 && y1 > H + 1)) return;
  int dx = abs(x1 - x0), dy = abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  int err = dx - dy;
  int tx = dx >= dy ? 0 : 1, ty = 1 - tx;                         // thick: the second pixel's offset
  int lt = lev - 3;
  bool inside = x0 > 1 && x1 > 1 && y0 > 1 && y1 > 1 && x0 < W - 2 && x1 < W - 2 && y0 < H - 2 && y1 < H - 2;
  int x = x0, y = y0, n = 0;
  for (;;) {
    bool on = cc_dash == 0 || (cc_dashC++ % cc_dash) == 0;
    if (on) {
      if (inside) {
        uint8_t* p = cc_buf + y * W + x;
        cc_putP(p, hue, lev);
        if (cc_thick && lt > 0) cc_putP(p + ty * W + tx, hue, lt);
      } else {
        cc_put(x, y, hue, lev);
        if (cc_thick && lt > 0) cc_put(x + tx, y + ty, hue, lt);
      }
    }
    if ((x == x1 && y == y1) || ++n > 600) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x += sx; }
    if (e2 < dx) { err += dx; y += sy; }
  }
}

static inline uint32_t cc_fade4(uint32_t x, uint32_t d4) {
  uint32_t h = (x >> 4) & 0x0F0F0F0Fu;
  uint32_t t = (h | 0x80808080u) - d4;
  uint32_t m = ((t & 0x80808080u) >> 7) * 0xFFu;
  return (((t & 0x7F7F7F7Fu) & m) << 4) | (x & 0x0F0F0F0Fu);
}
static void cc_fade(int dec) {
  uint32_t* b = (uint32_t*)cc_buf;
  uint32_t d4 = (uint32_t)dec * 0x01010101u;
  for (int i = 0; i < W * H / 4; i++) b[i] = cc_fade4(b[i], d4);
}

// a rib: a quadratic curve from base (bx,by) through control (cx,cy) to tip (tx,ty),
// all in 1/16 px. Crisp mode: a short polyline. Ghost mode: jittered, adding dots.
static int cc_ribLev = 9, cc_ghost = 0, cc_dotLev = 3;
static void cc_rib(int bx, int by, int cx, int cy, int tx, int ty, int hue, uint32_t id) {
  int len = (abs(tx - bx) + abs(ty - by) + abs(cx - bx) + abs(cy - by)) >> 4;   // rough length, px
  if (cc_ghost) {
    int n = len * 16 / 22 + 1;
    if (n > 90) n = 90;
    int step = 65536 / n;
    for (int k = 1; k <= n; k++) {
      int u = (k * step) >> 8, v = 256 - u;
      int x = (v * v * bx + 2 * u * v * cx + u * u * tx) >> 20;
      int y = (v * v * by + 2 * u * v * cy + u * u * ty) >> 20;
      uint32_t h = cc_hash(id, (uint32_t)k);
      x += (int)(h & 7) - 3; y += (int)((h >> 3) & 7) - 3;
      cc_add(x, y, hue, cc_dotLev * cc_mul >> 8);
    }
    return;
  }
  int ns = 2 + len / 7;                                           // polyline pieces
  if (ns > 8) ns = 8;
  int lev = cc_ribLev * cc_mul >> 8;
  int px = bx >> 4, py = by >> 4;
  cc_dashC = (int)(id & 3);
  for (int k = 1; k <= ns; k++) {
    int u = k * 256 / ns, v = 256 - u;
    int x = (v * v * bx + 2 * u * v * cx + u * u * tx) >> 20;
    int y = (v * v * by + 2 * u * v * cy + u * u * ty) >> 20;
    cc_line(px, py, x, y, hue, lev - (k > ns - 1 ? 1 : 0));     // the very end a touch dimmer
    px = x; py = y;
  }
}
// the tip flare: a short bright dash (2 px wide when big), pointing the way the rib points
static void cc_tip(int tx, int ty, float dx, float dy, int len) {
  int lev = (14 + (len > 3 ? 1 : 0)) * cc_mul >> 8;
  if (lev <= 0) return;
  int x0 = tx >> 4, y0 = ty >> 4;
  int x1 = x0 - (int)(dx * len), y1 = y0 - (int)(dy * len);
  int sd = cc_dash, st = cc_thick;
  cc_dash = 0; cc_thick = len >= 3 ? 1 : 0;
  cc_line(x0, y0, x1, y1, CC_TIP, lev);
  cc_dash = sd; cc_thick = st;
}

// ─── The creature ─────────────────────────────────────────────────────
#define CC_MAXSEG 80
static float cc_px[CC_MAXSEG + 1], cc_py[CC_MAXSEG + 1], cc_pa[CC_MAXSEG + 1];   // spine points (px) and heading
static float cc_tau = 0;                         // 0..1 round the loop
static float cc_beatPh = 0;                      // fast wave phase (radians)
static float cc_bodyPh = 0;                      // slow wave phase (radians)
static float cc_delay = 0.35f, cc_beat = 0.6f, cc_bodyAmp = 0.4f, cc_curl = 0.0f;
static float cc_ribLen = 1.0f, cc_scale = 1.0f, cc_tipTh = 0.8f, cc_tipK = 1.0f;

// lay out a spine of `segs` points with the head facing `head`. The body bends with the
// slow wave (waves = bends down the body) plus a steady curl. anchor 0: the body's
// middle sits at (hx,hy); anchor 1: the head does.
static void cc_spine(int segs, float hx, float hy, float head, float segLen, float waves, float bodySign, float phOff, int anchor) {
  float a = head, x = hx, y = hy;
  float dsn = 1.0f / (float)(segs > 1 ? segs - 1 : 1);
  float wstep = waves * CC_TWO_PI * dsn;
  float amp = bodySign * cc_bodyAmp * 2.2f;
  for (int i = 0; i < segs; i++) {
    float s = i * dsn;
    cc_px[i] = x; cc_py[i] = y; cc_pa[i] = a;
    float bend = amp * cc_cos(cc_bodyPh + phOff - wstep * i) + cc_curl * 1.6f * (0.3f + s);
    a += bend * dsn * 1.7f;
    x -= segLen * cc_cos(a);                                      // the body trails behind the head
    y -= segLen * cc_sin(a);
  }
  if (anchor == 0) {
    int m = segs / 2;
    float ddx = hx - cc_px[m], ddy = hy - cc_py[m];
    for (int i = 0; i < segs; i++) { cc_px[i] += ddx; cc_py[i] += ddy; }
  }
}

// the spine itself: a crisp line through its points
static void cc_spineLine(int segs, int closed) {
  int lev = (cc_ribLev + 1) * cc_mul >> 8;
  int sd = cc_dash; cc_dash = 0;
  for (int i = 1; i < segs; i++) cc_line((int)cc_px[i - 1], (int)cc_py[i - 1], (int)cc_px[i], (int)cc_py[i], CC_SPINE, lev);
  if (closed) cc_line((int)cc_px[segs - 1], (int)cc_py[segs - 1], (int)cc_px[0], (int)cc_py[0], CC_SPINE, lev);
  cc_dash = sd;
}

// ribs along the spine. sides: 1 = one side, 2 = both. fan: bristles per rib (1-3).
// lenProf: 0 = spindle (long middle), 1 = tapering to the tail, 2 = even.
// The beat's cos/sin are stepped down the body by rotation (no trig per rib).
static void cc_ribs(int segs, int sides, int fan, int lenProf, float baseLen, int hueOff, uint32_t idBase, float phOff) {
  float dsn = 1.0f / (float)(segs > 1 ? segs - 1 : 1);
  float c0 = cc_cos(cc_beatPh + phOff), s0 = cc_sin(cc_beatPh + phOff);   // beat phase at the head
  float cd = cc_cos(cc_delay), sd = cc_sin(cc_delay);                      // one rib's lag
  const float fc = 0.9492f, fs = 0.3146f;                                  // fan spread: cos, sin of 0.32
  float Lb = baseLen * cc_ribLen * cc_scale;
  for (int i = 0; i < segs; i++) {
    float s = i * dsn;
    float prof = lenProf == 0 ? cc_sin(CC_PI * (0.06f + 0.94f * s)) : (lenProf == 1 ? 1.0f - 0.75f * s : 1.0f);
    if (lenProf == 0 && prof < 0.25f) prof = 0.25f;
    float L = Lb * (0.35f + 0.65f * prof);
    int hue = cc_clamp(i * 13 / (segs > 1 ? segs - 1 : 1) + hueOff, 0, 13);
    float ca = cc_cos(cc_pa[i]), sa = cc_sin(cc_pa[i]);
    int bx = (int)(cc_px[i] * 16), by = (int)(cc_py[i] * 16);
    for (int side = 0; side < sides; side++) {
      float sg = side == 0 ? 1.0f : -1.0f;
      float cph = side ? -s0 : c0, sph = side ? c0 : s0;            // the far side beats a quarter-turn later
      float sw = cc_beat * cph;                                     // swing angle about the normal
      float csw = cc_cos(sw), ssw = cc_sin(sw);
      float C = ca * csw - sg * sa * ssw, S = sa * csw + sg * ca * ssw;   // cos, sin (heading ± swing)
      float dx = sg > 0 ? -S : S, dy = sg > 0 ? C : -C;            // pointing out from the body
      float bend = -sph * 0.45f * sg;                              // whip: bent back on the recovery stroke
      for (int f = 0; f < fan; f++) {
        float fdx = dx, fdy = dy, LL = L;
        if (fan > 1 && f != (fan - 1) / 2) {
          float fsn = f < (fan - 1) / 2 ? -fs : fs;
          fdx = dx * fc - dy * fsn; fdy = dx * fsn + dy * fc; LL = L * 0.8f;
        }
        int tx = bx + (int)(fdx * LL * 16), ty = by + (int)(fdy * LL * 16);
        int cx = bx + (int)((fdx * 0.5f - fdy * bend) * LL * 16), cy = by + (int)((fdy * 0.5f + fdx * bend) * LL * 16);
        cc_rib(bx, by, cx, cy, tx, ty, hue, idBase + (uint32_t)(i * 8 + side * 4 + f));
        if (cc_tipK > 0 && cph > cc_tipTh) {                         // tip flare where the fast wave peaks
          float e = (cph - cc_tipTh) / (1.0f - cc_tipTh);
          int tl = (int)(1 + e * (2.0f + 5.0f * cc_tipK) * cc_scale);
          cc_tip(tx, ty, fdx, fdy, tl);
        }
      }
    }
    // next rib: rotate the beat phase back by one delay
    float nc = c0 * cd + s0 * sd, ns = s0 * cd - c0 * sd;
    c0 = nc; s0 = ns;
  }
}

// a ring of ribs (Medusa, Radiolarian): n points round an ellipse, ribs out (dir = 1) or in (−1)
static void cc_ring(int n, float cx, float cy, float R, float squash, float rot, int dir, float waves, float lenK, int hueOff, uint32_t id, float phOff) {
  if (n > CC_MAXSEG) n = CC_MAXSEG;
  for (int i = 0; i < n; i++) {
    float a = CC_TWO_PI * i / n + rot;
    cc_px[i] = cx + R * cc_cos(a); cc_py[i] = cy + R * squash * cc_sin(a);
    cc_pa[i] = a - dir * CC_PI * 0.5f;                            // ribs leave along ±the radius
  }
  float savD = cc_delay, savL = cc_ribLen;
  cc_delay = cc_delay < 0.03f ? 0.0f : CC_TWO_PI * waves / n;    // whole waves round the ring: seamless
  cc_ribLen *= lenK;
  cc_ribs(n, 1, 1, 2, 34.0f, hueOff, id, phOff);
  cc_delay = savD; cc_ribLen = savL;
  cc_spineLine(n, 1);
}

// where a swimmer is and which way it faces: a figure-of-eight path, once per loop
static float cc_pathX, cc_pathY, cc_pathA;
static void cc_path(float ph, float cx0, float cy0, float rx, float ry) {
  float a = CC_TWO_PI * cc_tau + ph;
  cc_pathX = cx0 + rx * cc_sin(a); cc_pathY = cy0 + ry * cc_sin(2.0f * a);
  float vx = rx * cc_cos(a), vy = 2.0f * ry * cc_cos(2.0f * a);
  cc_pathA = (fabsf(vx) + fabsf(vy) > 0.5f) ? atan2f(vy, vx) : 0.0f;
}

static void cc_creature(int which, float cx0, float cy0, float swim, float sz, int segs, int knob5, int knob10) {
  switch (which) {
    case 1: {                                                    // Bristleworm: swimming, paddles of bristles
      float segLen = 210.0f * sz / segs;
      cc_path(0.0f, cx0, cy0, 70.0f * swim, 38.0f * swim);
      float ha = swim < 0.05f ? CC_PI * 0.15f * cc_sin(CC_TWO_PI * cc_tau) : cc_pathA;
      float saveAmp = cc_bodyAmp; cc_bodyAmp *= 1.6f;
      cc_spine(segs, cc_pathX, cc_pathY, ha, segLen, 1.5f, 1.0f, 0, 0);
      cc_bodyAmp = saveAmp;
      cc_ribs(segs, 2, 3, 0, 15.0f, 0, 1000, 0);
      cc_spineLine(segs, 0);
      break;
    }
    case 2: {                                                    // Cilia Field: three rows
      float savB = cc_bodyAmp, savC = cc_curl;
      cc_bodyAmp *= 0.25f; cc_curl = 0;
      int n = segs < 24 ? 24 : segs;
      for (int r = 0; r < 3; r++) {
        float y = cy0 + (r - 1) * 66.0f * sz + 14.0f * sz;
        cc_spine(n, cx0, y, CC_PI, 300.0f * sz / n, 1.0f, 1.0f, r * 2.1f, 0);   // facing left: the ribs point up
        cc_ribs(n, 1, 1, 2, 30.0f, r * 2, 2000 + r * 1000, r * 1.3f);
        cc_spineLine(n, 0);
      }
      cc_bodyAmp = savB; cc_curl = savC;
      break;
    }
    case 3: {                                                    // Medusa: pulsing ring, wave runs round
      int n = segs < 20 ? 20 : segs;
      float pulse = 1.0f + 0.18f * (knob5 / 512.0f) * cc_cos(CC_TWO_PI * 3.0f * cc_tau);
      cc_path(0.0f, cx0, cy0, 50.0f * swim, 25.0f * swim);
      float tent = 0.4f + knob10 * 1.6f / 1023.0f;               // tentacle length
      cc_ring(n, cc_pathX, cc_pathY, 46.0f * sz * pulse, 0.8f, CC_TWO_PI * cc_tau, 1, 3.0f, tent, 0, 3000, 0);
      break;
    }
    case 4: {                                                    // Fern: one-sided, curling
      float savC = cc_curl; cc_curl = cc_curl * 0.8f + 0.85f;
      float ha = -CC_PI * 0.5f + 0.4f * cc_sin(CC_TWO_PI * cc_tau) + swim * CC_TWO_PI * cc_tau;
      cc_spine(segs, cx0, cy0, ha + CC_PI, 220.0f * sz / segs, 0.7f, 1.0f, 0, 0);
      cc_ribs(segs, 1, 1, 1, 34.0f, 0, 4000, 0);
      cc_spineLine(segs, 0);
      cc_curl = savC;
      break;
    }
    case 5: {                                                    // Twin Helix: two bodies, opposite waves
      float segLen = 230.0f * sz / segs;
      cc_path(0.0f, cx0, cy0, 60.0f * swim, 30.0f * swim);
      float ha = swim < 0.05f ? 0.25f * cc_sin(CC_TWO_PI * cc_tau) : cc_pathA;
      cc_spine(segs, cc_pathX, cc_pathY, ha, segLen, 2.0f, 0.6f, 0, 0);
      cc_ribs(segs, 2, 1, 0, 20.0f, 0, 5000, 0);
      cc_spineLine(segs, 0);
      cc_spine(segs, cc_pathX, cc_pathY, ha, segLen, 2.0f, -0.6f, 0, 0);
      cc_ribs(segs, 2, 1, 0, 20.0f, 7, 6000, CC_PI);
      cc_spineLine(segs, 0);
      break;
    }
    case 6: {                                                    // Shoal: four small swimmers
      int n = segs / 2 < 10 ? 10 : segs / 2;
      for (int c = 0; c < 4; c++) {
        float rx = (40.0f + 80.0f * swim) * (c & 1 ? 1.0f : 0.8f), ry = (25.0f + 45.0f * swim);
        cc_path(c * CC_PI * 0.5f, cx0, cy0, rx, ry);
        cc_spine(n, cc_pathX, cc_pathY, cc_pathA, 95.0f * sz / n, 1.0f, 1.0f, c * 1.7f, 0);
        cc_ribs(n, 2, 1, 0, 11.0f, (c * 4) % 9, 7000 + c * 500, c * 1.1f);
        cc_spineLine(n, 0);
      }
      break;
    }
    case 7: {                                                    // Ghost: dense misty dots
      cc_path(0.0f, cx0, cy0, 60.0f * swim, 32.0f * swim);
      float ha = swim < 0.05f ? 0.3f * cc_sin(CC_TWO_PI * cc_tau) : cc_pathA;
      cc_spine(segs, cc_pathX, cc_pathY, ha, 230.0f * sz / segs, 1.0f, 1.0f, 0, 0);
      cc_ghost = 1;
      cc_ribs(segs, 2, 2, 0, 26.0f, 0, 8000, 0);
      cc_ghost = 0;
      break;
    }
    case 8: {                                                    // Feather Star: five arms from a centre
      int n = segs / 2 < 10 ? 10 : (segs / 2 > 40 ? 40 : segs / 2);
      float savC = cc_curl, savB = cc_bodyAmp;
      cc_path(0.0f, cx0, cy0, 40.0f * swim, 22.0f * swim);
      float spin = CC_TWO_PI * cc_tau + 0.3f * cc_sin(CC_TWO_PI * 2.0f * cc_tau);
      for (int k = 0; k < 5; k++) {
        float out = spin + CC_TWO_PI * k / 5;
        cc_curl = savC * 0.6f + 0.55f * cc_cos(CC_TWO_PI * cc_tau + k * 1.256f);   // arms curl in and out in turn
        cc_bodyAmp = savB * 0.6f;
        cc_spine(n, cc_pathX + 6.0f * sz * cc_cos(out), cc_pathY + 6.0f * sz * cc_sin(out), out + CC_PI, 92.0f * sz / n, 0.8f, 1.0f, k * 1.256f, 1);
        cc_ribs(n, 2, 1, 1, 14.0f, (k * 3) % 11, 9000 + k * 300, k * 1.256f);
        cc_spineLine(n, 0);
      }
      cc_curl = savC; cc_bodyAmp = savB;
      break;
    }
    case 9: {                                                    // Radiolarian: three rings, opposite waves
      int n = segs < 24 ? 24 : segs;
      cc_path(0.0f, cx0, cy0, 45.0f * swim, 22.0f * swim);
      float pulse = 1.0f + 0.08f * cc_cos(CC_TWO_PI * 2.0f * cc_tau);
      cc_ring(n, cc_pathX, cc_pathY, 62.0f * sz * pulse, 0.92f, CC_TWO_PI * cc_tau, 1, 4.0f, 0.55f, 0, 9500, 0);
      cc_ring(n * 2 / 3, cc_pathX, cc_pathY, 40.0f * sz * pulse, 0.92f, -CC_TWO_PI * cc_tau, -1, -3.0f, 0.42f, 6, 9800, 1.0f);
      cc_ring(n / 3 < 8 ? 8 : n / 3, cc_pathX, cc_pathY, 17.0f * sz * pulse, 0.92f, CC_TWO_PI * 2.0f * cc_tau, 1, 2.0f, 0.38f, 11, 9900, 2.0f);
      break;
    }
    case 10: {                                                   // Kelp: stalks swaying up from the floor
      int n = segs / 2 < 10 ? 10 : (segs / 2 > 40 ? 40 : segs / 2);
      float savB = cc_bodyAmp, savC = cc_curl;
      for (int k = 0; k < 5; k++) {
        float x = cx0 + (k - 2) * 58.0f * sz + 9.0f * cc_sin(k * 2.3f);
        float hgt = (150.0f + 45.0f * cc_sin(k * 1.7f + 0.5f)) * sz;
        cc_bodyAmp = savB * 0.32f;
        cc_curl = savC * 0.25f + 0.06f * cc_sin(CC_TWO_PI * cc_tau + k);
        cc_spine(n, x, (float)H + 4.0f, CC_PI * 0.5f, hgt / n, 0.9f, 1.0f, k * 0.9f, 1);  // rooted at the bottom, growing up
        cc_ribs(n, 2, 1, 0, 17.0f, (k * 3) % 12, 10000 + k * 300, k * 0.8f);
        cc_spineLine(n, 0);
      }
      cc_bodyAmp = savB; cc_curl = savC;
      break;
    }
    default: {                                                   // Centipede
      cc_path(0.0f, cx0, cy0, 60.0f * swim, 32.0f * swim);
      float ha = swim < 0.05f ? 0.3f * cc_sin(CC_TWO_PI * cc_tau) : cc_pathA;
      cc_spine(segs, cc_pathX, cc_pathY, ha, 230.0f * sz / segs, 1.0f, 1.0f, 0, 0);
      cc_ribs(segs, 2, 1, 0, 24.0f, 0, 100, 0);
      cc_spineLine(segs, 0);
      break;
    }
  }
}

// ─── Knobs ────────────────────────────────────────────────────────────
static float cc_sm[16];
static bool cc_smReady = false;
static inline int cc_k(int i) { int v = (int)cc_sm[i]; return v < 0 ? 0 : (v > 1023 ? 1023 : v); }

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_cilia_name() { return "CILIA"; }
const char* prog_cilia_character() { return "Cosine creatures: centipedes, worms, medusae, feather stars and kelp rippling with metachronal waves"; }

static const char* const cc_presetNames[] = {
  "Centipede", "Bristleworm", "Cilia Field", "Medusa", "Fern", "Twin Helix",
  "Shoal", "Ghost", "Feather Star", "Radiolarian", "Kelp", "Morph"
};
#define CC_NUM_PRESETS 12

const char* prog_cilia_presetName(int preset) {
  if (preset >= 0 && preset < CC_NUM_PRESETS) return cc_presetNames[preset];
  return NULL;
}

static const char* const cc_labels[16] = {
  "Palette", "Speed", "Segments", "Wave Delay", "Beat", "Body Wave", "Trails", "Rib Length",
  "Size", "Swim", "Curl", "Lines", "Pan X", "Tips", "Pan Y", "Glow"
};
const char* prog_cilia_potLabel(int preset, int pot) {
  if (preset == 3 && pot == 5) return "Pulse";
  if (preset == 3 && pot == 10) return "Tentacles";
  if (pot >= 0 && pot < 16) return cc_labels[pot];
  return "";
}

uint8_t prog_cilia_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;          // we fade the last frame ourselves (trails)
}

void prog_cilia_init() {
  static bool ready = false;
  if (!ready) {
    ready = true;
    cc_levK[0] = 0;
    for (int l = 1; l < 16; l++) cc_levK[l] = (int16_t)(256.0f * (0.16f + 0.84f * powf(l / 15.0f, 0.85f)));
  }
  cc_lastScheme = -1;
  if (display.getBuffer()) memset(display.getBuffer(), 0, W * H);
}

void prog_cilia_draw(int preset) {
  cc_buf = display.getBuffer();
  if (!cc_buf) return;
  if (preset < 0 || preset >= CC_NUM_PRESETS) preset = 0;

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  {
    float a = dt / 0.12f;
    if (a > 1.0f) a = 1.0f;
    for (int i = 0; i < 16; i++) {
      if (!cc_smReady) cc_sm[i] = pots[i];
      else cc_sm[i] += (pots[i] - cc_sm[i]) * a;
    }
    cc_smReady = true;
  }
  static int lastPreset = -1;
  if (preset != lastPreset) { memset(cc_buf, 0, W * H); lastPreset = preset; }

  // ── the loop clock: everything turns a whole number of times per loop ──
  float sk = cc_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.15f * powf(14.0f, (sk - 0.03f) / 0.97f);   // centre ≈ 0.6×
  const float LOOP = 12.0f;                                      // seconds per loop at 1×
  static int loopCount = 0;
  cc_tau += dt * speed / LOOP;
  if (cc_tau >= 1.0f) { cc_tau -= 1.0f; loopCount++; }

  // ── STARTLE ──
  static bool k12Was = false;
  static float kickT = 9.0f, extraPh = 0;
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !k12Was) kickT = 0;
  k12Was = k12;
  kickT += dt;
  float env = kickT < 0.15f ? kickT / 0.15f : expf(-(kickT - 0.15f) * 1.6f);
  env = env * env * (3.0f - 2.0f * env);
  extraPh += dt * env * 14.0f * (speed > 0 ? 1.0f : 0.0f);
  if (extraPh > CC_TWO_PI * 64) extraPh -= CC_TWO_PI * 64;

  // ── knobs ──
  int ck = cc_k(2);
  int segs = ck < 512 ? 12 + ck * 20 / 512 : 32 + (ck - 512) * 48 / 511;      // 12 … 80, centre 32
  cc_delay = cc_k(3) * 0.70f / 512.0f;                                          // 0 … 1.4 rad, centre 0.35
  cc_beat = (cc_k(4) * 1.2f / 1023.0f) * (1.0f + 0.7f * env);                   // centre 0.6 rad
  cc_bodyAmp = cc_k(5) * 0.8f / 512.0f;                                          // centre 0.8
  int trails = cc_k(6);
  cc_ribLen = 0.3f + cc_k(7) * 1.7f / 1023.0f;                                  // centre 1.15
  cc_scale = powf(2.0f, (cc_k(8) - 512) / 512.0f);                               // 0.5 … 2×
  float swim = cc_k(9) / 1023.0f;
  cc_curl = (cc_k(10) - 512) / 512.0f;
  if (cc_curl > -0.04f && cc_curl < 0.04f) cc_curl = 0;
  int lk = cc_k(11);                                                             // Lines: dotted / crisp / bold
  cc_dash = lk < 300 ? 2 + (300 - lk) * 3 / 300 : 0;
  cc_thick = lk > 720 ? 1 : 0;
  float tk = cc_k(13) / 1023.0f;
  cc_tipK = tk < 0.04f ? 0.0f : tk;
  cc_tipTh = 0.93f - 0.38f * tk - 0.2f * env;                                    // bigger flares when startled
  int gk = cc_k(15);
  cc_ribLev = 6 + gk * 7 / 1024;                                                 // 6 … 12, centre 9
  cc_dotLev = 2 + gk * 4 / 1024;
  float pxk = (cc_k(12) - 512) / 512.0f, pyk = (cc_k(14) - 512) / 512.0f;
  if (pxk > -0.04f && pxk < 0.04f) pxk = 0;
  if (pyk > -0.04f && pyk < 0.04f) pyk = 0;
  float cx0 = HALFW - pxk * 160.0f, cy0 = HALFH + pyk * 120.0f;

  // two waves, whole-number multiples of the loop
  cc_beatPh = CC_TWO_PI * 12.0f * cc_tau + extraPh;               // fast: 12 beats per loop
  cc_bodyPh = CC_TWO_PI * 2.0f * cc_tau;                          // slow: 2 bends per loop

  // ── colours ──
  int sc = cc_k(0) * CC_NSCHEME / 1024;
  if (sc > CC_NSCHEME - 1) sc = CC_NSCHEME - 1;
  cc_buildPalette(sc);

  // ── trails: fade (real-time rate) ──
  {
    static float fadeAcc = 0;
    float tr = 1.0f - trails / 1023.0f;
    fadeAcc += dt * (5.0f + tr * tr * 500.0f);
    int dec = (int)fadeAcc;
    if (dec > 15) dec = 15;
    fadeAcc -= (int)fadeAcc;
    if (dec > 0) cc_fade(dec);
  }

  // ── draw ──
  if (preset == 11) {
    // Morph: a new creature each loop, dissolving in over the last quarter
    static const uint8_t order[10] = {0, 8, 3, 1, 10, 5, 9, 4, 6, 2};
    int a = order[loopCount % 10], b = order[(loopCount + 1) % 10];
    float mix = cc_tau < 0.75f ? 0.0f : (cc_tau - 0.75f) / 0.25f;
    mix = mix * mix * (3.0f - 2.0f * mix);
    cc_mul = (int)(256.0f * (1.0f - mix));
    if (cc_mul > 20) cc_creature(a, cx0, cy0, swim, cc_scale, segs, cc_k(5), cc_k(10));
    cc_mul = (int)(256.0f * mix);
    if (cc_mul > 20) cc_creature(b, cx0, cy0, swim, cc_scale, segs, cc_k(5), cc_k(10));
    cc_mul = 256;
  } else {
    cc_mul = 256;
    cc_creature(preset, cx0, cy0, swim, cc_scale, segs, cc_k(5), cc_k(10));
  }
}
