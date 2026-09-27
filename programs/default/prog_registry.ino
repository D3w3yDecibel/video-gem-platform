// =====================================================================
// PROGRAM REGISTRY — default platform programs (Video_Gem* by RMNA)
//
// Registers the default programs that ship with this repository.
// Replace or extend when merging a custom program set into the sketch.
// =====================================================================

ProgEntry g_programs[] = {
  PROG_ENTRY_EX(0, prog_pixcity, prog_pixcity_init, PROG_FLAG_OWNS_GLOBALS),  // Pixel City (Dewey) — comes up at startup
  PROG_ENTRY(1, prog_basic_shapes),  // Basic Shapes (moved from slot 0; Symmetry off for now)
  PROG_ENTRY_EX(2, prog_color, NULL, PROG_FLAG_OWNS_GLOBALS),
  PROG_ENTRY(3, prog_inputs),
  PROG_ENTRY(9, prog_bitmaps),
  PROG_ENTRY(11, prog_fx),
};
int g_numPrograms = sizeof(g_programs) / sizeof(g_programs[0]);
