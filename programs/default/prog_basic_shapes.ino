// =====================================================================
// PROGRAM: BASIC SHAPES (Video_Gem_Basic_Shapes by RMNA)
// 12 presets: intro shapes from Gray Area workshop — circles, polygons,
// lines, grids. Adapted from original by Ramona Sharples (RMNA).
// =====================================================================

// ─── Shape drawing (from Video_Gem_Basic_Shapes) ─────────────────────

static void bs_nestedCircles() {
  display.fillScreen(0);
  int cx = HALFW, cy = HALFH;
  for (int i = 1; i <= 8; i++) {
    int r = i * 13;
    int color = 30 + i * 26;
    display.drawCircle(cx, cy, r, color);
  }
}

static void bs_bubbles() {
  display.fillScreen(10);
  display.fillCircle(120, 130, 70, 40);
  display.fillCircle(200, 100, 55, 80);
  display.fillCircle(160, 160, 40, 140);
  display.fillCircle(230, 150, 30, 190);
  display.fillCircle(180, 120, 18, 240);
}

static void bs_eye() {
  display.fillScreen(0);
  int cx = HALFW, cy = HALFH;
  display.fillEllipse(cx, cy, 140, 60, 60);
  display.fillEllipse(cx, cy, 100, 45, 120);
  display.fillEllipse(cx, cy, 50, 30, 200);
  display.fillEllipse(cx, cy, 15, 15, 255);
}

static void bs_mountains() {
  display.fillScreen(0);
  display.fillTriangle(10, H, 140, 30, 270, H, 50);
  display.fillTriangle(80, H, 190, 60, 300, H, 110);
  display.fillTriangle(130, H, 220, 90, 310, H, 180);
}

static void bs_nestedRoundRects() {
  display.fillScreen(0);
  for (int i = 0; i < 6; i++) {
    int inset = i * 18;
    int x = 30 + inset, y = 15 + inset;
    int w = 260 - inset * 2, h = 210 - inset * 2;
    int radius = 20 - i * 2;
    int color = 60 + i * 35;
    display.drawRoundRect(x, y, w, h, radius, color);
  }
}

static void bs_fannedLines() {
  display.fillScreen(0);
  int numLines = 10, lineSpacing = W / numLines;
  for (int i = 0; i < numLines; i++) {
    int endX = i * lineSpacing, endY = 0;
    int color = 80 + i * ((255 - 80) / numLines);
    display.drawLine(0, H, endX, endY, color);
  }
}

static void bs_rectangleGrid() {
  display.fillScreen(0);
  int cols = 40, rows = 4;
  int rw = W / cols, rh = H / rows;
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      display.fillRect(j * rw, i * rh, rw - 1, rh - 1,
        ((i * cols) + j) * (255 / (cols * rows)));
    }
  }
}

static void bs_pixelGrid() {
  display.fillScreen(0);
  for (int row = 0; row < 16; row++) {
    for (int col = 0; col < 20; col++) {
      int x = 40 + col * 12, y = 28 + row * 12;
      int color = (row * 20 + col) * 255 / 319;
      display.drawPixel(x, y, color);
    }
  }
}

static void bs_mystery1() {
  display.fillScreen(0);
  display.drawCircle(70, 70, 50, 255);
  display.fillCircle(200, 170, 30, 128);
  display.fillCircle(230, 170, 30, 255);
}

static void bs_mystery2() {
  display.fillScreen(0);
  display.fillCircle(160, 120, 80, 255);
  display.fillRect(60, 20, 200, 100, 0);
  display.fillCircle(120, 80, 20, 255);
  display.fillCircle(200, 80, 20, 255);
  display.drawCircle(160, 120, 120, 255);
}

static void bs_mystery3() {
  display.fillScreen(0);
  display.drawFastHLine(80, 120, 160, 128);
  display.drawLine(240, 120, 200, 80, 128);
  display.drawLine(240, 120, 200, 160, 128);
}

// ─── Public interface ────────────────────────────────────────────────

const char* prog_basic_shapes_name() { return "BASIC SHAPES"; }
const char* prog_basic_shapes_character() {
  return "Intro shapes — circles, polygons, grids (Video_Gem_Basic_Shapes by RMNA)";
}

static const char* const bs_presetNames[] = {
  "Nested Circles", "Bubbles", "Eye", "Mountains", "Nested Rects",
  "Fanned Lines", "Rect Grid", "Pixel Grid", "Dewey Dots", "Mystery 2", "Mystery 3", "Rects"
};

const char* prog_basic_shapes_presetName(int preset) {
  if (preset >= 0 && preset < 12) return bs_presetNames[preset];
  return NULL;
}

const char* prog_basic_shapes_potLabel(int preset, int pot) {
  (void)preset;
  (void)pot;
  return "";
}

uint8_t prog_basic_shapes_renderHint(int preset) {
  (void)preset;
  return RENDER_CLEAR;
}

void prog_basic_shapes_draw(int preset) {
  switch (preset) {
    case 0:  bs_nestedCircles();    break;
    case 1:  bs_bubbles();          break;
    case 2:  bs_eye();              break;
    case 3:  bs_mountains();        break;
    case 4:  bs_nestedRoundRects(); break;
    case 5:  bs_fannedLines();      break;
    case 6:  bs_rectangleGrid();    break;
    case 7:  bs_pixelGrid();        break;
    case 8:  bs_mystery1();         break;
    case 9:  bs_mystery2();         break;
    case 10: bs_mystery3();         break;
    case 11: bs_nestedRoundRects(); break;
    default: display.fillScreen(0); break;
  }
}
