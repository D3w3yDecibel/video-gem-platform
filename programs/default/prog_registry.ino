// =====================================================================
// PROGRAM REGISTRY — default platform programs (Video_Gem* by RMNA)
//
// Registers the default programs that ship with this repository.
// Replace or extend when merging a custom program set into the sketch.
// =====================================================================

ProgEntry g_programs[] = {
  PROG_ENTRY_EX(0, prog_crawl, prog_crawl_init, PROG_FLAG_OWNS_GLOBALS),  // Crawler (Dewey; replaced Basic Shapes)
  PROG_ENTRY_EX(1, prog_echo, prog_echo_init, PROG_FLAG_OWNS_GLOBALS),  // Echo Trails (Dewey; replaced Symmetry)
  PROG_ENTRY_EX(2, prog_rain, prog_rain_init, PROG_FLAG_OWNS_GLOBALS),  // Digital Rain (Dewey)
  PROG_ENTRY_EX(3, prog_dots, prog_dots_init, PROG_FLAG_OWNS_GLOBALS),  // Dot Cubes (Dewey; replaced Mind Melt)
  PROG_ENTRY(4, prog_liquid),   // Liquid Light (Dewey)
  PROG_ENTRY(5, prog_hypno),    // Hypnotic (Dewey)
  PROG_ENTRY_EX(6, prog_color, NULL, PROG_FLAG_OWNS_GLOBALS),  // Color Lab (moved from slot 2)
  PROG_ENTRY(7, prog_inputs),   // Inputs (moved from slot 3)
  PROG_ENTRY_EX(8, prog_weave, prog_weave_init, PROG_FLAG_OWNS_GLOBALS),  // Weave (Dewey)
  PROG_ENTRY_EX(9, prog_nodes, prog_nodes_init, PROG_FLAG_OWNS_GLOBALS),  // Nodes (Dewey; replaced Bitmaps)
  PROG_ENTRY(10, prog_fx),      // FX demo (moved from slot 11)
  PROG_ENTRY(11, prog_mapping), // Mapping helper (Dewey, moved from slot 5)
};
int g_numPrograms = sizeof(g_programs) / sizeof(g_programs[0]);
