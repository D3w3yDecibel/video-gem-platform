// =====================================================================
// PROGRAM: HORIZON — flying low over other worlds (by Dewey) — Set 3, slot 11
//
// Skim over an endless landscape: valleys, ridges and peaks rush towards
// you and sweep underneath as you bank through the turns. Every preset is a
// different world — a neon synthwave grid under a striped sun, red canyons,
// alpine lakes, rolling ocean, lava fields, an alien planet with two moons,
// arctic snow under the northern lights, a Tron wireframe, tropical islands,
// golden dunes, the Moon with the Earth rising.
//
// How it works: the classic "voxel space" landscape. For every screen column
// a ray runs out over a 256×256 height map (in flash), from near to far;
// each step draws the slice of ground it sees above everything nearer.
// Fixed point throughout; fog, light and grid lines are worked out per step.
//
// Presets:
//   k0 Synthwave   — a neon grid over dark hills, a striped sun on the horizon
//   k1 Canyon      — terraced red rock at sunset
//   k2 Alps        — green valleys, lakes, rock and snow
//   k3 Ocean       — rolling waves under the moon, a few islands
//   k4 Lava        — black rock and glowing, pulsing lava
//   k5 Alien       — a teal world with magenta peaks and two moons
//   k6 Arctic      — snowfields under the northern lights
//   k7 Wireframe   — the land as glowing grid lines only
//   k8 Islands     — tropical islands in a turquoise sea
//   k9 Dunes       — rolling golden sand dunes
//   k10 Moon       — grey craters, a black sky and the Earth
//   k11 Tour       — a new world every ~25 s
//
//   k12 — BOOST: a burst of speed and a swoop down towards the ground
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Palette (centre = the world's own colours)   p1 Speed (far left = hover)
//   p2 Altitude   p3 Turn (left / right; centre = straight on, the autopilot still weaves)
//   p4 Pitch (where the horizon sits)   p5 Mountains (height)   p6 Water Level
//   p7 Grid Lines   p8 Fog   p9 Sun (size)   p10 Draw Distance   p11 Terraces
//   p12 Sky (stars, aurora, moons)   p13 Autopilot (how much it weaves)
//   p14 Pulse (glow on the beat for grids and lava)   p15 Contours (height bands)
// =====================================================================

#include "vg3.h"
#include "hz_map.h"

// ─── Shared memory ────────────────────────────────────────────────────
#define HZ_MAXSTEP 256
static int16_t* hz_ybuf;                           // per column: the highest pixel drawn so far
static int32_t* hz_z;                              // per step: distance (1/256 cell)
static int32_t* hz_iz;                             // per step: projection scale
static uint8_t* hz_fog;                            // per step: fog 0-3 (× 4)
static int16_t* hz_dir;                            // per column: ray direction x, y (1/4096)
static void hz_mem() {
  hz_ybuf = (int16_t*)v3_scratch;                                   // 640 B
  hz_z = (int32_t*)(v3_scratch + 640);                              // 1 KB
  hz_iz = (int32_t*)(v3_scratch + 640 + 1024);                      // 1 KB
  hz_fog = v3_scratch + 640 + 2048;                                 // 256 B
  hz_dir = (int16_t*)(v3_scratch + 640 + 2048 + 256);               // 1280 B
}

// ─── Worlds ───────────────────────────────────────────────────────────
// palette: 0-47 sky (top → horizon), 48-63 sun / aurora / moons, 64 + band*16 + fog*4 + shade: the ground
enum { HG_NORMAL, HG_TERRACE, HG_OCEAN, HG_DUNES, HG_MOON, HG_WIRE };
struct HzWorld {
  uint8_t sky[4][3], sun[2][3];                    // sky top → horizon (4 stops); sun ramp ends
  uint8_t band[6][3];                              // ground colours: low → high (band 0 = water / lava)
  uint8_t grid[3], water[3];
  uint8_t sunR, sunY, stripes, stars, aurora, moons, kind, gridOn, waterLev, height, fog, terr, glowWater;
};
static const HzWorld hz_worlds[11] = {
  // 0 Synthwave
  {{{10,0,30},{60,0,90},{180,30,140},{255,90,120}}, {{255,240,60},{255,0,140}},
   {{20,0,60},{25,5,70},{35,8,90},{50,10,110},{70,20,130},{90,30,150}}, {255,0,200}, {20,0,60},
   46, 92, 1, 1, 0, 0, HG_NORMAL, 1, 0, 45, 35, 0, 0},
  // 1 Canyon
  {{{60,40,120},{200,90,90},{255,150,70},{255,200,120}}, {{255,250,200},{255,120,40}},
   {{90,40,30},{190,80,40},{220,110,50},{170,60,30},{240,150,80},{255,200,140}}, {255,255,255}, {60,80,140},
   20, 70, 0, 0, 0, 0, HG_TERRACE, 0, 10, 90, 40, 8, 0},
  // 2 Alps
  {{{40,90,200},{90,150,230},{170,210,250},{230,240,255}}, {{255,255,240},{255,240,200}},
   {{40,90,160},{80,150,60},{50,110,40},{110,110,100},{150,150,150},{245,250,255}}, {255,255,255}, {40,100,180},
   12, 30, 0, 0, 0, 0, HG_NORMAL, 0, 55, 100, 45, 0, 0},
  // 3 Ocean
  {{{0,0,20},{10,20,60},{40,60,110},{90,110,150}}, {{255,255,240},{180,200,230}},
   {{20,60,120},{200,190,140},{60,120,60},{90,90,90},{150,150,150},{230,230,230}}, {255,255,255}, {20,70,140},
   14, 40, 0, 1, 0, 0, HG_OCEAN, 0, 170, 70, 45, 0, 0},
  // 4 Lava
  {{{20,0,0},{80,10,0},{170,40,10},{255,90,20}}, {{255,240,120},{255,60,0}},
   {{255,90,0},{40,30,30},{55,40,35},{70,50,40},{90,60,50},{120,80,60}}, {255,200,0}, {255,80,0},
   18, 80, 0, 0, 0, 0, HG_NORMAL, 0, 70, 90, 40, 0, 1},
  // 5 Alien
  {{{0,40,30},{0,120,90},{80,220,160},{200,255,200}}, {{255,240,255},{200,60,255}},
   {{120,0,160},{0,140,140},{0,180,160},{60,200,170},{200,40,200},{255,120,255}}, {255,255,0}, {120,0,160},
   0, 0, 0, 1, 0, 2, HG_NORMAL, 0, 45, 90, 40, 0, 0},
  // 6 Arctic
  {{{0,0,15},{0,10,40},{20,40,80},{60,90,130}}, {{60,255,140},{200,60,255}},
   {{40,80,140},{190,210,230},{215,230,245},{235,240,255},{250,252,255},{255,255,255}}, {255,255,255}, {40,80,140},
   0, 0, 0, 1, 1, 0, HG_NORMAL, 0, 40, 70, 40, 0, 0},
  // 7 Wireframe
  {{{0,0,0},{0,0,10},{0,10,30},{0,30,60}}, {{0,255,255},{0,80,255}},
   {{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}}, {0,255,255}, {0,0,0},
   30, 90, 0, 1, 0, 0, HG_WIRE, 1, 0, 80, 30, 0, 0},
  // 8 Islands
  {{{40,120,230},{90,170,250},{170,220,255},{230,245,255}}, {{255,255,230},{255,240,180}},
   {{0,170,190},{240,220,160},{90,180,60},{40,140,40},{110,120,80},{160,160,130}}, {255,255,255}, {0,170,190},
   12, 30, 0, 0, 0, 0, HG_NORMAL, 0, 120, 80, 35, 0, 0},
  // 9 Dunes
  {{{60,120,220},{140,180,240},{240,220,190},{255,230,180}}, {{255,255,230},{255,220,140}},
   {{200,150,80},{210,160,90},{225,175,100},{235,190,115},{245,205,130},{255,220,150}}, {255,255,255}, {200,150,80},
   16, 50, 0, 0, 0, 0, HG_DUNES, 0, 0, 50, 35, 0, 0},
  // 10 Moon
  {{{0,0,0},{0,0,0},{2,2,6},{10,10,20}}, {{120,190,255},{20,60,160}},
   {{60,60,60},{80,80,80},{110,110,110},{140,140,140},{170,170,170},{210,210,210}}, {255,255,255}, {60,60,60},
   0, 0, 0, 1, 0, 1, HG_MOON, 0, 0, 45, 15, 0, 0},
};
static const char* const hz_presetNames[12] = {
  "Synthwave", "Canyon", "Alps", "Ocean", "Lava", "Alien",
  "Arctic", "Wireframe", "Islands", "Dunes", "Moon", "Tour"
};
static const char* const hz_labels[16] = {
  "Palette", "Speed", "Altitude", "Turn", "Pitch", "Mountains", "Water Level", "Grid Lines",
  "Fog", "Sun", "Draw Distance", "Terraces", "Sky", "Autopilot", "Pulse", "Contours"
};
const char* prog_horizon_name() { return "HORIZON"; }
const char* prog_horizon_character() { return "Flying low over other worlds: synthwave grids, canyons, oceans, lava, alien planets"; }
const char* prog_horizon_presetName(int preset) { return (preset >= 0 && preset < 12) ? hz_presetNames[preset] : NULL; }
const char* prog_horizon_potLabel(int preset, int pot) { (void)preset; return (pot >= 0 && pot < 16) ? hz_labels[pot] : ""; }
uint8_t prog_horizon_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// ─── State ────────────────────────────────────────────────────────────
static int hz_preset = -1, hz_world = 0, hz_worldPrev = 0;
static float hz_x = 128, hz_y = 128, hz_head = 0, hz_alt = 60, hz_t = 0, hz_X = 1, hz_tourT = 0, hz_boost = 0, hz_roll = 0, hz_turnV = 0;
static bool hz_k12was = false;
static uint8_t hz_band[256];                       // height → ground band (per world)

void prog_horizon_init() { hz_mem(); v3_reset(); hz_preset = -1; }

// the ground height (0-255) for a world kind
static inline int hz_ground(int kind, int ix, int iy) {
  ix &= 255; iy &= 255;
  switch (kind) {
    case HG_DUNES: return 110 + ((v3_sin(ix * 12 + iy * 5) * 50 + v3_sin(iy * 9 - ix * 3 + 300) * 30) >> 14) + (hz_map[(iy << 8) | ix] >> 3);
    case HG_MOON: { int h = 255 - hz_map[(iy << 8) | ix]; return h > 200 ? 200 + ((h - 200) >> 2) : h; }
    default: return hz_map[(iy << 8) | ix];
  }
}

static void hz_palette(int pal, float pulse) {
  const HzWorld& A = hz_worlds[hz_worldPrev];
  const HzWorld& B = hz_worlds[pal];
  int xf = (int)(hz_X * 256);
  #define HZ_MIX(a, b) v3_mix8((a), (b), xf)
  uint8_t sky[4][3];
  for (int i = 0; i < 4; i++) for (int q = 0; q < 3; q++) sky[i][q] = HZ_MIX(A.sky[i][q], B.sky[i][q]);
  v3_palGrad(sky, 4, 0, 48, false);
  for (int i = 0; i < 16; i++) {
    int r = HZ_MIX(v3_mix8(A.sun[1][0], A.sun[0][0], i * 17), v3_mix8(B.sun[1][0], B.sun[0][0], i * 17));
    int g = HZ_MIX(v3_mix8(A.sun[1][1], A.sun[0][1], i * 17), v3_mix8(B.sun[1][1], B.sun[0][1], i * 17));
    int b = HZ_MIX(v3_mix8(A.sun[1][2], A.sun[0][2], i * 17), v3_mix8(B.sun[1][2], B.sun[0][2], i * 17));
    display.setColor((uint8_t)(48 + i), r, g, b);
  }
  // ground: 12 bands (6 colours, the 6 in-between shades, band 11 = grid) × 4 fog × 4 light
  uint8_t bc[12][3];
  for (int i = 0; i < 6; i++) for (int q = 0; q < 3; q++) bc[i * 2][q] = HZ_MIX(A.band[i][q], B.band[i][q]);
  for (int i = 0; i < 5; i++) for (int q = 0; q < 3; q++) bc[i * 2 + 1][q] = v3_mix8(bc[i * 2][q], bc[i * 2 + 2][q], 128);
  for (int q = 0; q < 3; q++) { bc[0][q] = HZ_MIX(A.water[q], B.water[q]); bc[11][q] = HZ_MIX(A.grid[q], B.grid[q]); }
  bool glow = (xf < 128 ? A.glowWater : B.glowWater);
  static const int shadeK[4] = {120, 175, 220, 270};
  static const int fogK[4] = {0, 70, 150, 215};
  for (int b = 0; b < 12; b++) for (int f = 0; f < 4; f++) for (int s = 0; s < 4; s++) {
    int k = shadeK[s];
    if (b == 11 || (b == 0 && glow)) k = (int)(256 * (0.8f + 0.25f * s / 3.0f) * (0.75f + 0.35f * pulse));   // glowing: no shading
    int r = v3_clamp(bc[b][0] * k >> 8, 0, 255), g = v3_clamp(bc[b][1] * k >> 8, 0, 255), bl = v3_clamp(bc[b][2] * k >> 8, 0, 255);
    int fk = fogK[f];
    if (b == 11) fk = fk / 2;
    r = v3_mix8(r, sky[3][0], fk); g = v3_mix8(g, sky[3][1], fk); bl = v3_mix8(bl, sky[3][2], fk);
    display.setColor((uint8_t)(64 + b * 16 + f * 4 + s), r, g, bl);
  }
  #undef HZ_MIX
}

void prog_horizon_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  if (preset < 0 || preset >= 12) preset = 0;
  hz_mem();
  v3_frame();
  float dt = v3_dt;
  bool tour = preset == 11;
  if (preset != hz_preset) {
    hz_preset = preset; hz_world = tour ? v3_rnd(11) : preset; hz_worldPrev = hz_world; hz_X = 1; hz_tourT = 0;
  }
  if (tour) { hz_tourT += dt; if (hz_tourT > 25.0f) { hz_tourT = 0; hz_worldPrev = hz_world; hz_world = (hz_world + 1 + v3_rnd(10)) % 11; hz_X = 0; } }
  if (hz_X < 1) { hz_X += dt / 2.5f; if (hz_X > 1) { hz_X = 1; hz_worldPrev = hz_world; } }
  const HzWorld& Wd = hz_worlds[hz_world];

  // ── knobs ──
  float speed = v3_k(1) < 30 ? 0.0f : 9.0f * powf(3.0f, v3_bi(1));             // cells per second
  float alt = 14.0f * powf(2.5f, v3_bi(2));
  float turn = v3_centre(3) ? 0.0f : v3_bi(3);
  float pitch = v3_bi(4);
  float hsc = (v3_centre(5) ? Wd.height : 20 + v3_k(5) * 160 / 1023) / 100.0f;
  int water = v3_centre(6) ? Wd.waterLev : v3_k(6) * 220 / 1023;
  bool grid = v3_centre(7) ? Wd.gridOn : v3_k(7) > 600;
  float fog = v3_centre(8) ? Wd.fog / 100.0f : v3_u(8);
  float sunK = v3_centre(9) ? 1.0f : 0.3f + v3_u(9) * 1.6f;
  float dist = 150.0f * powf(1.7f, v3_bi(10));
  int terr = v3_centre(11) ? Wd.terr : v3_k(11) * 24 / 1023;
  float skyK = v3_centre(12) ? 1.0f : v3_u(12) * 2.0f;
  float auto_ = v3_centre(13) ? 0.5f : v3_u(13);
  float pulseK = v3_centre(14) ? (Wd.gridOn || Wd.glowWater ? 0.5f : 0.0f) : v3_u(14);
  float contours = v3_centre(15) ? 0.0f : v3_u(15);
  int palW = v3_centre(0) ? hz_world : v3_clamp(v3_k(0) * 11 / 1024, 0, 10);

  // ── fly ──
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !hz_k12was) hz_boost = 1.0f;
  hz_k12was = k12;
  hz_boost *= expf(-dt * 0.8f);
  hz_t += dt;
  float weave = auto_ * (0.55f * sinf(hz_t * 0.23f) + 0.35f * sinf(hz_t * 0.61f + 1.3f));
  float tv = (turn + weave) * 0.9f;
  hz_turnV += (tv - hz_turnV) * v3_clampf(dt * 1.5f, 0, 1);
  hz_head += hz_turnV * dt * (0.6f + speed / 20.0f);
  hz_roll = hz_turnV * 0.35f;
  float v = speed * (1.0f + 2.5f * hz_boost);
  hz_x += cosf(hz_head) * v * dt; hz_y += sinf(hz_head) * v * dt;
  if (hz_x > 1e6f || hz_x < -1e6f) hz_x = fmodf(hz_x, 256.0f);
  if (hz_y > 1e6f || hz_y < -1e6f) hz_y = fmodf(hz_y, 256.0f);
  // keep above the ground just ahead
  int kind = Wd.kind;
  float gh = 0;
  for (int k = 0; k < 6; k++) {
    int sx = (int)(hz_x + cosf(hz_head) * k * 2), sy = (int)(hz_y + sinf(hz_head) * k * 2);
    int h = hz_ground(kind, sx, sy);
    if (h < water) h = water;
    float hw = h * hsc * 0.25f; if (hw > gh) gh = hw;
  }
  float want = gh + alt * (1.0f - 0.6f * hz_boost);
  hz_alt += (want - hz_alt) * v3_clampf(dt * 2.0f, 0, 1);
  if (hz_alt < gh + 2) hz_alt = gh + 2;

  // ── colours ──
  float pulse = pulseK > 0 ? 0.5f + 0.5f * sinf(hz_t * 6.2832f * 1.0f) * pulseK : 0.5f;
  if (palW != hz_world && hz_X >= 1) { hz_worldPrev = palW; }
  hz_palette(palW, pulse);
  // height → band (with terraces and contour bands)
  {
    for (int h = 0; h < 256; h++) {
      int b;
      if (h <= water && water > 0) b = 0;
      else { int u = (h - water) * 10 / (256 - water + 1); b = 1 + v3_clamp(u, 0, 9); }
      if (contours > 0.05f && b > 0 && ((h >> 3) & 3) == 0 && (h & 7) < (int)(1 + contours * 3)) b = 11;
      hz_band[h] = (uint8_t)b;
    }
  }

  // ── sky ──
  int hzY = 70 + (int)(pitch * 60);                                   // the horizon line on screen
  int rollPx = (int)(hz_roll * 60);                                   // bank: the horizon tilts
  for (int y = 0; y < H; y++) {
    int g = (y - (hzY - 100)) * 47 / 100; g = v3_clamp(g, 0, 47);
    int g2 = v3_clamp((y + 1 - (hzY - 100)) * 47 / 100, 0, 47);
    uint8_t a = (uint8_t)g, b = (uint8_t)g2;
    uint32_t w = (y & 1) ? (uint32_t)(a | (b << 8) | (a << 16) | (b << 24)) : (uint32_t)(b | (a << 8) | (b << 16) | (a << 24));
    uint32_t* p = (uint32_t*)(v3_buf + y * W);
    for (int x = 0; x < W / 4; x++) p[x] = w;
  }
  const HzWorld& Ws = hz_worlds[palW];
  if (Ws.stars && skyK > 0.1f) {
    int ns = (int)(70 * skyK);
    for (int i = 0; i < ns; i++) {
      uint32_t h = v3_hash(i * 7919u + 3);
      int x = (int)((h % 640u) - (uint32_t)(hz_head * 60.0f) % 640u + 640) % 640 - 160;
      int y = (int)((h >> 10) % (uint32_t)(hzY > 10 ? hzY : 10));
      if ((unsigned)x < W && y < hzY - 4) v3_put(x, y, (uint8_t)(56 + (int)(v3_hash(i + (uint32_t)(hz_t * 3)) & 7)));
    }
  }
  if (Ws.aurora && skyK > 0.1f) {                                     // curtains of light, swaying (integer sines: 1024 per turn)
    int ta = (int)(hz_t * 163), ha = (int)(hz_head * 326);
    for (int x = 0; x < W; x++) {
      int u = x * 3 + ha;                                             // ≈ 0.02 radians per pixel
      int top = 20 + ((15 * v3_sin(u * 13 / 10 + ta * 4 / 10) + 10 * v3_sin(u * 31 / 10 - ta * 7 / 10)) >> 14);
      int len = (int)(((30 << 14) + 25 * v3_sin(u * 23 / 10 + ta / 2)) * skyK) >> 14;
      int fold = (((x * 5 + (int)(hz_t * 9) + ((4 * v3_sin(u * 7)) >> 14)) & 7) < 3) ? 5 : 0;   // curtain folds
      int dim = ((v3_sin(u * 5 + ta) + 16384) >> 13) + fold;
      for (int y = top; y < top + len && y < hzY - 2; y++) {
        if (y < 0) continue;
        int k = 15 - (y - top) * 15 / (len + 1) - dim;
        int idx = 48 + v3_clamp(k, 0, 15);
        uint8_t* p = v3_buf + y * W + x; if (*p < idx) *p = (uint8_t)idx;
      }
    }
  }
  if (Ws.sunR) {                                                      // the sun (striped, for synthwave)
    int R = (int)(Ws.sunR * sunK), sx = 160, sy = hzY - Ws.sunY / 3;   // (the sun sits ahead, low over the horizon)
    for (int y = sy - R; y <= sy + R; y++) {
      if ((unsigned)y >= H) continue;
      int dy = y - sy; int hw = (int)v3_isqrt((uint32_t)(R * R - dy * dy));
      if (Ws.stripes && dy > -R / 5) { int band = (dy + R) * 8 / (2 * R); if (((dy + R) % 9) < band / 2 + 1) continue; }   // the gaps widen downwards
      int idx = 48 + v3_clamp(15 - (dy + R) * 15 / (2 * R + 1), 0, 15);
      v3_hspan(sx - hw, sx + hw, y, (uint8_t)idx);
    }
  }
  for (int m = 0; m < Ws.moons && skyK > 0.1f; m++) {                // moons / the Earth
    int R = m == 0 ? 16 : 9, mx = 70 + m * 150 + (int)(sinf(hz_head) * 30), my = 35 + m * 20;
    for (int y = -R; y <= R; y++) { int hw = (int)v3_isqrt((uint32_t)(R * R - y * y)); for (int x = -hw; x <= hw; x++) {
      int lit = 8 + ((x + y) * 7) / R + (int)((v3_hash((x + 40) * 13 + (y + 40) * 7 + m) & 3));
      v3_put(mx + x, my + y, (uint8_t)(48 + v3_clamp(lit, 2, 15)));
    } }
  }

  // ── ground ──
  // the steps out from the camera: close together near, further apart far away
  int nst = 0;
  {
    float z = 0.8f, dz = 0.14f;
    int fogLim = (int)(4 * fog);
    while (z < dist && nst < HZ_MAXSTEP) {
      hz_z[nst] = (int32_t)(z * 256);
      hz_iz[nst] = (int32_t)(150.0f * 256.0f / z);                   // focal length 150 px
      float u = z / dist;
      int f = (int)(u * u * 4.0f * fog * 1.15f); hz_fog[nst] = (uint8_t)(v3_clamp(f, 0, 3) * 4);
      (void)fogLim;
      nst++; z += dz; dz *= 1.03f;
    }
  }
  float ch = cosf(hz_head), sh = sinf(hz_head);
  for (int x = 0; x < W; x++) {                                       // ray directions, 90° across the screen
    float a = (x - 160) / 160.0f;
    hz_dir[x * 2] = (int16_t)((ch - sh * a) * 4096); hz_dir[x * 2 + 1] = (int16_t)((sh + ch * a) * 4096);
    hz_ybuf[x] = H;
  }
  int32_t camx = (int32_t)(hz_x * 256), camy = (int32_t)(hz_y * 256);
  int32_t camh = (int32_t)(hz_alt * 16);                              // 1/16 cell
  int hs = (int)(hsc * 64);                                           // height scale: h × hs / 256 × 16 → 1/16 cell
  int waveT = (int)(hz_t * 6);
  int tmask = terr > 1 ? ~(terr - 1) : ~0;
  bool wire = kind == HG_WIRE;
  int gridW = grid ? 40 : -1;
  for (int x = 0; x < W; x++) {
    int32_t dxr = hz_dir[x * 2], dyr = hz_dir[x * 2 + 1];
    int ybuf = hz_ybuf[x];
    int roll = ((x - 160) * rollPx) / 160;
    uint8_t* col = v3_buf + x;
    for (int s = 0; s < nst; s++) {
      int32_t z = hz_z[s];
      int32_t wx = camx + ((dxr * z) >> 12), wy = camy + ((dyr * z) >> 12);
      int ix = (wx >> 8) & 255, iy = (wy >> 8) & 255;
      int h = kind == HG_NORMAL || kind == HG_OCEAN || kind == HG_WIRE || kind == HG_TERRACE ? hz_map[(iy << 8) | ix] : hz_ground(kind, ix, iy);
      if (terr > 1) h &= tmask;
      int band, shade;
      if (h <= water && water > 0) {                                  // water (or lava): flat, with moving waves
        int wv = hz_map[(((iy + waveT) & 255) << 8) | ((ix + (waveT >> 1)) & 255)];
        h = water + (kind == HG_OCEAN ? (wv >> 5) : 0);
        band = 0; shade = (wv & 15) == 0 ? 3 : ((wv & 15) == 1 ? 1 : 2);   // mostly calm, with glints
      } else {
        int hx = hz_map[(iy << 8) | ((ix + 1) & 255)];
        if (kind == HG_MOON) hx = 255 - hx;
        int d = hx - h;                                               // light from one side
        shade = d > 5 ? 3 : (d > 0 ? 2 : (d > -5 ? 1 : 0));
        band = hz_band[h];
      }
      int32_t hw = (h * hs) >> 4;                                     // 1/16 cell
      int y = hzY + roll + (int)(((camh - hw) * hz_iz[s]) >> 12);
      if (y < ybuf) {
        if (y < 0) y = 0;
        uint8_t pix;
        bool onGrid = gridW > 0 && ((wx & 0x7FF) < gridW + (s >> 1) * 6 || (wy & 0x7FF) < gridW + (s >> 1) * 6);
        if (wire) pix = onGrid ? (uint8_t)(64 + 11 * 16 + hz_fog[s] + 3) : (uint8_t)(64 + hz_fog[s]);
        else if (onGrid) pix = (uint8_t)(64 + 11 * 16 + hz_fog[s] + 3);
        else pix = (uint8_t)(64 + band * 16 + hz_fog[s] + shade);
        uint8_t* p = col + y * W;
        for (int yy = y; yy < ybuf; yy++, p += W) *p = pix;
        ybuf = y;
        if (ybuf <= 0) break;
      }
    }
    hz_ybuf[x] = (int16_t)ybuf;
  }
}
