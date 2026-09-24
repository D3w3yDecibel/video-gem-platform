// =====================================================================
// PROGRAM: CRAWLER (by Dewey)
// Little segmented creatures crawling through a field of circles.
// Inspired by the look of rybinfx's "Keep moving ~ anisotropic damping":
// thin circle outlines (merged where they overlap), and a wiggling chain
// of outlined segments with a dot in each, finding its way between them.
//
// How the creature moves ("anisotropic damping"): its head wiggles
// forward, and every body segment slides easily ALONG the body but hardly
// at all SIDEWAYS — like a snake's scales gripping the ground. That turns
// wiggling into crawling. Segments are linked by fixed distances, and get
// pushed out of any circle they bump into.
//
// Presets:
//   k0  Crawler   — one creature exploring (the camera follows it)
//   k1  Colony    — a few creatures of different sizes
//   k2  Serpent   — one very long snake
//   k3  Drift     — the bubbles float about on their own too
//   k4  Spotlight — darkness; only circles near the creature light up
//   k5  Lanterns  — circles light up when touched, then fade
//   k6  Centipede — a creature with rippling legs
//   k7  Break     — a pool rack to smash (see below)
//
//   k12 — press for a new field of circles
//
// This program OWNS the global knobs (it makes its own colors):
//   p0 Colors — creature colors: ember (red→purple), gold, ice, moss, rainbow
//   p1 Speed (far left = stop; right = fast)
//   p2 Circles (brightness: left = invisible)    p3 Wiggle (how wide)
// Camera:
//   p8 Zoom (center = normal)    p9 Follow (left = loose, right = tight)
//   p10 Wiggle Rate (how fast it undulates; center = natural)
//   p11 Friction (left = bubbles glide a long way, right = stop quickly)
//   p12 Power (how hard the creature hits — it's the cue ball)
//   p13 Bubble Color (far left = white, then around the colour wheel,
//                     far right = rainbow: every bubble its own colour)
//
// POOL PHYSICS: the creature hunts the nearest resting bubble and hits
// it like a cue ball; bubbles roll, bounce off each other (momentum
// passes through a cluster like a pool break), off the world's edge,
// flash and wobble when hit, and slowly roll to a stop.
//   k7  Break — a triangle rack of equal bubbles; re-racks itself when
//               everything has stopped (or press k12)
//
// Memory-slim version: fits alongside the other programs (~5 KB of RAM).
// =====================================================================

#define CR_MAXC   96        // circles (bubbles)
#define CR_MAXCR  4         // creatures
#define CR_MAXSEG 32        // segments per creature

// Palette layout
#define CR_GLOW0  64        // 64–95   lit circle outline (dim → bright)
#define CR_GLOWN  32
#define CR_BODY0  100       // 100–131 body gradient, head → tail
#define CR_BODYN  32
#define CR_DOT    140       // segment dots
#define CR_LEG    141       // legs
#define CR_RING   200       // circle outlines (200–215 in Rainbow bubbles mode)
#define CR_RINGN  16

// ─── Smooth knobs ─────────────────────────────────────────────────────
static float cr_sm[16];
static bool  cr_smReady = false;

static void cr_smoothKnobs(float dt) {
  float a = dt / 0.12f;
  if (a > 1.0f) a = 1.0f;
  for (int i = 0; i < 16; i++) {
    if (!cr_smReady) cr_sm[i] = pots[i];
    else cr_sm[i] += (pots[i] - cr_sm[i]) * a;
  }
  cr_smReady = true;
}
static inline float cr_potf(int idx, float lo, float hi) { return lo + (hi - lo) * (cr_sm[idx] / 1023.0f); }
static inline int cr_pot(int idx, int lo, int hi) {
  int v = (int)floorf(lo + (hi - lo + 1) * (cr_sm[idx] / 1024.0f));
  return v < lo ? lo : (v > hi ? hi : v);
}

// ─── Colors ───────────────────────────────────────────────────────────
static int cr_palScheme = -1, cr_palRing = -1, cr_palHue = -1;
static bool cr_rainbowRings = false;

// Hue 0–1 → fully saturated colour
static void cr_hue(float h, uint8_t* out) {
  h -= floorf(h);
  float x = h * 6.0f; int k = (int)x; float q = x - k;
  float r, g, b;
  switch (k) {
    case 0:  r = 1; g = q; b = 0; break;
    case 1:  r = 1 - q; g = 1; b = 0; break;
    case 2:  r = 0; g = 1; b = q; break;
    case 3:  r = 0; g = 1 - q; b = 1; break;
    case 4:  r = q; g = 0; b = 1; break;
    default: r = 1; g = 0; b = 1 - q; break;
  }
  out[0] = (uint8_t)(r * 255); out[1] = (uint8_t)(g * 255); out[2] = (uint8_t)(b * 255);
}

static void cr_mix(const uint8_t* a, const uint8_t* b, float t, uint8_t* out) {
  for (int i = 0; i < 3; i++) out[i] = (uint8_t)(a[i] + (b[i] - a[i]) * t);
}

// hueKey (p13): 0 = white, 1–60 = one colour around the hue wheel,
// 61 = rainbow (every bubble its own colour)
static void cr_buildPalette(int scheme, int ringLvl, int hueKey) {
  if (scheme == cr_palScheme && ringLvl == cr_palRing && hueKey == cr_palHue) return;
  cr_palScheme = scheme; cr_palRing = ringLvl; cr_palHue = hueKey;
  cr_rainbowRings = (hueKey >= 61);
  // Body gradient: head, middle, tail  |  dot color  |  accent (trails, lit circles)
  static const uint8_t S[5][15] = {
    {235, 50, 70,   210, 40, 150,  130, 70, 220,   150, 230, 110,   255, 90, 140},   // ember
    {255, 200, 60,  255, 130, 30,  200, 60, 30,    255, 255, 200,   255, 190, 80},   // gold
    {120, 230, 255,  60, 140, 255,  120, 80, 255,  255, 255, 255,   110, 200, 255},  // ice
    {170, 240, 90,   60, 200, 140,  30, 130, 150,  255, 240, 120,   150, 240, 120},  // moss
    {255, 60, 60,    60, 255, 120,  90, 90, 255,   255, 255, 255,   255, 255, 255},  // rainbow
  };
  const uint8_t* s = S[scheme];
  uint8_t c[3];
  for (int i = 0; i < 256; i++) display.setColor(i, 0, 0, 0);
  // Body gradient in two halves: head→middle, middle→tail
  for (int i = 0; i < CR_BODYN; i++) {
    float t = i / (float)(CR_BODYN - 1);
    if (scheme == 4) {                                     // rainbow: sweep the hues
      float h = t * 5.0f; int k = (int)h; float q = h - k;
      float r[6] = {1, 1 - q, 0, 0, q, 1}, g[6] = {q, 1, 1, 1 - q, 0, 0}, b[6] = {0, 0, q, 1, 1, 1 - q};
      k = k > 5 ? 5 : k;
      display.setColor(CR_BODY0 + i, r[k] * 255, g[k] * 255, b[k] * 255);
      continue;
    }
    if (t < 0.5f) cr_mix(s, s + 3, t * 2, c); else cr_mix(s + 3, s + 6, (t - 0.5f) * 2, c);
    display.setColor(CR_BODY0 + i, c[0], c[1], c[2]);
  }
  display.setColor(CR_DOT, s[9], s[10], s[11]);
  display.setColor(CR_LEG, s[3] / 2 + 40, s[4] / 2 + 40, s[5] / 2 + 40);
  // Bubble outline colour (p13) at brightness ringLvl (p2)
  uint8_t ring[3] = {(uint8_t)ringLvl, (uint8_t)ringLvl, (uint8_t)ringLvl};
  uint8_t flash[3] = {s[12], s[13], s[14]};              // hit flash: creature's accent…
  if (hueKey >= 1 && hueKey <= 60) {
    uint8_t h[3];
    cr_hue((hueKey - 1) / 60.0f, h);
    for (int i = 0; i < 3; i++) {
      ring[i] = (uint8_t)(h[i] * ringLvl / 200);
      flash[i] = (uint8_t)((h[i] + 255) / 2);            // …or a bright tint of the bubble colour
    }
  }
  // Lit / hit circles: outline colour → flash colour
  for (int i = 0; i < CR_GLOWN; i++) {
    cr_mix(ring, flash, (i + 1) / (float)CR_GLOWN, c);
    display.setColor(CR_GLOW0 + i, c[0], c[1], c[2]);
  }
  if (cr_rainbowRings) {
    for (int i = 0; i < CR_RINGN; i++) {
      uint8_t h[3];
      cr_hue(i / (float)CR_RINGN, h);
      display.setColor(CR_RING + i, h[0] * ringLvl / 200, h[1] * ringLvl / 200, h[2] * ringLvl / 200);
    }
  } else {
    display.setColor(CR_RING, ring[0], ring[1], ring[2]);
  }
  display.setColor(255, 255, 255, 255);
}

// ─── The world: circles ───────────────────────────────────────────────
static float cr_WW = 960, cr_WH = 720;         // world size (pixels at zoom 1)
static float   cr_cx[CR_MAXC], cr_cy[CR_MAXC], cr_cr[CR_MAXC];   // circle centre and radius
static uint8_t cr_ord[CR_MAXC];                // bubbles sorted left→right (fast collisions)
static uint8_t cr_glow[CR_MAXC];               // flash when hit (Lanterns: long glow) 0–255
static int16_t cr_vx[CR_MAXC], cr_vy[CR_MAXC]; // bubble velocity (1/8 px per second)
static uint8_t cr_wob[CR_MAXC];                // bubble wobble after a hit, 0–255
static float   cr_push = 0.9f;                 // how much of an overlap moves the bubble (vs the creature)
static float   cr_power = 2.0f;                // cue power (p12)
static float   cr_dtStep = 0.008f;             // current physics step
static float   cr_rest = 0.9f;                 // bounciness of bubble hits
static float   cr_svx = 0, cr_svy = 0;         // velocity of the snake point being collided (px/s)

// px/s → stored int16 (1/8 px/s), limited to ±600 px/s
static inline int16_t cr_v16(float v) {
  if (v > 600.0f) v = 600.0f; else if (v < -600.0f) v = -600.0f;
  return (int16_t)(v * 8.0f);
}
static int   cr_nc = 0;

static float cr_rand01() { return random(0, 10000) / 10000.0f; }

static void cr_makeCircles(int count, float rMin, float rMax) {
  if (count > CR_MAXC) count = CR_MAXC;
  cr_nc = 0;
  int tries = 0;
  while (cr_nc < count && tries < 4000) {
    tries++;
    float r = rMin + (rMax - rMin) * cr_rand01();
    float x = r + (cr_WW - 2 * r) * cr_rand01();
    float y = r + (cr_WH - 2 * r) * cr_rand01();
    // Like balls on a table: no overlaps
    bool ok = true;
    for (int j = 0; j < cr_nc && ok; j++) {
      float dx = x - cr_cx[j], dy = y - cr_cy[j];
      if (dx * dx + dy * dy < (r + cr_cr[j] + 3) * (r + cr_cr[j] + 3)) ok = false;
    }
    if (!ok) continue;
    cr_cx[cr_nc] = x; cr_cy[cr_nc] = y;
    cr_cr[cr_nc] = r;
    cr_glow[cr_nc] = 0; cr_wob[cr_nc] = 0;
    cr_vx[cr_nc] = cr_vy[cr_nc] = 0;
    cr_nc++;
  }
  for (int j = 0; j < cr_nc; j++) cr_ord[j] = (uint8_t)j;
}

// Break: a triangle rack of equal bubbles, apex pointing left
static void cr_makeRack(int rows, float r) {
  cr_nc = 0;
  float gap = r * 2.0f + 1.0f;
  float ax = cr_WW * 0.58f, ay = cr_WH * 0.5f;
  for (int row = 0; row < rows; row++)
    for (int i = 0; i <= row && cr_nc < CR_MAXC; i++) {
      cr_cx[cr_nc] = ax + row * gap * 0.8660f;
      cr_cy[cr_nc] = ay + (i - row * 0.5f) * gap;
      cr_cr[cr_nc] = r;
      cr_glow[cr_nc] = 0; cr_wob[cr_nc] = 0;
      cr_vx[cr_nc] = cr_vy[cr_nc] = 0;
      cr_nc++;
    }
  for (int j = 0; j < cr_nc; j++) cr_ord[j] = (uint8_t)j;
}

// ─── Creatures ────────────────────────────────────────────────────────
// Each is a chain of points. Verlet physics: we keep this frame's and last
// frame's position; the difference is the velocity.
static float cr_px[CR_MAXCR][CR_MAXSEG], cr_py[CR_MAXCR][CR_MAXSEG];
static float cr_qx[CR_MAXCR][CR_MAXSEG], cr_qy[CR_MAXCR][CR_MAXSEG];
static float cr_head[CR_MAXCR];                // heading angle
static float cr_phase[CR_MAXCR];               // wiggle phase
static float cr_turn[CR_MAXCR];                // wandering turn rate
static int   cr_seg[CR_MAXCR];                 // segments
static float cr_len[CR_MAXCR];                 // distance between segments
static float cr_wid[CR_MAXCR];                 // body half-width
static int   cr_n = 1;                         // creatures in use

static void cr_spawn(int k, int segs, float len, float wid) {
  // Find an open spot
  float x = cr_WW / 2, y = cr_WH / 2;
  for (int t = 0; t < 200; t++) {
    x = 40 + (cr_WW - 80) * cr_rand01();
    y = 40 + (cr_WH - 80) * cr_rand01();
    bool free = true;
    for (int j = 0; j < cr_nc && free; j++) {
      float dx = x - cr_cx[j], dy = y - cr_cy[j];
      if (dx * dx + dy * dy < (cr_cr[j] + 30) * (cr_cr[j] + 30)) free = false;
    }
    if (free) break;
  }
  cr_seg[k] = segs; cr_len[k] = len; cr_wid[k] = wid;
  cr_head[k] = cr_rand01() * TWO_PI;
  cr_phase[k] = cr_rand01() * TWO_PI;
  cr_turn[k] = 0;
  for (int i = 0; i < segs; i++) {
    cr_px[k][i] = cr_qx[k][i] = x - cosf(cr_head[k]) * len * i;
    cr_py[k][i] = cr_qy[k][i] = y - sinf(cr_head[k]) * len * i;
  }
}

// Only the circles near the creature are checked (much faster with lots
// of bubbles). cr_near() fills this list for a box around the creature.
static uint8_t cr_cand[CR_MAXC];
static int     cr_ncand = 0;

static void cr_near(float x0, float y0, float x1, float y1) {
  cr_ncand = 0;
  for (int j = 0; j < cr_nc; j++) {
    float r = cr_cr[j];
    if (cr_cx[j] + r < x0 || cr_cx[j] - r > x1 || cr_cy[j] + r < y0 || cr_cy[j] - r > y1) continue;
    cr_cand[cr_ncand++] = (uint8_t)j;
  }
}

// Push a point out of any nearby circle (and keep it inside the world)
static bool cr_collide(float& x, float& y, float pad) {
  bool hit = false;
  for (int m = 0; m < cr_ncand; m++) {
    int j = cr_cand[m];
    float dx = x - cr_cx[j], dy = y - cr_cy[j];
    float rr = cr_cr[j] + pad;
    float d2 = dx * dx + dy * dy;
    if (d2 < rr * rr && d2 > 0.0001f) {
      float d = sqrtf(d2);
      float nx = dx / d, ny = dy / d, pen = rr - d;         // n points from bubble to snake
      // Shove the bubble out of the way (part of the overlap)
      float sh = pen * cr_push;
      cr_cx[j] -= nx * sh; cr_cy[j] -= ny * sh;
      // Bounce: reflect the bubble's velocity off the snake (the snake is
      // much heavier). Only if they're moving toward each other.
      float bvx = cr_vx[j] * 0.125f, bvy = cr_vy[j] * 0.125f;
      float vn;
      // The creature's speed is multiplied by Power: it's the cue ball.
      vn = (bvx - cr_svx * cr_power) * nx + (bvy - cr_svy * cr_power) * ny;
      if (vn > 0) {
        float imp = (1.0f + cr_rest) * vn;
        bvx -= imp * nx; bvy -= imp * ny;
      }
      // A little extra kick from overlap, so even a slow snake nudges bubbles
      float kick = sh * 0.2f / cr_dtStep;
      if (kick > 300.0f) kick = 300.0f;
      bvx -= nx * kick; bvy -= ny * kick;
      cr_vx[j] = cr_v16(bvx); cr_vy[j] = cr_v16(bvy);
      // The creature slides round the (moved) bubble
      x = cr_cx[j] + nx * rr;
      y = cr_cy[j] + ny * rr;
      cr_glow[j] = 255;                                     // flash
      int w = cr_wob[j] + (int)(pen * 50.0f) + 20;
      cr_wob[j] = (uint8_t)(w > 255 ? 255 : w);             // wobble
      hit = true;
    }
  }
  if (x < pad) x = pad; else if (x > cr_WW - pad) x = cr_WW - pad;
  if (y < pad) y = pad; else if (y > cr_WH - pad) y = cr_WH - pad;
  return hit;
}

// One physics step for creature k
static int8_t cr_tgt[CR_MAXCR];                // bubble each creature is hunting (-1 = none)
static float  cr_aim = 0.5f;                   // p6: 0 = wander aimlessly, 1 = beeline for bubbles

static void cr_update(int k, float dt, float speed, float wiggle, float wander, float wigRate) {
  cr_dtStep = dt;
  int n = cr_seg[k];
  float L = cr_len[k];

  // Which circles are close enough to matter this step?
  float bx0 = cr_px[k][0], bx1 = bx0, by0 = cr_py[k][0], by1 = by0;
  for (int i = 1; i < n; i++) {
    float x = cr_px[k][i], y = cr_py[k][i];
    if (x < bx0) bx0 = x; else if (x > bx1) bx1 = x;
    if (y < by0) by0 = y; else if (y > by1) by1 = y;
  }
  float m = L * 3.5f + cr_wid[k] + speed * dt + 4.0f;     // look-ahead + body + this step's move
  cr_near(bx0 - m, by0 - m, bx1 + m, by1 + m);

  // Steering: wander a little, and turn toward the bubble being hunted
  cr_turn[k] += (cr_rand01() - 0.5f) * wander * dt * 6.0f;
  cr_turn[k] *= 0.97f;
  float hx = cr_px[k][0], hy = cr_py[k][0];
  int tg = cr_tgt[k];
  if (tg >= 0 && tg < cr_nc) {
    float want = atan2f(cr_cy[tg] - hy, cr_cx[tg] - hx);
    float diff = want - cr_head[k];
    while (diff > PI) diff -= TWO_PI;
    while (diff < -PI) diff += TWO_PI;
    cr_head[k] += diff * cr_aim * 4.0f * dt;
  }
  // Near the edge of the world? Turn back toward the middle.
  float edge = 70.0f;
  if (hx < edge || hy < edge || hx > cr_WW - edge || hy > cr_WH - edge) {
    float want = atan2f(cr_WH * 0.5f - hy, cr_WW * 0.5f - hx);
    float diff = want - cr_head[k];
    while (diff > PI) diff -= TWO_PI;
    while (diff < -PI) diff += TWO_PI;
    cr_head[k] += diff * 1.5f * dt;
  }
  cr_head[k] += cr_turn[k] * dt;
  if (cr_head[k] > PI) cr_head[k] -= TWO_PI; else if (cr_head[k] < -PI) cr_head[k] += TWO_PI;

  // The head wiggles side to side as it moves forward (slow, wide swings
  // read as crawling; the body follows the S-shaped path)
  float sp = speed > 150.0f ? 150.0f : speed;
  cr_phase[k] += dt * (1.2f + sp * 0.03f) * wigRate;
  float wig = wiggle * sinf(cr_phase[k]);
  float mv = speed * dt;
  float a = cr_head[k] + wig;
  cr_qx[k][0] = cr_px[k][0]; cr_qy[k][0] = cr_py[k][0];
  cr_px[k][0] += cosf(a) * mv;
  cr_py[k][0] += sinf(a) * mv;
  cr_svx = cosf(a) * speed; cr_svy = sinf(a) * speed;
  cr_collide(cr_px[k][0], cr_py[k][0], cr_wid[k]);

  // Body: Verlet with ANISOTROPIC damping —
  // keep most of the velocity along the body, lose most sideways.
  for (int i = 1; i < n; i++) {
    float vx = cr_px[k][i] - cr_qx[k][i], vy = cr_py[k][i] - cr_qy[k][i];
    float tx = cr_px[k][i - 1] - cr_px[k][i], ty = cr_py[k][i - 1] - cr_py[k][i];
    float tl = sqrtf(tx * tx + ty * ty);
    if (tl > 0.0001f) { tx /= tl; ty /= tl; }
    float along = vx * tx + vy * ty;
    float px = along * tx, py = along * ty;                  // along the body
    float sx = vx - px, sy = vy - py;                        // sideways
    vx = px * 0.92f + sx * 0.35f;
    vy = py * 0.92f + sy * 0.35f;
    cr_qx[k][i] = cr_px[k][i]; cr_qy[k][i] = cr_py[k][i];
    cr_px[k][i] += vx; cr_py[k][i] += vy;
  }
  // Keep segments at their set distance (a few passes = stiffer)
  for (int it = 0; it < 3; it++) {
    for (int i = 1; i < n; i++) {
      float dx = cr_px[k][i] - cr_px[k][i - 1], dy = cr_py[k][i] - cr_py[k][i - 1];
      float d = sqrtf(dx * dx + dy * dy);
      if (d < 0.0001f) continue;
      float corr = (d - L) / d;
      cr_px[k][i] -= dx * corr;                              // head leads: move only the follower
      cr_py[k][i] -= dy * corr;
    }
    float inv = 1.0f / dt;
    for (int i = 1; i < n; i++) {
      cr_svx = (cr_px[k][i] - cr_qx[k][i]) * inv; cr_svy = (cr_py[k][i] - cr_qy[k][i]) * inv;
      cr_collide(cr_px[k][i], cr_py[k][i], cr_wid[k] * 0.8f);
    }
  }
}

// ─── Camera ───────────────────────────────────────────────────────────
static float cr_camX, cr_camY, cr_zoom = 1;

static inline float cr_sx(float x) { return HALFW + (x - cr_camX) * cr_zoom; }
static inline float cr_sy(float y) { return HALFH + (y - cr_camY) * cr_zoom; }

// ─── Drawing ──────────────────────────────────────────────────────────
static uint8_t* cr_buf;

static inline void cr_px_(int x, int y, uint8_t c) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H) return;
  cr_buf[y * W + x] = c;
}

// Circle outlines, merged: an outline point is skipped if it lies inside
// a neighbouring circle — so overlapping circles show one combined edge.
static float cr_t = 0;                         // running time (seconds)

static void cr_drawCircles(bool lit, bool onlyLit) {
  // Screen positions (small ints, kept static so they don't sit on the stack)
  static int16_t sxs[CR_MAXC], sys[CR_MAXC], rs[CR_MAXC];
  for (int i = 0; i < cr_nc; i++) {
    sxs[i] = (int16_t)cr_sx(cr_cx[i]); sys[i] = (int16_t)cr_sy(cr_cy[i]);
    float r = cr_cr[i];
    if (cr_wob[i]) r *= 1.0f + 0.16f * (cr_wob[i] / 255.0f) * sinf(cr_t * 17.0f + i * 1.3f);
    rs[i] = (int16_t)(r * cr_zoom + 0.5f);
  }
  for (int i = 0; i < cr_nc; i++) {
    int cx = sxs[i], cy = sys[i], r = rs[i];
    if (cx + r < 0 || cx - r >= W || cy + r < 0 || cy - r >= H) continue;
    uint8_t col = cr_rainbowRings ? CR_RING + (i * 7) % CR_RINGN : CR_RING;
    if (lit) {
      if (cr_glow[i] > 5) col = CR_GLOW0 + (cr_glow[i] * (CR_GLOWN - 1)) / 255;
      else if (onlyLit) continue;
    }
    // Neighbours that overlap this circle (their insides hide our outline)
    uint8_t nb[10]; int nbn = 0;
    for (int j = 0; j < cr_nc && nbn < 10; j++) {
      if (j == i) continue;
      float dx = cr_cx[i] - cr_cx[j], dy = cr_cy[i] - cr_cy[j], rr = cr_cr[i] + cr_cr[j];
      if (dx * dx + dy * dy < rr * rr) nb[nbn++] = (uint8_t)j;
    }
    // Midpoint circle: one eighth computed, eight mirrored points drawn
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
      for (int p = 0; p < 8; p++) {
        int X = cx + ((p & 4) ? ((p & 1) ? -y : y) : ((p & 1) ? -x : x));
        int Y = cy + ((p & 4) ? ((p & 2) ? -x : x) : ((p & 2) ? -y : y));
        if ((unsigned)X >= (unsigned)W || (unsigned)Y >= (unsigned)H) continue;
        bool hidden = false;
        for (int m = 0; m < nbn && !hidden; m++) {
          int j = nb[m];
          int dx = X - sxs[j], dy = Y - sys[j], rj = rs[j] - 1;
          if (dx * dx + dy * dy < rj * rj) hidden = true;
        }
        if (!hidden) cr_buf[Y * W + X] = col;
      }
      y++;
      if (err < 0) err += 2 * y + 1; else { x--; err += 2 * (y - x) + 1; }
    }
  }
}

static void cr_line(int x0, int y0, int x1, int y1, uint8_t c) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (int g = 0; g < 600; g++) {
    cr_px_(x0, y0, c);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

// The creature: each segment an outlined box along the body, a dot in it.
// legs = 0 for none; otherwise leg length (Centipede).
static void cr_drawCreature(int k, float legs, float legPhase) {
  int n = cr_seg[k];
  for (int i = n - 2; i >= 0; i--) {                 // tail first, so the head is on top
    float ax = cr_px[k][i], ay = cr_py[k][i], bx = cr_px[k][i + 1], by = cr_py[k][i + 1];
    float mx = (ax + bx) * 0.5f, my = (ay + by) * 0.5f;
    float dx = ax - bx, dy = ay - by;
    float dl = sqrtf(dx * dx + dy * dy);
    if (dl < 0.0001f) continue;
    dx /= dl; dy /= dl;
    float nx = -dy, ny = dx;
    float f = i / (float)(n - 1);
    float hw = cr_wid[k] * (1.0f - 0.45f * f) * cr_zoom;          // tapers toward the tail
    float hl = cr_len[k] * 0.46f * cr_zoom;
    float sx = cr_sx(mx), sy = cr_sy(my);
    int c1x = (int)(sx + dx * hl + nx * hw), c1y = (int)(sy + dy * hl + ny * hw);
    int c2x = (int)(sx + dx * hl - nx * hw), c2y = (int)(sy + dy * hl - ny * hw);
    int c3x = (int)(sx - dx * hl - nx * hw), c3y = (int)(sy - dy * hl - ny * hw);
    int c4x = (int)(sx - dx * hl + nx * hw), c4y = (int)(sy - dy * hl + ny * hw);
    uint8_t col = CR_BODY0 + (int)(f * (CR_BODYN - 1));
    if (legs > 0) {                                   // rippling legs on both sides
      float sw = sinf(legPhase - i * 0.9f) * 0.8f;
      for (int sd = -1; sd <= 1; sd += 2) {
        float lx = nx * sd * (hw + legs * cr_zoom) + dx * sw * legs * cr_zoom * sd;
        float ly = ny * sd * (hw + legs * cr_zoom) + dy * sw * legs * cr_zoom * sd;
        cr_line((int)(sx + nx * hw * sd), (int)(sy + ny * hw * sd), (int)(sx + lx), (int)(sy + ly), CR_LEG);
      }
    }
    cr_line(c1x, c1y, c2x, c2y, col);
    cr_line(c2x, c2y, c3x, c3y, col);
    cr_line(c3x, c3y, c4x, c4y, col);
    cr_line(c4x, c4y, c1x, c1y, col);
    int d = (hw > 3.5f) ? 1 : 0;                      // dot: 2×2 (or 1 px when tiny)
    for (int yy = 0; yy <= d; yy++) for (int xx = 0; xx <= d; xx++) cr_px_((int)sx + xx, (int)sy + yy, CR_DOT);
  }
}

// ─── Main ─────────────────────────────────────────────────────────────
static bool cr_needWorld = true;
static bool cr_broken = false;                 // Break: has the rack been hit yet?
static int  cr_lastDensity = -1;

const char* prog_crawl_name() { return "CRAWLER"; }

const char* prog_crawl_character() {
  return "Wiggling creatures crawling through a field of circles";
}

static const char* const cr_presetNames[] = {
  "Crawler", "Colony", "Serpent", "Drift", "Spotlight", "Lanterns", "Centipede", "Break"
};
#define CR_NUM_PRESETS 8

const char* prog_crawl_presetName(int preset) {
  if (preset >= 0 && preset < CR_NUM_PRESETS) return cr_presetNames[preset];
  return NULL;
}

static const char* const cr_globalLabels[4] = { "Colors", "Speed", "Circles", "Wiggle" };
static const char* const cr_potLabels[CR_NUM_PRESETS][4] = {
  {"Length",  "Thickness", "Aim",    "Density"},   // Crawler
  {"Count",   "Size",      "Aim",    "Density"},   // Colony
  {"Length",  "Thickness", "Aim",    "Density"},   // Serpent
  {"Length",  "Float",     "Aim",    "Density"},   // Drift
  {"Length",  "Light",     "Aim",    "Density"},   // Spotlight
  {"Length",  "Fade",      "Aim",    "Density"},   // Lanterns
  {"Length",  "Legs",      "Aim",    "Density"},   // Centipede
  {"Length",  "Thickness", "Aim",    "Rack Size"}, // Break
};

const char* prog_crawl_potLabel(int preset, int pot) {
  if (pot >= 0 && pot < 4) return cr_globalLabels[pot];
  if (pot == 8) return "Zoom";
  if (pot == 9) return "Follow";
  if (pot == 10) return "Wiggle Rate";
  if (pot == 11) return "Friction";
  if (pot == 12) return "Power";
  if (pot == 13) return "Bubble Color";
  if (pot < 4 || pot > 7) return "";
  if (preset < 0 || preset >= CR_NUM_PRESETS) return "";
  return cr_potLabels[preset][pot - 4];
}

uint8_t prog_crawl_renderHint(int preset) {
  (void)preset;
  return RENDER_CLEAR;
}

void prog_crawl_init() {
  cr_palScheme = -1;
  cr_needWorld = true;
  display.fillScreen(0);
}

void prog_crawl_draw(int preset) {
  cr_buf = display.getBuffer();

  // Own clock (this program owns p1)
  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.05f) dt = 0.016f;
  cr_smoothKnobs(dt);

  // Global knobs
  int scheme  = cr_pot(0, 0, 4);
  // p1 Speed: far left = stopped, then 6 → 300 px per second (centre ≈ 40)
  float sk = cr_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 6.0f * powf(50.0f, (sk - 0.03f) / 0.97f);
  // p10 Wiggle rate: how fast it undulates (centre = matched to speed)
  float wigRate = powf(3.0f, cr_potf(10, -1.0f, 1.0f));
  int ringLvl = (int)cr_potf(2, 0, 200);
  float wiggle = cr_potf(3, 0.0f, 1.3f);
  // p13 Bubble colour: far left = white, then around the colour wheel,
  // far right = rainbow (each bubble different)
  float hk = cr_sm[13] / 1023.0f;
  int hueKey = hk < 0.05f ? 0 : (hk > 0.93f ? 61 : 1 + (int)((hk - 0.05f) / 0.88f * 59.99f));
  cr_buildPalette(scheme, preset == 4 ? 0 : ringLvl, hueKey);   // Spotlight: dark → lit

  // Per-preset knobs (p4–p7)
  float wander = 0.3f + 2.5f * (1.0f - cr_sm[6] / 1023.0f);   // p6 Aim: left = wander, right = hunt
  int density  = cr_pot(7, 0, 4);

  // New preset, new density, or k12? Build a new world.
  static int lastPreset = -1;
  static bool k12Was = false;
  bool k12 = keysPressed[KEY_MOD_A];
  if (preset != lastPreset || density != cr_lastDensity || (k12 && !k12Was)) cr_needWorld = true;
  k12Was = k12;

  if (cr_needWorld) {
    cr_needWorld = false;
    lastPreset = preset;
    cr_lastDensity = density;
    cr_WW = 960; cr_WH = 720;
    static const uint8_t counts[5] = {30, 45, 60, 75, 90};
    if (preset == 7) cr_makeRack(5 + density, 16.0f);         // 15 … 55 balls
    else cr_makeCircles(counts[density] + 6, preset == 2 ? 12 : 14, preset == 2 ? 32 : 40);
    for (int k = 0; k < CR_MAXCR; k++) cr_tgt[k] = -1;
    cr_broken = false;
    for (int j = 0; j < cr_nc; j++) cr_glow[j] = 0;
    // Creatures for this preset
    int segs = cr_pot(4, 8, 24);
    float thick = cr_potf(5, 3.0f, 8.0f);
    switch (preset) {
      case 1: cr_n = cr_pot(4, 2, CR_MAXCR);
              for (int k = 0; k < cr_n; k++) {
                float sz = cr_potf(5, 0.6f, 1.4f) * (0.6f + 0.25f * (k % 3));
                cr_spawn(k, 10 + (k * 5) % 12, 8.0f * sz, 5.0f * sz);
              }
              break;
      case 2: cr_n = 1; cr_spawn(0, cr_pot(4, 24, CR_MAXSEG), 7.0f, thick * 0.8f); break;
      case 7: cr_n = 1; cr_spawn(0, segs, 9.0f, thick);         // the cue: left of the rack, aimed at it
              for (int i = 0; i < cr_seg[0]; i++) {
                cr_px[0][i] = cr_qx[0][i] = cr_WW * 0.4f - 9.0f * i;
                cr_py[0][i] = cr_qy[0][i] = cr_WH * 0.5f;
              }
              cr_head[0] = 0; cr_tgt[0] = 0;
              break;
      default: cr_n = 1; cr_spawn(0, segs, 9.0f, thick); break;
    }
    cr_camX = cr_px[0][0]; cr_camY = cr_py[0][0];
  }

  // Bubbles roll like pool balls: friction slows them to a stop; they
  // bounce off the world's edge (the cushions) and off each other.
  cr_t += dt;
  float fric = 0.12f * powf(20.0f, cr_sm[11] / 1023.0f);    // p11: 0.12 … 2.4 per second
  cr_power = 0.8f * powf(8.0f, cr_sm[12] / 1023.0f);        // p12: 0.8× … 6.4× the creature's speed
  cr_aim = cr_potf(6, 0.0f, 1.0f);
  float drift = (preset == 3) ? cr_potf(5, 0.0f, 50.0f) : 0.0f;
  float keep = 1.0f - fric * dt;
  if (keep < 0) keep = 0;
  bool anyMoving = false;
  for (int j = 0; j < cr_nc; j++) {
    float vx = cr_vx[j] * 0.125f, vy = cr_vy[j] * 0.125f;
    if (drift > 0) {                                        // Drift: little random nudges
      vx += (cr_rand01() - 0.5f) * drift * 4.0f * dt;
      vy += (cr_rand01() - 0.5f) * drift * 4.0f * dt;
    } else if (cr_vx[j] == 0 && cr_vy[j] == 0) continue;     // resting ball: nothing to do
    vx *= keep; vy *= keep;
    if (drift == 0 && vx * vx + vy * vy < 9.0f) vx = vy = 0;  // rolled to a stop (< 3 px/s)
    cr_cx[j] += vx * dt; cr_cy[j] += vy * dt;
    float r = cr_cr[j];
    if (cr_cx[j] < r && vx < 0) { cr_cx[j] = r; vx = -vx * 0.8f; }
    else if (cr_cx[j] > cr_WW - r && vx > 0) { cr_cx[j] = cr_WW - r; vx = -vx * 0.8f; }
    if (cr_cy[j] < r && vy < 0) { cr_cy[j] = r; vy = -vy * 0.8f; }
    else if (cr_cy[j] > cr_WH - r && vy > 0) { cr_cy[j] = cr_WH - r; vy = -vy * 0.8f; }
    cr_vx[j] = cr_v16(vx); cr_vy[j] = cr_v16(vy);
    if (cr_vx[j] || cr_vy[j]) anyMoving = true;
  }
  // Ball ↔ ball. Keep the list sorted by left edge (it barely changes from
  // frame to frame, so this is quick), then only compare balls whose
  // left-to-right spans overlap.
  for (int a = 1; a < cr_nc; a++) {
    uint8_t v = cr_ord[a];
    float key = cr_cx[v] - cr_cr[v];
    int b = a - 1;
    while (b >= 0 && cr_cx[cr_ord[b]] - cr_cr[cr_ord[b]] > key) { cr_ord[b + 1] = cr_ord[b]; b--; }
    cr_ord[b + 1] = v;
  }
  for (int a = 0; a < cr_nc; a++) {
    int i = cr_ord[a];
    float right = cr_cx[i] + cr_cr[i];
    for (int b = a + 1; b < cr_nc; b++) {
      int j = cr_ord[b];
      if (cr_cx[j] - cr_cr[j] > right) break;                // nothing further right can touch
      if (!cr_vx[i] && !cr_vy[i] && !cr_vx[j] && !cr_vy[j]) continue;   // both resting
      float dx = cr_cx[j] - cr_cx[i], dy = cr_cy[j] - cr_cy[i], rr = cr_cr[i] + cr_cr[j];
      if (dy > rr || dy < -rr) continue;
      float d2 = dx * dx + dy * dy;
      if (d2 >= rr * rr || d2 < 0.0001f) continue;
      float d = sqrtf(d2), nx = dx / d, ny = dy / d;
      float mi = cr_cr[i] * cr_cr[i], mj = cr_cr[j] * cr_cr[j];   // bigger = heavier
      // Separate them (heavier one moves less)
      float pen = rr - d, si = pen * mj / (mi + mj), sj = pen - si;
      cr_cx[i] -= nx * si; cr_cy[i] -= ny * si;
      cr_cx[j] += nx * sj; cr_cy[j] += ny * sj;
      // Bounce: swap momentum along the line between their centres
      float vix = cr_vx[i] * 0.125f, viy = cr_vy[i] * 0.125f, vjx = cr_vx[j] * 0.125f, vjy = cr_vy[j] * 0.125f;
      float vn = (vix - vjx) * nx + (viy - vjy) * ny;        // closing speed
      if (vn <= 0) continue;
      float imp = (1.0f + cr_rest) * vn / (mi + mj);
      vix -= imp * mj * nx; viy -= imp * mj * ny;
      vjx += imp * mi * nx; vjy += imp * mi * ny;
      cr_vx[i] = cr_v16(vix); cr_vy[i] = cr_v16(viy);
      cr_vx[j] = cr_v16(vjx); cr_vy[j] = cr_v16(vjy);
      anyMoving = true;
      // Both flash and wobble, by how hard they hit
      int g = (int)(vn * 3.0f); if (g > 255) g = 255;
      if (g > cr_glow[i]) cr_glow[i] = (uint8_t)g;
      if (g > cr_glow[j]) cr_glow[j] = (uint8_t)g;
      int wi = cr_wob[i] + g / 3, wj = cr_wob[j] + g / 3;
      cr_wob[i] = (uint8_t)(wi > 255 ? 255 : wi); cr_wob[j] = (uint8_t)(wj > 255 ? 255 : wj);
    }
  }
  // Pick what to hunt: the nearest resting ball, preferring ones ahead.
  // Choose again when the target gets hit (starts rolling) or after a while.
  {
    static float huntT[CR_MAXCR];
    for (int k = 0; k < cr_n; k++) {
      huntT[k] += dt;
      int tg = cr_tgt[k];
      bool need = tg < 0 || tg >= cr_nc || huntT[k] > 6.0f ||
                  (cr_vx[tg] * cr_vx[tg] + cr_vy[tg] * cr_vy[tg]) > 25 * 64;   // rolling > 25 px/s
      if (!need) continue;
      float hx = cr_px[k][0], hy = cr_py[k][0], ch = cosf(cr_head[k]), sh = sinf(cr_head[k]);
      float best = 1e9f; int bi = -1;
      for (int j = 0; j < cr_nc; j++) {
        if (j == tg) continue;
        if (cr_vx[j] * cr_vx[j] + cr_vy[j] * cr_vy[j] > 25 * 64) continue;
        float dx = cr_cx[j] - hx, dy = cr_cy[j] - hy;
        float d = sqrtf(dx * dx + dy * dy) + 1.0f;
        float front = (dx * ch + dy * sh) / d;                 // 1 = straight ahead
        float score = d * (1.6f - 0.6f * front);
        if (score < best) { best = score; bi = j; }
      }
      cr_tgt[k] = (int8_t)bi;
      huntT[k] = 0;
    }
  }
  // Break: once everything has stopped for a few seconds, re-rack
  if (preset == 7) {
    static float stillT = 0;
    if (anyMoving) { cr_broken = true; stillT = 0; }
    else if (cr_broken) stillT += dt;
    if (cr_broken && stillT > 4.0f) { stillT = 0; cr_needWorld = true; }
  }
  // Flashes and wobbles fade (Lanterns: the glow lasts, p5 sets how long)
  {
    static float fadeAcc = 0, wobAcc = 0;
    fadeAcc += (preset == 5 ? cr_potf(5, 0.15f, 2.0f) : 3.0f) * 255.0f * dt;
    wobAcc += 1.6f * 255.0f * dt;
    int f = (int)fadeAcc, g = (int)wobAcc;
    fadeAcc -= f; wobAcc -= g;
    for (int j = 0; j < cr_nc; j++) {
      cr_glow[j] = (cr_glow[j] > f) ? cr_glow[j] - f : 0;
      cr_wob[j] = (cr_wob[j] > g) ? cr_wob[j] - g : 0;
    }
  }

  // Move the creatures (a few small steps per frame for stability)
  // (more, smaller steps when it's fast, so it never skips through a bubble)
  int steps = 2 + (int)(speed * dt / 3.0f);
  if (steps > 8) steps = 8;
  for (int s = 0; s < steps; s++)
    for (int k = 0; k < cr_n; k++)
      cr_update(k, dt / steps, speed * (k == 0 ? 1.0f : 0.8f + 0.1f * k), wiggle, wander, wigRate);

  // Camera
  {
    float kz = cr_sm[8] / 1023.0f;
    cr_zoom = (kz < 0.5f) ? powf(0.5f, (0.5f - kz) * 2.0f) : powf(3.0f, (kz - 0.5f) * 2.0f);
    float follow = cr_potf(9, 0.3f, 6.0f) * dt;          // how quickly the camera catches up
    if (follow > 1) follow = 1;
    cr_camX += (cr_px[0][0] - cr_camX) * follow;
    cr_camY += (cr_py[0][0] - cr_camY) * follow;
    // Keep the view inside the world
    float hw = HALFW / cr_zoom, hh = HALFH / cr_zoom;
    if (hw * 2 < cr_WW) { if (cr_camX < hw) cr_camX = hw; if (cr_camX > cr_WW - hw) cr_camX = cr_WW - hw; }
    else cr_camX = cr_WW / 2;
    if (hh * 2 < cr_WH) { if (cr_camY < hh) cr_camY = hh; if (cr_camY > cr_WH - hh) cr_camY = cr_WH - hh; }
    else cr_camY = cr_WH / 2;
  }

  // Spotlight: circles glow by how close they are to the creature's head
  if (preset == 4) {
    float R = cr_potf(5, 60.0f, 260.0f);
    float hx = cr_px[0][0], hy = cr_py[0][0];
    for (int j = 0; j < cr_nc; j++) {
      float dx = cr_cx[j] - hx, dy = cr_cy[j] - hy;
      float d = sqrtf(dx * dx + dy * dy) - cr_cr[j];
      float v = 1.0f - (d < 0 ? 0 : d) / R;
      uint8_t lv = v <= 0 ? 0 : (uint8_t)(v * v * 255.0f);
      if (lv > cr_glow[j]) cr_glow[j] = lv;                  // (keep a brighter hit flash)
    }
  }

  // Draw the scene
  cr_drawCircles(true, preset == 4);
  float legs = (preset == 6) ? cr_potf(5, 3.0f, 10.0f) : 0.0f;
  static float legPhase = 0;
  legPhase += dt * (speed > 150.0f ? 150.0f : speed) * 0.25f * wigRate;
  for (int k = 0; k < cr_n; k++) cr_drawCreature(k, legs, legPhase);
}
