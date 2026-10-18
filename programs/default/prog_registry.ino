// =====================================================================
// PROGRAM REGISTRY — SET 5 "ARCADE" (Dewey)
//
// Set 5: twelve self-playing arcade games in the spirit of the classic
// cabinets. Each one plays itself like an attract mode; turn p0/p1 or press
// k12 and you take over ("1UP"). Leave the controls for 8 seconds and the
// computer plays again ("DEMO"). All characters, creatures and names are
// original — the games borrow the gameplay, not the artwork.
//
//   slot 0  DIGGER   — tunnel through the earth, puff up the critters
//   slot 1  ROLLER   — race a marble down a floating isometric course
//   slot 2  TUBE     — hold the rim of a glowing vector tube
//   slot 3  ROCKS    — blast space rocks in vectors
//   slot 4  DEFENCE  — protect six cities from falling missiles
//   slot 5  CRAWLER  — a segmented bug winds down a mushroom field
//   slot 6  BRICKS   — paddle, ball and a wall of bricks
//   slot 7  LANDER   — set a craft down on a lunar pad
//   slot 8  TANKS    — wireframe tank battle from the gunner's seat
//   slot 9  HOPPER   — hop a pyramid of cubes, changing their colours
//   slot 10 MUNCHER  — eat the dots in a generated maze
//   slot 11 CYCLES   — light-trail duel, top-down or tilted 3-D
//
// Shared engine: vg5a.h (take-over input, palette looks, sprites, font,
// HUD, particles, camera) on top of vg3.h, plus vg5fx.h (p12-p15 live FX).
// Switch program: hold k14 (Control) + k15 (Shift) and press k0-k11.
// The screen shows "PROGRAM n" with n = slot + 1.
//
// Older sets (Set 1-4, and the first Set 5 "Strands, Plots & Clay") may
// still be in this folder but are not listed here, so the linker leaves
// them out: they cost no RAM.
//
// RAM budget: keep the compile's "Global variables" under ~88 KB.
// =====================================================================

ProgEntry g_programs[] = {
  PROG_ENTRY_EX(0, prog_digger, prog_digger_init, PROG_FLAG_OWNS_GLOBALS),    // DIGGER — slot 0
  PROG_ENTRY_EX(1, prog_roller, prog_roller_init, PROG_FLAG_OWNS_GLOBALS),    // ROLLER — slot 1
  PROG_ENTRY_EX(2, prog_tube, prog_tube_init, PROG_FLAG_OWNS_GLOBALS),        // TUBE — slot 2
  PROG_ENTRY_EX(3, prog_rocks, prog_rocks_init, PROG_FLAG_OWNS_GLOBALS),      // ROCKS — slot 3
  PROG_ENTRY_EX(4, prog_defence, prog_defence_init, PROG_FLAG_OWNS_GLOBALS),  // DEFENCE — slot 4
  PROG_ENTRY_EX(5, prog_crawler, prog_crawler_init, PROG_FLAG_OWNS_GLOBALS),  // CRAWLER — slot 5
  PROG_ENTRY_EX(6, prog_bricks, prog_bricks_init, PROG_FLAG_OWNS_GLOBALS),    // BRICKS — slot 6
  PROG_ENTRY_EX(7, prog_lander, prog_lander_init, PROG_FLAG_OWNS_GLOBALS),    // LANDER — slot 7
  PROG_ENTRY_EX(8, prog_tanks, prog_tanks_init, PROG_FLAG_OWNS_GLOBALS),      // TANKS — slot 8
  PROG_ENTRY_EX(9, prog_hopper, prog_hopper_init, PROG_FLAG_OWNS_GLOBALS),    // HOPPER — slot 9
  PROG_ENTRY_EX(10, prog_muncher, prog_muncher_init, PROG_FLAG_OWNS_GLOBALS), // MUNCHER — slot 10
  PROG_ENTRY_EX(11, prog_cycles, prog_cycles_init, PROG_FLAG_OWNS_GLOBALS),   // CYCLES — slot 11
};
int g_numPrograms = sizeof(g_programs) / sizeof(g_programs[0]);
