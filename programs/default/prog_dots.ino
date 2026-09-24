// =====================================================================
// PROGRAM: DOT CUBES (by Dewey)
// White dots on black: cubes built from lattices of dots, drifting and
// turning in space, with an endless zoom. Approximate take on a
// jn3008.art "zooming loop" of dots and cubes.
//
// Presets:
//   k0 Cube Field — fly forward through a field of dot cubes, forever
//   k1 Hex Zoom   — flat cube-hexagons of dots, endlessly zooming in
//   k2 Big Cube   — one large dot cube turning, a smaller one inside
//   k3 Moire      — two dot lattices sliding over each other
//   k4 Dot Sphere — spheres of dots turning among the cubes
//   k5 Tunnel     — fly down a square tunnel of dots
//   k6 Nested     — cubes inside cubes inside cubes, endless zoom
//
// This program OWNS the global knobs (it makes its own colours):
//   p0 Colors (white, amber, cyan, magenta, slow rainbow)
//   p1 Speed (zoom / fly speed; far left = stop)
//   p2 Spin (how fast the cubes turn)
//   p3 Density (dots per cube edge)
//   p4 Zoom    p5 Dot Size    p6 Background (sparse dots)    p7 Fog
//   p8 Tilt    p9 Sway (camera drifts side to side)
// =====================================================================

#define DC_LEVELS 63          // palette 1–63: dot brightness

// ─── Knobs (smoothed) ─────────────────────────────────────────────────
static float dc_sm[16];
static bool  dc_smReady = false;

static void dc_smoothKnobs(float dt) {
  float a = dt / 0.12f;
  if (a > 1.0f) a = 1.0f;
  for (int i = 0; i < 16; i++) {
    if (!dc_smReady) dc_sm[i] = pots[i];
    else dc_sm[i] += (pots[i] - dc_sm[i]) * a;
  }
  dc_smReady = true;
}
static inline float dc_potf(int idx, float lo, float hi) { return lo + (hi - lo) * (dc_sm[idx] / 1023.0f); }
static inline int dc_pot(int idx, int lo, int hi) {
  int v = (int)floorf(lo + (hi - lo + 1) * (dc_sm[idx] / 1024.0f));
  return v < lo ? lo : (v > hi ? hi : v);
}

// ─── Colours ──────────────────────────────────────────────────────────
static int dc_palKey = -1;

static void dc_buildPalette(int scheme, int hueStep) {
  int key = scheme * 1000 + (scheme == 4 ? hueStep : 0);
  if (key == dc_palKey) return;
  dc_palKey = key;
  static const uint8_t C[4][3] = {{255, 255, 255}, {255, 190, 90}, {120, 230, 255}, {255, 110, 220}};
  uint8_t col[3];
  if (scheme < 4) { col[0] = C[scheme][0]; col[1] = C[scheme][1]; col[2] = C[scheme][2]; }
  else {                                                   // slow rainbow
    float h = hueStep / 256.0f * 6.0f; int k = (int)h; float q = h - k;
    float r[6] = {1, 1 - q, 0, 0, q, 1}, g[6] = {q, 1, 1, 1 - q, 0, 0}, b[6] = {0, 0, q, 1, 1, 1 - q};
    k = k > 5 ? 5 : k;
    col[0] = (uint8_t)(90 + 165 * r[k]); col[1] = (uint8_t)(90 + 165 * g[k]); col[2] = (uint8_t)(90 + 165 * b[k]);
  }
  display.setColor(0, 0, 0, 0);
  for (int i = 1; i <= DC_LEVELS; i++) {
    float t = i / (float)DC_LEVELS;
    t = t * t;
    display.setColor(i, (uint8_t)(col[0] * t), (uint8_t)(col[1] * t), (uint8_t)(col[2] * t));
  }
  for (int i = DC_LEVELS + 1; i < 256; i++) display.setColor(i, col[0], col[1], col[2]);
}

// ─── Dots ─────────────────────────────────────────────────────────────
static uint8_t* dc_buf;
static float dc_dotSize = 1.0f;       // p5
static float dc_fog = 0.05f;          // p7
static float dc_f = 200.0f;           // focal length (zoom)
static int   dc_count = 0;            // dots drawn this frame (safety limit)
#define DC_MAXDOTS 7000     // (everything max-blends, so draw order doesn't matter: near things go first)

static inline void dc_px(int x, int y, uint8_t c) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H) return;
  uint8_t& p = dc_buf[y * W + x];
  if (p < c) p = c;
}

// A round dot at screen (x, y), radius r px, brightness level 1–63
static void dc_dotLv(float x, float y, float r, int lv) {
  if (++dc_count > DC_MAXDOTS) return;
  if (x < -4 || y < -4 || x > W + 4 || y > H + 4) return;
  if (lv > DC_LEVELS) lv = DC_LEVELS;
  int ix = (int)x, iy = (int)y;
  uint8_t c = (uint8_t)lv;
  if (r < 0.75f) { dc_px(ix, iy, c); return; }
  if (r < 1.4f) {                                        // small plus
    dc_px(ix, iy, c);
    uint8_t d = (uint8_t)(lv * 2 / 3 > 0 ? lv * 2 / 3 : 1);
    dc_px(ix + 1, iy, d); dc_px(ix - 1, iy, d); dc_px(ix, iy + 1, d); dc_px(ix, iy - 1, d);
    return;
  }
  int R = (int)(r + 0.5f);
  if (R > 5) R = 5;
  int rr = (int)(r * r);
  for (int dy = -R; dy <= R; dy++)
    for (int dx = -R; dx <= R; dx++)
      if (dx * dx + dy * dy <= rr) dc_px(ix + dx, iy + dy, c);
}
// …same, brightness 0–1
static inline void dc_dot(float x, float y, float r, float b) {
  int lv = (int)(b * DC_LEVELS);
  if (lv >= 1) dc_dotLv(x, y, r, lv);
}

// A 3D point seen by the camera (camera at 0,0,0 looking along +z)
static inline void dc_dot3(float x, float y, float z, float worldR) {
  if (z < 0.15f) return;
  float iz = 1.0f / z;
  float sx = HALFW + x * dc_f * iz, sy = HALFH + y * dc_f * iz;
  float b = expf(-z * dc_fog);
  float r = worldR * dc_f * iz * dc_dotSize;
  dc_dot(sx, sy, r, b);
}

// Cheap repeatable "random" from integers
static inline uint32_t dc_hash(int a, int b, int c) {
  uint32_t h = (uint32_t)a * 73856093u ^ (uint32_t)b * 19349663u ^ (uint32_t)c * 83492791u;
  h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
  return h;
}
static inline float dc_h01(uint32_t h, int shift) { return ((h >> shift) & 1023) / 1023.0f; }

// ─── A cube of dots ───────────────────────────────────────────────────
// Centre (cx,cy,cz), half-size s, rotated by angles ax (tilt) and ay (turn).
// Only faces turned toward the camera are drawn. n = dots per edge.
static void dc_cube(float cx, float cy, float cz, float s, float ax, float ay, int n) {
  if (cz + s < 0.2f) return;
  // Keep the dot pattern readable: fewer dots when the cube is small on screen
  float proj = s * dc_f / (cz > 0.3f ? cz : 0.3f);
  int nMax = (int)(proj / 2.5f);
  if (n > nMax) n = nMax;
  if (n < 2) { dc_dot3(cx, cy, cz, s * 0.15f); return; }
  float ca = cosf(ay), sa = sinf(ay), cb = cosf(ax), sb = sinf(ax);
  // rotation: turn about y, then tilt about x. Columns = rotated x, y, z axes
  float X[3] = {ca, sa * sb, -sa * cb};
  float Y[3] = {0, cb, sb};
  float Z[3] = {sa, -ca * sb, ca * cb};
  const float* ax3[3] = {X, Y, Z};
  float dotR = s / n * 0.18f;
  for (int f = 0; f < 6; f++) {
    const float* N = ax3[f >> 1];
    float sign = (f & 1) ? -1.0f : 1.0f;
    float nx = N[0] * sign, ny = N[1] * sign, nz = N[2] * sign;
    // face centre; visible if its normal points back toward the camera
    float fx = cx + nx * s, fy = cy + ny * s, fz = cz + nz * s;
    if (nx * fx + ny * fy + nz * fz >= 0) continue;
    const float* U = ax3[((f >> 1) + 1) % 3];
    const float* V = ax3[((f >> 1) + 2) % 3];
    // Brightness and dot size are worked out once per face (from its centre)
    if (fz < 0.2f) continue;
    float bright = expf(-fz * dc_fog);
    float rad = dotR * dc_f / fz * dc_dotSize;
    int lv = (int)(bright * DC_LEVELS);
    if (lv < 1) continue;
    // Step across the face in 3D; only the divide is done per dot
    float st = 2.0f * s / n;
    float sx0 = fx + (U[0] + V[0]) * (st * 0.5f - s), sy0 = fy + (U[1] + V[1]) * (st * 0.5f - s), sz0 = fz + (U[2] + V[2]) * (st * 0.5f - s);
    float ux = U[0] * st, uy = U[1] * st, uz = U[2] * st;
    float vx = V[0] * st, vy = V[1] * st, vz = V[2] * st;
    for (int i = 0; i < n; i++) {
      float px = sx0 + ux * i, py = sy0 + uy * i, pz = sz0 + uz * i;
      for (int j = 0; j < n; j++) {
        if (pz > 0.15f) {
          float iz = dc_f / pz;
          dc_dotLv(HALFW + px * iz, HALFH + py * iz, rad, lv);
        }
        px += vx; py += vy; pz += vz;
      }
    }
  }
}

// ─── Background: a sparse field of dots drifting slowly sideways ──────
static void dc_background(float amount, float travel) {
  if (amount < 0.02f) return;
  const float sp = 22.0f;
  float shift = travel * 6.0f;
  int si = (int)floorf(shift / sp);
  float off = shift - si * sp;
  float b = amount * 0.55f;
  int nx = (int)(HALFW / sp) + 2, ny = (int)(HALFH / (sp * 0.866f)) + 2;
  for (int j = -ny; j <= ny; j++)
    for (int i = -nx; i <= nx; i++) {
      uint32_t h = dc_hash(i + si, j, 7);
      if (dc_h01(h, 0) > amount) continue;
      float x = HALFW + (i + ((j & 1) ? 0.5f : 0.0f)) * sp - off + (dc_h01(h, 10) - 0.5f) * sp * 0.6f;
      float y = HALFH + j * sp * 0.866f + (dc_h01(h, 20) - 0.5f) * sp * 0.6f;
      dc_dot(x, y, 0.8f * dc_dotSize, b);
    }
}

// ─── Presets ──────────────────────────────────────────────────────────
// k0: field of cubes; camera flies forward through repeating cells
static void dc_field(float z0, float t, float spin, int n, float swayX) {
  const float C = 3.0f;                                  // cell size
  int zc0 = (int)floorf(z0 / C);
  for (int zc = zc0; zc <= zc0 + 8; zc++) {              // near first (so a dot cap drops far ones)
    for (int yc = -1; yc <= 1; yc++)
      for (int xc = -2; xc <= 2; xc++) {
        uint32_t h = dc_hash(xc, yc, zc);
        if (dc_h01(h, 0) < 0.45f) continue;              // empty cell
        float cx = (xc + 0.2f + 0.6f * dc_h01(h, 10)) * C - swayX;
        float cy = (yc + 0.2f + 0.6f * dc_h01(h, 20)) * C * 0.8f;
        float cz = (zc + 0.5f) * C - z0;
        float s = 0.35f + 0.3f * dc_h01(h, 5);
        float ph = dc_h01(h, 15) * TWO_PI;
        dc_cube(cx, cy, cz, s, 0.6f + 0.3f * sinf(t * 0.3f + ph), t * spin + ph, n);
      }
  }
}

// k1: flat hexagons (cubes seen corner-on), each face a dot lattice,
// on a triangular grid; three zoom levels blend for an endless zoom.
static void dc_hexLevel(float scale, float weight, int levelKey, int n, float rot) {
  if (weight < 0.03f) return;
  float R = scale;                                        // hexagon radius (px)
  float gx = R * 1.732f, gy = R * 1.5f;                   // grid spacing
  int nx = (int)(HALFW * 1.3f / gx) + 2, ny = (int)(HALFH * 1.3f / gy) + 2;
  float cr = cosf(rot), sr = sinf(rot);
  // the three face directions of a cube seen from its corner
  float ex[3], ey[3];
  for (int k = 0; k < 3; k++) { float a = rot + PI * 0.5f + k * TWO_PI / 3; ex[k] = cosf(a) * R; ey[k] = sinf(a) * R; }
  int m = n;
  float nmax = R / 4.0f;
  if (m > nmax) m = (int)nmax;
  if (m < 2) return;
  for (int j = -ny; j <= ny; j++)
    for (int i = -nx; i <= nx; i++) {
      uint32_t h = dc_hash(i, j, levelKey);
      if (dc_h01(h, 0) < 0.5f) continue;
      float lx = (i + ((j & 1) ? 0.5f : 0.0f)) * gx, ly = j * gy;
      float x0 = HALFW + lx * cr - ly * sr, y0 = HALFH + lx * sr + ly * cr;
      if (x0 < -R || x0 > W + R || y0 < -R || y0 > H + R) continue;
      // each rhombus face: spanned by two of the three corner directions
      for (int k = 0; k < 3; k++) {
        float ux = ex[k], uy = ey[k], vx = ex[(k + 1) % 3], vy = ey[(k + 1) % 3];
        float shade = weight * (0.55f + 0.2f * k);
        for (int a = 0; a < m; a++)
          for (int b = 0; b < m; b++) {
            float fa = (a + 0.5f) / m, fb = (b + 0.5f) / m;
            dc_dot(x0 + ux * fa + vx * fb, y0 + uy * fa + vy * fb, R / m * 0.16f * dc_dotSize, shade);
          }
      }
    }
}

static void dc_sphere(float cx, float cy, float cz, float r, float ay, float ax, int count) {
  float ca = cosf(ay), sa = sinf(ay), cb = cosf(ax), sb = sinf(ax);
  for (int i = 0; i < count; i++) {
    float y = 1.0f - (i + 0.5f) * 2.0f / count;
    float rr = sqrtf(1.0f - y * y);
    float a = i * 2.39996f;                               // golden angle
    float x = cosf(a) * rr, z = sinf(a) * rr;
    float x1 = x * ca + z * sa, z1 = -x * sa + z * ca;
    float y2 = y * cb - z1 * sb, z2 = y * sb + z1 * cb;
    if (z2 > 0.15f) continue;                             // back half hidden
    dc_dot3(cx + x1 * r, cy + y2 * r, cz + z2 * r, r * 0.035f);
  }
}

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_dots_name() { return "DOT CUBES"; }
const char* prog_dots_character() { return "Cubes of dots drifting, turning and zooming forever"; }

static const char* const dc_presetNames[] = {
  "Cube Field", "Hex Zoom", "Big Cube", "Moire", "Dot Sphere", "Tunnel", "Nested"
};
#define DC_NUM_PRESETS 7

const char* prog_dots_presetName(int preset) {
  if (preset >= 0 && preset < DC_NUM_PRESETS) return dc_presetNames[preset];
  return NULL;
}

static const char* const dc_labels[10] = {
  "Colors", "Speed", "Spin", "Density", "Zoom", "Dot Size", "Background", "Fog", "Tilt", "Sway"
};
const char* prog_dots_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 10) return dc_labels[pot];
  return "";
}

uint8_t prog_dots_renderHint(int preset) {
  (void)preset;
  return RENDER_CLEAR;
}

void prog_dots_init() {
  dc_palKey = -1;
}

void prog_dots_draw(int preset) {
  dc_buf = display.getBuffer();
  dc_count = 0;

  // Own clock (this program owns p1)
  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  dc_smoothKnobs(dt);

  int scheme = dc_pot(0, 0, 4);
  float sk = dc_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.1f * powf(30.0f, (sk - 0.03f) / 0.97f);   // 0.1 … 3
  float spin = dc_potf(2, 0.0f, 1.2f);
  int n = dc_pot(3, 3, 10);
  float kz = dc_sm[4] / 1023.0f;
  float zoom = (kz < 0.5f) ? powf(0.5f, (0.5f - kz) * 2.0f) : powf(3.0f, (kz - 0.5f) * 2.0f);
  dc_dotSize = dc_potf(5, 0.4f, 2.5f);
  float bg = dc_potf(6, 0.0f, 1.0f);
  dc_fog = dc_potf(7, 0.0f, 0.15f);
  float tilt = dc_potf(8, -1.2f, 1.2f);
  float sway = dc_potf(9, 0.0f, 1.0f);
  dc_f = 180.0f * zoom;

  static float t = 0, hueT = 0;
  t += dt;
  hueT += dt * 0.03f;
  dc_buildPalette(scheme, (int)(hueT * 256) & 255);

  static float travel = 0;                              // how far we've flown / zoomed
  travel += dt * speed;
  float swayX = sway * 1.5f * sinf(t * 0.23f);

  // Endless zoom phase (0–1, wraps): used by the flat and nested presets
  static float zph = 0;
  zph += dt * speed * 0.25f;
  if (zph >= 1.0f) zph -= 1.0f;
  static int zcount = 0;                                // how many times it wrapped
  static float lastZ = 0;
  if (zph < lastZ) zcount++;
  lastZ = zph;

  if (preset != 3) dc_background(bg * 0.6f, travel);

  switch (preset) {
    case 1: {   // Hex Zoom
      float base = 7.0f * zoom;
      for (int L = 0; L < 3; L++) {
        float s = base * powf(3.0f, L + zph);
        float w = 1.0f;
        float pos = (L + zph) / 3.0f;                      // 0 = smallest, 1 = biggest
        if (pos < 0.25f) w = pos / 0.25f;                  // fade in small
        if (pos > 0.7f) w = (1.0f - pos) / 0.3f;           // fade out big
        dc_hexLevel(s, w, L - zcount, n, t * spin * 0.2f + tilt * 0.3f);
      }
      break;
    }
    case 2: {   // Big Cube (and one inside it)
      float ay = t * spin, ax = 0.62f + tilt * 0.5f;
      dc_cube(swayX * 0.3f, 0, 4.2f, 1.3f, ax, ay, n + 2);
      dc_cube(swayX * 0.3f, 0, 4.2f, 0.55f, -ax * 0.8f, -ay * 1.3f, n);
      break;
    }
    case 3: {   // Moire: two dot lattices, one slightly turned and scaled
      float sp = 9.0f * zoom * (1.0f + 0.25f * sinf(travel * 0.5f));
      if (sp < 7.0f) sp = 7.0f;
      for (int L = 0; L < 2; L++) {
        float a = L ? 0.06f * sinf(t * spin * 0.5f) + tilt * 0.05f : 0.0f;
        float s = L ? sp * 1.04f : sp;
        float ca = cosf(a), sa = sinf(a);
        int nx = (int)(W * 0.62f / s) + 1, ny = (int)(H * 0.62f / (s * 0.866f)) + 1;
        for (int j = -ny; j <= ny; j++)
          for (int i = -nx; i <= nx; i++) {
            float lx = (i + ((j & 1) ? 0.5f : 0.0f)) * s + swayX * 10.0f * L, ly = j * s * 0.866f;
            dc_dot(HALFW + lx * ca - ly * sa, HALFH + lx * sa + ly * ca, 0.9f * dc_dotSize, 0.85f);
          }
      }
      break;
    }
    case 4: {   // Dot spheres among cubes
      for (int k = 0; k < 3; k++) {
        float a = t * 0.2f + k * TWO_PI / 3;
        float x = cosf(a) * 1.8f - swayX, z = 5.0f + sinf(a) * 1.5f;
        dc_sphere(x, 0.3f * sinf(t * 0.4f + k), z, 0.8f, t * spin + k, tilt * 0.5f, 60 + n * 25);
        dc_cube(cosf(a + 1.0f) * 2.2f - swayX, -0.9f, 6.0f + sinf(a + 1.0f), 0.4f, 0.6f, -t * spin + k, n);
      }
      break;
    }
    case 5: {   // Tunnel: dots on the walls of a square tunnel
      const float step = 0.5f;
      float zOff = fmodf(travel * 2.0f, step);
      float roll = tilt * 0.4f + sinf(t * 0.2f) * sway * 0.4f;
      float cr = cosf(roll), sr = sinf(roll);
      int per = 4 + n;                                      // dots across each wall
      for (int zi = 40; zi >= 0; zi--) {
        float z = zi * step - zOff + 0.3f;
        for (int w = 0; w < 4; w++)
          for (int k = 0; k < per; k++) {
            float u = -1.0f + (2.0f * k + 1.0f) / per;
            float x, y;
            if (w == 0)      { x = u * 1.6f; y = -1.2f; }
            else if (w == 1) { x = u * 1.6f; y = 1.2f; }
            else if (w == 2) { x = -1.6f; y = u * 1.2f; }
            else             { x = 1.6f;  y = u * 1.2f; }
            dc_dot3(x * cr - y * sr, x * sr + y * cr, z, 0.035f);
          }
      }
      break;
    }
    case 6: {   // Nested cubes: each 2.2× the last, endless zoom inward
      float grow = powf(2.2f, zph);
      for (int L = 5; L >= 0; L--) {
        float s = 0.25f * powf(2.2f, (float)L) * grow;
        if (s > 6.0f) continue;
        float z = 4.5f;
        if (s > 3.2f) continue;                               // too close — skip
        int lk = L - zcount;
        dc_cube(swayX * 0.2f, 0, z, s, 0.62f + tilt * 0.5f + 0.2f * (lk & 1),
                t * spin * ((lk & 1) ? -0.7f : 1.0f) + lk * 0.8f, n);
      }
      break;
    }
    default:    // Cube Field
      dc_field(travel * 3.0f, t, spin, n, swayX);
      break;
  }
}
