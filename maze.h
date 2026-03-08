/*
 * maze.h - Sultan's Rogue v0.4
 * Amstrad CPC 6128 / z88dk
 *
 * Constants, global state, and function prototypes.
 */

#ifndef MAZE_HDR_H
#define MAZE_HDR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int fgetc_cons(void);

/* ============================================================
 * CONSTANTS
 * ============================================================ */

/* CPC MODE 1 = 40 cols x 25 rows.
 * Maze width 39 (odd, so generator works) fills cols 1-39.
 * Height 16 as before, leaving room for status below.       */
#define MAZE_W      39
#define MAZE_H      16
#define NUM_GEMS     6

#define DIR_N  0
#define DIR_E  1
#define DIR_S  2
#define DIR_W  3

#define CELL_WALL  1
#define CELL_GEM   2
#define CELL_EXIT  4

#define MAX_SCORES  10
#define NAME_LEN     9   /* 8 chars + null */

/* Larger stack for wider maze (39*16/4 ~ 156, round up) */
#define STACK_SIZE 200

/* Energy awarded per gem collected when exiting */
#define ENERGY_PER_GEM  25

/* CPC MODE 1 with z88dk -clib=ansi:
 * Only two ANSI foreground colours work reliably:
 *   ANSI 31 (red)    -> CPC pen 1 (bright red)
 *   ANSI 33 (yellow) -> CPC pen 3 (bright yellow)
 * Background is pen 0 (blue).
 * Pen 2 (nominally green/cyan) is unreliable.
 *
 * Colour assignments:
 *   Yellow = hedges, player, exit (bright, visible)
 *   Red    = gems, ghost (danger/treasure)          */
#define COL_RED     31   /* pen 1: gems, ghost   */
#define COL_YELLOW  33   /* pen 3: hedges, player */

/* ============================================================
 * GLOBAL STATE (defined in maze.c)
 * ============================================================ */

/* Maze grid */
extern unsigned char maze[MAZE_H][MAZE_W];

/* Player */
extern unsigned char px, py, pdir;
extern int energy;
extern unsigned char gems_collected, gems_total;

/* Ghost */
extern unsigned char ghost_x, ghost_y, ghost_timer;

/* Game flow */
extern unsigned char game_running;
extern unsigned char maze_seed;
extern int score;
extern unsigned char level;

/* Items */
extern unsigned char gem_x[NUM_GEMS], gem_y[NUM_GEMS];
extern unsigned char gem_taken[NUM_GEMS];
extern unsigned char exit_x, exit_y;

/* Direction tables */
extern const signed char dx[];
extern const signed char dy[];

/* Partial-redraw previous positions */
extern unsigned char old_px, old_py, old_gx, old_gy;

/* Maze generation stack */
extern unsigned char stk_x[STACK_SIZE];
extern unsigned char stk_y[STACK_SIZE];
extern unsigned int  stk_ptr;

/* High score table */
extern char hs_name[MAX_SCORES][NAME_LEN];
extern int  hs_score[MAX_SCORES];
extern unsigned char hs_count;

/* RNG */
extern unsigned int rng_state;

/* ============================================================
 * FUNCTION PROTOTYPES
 * ============================================================ */

/* RNG */
unsigned char rng(void);

/* Screen helpers */
void cls(void);
void goto_xy(unsigned char x, unsigned char y);
void set_colour(unsigned char c);
void reset_colour(void);
void print_at(unsigned char x, unsigned char y, char *s);
void print_num_at(unsigned char x, unsigned char y, int n);
unsigned char confirm(unsigned char row);

/* High scores */
void init_high_scores(void);
unsigned char hs_qualifies(int s);
void hs_insert(unsigned char pos, char *name, int s);
void show_high_scores(void);
void get_player_name(char *buf);

/* Maze */
void generate_maze(void);
void place_items(void);
void start_next_level(void);

/* Helpers */
unsigned char is_wall(unsigned char cx, unsigned char cy);
unsigned char is_border(unsigned char cx, unsigned char cy);

/* Ghost */
void move_ghost(void);

/* Drawing */
void draw_cell(unsigned char x, unsigned char y);
void draw_map(void);
void update_map(void);
void draw_status(void);

/* Screens */
void level_complete_screen(int bonus, int energy_bonus);
unsigned char early_exit_prompt(void);
unsigned char title_screen(void);
void gameover_screen(void);

/* Game */
unsigned char game_loop(void);

#endif /* MAZE_HDR_H */