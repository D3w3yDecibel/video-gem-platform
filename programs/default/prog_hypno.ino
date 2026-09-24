// =====================================================================
// PROGRAM: HYPNOTIC (by Dewey)
// Op-art illusions, a morphing fractal, and pendulum drawings.
//
// Presets:
//   k0  Julia Dream     — a Julia-set fractal whose shape endlessly morphs
//   k1  Op Waves        — bending black & white stripes (1960s op-art style)
//   k2  Hypno Checker   — a checkerboard tunnel that twists and rushes at you
//   k3  Hypno Squares   — nested spinning squares, each turned a little more
//   k4  Harmonograph    — the swirling curves of a swinging-pendulum drawing machine
//   k5  Spirograph      — gear-drawing loops that slowly evolve
//   k6  Hypno Spiral    — the classic black & white hypnosis spiral
//
// Color (p7 on the op-art presets k1, k2, k3, k6):
//   all the way left = pure black & white. Turn up to pick a palette color
//   instead — then p0 (palette) and p3 (color cycling) change it too.
//
// Global knobs: p0 palette, p1 speed, p2 how long drawings last (k4, k5),
// p3 color cycling.
// =====================================================================

#define HY_GW (W / 2)   // half-resolution grid for the per-pixel presets
#define HY_GH (H / 2)

// ─── Shared helpers ───────────────────────────────────────────────────
static float hy_dt = 0.016f;         // seconds since last frame (set in draw)

// Keep a phase moving at a knob-controlled speed without jumps
static float hy_advance(float& phase, float speed) {
  phase += speed * hy_dt;
  if (phase > 100000.0f) phase -= 100000.0f;
  return phase;
}

static inline void hy_plot2x2(uint8_t* buf, int gx, int gy, uint8_t c) {
  uint16_t pair = c | (c << 8);
  *((uint16_t*)(buf + (gy * 2) * W) + gx) = pair;
  *((uint16_t*)(buf + (gy * 2 + 1) * W) + gx) = pair;
}

// Foreground color from p7: far left = white (255); otherwise a palette color
static uint8_t hy_fg() {
  int k = potMap(7, 0, 255);
  return (k < 12) ? 255 : (uint8_t)k;
}

// Angle and distance lookups (same trick as Liquid Light's Vortex):
// tiny tables replace slow atan2/sqrt.
static uint8_t  hy_atanTab[65];
static uint16_t hy_hypTab[65];
static bool     hy_polarReady = false;

static void hy_buildPolar() {
  if (hy_polarReady) return;
  for (int i = 0; i <= 64; i++) {
    float r = i / 64.0f;
    hy_atanTab[i] = (uint8_t)(atanf(r) / TWO_PI * 256.0f + 0.5f);
    hy_hypTab[i]  = (uint16_t)(sqrtf(1.0f + r * r) * 256.0f + 0.5f);
  }
  hy_polarReady = true;
}

// ang: 0–255 = once around. dist: distance in grid cells.
static inline void hy_polar(int dx, int dy, int& ang, int& dist) {
  int adx = abs(dx), ady = abs(dy);
  int mx = (adx > ady) ? adx : ady;
  int mn = (adx > ady) ? ady : adx;
  if (mx == 0) { ang = 0; dist = 0; return; }
  int r = (mn * 64) / mx;
  int a = hy_atanTab[r];
  if (ady > adx) a = 64 - a;
  if (dx < 0) a = 128 - a;
  if (dy < 0) a = 256 - a;
  ang = a & 255;
  dist = (mx * hy_hypTab[r]) >> 8;
}

// ─── Preset 0: Julia Dream ────────────────────────────────────────────
// For every point z we repeat z = z² + c and count how many steps it
// takes to fly off to infinity. That count becomes the color. Moving c
// slowly around a circle makes the whole fractal morph and breathe.
// Math is done in whole numbers (fixed point, 12 fractional bits) because
// the chip has no floating-point hardware. Half the rows are redrawn each
// frame (alternating) to keep it smooth.
static void hy_julia() {
  float morph = potMap(4, 0, 100) / 400.0f;   // p4: morph speed
  int maxIt   = potMap(5, 8, 40);             // p5: detail (iterations)
  float zoom  = powf(4.0f, pots[6] / 1023.0f);// p6: zoom 1x–4x
  int bands   = potMap(7, 1, 6);              // p7: color bands

  static float a = 0;
  static int field = 0;
  hy_advance(a, morph);
  field ^= 1;

  const int32_t ONE = 4096;                   // 1.0 in 12-bit fixed point
  int32_t cx = (int32_t)(0.7885f * cosf(a) * ONE);
  int32_t cy = (int32_t)(0.7885f * sinf(a) * ONE);
  int32_t step = (int32_t)(3.2f / zoom / HY_GW * ONE);   // size of one grid cell
  int32_t x0 = -step * (HY_GW / 2);
  int32_t y0 = -step * (HY_GH / 2);

  // Color for each iteration count (computed once, not per pixel)
  static uint8_t itCol[41];
  for (int i = 0; i <= maxIt; i++) itCol[i] = 1 + ((i * bands * 254) / maxIt) % 254;

  uint8_t* buf = display.getBuffer();
  const int32_t LIMIT = 4 * ONE;
  for (int gy = field; gy < HY_GH; gy += 2) {
    int32_t zy0 = y0 + gy * step;
    for (int gx = 0; gx < HY_GW; gx++) {
      int32_t zx = x0 + gx * step, zy = zy0;
      int it = 0;
      while (it < maxIt) {
        int32_t x2 = (zx * zx) >> 12;
        int32_t y2 = (zy * zy) >> 12;
        if (x2 + y2 > LIMIT) break;
        zy = ((zx * zy) >> 11) + cy;          // 2·x·y + cy
        zx = x2 - y2 + cx;
        it++;
      }
      hy_plot2x2(buf, gx, gy, (it >= maxIt) ? 0 : itCol[it]);
    }
  }
}

// ─── Preset 1: Op Waves ───────────────────────────────────────────────
// Vertical black and white stripes, each pushed sideways by a wave that
// changes as you go down the screen. Your eyes do the rest.
static void hy_opWaves() {
  int width = potMap(4, 3, 24);       // p4: stripe width (px)
  int amp   = potMap(5, 0, 60);       // p5: wave strength (px)
  int freq  = potMap(6, 1, 8);        // p6: waves down the screen
  uint8_t fg = hy_fg();               // p7: color

  static float ph = 0;
  int t = (int)hy_advance(ph, 60.0f);
  uint8_t* buf = display.getBuffer();
  int invW = 65536 / width;           // divide-free stripe math
  for (int y = 0; y < H; y++) {
    // Each row has its own sideways shift, plus a slow second wave
    int rowShift = ((sinTab[(y * freq + t) & 255] - 128) * amp) >> 7;
    uint8_t* out = buf + y * W;
    for (int x = 0; x < W; x++) {
      int bend = ((sinTab[(x + y + (t >> 1)) & 255] - 128) * amp) >> 9;
      int xs = x + rowShift + bend + 1024;
      out[x] = (((xs * invW) >> 16) & 1) ? fg : 0;
    }
  }
}

// ─── Preset 2: Hypno Checker ──────────────────────────────────────────
// A checkerboard wrapped into a tunnel: squares are slices of angle
// around the center and rings of distance. Using 1/distance for the rings
// creates perspective, so it looks like you're flying down a tube.
static void hy_hypnoChecker() {
  int sectors = potMap(4, 2, 16) * 2; // p4: number of slices around
  int twist   = potMap(5, -40, 40);   // p5: twist
  int speed   = potMap(6, 0, 120);    // p6: flying speed
  uint8_t fg  = hy_fg();              // p7: color
  hy_buildPolar();

  static float ph = 0, rot = 0;
  int t = (int)hy_advance(ph, speed);
  int r8 = (int)hy_advance(rot, 12.0f);
  uint8_t* buf = display.getBuffer();
  for (int gy = 0; gy < HY_GH; gy++) {
    int dy = gy - HY_GH / 2;
    for (int gx = 0; gx < HY_GW; gx++) {
      int dx = gx - HY_GW / 2;
      int ang, dist;
      hy_polar(dx, dy, ang, dist);
      int depth = 2048 / (dist + 2);                     // perspective rings
      int a = ((ang + r8 + ((depth * twist) >> 4)) * sectors) >> 8;
      int ring = (depth + t) >> 2;
      uint8_t c = ((a + ring) & 1) ? fg : 0;
      if (dist < 3) c = 0;                               // black hole in the middle
      hy_plot2x2(buf, gx, gy, c);
    }
  }
}

// ─── Preset 3: Hypno Squares ──────────────────────────────────────────
// Filled squares from huge to tiny, alternating colors, each rotated a
// little more than the last — which makes a twisting spiral. They also
// grow steadily, so it feels like falling inward forever.
static void hy_fillSquare(int cx, int cy, int half, float ang, uint8_t c) {
  float co = cosf(ang) * half, si = sinf(ang) * half;
  int x0 = cx + (int)( co - si), y0 = cy + (int)( si + co);
  int x1 = cx + (int)(-co - si), y1 = cy + (int)(-si + co);
  int x2 = cx + (int)(-co + si), y2 = cy + (int)(-si - co);
  int x3 = cx + (int)( co + si), y3 = cy + (int)( si - co);
  display.fillTriangle(x0, y0, x1, y1, x2, y2, c);
  display.fillTriangle(x0, y0, x2, y2, x3, y3, c);
}

static void hy_hypnoSquares() {
  int layers  = potMap(4, 6, 30);               // p4: number of squares
  float twist = potMap(5, -30, 30) / 100.0f;    // p5: turn per square (radians)
  float speed = potMap(6, 0, 100) / 100.0f;     // p6: falling speed
  uint8_t fg  = hy_fg();                        // p7: color

  static float ph = 0, rot = 0;
  hy_advance(ph, speed);
  hy_advance(rot, 0.2f);
  float grow = 0.82f;                           // each square is 82% of the last
  float f = ph - floorf(ph);                    // 0..1: progress to the next layer
  float size = 260.0f * powf(1.0f / grow, f);
  bool flip = ((int)floorf(ph)) & 1;            // keep colors steady as layers shift
  for (int i = 0; i < layers && size > 1.0f; i++) {
    uint8_t c = ((i + flip) & 1) ? fg : 0;
    hy_fillSquare(HALFW, HALFH, (int)size, rot + (i - f) * twist, c);
    size *= grow;
  }
}

// ─── Preset 4: Harmonograph ───────────────────────────────────────────
// A Victorian drawing machine: two swinging pendulums move a pen in x
// and y. Their slightly different speeds trace looping, braided curves
// that slowly shrink as the swing dies away. Then a new drawing begins.
static float hy_hf[4], hy_hp[4];     // frequencies and phases of the 4 swings
static float hy_ht = 0;              // pendulum time
static bool  hy_hNew = true;

static void hy_harmonograph() {
  float speed = potMap(4, 10, 400) / 10.0f;     // p4: drawing speed
  float decay = potMap(5, 1, 40) / 1000.0f;     // p5: how fast the swing dies
  float detune = potMap(6, 0, 40) / 1000.0f;    // p6: detune (more = more tangled)
  int spread  = potMap(7, 1, 40);               // p7: color change along the line

  if (hy_hNew) {
    // Near-whole-number frequency ratios make the prettiest figures
    static const float ratios[] = {1, 2, 3, 1.5f, 1, 2};
    for (int i = 0; i < 4; i++) {
      hy_hf[i] = ratios[random(6)] + (random(-100, 101) / 100.0f) * detune * 10;
      hy_hp[i] = random(0, 628) / 100.0f;
    }
    hy_ht = 0;
    hy_hNew = false;
  }

  // Draw many short segments per frame so the curve flows smoothly
  const int SEGS = 120;
  float dtSeg = speed * hy_dt / SEGS;
  float px = 0, py = 0;
  for (int s = 0; s <= SEGS; s++) {
    float t = hy_ht;
    float e = expf(-decay * t);
    float x = HALFW + e * (70 * sinf(hy_hf[0] * t + hy_hp[0]) + 70 * sinf(hy_hf[1] * t + hy_hp[1]));
    float y = HALFH + e * (55 * sinf(hy_hf[2] * t + hy_hp[2]) + 55 * sinf(hy_hf[3] * t + hy_hp[3]));
    uint8_t c = 1 + ((int)(t * spread) % 254);
    if (s > 0) {
      display.drawLine((int)px, (int)py, (int)x, (int)y, c);
      display.drawLine((int)px + 1, (int)py, (int)x + 1, (int)y, c);
    }
    px = x; py = y;
    if (s < SEGS) hy_ht += dtSeg;
    if (e < 0.05f) hy_hNew = true;                // swing died out: new drawing
  }
}

// ─── Preset 5: Spirograph ─────────────────────────────────────────────
// A small gear rolling inside a big ring, with the pen a little off the
// gear's center (a "hypotrochoid"). The gear size slowly changes, so the
// figure keeps evolving; the whole thing also turns.
static void hy_spirograph() {
  float ratio = potMap(4, 20, 90) / 100.0f;     // p4: gear size (fraction of ring)
  float pen   = potMap(5, 20, 150) / 100.0f;    // p5: pen offset
  float speed = potMap(6, 20, 400) / 10.0f;     // p6: drawing speed
  int spread  = potMap(7, 1, 40);               // p7: color change along the line

  static float t = 0, morph = 0, rot = 0;
  hy_advance(morph, 0.02f);
  hy_advance(rot, 0.1f);
  float Rb = 100.0f;                            // big ring radius
  float r = Rb * (ratio + 0.03f * sinf(morph)); // gear radius (slowly breathing)
  float d = r * pen;
  float k = (Rb - r) / r;

  const int SEGS = 120;
  float dtSeg = speed * hy_dt / SEGS;
  float cr = cosf(rot), sr = sinf(rot);
  float px = 0, py = 0;
  for (int s = 0; s <= SEGS; s++) {
    float x = (Rb - r) * cosf(t) + d * cosf(k * t);
    float y = (Rb - r) * sinf(t) - d * sinf(k * t);
    float X = HALFW + (x * cr - y * sr) * 1.1f;
    float Y = HALFH + (x * sr + y * cr) * 1.0f;
    uint8_t c = 1 + ((int)(t * spread) % 254);
    if (s > 0) {
      display.drawLine((int)px, (int)py, (int)X, (int)Y, c);
      display.drawLine((int)px, (int)py + 1, (int)X, (int)Y + 1, c);
    }
    px = X; py = Y;
    if (s < SEGS) t += dtSeg;
  }
  if (t > 10000.0f) t -= 10000.0f;
}

// ─── Preset 6: Hypno Spiral ───────────────────────────────────────────
// The classic: black and white spiral arms turning around the center.
// Arms = slices of angle; tightness bends them with distance; the
// spiral also slowly breathes tighter and looser.
static void hy_hypnoSpiral() {
  int arms  = potMap(4, 1, 8);        // p4: number of arms
  int tight = potMap(5, 1, 24);       // p5: how tightly they wind
  int speed = potMap(6, 0, 200);      // p6: spin speed
  uint8_t fg = hy_fg();               // p7: color
  hy_buildPolar();

  static float ph = 0, breath = 0;
  int t = (int)hy_advance(ph, speed);
  hy_advance(breath, 0.5f);
  int tb = tight * 16 + (int)(sinf(breath) * tight * 4);   // breathing tightness (x16)
  uint8_t* buf = display.getBuffer();
  for (int gy = 0; gy < HY_GH; gy++) {
    int dy = gy - HY_GH / 2;
    for (int gx = 0; gx < HY_GW; gx++) {
      int dx = gx - HY_GW / 2;
      int ang, dist;
      hy_polar(dx, dy, ang, dist);
      int v = ang * arms + ((dist * tb) >> 4) - t;
      hy_plot2x2(buf, gx, gy, ((v >> 7) & 1) ? fg : 0);
    }
  }
}

// ─── Public interface ─────────────────────────────────────────────────

const char* prog_hypno_name() { return "HYPNOTIC"; }

const char* prog_hypno_character() {
  return "Op-art illusions, a morphing fractal, pendulum drawings";
}

static const char* const hy_presetNames[] = {
  "Julia Dream", "Op Waves", "Hypno Checker", "Hypno Squares",
  "Harmonograph", "Spirograph", "Hypno Spiral"
};
#define HY_NUM_PRESETS 7

const char* prog_hypno_presetName(int preset) {
  if (preset >= 0 && preset < HY_NUM_PRESETS) return hy_presetNames[preset];
  return NULL;
}

static const char* const hy_potLabels[HY_NUM_PRESETS][4] = {
  {"Morph",   "Detail",  "Zoom",   "Bands"},   // Julia Dream
  {"Width",   "Wave",    "Waves",  "Color"},   // Op Waves
  {"Slices",  "Twist",   "Speed",  "Color"},   // Hypno Checker
  {"Squares", "Twist",   "Speed",  "Color"},   // Hypno Squares
  {"Speed",   "Decay",   "Detune", "Spread"},  // Harmonograph
  {"Gear",    "Pen",     "Speed",  "Spread"},  // Spirograph
  {"Arms",    "Tight",   "Spin",   "Color"},   // Hypno Spiral
};

const char* prog_hypno_potLabel(int preset, int pot) {
  if (pot < 4 || pot > 7) return "";
  if (preset < 0 || preset >= HY_NUM_PRESETS) return "";
  return hy_potLabels[preset][pot - 4];
}

uint8_t prog_hypno_renderHint(int preset) {
  switch (preset) {
    case 3:  return RENDER_CLEAR;      // squares are redrawn from scratch
    default: return RENDER_PERPIXEL;   // we paint every pixel ourselves (k4/k5 fade slowly, below)
  }
}

void prog_hypno_draw(int preset) {
  // Time since last frame (follows the global Speed knob, p1)
  static float lastTime = 0;
  float dt = globalTime - lastTime;
  lastTime = globalTime;
  hy_dt = (dt > 0 && dt < 0.1f) ? dt : 0.016f;

  static int lastPreset = -1;
  if (preset != lastPreset) {
    lastPreset = preset;
    hy_hNew = true;
    display.fillScreen(0);
  }

  // Drawings (k4, k5) fade slowly so the whole figure builds up.
  // p2 sets how long they last: left = several seconds, right = quick.
  if (preset == 4 || preset == 5) {
    static float fadeAcc = 0;
    fadeAcc += potMap(2, 4, 120) * hy_dt;
    int n = (int)fadeAcc;
    if (n > 0) { fadeAcc -= n; fadeScreen(n > 255 ? 255 : n); }
  }

  switch (preset) {
    case 0: hy_julia();         break;
    case 1: hy_opWaves();       break;
    case 2: hy_hypnoChecker();  break;
    case 3: hy_hypnoSquares();  break;
    case 4: hy_harmonograph();  break;
    case 5: hy_spirograph();    break;
    case 6: hy_hypnoSpiral();   break;
    default:
      display.fillScreen(0);
      display.setTextColor(255);
      display.setTextSize(1);
      display.setCursor(100, 116);
      display.print("empty preset");
      break;
  }
}
