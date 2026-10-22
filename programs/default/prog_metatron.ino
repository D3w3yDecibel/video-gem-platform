// =====================================================================
// PROGRAM: METATRON — sacred solids in light (by Dewey) — Set 6, slot 2
//
// The forms of sacred geometry as glowing wireframes turning in space:
// Metatron's Cube rising off the page into 3-D, the Merkaba's two
// tetrahedra spinning against each other, all five Platonic solids nested
// one inside another, Buckminster Fuller's "jitterbug" folding a
// cuboctahedron through an icosahedron to an octahedron, a tesseract
// turning through the fourth dimension, the Sri Yantra … Lines that cross
// add their light, and the trails smear each turn into a ghostly bloom.
//
// Presets: k0 Metatron's Cube · k1 Merkaba · k2 Platonic Nest · k3 Icosahedron ·
//          k4 Dodecahedron · k5 Jitterbug · k6 Tesseract · k7 Geodesic ·
//          k8 Vector Matrix · k9 Sri Yantra · k10 Torus · k11 Tour
//   k12 — EXPLODE: the shape bursts apart and draws itself back together
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Spin (◄ one way | the other ►) · p6 Tumble · p7 Glow · p8 Trails
//   p9 Perspective · p10 Nodes (glowing orbs at the corners) · p11 Morph
//   p12-p15 live FX
// =====================================================================
#include "vg6.h"

struct MtLook { uint8_t pal, size, spin, tumble, glow, trail, persp, nodes, morph, flow, spread; };
static const MtLook mt_looks[11] = {
  // pal size spin tum glow trail persp nodes morph flow spread
  {  0, 52, 30, 20, 1, 60, 40, 60, 40, 5, 70 },   // Metatron's Cube
  {  9, 58, 50, 15, 2, 70, 50, 40, 50, 6, 50 },   // Merkaba
  { 11, 80, 30, 25, 1, 55, 45, 20, 30, 8,100 },  // Platonic Nest
  {  4, 70, 35, 30, 2, 65, 50, 50, 30, 5, 60 },   // Icosahedron
  {  5, 72, 30, 30, 2, 65, 50, 40, 30, 6, 60 },  // Dodecahedron
  {  2, 75, 25, 20, 2, 60, 45, 50, 50, 5, 70 },   // Jitterbug
  { 10, 110, 30, 15, 2, 70, 55, 40, 50, 7, 80 },   // Tesseract
  {  6, 80, 25, 25, 1, 55, 45, 30, 30, 5, 70 },  // Geodesic
  {  8, 85, 20, 25, 1, 60, 50, 30, 40, 6, 90 },   // Vector Matrix
  {  1, 75, 15, 10, 2, 50, 30, 20, 30, 4, 60 },  // Sri Yantra
  {  3, 80, 30, 30, 0, 60, 50, 0,  40, 6, 80 },  // Torus
};
static const char* const mt_names[12] = { "Metatron's Cube", "Merkaba", "Platonic Nest", "Icosahedron", "Dodecahedron", "Jitterbug", "Tesseract", "Geodesic", "Vector Matrix", "Sri Yantra", "Torus", "Tour" };
static const char* const mt_lbl[7] = { "Spin", "Tumble", "Glow", "Trails", "Perspective", "Nodes", "Morph" };
const char* prog_metatron_name() { return "METATRON"; }
const char* prog_metatron_character() { return "Sacred solids in light: Metatron's Cube, Merkaba, nested Platonic solids, tesseract, turning in space"; }
const char* prog_metatron_presetName(int p) { return (p >= 0 && p < 12) ? mt_names[p] : NULL; }
const char* prog_metatron_potLabel(int p, int pot) { (void)p; return v6_label(pot, mt_lbl); }
uint8_t prog_metatron_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

// ─── The shape: vertices and edges ────────────────────────────────────
#define MT_MV 200
#define MT_ME 440
static float* mt_vx; static float* mt_vy; static float* mt_vz;   // vertices (3-D)
static uint8_t* mt_vh;                                           // a hue per vertex
static uint8_t* mt_e;                                            // edge pairs
static uint8_t* mt_eh;                                           // a hue per edge
static int16_t* mt_sx; static int16_t* mt_sy; static int16_t* mt_sz;   // projected (1/8 px), depth
static uint8_t* mt_ring;                                         // vertices drawn as circles (Metatron's 13), radius in figure units × 100
static int mt_nv = 0, mt_ne = 0;
static void mt_mem() {
  uint8_t* p = v6_mem;
  mt_vx = (float*)p; mt_vy = mt_vx + MT_MV; mt_vz = mt_vy + MT_MV; p += MT_MV * 12;
  mt_sx = (int16_t*)p; mt_sy = mt_sx + MT_MV; mt_sz = mt_sy + MT_MV; p += MT_MV * 6;
  mt_e = p; p += MT_ME * 2;
  mt_eh = p; p += MT_ME;
  mt_vh = p; p += MT_MV;
  mt_ring = p; p += MT_MV;                                       // ≈ 5.3 KB
}
static int mt_v(float x, float y, float z, int h) {
  if (mt_nv >= MT_MV) return MT_MV - 1;
  mt_vx[mt_nv] = x; mt_vy[mt_nv] = y; mt_vz[mt_nv] = z; mt_vh[mt_nv] = (uint8_t)h; mt_ring[mt_nv] = 0;
  return mt_nv++;
}
static void mt_edge(int a, int b, int h) {
  if (mt_ne >= MT_ME || a == b) return;
  mt_e[mt_ne * 2] = (uint8_t)a; mt_e[mt_ne * 2 + 1] = (uint8_t)b; mt_eh[mt_ne] = (uint8_t)h; mt_ne++;
}
// connect every pair of vertices from v0 on whose distance is the shortest found (± 2 %)
static void mt_nearestT(int v0, int h, float tol) {
  float best = 1e9f;
  for (int i = v0; i < mt_nv; i++) for (int j = i + 1; j < mt_nv; j++) {
    float dx = mt_vx[i] - mt_vx[j], dy = mt_vy[i] - mt_vy[j], dz = mt_vz[i] - mt_vz[j], d = dx * dx + dy * dy + dz * dz;
    if (d > 1e-4f && d < best) best = d;
  }
  for (int i = v0; i < mt_nv; i++) for (int j = i + 1; j < mt_nv; j++) {
    float dx = mt_vx[i] - mt_vx[j], dy = mt_vy[i] - mt_vy[j], dz = mt_vz[i] - mt_vz[j], d = dx * dx + dy * dy + dz * dz;
    if (d < best * tol && d > 1e-4f) mt_edge(i, j, h);
  }
}
static void mt_nearest(int v0, int h) { mt_nearestT(v0, h, 1.04f); }
static const float MT_PHI = 1.618034f;
static void mt_icosa(float s, int h) {
  int v0 = mt_nv;
  for (int k = 0; k < 3; k++) for (int a = -1; a <= 1; a += 2) for (int b = -1; b <= 1; b += 2) {
    float p[3] = { 0, a * s, b * s * MT_PHI };
    mt_v(p[k % 3], p[(k + 1) % 3], p[(k + 2) % 3], h);
  }
  mt_nearest(v0, h);
}
static void mt_dodeca(float s, int h) {
  int v0 = mt_nv;
  for (int a = -1; a <= 1; a += 2) for (int b = -1; b <= 1; b += 2) for (int c = -1; c <= 1; c += 2) mt_v(a * s, b * s, c * s, h);
  for (int k = 0; k < 3; k++) for (int a = -1; a <= 1; a += 2) for (int b = -1; b <= 1; b += 2) {
    float p[3] = { 0, a * s / MT_PHI, b * s * MT_PHI };
    mt_v(p[k % 3], p[(k + 1) % 3], p[(k + 2) % 3], h);
  }
  mt_nearest(v0, h);
}
static void mt_tetra(float s, int sign, int h) {
  int v0 = mt_nv;
  mt_v(s * sign, s * sign, s * sign, h); mt_v(s * sign, -s * sign, -s * sign, h); mt_v(-s * sign, s * sign, -s * sign, h); mt_v(-s * sign, -s * sign, s * sign, h);
  for (int i = 0; i < 4; i++) for (int j = i + 1; j < 4; j++) mt_edge(v0 + i, v0 + j, h);
}
static void mt_cube(float s, int h) {
  int v0 = mt_nv;
  for (int a = -1; a <= 1; a += 2) for (int b = -1; b <= 1; b += 2) for (int c = -1; c <= 1; c += 2) mt_v(a * s, b * s, c * s, h);
  mt_nearest(v0, h);
}
static void mt_octa(float s, int h) {
  int v0 = mt_nv;
  for (int k = 0; k < 3; k++) for (int a = -1; a <= 1; a += 2) { float p[3] = { 0, 0, 0 }; p[k] = a * s; mt_v(p[0], p[1], p[2], h); }
  mt_nearest(v0, h);
}

static float mt_t = 0, mt_morphT = 0;
// build the shape for look lk (animated shapes are rebuilt every frame)
static void mt_build(int lk, float m) {
  mt_nv = 0; mt_ne = 0;
  switch (lk) {
    case 0: {                                                   // Metatron's Cube: the Fruit of Life's 13 centres, all joined
      float lift = 0.5f - 0.5f * cosf(m * 0.6f);                // rises from flat into a cube-in-a-cube
      mt_v(0, 0, 0, 0); mt_ring[0] = 50;
      for (int k = 1; k <= 2; k++) for (int i = 0; i < 6; i++) {
        float a = i * 1.0471976f, d = k;
        float z = (i & 1 ? 1 : -1) * lift * 0.82f * k;
        int v = mt_v(cosf(a) * d * (1.0f - lift * 0.18f), sinf(a) * d * (1.0f - lift * 0.18f), z, k * 4 + i);
        mt_ring[v] = 50;
      }
      for (int i = 0; i < 13; i++) for (int j = i + 1; j < 13; j++) mt_edge(i, j, (i + j) & 15);
      break;
    }
    case 1: {                                                   // Merkaba: two tetrahedra turning against each other
      float a = m * 0.8f, ca = cosf(a), sa = sinf(a);
      mt_tetra(1.15f, 1, 2); mt_tetra(1.15f, -1, 10);
      for (int i = 0; i < 8; i++) {                             // spin them about the vertical axis (1,1,1) ≈ turn x/z
        float x = mt_vx[i], z = mt_vz[i], s = i < 4 ? 1.0f : -1.0f;
        mt_vx[i] = x * ca - z * sa * s; mt_vz[i] = x * sa * s + z * ca;
      }
      int c = mt_nv;                                            // a ring round its middle
      for (int i = 0; i < 24; i++) { float b = i * 0.2617994f; mt_v(cosf(b) * 1.9f, 0, sinf(b) * 1.9f, 6); }
      for (int i = 0; i < 24; i++) mt_edge(c + i, c + (i + 1) % 24, 6);
      break;
    }
    case 2: {                                                   // Platonic Nest: Kepler's five, one inside the next
      mt_octa(0.7f, 1); mt_cube(0.75f, 4); mt_tetra(0.75f, 1, 7); mt_icosa(0.62f, 10); mt_dodeca(0.85f, 13);
      break;
    }
    case 3: mt_icosa(0.85f, 5); mt_dodeca(0.32f, 12); break;                             // Icosahedron (with its dual inside)
    case 4: mt_dodeca(0.85f, 3); mt_icosa(0.45f, 11); break;                             // Dodecahedron (with its dual inside)
    case 5: {                                                   // Jitterbug: cuboctahedron ↔ icosahedron ↔ octahedron
      float t = 0.5f + 0.5f * sinf(m * 0.7f);                   // 1 cuboctahedron … 0.618 icosahedron … 0 octahedron
      for (int pass = 0; pass < 2; pass++) {
        // pass 0 lays the points out as an icosahedron to find its 30 edges; pass 1 moves them to t,
        // keeping the same pairs all the way through the fold — that is the jitterbug's twist
        float tt = pass ? t : 0.618034f;
        mt_nv = 0;
        for (int k = 0; k < 3; k++) for (int a = -1; a <= 1; a += 2) for (int b = -1; b <= 1; b += 2) {
          float p[3] = { 0, a * tt * 1.3f, b * 1.3f };
          mt_v(p[k % 3], p[(k + 1) % 3], p[(k + 2) % 3], k * 5);
        }
        if (!pass) mt_nearest(0, 0);
      }
      for (int e = 0; e < mt_ne; e++) mt_eh[e] = (uint8_t)((e * 7) & 15);
      break;
    }
    case 6: {                                                   // Tesseract: a 4-D cube, turning through the 4th dimension
      float a = m * 0.5f, b = m * 0.31f, ca = cosf(a), sa = sinf(a), cb = cosf(b), sb = sinf(b);
      for (int i = 0; i < 16; i++) {
        float x = i & 1 ? 1 : -1, y = i & 2 ? 1 : -1, z = i & 4 ? 1 : -1, w = i & 8 ? 1 : -1;
        float x2 = x * ca - w * sa, w2 = x * sa + w * ca;       // turn in the x-w plane …
        float y2 = y * cb - w2 * sb, w3 = y * sb + w2 * cb;     // … and the y-w plane
        float p = 1.6f / (2.6f - w3 * 0.6f);                    // 4-D → 3-D perspective
        mt_v(x2 * p * 0.75f, y2 * p * 0.75f, z * p * 0.75f, i & 8 ? 9 : 2);
      }
      for (int i = 0; i < 16; i++) for (int k = 0; k < 4; k++) { int j = i ^ (1 << k); if (j > i) mt_edge(i, j, k == 3 ? 6 : (i & 8 ? 9 : 2)); }
      break;
    }
    case 7: {                                                   // Geodesic: an icosahedron split once, pushed out to a sphere
      mt_icosa(1.0f, 0);
      int n0 = mt_nv, e0 = mt_ne;
      for (int e = 0; e < e0; e++) {                            // a new vertex in the middle of every edge
        int a = mt_e[e * 2], b = mt_e[e * 2 + 1];
        mt_v((mt_vx[a] + mt_vx[b]) * 0.5f, (mt_vy[a] + mt_vy[b]) * 0.5f, (mt_vz[a] + mt_vz[b]) * 0.5f, 8);
      }
      for (int i = 0; i < mt_nv; i++) {
        float d = sqrtf(mt_vx[i] * mt_vx[i] + mt_vy[i] * mt_vy[i] + mt_vz[i] * mt_vz[i]);
        float s = 1.3f / d * (i >= n0 ? 1.0f + 0.12f * sinf(m * 1.2f + i) : 1.0f);   // the new points breathe
        mt_vx[i] *= s; mt_vy[i] *= s; mt_vz[i] *= s;
      }
      mt_ne = 0; mt_nearestT(0, 4, 1.35f);
      for (int e = 0; e < mt_ne; e++) mt_eh[e] = (uint8_t)((mt_e[e * 2] < n0 || mt_e[e * 2 + 1] < n0) ? 4 : 12);
      break;
    }
    case 8: {                                                   // Vector Matrix: Fuller's lattice of tetrahedra and octahedra
      for (int x = -2; x <= 2; x++) for (int y = -2; y <= 2; y++) for (int z = -2; z <= 2; z++) {
        if ((x + y + z) & 1) continue;
        if (x * x + y * y + z * z > 5) continue;
        mt_v(x * 0.62f, y * 0.62f, z * 0.62f, (x * x + y * y + z * z) * 3);
      }
      mt_nearest(0, 0);
      for (int e = 0; e < mt_ne; e++) mt_eh[e] = (uint8_t)((mt_vh[mt_e[e * 2]] + mt_vh[mt_e[e * 2 + 1]]) / 2);
      break;
    }
    case 9: {                                                   // Sri Yantra: four triangles up, five down, round a point
      // each triangle's base and apex height (×100, y up), roughly as in a classic drawing
      static const int8_t ty[9][2] = { {-73, 96}, {-52, 64}, {-30, 44}, {-12, 22},              // pointing up
                                       {73, -96}, {60, -66}, {42, -48}, {26, -30}, {10, -16} };  // pointing down
      for (int k = 0; k < 9; k++) {
        float by = ty[k][0] / 100.0f, ay = ty[k][1] / 100.0f;
        float bx = sqrtf(v3_clampf(1.0f - by * by, 0.05f, 1.0f)) * (k == 0 || k == 4 ? 1.0f : 0.92f);
        float lift = 0.15f * sinf(m * 0.8f + k * 0.7f);         // each triangle floats a little off the page
        int v0 = mt_v(-bx, -by, lift, k), v1 = mt_v(bx, -by, lift, k), v2 = mt_v(0, -ay, lift, k);
        mt_edge(v0, v1, k + 2); mt_edge(v1, v2, k + 2); mt_edge(v2, v0, k + 2);
      }
      for (int r = 0; r < 2; r++) {                             // two circles for the lotus petals
        int b0 = mt_nv;
        for (int i = 0; i < 32; i++) { float a = i * 0.19635f; mt_v(cosf(a) * (1.08f + r * 0.18f), sinf(a) * (1.08f + r * 0.18f), 0, 12 + r); }
        for (int i = 0; i < 32; i++) mt_edge(b0 + i, b0 + (i + 1) % 32, 12 + r);
      }
      int b0 = mt_nv;                                           // the square of gates
      const float q = 1.45f;
      mt_v(-q, -q, 0, 14); mt_v(q, -q, 0, 14); mt_v(q, q, 0, 14); mt_v(-q, q, 0, 14);
      for (int i = 0; i < 4; i++) mt_edge(b0 + i, b0 + (i + 1) % 4, 14);
      int bindu = mt_v(0, -0.02f, 0, 0); mt_ring[bindu] = 4;   // the bindu, the point at the centre
      break;
    }
    default: {                                                  // Torus: a ring of rings
      const int NU = 20, NV = 10;
      float tw = m * 0.4f;
      for (int u = 0; u < NU; u++) for (int v = 0; v < NV; v++) {
        float a = u * 6.2831853f / NU, b = v * 6.2831853f / NV + tw + u * 0.15f;
        float rr = 1.0f + 0.45f * cosf(b);
        mt_v(rr * cosf(a), 0.45f * sinf(b), rr * sinf(a), (v * 16) / NV);
      }
      for (int u = 0; u < NU; u++) for (int v = 0; v < NV; v++) {
        int i = u * NV + v;
        mt_edge(i, ((u + 1) % NU) * NV + v, (v * 16) / NV);
        mt_edge(i, u * NV + (v + 1) % NV, (u * 16) / NU);
      }
      break;
    }
  }
}

static float mt_ax = 0, mt_ay = 0, mt_az = 0, mt_boom = 0;
static float mt_size, mt_spin, mt_tumble, mt_glow, mt_trail, mt_persp, mt_nodes, mt_morph;

void prog_metatron_init() {
  mt_mem(); v6_init();
  mt_ax = 0.3f; mt_ay = 0; mt_az = 0; mt_morphT = 0; mt_boom = 0;
  if (display.getBuffer()) memset(display.getBuffer(), 0, W * H);
}

void prog_metatron_draw(int preset) {
  int look = v6_begin(preset, 11);
  if (!v3_buf) return;
  mt_mem();
  const MtLook& L = mt_looks[look];
  float dt = v6_f.dt, sdt = v6_f.sdt;

  float tSpin = v3_centre(5) ? L.spin / 100.0f : v3_bi(5) * 1.5f;
  float tTum = v3_centre(6) ? L.tumble / 100.0f : v3_u(6) * 1.2f;
  float tGlow = v3_centre(7) ? L.glow : v3_u(7) * 3.0f;
  float tTrail = v3_centre(8) ? L.trail / 100.0f : v3_u(8) * 0.95f;
  float tPersp = v3_centre(9) ? L.persp / 100.0f : v3_u(9);
  float tNodes = v3_centre(10) ? L.nodes / 100.0f : v3_u(10);
  float tMorph = v3_centre(11) ? L.morph / 100.0f : v3_u(11) * 2.0f;
  v6_ease(&mt_size, L.size, 2.0f); v6_ease(&mt_spin, tSpin, 2.0f); v6_ease(&mt_tumble, tTum, 2.0f); v6_ease(&mt_glow, tGlow, 3.0f);
  v6_ease(&mt_trail, tTrail, 3.0f); v6_ease(&mt_persp, tPersp, 2.0f); v6_ease(&mt_nodes, tNodes, 3.0f); v6_ease(&mt_morph, tMorph, 2.0f);
  v6_colours(L.pal, L.flow / 100.0f, L.spread / 100.0f, 110);

  if (look == 9) {                                              // the Sri Yantra is flat: it turns on the page and only tilts a little
    mt_az += sdt * mt_spin * 0.5f; v6_ease(&mt_ax, 0.35f * sinf(v6_t * 0.2f) * mt_tumble * 4.0f, 1.0f); v6_ease(&mt_ay, 0.3f * sinf(v6_t * 0.13f) * mt_tumble * 4.0f, 1.0f);
  } else {
    mt_ay += sdt * mt_spin * 0.9f;
    mt_ax += sdt * mt_tumble * 0.37f * sinf(v6_t * 0.13f + 0.5f);
    mt_az += sdt * mt_tumble * 0.23f;
  }
  mt_morphT += sdt * mt_morph * 1.6f;
  if (v6_k12hit) mt_boom = 1.0f;
  mt_boom -= dt * 0.6f; if (mt_boom < 0) mt_boom = 0;

  // fade the last frame (trails); a change of look clears it faster
  int fade = 15 - (int)(mt_trail * 14.0f + 0.5f);
  if (v6_fade < 0.3f) fade = v6_max(fade, 6);
  if (fade >= 15) memset(v3_buf, 0, W * H); else v3_fadeLev(v6_max(fade, 1));

  mt_build(look, mt_morphT);
  // turn and project
  float cx = cosf(mt_ax), sx = sinf(mt_ax), cy = cosf(mt_ay), sy = sinf(mt_ay), cz = cosf(mt_az), sz = sinf(mt_az);
  float scale = mt_size * v6_zoom(1.0f) * 8.0f;                 // 1/8 px per unit
  float pd = 2.0f + (1.0f - mt_persp) * 12.0f;                  // the eye's distance (far = flat)
  float boom = mt_boom * mt_boom * (3.0f - 2.0f * mt_boom);
  for (int i = 0; i < mt_nv; i++) {
    float x = mt_vx[i], y = mt_vy[i], z = mt_vz[i];
    if (boom > 0) {                                             // explode: each vertex flies out along its own direction
      float k = 1.0f + boom * (1.5f + 0.8f * sinf(i * 2.3f));
      x *= k; y *= k; z *= k;
    }
    float x1 = x * cy + z * sy, z1 = -x * sy + z * cy;
    float y1 = y * cx - z1 * sx, z2 = y * sx + z1 * cx;
    float x2 = x1 * cz - y1 * sz, y2 = x1 * sz + y1 * cz;
    float p = pd / (pd + z2);
    mt_sx[i] = (int16_t)v3_clamp((int)(1280 + x2 * p * scale), -16000, 16000);
    mt_sy[i] = (int16_t)v3_clamp((int)(960 + y2 * p * scale), -16000, 16000);
    mt_sz[i] = (int16_t)(z2 * 100);
  }
  // edges: nearer is brighter
  int halo = (int)(mt_glow + 0.5f);
  for (int e = 0; e < mt_ne; e++) {
    int a = mt_e[e * 2], b = mt_e[e * 2 + 1];
    int z = (mt_sz[a] + mt_sz[b]) / 2;
    int lev = v3_clamp(11 - z / 25, 4, 14);
    int hue = (mt_eh[e] + (int)(mt_ay * 2)) & 15;
    int x0 = mt_sx[a], y0 = mt_sy[a], x1 = mt_sx[b], y1 = mt_sy[b];
    if (halo) {                                                 // a soft halo: the line again, a pixel to either side
      int ox = 0, oy = 0;
      if (v3_abs(x1 - x0) > v3_abs(y1 - y0)) oy = 8; else ox = 8;
      for (int h = 1; h <= halo; h++) {
        int hl = lev / (2 + 2 * h);
        v6_lineAdd(x0 + ox * h, y0 + oy * h, x1 + ox * h, y1 + oy * h, hl, hue);
        v6_lineAdd(x0 - ox * h, y0 - oy * h, x1 - ox * h, y1 - oy * h, hl, hue);
      }
    }
    v6_lineAdd(x0, y0, x1, y1, lev, hue);
  }
  // nodes: glowing orbs at the corners; Metatron's centres are circles
  int nr = (int)(mt_nodes * 4.0f + 0.5f);
  for (int i = 0; i < mt_nv; i++) {
    int lev = v3_clamp(13 - mt_sz[i] / 25, 5, 15), hue = (mt_vh[i] + 8) & 15;
    if (mt_ring[i]) {
      float p = pd / (pd + mt_sz[i] / 100.0f);
      v6_ring(mt_sx[i], mt_sy[i], (int)(mt_ring[i] / 100.0f * scale * p), 0, 1024, lev - 3, (mt_vh[i] + 4) & 15, halo ? 1 : 0);
    }
    if (nr > 0) v6_dot(mt_sx[i], mt_sy[i], nr, lev, hue);
  }
  v6_end();
}
