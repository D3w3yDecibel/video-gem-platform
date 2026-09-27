// =====================================================================
// PROGRAM: TILE RUNNER (by Dewey)
// The whole screen is a wall of little studded stone tiles. A figure
// made OUT of tiles runs, dances and leaps across it: the tiles inside
// its shape flip to darkness up top and bubbling lava below, with a
// ragged edge of bricks and a scatter of gems and rings around it.
// Rough take on the "pixel-art tiles as a mosaic" look (inspired by a
// perfectl00p post) — original tiles and figure.
//
// Presets:
//   k0 Runner   — running on the spot
//   k1 Dancer   — dancing
//   k2 Pass-by  — runs across the wall and comes round again
//   k3 Twins    — two runners, mirrored
//   k4 Leap     — big bounding leaps
//   k5 Ripple   — no figure: waves of tiles flipping out from the centre
//   k6 Glitch   — runner, with tiles flickering all over the wall
//   Calisthenics:
//   k7 Jumping Jacks   k8 Squats   k9 Push-ups   k10 Sit-ups
//   k11 Workout — cycles through every exercise (also toe touches,
//                 side bends, high knees and lunges)
//
// This program OWNS the global knobs (it makes its own colours):
//   p0 Colors (stone, ice, jungle, royal, neon)
//   p1 Speed (far left = freeze)   p2 Sparkle (gems & rings)
//   p3 Tile Size
//   p4 Figure Size   p5 Lava Line (where dark turns to lava)
//   p6 Edge (brick border)   p7 Flicker
//   p8 Crisp (left = blocky tile edges; rest = sharp human outline)
// =====================================================================

// Palette
#define TR_BG        0
#define TR_STUD_HI   1     // stud: lit faces
#define TR_STUD_MID  2
#define TR_STUD_LO   3     // stud: shaded faces
#define TR_STUD_TOP  4     // stud: tip highlight
#define TR_GAP       5     // mortar between tiles
#define TR_VOID      6     // figure: dark
#define TR_VOID2     7     // figure: dark speck
#define TR_LAVA0     8     // 8–11 lava shades
#define TR_BRICK     12
#define TR_BRICK_LN  13
#define TR_GEM       14
#define TR_GEM_HI    15
#define TR_RING      16
#define TR_RING_HI   17

// Tile types
enum { TT_STUD = 0, TT_VOID, TT_LAVA, TT_BRICK, TT_GEM, TT_RING };

// ─── Knobs (smoothed) ─────────────────────────────────────────────────
static float tr_sm[16];
static bool  tr_smReady = false;

static void tr_smoothKnobs(float dt) {
  float a = dt / 0.12f;
  if (a > 1.0f) a = 1.0f;
  for (int i = 0; i < 16; i++) {
    if (!tr_smReady) tr_sm[i] = pots[i];
    else tr_sm[i] += (pots[i] - tr_sm[i]) * a;
  }
  tr_smReady = true;
}
static inline float tr_potf(int idx, float lo, float hi) { return lo + (hi - lo) * (tr_sm[idx] / 1023.0f); }
static inline int tr_pot(int idx, int lo, int hi) {
  int v = (int)floorf(lo + (hi - lo + 1) * (tr_sm[idx] / 1024.0f));
  return v < lo ? lo : (v > hi ? hi : v);
}

// ─── Colours ──────────────────────────────────────────────────────────
static int tr_palScheme = -1;

static void tr_buildPalette(int scheme) {
  if (scheme == tr_palScheme) return;
  tr_palScheme = scheme;
  // stud base colour for each scheme (the wall), and the lava hue
  static const uint8_t wall[5][3] = {{150, 150, 155}, {120, 170, 210}, {110, 160, 100}, {150, 120, 190}, {70, 70, 80}};
  static const uint8_t lava[5][3] = {{230, 70, 30}, {255, 120, 60}, {240, 90, 40}, {230, 60, 120}, {60, 255, 180}};
  const uint8_t* w = wall[scheme];
  const uint8_t* l = lava[scheme];
  for (int i = 0; i < 256; i++) display.setColor(i, 0, 0, 0);
  display.setColor(TR_STUD_HI,  w[0] + (255 - w[0]) / 2, w[1] + (255 - w[1]) / 2, w[2] + (255 - w[2]) / 2);
  display.setColor(TR_STUD_MID, w[0], w[1], w[2]);
  display.setColor(TR_STUD_LO,  w[0] / 2, w[1] / 2, w[2] / 2);
  display.setColor(TR_STUD_TOP, 250, 250, 250);
  display.setColor(TR_GAP,      w[0] / 5, w[1] / 5, w[2] / 5);
  display.setColor(TR_VOID,     4, 4, 8);
  display.setColor(TR_VOID2,    60, 60, 90);
  for (int k = 0; k < 4; k++) {
    float f = 0.45f + 0.18f * k;
    display.setColor(TR_LAVA0 + k, (uint8_t)(l[0] * f > 255 ? 255 : l[0] * f),
                     (uint8_t)(l[1] * f > 255 ? 255 : l[1] * f), (uint8_t)(l[2] * f > 255 ? 255 : l[2] * f));
  }
  display.setColor(TR_BRICK,    l[0] / 2 + 30, l[1] / 3, l[2] / 3);
  display.setColor(TR_BRICK_LN, 20, 8, 8);
  display.setColor(TR_GEM,      40, 200, 230);
  display.setColor(TR_GEM_HI,   220, 255, 255);
  display.setColor(TR_RING,     240, 170, 40);
  display.setColor(TR_RING_HI,  255, 240, 170);
}

// ─── Tiles ────────────────────────────────────────────────────────────
// The fixed tiles (stud, brick, gem, ring) are worked out once per tile
// size and kept in a small cache; lava keeps a pattern and animates it.
static uint8_t* tr_buf;
#define TR_MAXS 12
static uint8_t tr_cache[4][TR_MAXS * TR_MAXS];     // stud, brick, gem, ring
static uint8_t tr_lavaBase[TR_MAXS * TR_MAXS];
static int     tr_cacheS = -1;
static const uint8_t tr_lavaCol[16] = {
  TR_LAVA0 + 3, TR_LAVA0 + 3, TR_LAVA0 + 3, TR_LAVA0 + 3, TR_LAVA0 + 2, TR_LAVA0 + 2, TR_LAVA0 + 2, TR_LAVA0 + 2,
  TR_LAVA0 + 2, TR_LAVA0 + 1, TR_LAVA0 + 1, TR_LAVA0 + 1, TR_LAVA0 + 1, TR_LAVA0, TR_LAVA0, TR_LAVA0
};

// (u, v) = pixel within the tile, s = tile size — the fixed designs
static uint8_t tr_makePix(int type, int u, int v, int s) {
  int e = s - 1;
  switch (type) {
    case TT_STUD: {
      if (u == e || v == e) return TR_GAP;                       // mortar line
      // pyramid stud: four faces, lit from top-left
      int du = 2 * u - (e - 1), dv = 2 * v - (e - 1);
      if (abs(du) <= 1 && abs(dv) <= 1) return TR_STUD_TOP;
      if (abs(du) > abs(dv)) return du < 0 ? TR_STUD_HI : TR_STUD_LO;
      return dv < 0 ? TR_STUD_HI : (abs(du) == abs(dv) ? TR_STUD_MID : TR_STUD_LO);
    }
    case TT_BRICK: {
      if (u == e || v == e) return TR_BRICK_LN;
      int half = s / 2;
      if (v == half - 1) return TR_BRICK_LN;
      if (v < half ? (u == half - 1) : (u == 0)) return TR_BRICK_LN;
      return TR_BRICK;
    }
    case TT_GEM: {
      int c = e / 2;
      int d = abs(u - c) + abs(v - c);
      if (d > c - 1) return TR_VOID;
      return (u < c && v < c) ? TR_GEM_HI : TR_GEM;
    }
    default: {   // ring
      int c2 = e;
      int dx = 2 * u - c2, dy = 2 * v - c2, d2 = dx * dx + dy * dy;
      int ro = e * e, ri = (e - 3) * (e - 3);
      if (d2 > ro || d2 < ri) return TR_VOID;
      return (dx < 0 && dy < 0) ? TR_RING_HI : TR_RING;
    }
  }
}

static void tr_buildCache(int s) {
  if (s == tr_cacheS) return;
  tr_cacheS = s;
  static const int types[4] = {TT_STUD, TT_BRICK, TT_GEM, TT_RING};
  for (int k = 0; k < 4; k++)
    for (int v = 0; v < s; v++)
      for (int u = 0; u < s; u++) tr_cache[k][v * s + u] = tr_makePix(types[k], u, v, s);
  for (int v = 0; v < s; v++)
    for (int u = 0; u < s; u++) tr_lavaBase[v * s + u] = (uint8_t)((u * 3 + v * 2 + ((u * v) >> 2)) & 15);
}

static inline const uint8_t* tr_cached(int type) {
  return tr_cache[type == TT_STUD ? 0 : type == TT_BRICK ? 1 : type == TT_GEM ? 2 : 3];
}

// One pixel of any tile (used where a tile is split by the outline)
static inline uint8_t tr_tilePix(int type, int u, int v, int s, int a, uint32_t h) {
  if (type == TT_LAVA) return tr_lavaCol[(tr_lavaBase[v * s + u] + a + (h & 7)) & 15];
  if (type == TT_VOID) return (((u * 7 + v * 13 + (int)(h & 63)) & 63) == 0) ? TR_VOID2 : TR_VOID;
  return tr_cached(type)[v * s + u];
}

static void tr_drawTile(int tx, int ty, int s, int type, int a, uint32_t h) {
  int x0 = tx * s, y0 = ty * s;
  int w = s, hgt = s;
  if (x0 + w > W) w = W - x0;
  if (y0 + hgt > H) hgt = H - y0;
  if (w <= 0 || hgt <= 0) return;
  uint8_t* dst = tr_buf + y0 * W + x0;
  if (type == TT_LAVA) {
    int off = (a + (int)(h & 7)) & 15;
    for (int v = 0; v < hgt; v++, dst += W) {
      const uint8_t* b = tr_lavaBase + v * s;
      for (int u = 0; u < w; u++) dst[u] = tr_lavaCol[(b[u] + off) & 15];
    }
  } else if (type == TT_VOID) {
    for (int v = 0; v < hgt; v++, dst += W) memset(dst, TR_VOID, w);
    int sp = (int)((h >> 8) % (uint32_t)(s * s));          // one faint speck
    if (sp % s < w && sp / s < hgt && (h & 3) == 0) tr_buf[(y0 + sp / s) * W + x0 + sp % s] = TR_VOID2;
  } else {
    const uint8_t* src = tr_cached(type);
    for (int v = 0; v < hgt; v++, dst += W, src += s) memcpy(dst, src, w);
  }
}

// ─── The figure: tapered "bones" ─────────────────────────────────────
// Each bone is a capsule that can be thicker at one end (like a thigh).
// Stored in whole numbers (1/16 of a tile) so the per-tile maths needs no
// floating point — the RP2040 has no hardware for decimals, integers are
// many times faster.
#define TR_BONES 36                                // 17 per figure, two figures
#define TR_Q 16                                    // sub-tile steps
static int16_t tr_ax[TR_BONES], tr_ay[TR_BONES], tr_dx[TR_BONES], tr_dy[TR_BONES];
static int32_t tr_l2[TR_BONES];
static int16_t tr_ra[TR_BONES], tr_dr[TR_BONES];   // radius at the start, and change to the end
static int16_t tr_x0[TR_BONES], tr_y0[TR_BONES], tr_x1[TR_BONES], tr_y1[TR_BONES];  // bounding boxes
static uint8_t tr_la[TR_BONES], tr_lb[TR_BONES];  // body level at each end: 0 = top of head, 255 = feet
static int     tr_nb = 0;
static float   tr_fx0, tr_fy0, tr_fx1, tr_fy1;    // box around all figures (tiles)

static inline int16_t tr_q(float v) {              // tiles → 1/16 tiles, kept in range
  v *= TR_Q;
  if (v > 30000) v = 30000; else if (v < -30000) v = -30000;
  return (int16_t)v;
}

static void tr_bone(float ax, float ay, float bx, float by, float ra, float rb, int fig, int la, int lb) {
  (void)fig;
  if (tr_nb >= TR_BONES) return;
  int i = tr_nb++;
  tr_la[i] = (uint8_t)la; tr_lb[i] = (uint8_t)lb;
  tr_ax[i] = tr_q(ax); tr_ay[i] = tr_q(ay);
  tr_dx[i] = (int16_t)(tr_q(bx) - tr_ax[i]); tr_dy[i] = (int16_t)(tr_q(by) - tr_ay[i]);
  tr_l2[i] = (int32_t)tr_dx[i] * tr_dx[i] + (int32_t)tr_dy[i] * tr_dy[i];
  tr_ra[i] = tr_q(ra); tr_dr[i] = (int16_t)(tr_q(rb) - tr_ra[i]);
  float r = ra > rb ? ra : rb;
  float x0 = (ax < bx ? ax : bx) - r, x1 = (ax > bx ? ax : bx) + r;
  float y0 = (ay < by ? ay : by) - r, y1 = (ay > by ? ay : by) + r;
  tr_x0[i] = tr_q(x0); tr_x1[i] = tr_q(x1); tr_y0[i] = tr_q(y0); tr_y1[i] = tr_q(y1);
  if (x0 < tr_fx0) tr_fx0 = x0;
  if (x1 > tr_fx1) tr_fx1 = x1;
  if (y0 < tr_fy0) tr_fy0 = y0;
  if (y1 > tr_fy1) tr_fy1 = y1;
}

// Whole-number square root (for the few points right near a bone)
static inline int32_t tr_isqrt(int32_t n) {
  if (n <= 0) return 0;
  int32_t x = (int32_t)sqrtf((float)n);           // close guess, then tidy up
  while (x * x > n) x--;
  while ((x + 1) * (x + 1) <= n) x++;
  return x;
}

// ─── Poses ────────────────────────────────────────────────────────────
// A pose is a set of joint angles. Angles are measured from straight
// down; side-on, + = toward the way the figure faces; facing you, + = out
// to that side. [0] = one side, [1] = the other.
enum { TM_RUN = 0, TM_DANCE, TM_LEAP, TM_JACKS, TM_SQUAT, TM_PUSHUP, TM_SITUP,
       TM_TOETOUCH, TM_SIDEBEND, TM_HIGHKNEES, TM_LUNGE };
static float tr_pA1[2], tr_pA2[2];            // upper arm, forearm
static float tr_pT1[2], tr_pT2[2], tr_pT3[2]; // thigh, shin, foot
static float tr_pLean;                         // torso tilt (side-on: forward; facing you: sideways)
static float tr_pLift;                         // extra hop off the ground (× height)
static float tr_pRot;                          // whole-body turn (push-ups, sit-ups)
static float tr_pShiftX;                       // hips shift (× height)
static bool  tr_pFront;                        // facing you?
static uint8_t tr_pPivot;                      // 0 = stand on feet, 1 = turn about the feet, 2 = about the hips

static inline float tr_ease(float u) { return 0.5f - 0.5f * cosf(u); }   // 0 → 1 → 0 smoothly

static void tr_pose(int mode, float ph) {
  tr_pLean = 0; tr_pLift = 0; tr_pRot = 0; tr_pShiftX = 0; tr_pFront = false; tr_pPivot = 0;
  float f = tr_ease(ph);
  for (int sd = 0; sd < 2; sd++) {
    float p = ph + (sd ? PI : 0);
    float s1 = sinf(p), c1 = cosf(p);
    float vUA = 0.1f, vFA = 0.3f, vTH = 0, vSH = 0, vFT = 1.5f;
    switch (mode) {
      case TM_RUN: {
        tr_pLean = 0.16f; tr_pLift = 0.03f * fabsf(cosf(ph));
        vTH = 0.75f * s1; vSH = vTH - (0.2f + 1.5f * (c1 > 0 ? c1 : 0)); vFT = vSH + 1.45f;
        vUA = -0.85f * s1; vFA = vUA + 1.5f;
        break;
      }
      case TM_LEAP: {
        tr_pLean = 0.28f; tr_pLift = 0.2f * fabsf(sinf(ph * 0.5f));
        vTH = 1.1f * s1; vSH = vTH - (0.2f + 0.9f * (c1 > 0 ? c1 : 0)); vFT = vSH + 1.45f;
        vUA = -1.1f * s1; vFA = vUA + 1.5f;
        break;
      }
      case TM_DANCE: {
        tr_pFront = true; tr_pLean = 0.12f * sinf(ph); tr_pLift = 0.03f * fabsf(sinf(ph * 2));
        float side = sd ? -1.0f : 1.0f;
        float g = 0.5f + 0.5f * sinf(ph * 2 + side * 1.6f);
        vTH = 0.12f + 0.2f * g; vSH = vTH - 0.35f * (0.5f + 0.5f * sinf(ph * 2 + side * 1.6f + 1.0f)); vFT = 1.4f;
        vUA = 1.0f + 1.3f * (0.5f + 0.5f * sinf(ph * 2 + side * 1.3f));
        vFA = vUA + 0.3f + 0.9f * (0.5f + 0.5f * sinf(ph * 3 + side));
        break;
      }
      case TM_JACKS: {                                  // facing you: arms and legs out and in
        tr_pFront = true; tr_pLift = 0.06f * fabsf(sinf(ph));
        vUA = 0.2f + 2.75f * f; vFA = vUA + 0.1f + 0.15f * f;
        vTH = 0.05f + 0.32f * f; vSH = vTH; vFT = 1.3f;
        break;
      }
      case TM_SQUAT: {                                  // side-on: sit back, arms forward
        tr_pLean = 0.55f * f; tr_pShiftX = -0.06f * f;
        vTH = 1.35f * f; vSH = vTH - 2.25f * f; vFT = 1.5708f;
        vUA = 0.15f + 1.4f * f; vFA = vUA + 0.05f;
        break;
      }
      case TM_PUSHUP: {                                 // body turned to lie face-down
        tr_pPivot = 1;
        float bend = f;
        float fl = 1.28f + 0.14f * bend;                // which way is the floor (from "down")
        tr_pRot = fl;
        vTH = 0; vSH = 0; vFT = fl;                        // legs straight, toes on the floor
        vUA = fl - 0.95f * bend; vFA = fl + 0.95f * bend; // elbows fold back toward the feet
        break;
      }
      case TM_SITUP: {                                  // lying on the back, knees up, curling up
        tr_pPivot = 2; tr_pRot = -1.5708f;
        tr_pLean = 1.15f * f;                           // torso curls up
        vTH = 0.95f; vSH = vTH - 2.05f; vFT = vSH + 1.5f;
        float torso = PI - tr_pLean;                    // direction of the spine
        vUA = torso + 0.35f + (sd ? 0.1f : 0.0f); vFA = vUA + 2.6f;   // hands behind the head
        break;
      }
      case TM_TOETOUCH: {                               // side-on: reach up, fold down to the toes
        tr_pLean = 1.5f * f; tr_pShiftX = -0.08f * f;
        vTH = -0.08f * f; vSH = vTH; vFT = 1.5708f;
        vUA = (PI - 0.2f) * (1 - f) + (0.35f + tr_pLean * 0.15f) * f; vFA = vUA + 0.05f;
        break;
      }
      case TM_SIDEBEND: {                               // facing you: lean to each side, arm over the top
        tr_pFront = true;
        float lean = 0.45f * sinf(ph);
        tr_pLean = lean;
        float side = sd ? -1.0f : 1.0f;                 // arm on the side we lean away from goes up
        float up = (lean * side < 0) ? fabsf(lean) / 0.45f : 0.0f;
        vUA = 0.25f + 2.6f * up; vFA = vUA + 0.35f * up + 0.1f;
        vTH = 0.14f; vSH = vTH; vFT = 1.4f;
        break;
      }
      case TM_HIGHKNEES: {                              // side-on: knees driven up high
        tr_pLean = 0.05f; tr_pLift = 0.04f * fabsf(cosf(ph));
        float up = s1 > 0 ? s1 : 0;
        vTH = 1.55f * up; vSH = vTH - 1.9f * up; vFT = vSH + 1.5f;
        vUA = -0.9f * s1; vFA = vUA + 1.6f;
        break;
      }
      case TM_LUNGE: {                                  // side-on: step forward and sink
        float g = tr_ease(ph * 0.5f) ;                  // alternate legs each rep
        float lead = (sinf(ph * 0.5f) > 0) == (sd == 0) ? 1.0f : 0.0f;
        tr_pLean = 0.05f;
        if (lead > 0) { vTH = 1.25f * f; vSH = vTH - 1.25f * f; }
        else          { vTH = -0.45f * f; vSH = vTH - 1.35f * f; }
        vFT = vSH + 1.5708f;
        vUA = -0.35f; vFA = 1.2f;                         // hands on hips
        (void)g;
        break;
      }
    }
    tr_pA1[sd] = vUA; tr_pA2[sd] = vFA; tr_pT1[sd] = vTH; tr_pT2[sd] = vSH; tr_pT3[sd] = vFT;
  }
}

// Build a human figure from the current pose. (cx, groundY) = where it
// stands, in tiles; S = height in tiles; m = facing (+1 right, −1 left).
static void tr_figure(float cx, float groundY, float S, float m, int fig) {
  bool front = tr_pFront;
  // direction for an angle, for side sd (facing you: out to that side)
  #define TR_DIRX(a, sd) (sinf(a) * (front ? ((sd) ? -1.0f : 1.0f) : m))
  float L = tr_pLean;
  float lx = front ? sinf(L) : sinf(L) * m, ly = -cosf(L);
  float sw = front ? 0.11f : 0.035f;                 // half shoulder width
  float pw = front ? 0.06f : 0.02f;                  // half hip width
  // joints (relative to the pelvis at 0,0)
  float chX = lx * 0.27f, chY = ly * 0.27f;
  float nkX = lx * 0.36f, nkY = ly * 0.36f;
  float hdX = nkX + lx * 0.1f, hdY = nkY + ly * 0.1f;
  float kX[2], kY[2], aX[2], aY[2], tX[2], tY[2], sX[2], sY[2], eX[2], eY[2], wX[2], wY[2], hpX[2];
  float lowest = -1e9f;
  for (int sd = 0; sd < 2; sd++) {
    float side = sd ? -1.0f : 1.0f;
    hpX[sd] = front ? side * pw : 0;
    float hy = 0.02f;
    // knee bends shorten the leg a little when seen from the front
    float kneeBend = fabsf(tr_pT1[sd] - tr_pT2[sd]);
    float shin = front ? 0.235f * (1.0f - 0.25f * kneeBend) : 0.235f;
    kX[sd] = hpX[sd] + TR_DIRX(tr_pT1[sd], sd) * 0.245f; kY[sd] = hy + cosf(tr_pT1[sd]) * 0.245f;
    aX[sd] = kX[sd] + TR_DIRX(tr_pT2[sd], sd) * shin;    aY[sd] = kY[sd] + cosf(tr_pT2[sd]) * shin;
    tX[sd] = aX[sd] + TR_DIRX(tr_pT3[sd], sd) * 0.075f;  tY[sd] = aY[sd] + cosf(tr_pT3[sd]) * 0.075f;
    if (aY[sd] > lowest) lowest = aY[sd];
    if (tY[sd] > lowest) lowest = tY[sd];
    // arms, from the shoulders
    sX[sd] = chX + (front ? side * sw : 0); sY[sd] = chY + 0.01f;
    float a1 = tr_pA1[sd], a2 = tr_pA2[sd];
    if (front) {                                     // sway: arms hang relative to the tilted body
      a1 -= side * L; a2 -= side * L;
    }
    eX[sd] = sX[sd] + TR_DIRX(a1, sd) * 0.165f; eY[sd] = sY[sd] + cosf(a1) * 0.165f;
    wX[sd] = eX[sd] + TR_DIRX(a2, sd) * 0.15f;  wY[sd] = eY[sd] + cosf(a2) * 0.15f;
  }
  #undef TR_DIRX
  // Where do the hips go, and how is the body turned?
  float px = 0, py = 0;                                 // pivot (relative)
  float wx0 = cx, wy0;                                  // where the pivot lands
  if (tr_pPivot == 1) {                                 // turn about the toes (push-ups)
    px = (tX[0] + tX[1]) * 0.5f; py = (tY[0] + tY[1]) * 0.5f;
    wx0 = cx - m * 0.45f * S; wy0 = groundY - 0.02f * S;
  } else if (tr_pPivot == 2) {                          // turn about the hips (sit-ups)
    wx0 = cx - m * 0.1f * S; wy0 = groundY - 0.075f * S;
  } else {                                              // standing: lowest foot on the ground
    wy0 = groundY - (lowest + 0.025f) * S - tr_pLift * S;
    wx0 = cx + tr_pShiftX * S * m;
  }
  float rc = cosf(tr_pRot * m), rs = sinf(tr_pRot * m);
  // place a relative point in the world
  #define TR_PX(x, y) (wx0 + (((x) - px) * rc - ((y) - py) * rs) * S)
  #define TR_PY(x, y) (wy0 + (((x) - px) * rs + ((y) - py) * rc) * S)
  // body level (head → feet) from the height in the upright pose, so lava
  // stays on the legs even when the figure lies down
  #define TR_LV(y) ((int)((y) < -0.55f ? 0 : (y) > 0.5f ? 255 : ((y) + 0.55f) * 242.8f))
  #define TR_B(x1, y1, x2, y2, r1, r2) { float A = TR_PX(x1, y1), B = TR_PY(x1, y1), C = TR_PX(x2, y2), D = TR_PY(x2, y2); \
      tr_bone(A, B, C, D, (r1) * S, (r2) * S, fig, TR_LV(y1), TR_LV(y2)); }
  TR_B(0, 0, chX, chY, 0.075f, 0.095f);                                  // torso
  TR_B(chX - sw, chY + 0.01f, chX + sw, chY + 0.01f, 0.05f, 0.05f);    // shoulders
  TR_B(-pw, 0, pw, 0, 0.07f, 0.07f);                                     // hips
  TR_B(chX, chY, nkX, nkY, 0.035f, 0.03f);                               // neck
  TR_B(hdX, hdY, hdX + lx * 0.02f, hdY + ly * 0.02f, 0.068f, 0.062f);    // head
  for (int sd = 0; sd < 2; sd++) {
    TR_B(hpX[sd], 0.02f, kX[sd], kY[sd], 0.068f, 0.045f);                // thigh
    TR_B(kX[sd], kY[sd], aX[sd], aY[sd], 0.045f, 0.028f);                // shin
    TR_B(aX[sd], aY[sd], tX[sd], tY[sd], 0.028f, 0.022f);                // foot
    TR_B(sX[sd], sY[sd], eX[sd], eY[sd], 0.04f, 0.032f);                 // upper arm
    TR_B(eX[sd], eY[sd], wX[sd], wY[sd], 0.03f, 0.024f);                 // forearm
    float hx2 = wX[sd] + (wX[sd] - eX[sd]) * 0.2f, hy2 = wY[sd] + (wY[sd] - eY[sd]) * 0.2f;
    TR_B(wX[sd], wY[sd], hx2, hy2, 0.03f, 0.026f);                       // hand
  }
  #undef TR_B
  #undef TR_LV
  #undef TR_PX
  #undef TR_PY
  (void)fig;
}

// How deep inside a figure is the point (x, y)? (all in 1/16 tiles)
// > 0 inside. Anything farther than `margin` outside reports −margin.
// Bones that reach a given row (worked out once per row, not per point)
static uint8_t tr_rowB[TR_BONES];
static int     tr_rowN = 0;

static void tr_rowBones(int y, int margin) {
  tr_rowN = 0;
  for (int b = 0; b < tr_nb; b++)
    if (y >= tr_y0[b] - margin && y <= tr_y1[b] + margin) tr_rowB[tr_rowN++] = (uint8_t)b;
}

static int tr_depth(int x, int y, int margin, uint8_t& lvl) {
  int best = -margin;
  lvl = 0;
  for (int k = 0; k < tr_rowN; k++) {
    int b = tr_rowB[k];
    if (x < tr_x0[b] - margin || x > tr_x1[b] + margin) continue;
    int32_t rx = x - tr_ax[b], ry = y - tr_ay[b];
    int32_t num = rx * tr_dx[b] + ry * tr_dy[b];
    int32_t t;                                      // 0 … 256 along the bone
    if (num <= 0 || tr_l2[b] == 0) t = 0;
    else if (num >= tr_l2[b]) t = 256;
    else t = (num << 8) / tr_l2[b];                 // (bones are short enough that this fits)
    int32_t px = ((tr_dx[b] * t) >> 8) - rx, py = ((tr_dy[b] * t) >> 8) - ry;
    int32_t r = tr_ra[b] + ((tr_dr[b] * t) >> 8);
    int32_t d2 = px * px + py * py, lim = r - best; // only closer than the best so far matters
    if (lim <= 0 || d2 >= lim * lim) continue;
    int d = (int)(r - tr_isqrt(d2));
    if (d > best) { best = d; lvl = (uint8_t)(tr_la[b] + (((int)tr_lb[b] - tr_la[b]) * t >> 8)); }
  }
  return best;
}

// Edge tile, drawn pixel by pixel. The depth at the four corners is blended
// across the tile, so the outline stays sharp without any extra maths.
static void tr_drawEdgeTile(int tx, int ty, int s, int D00, int D10, int D01, int D11,
                            int outType, int typeIn, int a, uint32_t h) {
  int s2 = 2 * s;
  for (int v = 0; v < s; v++) {
    int y = ty * s + v;
    if ((unsigned)y >= (unsigned)H) break;
    int wy = 2 * v + 1;                                     // weight toward the bottom edge (of 2s)
    int L = D00 * (s2 - wy) + D01 * wy, R = D10 * (s2 - wy) + D11 * wy;
    uint8_t* row = tr_buf + y * W;
    for (int u = 0; u < s; u++) {
      int x = tx * s + u;
      if ((unsigned)x >= (unsigned)W) break;
      int wx = 2 * u + 1;
      bool in = ((long)L * (s2 - wx) + (long)R * wx) > 0;
      row[x] = tr_tilePix(in ? typeIn : outType, u, v, s, a, h);
    }
  }
}

static inline uint32_t tr_hash(int a, int b, int c) {
  uint32_t h = (uint32_t)a * 73856093u ^ (uint32_t)b * 19349663u ^ (uint32_t)c * 83492791u;
  h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
  return h;
}

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_tiles_name() { return "TILE RUNNER"; }
const char* prog_tiles_character() { return "A figure made of tiles running across a wall of stone studs"; }

static const char* const tr_presetNames[] = {
  "Runner", "Dancer", "Pass-by", "Twins", "Leap", "Ripple", "Glitch",
  "Jumping Jacks", "Squats", "Push-ups", "Sit-ups", "Workout"
};
#define TR_NUM_PRESETS 12

const char* prog_tiles_presetName(int preset) {
  if (preset >= 0 && preset < TR_NUM_PRESETS) return tr_presetNames[preset];
  return NULL;
}

static const char* const tr_labels[9] = {
  "Colors", "Speed", "Sparkle", "Tile Size", "Figure Size", "Lava Line", "Edge", "Flicker", "Crisp"
};
const char* prog_tiles_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 9) return tr_labels[pot];
  return "";
}

uint8_t prog_tiles_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;        // every pixel is drawn each frame
}

void prog_tiles_init() {
  tr_palScheme = -1;
  tr_cacheS = -1;
}

void prog_tiles_draw(int preset) {
  tr_buf = display.getBuffer();

  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  tr_smoothKnobs(dt);

  int scheme = tr_pot(0, 0, 4);
  float sk = tr_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.15f * powf(20.0f, (sk - 0.03f) / 0.97f);   // 0.15 … 3
  float sparkle = tr_potf(2, 0.0f, 0.5f);
  static const uint8_t sizes[6] = {3, 4, 5, 6, 8, 12};
  int s = sizes[tr_pot(3, 0, 5)];
  tr_buildCache(s);
  float figSize = tr_potf(4, 0.45f, 1.05f);
  float lavaLine = tr_potf(5, 0.0f, 1.0f);       // 0 = all lava, 1 = all dark (centre = waist)
  float ek = tr_sm[6] / 1023.0f;
  float edge = 1.4f * ek * ek;                   // brick border (centre ≈ 0.35 tile)
  bool crisp = tr_sm[8] > 200;                   // p8: left = blocky tiles only, else crisp outline
  float fk = tr_sm[7] / 1023.0f;
  float flicker = 0.3f * fk * fk * fk;          // mostly gentle; far right = lots
  tr_buildPalette(scheme);

  static float ph = 0, tAnim = 0, pos = 0;
  ph += dt * speed * 5.0f;
  tAnim += dt * speed * 20.0f;
  int a = (int)tAnim;

  int cols = (W + s - 1) / s, rows = (H + s - 1) / s;
  float hgt = rows * 0.88f * figSize;

  // Figures for this preset
  int nFig = (preset == 3) ? 2 : (preset == 5 ? 0 : 1);
  if (preset == 2) {
    pos += dt * speed * cols * 0.12f;
    if (pos > cols + hgt * 0.5f) pos = -hgt * 0.5f;
  }
  tr_nb = 0;
  tr_fx0 = tr_fy0 = 1e9f; tr_fx1 = tr_fy1 = -1e9f;
  float groundY = rows * 0.5f + hgt * 0.5f;
  // Workout: a new exercise every few seconds
  static const uint8_t workout[8] = {TM_JACKS, TM_SQUAT, TM_PUSHUP, TM_SITUP, TM_TOETOUCH, TM_SIDEBEND, TM_HIGHKNEES, TM_LUNGE};
  static float wT = 0;
  wT += dt;
  if (nFig > 0) {
    int mode;
    float rate = 1.0f;                                   // exercises go a little slower than running
    switch (preset) {
      case 1:  mode = TM_DANCE; break;
      case 4:  mode = TM_LEAP; break;
      case 7:  mode = TM_JACKS; rate = 0.55f; break;
      case 8:  mode = TM_SQUAT; rate = 0.4f; break;
      case 9:  mode = TM_PUSHUP; rate = 0.45f; break;
      case 10: mode = TM_SITUP; rate = 0.4f; break;
      case 11: mode = workout[((int)(wT / 7.0f)) & 7]; rate = (mode == TM_HIGHKNEES) ? 0.8f : 0.45f; break;
      default: mode = TM_RUN; break;
    }
    static float ex = 0;
    ex += dt * speed * 5.0f * rate;
    float usePh = (preset >= 7) ? ex : ph;
    tr_pose(mode, usePh);
    float fx = (preset == 2) ? pos : (preset == 3 ? cols * 0.3f : cols * 0.5f);
    float sz = preset == 3 ? hgt * 0.85f : hgt;
    tr_figure(fx, groundY, sz, 1.0f, 0);
    if (preset == 3) { tr_pose(TM_RUN, ph + 0.5f); tr_figure(cols * 0.7f, groundY, sz, -1.0f, 1); }
  }
  int lavaLv = (int)(lavaLine * 255);              // body level where dark turns to lava

  // Ripple: rings flipping outward from the centre
  float rip = tAnim * 0.15f;
  int frameKey = (int)(tAnim * 0.25f);
  float fl = preset == 6 ? flicker + 0.08f : flicker;

  // Depth at the tile corners, two rows at a time (top and bottom edge of a tile row)
  static int16_t cTop[108], cBot[108];
  static uint8_t fTop[108], fBot[108];
  float dm = edge + 2.0f;                                 // how far out we care about (tiles)
  int dmQ = (int)(dm * TR_Q), edgeQ = (int)(edge * TR_Q);
  int bx0 = (int)floorf(tr_fx0 - dm) - 1, bx1 = (int)ceilf(tr_fx1 + dm) + 1;
  int by0 = (int)floorf(tr_fy0 - dm) - 1, by1 = (int)ceilf(tr_fy1 + dm) + 1;
  if (bx0 < 0) bx0 = 0;
  if (bx1 > cols) bx1 = cols;
  bool haveTop = false;

  for (int ty = 0; ty < rows; ty++) {
    bool figRow = nFig > 0 && preset != 5 && ty >= by0 && ty <= by1;
    if (figRow) {
      if (!haveTop) {
        tr_rowBones(ty * TR_Q, dmQ);
        for (int x = bx0; x <= bx1; x++) cTop[x] = (int16_t)tr_depth(x * TR_Q, ty * TR_Q, dmQ, fTop[x]);
        haveTop = true;
      }
      tr_rowBones((ty + 1) * TR_Q, dmQ);
      for (int x = bx0; x <= bx1; x++) cBot[x] = (int16_t)tr_depth(x * TR_Q, (ty + 1) * TR_Q, dmQ, fBot[x]);
    }
    for (int tx = 0; tx < cols; tx++) {
      float cx = tx + 0.5f, cy = ty + 0.5f;
      uint32_t h = 0;
      int type = TT_STUD;
      bool edgeTile = false;
      int lvl = 0;
      int d00 = 0, d10 = 0, d01 = 0, d11 = 0;
      if (preset == 5) {
        h = tr_hash(tx, ty, 11);
        float dx = cx - cols * 0.5f, dy = cy - rows * 0.5f;
        float d = sqrtf(dx * dx + dy * dy) - rip;
        int band = ((int)floorf(d * 0.35f)) & 3;
        type = band == 0 ? TT_LAVA : band == 1 ? TT_BRICK : band == 2 ? TT_STUD : TT_VOID;
        if (type == TT_VOID && ((h >> 5) & 1023) < sparkle * 1023) type = (h & 1) ? TT_GEM : TT_RING;
      } else if (figRow && tx >= bx0 && tx < bx1) {
        d00 = cTop[tx]; d10 = cTop[tx + 1]; d01 = cBot[tx]; d11 = cBot[tx + 1];
        // body level from the deepest corner
        lvl = fTop[tx]; int dd = d00;
        if (d10 > dd) { dd = d10; lvl = fTop[tx + 1]; }
        if (d01 > dd) { dd = d01; lvl = fBot[tx]; }
        if (d11 > dd) { lvl = fBot[tx + 1]; }
        int in = (d00 + d10 + d01 + d11) >> 2;
        int mn = d00 < d10 ? d00 : d10; if (d01 < mn) mn = d01; if (d11 < mn) mn = d11;
        int mx = d00 > d10 ? d00 : d10; if (d01 > mx) mx = d01; if (d11 > mx) mx = d11;
        if (crisp && mn < 0 && mx > 0) edgeTile = true;
        else if (in > 0) type = (lvl > lavaLv) ? TT_LAVA : TT_VOID;
        if (!edgeTile && in <= 0 && in > -edgeQ) {
          // ragged border: bricks, with gems and rings scattered in
          uint32_t hh = tr_hash(tx, ty, frameKey);
          if ((hh & 1023) < 600) type = TT_BRICK;
          if (((hh >> 10) & 1023) < sparkle * 1400) type = (hh & (1 << 20)) ? TT_GEM : TT_RING;
        } else if (!edgeTile && in <= -edgeQ && in > -dmQ) {
          uint32_t hh = tr_hash(tx, ty, frameKey);
          if (((hh >> 10) & 1023) < sparkle * 300) type = (hh & (1 << 20)) ? TT_GEM : TT_RING;
        }
      }
      if (fl > 0.005f && !edgeTile) {
        uint32_t hh = tr_hash(tx, ty, a);
        if ((hh & 1023) < fl * 1023 * 0.2f) type = (int)((hh >> 12) % 6);
      }
      if (h == 0 && (edgeTile || type == TT_VOID || type == TT_LAVA)) h = tr_hash(tx, ty, 11);
      if (edgeTile) {
        uint32_t hh = tr_hash(tx, ty, frameKey);
        int outType = (edge > 0.35f && (hh & 1023) < 600) ? TT_BRICK : TT_STUD;
        tr_drawEdgeTile(tx, ty, s, d00, d10, d01, d11, outType, lvl > lavaLv ? TT_LAVA : TT_VOID, a, h);
      } else {
        tr_drawTile(tx, ty, s, type, a, h);
      }
    }
    if (figRow) {                                         // bottom row becomes the next top row
      for (int x = bx0; x <= bx1; x++) { cTop[x] = cBot[x]; fTop[x] = fBot[x]; }
    }
  }
}
