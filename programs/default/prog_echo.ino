// =====================================================================
// PROGRAM: ECHO TRAILS (by Dewey)
// Glowing neon line figures that leave a stack of fading, colour-shifting
// echoes behind them, with sparkles drifting through the dark.
// Rough take on the "neon outline + echo trails" look (inspired by a
// riebschlager post) — with original figures.
//
// How the echoes work (no extra memory — the picture IS the memory):
//   palette 1–199   echoes: the higher the number, the newer/brighter.
//                   Every frame each echo pixel counts down (fades) and
//                   the whole echo layer can slide a little (Drift).
//   palette 220–255 the live figure, drawn fresh every frame. Every few
//                   frames (Spacing) it is "stamped" into the echo layer
//                   as a new echo; otherwise it is wiped before redrawing.
//
// Presets:
//   k0 Dancer    — a line-drawn figure dancing
//   k1 Duet      — two dancers, mirrored
//   k2 Jelly     — a jellyfish pulsing, tentacles waving
//   k3 Ribbon    — a looping ribbon (Lissajous curve)
//   k4 Bloom     — a spinning, morphing flower
//   k5 Contours  — stacked wavy lines, like a moving landscape
//   k6 Orbit     — rings circling each other
//
// This program OWNS the global knobs (it makes its own colours):
//   p0 Colors  — slime (green→blue), fire, ice, sunset, rainbow
//   p1 Speed   (far left = frozen)
//   p2 Trail   (left = short echoes, right = long)
//   p3 Spacing (left = smooth smear, right = separate echoes)
//   p4 Size    p5 Sparkles    p6 Drift direction    p7 Drift amount
//   p8 Hue Cycle (colours slowly rotate; left = off)
//   p9 Lines   (1–3 nested outlines)    p10 Energy (how wild it moves)
// =====================================================================

#define EC_ECHO_TOP  199     // newest echo
#define EC_LIVE      220     // live figure: 220–255
#define EC_CORE      255
#define EC_HALO      232
#define EC_SPARK     244
#define EC_NSPARK    48

// ─── Knobs (smoothed) ─────────────────────────────────────────────────
static float ec_sm[16];
static bool  ec_smReady = false;

static void ec_smoothKnobs(float dt) {
  float a = dt / 0.12f;
  if (a > 1.0f) a = 1.0f;
  for (int i = 0; i < 16; i++) {
    if (!ec_smReady) ec_sm[i] = pots[i];
    else ec_sm[i] += (pots[i] - ec_sm[i]) * a;
  }
  ec_smReady = true;
}
static inline float ec_potf(int idx, float lo, float hi) { return lo + (hi - lo) * (ec_sm[idx] / 1023.0f); }
static inline int ec_pot(int idx, int lo, int hi) {
  int v = (int)floorf(lo + (hi - lo + 1) * (ec_sm[idx] / 1024.0f));
  return v < lo ? lo : (v > hi ? hi : v);
}

// ─── Colours ──────────────────────────────────────────────────────────
static void ec_hsv(float h, float s, float v, uint8_t* out) {
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
  r = (1 - s) + s * r; g = (1 - s) + s * g; b = (1 - s) + s * b;
  out[0] = (uint8_t)(r * v * 255); out[1] = (uint8_t)(g * v * 255); out[2] = (uint8_t)(b * v * 255);
}

static int ec_palKey = -1;

// scheme: base hue of the live figure, and how far the hue travels as echoes age
static void ec_buildPalette(int scheme, int hueStep) {
  int key = scheme * 1000 + hueStep;
  if (key == ec_palKey) return;
  ec_palKey = key;
  static const float base[5] = {0.30f, 0.10f, 0.52f, 0.92f, 0.0f};
  static const float span[5] = {0.30f, -0.10f, 0.15f, -0.25f, 1.0f};
  float h0 = base[scheme] + hueStep / 256.0f;
  uint8_t c[3];
  display.setColor(0, 0, 0, 0);
  for (int i = 1; i <= EC_ECHO_TOP; i++) {             // echoes: fade + hue drift with age
    float t = i / (float)EC_ECHO_TOP;                   // 1 = newest
    float v = t * t * 0.85f;
    ec_hsv(h0 + (1.0f - t) * span[scheme], 0.9f, v, c);
    display.setColor(i, c[0], c[1], c[2]);
  }
  for (int i = EC_ECHO_TOP + 1; i < EC_LIVE; i++) display.setColor(i, 0, 0, 0);
  for (int i = EC_LIVE; i < 256; i++) {                // live figure: bright, core almost white
    float t = (i - EC_LIVE) / (float)(255 - EC_LIVE);
    ec_hsv(h0, 1.0f - 0.75f * t * t, 0.75f + 0.25f * t, c);
    display.setColor(i, c[0], c[1], c[2]);
  }
}

// ─── Drawing (max-blend: never overwrite something brighter) ──────────
static uint8_t* ec_buf;

static inline void ec_put(int x, int y, uint8_t v) {
  if ((unsigned)x >= (unsigned)W || (unsigned)y >= (unsigned)H) return;
  uint8_t& p = ec_buf[y * W + x];
  if (p < v) p = v;
}
// A glowing pixel: bright core + dimmer halo around it
static inline void ec_glow(int x, int y) {
  ec_put(x, y, EC_CORE);
  ec_put(x + 1, y, EC_HALO); ec_put(x - 1, y, EC_HALO);
  ec_put(x, y + 1, EC_HALO); ec_put(x, y - 1, EC_HALO);
}

static void ec_line(float fx0, float fy0, float fx1, float fy1) {
  int x0 = (int)fx0, y0 = (int)fy0, x1 = (int)fx1, y1 = (int)fy1;
  // skip lines that are entirely off screen
  if ((x0 < -2 && x1 < -2) || (x0 > W + 1 && x1 > W + 1) || (y0 < -2 && y1 < -2) || (y0 > H + 1 && y1 > H + 1)) return;
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  for (int g = 0; g < 800; g++) {
    ec_glow(x0, y0);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

// Screen mapping: figure units → pixels
static float ec_cx, ec_cy, ec_S;
static inline float ec_X(float x) { return ec_cx + x * ec_S; }
static inline float ec_Y(float y) { return ec_cy + y * ec_S; }

static void ec_circle(float x, float y, float r) {
  int n = 10 + (int)(r * ec_S * 0.6f);
  if (n > 48) n = 48;
  float px = ec_X(x + r), py = ec_Y(y);
  for (int i = 1; i <= n; i++) {
    float a = i * TWO_PI / n;
    float qx = ec_X(x + r * cosf(a)), qy = ec_Y(y + r * sinf(a));
    ec_line(px, py, qx, qy);
    px = qx; py = qy;
  }
}

// A "tube": outline around a chain of points (like an arm), with a
// rounded cap at the far end. w0 → w1 = width at the start and the end.
static void ec_tube(const float* xs, const float* ys, int n, float w0, float w1) {
  float lx = 0, ly = 0, rx = 0, ry = 0;
  for (int i = 0; i < n; i++) {
    // direction at this point (average of neighbouring segments)
    float dx, dy;
    if (i == 0)          { dx = xs[1] - xs[0];         dy = ys[1] - ys[0]; }
    else if (i == n - 1) { dx = xs[i] - xs[i - 1];     dy = ys[i] - ys[i - 1]; }
    else                 { dx = xs[i + 1] - xs[i - 1]; dy = ys[i + 1] - ys[i - 1]; }
    float l = sqrtf(dx * dx + dy * dy);
    if (l < 1e-5f) { dx = 0; dy = 1; l = 1; }
    float nx = -dy / l, ny = dx / l;
    float w = w0 + (w1 - w0) * i / (float)(n - 1);
    float Lx = ec_X(xs[i] + nx * w), Ly = ec_Y(ys[i] + ny * w);
    float Rx = ec_X(xs[i] - nx * w), Ry = ec_Y(ys[i] - ny * w);
    if (i > 0) { ec_line(lx, ly, Lx, Ly); ec_line(rx, ry, Rx, Ry); }
    lx = Lx; ly = Ly; rx = Rx; ry = Ry;
    if (i == n - 1) {                                   // rounded tip
      float a0 = atan2f(ny, nx);
      float px = Lx, py = Ly;
      for (int k = 1; k <= 6; k++) {
        float a = a0 - k * PI / 6;
        float qx = ec_X(xs[i] + cosf(a) * w), qy = ec_Y(ys[i] + sinf(a) * w);
        ec_line(px, py, qx, qy);
        px = qx; py = qy;
      }
    }
  }
}

// ─── Figures ──────────────────────────────────────────────────────────
// All in figure units: about 1.0 tall, centred on (0,0), y down.

// Dancer: a simple jointed figure. t = dance time, e = energy, m = mirror (±1)
static void ec_dancer(float ox, float t, float e, float m, int lines) {
  float bob = 0.03f * e * sinf(t * 2.0f);
  float sway = 0.12f * e * sinf(t);
  float hipX = ox + sway * 0.4f, hipY = 0.12f + bob;
  float lean = m * 0.25f * e * sinf(t + 0.6f);
  float chX = hipX + sinf(lean) * 0.32f, chY = hipY - cosf(lean) * 0.32f;
  float nkX = hipX + sinf(lean) * 0.40f, nkY = hipY - cosf(lean) * 0.40f;
  float hdX = nkX + sinf(lean * 1.4f) * 0.08f, hdY = nkY - 0.08f;
  for (int c = 0; c < lines; c++) {
    float s = 1.0f - c * 0.33f;                         // nested outlines
    // torso
    float tx[3] = {hipX, (hipX + chX) * 0.5f, nkX}, ty[3] = {hipY, (hipY + chY) * 0.5f, nkY};
    ec_tube(tx, ty, 3, 0.075f * s, 0.06f * s);
    // head
    ec_circle(hdX, hdY, 0.075f * s);
    // arms
    for (int side = -1; side <= 1; side += 2) {
      float sd = side * m;
      float ph = t * 2.0f + (side > 0 ? 0 : PI * 0.7f);
      float up = -1.2f + 1.1f * e * sinf(ph);            // shoulder angle (from hanging down)
      float el = 0.4f + 0.9f * e * (0.5f + 0.5f * sinf(ph + 1.2f));
      float shX = chX + sd * 0.07f, shY = chY + 0.01f;
      float a1 = PI * 0.5f - sd * (PI * 0.5f + up);
      float elX = shX + cosf(a1) * 0.2f, elY = shY + sinf(a1) * 0.2f;
      float a2 = a1 - sd * el;
      float hnX = elX + cosf(a2) * 0.18f, hnY = elY + sinf(a2) * 0.18f;
      float ax[3] = {shX, elX, hnX}, ay[3] = {shY, elY, hnY};
      ec_tube(ax, ay, 3, 0.04f * s, 0.03f * s);
    }
    // legs
    for (int side = -1; side <= 1; side += 2) {
      float sd = side * m;
      float ph = t * 2.0f + (side > 0 ? 0 : PI);
      float lift = 0.5f + 0.5f * sinf(ph);
      float hpX = hipX + sd * 0.06f, hpY = hipY + 0.02f;
      float a1 = PI * 0.5f - sd * (0.15f + 0.5f * e * lift);
      float knX = hpX + cosf(a1) * 0.24f, knY = hpY + sinf(a1) * 0.24f;
      float a2 = PI * 0.5f + sd * 0.1f + sd * 0.8f * e * lift;
      float ftX = knX + cosf(a2) * 0.24f, ftY = knY + sinf(a2) * 0.24f;
      float lx[3] = {hpX, knX, ftX}, ly[3] = {hpY, knY, ftY};
      ec_tube(lx, ly, 3, 0.05f * s, 0.035f * s);
    }
  }
}

static void ec_jelly(float t, float e, int lines) {
  float pulse = 0.5f + 0.5f * sinf(t * 1.5f);
  float yb = -0.05f - 0.06f * pulse * e;                // bobs up as it pulses
  for (int c = 0; c < lines; c++) {
    float s = 1.0f - c * 0.28f;
    float R = 0.32f * s * (1.0f + 0.12f * e * (0.5f - pulse));
    float Ry = R * (0.8f - 0.15f * pulse * e);
    // dome
    float px = ec_X(-R), py = ec_Y(yb);
    for (int i = 1; i <= 24; i++) {
      float a = PI + i * PI / 24;
      float qx = ec_X(R * cosf(a)), qy = ec_Y(yb + Ry * sinf(a));
      ec_line(px, py, qx, qy); px = qx; py = qy;
    }
    // frilly rim
    px = ec_X(-R); py = ec_Y(yb);
    for (int i = 1; i <= 24; i++) {
      float x = -R + 2 * R * i / 24;
      float qx = ec_X(x), qy = ec_Y(yb + 0.025f * s * sinf(i * 1.9f + t * 4));
      ec_line(px, py, qx, qy); px = qx; py = qy;
    }
  }
  // tentacles
  for (int k = 0; k < 6; k++) {
    float x0 = -0.25f + k * 0.1f;
    float px = ec_X(x0), py = ec_Y(yb);
    for (int i = 1; i <= 14; i++) {
      float f = i / 14.0f;
      float x = x0 + 0.06f * e * f * sinf(t * 2.0f - f * 5.0f + k);
      float y = yb + f * (0.5f + 0.08f * sinf(k * 2.3f));
      float qx = ec_X(x), qy = ec_Y(y);
      ec_line(px, py, qx, qy); px = qx; py = qy;
    }
  }
}

static void ec_ribbon(float t, float e, int lines) {
  float a = 3, b = 2 + (e > 0.66f ? 1 : 0);
  for (int c = 0; c < lines; c++) {
    float s = 1.0f - c * 0.25f;
    float px = 0, py = 0;
    for (int i = 0; i <= 120; i++) {
      float u = i * TWO_PI / 120;
      float x = 0.55f * s * sinf(a * u + t * 0.7f);
      float y = 0.42f * s * sinf(b * u + t * 0.5f * (0.5f + e));
      float qx = ec_X(x), qy = ec_Y(y);
      if (i) ec_line(px, py, qx, qy);
      px = qx; py = qy;
    }
  }
}

static void ec_bloom(float t, float e, int lines) {
  int petals = 5 + ((int)(t * 0.15f) % 4);
  for (int c = 0; c < lines; c++) {
    float s = 1.0f - c * 0.3f;
    float px = 0, py = 0;
    for (int i = 0; i <= 96; i++) {
      float u = i * TWO_PI / 96;
      float r = 0.32f * s * (1.0f + (0.2f + 0.3f * e) * sinf(petals * u + t * (c & 1 ? -1.0f : 1.0f)));
      float qx = ec_X(r * cosf(u + t * 0.3f)), qy = ec_Y(r * sinf(u + t * 0.3f));
      if (i) ec_line(px, py, qx, qy);
      px = qx; py = qy;
    }
  }
}

static void ec_contours(float t, float e, int lines) {
  int rows = 3 + lines * 2;
  for (int k = 0; k < rows; k++) {
    float y0 = -0.4f + 0.8f * k / (rows - 1);
    float px = 0, py = 0;
    for (int i = 0; i <= 40; i++) {
      float x = -0.9f + 1.8f * i / 40;
      float y = y0 + (0.05f + 0.1f * e) * sinf(x * 5.0f + t + k * 0.7f) * sinf(t * 0.4f + k);
      float qx = ec_X(x), qy = ec_Y(y);
      if (i) ec_line(px, py, qx, qy);
      px = qx; py = qy;
    }
  }
}

static void ec_orbit(float t, float e, int lines) {
  int n = 3 + lines;
  for (int k = 0; k < n; k++) {
    float a = t * (0.6f + 0.2f * k) + k * TWO_PI / n;
    float R = 0.25f + 0.1f * e * sinf(t * 0.7f + k);
    ec_circle(R * cosf(a), R * 0.7f * sinf(a), 0.06f + 0.05f * (k % 3));
  }
  ec_circle(0, 0, 0.05f + 0.03f * sinf(t * 2));
}

// ─── Sparkles ─────────────────────────────────────────────────────────
static int16_t ec_spx[EC_NSPARK], ec_spy[EC_NSPARK];   // position × 4
static uint8_t ec_spv[EC_NSPARK];                      // rising speed

static void ec_resetSpark(int i, bool anywhere) {
  ec_spx[i] = (int16_t)random(0, W * 4);
  ec_spy[i] = (int16_t)(anywhere ? random(0, H * 4) : H * 4 + random(0, 40));
  ec_spv[i] = (uint8_t)random(2, 10);
}

// ─── Main ─────────────────────────────────────────────────────────────
const char* prog_echo_name() { return "ECHO TRAILS"; }
const char* prog_echo_character() { return "Neon line figures leaving fading, colour-shifting echoes"; }

static const char* const ec_presetNames[] = {
  "Dancer", "Duet", "Jelly", "Ribbon", "Bloom", "Contours", "Orbit"
};
#define EC_NUM_PRESETS 7

const char* prog_echo_presetName(int preset) {
  if (preset >= 0 && preset < EC_NUM_PRESETS) return ec_presetNames[preset];
  return NULL;
}

static const char* const ec_labels[11] = {
  "Colors", "Speed", "Trail", "Spacing", "Size", "Sparkles", "Drift Dir", "Drift", "Hue Cycle", "Lines", "Energy"
};
const char* prog_echo_potLabel(int preset, int pot) {
  (void)preset;
  if (pot >= 0 && pot < 11) return ec_labels[pot];
  return "";
}

uint8_t prog_echo_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;                 // the echoes live in the picture — never clear it
}

void prog_echo_init() {
  ec_palKey = -1;
  display.fillScreen(0);
  for (int i = 0; i < EC_NSPARK; i++) ec_resetSpark(i, true);
}

void prog_echo_draw(int preset) {
  ec_buf = display.getBuffer();

  // Own clock (this program owns p1)
  static unsigned long lastMs = 0;
  unsigned long now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;
  if (dt <= 0 || dt > 0.1f) dt = 0.016f;
  ec_smoothKnobs(dt);

  // Knobs
  int scheme = ec_pot(0, 0, 4);
  float sk = ec_sm[1] / 1023.0f;
  float speed = (sk < 0.03f) ? 0.0f : 0.15f * powf(20.0f, (sk - 0.03f) / 0.97f);   // 0.15× … 3×
  int fade = 1 + (int)((1.0f - ec_sm[2] / 1023.0f) * 11.0f);   // echo fade per stamp
  int spacing = ec_pot(3, 1, 10);                              // frames between echoes
  float size = ec_potf(4, 0.4f, 1.6f);
  int sparks = ec_pot(5, 0, EC_NSPARK);
  float ddir = ec_potf(6, 0.0f, TWO_PI);
  float damt = ec_potf(7, 0.0f, 3.0f);                         // px per echo
  float hueRate = ec_potf(8, 0.0f, 0.15f);                     // turns per second
  int lines = ec_pot(9, 1, 3);
  float energy = ec_potf(10, 0.1f, 1.0f);

  static float hue = 0;
  hue += hueRate * dt;
  if (hue > 1) hue -= 1;
  ec_buildPalette(scheme, (int)(hue * 256) & 255);

  static float t = 0;
  t += dt * speed * 2.0f;

  // 1) Update the echo layer. On a stamp frame the live figure becomes the
  //    newest echo; otherwise it's wiped. Echoes fade and slide (Drift).
  static int frame = 0;
  static float accX = 0, accY = 0;
  bool stamp = (++frame % spacing) == 0;
  int sx = 0, sy = 0;
  if (stamp) {
    accX += cosf(ddir) * damt; accY += sinf(ddir) * damt;
    sx = (int)accX; sy = (int)accY;
    accX -= sx; accY -= sy;
  }
  int f = stamp ? fade : 0;          // echoes only age when a new one is laid down
  // Slide by (sx, sy): destination pixel takes the source pixel at (x - sx, y - sy).
  // Walk in the direction that never reads a pixel we've already written.
  int off = sy * W + sx;
  if (off >= 0) {
    for (int y = H - 1; y >= 0; y--) {
      int ys = y - sy;
      uint8_t* row = ec_buf + y * W;
      for (int x = W - 1; x >= 0; x--) {
        int xs = x - sx;
        uint8_t v = ((unsigned)xs < (unsigned)W && (unsigned)ys < (unsigned)H) ? ec_buf[ys * W + xs] : 0;
        if (v >= EC_LIVE) v = stamp ? (uint8_t)(v - (EC_CORE - EC_ECHO_TOP)) : 0;
        else if (v > EC_ECHO_TOP) v = 0;                          // (anything stray, e.g. the info overlay)
        else v = (v > f) ? v - f : 0;
        row[x] = v;
      }
    }
  } else {
    for (int y = 0; y < H; y++) {
      int ys = y - sy;
      uint8_t* row = ec_buf + y * W;
      for (int x = 0; x < W; x++) {
        int xs = x - sx;
        uint8_t v = ((unsigned)xs < (unsigned)W && (unsigned)ys < (unsigned)H) ? ec_buf[ys * W + xs] : 0;
        if (v >= EC_LIVE) v = stamp ? (uint8_t)(v - (EC_CORE - EC_ECHO_TOP)) : 0;
        else if (v > EC_ECHO_TOP) v = 0;
        else v = (v > f) ? v - f : 0;
        row[x] = v;
      }
    }
  }

  // 2) Sparkles drift upward (live pixels, so they leave echo streaks too)
  for (int i = 0; i < sparks; i++) {
    ec_spy[i] -= (int16_t)(ec_spv[i] * speed + 0.5f);
    ec_spx[i] += (int16_t)(2.0f * sinf(t + i));
    if (ec_spy[i] < 0 || ec_spx[i] < 0 || ec_spx[i] >= W * 4) ec_resetSpark(i, false);
    int x = ec_spx[i] >> 2, y = ec_spy[i] >> 2;
    if (((i * 37 + frame) & 15) != 0) ec_put(x, y, EC_SPARK);   // twinkle
  }

  // 3) The live figure
  ec_cx = HALFW; ec_cy = HALFH; ec_S = H * 0.72f * size;
  switch (preset) {
    case 1:  ec_dancer(-0.35f, t, energy, 1.0f, lines); ec_dancer(0.35f, t, energy, -1.0f, lines); break;
    case 2:  ec_jelly(t, energy, lines); break;
    case 3:  ec_ribbon(t, energy, lines); break;
    case 4:  ec_bloom(t, energy, lines); break;
    case 5:  ec_contours(t, energy, lines); break;
    case 6:  ec_orbit(t, energy, lines); break;
    default: ec_dancer(0.0f, t, energy, 1.0f, lines); break;
  }
}
