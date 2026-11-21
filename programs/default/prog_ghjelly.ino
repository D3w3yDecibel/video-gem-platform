// =====================================================================
// PROGRAM: WOBBLERS — soft-body jelly creatures (by Dewey) — Set 8 slot 10
//
// Replaces BOUNCE. A little world of squishy, iridescent jelly creatures
// with googly eyes. Each one is a real soft body: a ring of 16 points held
// together by springs and an inner pressure, so they squash when they land,
// bulge when they bump, sag when they're soft, and hop about on their own.
// Their eyes roll with real inertia; their skin shimmers with oil-slick
// colour. Gravity can point anywhere: they float, tumble round the screen,
// fall upward, or rise and sink like a lava lamp.
//
// How it works: position-based soft bodies — Verlet steps, then each body's
// ring lengths, radius and area are pulled back toward rest (how hard = the
// Wobble knob), bodies push each other's points out, walls bounce. The
// jellies are filled scanline by scanline at full resolution with a
// per-pixel dome shade, highlight, outline and shifting hue; the warped
// funhouse backdrop is a soft field on the 80 × 60 grid.
//
// Presets: k0 Jelly Party · k1 Zero-G · k2 Tumble · k3 Big Boy · k4 Swarm ·
//          k5 Bounce House · k6 Slime Pit · k7 Disco · k8 Stack · k9 Lava Lamp ·
//          k10 Moon · k11 Tour
//   k12 — BOING!: everybody jumps at once
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom (jelly size, live) · p3 Colour Flow · p4 Hue Spread
//   p5 Jellies · p6 World (swirl/dots/checker/void) · p7 Gravity (◄ up · 0 · down ►) ·
//   p8 Wobble · p9 Hops · p10 Eyes
//   p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8JE_SWIRL, H8JE_DOTS, H8JE_CHECK, H8JE_VOID, H8JE_NWORLD };
enum { H8JE_NORMAL, H8JE_TUMBLE, H8JE_FLIP, H8JE_LAVA, H8JE_BIG };
struct H8JeLook { uint8_t n, rmin, rmax, eyes, world, grav, wob, hops, bounce, mode, pal; };
static const H8JeLook h8je_looks[11] = {
  //  n rmin rmax eyes world        grav wob hops bnc mode          pal
  {   6, 22, 40,  2, H8JE_SWIRL,     80, 55,  55, 40, H8JE_NORMAL, V8P_CANDY    },   // Jelly Party
  {   7, 18, 34,  0, H8JE_DOTS,      50, 50,  40, 85, H8JE_NORMAL, V8P_VAPOR    },   // Zero-G
  {   6, 20, 36,  2, H8JE_CHECK,     80, 55,  30, 40, H8JE_TUMBLE, V8P_NEON     },   // Tumble
  {   5, 16, 24,  2, H8JE_SWIRL,     80, 60,  50, 40, H8JE_BIG,    V8P_ACID     },   // Big Boy
  {  12, 10, 16,  0, H8JE_VOID,      78, 45,  70, 50, H8JE_NORMAL, V8P_RAINBOW  },   // Swarm
  {   6, 20, 32,  2, H8JE_CHECK,     85, 35,  80, 92, H8JE_NORMAL, V8P_RETROPOP },   // Bounce House
  {   5, 26, 42,  0, H8JE_DOTS,      82, 92,  25, 15, H8JE_NORMAL, V8P_TOXIC    },   // Slime Pit
  {   7, 18, 32,  2, H8JE_VOID,      80, 55,  45, 50, H8JE_FLIP,   V8P_ELECTRIC },   // Disco
  {   9, 20, 32,  1, H8JE_SWIRL,     85, 25,  10, 10, H8JE_NORMAL, V8P_SHERBET  },   // Stack
  {   7, 20, 34,  2, H8JE_SWIRL,     70, 60,   0, 20, H8JE_LAVA,   V8P_LAVA     },   // Lava Lamp
  {   6, 18, 30,  3, H8JE_VOID,      60, 50,  85, 60, H8JE_NORMAL, V8P_GALAXY   },   // Moon
};
static const char* const h8je_names[12] = { "Jelly Party", "Zero-G", "Tumble", "Big Boy", "Swarm", "Bounce House", "Slime Pit", "Disco", "Stack", "Lava Lamp", "Moon", "Tour" };
static const char* const h8je_lbl[6] = { "Jellies", "World", "Gravity", "Wobble", "Hops", "Eyes" };
const char* prog_ghjelly_name() { return "WOBBLERS"; }
const char* prog_ghjelly_character() { return "Wobblers: squishy soft-body jelly creatures with googly eyes, bouncing in a funhouse"; }
const char* prog_ghjelly_presetName(int p) { return (p >= 0 && p < 12) ? h8je_names[p] : NULL; }
const char* prog_ghjelly_potLabel(int p, int pot) { (void)p; return v8_label(pot, h8je_lbl); }
uint8_t prog_ghjelly_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

#define H8JE_MAXB 14
#define H8JE_N 16
static float h8je_x[H8JE_MAXB][H8JE_N], h8je_y[H8JE_MAXB][H8JE_N], h8je_ox[H8JE_MAXB][H8JE_N], h8je_oy[H8JE_MAXB][H8JE_N];
static float h8je_r[H8JE_MAXB], h8je_cx[H8JE_MAXB], h8je_cy[H8JE_MAXB], h8je_vx[H8JE_MAXB], h8je_vy[H8JE_MAXB];
static float h8je_px[H8JE_MAXB], h8je_py[H8JE_MAXB], h8je_pvx[H8JE_MAXB], h8je_pvy[H8JE_MAXB];   // googly pupils (−1 … 1)
static float h8je_hopT[H8JE_MAXB], h8je_blink[H8JE_MAXB], h8je_heat[H8JE_MAXB], h8je_mood[H8JE_MAXB];
static uint8_t h8je_eyes[H8JE_MAXB], h8je_hue[H8JE_MAXB];
static int h8je_n = 0, h8je_capI = 0;
static float h8je_wob = 0.5f, h8je_grav = 0.8f, h8je_hops = 0.5f, h8je_eyeK = 0.5f;
static float h8je_gAng = 1.5708f, h8je_flipT = 0, h8je_t = 0, h8je_capT = 0, h8je_zoom = 1;
static const char* const h8je_caps[6] = { "BOING!", "WHEE!", "BLORP!", "SPLAT!", "WOBBLE!", "BOOP!" };
static const float h8je_cos16[H8JE_N] = { 1, 0.9239f, 0.7071f, 0.3827f, 0, -0.3827f, -0.7071f, -0.9239f, -1, -0.9239f, -0.7071f, -0.3827f, 0, 0.3827f, 0.7071f, 0.9239f };

static void h8je_spawn(int b, int look) {
  const H8JeLook& L = h8je_looks[look];
  float r = L.rmin + v3_rndf() * (L.rmax - L.rmin);
  int eyes = L.eyes ? L.eyes : 1 + v3_rnd(3);
  if (L.mode == H8JE_BIG && b == 0) { r = 66; eyes = 7; }
  h8je_r[b] = r; h8je_eyes[b] = (uint8_t)eyes;
  h8je_hue[b] = (uint8_t)((b * 5 + v3_rnd(3)) & 15);
  float cx = r + 4 + v3_rndf() * (W - 2 * r - 8), cy = r + 4 + v3_rndf() * (H * 0.6f - r);
  float vx = (v3_rndf() - 0.5f) * 3.0f, vy = (v3_rndf() - 0.5f) * 3.0f;
  for (int i = 0; i < H8JE_N; i++) {
    float c = h8je_cos16[i], s = h8je_cos16[(i + 12) & 15];
    h8je_x[b][i] = cx + r * c; h8je_y[b][i] = cy + r * s;
    h8je_ox[b][i] = h8je_x[b][i] - vx; h8je_oy[b][i] = h8je_y[b][i] - vy;
  }
  h8je_cx[b] = cx; h8je_cy[b] = cy; h8je_vx[b] = h8je_vy[b] = 0;
  h8je_px[b] = h8je_py[b] = h8je_pvx[b] = h8je_pvy[b] = 0;
  h8je_hopT[b] = 0.5f + v3_rndf() * 3.0f; h8je_blink[b] = 1 + v3_rndf() * 5; h8je_heat[b] = v3_rndf(); h8je_mood[b] = 0;
}
void prog_ghjelly_init() { v8_init(); h8je_n = 0; h8je_capT = 0; h8je_gAng = 1.5708f; }

// kick every point of body b by (jx, jy) px/s over a substep of h seconds
static void h8je_kick(int b, float jx, float jy, float h) {
  for (int i = 0; i < H8JE_N; i++) { h8je_ox[b][i] -= jx * h; h8je_oy[b][i] -= jy * h; }
}

// one physics substep for all bodies
static void h8je_step(float h, float gx, float gy, float restit, int mode) {
  float soft = h8je_wob;
  float hard = 1 - soft, sRing = 0.08f + hard * hard * 0.9f, sRad = hard * hard * hard * 0.5f, sArea = 0.45f + hard * 0.4f, sSm = 0.1f + soft * soft * 0.35f;
  float damp = 0.998f, h2 = h * h;
  for (int b = 0; b < h8je_n; b++) {
    float ax = gx, ay = gy;
    if (mode == H8JE_LAVA) { float k = 1.0f - 2.0f * h8je_heat[b]; ax *= k * 0.6f; ay *= k * 0.6f; }
    float* X = h8je_x[b]; float* Y = h8je_y[b]; float* OX = h8je_ox[b]; float* OY = h8je_oy[b];
    for (int i = 0; i < H8JE_N; i++) {
      float vx = (X[i] - OX[i]) * damp, vy = (Y[i] - OY[i]) * damp;
      OX[i] = X[i]; OY[i] = Y[i];
      X[i] += vx + ax * h2; Y[i] += vy + ay * h2;
    }
    float r = h8je_r[b] * h8je_zoom, L0 = r * 0.3902f, A0 = 3.0615f * r * r;   // ring rest length, area of the 16-gon
    float r2 = r * r, L02 = L0 * L0, iR = 1.0f / r2, iL = 0.25f / L02;
    // one pass of constraints, no square roots or divides: (L − d)/d ≈ (L² − d²)/2L² near rest
    {
      float cx = 0, cy = 0; for (int i = 0; i < H8JE_N; i++) { cx += X[i]; cy += Y[i]; } cx *= 1.0f / H8JE_N; cy *= 1.0f / H8JE_N;
      for (int i = 0; i < H8JE_N; i++) {                                   // radius (hard limits: 0.35 r … 1.5 r)
        float dx = X[i] - cx, dy = Y[i] - cy, d2 = dx * dx + dy * dy;
        float st = (d2 > 2.25f * r2 || d2 < 0.12f * r2) ? 1.0f : sRad;
        float k = v3_clampf((r2 - d2) * iR * 0.5f, -0.5f, 0.5f) * st;            // ≈ (r − d)/d
        X[i] += dx * k; Y[i] += dy * k;
      }
      for (int i = 0; i < H8JE_N; i++) {                                   // skin
        int j = (i + 1) & (H8JE_N - 1);
        float dx = X[j] - X[i], dy = Y[j] - Y[i], d2 = dx * dx + dy * dy;
        float k = v3_clampf((L02 - d2) * iL, -0.3f, 0.3f) * (d2 > 2.56f * L02 ? 0.9f : sRing);   // ≈ (L − d)/2d
        X[i] -= dx * k; Y[i] -= dy * k; X[j] += dx * k; Y[j] += dy * k;
      }
      { float px0 = X[H8JE_N - 1], py0 = Y[H8JE_N - 1], fx = X[0], fy = Y[0];   // smooth the skin a little (no crinkles)
        for (int i = 0; i < H8JE_N; i++) {
          float nx = i + 1 < H8JE_N ? X[i + 1] : fx, ny = i + 1 < H8JE_N ? Y[i + 1] : fy, cxx = X[i], cyy = Y[i];
          X[i] += ((px0 + nx) * 0.5f - X[i]) * sSm; Y[i] += ((py0 + ny) * 0.5f - Y[i]) * sSm;
          px0 = cxx; py0 = cyy;
        } }
      float A = 0;                                                         // pressure: keep the area
      for (int i = 0; i < H8JE_N; i++) { int j = (i + 1) & (H8JE_N - 1); A += X[i] * Y[j] - X[j] * Y[i]; }
      A = fabsf(A) * 0.5f + 1.0f;
      float f = 1.0f + v3_clampf((A0 - A) / (2.0f * A), -0.2f, 0.2f) * sArea;   // ≈ √(A0/A)
      for (int i = 0; i < H8JE_N; i++) { X[i] = cx + (X[i] - cx) * f; Y[i] = cy + (Y[i] - cy) * f; }
    }
    float cx = 0, cy = 0; for (int i = 0; i < H8JE_N; i++) { cx += X[i]; cy += Y[i]; }
    h8je_cx[b] = cx * (1.0f / H8JE_N); h8je_cy[b] = cy * (1.0f / H8JE_N);
  }
  // bodies push each other apart
  for (int a = 0; a < h8je_n; a++) for (int b = 0; b < h8je_n; b++) {
    if (a == b) continue;
    float rb = h8je_r[b] * h8je_zoom * 0.97f, ra = h8je_r[a] * h8je_zoom;
    float dcx = h8je_cx[a] - h8je_cx[b], dcy = h8je_cy[a] - h8je_cy[b];
    if (dcx * dcx + dcy * dcy > (ra + rb) * (ra + rb) * 1.2f) continue;
    float sx = 0, sy = 0;
    for (int i = 0; i < H8JE_N; i++) {
      float dx = h8je_x[a][i] - h8je_cx[b], dy = h8je_y[a][i] - h8je_cy[b], d2 = dx * dx + dy * dy;
      if (d2 >= rb * rb) continue;
      float pen = (rb * rb - d2) / (2.0f * d2 + 1.0f); if (pen > 1.0f) pen = 1.0f;   // ≈ (rb − d)/d
      h8je_x[a][i] += dx * pen * 0.6f; h8je_y[a][i] += dy * pen * 0.6f;
      sx += dx * pen * 0.4f; sy += dy * pen * 0.4f;
    }
    if (sx != 0 || sy != 0) { sx *= 1.0f / H8JE_N; sy *= 1.0f / H8JE_N; for (int i = 0; i < H8JE_N; i++) { h8je_x[b][i] -= sx; h8je_y[b][i] -= sy; } }
  }
  // walls: bounce, with a little friction along them
  for (int b = 0; b < h8je_n; b++) for (int i = 0; i < H8JE_N; i++) {
    float& x = h8je_x[b][i]; float& y = h8je_y[b][i]; float& ox = h8je_ox[b][i]; float& oy = h8je_oy[b][i];
    if (x < 1)     { float v = x - ox; x = 1;         ox = x + v * restit; oy = y - (y - oy) * 0.9f; }
    if (x > W - 2) { float v = x - ox; x = W - 2;     ox = x + v * restit; oy = y - (y - oy) * 0.9f; }
    if (y < 1)     { float v = y - oy; y = 1;         oy = y + v * restit; ox = x - (x - ox) * 0.9f; }
    if (y > H - 2) { float v = y - oy; y = H - 2;     oy = y + v * restit; ox = x - (x - ox) * 0.9f; }
  }
}

// a jelly at full resolution: dome shade, highlight, oil-slick hue, dark outline
__attribute__((optimize("O2"))) static void h8je_fill(int b, int hueShift) {
  int32_t X[H8JE_N], Y[H8JE_N]; int ymin = 1 << 20, ymax = -(1 << 20);
  float mr = 0;
  for (int i = 0; i < H8JE_N; i++) {
    X[i] = (int32_t)(h8je_x[b][i] * 16); Y[i] = (int32_t)(h8je_y[b][i] * 16);
    if (Y[i] < ymin) ymin = Y[i]; if (Y[i] > ymax) ymax = Y[i];
    float dx = h8je_x[b][i] - h8je_cx[b], dy = h8je_y[b][i] - h8je_cy[b]; mr += dx * dx + dy * dy;
  }
  int R = (int)sqrtf(mr / H8JE_N) + 1, icx = (int)h8je_cx[b], icy = (int)h8je_cy[b];
  int32_t inv = (1 << 16) / (R * R + 1);
  int hlx = R * 35 / 100, hly = R * 40 / 100;
  int hb = h8je_hue[b] * 16 + hueShift;
  int y0 = (ymin + 15) >> 4, y1 = ymax >> 4;
  if (y0 < 0) y0 = 0; if (y1 > H - 1) y1 = H - 1;
  for (int y = y0; y <= y1; y++) {
    int32_t yq = y * 16 + 8, xl = 1 << 20, xr = -(1 << 20);
    for (int i = 0; i < H8JE_N; i++) {
      int j = (i + 1) & (H8JE_N - 1);
      int32_t ya = Y[i], yb = Y[j];
      if ((ya <= yq && yb > yq) || (yb <= yq && ya > yq)) {
        int32_t x = X[i] + (yq - ya) * (X[j] - X[i]) / (yb - ya);
        if (x < xl) xl = x; if (x > xr) xr = x;
      }
    }
    if (xl > xr) continue;
    int xs = xl >> 4, xe = xr >> 4;
    bool edgeRow = (y - y0 < 2 && y0 > 0) || (y1 - y < 2 && y1 < H - 1) || (yq - ymin < 32) || (ymax - yq < 32);
    if (xs < 0) xs = 0; if (xe > W - 1) xe = W - 1;
    uint8_t* row = v3_buf + y * W;
    int dy = y - icy, dy2 = dy * dy, hy = dy + hly, hy2 = hy * hy;
    const uint8_t* bay = v8_bayer + ((y & 3) << 2);
    for (int x = xs; x <= xe; x++) {
      if (edgeRow || x - (xl >> 4) < 2 || (xr >> 4) - x < 2) { row[x] = (uint8_t)(0x10 | ((hb >> 4) & 15)); continue; }
      int dx = x - icx;
      int d2 = ((dx * dx + dy2) * inv) >> 8; if (d2 > 300) d2 = 300;
      int l16 = 222 - ((d2 * 125) >> 8);
      int hx = dx + hlx, hd = ((hx * hx + hy2) * inv) >> 8;
      if (hd < 26) l16 += (26 - hd) * 5;
      int hu = hb + (d2 >> 2) + ((dx * 3 - dy * 5) >> 3);
      int bb = bay[x & 3];
      int lev = (l16 + bb) >> 4; if (lev > 15) lev = 15; if (lev < 1) lev = 1;
      row[x] = (uint8_t)((lev << 4) | (((hu + bb) >> 4) & 15));
    }
  }
}

// googly eyes and a mouth
static void h8je_face(int b, float eyeScale) {
  float bx0 = 1e9f, bx1 = -1e9f, by0 = 1e9f, by1 = -1e9f;
  for (int i = 0; i < H8JE_N; i++) { float x = h8je_x[b][i], y = h8je_y[b][i]; if (x < bx0) bx0 = x; if (x > bx1) bx1 = x; if (y < by0) by0 = y; if (y > by1) by1 = y; }
  float r = h8je_r[b] * h8je_zoom;
  float sx = (bx1 - bx0) / (2 * r), sy = (by1 - by0) / (2 * r);
  float cx = (bx0 + bx1) * 0.5f, cy = (by0 + by1) * 0.5f;
  int n = h8je_eyes[b];
  if (eyeScale < 0.15f) n = 0;
  else if (eyeScale > 0.85f) n = n + 2 + (b & 1);                        // the Eyes knob far right: more eyes. Weird.
  if (n > 9) n = 9;
  uint8_t dark = (uint8_t)(0x10 | (h8je_hue[b] & 15)), white = (uint8_t)(0xF0 | ((h8je_hue[b] + 8) & 15));
  bool blink = h8je_blink[b] < 0.13f;
  float eRbase = r * 0.5f / sqrtf((float)n + 1.0f) * (0.7f + eyeScale * 0.6f);
  for (int e = 0; e < n; e++) {
    float ux, uy;
    if (n == 1) { ux = 0; uy = -0.25f; }
    else { float a = 3.14159f * (1.15f + 0.7f * e / (n - 1)); ux = 0.45f * cosf(a); uy = 0.45f * sinf(a) + 0.05f; }
    if (n > 5 && (e & 1)) uy += 0.22f;                                   // a second row
    float er = eRbase * (0.85f + 0.3f * ((v3_hash((uint32_t)(b * 31 + e)) & 255) / 255.0f));
    int ex = (int)(cx + ux * r * sx), ey = (int)(cy + uy * r * sy), eR = (int)er;
    if (eR < 2) eR = 2;
    if (blink) { v8_thick(ex - eR, ey, ex + eR, ey, eR > 6 ? 2 : 1, dark); continue; }
    v3_disc(ex, ey, eR + 1, dark);
    v3_disc(ex, ey, eR, white);
    int pr = eR / 2 > 1 ? eR / 2 : 1;
    int ppx = ex + (int)(h8je_px[b] * (eR - pr)), ppy = ey + (int)(h8je_py[b] * (eR - pr));
    v3_disc(ppx, ppy, pr, 0x00);
    if (pr > 2) v3_put(ppx - pr / 2, ppy - pr / 2, white);
  }
  // mouth: a smile, or an "O" when flying fast (or just booped)
  int mx = (int)cx, my = (int)(cy + r * sy * (n > 5 ? 0.42f : 0.3f));
  float sp = sqrtf(h8je_vx[b] * h8je_vx[b] + h8je_vy[b] * h8je_vy[b]);
  int mw = (int)(r * sx * 0.28f) + 1;
  if (sp > 260 || h8je_mood[b] > 0) {
    int ry = mw * 2 / 3 + 1;
    v8_ellipse(mx, my + ry / 2, mw * 2 / 3 + 1, ry, dark);
    if (mw > 4) v8_ellipse(mx, my + ry / 2 + ry / 3, mw / 3 + 1, ry / 3 + 1, (uint8_t)(0x60 | ((h8je_hue[b] + 8) & 15)));
  } else {
    int th = r > 30 ? 2 : 1;
    for (int k = -6; k <= 6; k++) { float u = k / 6.0f; v3_disc(mx + (int)(u * mw), my + (int)((1 - u * u) * mw * 0.45f), th - 1, dark); }
    if (r > 24) {                                                         // cheeks
      uint8_t blush = (uint8_t)(0x90 | ((h8je_hue[b] + 3) & 15));
      v3_disc(mx - mw - 3, my - 2, (int)(r * 0.08f) + 1, blush); v3_disc(mx + mw + 3, my - 2, (int)(r * 0.08f) + 1, blush);
    }
  }
}

void prog_ghjelly_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  const H8JeLook& L = h8je_looks[look];
  float rate = v8_rate();
  int nT = v3_centre(5) ? L.n : 1 + (int)(v3_u(5) * 13.0f + 0.5f);
  v6_ease(&h8je_wob, v8_kn(8, L.wob / 100.0f), rate);
  v6_ease(&h8je_grav, v8_kn(7, L.grav / 100.0f), rate);
  v6_ease(&h8je_hops, v8_kn(9, L.hops / 100.0f), rate);
  v6_ease(&h8je_eyeK, v8_kn(10, 0.5f), rate);
  v6_ease(&h8je_zoom, v3_clampf(v8_zoom(1.0f), 0.5f, 1.8f), rate);
  int world = v3_pick(6, L.world, H8JE_NWORLD);
  v8_colours(L.pal, 0.03f, 0.9f, 255, 0.0f);
  float dt = v3_clampf(v6_f.sdt / V8_CHILL, 0.0f, 0.05f);                 // physics at real pace (Speed still scales it)
  float rdt = v6_f.dt;
  h8je_t += rdt;

  // ── who's here ──
  if (v6_f.fresh || v6_first) h8je_n = 0;
  nT = v3_clamp(nT, 1, H8JE_MAXB);
  while (h8je_n < nT) { h8je_spawn(h8je_n, look); h8je_n++; }
  if (h8je_n > nT) h8je_n = nT;

  // ── gravity ──
  float G = (h8je_grav - 0.5f) * 2.0f * 1300.0f;                          // px/s², negative = falls up
  if (L.mode == H8JE_TUMBLE) h8je_gAng += rdt * 0.35f;
  else if (L.mode == H8JE_FLIP) { h8je_flipT += rdt; float tgt = ((int)(h8je_flipT / 5.0f) & 1) ? 4.7124f : 1.5708f; h8je_gAng += (tgt - h8je_gAng) * v3_clampf(rdt * 3.0f, 0, 1); }
  else h8je_gAng += (1.5708f - h8je_gAng) * v3_clampf(rdt * 2.0f, 0, 1);
  if (h8je_gAng > 7.854f && L.mode == H8JE_TUMBLE) h8je_gAng -= 6.2832f;
  float gdx = cosf(h8je_gAng), gdy = sinf(h8je_gAng), gx = gdx * G, gy = gdy * G;
  float restit = L.bounce / 100.0f;

  // ── hops, BOING!, moods ──
  bool boing = v6_k12hit;
  if (boing) { h8je_capT = 1.4f; h8je_capI = v3_rnd(6); }
  float hs = dt / 2.0f;
  for (int b = 0; b < h8je_n; b++) {
    h8je_hopT[b] -= dt;
    float up = G >= 0 ? -1.0f : 1.0f;
    if (boing || (h8je_hops > 0.03f && h8je_hopT[b] <= 0)) {
      float js = boing ? 700.0f : 180.0f + h8je_hops * 520.0f;
      float jx, jy;
      if (fabsf(G) < 60) { float a = v3_rndf() * 6.2832f; jx = cosf(a) * js * 0.6f; jy = sinf(a) * js * 0.6f; }
      else { float side = (v3_rndf() - 0.5f) * js * 0.8f; jx = up * gdx * js - gdy * side; jy = up * gdy * js + gdx * side; }
      h8je_kick(b, jx, jy, hs);
      h8je_hopT[b] = (1.0f + v3_rndf() * 4.0f) * (1.4f - h8je_hops);
      if (boing) h8je_mood[b] = 1.2f;
    }
    if (h8je_mood[b] > 0) h8je_mood[b] -= dt;
    h8je_blink[b] -= rdt; if (h8je_blink[b] < 0) h8je_blink[b] = 2.5f + v3_rndf() * 5;
    if (L.mode == H8JE_LAVA) {                                             // warm at the bottom → rise; cool at the top → sink
      float pos = h8je_cy[b] / H;
      if (pos > 0.72f) h8je_heat[b] += dt * 0.35f; else if (pos < 0.3f) h8je_heat[b] -= dt * 0.35f;
      h8je_heat[b] = v3_clampf(h8je_heat[b], 0, 1);
    }
  }
  if (h8je_n > 0 && fabsf(G) < 60 && v3_rnd(60) == 0) { int b = v3_rnd(h8je_n); h8je_kick(b, (v3_rndf() - 0.5f) * 200, (v3_rndf() - 0.5f) * 200, hs); }

  // ── physics: 2 substeps ──
  float pcx[H8JE_MAXB], pcy[H8JE_MAXB];
  for (int b = 0; b < h8je_n; b++) { pcx[b] = h8je_cx[b]; pcy[b] = h8je_cy[b]; }
#ifndef H8JE_NOPHYS
  if (dt > 0) for (int s = 0; s < 2; s++) h8je_step(hs, gx, gy, restit, L.mode);
#endif
  // googly pupils: they lag behind the body's moves and fall toward gravity
  for (int b = 0; b < h8je_n; b++) {
    float vx = dt > 0 ? (h8je_cx[b] - pcx[b]) / dt : 0, vy = dt > 0 ? (h8je_cy[b] - pcy[b]) / dt : 0;
    float ax = dt > 0 ? (vx - h8je_vx[b]) / dt : 0, ay = dt > 0 ? (vy - h8je_vy[b]) / dt : 0;
    ax = v3_clampf(ax, -20000, 20000); ay = v3_clampf(ay, -20000, 20000);
    h8je_vx[b] = vx; h8je_vy[b] = vy;
    float gpx = fabsf(G) > 60 ? gdx * (G > 0 ? 1 : -1) * 6.0f : 0, gpy = fabsf(G) > 60 ? gdy * (G > 0 ? 1 : -1) * 6.0f : 0;
    if (fabsf(G) <= 60) { gpx = 4.0f * sinf(h8je_t * 0.7f + b); gpy = 4.0f * cosf(h8je_t * 0.53f + b * 2); }
    h8je_pvx[b] += (-ax * 0.004f + gpx - h8je_px[b] * 8.0f) * rdt;
    h8je_pvy[b] += (-ay * 0.004f + gpy - h8je_py[b] * 8.0f) * rdt;
    float dmp = 1.0f - v3_clampf(rdt * 2.5f, 0, 1); h8je_pvx[b] *= dmp; h8je_pvy[b] *= dmp;
    h8je_px[b] += h8je_pvx[b] * rdt; h8je_py[b] += h8je_pvy[b] * rdt;
    float m = h8je_px[b] * h8je_px[b] + h8je_py[b] * h8je_py[b];
    if (m > 1.0f) { m = 1.0f / sqrtf(m); h8je_px[b] *= m; h8je_py[b] *= m; h8je_pvx[b] *= -0.5f; h8je_pvy[b] *= -0.5f; }
  }

  // ── the funhouse backdrop (soft field) ──
  int t1 = (int)(h8je_t * 40), t2 = (int)(h8je_t * 31), t3 = (int)(h8je_t * 55);
  uint8_t* LEV = V8_LEV; uint8_t* HUE = V8_HUE;
  for (int j = 0; j < V8_GH; j++) {
    int wj = v3_sin(j * 14 + t1);
    for (int i = 0; i < V8_GW; i++) {
      int u = i * 8 + (wj >> 10), v = j * 8 + (v3_sin(i * 11 - t2) >> 10);
      int lev, hue;
      if (world == H8JE_DOTS) {
        int fu = (u & 63) - 32, fv = (v & 63) - 32, d2 = fu * fu + fv * fv;
        int rr = 15 + (v3_sin(t3 + (u >> 6) * 90 + (v >> 6) * 150) >> 11);   // the dots breathe
        lev = d2 < rr * rr ? 95 : (d2 < (rr + 6) * (rr + 6) ? 95 - (int)((v3_isqrt(d2) - rr) * 11) : 26);
        hue = (((u >> 6) * 3 + (v >> 6) * 5) * 16) + t1 / 2;
      } else if (world == H8JE_CHECK) {
        int c = ((u >> 6) ^ (v >> 6)) & 1;
        lev = c ? 80 : 28;
        hue = (c ? 128 : 0) + ((u + v) >> 3) + t1 / 3;
      } else if (world == H8JE_VOID) {
        lev = 6 + ((v3_sin(u * 3 + t2) + v3_sin(v * 4 - t1)) >> 12);
        hue = 140 + (u >> 2);
      } else {
        int32_t s = v3_sin((u + v) * 5 + t3);
        lev = 50 + (s >> 9);
        hue = ((u - v) >> 1) + t1;
      }
      int c = j * V8_GW + i; LEV[c] = (uint8_t)v3_clamp(lev, 0, 226); HUE[c] = (uint8_t)(hue & 255);
    }
  }
  v8_upscale(3);
  if (world == H8JE_VOID) v8_stars(70, 10, 1234);

  // ── the jellies ──
  int hueShift = (int)(h8je_t * 20.0f);
  for (int b = 0; b < h8je_n; b++) h8je_fill(b, hueShift);
  for (int b = 0; b < h8je_n; b++) h8je_face(b, h8je_eyeK);

  if (h8je_capT > 0) {
    h8je_capT -= rdt;
    int sc = h8je_capT > 1.25f ? 2 + (int)((1.4f - h8je_capT) * 20) : (h8je_capT > 0.3f ? 4 : 3);   // pops in, then shrinks away
    if (h8je_capT < 0.15f) sc = 2;
    v8_textC(160, 14, h8je_caps[h8je_capI], sc, (uint8_t)(0xF0 | ((int)(h8je_t * 12) & 15)));
  }
  v8_end();
}
