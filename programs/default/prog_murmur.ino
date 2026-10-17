// =====================================================================
// PROGRAM: MURMUR — flocks, schools and swarms (by Dewey) — Set 3, slot 6
//
// Hundreds of birds (or fish, or fireflies) each follow three simple
// habits — keep a little apart, fly the way your neighbours fly, stay
// close to the group — and together they move like one living cloud.
// When a hawk dives in, the birds nearest it swerve, their neighbours copy
// them, and a dark wave of panic ripples through the whole murmuration.
//
// How it works: "boids" in fixed point, with a grid so each bird only
// checks the birds near it (and stops after its 10 nearest, like real
// starlings). Panic spreads bird to bird. Each bird is drawn as a small
// shape pointing the way it flies, sized by how near it is.
//
// Presets:
//   k0 Starlings  — a murmuration at dusk, now and then a hawk
//   k1 Hawk       — two hawks hunting; waves of panic sweep the flock
//   k2 Fish School — silver fish turning together in blue water
//   k3 Bait Ball  — a spinning ball of fish, sharks circling
//   k4 Fireflies  — glowing fireflies drifting in the night
//   k5 Neon Flow  — every bird coloured by the way it is flying, long trails
//   k6 Two Flocks — two flocks in contrasting colours that won't mix
//   k7 Swarm      — a buzzing swarm of gnats round a light
//   k8 Migration  — long lines of birds crossing a dawn sky
//   k9 Galaxy     — a whirl of coloured stars
//   k10 Ribbons   — a flock that leaves ribbons of colour behind it
//   k11 Sky Show  — moves through the others every ~25 s
//
//   k12 — SCATTER: a hawk dives through the middle of the flock
//
// Knobs (centre = the preset's own wherever it says so):
//   p0 Scene / Palette (centre = preset)   p1 Speed (far left = freeze)   p2 Count
//   p3 Cohesion   p4 Alignment   p5 Separation   p6 Vision (how far each bird looks)
//   p7 Predators (0-3)   p8 Trails   p9 Bird Size   p10 Colour By (centre = preset:
//   flock, heading, speed, depth, panic)   p11 Turbulence   p12 Roam (how far the flock wanders)
//   p13 Panic (how strongly fright spreads)   p14 Depth (near birds bigger)   p15 Flocks (1-3)
// =====================================================================

#include "vg3.h"

// ─── Shared memory ────────────────────────────────────────────────────
#define MU_MAX 400
struct MuBird { int32_t x, y; int16_t vx, vy; uint8_t z, fr, sp, pad; };   // position 1/64 px, velocity 1/64 px per frame
static MuBird* mu_b;
static int16_t* mu_next;                           // grid chains
static int16_t* mu_head;                           // grid heads
#define MU_GC 40                                   // grid cell size (px)
#define MU_GX 9                                    // 320 / 40 + 1
#define MU_GY 7
static void mu_mem() {
  mu_b = (MuBird*)v3_scratch;                                        // 400 × 16 B
  mu_next = (int16_t*)(v3_scratch + MU_MAX * 16);                    // 800 B
  mu_head = (int16_t*)(v3_scratch + MU_MAX * 16 + MU_MAX * 2);       // 63 × 2 B
}

// ─── Scenes: sky gradient (4 stops), sun, bird colours ────────────────
// palette: 0-47 sky gradient, 48-63 sun / glow, 64 + hue*16 + level birds (hues 0-3 flocks, 4-10 a colour wheel, 11 hawks)
#define MU_NSC 8
struct MuScene { uint8_t sky[4][3]; uint8_t sun[3]; uint8_t bird[4][3]; uint8_t hawk[3]; uint8_t stars, sunY, sunR, light; };
static const MuScene mu_scenes[MU_NSC] = {
  {{{40,20,90},{170,50,110},{255,120,60},{255,190,90}},  {255,240,170}, {{12,6,20},{40,10,40},{20,10,30},{60,20,40}},   {120,0,10},   0, 200, 26, 0},  // 0 Dusk
  {{{20,0,10},{120,10,20},{230,60,20},{255,170,40}},     {255,250,200}, {{10,0,0},{40,0,10},{20,0,0},{60,10,0}},       {255,230,0},  0, 205, 30, 0},  // 1 Blood Sunset
  {{{120,220,255},{30,130,220},{0,60,150},{0,20,60}},    {230,255,255}, {{235,245,255},{180,220,255},{255,255,255},{140,255,230}}, {20,30,50}, 0, 0, 0, 1},   // 2 Ocean
  {{{0,8,30},{0,15,50},{5,25,70},{10,40,80}},           {255,240,180}, {{255,230,80},{180,255,90},{255,160,40},{120,255,200}}, {255,60,60}, 1, 40, 12, 1}, // 3 Night
  {{{0,0,0},{0,0,8},{4,0,16},{10,0,24}},                {255,255,255}, {{255,0,170},{0,230,255},{255,230,0},{120,255,0}}, {255,255,255}, 1, 0, 0, 1},   // 4 Neon
  {{{255,230,170},{200,220,240},{120,170,230},{60,100,190}}, {255,255,230}, {{20,30,80},{80,20,60},{10,60,70},{60,40,10}}, {200,40,0}, 0, 60, 22, 0},  // 5 Dawn
  {{{60,70,60},{110,120,100},{160,170,140},{90,100,90}}, {230,240,220}, {{255,255,255},{255,230,180},{220,255,255},{255,200,230}}, {20,20,20}, 0, 0, 0, 1},   // 6 Storm
  {{{0,0,10},{10,0,30},{25,0,50},{5,0,20}},             {255,200,255}, {{255,120,40},{80,160,255},{255,80,200},{255,255,140}}, {255,255,255}, 1, 120, 40, 1},  // 7 Galaxy
};

// ─── Presets ──────────────────────────────────────────────────────────
enum { MC_FLOCK, MC_HEAD, MC_SPEED, MC_DEPTH, MC_PANIC };
enum { MS_BIRD, MS_FISH, MS_DOT, MS_GLOW };
struct MuLook { uint8_t scene; uint16_t n; uint8_t coh, ali, sep, vis, preds, trail, size, colBy, turb, roam, panic, depth, flocks, shape, mill, speed, wrap; };
static const MuLook mu_looks[11] = {
  // sc   n  coh ali sep vis prd trl size col       turb roam pan dep fl shape     mill spd wrap
  {  0, 380, 28, 65, 75, 70,  1,  0, 55, MC_PANIC,  20, 55, 75, 60, 1, MS_BIRD,  0, 50, 0},   // Starlings
  {  1, 380, 30, 65, 75, 70,  2,  0, 55, MC_PANIC,  15, 50, 95, 55, 1, MS_BIRD,  0, 55, 0},   // Hawk
  {  2, 260, 45, 75, 50, 50,  0, 20, 55, MC_DEPTH,  15, 60, 60, 70, 1, MS_FISH,  0, 45, 0},   // Fish School
  {  2, 300, 80, 55, 40, 45,  2, 10, 50, MC_HEAD,   10, 20, 70, 60, 1, MS_FISH, 70, 50, 0},   // Bait Ball
  {  3, 120, 15, 15, 40, 40,  0, 60, 40, MC_FLOCK,  70, 80, 20, 40, 3, MS_GLOW,  0, 20, 0},   // Fireflies
  {  4, 260, 45, 70, 45, 55,  0, 85, 35, MC_HEAD,   25, 70, 40, 30, 1, MS_DOT,   0, 60, 0},   // Neon Flow
  {  6, 300, 55, 60, 50, 50,  0, 15, 45, MC_FLOCK,  20, 60, 50, 50, 2, MS_BIRD,  0, 50, 0},   // Two Flocks
  {  3, 220, 90, 10, 30, 30,  0, 25, 25, MC_SPEED,  95, 15, 30, 30, 1, MS_DOT,   0, 70, 0},   // Swarm
  {  5, 160, 25, 95, 70, 70,  0,  0, 55, MC_FLOCK,   5, 30, 30, 50, 2, MS_BIRD,  0, 45, 1},   // Migration
  {  7, 340, 40, 50, 40, 50,  0, 45, 30, MC_HEAD,   20, 10, 30, 70, 3, MS_GLOW, 85, 45, 0},   // Galaxy
  {  4, 200, 50, 75, 50, 60,  1, 100, 40, MC_HEAD,  30, 75, 50, 40, 2, MS_DOT,   0, 55, 0},   // Ribbons
};
static const char* const mu_presetNames[12] = {
  "Starlings", "Hawk", "Fish School", "Bait Ball", "Fireflies", "Neon Flow",
  "Two Flocks", "Swarm", "Migration", "Galaxy", "Ribbons", "Sky Show"
};
static const char* const mu_labels[16] = {
  "Scene", "Speed", "Count", "Cohesion", "Alignment", "Separation", "Vision", "Predators",
  "Trails", "Bird Size", "Colour By", "Turbulence", "Roam", "Panic", "Depth", "Flocks"
};
const char* prog_murmur_name() { return "MURMUR"; }
const char* prog_murmur_character() { return "Flocks of birds, schools of fish and swarms moving as one, with waves of panic when a hawk dives in"; }
const char* prog_murmur_presetName(int preset) { return (preset >= 0 && preset < 12) ? mu_presetNames[preset] : NULL; }
const char* prog_murmur_potLabel(int preset, int pot) { (void)preset; return (pot >= 0 && pot < 16) ? mu_labels[pot] : ""; }
uint8_t prog_murmur_renderHint(int preset) { (void)preset; return RENDER_PERPIXEL; }

// ─── State ────────────────────────────────────────────────────────────
static int mu_preset = -1, mu_look = 0, mu_n = 0, mu_scNow = 0, mu_scPrev = 0;
static float mu_scX = 1, mu_t = 0, mu_showT = 0, mu_roamT = 0;
static float mu_ax = 160, mu_ay = 110;             // the point the flock is drawn to
#define MU_MAXP 3
static float mu_px[MU_MAXP], mu_py[MU_MAXP], mu_pvx[MU_MAXP], mu_pvy[MU_MAXP], mu_pT[MU_MAXP];
static int mu_np = 0;
static bool mu_k12was = false;
static float mu_dive = 0;

static void mu_spawn(int i, int flocks) {
  MuBird* b = &mu_b[i];
  float a = v3_rndf() * 6.2831853f, r = 20 + v3_rndf() * 60;
  b->x = (int32_t)((mu_ax + r * cosf(a)) * 64); b->y = (int32_t)((mu_ay + r * sinf(a) * 0.6f) * 64);
  float h = v3_rndf() * 6.2831853f;
  b->vx = (int16_t)(cosf(h) * 120); b->vy = (int16_t)(sinf(h) * 120);
  b->z = (uint8_t)v3_rnd(256); b->fr = 0; b->sp = (uint8_t)(i % (flocks < 1 ? 1 : flocks));
}

static void mu_palette() {
  int xf = (int)(mu_scX * 256);
  const MuScene& A = mu_scenes[mu_scPrev];
  const MuScene& B = mu_scenes[mu_scNow];
  // sky: 48-step gradient through 4 stops
  for (int i = 0; i < 48; i++) {
    int p = i * 3 * 256 / 47, s0 = p >> 8, f = p & 255; if (s0 > 2) { s0 = 2; f = 255; }
    int c[3];
    for (int q = 0; q < 3; q++) {
      int a = v3_mix8(A.sky[s0][q], A.sky[s0 + 1][q], f), b = v3_mix8(B.sky[s0][q], B.sky[s0 + 1][q], f);
      c[q] = v3_mix8(a, b, xf);
    }
    display.setColor((uint8_t)i, c[0], c[1], c[2]);
  }
  // sun / glow: 16 steps from the sky's middle colour to the sun
  for (int i = 0; i < 16; i++) {
    int c[3];
    for (int q = 0; q < 3; q++) {
      int sa = v3_mix8(A.sky[2][q], A.sun[q], i * 17), sb = v3_mix8(B.sky[2][q], B.sun[q], i * 17);
      c[q] = v3_mix8(sa, sb, xf);
    }
    display.setColor((uint8_t)(48 + i), c[0], c[1], c[2]);
  }
  // birds: hues 0-3 flock colours, 4-11 a colour wheel round the four, hue 3 also the hawk (at 12)
  uint8_t col[12][3];
  for (int h = 0; h < 4; h++) for (int q = 0; q < 3; q++) col[h][q] = v3_mix8(A.bird[h][q], B.bird[h][q], xf);
  for (int h = 0; h < 7; h++) {                                     // the wheel (hues 4-10): round the four flock colours
    int p = h * 4 * 256 / 7, s0 = p >> 8, s1 = (s0 + 1) & 3, f = p & 255;
    for (int q = 0; q < 3; q++) col[4 + h][q] = v3_mix8(col[s0][q], col[s1][q], f);
  }
  for (int q = 0; q < 3; q++) col[11][q] = v3_mix8(A.hawk[q], B.hawk[q], xf);   // hue 11: the hawks
  bool light = (xf < 128 ? A.light : B.light);
  for (int h = 0; h < 12; h++) {
    for (int l = 0; l < 16; l++) {
      int r, g, b;
      if (light) { int k = 40 + l * 215 / 15; r = col[h][0] * k / 255; g = col[h][1] * k / 255; b = col[h][2] * k / 255; }   // glowing: dim … full
      else {                                                       // silhouettes: level = how far away (lighter, towards the sky)
        int k = l * 12;
        r = v3_mix8(col[h][0], 150, k); g = v3_mix8(col[h][1], 110, k); b = v3_mix8(col[h][2], 130, k);
      }
      display.setColor((uint8_t)(64 + h * 16 + l), r, g, b);
    }
  }
}

void prog_murmur_init() {
  mu_mem();
  v3_reset();
  mu_preset = -1;
}

static inline uint8_t mu_pix(int hue, int lev) { int i = 64 + hue * 16 + v3_clamp(lev, 0, 15); return (uint8_t)(i > 255 ? 255 : i); }

void prog_murmur_draw(int preset) {
  v3_buf = display.getBuffer();
  if (!v3_buf) return;
  if (preset < 0 || preset >= 12) preset = 0;
  mu_mem();
  v3_frame();
  float dt = v3_dt;
  bool show = preset == 11;
  if (preset != mu_preset) {
    mu_preset = preset;
    mu_look = show ? v3_rnd(11) : preset;
    mu_scNow = mu_scPrev = mu_looks[mu_look].scene; mu_scX = 1;
    mu_ax = 160; mu_ay = 110; mu_n = 0;
    for (int i = 0; i < MU_MAX; i++) mu_spawn(i, mu_looks[mu_look].flocks);
    mu_np = 0; mu_showT = 0;
  }
  if (show) {
    mu_showT += dt;
    if (mu_showT > 25.0f) { mu_showT = 0; mu_look = (mu_look + 1 + v3_rnd(10)) % 11; if (v3_centre(0)) { mu_scPrev = mu_scNow; mu_scNow = mu_looks[mu_look].scene; mu_scX = 0; } }
  }
  const MuLook& L = mu_looks[mu_look];

  // ── knobs ──
  float speed = v3_k(1) < 30 ? 0.0f : (L.speed / 50.0f) * powf(3.0f, v3_bi(1));
  int n = v3_centre(2) ? L.n : 30 + v3_k(2) * (MU_MAX - 30) / 1023;
  int coh = v3_centre(3) ? L.coh : v3_k(3) * 100 / 1023;
  int ali = v3_centre(4) ? L.ali : v3_k(4) * 100 / 1023;
  int sep = v3_centre(5) ? L.sep : v3_k(5) * 100 / 1023;
  int vis = v3_centre(6) ? L.vis : 10 + v3_k(6) * 90 / 1023;
  int preds = v3_centre(7) ? L.preds : v3_k(7) * 4 / 1024;
  int trail = v3_centre(8) ? L.trail : v3_k(8) * 100 / 1023;
  int size = v3_centre(9) ? L.size : 10 + v3_k(9) * 90 / 1023;
  int colBy = v3_pick(10, L.colBy, 5);
  int turb = v3_centre(11) ? L.turb : v3_k(11) * 100 / 1023;
  int roam = v3_centre(12) ? L.roam : v3_k(12) * 100 / 1023;
  int panic = v3_centre(13) ? L.panic : v3_k(13) * 100 / 1023;
  int depth = v3_centre(14) ? L.depth : v3_k(14) * 100 / 1023;
  int flocks = v3_centre(15) ? L.flocks : 1 + v3_k(15) * 3 / 1024;
  if (!v3_centre(0)) { int sc = v3_clamp(v3_k(0) * MU_NSC / 1024, 0, MU_NSC - 1); if (sc != mu_scNow) { mu_scPrev = mu_scNow; mu_scNow = sc; mu_scX = 0; } }
  else if (!show && mu_scNow != L.scene && mu_scX >= 1) { mu_scPrev = mu_scNow; mu_scNow = L.scene; mu_scX = 0; }
  if (mu_scX < 1) { mu_scX += dt / 2.5f; if (mu_scX > 1) mu_scX = 1; }
  if (flocks < 1) flocks = 1; if (flocks > 3) flocks = 3;
  if (n != mu_n) { for (int i = mu_n; i < n; i++) mu_spawn(i, flocks); mu_n = n; }
  for (int i = 0; i < mu_n; i++) if (mu_b[i].sp >= flocks) mu_b[i].sp = (uint8_t)(i % flocks);
  float sdt = dt * speed;
  mu_t += sdt;

  // ── the flock's roost wanders ──
  mu_roamT += sdt * 0.25f;
  float rr = roam / 100.0f;
  mu_ax = 160 + rr * (95 * sinf(mu_roamT * 0.71f) + 25 * sinf(mu_roamT * 1.9f));
  mu_ay = 108 + rr * (55 * sinf(mu_roamT * 0.53f + 1.0f) + 15 * sinf(mu_roamT * 1.37f));

  // ── predators ──
  bool k12 = keysPressed[KEY_MOD_A];
  if (k12 && !mu_k12was) mu_dive = 1.0f;
  mu_k12was = k12;
  int np = preds + (mu_dive > 0.05f ? 1 : 0);
  if (np > MU_MAXP) np = MU_MAXP;
  if (np != mu_np) { for (int p = mu_np; p < np; p++) { mu_px[p] = v3_rndf() < 0.5f ? -30 : W + 30; mu_py[p] = v3_rnd(H); mu_pvx[p] = 0; mu_pvy[p] = 0; mu_pT[p] = 0; } mu_np = np; }
  mu_dive *= expf(-dt * 0.5f);
  // find the flock's middle (for hawks to aim at)
  int32_t mx = 0, my = 0;
  for (int i = 0; i < mu_n; i += 4) { mx += mu_b[i].x >> 6; my += mu_b[i].y >> 6; }
  int cnt = (mu_n + 3) / 4; if (cnt < 1) cnt = 1;
  float fcx = (float)mx / cnt, fcy = (float)my / cnt;
  for (int p = 0; p < mu_np; p++) {
    mu_pT[p] += sdt;
    // dive at the flock's middle, overshoot, swing round and come again
    float tx = fcx + 60 * sinf(mu_pT[p] * 0.8f + p * 2.1f), ty = fcy + 40 * cosf(mu_pT[p] * 0.6f + p);
    float dx = tx - mu_px[p], dy = ty - mu_py[p], d = sqrtf(dx * dx + dy * dy) + 1;
    float acc = 260.0f;
    mu_pvx[p] += dx / d * acc * sdt; mu_pvy[p] += dy / d * acc * sdt;
    float sp = sqrtf(mu_pvx[p] * mu_pvx[p] + mu_pvy[p] * mu_pvy[p]) + 0.01f, smax = 190.0f;
    if (sp > smax) { mu_pvx[p] *= smax / sp; mu_pvy[p] *= smax / sp; }
    mu_px[p] += mu_pvx[p] * sdt; mu_py[p] += mu_pvy[p] * sdt;
    mu_px[p] = v3_clampf(mu_px[p], -60, W + 60); mu_py[p] = v3_clampf(mu_py[p], -60, H + 60);
  }

  // ── grid ──
  for (int i = 0; i < MU_GX * MU_GY; i++) mu_head[i] = -1;
  for (int i = 0; i < mu_n; i++) {
    int gx = v3_clamp((mu_b[i].x >> 6) / MU_GC, 0, MU_GX - 1), gy = v3_clamp((mu_b[i].y >> 6) / MU_GC, 0, MU_GY - 1);
    int c = gy * MU_GX + gx; mu_next[i] = mu_head[c]; mu_head[c] = (int16_t)i;
  }

  // ── flocking (fixed point) ──
  if (sdt > 0) {
    int visPx = 8 + vis * 32 / 100, vis2 = visPx * visPx;            // how far each bird looks (8-40 px)
    int sepPx = 4 + sep * 10 / 100, sep2 = sepPx * sepPx;
    int vmax = (int)(64 * (1.4f + 1.6f * L.speed / 100.0f) * speed), vmin = vmax / 2;
    if (vmax < 20) vmax = 20; if (vmin < 8) vmin = 8;
    int ptx[MU_MAXP], pty[MU_MAXP];
    for (int p = 0; p < mu_np; p++) { ptx[p] = (int)mu_px[p]; pty[p] = (int)mu_py[p]; }
    int ax = (int)mu_ax, ay = (int)mu_ay;
    int mill = L.mill;
    int frDecay = 256 - 6, frPass = 140 + panic;                     // how panic fades and passes on (/256)
    uint32_t tseed = (uint32_t)(mu_t * 3.0f);
    for (int i = 0; i < mu_n; i++) {
      MuBird* b = &mu_b[i];
      int bx = b->x >> 6, by = b->y >> 6;
      int gx = v3_clamp(bx / MU_GC, 0, MU_GX - 1), gy = v3_clamp(by / MU_GC, 0, MU_GY - 1);
      int32_t sx = 0, sy = 0, avx = 0, avy = 0, cx = 0, cy = 0; int nn = 0, other = 0; int32_t ox = 0, oy = 0;
      int frMax = 0;
      for (int yy = gy - 1; yy <= gy + 1 && nn < 10; yy++) {
        if (yy < 0 || yy >= MU_GY) continue;
        for (int xx = gx - 1; xx <= gx + 1 && nn < 10; xx++) {
          if (xx < 0 || xx >= MU_GX) continue;
          for (int j = mu_head[yy * MU_GX + xx]; j >= 0 && nn < 10; j = mu_next[j]) {
            if (j == i) continue;
            const MuBird* o = &mu_b[j];
            int dx = (o->x >> 6) - bx, dy = (o->y >> 6) - by, d2 = dx * dx + dy * dy;
            if (d2 > vis2) continue;
            if (o->sp != b->sp) {                                    // another flock: keep clear of it
              if (d2 < sep2 * 4) { ox -= dx; oy -= dy; other++; }
              continue;
            }
            nn++;
            avx += o->vx; avy += o->vy; cx += dx; cy += dy;
            if (d2 < sep2) { int k = sep2 - d2; sx -= dx * k / sep2; sy -= dy * k / sep2; }
            if (o->fr > frMax) frMax = o->fr;
          }
        }
      }
      int32_t fx = 0, fy = 0;                                        // steering, 1/64 px per frame²
      if (nn) {
        fx += ((avx / nn - b->vx) * ali) >> 9;                        // fly like your neighbours
        fy += ((avy / nn - b->vy) * ali) >> 9;
        fx += ((cx / nn) * coh) >> 6;                                 // stay close
        fy += ((cy / nn) * coh) >> 6;
        fx += (sx * sep) >> 3;                                        // but not too close
        fy += (sy * sep) >> 3;
      }
      if (other) { fx += ox * 2; fy += oy * 2; }
      // the roost pulls the flock gently (and a mill swirls round it)
      int dxa = ax - bx, dya = ay - by;
      if (mill) { fx += dxa >> 2; fy += dya >> 2; } else { fx += dxa >> 3; fy += dya >> 3; }
      if (mill) { fx += (-dya * mill) >> 8; fy += (dxa * mill) >> 8; }
      // turbulence: a slowly changing wind that differs across the sky
      if (turb) {
        uint32_t h = v3_hash((uint32_t)((bx >> 5) + (by >> 5) * 17) * 2654435761u + tseed);
        fx += (((int)(h & 255) - 128) * turb) >> 8; fy += (((int)((h >> 8) & 255) - 128) * turb) >> 8;
      }
      // edges: turn back before leaving the screen (or wrap round for migration)
      if (!L.wrap) {
        if (bx < 30) fx += (30 - bx) * 3; if (bx > W - 30) fx -= (bx - (W - 30)) * 3;
        if (by < 24) fy += (24 - by) * 3; if (by > H - 24) fy -= (by - (H - 24)) * 3;
      } else { if (by < 24) fy += (24 - by) * 3; if (by > H - 24) fy -= (by - (H - 24)) * 3; }
      // predators: flee, and panic
      int fr = b->fr * frDecay >> 8;
      int pass = frMax * frPass >> 8; if (pass > fr) fr = pass;
      for (int p = 0; p < mu_np; p++) {
        int dx = bx - ptx[p], dy = by - pty[p], d2 = dx * dx + dy * dy;
        if (d2 < 55 * 55) {
          int k = 55 * 55 - d2;
          fx += dx * k >> 10; fy += dy * k >> 10;
          if (d2 < 32 * 32) fr = 255;
        }
      }
      if (fr > 255) fr = 255;
      b->fr = (uint8_t)fr;
      if (fr > 60) { fx += (-b->vy * fr) >> 9; fy += (b->vx * fr) >> 9; }     // panicked birds bank sharply
      // integrate (per 1/60 s step, scaled by the speed knob)
      int st = (int)(sdt * 60 * 256);
      int32_t vx = b->vx + ((fx * st) >> 8), vy = b->vy + ((fy * st) >> 8);
      int32_t v2 = vx * vx + vy * vy;
      int vlim = vmax + (fr * vmax >> 9);
      if (v2 > vlim * vlim) { int v = (int)v3_isqrt((uint32_t)v2); vx = vx * vlim / v; vy = vy * vlim / v; }
      else if (v2 < vmin * vmin && v2 > 0) { int v = (int)v3_isqrt((uint32_t)v2); vx = vx * vmin / v; vy = vy * vmin / v; }
      else if (v2 == 0) vx = vmin;
      b->vx = (int16_t)vx; b->vy = (int16_t)vy;
      b->x += (vx * st) >> 8; b->y += (vy * st) >> 8;
      if (L.wrap) { if (b->x < -20 * 64) b->x += (W + 40) * 64; if (b->x > (W + 20) * 64) b->x -= (W + 40) * 64; }
      // depth drifts slowly
      int zt = (int)((v3_hash(i * 977u + (tseed >> 3)) & 255));
      b->z = (uint8_t)(b->z + ((zt - b->z) >> 6));
    }
  }

  // ── sky ──
  mu_palette();
  const MuScene& S = mu_scenes[mu_scX < 0.5f ? mu_scPrev : mu_scNow];
  for (int y = 0; y < H; y++) {
    int g = y * 47 * 256 / (H - 1);
    uint8_t a = (uint8_t)(g >> 8), b = (uint8_t)(a < 47 ? a + 1 : a);
    int f = g & 255;
    uint8_t c0 = f > 128 + 64 ? b : a, c1 = f > 64 ? b : a;           // dither between neighbouring steps
    uint32_t w0 = (y & 1) ? (uint32_t)(c0 | (c1 << 8) | (c0 << 16) | (c1 << 24)) : (uint32_t)(c1 | (c0 << 8) | (c1 << 16) | (c0 << 24));
    uint32_t* p = (uint32_t*)(v3_buf + y * W);
    for (int x = 0; x < W / 4; x++) p[x] = w0;
  }
  if (S.sunR) {                                                       // the sun (or moon) and its glow
    int sx = 160 + (int)(40 * sinf(mu_t * 0.02f)), sy = S.sunY, R = S.sunR;
    for (int y = sy - R * 2; y <= sy + R * 2; y++) {
      if ((unsigned)y >= H) continue;
      for (int x = sx - R * 2; x <= sx + R * 2; x++) {
        if ((unsigned)x >= W) continue;
        int d2 = (x - sx) * (x - sx) + (y - sy) * (y - sy);
        if (d2 > 4 * R * R) continue;
        int lev = d2 <= R * R ? 15 : 13 - (d2 - R * R) * 13 / (3 * R * R);
        if (lev > 0) { uint8_t* p = v3_buf + y * W + x; int cur = *p; int sv = 48 + lev; if (cur < 48 || sv > cur) *p = (uint8_t)sv; }
      }
    }
  }
  if (S.stars) for (int i = 0; i < 90; i++) {
    uint32_t h = v3_hash(i * 7919u + 13);
    int x = h % W, y = (h >> 9) % H;
    int tw = (int)(v3_hash(i + (uint32_t)(mu_t * 2)) & 3);
    v3_put(x, y, (uint8_t)(56 + tw * 2));
  }

  // ── birds ──
  bool light = mu_scenes[mu_scNow].light;
  int sz = 2 + size * 6 / 100;                                       // body length (px)
  int tl = trail;
  for (int i = 0; i < mu_n; i++) {
    const MuBird* b = &mu_b[i];
    int x = b->x >> 6, y = b->y >> 6;
    if (x < -10 || x > W + 10 || y < -10 || y > H + 10) continue;
    int hue, lev;
    int zk = 128 + ((b->z - 128) * depth) / 100;                       // 0 far … 255 near
    switch (colBy) {
      case MC_HEAD: { int a = v3_atan(b->vy, b->vx); hue = 4 + (a * 7 >> 10); break; }
      case MC_SPEED: { int v = (int)v3_isqrt((uint32_t)(b->vx * b->vx + b->vy * b->vy)); hue = 4 + v3_clamp(v * 7 / 256, 0, 6); break; }
      case MC_DEPTH: hue = zk < 85 ? 2 : (zk < 170 ? 0 : 1); break;
      case MC_PANIC: hue = b->fr > 90 ? 1 : 0; break;
      default: hue = b->sp; break;
    }
    if (colBy == MC_PANIC && flocks > 1) hue = b->fr > 90 ? 3 : b->sp;
    lev = light ? 6 + zk * 9 / 255 : 13 - zk * 13 / 255;            // near: brighter (glowing) or darker (silhouette)
    if (colBy == MC_PANIC && !light && b->fr > 90) lev = 0;            // a panicking bird banks: its wings go dark against the sky
    if (colBy == MC_PANIC && light && b->fr > 90) lev = 15;
    int len = sz * (64 + zk * depth / 100) / 128 + 1;
    int vx = b->vx, vy = b->vy;
    int v = (int)v3_isqrt((uint32_t)(vx * vx + vy * vy)) + 1;
    int ux = vx * 64 / v, uy = vy * 64 / v;                            // unit heading, 1/64
    uint8_t c = mu_pix(hue, lev);
    if (tl > 0) {                                                       // trail: a streak back along the flight
      int tlen = 2 + tl * len * 3 / 100 + tl * v / 600;
      uint8_t ct = mu_pix(hue, light ? lev * 2 / 3 : (lev + 15) / 2);
      v3_line(x, y, x - (ux * tlen >> 6), y - (uy * tlen >> 6), ct);
    }
    switch (L.shape) {
      case MS_BIRD: {                                                   // a chevron: two swept wings
        int w = len * 2 / 3 + 1;
        int bx = x - (ux * len >> 6), by = y - (uy * len >> 6);
        int flap = ((int)(mu_t * 12) + i) & 3; int wf = flap == 0 ? w / 2 : w;
        v3_line(x, y, bx + (uy * wf >> 6), by - (ux * wf >> 6), c);
        v3_line(x, y, bx - (uy * wf >> 6), by + (ux * wf >> 6), c);
        break;
      }
      case MS_FISH: {                                                   // a slim body and a tail
        int tx = x - (ux * len >> 6), ty = y - (uy * len >> 6);
        v3_line(x, y, tx, ty, c);
        v3_line(x + (uy >> 6), y - (ux >> 6), tx, ty, c);
        int tw = len / 3 + 1;
        v3_line(tx, ty, tx - (ux * tw >> 6) + (uy * tw >> 6), ty - (uy * tw >> 6) - (ux * tw >> 6), c);
        v3_line(tx, ty, tx - (ux * tw >> 6) - (uy * tw >> 6), ty - (uy * tw >> 6) + (ux * tw >> 6), c);
        break;
      }
      case MS_GLOW: {                                                   // a glowing point with a halo
        int r = len / 3;
        int pulse = (int)(v3_hash(i * 31u + (uint32_t)(mu_t * 3)) & 7);
        if (r > 0) v3_disc(x, y, r, mu_pix(hue, lev - 7));
        v3_put(x, y, mu_pix(hue, 12 + (pulse > 4 ? 3 : 0)));
        v3_put(x + 1, y, mu_pix(hue, lev - 2)); v3_put(x, y + 1, mu_pix(hue, lev - 2));
        break;
      }
      default: {                                                        // a dot, bigger when near
        int r = len / 4;
        if (r > 0) v3_disc(x, y, r, c); else { v3_put(x, y, c); v3_put(x + 1, y, c); }
      }
    }
  }
  // hawks
  for (int p = 0; p < mu_np; p++) {
    int x = (int)mu_px[p], y = (int)mu_py[p];
    float sp = sqrtf(mu_pvx[p] * mu_pvx[p] + mu_pvy[p] * mu_pvy[p]) + 0.01f;
    int ux = (int)(mu_pvx[p] / sp * 64), uy = (int)(mu_pvy[p] / sp * 64);
    int len = 9 + size / 12, w = len;
    uint8_t c = mu_pix(11, light ? 15 : 2);
    int bx = x - (ux * len >> 6), by = y - (uy * len >> 6);
    for (int k = -1; k <= 1; k++) {
      v3_line(x + k, y, bx + (uy * w >> 6), by - (ux * w >> 6), c);
      v3_line(x + k, y, bx - (uy * w >> 6), by + (ux * w >> 6), c);
    }
    v3_line(x, y, x - (ux * len * 3 / 2 >> 6), y - (uy * len * 3 / 2 >> 6), c);
  }
}
