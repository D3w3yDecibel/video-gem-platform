// =====================================================================
// PROGRAM: LIQUID LIGHT (by Dewey)
// Oil-and-water style visuals inspired by 1960s liquid light shows.
//
// Presets:
//   k0  Oil Plasma  — warped plasma field that flows like oil on glass
//   k1  Lava Blobs  — drifting blobs whose trails melt through the palette
//   k2  Moire Pool  — two ripple sources interfering like drops in a dish
//   k3  Vortex      — spiral arms swirling into a whirlpool
//   k4  Kaleido Oil — oil plasma mirrored four ways, like an ink blot
//   k5  Ink Drops   — drops land and spread into blooming rings
//   k6  Bubbles     — bubbles rise and wobble, leaving trails
//   k7  Wet Show    — spinning glass dish of oil and water, overhead-projector style
//   k8  Chroma Rain — colored streaks falling with bright heads
//   k9  Silk        — rippling stripes like silk or an aurora curtain
//   k10 Oil Cells   — floating oil bubbles with dark edges (Voronoi)
//   k11 Mandala     — many-mirror kaleidoscope
//
// Knob motion: p8–p11 set LFO rate and p12–p15 set LFO depth for the
// preset knob above them (p4–p7). Depth at zero = no motion.
//
// Global pots (platform): p0 palette, p1 speed, p2 trail, p3 color cycle
// Tip: p3 (color cycle) is what makes it "flow" — turn it up a little.
// =====================================================================

// ─── Shared helpers ───────────────────────────────────────────────────

// Half-resolution grid: we compute one value per 2×2 block of pixels.
// 160×120 = 19,200 calculations per frame instead of 76,800 — fast
// enough for 60 fps, and the soft, blocky look suits liquid light.
#define LIQ_GW (W / 2)
#define LIQ_GH (H / 2)

// Palette index lookup: turns a raw 0..765 value into a color index
// 1..254 (0 = black and 255 = white are reserved by the platform).
// "bands" repeats the palette N times across the range, which gives
// more, thinner color bands — like adding more oil to the dish.
static uint8_t liq_lut[768];
static int liq_lutBands = -1;

static void liq_buildLut(int bands) {
  if (bands == liq_lutBands) return;   // only rebuild when the knob moves
  liq_lutBands = bands;
  for (int v = 0; v < 768; v++) {
    liq_lut[v] = 1 + ((v * bands / 3) % 254);
  }
}

// Write one color into a 2×2 block of the screen buffer.
static inline void liq_plot2x2(uint8_t* buf, int gx, int gy, uint8_t c) {
  uint16_t pair = c | (c << 8);
  uint16_t* row0 = (uint16_t*)(buf + (gy * 2) * W) + gx;
  uint16_t* row1 = (uint16_t*)(buf + (gy * 2 + 1) * W) + gx;
  *row0 = pair;
  *row1 = pair;
}

// ─── Knob motion: LFOs on p8–p15 ──────────────────────────────────────
// Each preset knob p4–p7 has its own LFO (a slow wave that moves the
// knob for you). The two knobs in the column BELOW each preset knob
// control it:
//
//     p4    p5    p6    p7      ← preset knobs (set the center value)
//     p8    p9    p10   p11     ← LFO RATE  for the knob above
//     p12   p13   p14   p15     ← LFO DEPTH for the knob above
//
// Depth all the way down = no motion, so the knob behaves normally.
static int liq_pot(int idx, int lo, int hi) {
  int v = pots[idx];
  if (idx >= 4 && idx <= 7) {
    int j = idx - 4;                        // which LFO (0–3)
    int rate  = pots[8 + j];
    int depth = pots[12 + j];
    // Rate curve: gentle at the bottom for slow drifts (~0.01 Hz up to ~2 Hz)
    g_lfos[j].freq = 2 + (uint32_t)rate * rate / 2048;
    // Offset each LFO a quarter-turn so they don't all move together
    v += (lfoSineBi(j, j * 64) * depth) / 128;
    if (v < 0) v = 0;
    if (v > 1023) v = 1023;
  }
  return map(v, 0, 1023, lo, hi);
}

// ─── Smooth speed changes ─────────────────────────────────────────────
// If we calculated position = time × speed, then changing speed (by hand
// or with an LFO) would make the picture JUMP. Instead each preset keeps
// its own "phase" and adds speed × (time since last frame) every frame,
// so speed changes are always smooth.
static float liq_dt = 0.016f;       // seconds since the last frame (set in draw)

#define LIQ_WRAP 5120.0f            // keeps phases small; a multiple of 256
                                    // that also works for the 0.7/0.45/0.4 ratios

static float liq_advance(float& phase, float speed) {
  phase += speed * liq_dt;
  if (phase >= LIQ_WRAP) phase -= LIQ_WRAP;
  return phase;
}

// ─── Preset 0: Oil Plasma ─────────────────────────────────────────────
// Classic plasma = add up a few sine waves running in different
// directions. The "warp" knob feeds each wave into the other one,
// which bends the straight bands into oily, swirling shapes.
static void liq_oilPlasma() {
  int scale  = liq_pot(4, 1, 8);    // p4: zoom (small = big blobs)
  int warp   = liq_pot(5, 0, 64);   // p5: how much the waves bend each other
  int drift  = liq_pot(6, 5, 80);   // p6: how fast the waves travel
  int bands  = liq_pot(7, 1, 8);    // p7: number of color bands
  liq_buildLut(bands);

  static float ph = 0;
  liq_advance(ph, drift);
  int t  = (int)ph;
  int t2 = (int)(ph * 0.7f);
  int t3 = (int)(ph * 0.45f);

  // Pre-compute one wave per column and one per row (saves a lot of work)
  static uint8_t colWave[LIQ_GW];
  static uint8_t rowWave[LIQ_GH];
  for (int x = 0; x < LIQ_GW; x++) colWave[x] = sinTab[(x * scale + t) & 255];
  for (int y = 0; y < LIQ_GH; y++) rowWave[y] = sinTab[(y * scale - t2) & 255];

  uint8_t* buf = display.getBuffer();
  for (int y = 0; y < LIQ_GH; y++) {
    int rw = (rowWave[y] * warp) >> 6;
    for (int x = 0; x < LIQ_GW; x++) {
      int cw = (colWave[x] * warp) >> 6;
      int v = sinTab[(x * scale + rw + t) & 255]
            + sinTab[(y * scale + cw - t2) & 255]
            + sinTab[((x + y) * scale / 2 + t3) & 255];
      liq_plot2x2(buf, x, y, liq_lut[v]);
    }
  }
}

// ─── Preset 1: Lava Blobs ─────────────────────────────────────────────
// Filled circles wander on slow looping paths. This preset uses trails
// (p2): each frame the old image fades by stepping DOWN the palette,
// so the trails don't just darken — they melt through the colors.
static void liq_lavaBlobs() {
  int count  = liq_pot(4, 1, 8);     // p4: number of blobs
  int size   = liq_pot(5, 8, 60);    // p5: blob size
  int wander = liq_pot(6, 10, 100);  // p6: how far they roam (%)
  int spread = liq_pot(7, 0, 60);    // p7: color difference between blobs

  float xr = (HALFW - 10) * wander / 100.0f;
  float yr = (HALFH - 10) * wander / 100.0f;

  for (int i = 0; i < count; i++) {
    // Each blob gets its own slightly different speeds, so the paths
    // never quite repeat (these are called Lissajous curves).
    float fx = 0.21f + i * 0.067f;
    float fy = 0.17f + i * 0.053f;
    float ph = i * 1.7f;
    int cx = HALFW + (int)(sinf(globalTime * fx + ph) * xr);
    int cy = HALFH + (int)(cosf(globalTime * fy + ph * 0.6f) * yr);
    int r  = size + (int)(sinf(globalTime * 0.9f + ph) * size * 0.3f);
    int c  = 1 + ((200 + i * spread) % 254);
    display.fillCircle(cx, cy, r, c);
  }
}

// ─── Preset 2: Moire Pool ─────────────────────────────────────────────
// Two ripple sources, like two drops of oil in a dish. Where the rings
// overlap they interfere and make moire patterns. We use distance
// squared (no square root), so rings bunch up toward the edges — a
// "zone plate" look that reads nicely when projected.
static void liq_moirePool() {
  int density = liq_pot(4, 2, 32);   // p4: ring density (bigger = more rings)
  int apart   = liq_pot(5, 0, 120);  // p5: distance between the two sources
  int speed   = liq_pot(6, 5, 120);  // p6: ripple speed
  int bands   = liq_pot(7, 1, 6);    // p7: number of color bands
  liq_buildLut(bands);

  static float ph = 0;
  int t = (int)liq_advance(ph, speed);

  // The two sources orbit the center slowly
  float a = globalTime * 0.3f;
  int ox = (int)(cosf(a) * apart / 2) / 2;   // /2 again: half-res grid
  int oy = (int)(sinf(a) * apart / 3) / 2;
  int ax = LIQ_GW / 2 + ox, ay = LIQ_GH / 2 + oy;
  int bx = LIQ_GW / 2 - ox, by = LIQ_GH / 2 - oy;

  // Pre-compute squared distances per column/row for each source
  static int dxa[LIQ_GW], dxb[LIQ_GW], dya[LIQ_GH], dyb[LIQ_GH];
  for (int x = 0; x < LIQ_GW; x++) { dxa[x] = (x - ax) * (x - ax); dxb[x] = (x - bx) * (x - bx); }
  for (int y = 0; y < LIQ_GH; y++) { dya[y] = (y - ay) * (y - ay); dyb[y] = (y - by) * (y - by); }

  uint8_t* buf = display.getBuffer();
  for (int y = 0; y < LIQ_GH; y++) {
    for (int x = 0; x < LIQ_GW; x++) {
      int da = ((dxa[x] + dya[y]) * density) >> 6;
      int db = ((dxb[x] + dyb[y]) * density) >> 6;
      int v = sinTab[(da - t) & 255] + sinTab[(db - t) & 255];
      liq_plot2x2(buf, x, y, liq_lut[v]);
    }
  }
}

// ─── Preset 3: Vortex ─────────────────────────────────────────────────
// Color spirals into a whirlpool. For every point we work out its ANGLE
// around the center and its DISTANCE from the center. Mixing the two is
// what makes a spiral: "arms" repeats the angle, "twist" bends the arms
// the further out you go.
//
// Real angle/distance math (atan2, sqrt) is too slow for 19,200 points
// per frame, so we use two tiny lookup tables built once at startup.
static uint8_t  liq_atanTab[65];   // angle for slope 0..1, in 1/256ths of a turn
static uint16_t liq_hypTab[65];    // distance multiplier for slope 0..1 (x256)
static bool     liq_polarReady = false;

static void liq_buildPolar() {
  if (liq_polarReady) return;
  for (int i = 0; i <= 64; i++) {
    float r = i / 64.0f;
    liq_atanTab[i] = (uint8_t)(atanf(r) / TWO_PI * 256.0f + 0.5f);
    liq_hypTab[i]  = (uint16_t)(sqrtf(1.0f + r * r) * 256.0f + 0.5f);
  }
  liq_polarReady = true;
}

// Angle (0–255 = once around) and distance of the point (dx, dy) from
// the center, using the lookup tables above. Shared by several presets.
static inline void liq_polar(int dx, int dy, int& ang, int& dist) {
  int adx = abs(dx), ady = abs(dy);
  int mx = (adx > ady) ? adx : ady;
  int mn = (adx > ady) ? ady : adx;
  if (mx == 0) { ang = 0; dist = 0; return; }
  int r = (mn * 64) / mx;                  // slope, 0..64
  int a = liq_atanTab[r];                  // 0..32 (one eighth of a turn)
  if (ady > adx) a = 64 - a;               // steep half of the quadrant
  if (dx < 0) a = 128 - a;                 // left side
  if (dy < 0) a = 256 - a;                 // top half
  ang = a & 255;
  dist = (mx * liq_hypTab[r]) >> 8;        // true distance from center
}

static void liq_vortex() {
  int arms  = liq_pot(4, 1, 6);     // p4: number of spiral arms
  int twist = liq_pot(5, 0, 24);    // p5: how tightly the arms wind
  int speed = liq_pot(6, 5, 100);   // p6: spin speed
  int bands = liq_pot(7, 1, 6);     // p7: number of color bands
  liq_buildLut(bands);
  liq_buildPolar();

  static float ph = 0;
  liq_advance(ph, speed);
  int t  = (int)ph;
  int t2 = (int)(ph * 0.4f);

  uint8_t* buf = display.getBuffer();
  for (int y = 0; y < LIQ_GH; y++) {
    int dy = y - LIQ_GH / 2;
    for (int x = 0; x < LIQ_GW; x++) {
      int dx = x - LIQ_GW / 2;
      int ang, dist;
      liq_polar(dx, dy, ang, dist);
      int v = sinTab[(ang * arms + ((dist * twist) >> 2) - t) & 255]
            + sinTab[(dist * 3 - t2) & 255];
      liq_plot2x2(buf, x, y, liq_lut[v]);
    }
  }
}

// ─── Preset 4: Kaleido Oil ────────────────────────────────────────────
// Same recipe as Oil Plasma, but we only calculate the top-left quarter
// and mirror it into the other three — like an ink-blot or a two-mirror
// kaleidoscope. Bonus: it's 4x less work for the chip.
static void liq_kaleidoOil() {
  int scale = liq_pot(4, 2, 12);    // p4: zoom
  int warp  = liq_pot(5, 0, 64);    // p5: warp
  int drift = liq_pot(6, 5, 80);    // p6: drift speed
  int bands = liq_pot(7, 1, 8);     // p7: color bands
  liq_buildLut(bands);

  static float ph = 0;
  liq_advance(ph, drift);
  int t  = (int)ph;
  int t2 = (int)(ph * 0.7f);
  int t3 = (int)(ph * 0.45f);

  const int QW = LIQ_GW / 2, QH = LIQ_GH / 2;
  static uint8_t colWave[LIQ_GW / 2];
  static uint8_t rowWave[LIQ_GH / 2];
  for (int x = 0; x < QW; x++) colWave[x] = sinTab[(x * scale + t) & 255];
  for (int y = 0; y < QH; y++) rowWave[y] = sinTab[(y * scale - t2) & 255];

  uint8_t* buf = display.getBuffer();
  for (int y = 0; y < QH; y++) {
    int rw = (rowWave[y] * warp) >> 6;
    for (int x = 0; x < QW; x++) {
      int cw = (colWave[x] * warp) >> 6;
      int v = sinTab[(x * scale + rw + t) & 255]
            + sinTab[(y * scale + cw - t2) & 255]
            + sinTab[((x + y) * scale / 2 + t3) & 255];
      uint8_t c = liq_lut[v];
      liq_plot2x2(buf, x,              y,              c);  // top-left
      liq_plot2x2(buf, LIQ_GW - 1 - x, y,              c);  // top-right
      liq_plot2x2(buf, x,              LIQ_GH - 1 - y, c);  // bottom-left
      liq_plot2x2(buf, LIQ_GW - 1 - x, LIQ_GH - 1 - y, c);  // bottom-right
    }
  }
}

// ─── Preset 5: Ink Drops ──────────────────────────────────────────────
// Drops land at random spots and spread out as rings. Uses trails, so
// the rings smear into soft blooms. This preset REMEMBERS things between
// frames (where each drop landed and when), stored in the array below.
#define LIQ_MAX_DROPS 10

struct LiqDrop {
  int16_t x, y;
  float born;       // globalTime when the drop landed
  uint8_t color;
  bool alive;
};

static LiqDrop liq_drops[LIQ_MAX_DROPS];
static float liq_nextDrop = 0;
static uint8_t liq_dropColor = 1;

static void liq_inkDrops() {
  int rate   = liq_pot(4, 1, 30);   // p4: how often drops fall
  int grow   = liq_pot(5, 10, 120); // p5: how fast rings spread (px/sec)
  int rings  = liq_pot(6, 1, 6);    // p6: rings per drop
  int spread = liq_pot(7, 1, 60);   // p7: color change between drops

  float now = globalTime;
  // The speed knob (p1) rescales time, so time can jump. Recover if it does.
  if (liq_nextDrop > now + 10.0f) liq_nextDrop = now;

  // Time for a new drop? Reuse a free slot, or the oldest one.
  if (now >= liq_nextDrop) {
    int slot = 0;
    for (int i = 0; i < LIQ_MAX_DROPS; i++) {
      if (!liq_drops[i].alive) { slot = i; break; }
      if (liq_drops[i].born < liq_drops[slot].born) slot = i;
    }
    liq_drops[slot].x = random(20, W - 20);
    liq_drops[slot].y = random(20, H - 20);
    liq_drops[slot].born = now;
    liq_drops[slot].color = liq_dropColor;
    liq_drops[slot].alive = true;
    liq_dropColor = 1 + ((liq_dropColor - 1 + spread) % 254);
    liq_nextDrop = now + 10.0f / rate;
  }

  // Draw every live drop as a set of growing rings
  for (int i = 0; i < LIQ_MAX_DROPS; i++) {
    LiqDrop& d = liq_drops[i];
    if (!d.alive) continue;
    float age = now - d.born;
    int r = (int)(age * grow);
    if (age < 0 || r > 300) { d.alive = false; continue; }
    for (int k = 0; k < rings; k++) {
      int rr = r - k * 10;
      if (rr <= 0) break;
      uint8_t c = 1 + ((d.color - 1 + k * 8) % 254);
      display.drawCircle(d.x, d.y, rr, c);
      display.drawCircle(d.x, d.y, rr + 1, c);   // 2 px thick
    }
  }
}

// ─── Preset 6: Bubbles ────────────────────────────────────────────────
// Bubbles rise and wobble side to side, with trails left behind them.
// No memory needed: each bubble's lane, size and speed are worked out
// from its number (i) every frame, so they're the same every time.
static void liq_bubbles() {
  int count  = liq_pot(4, 3, 24);   // p4: number of bubbles
  int size   = liq_pot(5, 4, 30);   // p5: bubble size
  int rise   = liq_pot(6, 10, 120); // p6: rise speed (px/sec)
  int wobble = liq_pot(7, 0, 30);   // p7: side-to-side wobble

  static float risen = 0;           // total distance risen so far (px)
  risen += rise * liq_dt;
  if (risen > 1000000.0f) risen = 0;

  for (int i = 0; i < count; i++) {
    uint32_t h = (uint32_t)(i + 1) * 2654435761u;   // scramble i into "random" bits
    int lane  = (h >> 8) % W;
    float spd = 0.6f + ((h >> 16) & 255) / 512.0f;   // this bubble's speed factor
    int r     = size * (40 + ((h >> 4) & 63)) / 100 + 2;

    float travel = H + 4 * r;
    float pos = fmodf(risen * spd + (h & 1023), travel);
    int y = H + 2 * r - (int)pos;
    int x = lane + (int)(sinf(globalTime * 1.5f + i) * wobble);
    uint8_t c = 1 + ((i * 23 + (int)(globalTime * 20)) % 254);

    display.drawCircle(x, y, r, c);
    display.fillCircle(x - r / 3, y - r / 3, r / 5 + 1, 255);  // white shine
  }
}

// ─── Preset 7: Wet Show ───────────────────────────────────────────────
// The classic 1960s overhead-projector trick: colored oil and water
// squeezed between two glass clock-faces, slowly turned by hand. Here the
// round "dish" spins, the liquid flows inside it, and the squeeze knob
// makes it breathe in and out like the glasses being pressed together.
static void liq_wetShow() {
  int dish    = liq_pot(4, 30, 80);  // p4: dish size (60 ≈ fills the height)
  int spin    = liq_pot(5, 0, 60);   // p5: dish rotation speed
  int squeeze = liq_pot(6, 0, 40);   // p6: squeeze (breathing) amount
  int bands   = liq_pot(7, 1, 6);    // p7: color bands
  liq_buildLut(bands);
  liq_buildPolar();

  static float rot = 0, flow = 0;
  liq_advance(rot, spin);
  liq_advance(flow, 20);
  int r8 = (int)rot & 255;                  // dish angle, 1/256ths of a turn
  int t  = (int)flow;
  int t2 = (int)(flow * 0.7f);
  // Breathing: pattern scale swings around 64 (= 1.0x)
  int pulse = 64 + (((sinTab[((int)(flow * 2)) & 255] - 128) * squeeze) >> 7);

  uint8_t* buf = display.getBuffer();
  for (int y = 0; y < LIQ_GH; y++) {
    int dy = y - LIQ_GH / 2;
    for (int x = 0; x < LIQ_GW; x++) {
      int dx = x - LIQ_GW / 2;
      int ang, dist;
      liq_polar(dx, dy, ang, dist);
      if (dist > dish) { liq_plot2x2(buf, x, y, 0); continue; }   // outside the glass

      // Turn the point by the dish angle, then back into x/y
      int a  = (ang + r8) & 255;
      int rx = (dist * (sinTab[(a + 64) & 255] - 128)) >> 7;
      int ry = (dist * (sinTab[a] - 128)) >> 7;
      rx = (rx * pulse) >> 6;
      ry = (ry * pulse) >> 6;

      int v = sinTab[(rx * 3 + (sinTab[(ry * 2 + t) & 255] >> 2) + t) & 255]
            + sinTab[(ry * 3 + (sinTab[(rx * 2 - t2) & 255] >> 2)) & 255]
            + sinTab[(dist * 2 - t2) & 255];
      liq_plot2x2(buf, x, y, liq_lut[v]);
    }
  }
}

// ─── Preset 8: Chromatic Rain ─────────────────────────────────────────
// Colored streaks fall down the screen with a bright white head, and the
// trails (p2) smear them into glowing rain. Like Bubbles, each drop's
// column and speed come from its number, so nothing needs storing.
static void liq_chromaRain() {
  int count  = liq_pot(4, 5, 60);    // p4: number of streaks
  int len    = liq_pot(5, 4, 60);    // p5: streak length
  int fall   = liq_pot(6, 20, 240);  // p6: fall speed (px/sec)
  int spread = liq_pot(7, 0, 40);    // p7: color difference between streaks

  static float fallen = 0, hue = 0;
  fallen += fall * liq_dt;
  if (fallen > 1000000.0f) fallen = 0;
  liq_advance(hue, 15);

  for (int i = 0; i < count; i++) {
    uint32_t h = (uint32_t)(i + 7) * 2654435761u;
    int x = (h >> 8) % W;
    float spd = 0.5f + ((h >> 16) & 255) / 256.0f;
    float travel = H + len * 2;
    int y = (int)fmodf(fallen * spd + (h & 1023), travel) - len;
    uint8_t c = 1 + (((int)hue + i * spread) % 254);
    display.drawFastVLine(x, y, len, c);
    display.drawPixel(x, y + len, 255);     // bright white head
  }
}

// ─── Preset 9: Silk ───────────────────────────────────────────────────
// Stripes of color that ripple sideways like silk or an aurora curtain.
// Every column gets its own wavy offset, and each row just reads along it.
static void liq_silk() {
  int density = liq_pot(4, 1, 8);    // p4: stripe density
  int sway    = liq_pot(5, 0, 120);  // p5: how far the stripes sway
  int drift   = liq_pot(6, 5, 80);   // p6: drift speed
  int bands   = liq_pot(7, 1, 6);    // p7: color bands
  liq_buildLut(bands);

  static float ph = 0;
  liq_advance(ph, drift);
  int t  = (int)ph;
  int t2 = (int)(ph * 0.7f);
  int t3 = (int)(ph * 0.45f);

  static int off[LIQ_GW];
  for (int x = 0; x < LIQ_GW; x++) {
    off[x] = (((sinTab[(x * 2 + t) & 255] - 128) * sway) >> 7)
           + (((sinTab[(x * 3 - t2) & 255] - 128) * sway) >> 8);
  }

  uint8_t* buf = display.getBuffer();
  for (int y = 0; y < LIQ_GH; y++) {
    for (int x = 0; x < LIQ_GW; x++) {
      int v = sinTab[(y * density + off[x]) & 255] * 2
            + (sinTab[(x + y / 2 + t3) & 255] >> 1);
      liq_plot2x2(buf, x, y, liq_lut[v]);
    }
  }
}

// ─── Preset 10: Oil Cells ─────────────────────────────────────────────
// Oil bubbles floating in water: the screen is split into cells, one per
// floating point, with dark edges where two cells meet. (This pattern is
// called a Voronoi diagram.) Each cell has its own color with a glow
// that shifts from its center outward.
#define LIQ_MAX_CELLS 10

static void liq_oilCells() {
  int n      = liq_pot(4, 3, LIQ_MAX_CELLS);  // p4: number of cells
  int edge   = liq_pot(5, 0, 120);            // p5: edge thickness
  int drift  = liq_pot(6, 2, 60);             // p6: drift speed
  int spread = liq_pot(7, 5, 60);             // p7: color difference between cells

  static float ph = 0, hue = 0;
  liq_advance(ph, drift / 100.0f);
  liq_advance(hue, 8);

  // Where is each cell's center this frame?
  int px[LIQ_MAX_CELLS], py[LIQ_MAX_CELLS];
  for (int i = 0; i < n; i++) {
    float fx = 1.0f + i * 0.37f, fy = 0.8f + i * 0.29f;
    px[i] = LIQ_GW / 2 + (int)(sinf(ph * fx + i * 1.7f) * (LIQ_GW / 2 - 4));
    py[i] = LIQ_GH / 2 + (int)(cosf(ph * fy + i * 2.3f) * (LIQ_GH / 2 - 4));
  }

  uint8_t* buf = display.getBuffer();
  int dyy[LIQ_MAX_CELLS];
  for (int y = 0; y < LIQ_GH; y++) {
    for (int i = 0; i < n; i++) dyy[i] = (y - py[i]) * (y - py[i]);
    for (int x = 0; x < LIQ_GW; x++) {
      // Find the nearest and second-nearest centers
      int d1 = 1 << 30, d2 = 1 << 30, near = 0;
      for (int i = 0; i < n; i++) {
        int dx = x - px[i];
        int d = dx * dx + dyy[i];
        if (d < d1) { d2 = d1; d1 = d; near = i; }
        else if (d < d2) { d2 = d; }
      }
      uint8_t c;
      if (d2 - d1 < edge) c = 0;                                 // dark edge
      else c = 1 + (((int)hue + near * spread + (d1 >> 3)) % 254); // cell glow
      liq_plot2x2(buf, x, y, c);
    }
  }
}

// ─── Preset 11: Mandala ───────────────────────────────────────────────
// A kaleidoscope with many mirrors. We fold the angle around the center
// into a small wedge and mirror it, so whatever happens in one wedge
// repeats all the way round.
static void liq_mandala() {
  int seg   = liq_pot(4, 3, 12);    // p4: number of mirror segments
  int zoom  = liq_pot(5, 1, 8);     // p5: ring zoom
  int spin  = liq_pot(6, 0, 60);    // p6: rotation speed
  int bands = liq_pot(7, 1, 6);     // p7: color bands
  liq_buildLut(bands);
  liq_buildPolar();

  static float ph = 0, rot = 0;
  liq_advance(ph, 30);
  liq_advance(rot, spin);
  int r8 = (int)rot & 255;
  int t  = (int)ph;
  int t2 = (int)(ph * 0.7f);

  uint8_t* buf = display.getBuffer();
  for (int y = 0; y < LIQ_GH; y++) {
    int dy = y - LIQ_GH / 2;
    for (int x = 0; x < LIQ_GW; x++) {
      int dx = x - LIQ_GW / 2;
      int ang, dist;
      liq_polar(dx, dy, ang, dist);
      int a = (((ang + r8) & 255) * seg) & 255;   // which part of the wedge
      if (a > 127) a = 255 - a;                   // mirror inside the wedge
      int v = sinTab[(a + dist * zoom - t) & 255]
            + sinTab[(a * 2 - ((dist * zoom) >> 1) + t2) & 255]
            + sinTab[(dist * zoom + t2) & 255];
      liq_plot2x2(buf, x, y, liq_lut[v]);
    }
  }
}

// ─── Public interface (what the platform calls) ───────────────────────

const char* prog_liquid_name() { return "LIQUID LIGHT"; }

const char* prog_liquid_character() {
  return "Oil-and-water light show: 12 presets, LFOs on p8-p15";
}

static const char* const liq_presetNames[] = {
  "Oil Plasma", "Lava Blobs", "Moire Pool", "Vortex",
  "Kaleido Oil", "Ink Drops", "Bubbles", "Wet Show",
  "Chroma Rain", "Silk", "Oil Cells", "Mandala"
};
#define LIQ_NUM_PRESETS 12

const char* prog_liquid_presetName(int preset) {
  if (preset >= 0 && preset < LIQ_NUM_PRESETS) return liq_presetNames[preset];
  return NULL;   // empty key: nothing shown in the info overlay
}

static const char* const liq_potLabels[LIQ_NUM_PRESETS][4] = {
  {"Zoom",    "Warp",   "Drift", "Bands"},   // Oil Plasma
  {"Count",   "Size",   "Wander", "Spread"}, // Lava Blobs
  {"Density", "Apart",  "Speed", "Bands"},   // Moire Pool
  {"Arms",    "Twist",  "Spin",  "Bands"},   // Vortex
  {"Zoom",    "Warp",   "Drift", "Bands"},   // Kaleido Oil
  {"Rate",    "Grow",   "Rings", "Spread"},  // Ink Drops
  {"Count",   "Size",   "Rise",  "Wobble"},  // Bubbles
  {"Dish",    "Spin",   "Squeeze", "Bands"}, // Wet Show
  {"Count",   "Length", "Fall",  "Spread"},  // Chroma Rain
  {"Density", "Sway",   "Drift", "Bands"},   // Silk
  {"Cells",   "Edge",   "Drift", "Spread"},  // Oil Cells
  {"Segments", "Zoom",  "Spin",  "Bands"},   // Mandala
};

static const char* const liq_lfoLabels[8] = {
  "Rate p4", "Rate p5", "Rate p6", "Rate p7",
  "Depth p4", "Depth p5", "Depth p6", "Depth p7"
};

const char* prog_liquid_potLabel(int preset, int pot) {
  if (pot >= 8 && pot <= 15) return liq_lfoLabels[pot - 8];  // LFO knobs
  if (pot < 4 || pot > 7) return "";          // p0-p3 use platform labels
  if (preset < 0 || preset >= LIQ_NUM_PRESETS) return "";
  return liq_potLabels[preset][pot - 4];
}

uint8_t prog_liquid_renderHint(int preset) {
  switch (preset) {
    case 1:                            // Lava Blobs
    case 5:                            // Ink Drops
    case 6:                            // Bubbles
    case 8:  return RENDER_TRAIL;      // Chroma Rain — trails melt through the palette
    case 0:
    case 2:
    case 3:
    case 4:
    case 7:
    case 9:
    case 10:
    case 11: return RENDER_PERPIXEL;   // we paint every pixel ourselves
    default: return RENDER_CLEAR;
  }
}

void prog_liquid_draw(int preset) {
  // How long since the last frame? (used for smooth speed changes)
  static float lastTime = 0;
  float dt = globalTime - lastTime;
  lastTime = globalTime;
  liq_dt = (dt > 0 && dt < 0.1f) ? dt : 0.016f;

  switch (preset) {
    case 0:  liq_oilPlasma(); break;
    case 1:  liq_lavaBlobs(); break;
    case 2:  liq_moirePool(); break;
    case 3:  liq_vortex();     break;
    case 4:  liq_kaleidoOil(); break;
    case 5:  liq_inkDrops();   break;
    case 6:  liq_bubbles();    break;
    case 7:  liq_wetShow();    break;
    case 8:  liq_chromaRain(); break;
    case 9:  liq_silk();       break;
    case 10: liq_oilCells();   break;
    case 11: liq_mandala();    break;
    default:
      display.setTextColor(255);
      display.setTextSize(1);
      display.setCursor(100, 116);
      display.print("empty preset");
      break;
  }
}
