// =====================================================================
// PROGRAM: MIND MELT (by Dewey)
// Patterns that make THEMSELVES. Nothing here is drawn directly — each
// frame is built from the previous frame by a simple rule, and complex,
// living, trippy structure grows out of it.
//
// Presets:
//   k0  Spiral Demons   — cyclic cellular automaton: random noise organizes
//                         itself into rotating spirals ("demons")
//   k1  Turbulence      — same idea with a wider neighborhood: boiling, twisting waves
//   k2  Rock Paper Scissors — species chase each other in spiral waves
//   k3  Infinite Tunnel — video feedback: each frame is the last one zoomed in,
//                         like pointing a camera at its own monitor
//   k4  Implode         — reverse feedback: everything pours inward forever
//   k5  Life Melt       — Conway's Game of Life, with dying cells melting
//                         through the palette
//   k6  Demon Tunnel    — spiral demons falling into a feedback tunnel
//
//   k12 — press any time to reseed (start fresh from new random noise)
//   p8  — RATE for every preset: how many steps per second the rules run
//         (left = 1/sec, center ≈ 6/sec, right = 40/sec). p1 Speed scales it too.
//
// Uses the platform globals: p0 palette, p3 color cycling (turn it up!).
// Works on a 160×120 grid; each grid cell is a 2×2 block of pixels.
//
// How the "rules" work: every cell looks at its neighbors and decides its
// next color. The rules are tiny, but millions of cells following them
// produce spirals, waves and tunnels nobody drew.
// =====================================================================

#define MM_GW (W / 2)   // 160 grid columns
#define MM_GH (H / 2)   // 120 grid rows

// ─── Grid helpers: the screen itself stores the state ─────────────────
static inline uint8_t mm_get(const uint8_t* buf, int x, int y) {
  return buf[(y * 2) * W + x * 2];
}

static inline void mm_put(uint8_t* buf, int x, int y, uint8_t c) {
  uint16_t pair = c | (c << 8);
  *((uint16_t*)(buf + (y * 2) * W) + x) = pair;
  *((uint16_t*)(buf + (y * 2 + 1) * W) + x) = pair;
}

// Tiny fast random number generator (xorshift) — random() is too slow
// to call 19,200 times a frame.
static uint32_t mm_rng = 2463534242u;
static inline uint32_t mm_rand() {
  mm_rng ^= mm_rng << 13;
  mm_rng ^= mm_rng >> 17;
  mm_rng ^= mm_rng << 5;
  return mm_rng;
}

// Three rows of "old" values, so we can update the screen in place while
// still reading each cell's neighbors from before this step.
static uint8_t mm_rowA[MM_GW], mm_rowB[MM_GW], mm_rowC[MM_GW], mm_row0[MM_GW];

// ─── Cyclic automaton (used by k0, k1, k2, k6) ────────────────────────
// Each cell is in one of N states, shown as N evenly spaced colors.
// Rule: if enough neighbors are in the NEXT state (s+1), this cell moves
// on to that state too. That "chasing" is what grows spirals.
static void mm_decodeRow(const uint8_t* buf, int y, uint8_t* out, int step, int n) {
  for (int x = 0; x < MM_GW; x++) {
    uint8_t v = mm_get(buf, x, y);
    int s = v ? (v - 1) / step : 0;
    out[x] = (s < n) ? s : n - 1;
  }
}

static void mm_seedStates(int n) {
  uint8_t* buf = display.getBuffer();
  int step = 254 / n;
  for (int y = 0; y < MM_GH; y++)
    for (int x = 0; x < MM_GW; x++)
      mm_put(buf, x, y, 1 + (mm_rand() % n) * step);
}

//   n      = number of states       thr   = neighbors needed to advance
//   moore  = also count diagonals   jitter = random extra threshold (0 = none)
static void mm_cyclicStep(int n, int thr, bool moore, int jitter) {
  uint8_t* buf = display.getBuffer();
  int step = 254 / n;
  uint8_t* prev = mm_rowA;
  uint8_t* cur  = mm_rowB;
  uint8_t* next = mm_rowC;
  mm_decodeRow(buf, MM_GH - 1, prev, step, n);   // row above row 0 (wraps)
  mm_decodeRow(buf, 0, cur, step, n);
  memcpy(mm_row0, cur, MM_GW);                    // keep original row 0 for the last row

  for (int y = 0; y < MM_GH; y++) {
    if (y + 1 < MM_GH) mm_decodeRow(buf, y + 1, next, step, n);
    else memcpy(next, mm_row0, MM_GW);

    for (int x = 0; x < MM_GW; x++) {
      int s = cur[x];
      int t = (s + 1 < n) ? s + 1 : 0;           // the state that "eats" this one
      int xl = x ? x - 1 : MM_GW - 1;
      int xr = (x < MM_GW - 1) ? x + 1 : 0;
      int cnt = (prev[x] == t) + (next[x] == t) + (cur[xl] == t) + (cur[xr] == t);
      if (moore) cnt += (prev[xl] == t) + (prev[xr] == t) + (next[xl] == t) + (next[xr] == t);
      int th = thr;
      if (jitter) th += mm_rand() % (jitter + 1);
      if (cnt >= th) s = t;
      mm_put(buf, x, y, 1 + s * step);
    }
    // Slide the three-row window down
    uint8_t* tmp = prev; prev = cur; cur = next; next = tmp;
  }
}

// Sprinkle random states (keeps things from settling down)
static void mm_noise(int count, int n) {
  uint8_t* buf = display.getBuffer();
  int step = 254 / n;
  for (int i = 0; i < count; i++)
    mm_put(buf, mm_rand() % MM_GW, mm_rand() % MM_GH, 1 + (mm_rand() % n) * step);
}

// ─── Feedback zoom (used by k3, k4, k6) ───────────────────────────────
// Each cell copies the color from a spot closer to (or farther from) the
// center, slightly rotated — so last frame's picture gets bigger (or
// smaller) and twists every frame. That's video feedback.
//   z        = zoom per step (1.0 = none)      twistDeg = rotation per step
//   shift    = add this to every color (makes rings of color travel)
//   implode  = shrink instead of grow
//
// We update the screen in place, so the ORDER matters: when zooming in,
// cells take their color from nearer the center, so we work from the
// edges inward (the sources haven't been changed yet). Imploding works
// the other way round.
static void mm_feedback(float z, float twistDeg, int shift, bool implode) {
  uint8_t* buf = display.getBuffer();
  float k = implode ? z : 1.0f / z;
  float a = twistDeg * (PI / 180.0f);
  int32_t ca = (int32_t)(cosf(a) * k * 65536.0f);   // fixed point 16.16
  int32_t sa = (int32_t)(sinf(a) * k * 65536.0f);
  const int32_t cx2 = (int32_t)(MM_GW - 1) << 16;   // center, in doubled units
  const int32_t cy2 = (int32_t)(MM_GH - 1) << 16;

  for (int i = 0; i < MM_GH; i++) {
    int j = implode ? MM_GH - 1 - i : i;
    int y = (j & 1) ? (MM_GH - 1 - j / 2) : (j / 2);    // 0, last, 1, last-1, … (outside in)
    int Y = 2 * y - (MM_GH - 1);                         // doubled distance from center
    for (int m = 0; m < MM_GW; m++) {
      int nn = implode ? MM_GW - 1 - m : m;
      int x = (nn & 1) ? (MM_GW - 1 - nn / 2) : (nn / 2);
      int X = 2 * x - (MM_GW - 1);
      int32_t tx = cx2 + X * ca + Y * sa;
      int32_t ty = cy2 - X * sa + Y * ca;
      int sx = (tx + 65536) >> 17;                       // back to cells, rounded
      int sy = (ty + 65536) >> 17;
      uint8_t v = 0;
      if (sx >= 0 && sx < MM_GW && sy >= 0 && sy < MM_GH) v = mm_get(buf, sx, sy);
      if (v && shift) v = 1 + ((v - 1 + shift) % 254);
      mm_put(buf, x, y, v);
    }
  }
}

// ─── Seeds: something new to feed into the feedback loop ──────────────
// Drawn thick, since the feedback reads every other pixel.
static void mm_thickPoly(int cx, int cy, int r, int sides, float rot, uint8_t c) {
  drawPoly(cx, cy, r, sides, rot, c);
  drawPoly(cx + 1, cy, r, sides, rot, c);
  drawPoly(cx, cy + 1, r, sides, rot, c);
  drawPoly(cx + 1, cy + 1, r, sides, rot, c);
}

static void mm_seedCenter(int shape, float t, uint8_t c) {
  switch (shape) {
    case 0: {                       // ring of orbiting dots
      for (int i = 0; i < 6; i++) {
        float a = t * 2.0f + i * (TWO_PI / 6);
        display.fillCircle(HALFW + (int)(cosf(a) * 14), HALFH + (int)(sinf(a) * 14), 3,
                           1 + ((c - 1 + i * 20) % 254));
      }
      break;
    }
    case 1:                         // spinning triangle
      mm_thickPoly(HALFW, HALFH, 18, 3, t * 1.5f, c);
      break;
    case 2: {                       // star
      float rot = t * -1.2f;
      for (int d = 0; d < 2; d++) drawStarShape(HALFW + d, HALFH, 22, 9, 5, rot, c);
      for (int d = 0; d < 2; d++) drawStarShape(HALFW, HALFH + d, 22, 9, 5, rot, c);
      break;
    }
    default:                        // random sparks near the middle
      for (int i = 0; i < 12; i++)
        display.fillRect(HALFW - 20 + (int)(mm_rand() % 40), HALFH - 20 + (int)(mm_rand() % 40),
                         2, 2, 1 + (mm_rand() % 254));
      break;
  }
}

static void mm_seedEdges(int shape, float t, uint8_t c) {
  switch (shape) {
    case 0:                         // glowing frame around the screen
      display.drawRect(0, 0, W, H, c);
      display.drawRect(1, 1, W - 2, H - 2, c);
      break;
    case 1: {                       // corner dots
      int r = 6;
      display.fillCircle(r, r, r, c);
      display.fillCircle(W - 1 - r, r, r, 1 + ((c + 60) % 254));
      display.fillCircle(r, H - 1 - r, r, 1 + ((c + 120) % 254));
      display.fillCircle(W - 1 - r, H - 1 - r, r, 1 + ((c + 180) % 254));
      break;
    }
    case 2:                         // noisy edges
      for (int i = 0; i < 40; i++) {
        int e = mm_rand() % 4, p = mm_rand() % (e < 2 ? W : H);
        int x = (e == 0 || e == 1) ? p : (e == 2 ? 0 : W - 2);
        int y = (e == 2 || e == 3) ? p : (e == 0 ? 0 : H - 2);
        display.fillRect(x, y, 2, 2, 1 + (mm_rand() % 254));
      }
      break;
    default: {                      // a bar sweeping across the whole screen
      float a = t * 0.8f;
      int dx = (int)(cosf(a) * 200), dy = (int)(sinf(a) * 200);
      for (int d = 0; d < 2; d++) display.drawLine(HALFW - dx + d, HALFH - dy, HALFW + dx + d, HALFH + dy, c);
      break;
    }
  }
}

// ─── Game of Life with melting trails (k5) ────────────────────────────
// Live cells are color 254. When a cell dies it doesn't vanish — its
// color keeps stepping down the palette, so every death leaves a trail.
#define MM_ALIVE 254

static void mm_seedLife(int pct) {
  uint8_t* buf = display.getBuffer();
  for (int y = 0; y < MM_GH; y++)
    for (int x = 0; x < MM_GW; x++)
      mm_put(buf, x, y, ((int)(mm_rand() % 100) < pct) ? MM_ALIVE : 0);
}

static void mm_readRaw(const uint8_t* buf, int y, uint8_t* out) {
  for (int x = 0; x < MM_GW; x++) out[x] = mm_get(buf, x, y);
}

// Returns how many cells are alive afterwards
static int mm_lifeStep(int fade) {
  uint8_t* buf = display.getBuffer();
  uint8_t* prev = mm_rowA;
  uint8_t* cur  = mm_rowB;
  uint8_t* next = mm_rowC;
  mm_readRaw(buf, MM_GH - 1, prev);
  mm_readRaw(buf, 0, cur);
  memcpy(mm_row0, cur, MM_GW);
  int population = 0;

  for (int y = 0; y < MM_GH; y++) {
    if (y + 1 < MM_GH) mm_readRaw(buf, y + 1, next);
    else memcpy(next, mm_row0, MM_GW);

    for (int x = 0; x < MM_GW; x++) {
      int xl = x ? x - 1 : MM_GW - 1;
      int xr = (x < MM_GW - 1) ? x + 1 : 0;
      int n = (prev[xl] == MM_ALIVE) + (prev[x] == MM_ALIVE) + (prev[xr] == MM_ALIVE)
            + (cur[xl]  == MM_ALIVE)                         + (cur[xr]  == MM_ALIVE)
            + (next[xl] == MM_ALIVE) + (next[x] == MM_ALIVE) + (next[xr] == MM_ALIVE);
      uint8_t v = cur[x];
      bool alive = (v == MM_ALIVE);
      if (alive ? (n == 2 || n == 3) : (n == 3)) {
        v = MM_ALIVE;                              // born or survives
        population++;
      } else if (alive) {
        v = 240;                                   // just died: start melting
      } else {
        v = (v > fade) ? v - fade : 0;             // keep melting
      }
      mm_put(buf, x, y, v);
    }
    uint8_t* tmp = prev; prev = cur; cur = next; next = tmp;
  }
  return population;
}

// ─── Step rate (p8, all presets) ──────────────────────────────────────
// The rules run a set number of times per SECOND (not once per frame),
// so things evolve at a watchable pace. p8 sets the rate: all the way
// left = 1 step a second, center ≈ 6, right = 40. The global Speed
// knob (p1) speeds everything up or down on top of that.
static float mm_dt = 0.016f;      // seconds since last frame (set in draw)

static int mm_stepsDue() {
  static float acc = 0;
  float rate = powf(40.0f, pots[8] / 1023.0f);   // 1 … 40 steps per second
  acc += rate * mm_dt;
  int n = (int)acc;
  acc -= n;
  if (n > 3) { n = 3; acc = 0; }                 // never more than 3 per frame
  return n;
}

// ─── Presets ──────────────────────────────────────────────────────────
static bool mm_needSeed = true;

static void mm_spiralDemons() {
  int n     = potMap(4, 3, 20);       // p4: number of states (colors)
  int thr   = potMap(5, 1, 3);        // p5: threshold (1 = fast spirals)
  int noise = potMap(7, 0, 200);      // p7: random noise per step
  static int lastN = -1;
  if (mm_needSeed || n != lastN) { mm_seedStates(n); lastN = n; mm_needSeed = false; }
  for (int i = mm_stepsDue(); i > 0; i--) {
    mm_cyclicStep(n, thr, false, 0);
    mm_noise(noise, n);
  }
}

static void mm_turbulence() {
  int n     = potMap(4, 3, 12);       // p4: number of states
  int thr   = potMap(5, 1, 6);        // p5: threshold (try 3)
  int noise = potMap(7, 0, 200);      // p7: random noise per step
  static int lastN = -1;
  if (mm_needSeed || n != lastN) { mm_seedStates(n); lastN = n; mm_needSeed = false; }
  for (int i = mm_stepsDue(); i > 0; i--) {
    mm_cyclicStep(n, thr, true, 0);
    mm_noise(noise, n);
  }
}

static void mm_rockPaperScissors() {
  int n      = potMap(4, 3, 8);       // p4: number of species
  int thr    = potMap(5, 1, 5);       // p5: threshold
  int jitter = potMap(6, 0, 3);       // p6: randomness in the rule
  int noise  = potMap(7, 0, 200);     // p7: random noise per step
  static int lastN = -1;
  if (mm_needSeed || n != lastN) { mm_seedStates(n); lastN = n; mm_needSeed = false; }
  for (int i = mm_stepsDue(); i > 0; i--) {
    mm_cyclicStep(n, thr, true, jitter);
    mm_noise(noise, n);
  }
}

static void mm_infiniteTunnel() {
  float z   = 1.0f + potMap(4, 10, 100) / 1000.0f; // p4: zoom per step (1.01–1.10)
  float tw  = potMap(5, -80, 80) / 10.0f;         // p5: twist (degrees per step)
  int shape = potMap(6, 0, 3);                    // p6: seed shape
  int shift = potMap(7, 0, 8);                    // p7: color shift per step
  if (mm_needSeed) { display.fillScreen(0); mm_needSeed = false; }
  for (int i = mm_stepsDue(); i > 0; i--) mm_feedback(z, tw, shift, false);
  mm_seedCenter(shape, globalTime, 1 + ((int)(globalTime * 40) % 254));
}

static void mm_implode() {
  float z   = 1.0f + potMap(4, 10, 100) / 1000.0f; // p4: shrink per step (1.01–1.10)
  float tw  = potMap(5, -80, 80) / 10.0f;         // p5: twist (degrees per step)
  int shape = potMap(6, 0, 3);                    // p6: edge seed
  int shift = potMap(7, 0, 8);                    // p7: color shift per step
  if (mm_needSeed) { display.fillScreen(0); mm_needSeed = false; }
  for (int i = mm_stepsDue(); i > 0; i--) mm_feedback(z, tw, shift, true);
  mm_seedEdges(shape, globalTime, 1 + ((int)(globalTime * 40) % 254));
}

static void mm_lifeMelt() {
  int fade   = potMap(4, 1, 30);      // p4: how fast dead cells melt away
  int births = potMap(5, 0, 200);     // p5: random births per step (chaos)
  int dens   = potMap(7, 10, 50);     // p7: starting density (%) for reseeds
  if (mm_needSeed) { mm_seedLife(dens); mm_needSeed = false; }
  uint8_t* buf = display.getBuffer();
  for (int i = mm_stepsDue(); i > 0; i--) {
    int pop = mm_lifeStep(fade);
    for (int b = 0; b < births; b++) mm_put(buf, mm_rand() % MM_GW, mm_rand() % MM_GH, MM_ALIVE);
    if (pop < 40) mm_seedLife(dens);              // it died out — start again
  }
}

static void mm_demonTunnel() {
  int n    = potMap(4, 3, 16);        // p4: number of states
  float z  = 1.0f + potMap(5, 0, 60) / 1000.0f;   // p5: zoom per step
  float tw = potMap(6, -40, 40) / 10.0f;          // p6: twist (degrees per step)
  int noise = potMap(7, 0, 200);      // p7: noise per step
  static int lastN = -1;
  if (mm_needSeed || n != lastN) { mm_seedStates(n); lastN = n; mm_needSeed = false; }
  for (int i = mm_stepsDue(); i > 0; i--) {
    mm_cyclicStep(n, 1, false, 0);
    mm_feedback(z, tw, 0, false);
    mm_noise(noise, n);
  }
}

// ─── Public interface ─────────────────────────────────────────────────

const char* prog_melt_name() { return "MIND MELT"; }

const char* prog_melt_character() {
  return "Self-organizing spirals, feedback tunnels, melting life";
}

static const char* const mm_presetNames[] = {
  "Spiral Demons", "Turbulence", "Rock Paper Sciss", "Infinite Tunnel",
  "Implode", "Life Melt", "Demon Tunnel"
};
#define MM_NUM_PRESETS 7

const char* prog_melt_presetName(int preset) {
  if (preset >= 0 && preset < MM_NUM_PRESETS) return mm_presetNames[preset];
  return NULL;
}

static const char* const mm_potLabels[MM_NUM_PRESETS][4] = {
  {"States",  "Threshold", "",       "Noise"},   // Spiral Demons
  {"States",  "Threshold", "",       "Noise"},   // Turbulence
  {"Species", "Threshold", "Jitter", "Noise"},   // Rock Paper Scissors
  {"Zoom",    "Twist",     "Seed",   "Color Shft"}, // Infinite Tunnel
  {"Shrink",  "Twist",     "Seed",   "Color Shft"}, // Implode
  {"Melt",    "Births",    "",       "Density"}, // Life Melt
  {"States",  "Zoom",      "Twist",  "Noise"},   // Demon Tunnel
};

const char* prog_melt_potLabel(int preset, int pot) {
  if (pot == 8) return "Rate";
  if (pot < 4 || pot > 7) return "";
  if (preset < 0 || preset >= MM_NUM_PRESETS) return "";
  return mm_potLabels[preset][pot - 4];
}

uint8_t prog_melt_renderHint(int preset) {
  (void)preset;
  return RENDER_PERPIXEL;    // the last frame IS the state — never clear it
}

void prog_melt_init() {
  mm_needSeed = true;
  mm_rng ^= millis() * 2654435761u;   // different noise each time
  if (!mm_rng) mm_rng = 1;
}

void prog_melt_draw(int preset) {
  // Time since last frame (follows the global Speed knob, p1)
  static float lastTime = 0;
  float dt = globalTime - lastTime;
  lastTime = globalTime;
  mm_dt = (dt > 0 && dt < 0.1f) ? dt : 0.016f;

  // New preset, or k12 pressed? Start fresh.
  static int lastPreset = -1;
  static bool k12Was = false;
  bool k12 = keysPressed[KEY_MOD_A];
  if (preset != lastPreset || (k12 && !k12Was)) {
    mm_needSeed = true;
    lastPreset = preset;
  }
  k12Was = k12;

  switch (preset) {
    case 0: mm_spiralDemons();      break;
    case 1: mm_turbulence();        break;
    case 2: mm_rockPaperScissors(); break;
    case 3: mm_infiniteTunnel();    break;
    case 4: mm_implode();           break;
    case 5: mm_lifeMelt();          break;
    case 6: mm_demonTunnel();       break;
    default:
      display.fillScreen(0);
      display.setTextColor(255);
      display.setTextSize(1);
      display.setCursor(100, 116);
      display.print("empty preset");
      break;
  }
}
