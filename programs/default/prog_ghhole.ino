// =====================================================================
// PROGRAM: BLACK HOLE — gravitational lens & accretion disk (by Dewey) — Set 8 slot 3
//
// Replaces KALEIDO. A black hole you can play: space bends around it, so
// the nebula, stars or spacetime grid behind it smear into arcs and an
// Einstein ring. A tilted disk of glowing gas swirls in (the inner rings
// turn faster than the outer, so the streaks shear into spirals), one side
// brighter where the gas rushes toward you, and the far side of the disk
// is bent up over the top of the hole and under the bottom, the way it looks
// in Interstellar. A thin photon ring hugs the black shadow. Quasar jets,
// a wormhole you can see through, and a binary pair that spirals together
// and merges.
//
// How it works: on the 80 × 60 grid each cell is pushed toward the hole by
// the lens equation (q = p·(1 − E²/r²), one divide), and the background is
// looked up at the bent place. The disk is a flattened ellipse in front;
// the bent far side is a ring of the same disk texture round the shadow.
// Every ring of the disk turns at its own (Kepler) speed from a table.
//
// Presets: k0 Gargantua · k1 Quasar · k2 Wormhole · k3 Spacetime · k4 Binary ·
//          k5 Eclipse · k6 Edge-On · k7 Lens Field · k8 Singularity ·
//          k9 Twin Grid · k10 Deep Field · k11 Tour
//   k12 — FEED: a star falls in, is stretched into a streak (spaghettified)
//         and swallowed in a flare. In the binary looks: MERGE — the pair
//         spiral together faster and faster and merge in a flash.
//         (Both also happen by themselves every 35-60 s.)
//
// Knobs: p0 Palette · p1 Speed · p2 Zoom · p3 Colour Flow · p4 Hue Spread
//   p5 Mass · p6 Space (nebula/stars/grid/wormhole) · p7 Tilt · p8 Disk · p9 Spin · p10 Jets
//   p11 Key: Glow|Black · p12-p15 live FX
// =====================================================================
#include "vg8.h"

enum { H8BH_NEBULA, H8BH_STARS, H8BH_GRID, H8BH_WORM, H8BH_NSPACE };
struct H8BhLook { uint8_t holes, space, mass, tilt, disk, spin, jets, pal, fl, spread; int8_t key; };
static const H8BhLook h8bh_looks[11] = {
  // hl space        mass tilt disk spin jets pal            fl spr key
  { 1, H8BH_NEBULA,   50,  18,  70,  45,   0, V8P_INFRARED,   2,  70,  0 },   // Gargantua
  { 1, H8BH_STARS,    40,  45,  60,  55,  80, V8P_ELECTRIC,   3,  80,  0 },   // Quasar
  { 1, H8BH_WORM,     55,  35,  45,  45,   0, V8P_GALAXY,     3,  90,  0 },   // Wormhole
  { 1, H8BH_GRID,     45,  28,  40,  45,   0, V8P_NEON,       2,  80,  0 },   // Spacetime
  { 2, H8BH_NEBULA,   50,  40,  55,  50,   0, V8P_FIREICE,    3,  80,  0 },   // Binary
  { 1, H8BH_STARS,    60,  88,  55,  40,   0, V8P_SUNSET,     2,  70,  0 },   // Eclipse
  { 1, H8BH_NEBULA,   45,   4,  80,  50,  35, V8P_LAVA,       2,  60,  0 },   // Edge-On
  { 1, H8BH_STARS,    60,  25,  15,  40,   0, V8P_RETROPOP,   3,  90,  0 },   // Lens Field
  { 1, H8BH_WORM,     80,  40,  60,  60,  45, V8P_ACID,       3,  90,  0 },   // Singularity
  { 2, H8BH_GRID,     50,  30,  45,  50,   0, V8P_TOXIC,      2,  80,  0 },   // Twin Grid
  { 1, H8BH_STARS,    45,  55,  65,  35,  20, V8P_EMBER,      2,  50,  0 },   // Deep Field
};
static const char* const h8bh_names[12] = { "Gargantua", "Quasar", "Wormhole", "Spacetime", "Binary", "Eclipse", "Edge-On", "Lens Field", "Singularity", "Twin Grid", "Deep Field", "Tour" };
static const char* const h8bh_lbl[6] = { "Mass", "Space", "Tilt", "Disk", "Spin", "Jets" };
const char* prog_ghhole_name() { return "BLACK HOLE"; }
const char* prog_ghhole_character() { return "Black hole: space bends into an Einstein ring round a swirling, tilted disk of light"; }
const char* prog_ghhole_presetName(int p) { return (p >= 0 && p < 12) ? h8bh_names[p] : NULL; }
const char* prog_ghhole_potLabel(int p, int pot) { (void)p; return v8_label(pot, h8bh_lbl); }
uint8_t prog_ghhole_renderHint(int p) { (void)p; return RENDER_PERPIXEL; }

#define H8BH_NB 128                                        // disk rings ("bands"), each 1/16 of the inner radius wide
static float h8bh_om[H8BH_NB];                             // Kepler speed of each band (inner = 1)
static float h8bh_ph[H8BH_NB];                             // each band's turn, 0 … 1024
static int16_t h8bh_phI[H8BH_NB];
static uint8_t h8bh_prof[H8BH_NB], h8bh_bhue[H8BH_NB];
static float h8bh_mass = 0.5f, h8bh_tilt = 0.3f, h8bh_disk = 0.6f, h8bh_spin = 0.5f, h8bh_jets = 0, h8bh_holesF = 1;
static float h8bh_t = 0, h8bh_flare = 0, h8bh_auto = 30;
static float h8bh_feedR = -1, h8bh_feedPhi = 0;          // falling star: radius in bands (−1 = none), angle
static float h8bh_sep = 1, h8bh_orb = 0; static int h8bh_merging = 0;   // binary: separation 0 … 1, orbit angle, 1 = spiralling in, 2 = re-forming
static int h8bh_bMax = 64, h8bh_feedB = -1, h8bh_feedArc = 0, h8bh_feedA = 0, h8bh_tw = 0, h8bh_dop = 120;

void prog_ghhole_init() {
  v8_init();
  for (int b = 0; b < H8BH_NB; b++) { h8bh_om[b] = powf(1.0f + b / 16.0f, -1.5f); h8bh_ph[b] = (float)v3_rnd(1024); }
  h8bh_feedR = -1; h8bh_flare = 0; h8bh_sep = 1; h8bh_merging = 0; h8bh_auto = 30 + v3_rndf() * 20;
}

// mix a new light's hue into the cell by how much of the light it brings (no hard colour edges)
static inline int h8bh_mix(int hue, int lev, int nh, int add) { if (add <= 0) return hue; return hue + v8_hwrap((nh - hue) & 255) * add * 3 / (add * 3 + lev + 1); }
// the disk's light at angle phi (0 … 1023) in band b; hue out
static inline int h8bh_tex(int phi, int b, int* hue) {
  int pr = phi - h8bh_phI[b];                                                  // turned with its band
  int32_t s = v3_sin(pr * 3 + b * 37) + (v3_sin(pr * 8 - b * 23 + h8bh_tw) >> 1) + (v3_sin(b * 71 + (pr >> 1)) >> 1);   // ±32768
  int lev = (h8bh_prof[b] * (170 + (s >> 8))) >> 8;
  *hue = h8bh_bhue[b] + (s >> 10);
  if (h8bh_feedB >= 0) {                                                       // the falling star and its streak
    int db = b - h8bh_feedB; if (db < 0) db = -db;
    if (db < 4) { int da = (h8bh_feedA - phi) & 1023; if (da < h8bh_feedArc) { lev += (4 - db) * (h8bh_feedArc - da) * 90 / h8bh_feedArc; *hue = 136 + (da >> 3); } }
  }
  return lev;
}
// the far side of the disk bent round the shadow, and the photon ring, for one hole: light to add
static int h8bh_ringW = 7;                                   // the bent disk ring's width, tenths of the shadow
static inline int h8bh_ring(int dx, int dy, int r, int rs, int diskOn, int* hue) {
  int add = 0, a = v3_atan(dy, dx);
  int c = v3_cos(a);
  int dop = 256 + ((c * h8bh_dop) >> 14);                                   // the side coming toward you is brighter
  if (diskOn && r < rs * (10 + h8bh_ringW) / 10) {
    int b = (r - rs) * h8bh_bMax * 10 / (rs * h8bh_ringW);                             // squeeze the whole disk into the ring
    if (b >= 0 && b < h8bh_bMax) {
      int hh, l = h8bh_tex(a, b, &hh);
      int w = 90 + ((v3_abs(v3_sin(a)) * 166) >> 14);                        // strongest over the top and under the bottom
      add += (((l * w) >> 8) * dop) >> 8;
      *hue = hh - (c >> 11);
    }
  }
  int rr = rs + 1 + rs / 30, wr = 3 + rs / 22, d = v3_abs(r - rr);
  if (d < wr) { add += (wr - d) * (130 + (int)(h8bh_flare * 120)) / wr * dop >> 8; if (d < wr / 2 + 1) *hue = 20; }
  return add;
}

void prog_ghhole_draw(int preset) {
  int look = v8_begin(preset, 11);
  if (!v3_buf) return;
  const H8BhLook& L = h8bh_looks[look];
  float rate = v8_rate();
  v6_ease(&h8bh_mass, v8_kn(5, L.mass / 100.0f), rate);
  v6_ease(&h8bh_tilt, v8_kn(7, L.tilt / 100.0f), rate);
  v6_ease(&h8bh_disk, v8_kn(8, L.disk / 100.0f), rate);
  v6_ease(&h8bh_spin, v8_kn(9, L.spin / 100.0f), rate);
  v6_ease(&h8bh_jets, v8_kn(10, L.jets / 100.0f), rate);
  v6_ease(&h8bh_holesF, (float)L.holes, rate);
  int space = v3_pick(6, L.space, H8BH_NSPACE);
  bool binary = h8bh_holesF > 1.5f;
  v8_colours(L.pal, L.fl / 100.0f, L.spread / 100.0f, 110, L.key / 100.0f);
  float sdt = v6_f.sdt, dt = v6_f.dt;
  h8bh_t += sdt; if (h8bh_t > 20000.0f) h8bh_t -= 20000.0f;
  float t = h8bh_t;

  // ── size and shape ──
  float zm = v8_zoom(1.0f);
  int rs = (int)((12 + h8bh_mass * 58) * zm); rs = v3_clamp(rs, 6, 170);       // the black shadow, px
  int rsE2 = (rs * 8 / 5) * (rs * 8 / 5);                                      // Einstein radius²
  int rin = rs * 5 / 4;                                                        // disk inner edge
  float tl = h8bh_tilt * 85.0f + 3.0f + 3.0f * sinf(t * 0.07f);                // degrees above edge-on, sways a little
  int ci = v3_clamp((int)(256.0f * sinf(tl * 0.01745f)), 14, 256);
  float dsz = 2.2f + h8bh_disk * 3.0f;                                         // outer edge = inner × this
  h8bh_bMax = v3_clamp((int)((dsz - 1.0f) * 16.0f), 8, H8BH_NB);
  h8bh_dop = 40 + (256 - ci) * 150 / 256;
  int diskOn = (h8bh_disk > 0.03f || h8bh_feedR >= 0) ? 1 : 0;
  int roll = (int)(60.0f * sinf(t * 0.031f) + 25.0f * sinf(t * 0.017f + 1.0f));   // the whole view rolls gently
  int cr = v3_cos(roll), sr = v3_sin(roll);
  int cx = 160 + (int)(10.0f * sinf(t * 0.043f)), cy = 120 + (int)(6.0f * sinf(t * 0.057f + 2.0f));

  // ── events: FEED (one hole) and MERGE (two) ──
  h8bh_auto -= dt;
  bool fire = v6_k12hit || h8bh_auto <= 0;
  if (fire) {
    h8bh_auto = 35 + v3_rndf() * 25;
    if (binary) { if (h8bh_merging == 0) h8bh_merging = 1; }
    else if (h8bh_feedR < 0) { h8bh_feedR = (float)h8bh_bMax + 6.0f; h8bh_feedPhi = (float)v3_rnd(1024); }
  }
  float spinK = 40.0f + h8bh_spin * h8bh_spin * 900.0f;                          // inner band, steps of 1024 per second
  for (int b = 0; b < H8BH_NB; b++) {
    h8bh_ph[b] += sdt * spinK * h8bh_om[b]; if (h8bh_ph[b] > 1024.0f) h8bh_ph[b] -= 1024.0f;
    h8bh_phI[b] = (int16_t)h8bh_ph[b];
  }
  h8bh_tw = (int)(t * 60.0f);
  h8bh_feedB = -1;
  if (h8bh_feedR >= 0) {
    int fb = v3_clamp((int)h8bh_feedR, 0, H8BH_NB - 1);
    h8bh_feedPhi += sdt * spinK * h8bh_om[fb] * 1.3f; if (h8bh_feedPhi > 1024.0f) h8bh_feedPhi -= 1024.0f;
    h8bh_feedR -= dt * (4.0f + 60.0f / (h8bh_feedR + 3.0f));                    // falls faster as it gets closer
    float done = 1.0f - v3_clampf(h8bh_feedR / (float)(h8bh_bMax + 6), 0.0f, 1.0f);
    h8bh_feedB = fb; h8bh_feedA = (int)h8bh_feedPhi; h8bh_feedArc = 30 + (int)(done * done * 420.0f);   // stretched longer and longer
    if (h8bh_feedR <= 0) { h8bh_feedR = -1; h8bh_flare = 1.0f; }
  }
  if (h8bh_merging == 1) {
    h8bh_sep -= dt * 0.06f / (h8bh_sep + 0.08f);                                // the chirp: ever faster
    if (h8bh_sep < 0.1f) { h8bh_sep = 0.0f; h8bh_merging = 2; h8bh_flare = 1.0f; }
  } else if (h8bh_merging == 2) {
    h8bh_sep += dt * 0.06f; if (h8bh_sep >= 1.0f) { h8bh_sep = 1.0f; h8bh_merging = 0; }
  }
  h8bh_orb += sdt * 0.25f / (h8bh_sep * sqrtf(h8bh_sep) + 0.04f); if (h8bh_orb > 6.2832f) h8bh_orb -= 6.2832f;
  if (h8bh_flare > 0) { h8bh_flare -= dt * 0.4f; if (h8bh_flare < 0) h8bh_flare = 0; }
  float fl = h8bh_flare;

  // ── the disk's light per band ──
  float dk = v3_clampf(h8bh_disk * 1.4f, 0.0f, 1.2f) * (1.0f + fl * 0.8f);
  for (int b = 0; b < H8BH_NB; b++) {
    int p = 0;
    if (b < h8bh_bMax) {
      p = 255 * 16 / (16 + b);
      int edge = h8bh_bMax - b; if (edge < 14) p = p * edge / 14;
      if (b < 2) p = p * (b + 2) / 4;
      p = (int)(p * dk);
    }
    h8bh_prof[b] = (uint8_t)v3_clamp(p, 0, 255);
    h8bh_bhue[b] = (uint8_t)(b * 3 / 2);
  }

  // ── binary: where the holes are ──
  int nh = 1, hx[2] = { 0, 0 }, hy[2] = { 0, 0 }, hr = rs, hE2 = rsE2;
  if (binary && h8bh_sep > 0.1f) {
    nh = 2; hr = v3_clamp(rs * 3 / 5, 5, 120); hE2 = (hr * 8 / 5) * (hr * 8 / 5);
    int D = (int)(h8bh_sep * (float)(hr * 3 + 30));
    int ox = (int)(D * cosf(h8bh_orb)), oy = (int)(D * sinf(h8bh_orb)) * ci / 256;
    hx[0] = ox; hy[0] = oy; hx[1] = -ox; hy[1] = -oy;
  } else if (binary) { hr = rs * 3 / 4; hE2 = (hr * 8 / 5) * (hr * 8 / 5); }
  h8bh_ringW = nh == 2 ? 14 : 7;
  int hr2 = hr * hr, ringOut = hr * (12 + h8bh_ringW) / 10, ringOut2 = ringOut * ringOut;
  int rin2 = rin * rin, rout = rin * h8bh_bMax / 16 + rin, rout2 = rout * rout;
  bool primary = !binary && diskOn;
  int ciInv = 65536 / ci;
  int jetA = binary ? 0 : (int)(h8bh_jets * 256.0f * (1.0f + fl * 1.5f));

  // background motion
  int bx = (int)(t * 9.0f), by = (int)(t * 5.0f);
  int n1 = (int)(t * 30), n2 = (int)(t * 23), n3 = (int)(t * 17);
  int bloom = (int)(fl * fl * 90.0f);
  uint8_t* LEV = V8_LEV; uint8_t* HUE = V8_HUE;

  for (int j = 0; j < V8_GH; j++) {
    int py0 = j * 4 + 2 - cy;
    for (int i = 0; i < V8_GW; i++) {
      int px0 = i * 4 + 2 - cx;
      int px = (px0 * cr - py0 * sr) >> 14, py = (px0 * sr + py0 * cr) >> 14;   // rolled view
      // lens: bend the background toward each hole
      int qx = px, qy = py, inside = -1;
      int dxs[2], dys[2], r2s[2];
      for (int h = 0; h < nh; h++) {
        int dx = px - hx[h], dy = py - hy[h], r2 = dx * dx + dy * dy;
        dxs[h] = dx; dys[h] = dy; r2s[h] = r2;
        if (r2 < hr2) { inside = h; continue; }
        int f = (hE2 << 8) / r2;
        qx -= (dx * f) >> 8; qy -= (dy * f) >> 8;
      }
      int lev = 0, hue = 0;
      // background, looked up where the light came from
      if (inside >= 0 && space != H8BH_WORM) { lev = 0; hue = 0; }
      else if (inside >= 0) {                                                  // WORMHOLE: another universe through the hole
        int dx = dxs[inside], dy = dys[inside];
        int r = (int)v3_isqrt((uint32_t)r2s[inside]);
        int a = v3_atan(dy, dx);
        int z = hr * 64 / (r + 4);                                             // depth down the tunnel
        int32_t s = v3_sin(z * 40 - n1 * 6) + v3_sin(a * 5 + z * 20 + n2 * 2);
        lev = 105 + (s >> 9);
        int rim = (hr - r) * 256 / (hr / 3 + 1); if (rim < 256) lev = (lev * rim) >> 8;   // dark rim keeps the shadow's edge
        hue = 150 + z * 3 + (s >> 11);
      } else {
        int gx = qx + bx, gy = qy + by;
        int32_t s = v3_sin(gx * 5 + n1) + v3_sin(gy * 7 - n2) + v3_sin(((gx + gy) * 3) + n3);   // ±49152
        if (space == H8BH_GRID) {
          int mx = gx & 31, my = gy & 31; if (mx > 16) mx = 32 - mx; if (my > 16) my = 32 - my;
          int d = mx < my ? mx : my;
          lev = 10 + (d < 5 ? (5 - d) * 26 : 0);
          hue = 140 + (((gx >> 5) + (gy >> 5)) & 3) * 14 + (s >> 12);
        } else if (space == H8BH_STARS) {
          int sx = gx >> 4, sy = gy >> 4;
          uint32_t hs = v3_hash((uint32_t)(sx * 73856093) ^ (uint32_t)(sy * 19349663));
          lev = 34 + (s >> 11);
          hue = 150 + (s >> 10);
          if ((hs & 255) < 110) {
            int d = v3_abs((gx & 15) - (int)((hs >> 8) & 15)) + v3_abs((gy & 15) - (int)((hs >> 12) & 15));
            if (d < 5) { lev += (5 - d) * (30 + (int)((hs >> 16) & 31)); hue = 128 + (int)((hs >> 21) & 63); }
          }
        } else {
          lev = 42 + (s >> 10);                                                // NEBULA: dim, so the hole is the star
          hue = 140 + (s >> 9);
        }
        if (inside < 0 && nh == 1) {                                           // soft edge on the shadow
          int e = r2s[0] - hr2, ew = hr * 6 + 9;
          if (e < ew) lev = lev * e / ew;
        }
      }
      if (lev < 0) lev = 0;
      // the bent far side of the disk and the photon ring, round each hole
      for (int h = 0; h < nh; h++) {
        if (inside == h || r2s[h] > ringOut2) continue;
        int r = (int)v3_isqrt((uint32_t)r2s[h]), rh = hue;
        int add = h8bh_ring(dxs[h], dys[h], r, hr, diskOn, &rh);
        if (add > 0) { hue = h8bh_mix(hue, lev, rh, add); lev += add; }
      }
      // the disk itself, a flattened ellipse: the near half passes in front of the hole
      if (primary) {
        int dy = (py * ciInv) >> 8, rho2 = px * px + dy * dy;
        if (rho2 >= rin2 && rho2 < rout2 && (py > 0 || inside < 0)) {
          int rho = (int)v3_isqrt((uint32_t)rho2);
          int b = (rho - rin) * 16 / rin;
          if (b >= 0 && b < h8bh_bMax) {
            int a = v3_atan(dy, px), hh;
            int l = h8bh_tex(a, b, &hh);
            int c = v3_cos(a);
            l = (l * (256 + ((c * h8bh_dop) >> 14))) >> 8;
            hh -= c >> 11;
            if (py > 0) { lev = (lev * (256 - v3_clamp(l, 0, 256))) >> 8; }       // the near side hides what is behind it
            hue = h8bh_mix(hue, lev, hh, l);
            lev += l;
          }
        }
      }
      // jets out of the poles
      if (jetA) {
        int ay = v3_abs(py), w = 2 + (ay >> 3);
        if (ay > hr * 7 / 10 && v3_abs(px) < w && (py > 0 || inside < 0)) {
          int k = (w - v3_abs(px)) * 256 / w;
          int knot = 160 + (v3_sin(ay * 9 - (int)(t * 900.0f)) >> 8);         // knots racing outward
          int fade = 256 - ay * 256 / 200; if (fade < 0) fade = 0;
          int jl = (((k * knot) >> 8) * fade >> 8) * jetA >> 8;
          hue = h8bh_mix(hue, lev, 100 + (ay >> 2), jl);
          lev += jl;
        }
      }
      lev += bloom;
      int c = j * V8_GW + i;
      LEV[c] = (uint8_t)v3_clamp(lev, 0, 226); HUE[c] = (uint8_t)(hue & 255);
    }
  }
  v8_upscale(5);
  v8_end();
}
