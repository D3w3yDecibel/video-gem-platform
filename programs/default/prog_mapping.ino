// =====================================================================
// PROGRAM: MAPPING (by Dewey)
// Projection-mapping helper: test patterns for lining up a projector,
// plus a "corner-pin" quad you can stretch onto a real surface (a wall,
// a box, a canvas) and fill with moving color.
//
// Presets:
//   k0  Grid        — alignment grid with a center cross
//   k1  Circles     — concentric circles + crosshair (lens/keystone check)
//   k2  Checker     — checkerboard for focus
//   k3  Solid       — full-screen solid color (brightness / color check)
//   k4  Quad Pin    — outline of a four-corner shape you move with knobs
//   k5  Quad Tunnel — same shape, filled with nested quads that flow inward
//   k6  Quad Plasma — same shape, filled with bands that sweep across it
//
// Quad corners (k4–k6) — each corner uses two knobs, X then Y:
//   p4/p5   top-left      p6/p7   top-right
//   p8/p9   bottom-right  p10/p11 bottom-left
// All three quad presets share the same corners, so line it up once on
// k4, then switch to k5 or k6 and it stays in place.
// =====================================================================

// ─── Test patterns ────────────────────────────────────────────────────

static void map_grid() {
  int spacing = potMap(4, 10, 80);   // p4: grid spacing (px)
  int c = potMap(5, 1, 255);         // p5: line color (255 = white)
  for (int x = HALFW % spacing; x < W; x += spacing) display.drawFastVLine(x, 0, H, c);
  for (int y = HALFH % spacing; y < H; y += spacing) display.drawFastHLine(0, y, W, c);
  // Center cross and border in white
  display.drawFastHLine(0, HALFH, W, 255);
  display.drawFastVLine(HALFW, 0, H, 255);
  display.drawRect(0, 0, W, H, 255);
}

static void map_circles() {
  int spacing = potMap(4, 8, 60);    // p4: ring spacing (px)
  int c = potMap(5, 1, 255);         // p5: ring color
  for (int r = spacing; r < 200; r += spacing) display.drawCircle(HALFW, HALFH, r, c);
  display.drawFastHLine(0, HALFH, W, 255);
  display.drawFastVLine(HALFW, 0, H, 255);
  // Corner circles: if these look like ovals, the projector is keystoned
  display.drawCircle(20, 20, 16, 255);
  display.drawCircle(W - 21, 20, 16, 255);
  display.drawCircle(20, H - 21, 16, 255);
  display.drawCircle(W - 21, H - 21, 16, 255);
}

static void map_checker() {
  int size = potMap(4, 2, 40);       // p4: square size (px)
  int c = potMap(5, 1, 255);         // p5: square color
  for (int y = 0; y < H; y += size)
    for (int x = 0; x < W; x += size)
      if (((x / size) + (y / size)) & 1) display.fillRect(x, y, size, size, c);
}

static void map_solid() {
  display.fillScreen(potMap(4, 1, 255));   // p4: color (255 = white)
}

// ─── Corner-pin quad ──────────────────────────────────────────────────
// Note: these helpers pass plain numbers (not a custom struct) on
// purpose. The Arduino IDE auto-writes declarations for every function
// near the top of the sketch, BEFORE any struct in this file exists,
// so a struct in a function's parameters would fail to compile.

// Corner positions, clockwise from top-left: 0=TL 1=TR 2=BR 3=BL
static int map_qx[4], map_qy[4];

static void map_readCorners() {
  map_qx[0] = potMap(4, 0, W - 1);  map_qy[0] = potMap(5, 0, H - 1);   // top-left
  map_qx[1] = potMap(6, 0, W - 1);  map_qy[1] = potMap(7, 0, H - 1);   // top-right
  map_qx[2] = potMap(8, 0, W - 1);  map_qy[2] = potMap(9, 0, H - 1);   // bottom-right
  map_qx[3] = potMap(10, 0, W - 1); map_qy[3] = potMap(11, 0, H - 1);  // bottom-left
}

// Number part-way between a and b (t goes 0..256)
static int map_lerp(int a, int b, int t) {
  return a + ((b - a) * t) / 256;
}

// Fill a four-corner shape (corners in order) as two triangles
static void map_fillQuad(int ax, int ay, int bx, int by,
                         int cx, int cy, int dx, int dy, uint8_t col) {
  display.fillTriangle(ax, ay, bx, by, cx, cy, col);
  display.fillTriangle(ax, ay, cx, cy, dx, dy, col);
}

static void map_outlineQuad(uint8_t col) {
  for (int i = 0; i < 4; i++) {
    int j = (i + 1) % 4;
    display.drawLine(map_qx[i], map_qy[i], map_qx[j], map_qy[j], col);
  }
}

// Frame-to-frame timer for smooth speed changes (same idea as Liquid Light)
static float map_advance(float& phase, float speed) {
  static float lastT = 0;
  static float dt = 0.016f;
  static unsigned long lastFrame = 0;
  unsigned long now = millis();
  if (now != lastFrame) {            // only measure once per frame
    float d = globalTime - lastT;
    lastT = globalTime;
    dt = (d > 0 && d < 0.1f) ? d : 0.016f;
    lastFrame = now;
  }
  phase += speed * dt;
  if (phase >= 25400.0f) phase -= 25400.0f;   // multiple of 254 keeps colors continuous
  return phase;
}

static void map_quadPin() {
  map_readCorners();
  int fill = potMap(12, 0, 255);     // p12: fill color (0 = outline only)
  if (fill > 0) {
    map_fillQuad(map_qx[0], map_qy[0], map_qx[1], map_qy[1],
                 map_qx[2], map_qy[2], map_qx[3], map_qy[3], fill);
  }
  map_outlineQuad(255);
  // Corner handles, numbered so you know which knobs move which corner
  display.setTextColor(255);
  display.setTextSize(1);
  for (int i = 0; i < 4; i++) {
    display.drawCircle(map_qx[i], map_qy[i], 5, 255);
    display.setCursor(map_qx[i] + 7, map_qy[i] - 3);
    display.print(i + 1);
  }
}

// Nested copies of the quad, shrinking toward its center, with colors
// that cycle — a tunnel that stays locked to your surface.
static void map_quadTunnel() {
  map_readCorners();
  int layers = potMap(12, 2, 24);    // p12: number of layers
  int speed  = potMap(13, 0, 200);   // p13: flow speed
  int spread = potMap(14, 1, 40);    // p14: color step between layers

  int cx = (map_qx[0] + map_qx[1] + map_qx[2] + map_qx[3]) / 4;
  int cy = (map_qy[0] + map_qy[1] + map_qy[2] + map_qy[3]) / 4;
  static float ph = 0;
  map_advance(ph, speed);

  for (int i = 0; i < layers; i++) {
    int t = (i * 256) / layers;          // 0 = full size, toward 256 = center
    uint8_t col = 1 + (((int)ph + i * spread) % 254);
    map_fillQuad(map_lerp(map_qx[0], cx, t), map_lerp(map_qy[0], cy, t),
                 map_lerp(map_qx[1], cx, t), map_lerp(map_qy[1], cy, t),
                 map_lerp(map_qx[2], cx, t), map_lerp(map_qy[2], cy, t),
                 map_lerp(map_qx[3], cx, t), map_lerp(map_qy[3], cy, t), col);
  }
}

// Strips that sweep across the quad from its left edge to its right edge.
static void map_quadPlasma() {
  map_readCorners();
  int strips = potMap(12, 4, 48);    // p12: number of strips
  int speed  = potMap(13, 0, 200);   // p13: sweep speed
  int spread = potMap(14, 1, 40);    // p14: color step between strips
  int wave   = potMap(15, 0, 60);    // p15: wave amount

  static float ph = 0;
  map_advance(ph, speed);

  for (int i = 0; i < strips; i++) {
    int t0 = (i * 256) / strips, t1 = ((i + 1) * 256) / strips;
    // Top edge runs TL→TR, bottom edge runs BL→BR
    int ax = map_lerp(map_qx[0], map_qx[1], t0), ay = map_lerp(map_qy[0], map_qy[1], t0);
    int bx = map_lerp(map_qx[0], map_qx[1], t1), by = map_lerp(map_qy[0], map_qy[1], t1);
    int cx = map_lerp(map_qx[3], map_qx[2], t1), cy = map_lerp(map_qy[3], map_qy[2], t1);
    int dx = map_lerp(map_qx[3], map_qx[2], t0), dy = map_lerp(map_qy[3], map_qy[2], t0);
    int w = ((sinTab[(i * 16 + (int)ph) & 255] - 128) * wave) / 128;
    uint8_t col = 1 + ((((int)ph + i * spread + w) % 254 + 254) % 254);
    map_fillQuad(ax, ay, bx, by, cx, cy, dx, dy, col);
  }
  map_outlineQuad(255);
}

// ─── Public interface ─────────────────────────────────────────────────

const char* prog_mapping_name() { return "MAPPING"; }

const char* prog_mapping_character() {
  return "Projector test patterns + corner-pin quad for mapping";
}

static const char* const map_presetNames[] = {
  "Grid", "Circles", "Checker", "Solid", "Quad Pin", "Quad Tunnel", "Quad Plasma"
};
#define MAP_NUM_PRESETS 7

const char* prog_mapping_presetName(int preset) {
  if (preset >= 0 && preset < MAP_NUM_PRESETS) return map_presetNames[preset];
  return NULL;
}

const char* prog_mapping_potLabel(int preset, int pot) {
  static const char* const corners[8] = {
    "TL x", "TL y", "TR x", "TR y", "BR x", "BR y", "BL x", "BL y"
  };
  switch (preset) {
    case 0: case 1:
      if (pot == 4) return "Spacing";
      if (pot == 5) return "Color";
      return "";
    case 2:
      if (pot == 4) return "Size";
      if (pot == 5) return "Color";
      return "";
    case 3:
      return (pot == 4) ? "Color" : "";
    case 4: case 5: case 6:
      if (pot >= 4 && pot <= 11) return corners[pot - 4];
      if (preset == 4 && pot == 12) return "Fill";
      if (preset == 5) {
        if (pot == 12) return "Layers";
        if (pot == 13) return "Speed";
        if (pot == 14) return "Spread";
      }
      if (preset == 6) {
        if (pot == 12) return "Strips";
        if (pot == 13) return "Speed";
        if (pot == 14) return "Spread";
        if (pot == 15) return "Wave";
      }
      return "";
  }
  return "";
}

uint8_t prog_mapping_renderHint(int preset) {
  (void)preset;
  return RENDER_CLEAR;   // every preset draws a fresh frame on black
}

void prog_mapping_draw(int preset) {
  switch (preset) {
    case 0: map_grid();       break;
    case 1: map_circles();    break;
    case 2: map_checker();    break;
    case 3: map_solid();      break;
    case 4: map_quadPin();    break;
    case 5: map_quadTunnel(); break;
    case 6: map_quadPlasma(); break;
    default:
      display.setTextColor(255);
      display.setTextSize(1);
      display.setCursor(100, 116);
      display.print("empty preset");
      break;
  }
}
