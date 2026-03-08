/*
 * Sultan's Rogue - version 0.4
 * Amstrad CPC 6128 / z88dk
 *
 * A rogue-like based on the Sultan's Maze game.
 *
 * by @mathsDOTearth on github
 * https://github.com/mathsDOTearth/CPCprogramming/
 *
 * v0.4 - Full-width maze (39x16).
 *        Early exit option with partial score/energy.
 *        Energy bonus = 25 per gem collected on exit.
 *
 * v0.3 - High score table (top 10, 8-char names).
 *
 * v0.2 - Continuous play through random mazes.
 *
 * v0.1 - Single 16x16 maze
 * 
 * Compile:
 *   zcc +cpc -clib=ansi -lndos -O2 -create-app maze.c -o maze.bin
 *   iDSK maze.dsk -n
 *   iDSK maze.dsk -i ./maze.cpc
 * Run: run"maze.cpc
 */

#pragma output CRT_STACK_SIZE = 512

#include "maze.h"

/* ============================================================
 * GLOBAL STATE (storage - declared extern in maze.h)
 * ============================================================ */

unsigned char maze[MAZE_H][MAZE_W];
unsigned char px, py, pdir;
int energy;
unsigned char gems_collected, gems_total;
unsigned char ghost_x, ghost_y, ghost_timer;
unsigned char game_running;
unsigned char maze_seed;
int score;
unsigned char level;

unsigned char gem_x[NUM_GEMS], gem_y[NUM_GEMS];
unsigned char gem_taken[NUM_GEMS];
unsigned char exit_x, exit_y;

const signed char dx[] = { 0, 1, 0, -1 };
const signed char dy[] = { -1, 0, 1, 0 };

unsigned char old_px, old_py, old_gx, old_gy;

unsigned char stk_x[STACK_SIZE];
unsigned char stk_y[STACK_SIZE];
unsigned int  stk_ptr;

char hs_name[MAX_SCORES][NAME_LEN];
int  hs_score[MAX_SCORES];
unsigned char hs_count;

unsigned int rng_state;

/* ============================================================
 * RNG (LFSR)
 * ============================================================ */

unsigned char rng(void)
{
    unsigned char bit;
    bit = ((rng_state >> 0) ^ (rng_state >> 2) ^
           (rng_state >> 3) ^ (rng_state >> 5)) & 1;
    rng_state = (rng_state >> 1) | (bit << 15);
    return (unsigned char)(rng_state & 0xFF);
}

/* ============================================================
 * SCREEN HELPERS
 * ============================================================ */

void cls(void)
{
    putchar(27); putchar('['); putchar('2'); putchar('J');
    putchar(27); putchar('['); putchar('H');
}

void goto_xy(unsigned char x, unsigned char y)
{
    putchar(27); putchar('[');
    if (y >= 10) putchar('0' + y / 10);
    putchar('0' + y % 10);
    putchar(';');
    if (x >= 10) putchar('0' + x / 10);
    putchar('0' + x % 10);
    putchar('H');
}

/* Set ANSI foreground colour: e.g. COL_GREEN (32) -> ESC[32m */
void set_colour(unsigned char c)
{
    putchar(27); putchar('[');
    if (c >= 10) putchar('0' + c / 10);
    putchar('0' + c % 10);
    putchar('m');
}

/* Reset to default colour: ESC[0m */
void reset_colour(void)
{
    putchar(27); putchar('['); putchar('0'); putchar('m');
}

void print_at(unsigned char x, unsigned char y, char *s)
{
    goto_xy(x, y);
    while (*s) { putchar(*s); s++; }
}

void print_num_at(unsigned char x, unsigned char y, int n)
{
    char buf[8];
    int i = 0;
    goto_xy(x, y);
    if (n == 0) { putchar('0'); return; }
    if (n < 0) { putchar('-'); n = -n; }
    while (n > 0 && i < 7) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i > 0) putchar(buf[--i]);
}

unsigned char confirm(unsigned char row)
{
    int key;
    print_at(1, row, "Are you sure? (Y/N)              ");
    while (1) {
        key = fgetc_cons();
        if (key == 'y' || key == 'Y') return 1;
        if (key == 'n' || key == 'N') return 0;
    }
}

/* ============================================================
 * HIGH SCORE TABLE
 * ============================================================ */

void init_high_scores(void)
{
    hs_count = 8;

    strcpy(hs_name[0], "Rich");     hs_score[0] = 1000;
    strcpy(hs_name[1], "Bob");      hs_score[1] = 900;
    strcpy(hs_name[2], "Nick");     hs_score[2] = 800;
    strcpy(hs_name[3], "Fred");     hs_score[3] = 700;
    strcpy(hs_name[4], "Tonks");    hs_score[4] = 600;
    strcpy(hs_name[5], "Frodo");    hs_score[5] = 500;
    strcpy(hs_name[6], "Roland");   hs_score[6] = 400;
    strcpy(hs_name[7], "Arnold");   hs_score[7] = 300;
}

unsigned char hs_qualifies(int s)
{
    unsigned char i;
    if (s <= 0) return 255;
    for (i = 0; i < hs_count; i++) {
        if (s > hs_score[i]) return i;
    }
    if (hs_count < MAX_SCORES) return hs_count;
    return 255;
}

void hs_insert(unsigned char pos, char *name, int s)
{
    unsigned char i;

    if (hs_count < MAX_SCORES) hs_count++;

    for (i = hs_count - 1; i > pos; i--) {
        strcpy(hs_name[i], hs_name[i - 1]);
        hs_score[i] = hs_score[i - 1];
    }

    strcpy(hs_name[pos], name);
    hs_score[pos] = s;
}

void show_high_scores(void)
{
    unsigned char i, j, pad;
    char buf[8];
    int n;

    cls();
    puts("");
    puts("    ==============================");
    puts("      SULTAN'S ROGUE HIGH SCORES");
    puts("    ==============================");
    puts("");

    for (i = 0; i < hs_count; i++) {
        putchar(' '); putchar(' ');
        if (i < 9) {
            putchar(' ');
            putchar('1' + i);
        } else {
            putchar('1'); putchar('0');
        }
        putchar('.'); putchar(' ');

        j = 0;
        while (hs_name[i][j]) { putchar(hs_name[i][j]); j++; }
        pad = j;
        while (pad < 10) { putchar(' '); pad++; }

        n = hs_score[i];
        j = 0;
        if (n == 0) { buf[j++] = '0'; }
        else { while (n > 0 && j < 7) { buf[j++] = '0' + (n % 10); n /= 10; } }
        pad = j;
        while (pad < 6) { putchar(' '); pad++; }
        while (j > 0) putchar(buf[--j]);

        putchar('\n');
    }

    for (i = hs_count; i < MAX_SCORES; i++) {
        putchar(' '); putchar(' ');
        if (i < 9) {
            putchar(' ');
            putchar('1' + i);
        } else {
            putchar('1'); putchar('0');
        }
        putchar('.'); putchar(' ');
        puts("---");
    }

    puts("");
    puts("  Press any key...");
    fgetc_cons();
}

void get_player_name(char *buf)
{
    int key;
    unsigned char len = 0;

    puts("");
    puts("  Enter your name (max 8):");
    putchar(' '); putchar(' ');

    while (1) {
        key = fgetc_cons();

        if ((key == 13 || key == 10) && len > 0) break;

        if ((key == 8 || key == 127) && len > 0) {
            len--;
            putchar(8); putchar(' '); putchar(8);
            continue;
        }

        if (key >= 32 && key <= 126 && len < 8) {
            buf[len++] = (char)key;
            putchar(key);
        }
    }

    buf[len] = '\0';
}

/* ============================================================
 * MAZE GENERATION
 * ============================================================ */

void generate_maze(void)
{
    unsigned char x, y, nx, ny;
    unsigned char dirs[4];
    unsigned char i, j, count;

    for (y = 0; y < MAZE_H; y++)
        for (x = 0; x < MAZE_W; x++)
            maze[y][x] = CELL_WALL;

    x = 1; y = 1; maze[y][x] = 0;
    stk_ptr = 0;
    stk_x[stk_ptr] = x; stk_y[stk_ptr] = y; stk_ptr++;

    while (stk_ptr > 0) {
        x = stk_x[stk_ptr - 1]; y = stk_y[stk_ptr - 1];
        count = 0;
        for (i = 0; i < 4; i++) {
            nx = x + dx[i] * 2; ny = y + dy[i] * 2;
            if (nx >= 1 && nx < MAZE_W - 1 &&
                ny >= 1 && ny < MAZE_H - 1 &&
                maze[ny][nx] == CELL_WALL)
                dirs[count++] = i;
        }
        if (count == 0) { stk_ptr--; }
        else {
            j = rng() % count; i = dirs[j];
            nx = x + dx[i] * 2; ny = y + dy[i] * 2;
            maze[y + dy[i]][x + dx[i]] = 0;
            maze[ny][nx] = 0;
            stk_x[stk_ptr] = nx; stk_y[stk_ptr] = ny; stk_ptr++;
        }
    }

    /* Extra wall removal passes - more for wider maze */
    for (i = 0; i < 20; i++) {
        x = (rng() % (MAZE_W - 4)) + 2;
        y = (rng() % (MAZE_H - 4)) + 2;
        if (maze[y][x] == CELL_WALL) {
            count = 0;
            if (y > 0 && maze[y-1][x] == 0) count++;
            if (y < MAZE_H-1 && maze[y+1][x] == 0) count++;
            if (x > 0 && maze[y][x-1] == 0) count++;
            if (x < MAZE_W-1 && maze[y][x+1] == 0) count++;
            if (count == 2) maze[y][x] = 0;
        }
    }
}

/* ============================================================
 * PLACE ITEMS
 * ============================================================ */

void place_items(void)
{
    unsigned char i, x, y;

    px = 1; py = 1; pdir = DIR_E;
    maze[1][1] = 0;

    gems_total = NUM_GEMS;
    for (i = 0; i < NUM_GEMS; i++) {
        do {
            x = (rng() % (MAZE_W - 2)) + 1;
            y = (rng() % (MAZE_H - 2)) + 1;
        } while (maze[y][x] != 0 || (x <= 2 && y <= 2));
        gem_x[i] = x; gem_y[i] = y; gem_taken[i] = 0;
        maze[y][x] |= CELL_GEM;
    }

    exit_x = MAZE_W - 2; exit_y = MAZE_H - 2;
    maze[exit_y][exit_x] = CELL_EXIT;
    if (maze[exit_y - 1][exit_x] == CELL_WALL &&
        maze[exit_y][exit_x - 1] == CELL_WALL)
        maze[exit_y - 1][exit_x] = 0;

    do {
        ghost_x = (rng() % (MAZE_W - 4)) + 2;
        ghost_y = (rng() % (MAZE_H - 4)) + 2;
    } while ((maze[ghost_y][ghost_x] & CELL_WALL) ||
             (ghost_x + ghost_y < 8));

    ghost_timer = 0;
    gems_collected = 0;
    game_running = 1;
    old_px = px; old_py = py;
    old_gx = ghost_x; old_gy = ghost_y;
}

/* ============================================================
 * HELPERS
 * ============================================================ */

unsigned char is_wall(unsigned char cx, unsigned char cy)
{
    if (cx >= MAZE_W || cy >= MAZE_H) return 1;
    return (maze[cy][cx] & CELL_WALL) ? 1 : 0;
}

unsigned char is_border(unsigned char cx, unsigned char cy)
{
    if (cx == 0 || cx == MAZE_W - 1) return 1;
    if (cy == 0 || cy == MAZE_H - 1) return 1;
    return 0;
}

/* ============================================================
 * GHOST AI
 * ============================================================ */

void move_ghost(void)
{
    unsigned char best_dir, best_dist, i, nx, ny, dist;
    unsigned char moved;

    if ((rng() & 3) == 0) {
        i = rng() & 3;
        nx = ghost_x + dx[i]; ny = ghost_y + dy[i];
        if (!is_wall(nx, ny)) {
            ghost_x = nx; ghost_y = ny; return;
        }
    }

    best_dir = 0; best_dist = 255; moved = 0;
    for (i = 0; i < 4; i++) {
        nx = ghost_x + dx[i]; ny = ghost_y + dy[i];
        if (!is_wall(nx, ny)) {
            dist = abs((int)nx - (int)px) + abs((int)ny - (int)py);
            if (dist < best_dist) {
                best_dist = dist; best_dir = i; moved = 1;
            }
        }
    }
    if (moved) {
        ghost_x += dx[best_dir]; ghost_y += dy[best_dir];
    }
}

/* ============================================================
 * DRAWING
 * ============================================================ */

void draw_cell(unsigned char x, unsigned char y)
{
    char ch;
    unsigned char col = 0;
    goto_xy(x + 1, y + 2);
    if (x == px && y == py) {
        col = COL_YELLOW;
        switch (pdir) {
            case DIR_N: ch = '^'; break;
            case DIR_E: ch = '>'; break;
            case DIR_S: ch = 'v'; break;
            case DIR_W: ch = '<'; break;
            default:    ch = '@'; break;
        }
    } else if (x == ghost_x && y == ghost_y) {
        col = COL_RED;
        ch = 'G';
    } else if (maze[y][x] & CELL_WALL) {
        col = COL_YELLOW;
        ch = '#';
    } else if (maze[y][x] & CELL_GEM) {
        col = COL_RED;
        ch = '*';
    } else if (maze[y][x] & CELL_EXIT) {
        col = COL_YELLOW;
        ch = 'E';
    } else {
        ch = '.';
    }
    if (col) set_colour(col);
    putchar(ch);
    if (col) reset_colour();
}

void draw_map(void)
{
    unsigned char x, y;
    for (y = 0; y < MAZE_H; y++) {
        goto_xy(1, y + 2);
        for (x = 0; x < MAZE_W; x++) {
            if (x == px && y == py) {
                set_colour(COL_YELLOW);
                switch (pdir) {
                    case DIR_N: putchar('^'); break;
                    case DIR_E: putchar('>'); break;
                    case DIR_S: putchar('v'); break;
                    case DIR_W: putchar('<'); break;
                    default:    putchar('@'); break;
                }
                reset_colour();
            } else if (x == ghost_x && y == ghost_y) {
                set_colour(COL_RED);
                putchar('G');
                reset_colour();
            } else if (maze[y][x] & CELL_WALL) {
                set_colour(COL_YELLOW);
                putchar('#');
                reset_colour();
            } else if (maze[y][x] & CELL_GEM) {
                set_colour(COL_RED);
                putchar('*');
                reset_colour();
            } else if (maze[y][x] & CELL_EXIT) {
                set_colour(COL_YELLOW);
                putchar('E');
                reset_colour();
            } else {
                putchar('.');
            }
        }
    }
}

void update_map(void)
{
    draw_cell(old_px, old_py);
    draw_cell(old_gx, old_gy);
    draw_cell(px, py);
    draw_cell(ghost_x, ghost_y);
}

void draw_status(void)
{
    /* Row 19: score and level */
    print_at(1, 19, "Score:");
    print_num_at(8, 19, score);
    putchar(' '); putchar(' '); putchar(' ');

    print_at(20, 19, "Lvl:");
    print_num_at(25, 19, (int)level);
    putchar(' '); putchar(' ');

    print_at(30, 19, "Maze#");
    print_num_at(35, 19, (int)maze_seed);
    putchar(' '); putchar(' ');

    /* Row 20: energy and gems */
    print_at(1, 20, "Energy:");
    print_num_at(9, 20, energy);
    putchar(' '); putchar(' '); putchar(' ');

    print_at(20, 20, "Gems:");
    print_num_at(26, 20, (int)gems_collected);
    putchar('/');
    putchar('0' + gems_total);

    /* Row 22: controls */
    print_at(1, 22, "WASD=move P=part Q=quit");
}

/* ============================================================
 * LEVEL COMPLETE SCREEN
 * ============================================================ */

void level_complete_screen(int bonus, int energy_bonus)
{
    cls();
    puts("");
    puts("  ********************************");
    puts("  *      MAZE COMPLETE!          *");
    puts("  ********************************");
    puts("");
    printf("  Maze #%d cleared!\n", maze_seed);
    puts("");
    printf("  Score bonus:    +%d\n", bonus);
    printf("  Energy bonus:   +%d\n", energy_bonus);
    puts("");
    printf("  Total score: %d\n", score);
    printf("  Energy: %d\n", energy);
    puts("");
    printf("  Level %d complete!\n", level);
    puts("");
    puts("  Next maze loading...");
    puts("  Press any key!");
    fgetc_cons();
}

/* ============================================================
 * EARLY EXIT PROMPT
 * Shows partial score/energy offer. Returns 1 to exit now,
 * 0 to keep collecting.
 * ============================================================ */

unsigned char early_exit_prompt(void)
{
    int partial_score;
    int partial_energy;
    int key;
    unsigned char remaining;

    remaining = gems_total - gems_collected;
    partial_score = (int)gems_collected * 15;
    partial_energy = (int)gems_collected * ENERGY_PER_GEM;

    cls();
    puts("");
    puts("  ********************************");
    puts("  *     EXIT  THE  MAZE?         *");
    puts("  ********************************");
    puts("");
    printf("  Gems collected: %d / %d\n",
           gems_collected, gems_total);
    printf("  Gems remaining: %d\n", remaining);
    puts("");
    puts("  -- Leave now --");
    printf("  Score bonus:  +%d\n", partial_score);
    printf("  Energy bonus: +%d\n", partial_energy);
    puts("");
    puts("  -- Collect all gems --");
    printf("  Score bonus:  +100\n");
    printf("  Energy bonus: +%d\n",
           (int)gems_total * ENERGY_PER_GEM);
    puts("");
    puts("  Leave now? (Y=exit / N=stay)");

    while (1) {
        key = fgetc_cons();
        if (key == 'y' || key == 'Y') return 1;
        if (key == 'n' || key == 'N') return 0;
    }
}

/* ============================================================
 * TITLE SCREEN
 * Returns: 0 = play game, 1 = quit program
 * ============================================================ */

unsigned char title_screen(void)
{
    int key;
    unsigned char seed;

    cls();
    puts("");
    puts("  ==================================");
    puts("       SULTAN'S ROGUE  v0.4");
    puts("  ==================================");
    puts("");
    puts("  The Sultan's jewels lie scattered");
    puts("  deep in the ever-shifting maze.");
    puts("  The ghost of his bodyguard still");
    puts("  haunts the halls...");
    puts("");
    puts("  Collect gems & find the exit!");
    puts("  Exit early for partial rewards");
    puts("  or find all 6 for full bonus.");
    puts("  How many mazes can you clear?");
    puts("");
    puts("  W-Fwd S-Back A/D-Turn");
    puts("  P-Part hedge  H-Scores  Q-Quit");
    puts("");
    puts("  Start maze (1-255) or ENTER:");

    seed = 0;
    while (1) {
        key = fgetc_cons();
        if (key == 13 || key == 10) break;
        if (key == 'q' || key == 'Q') {
            if (confirm(24)) return 1;
            print_at(1, 24, "  Start maze (1-255) or ENTER:  ");
            continue;
        }
        if (key == 'h' || key == 'H') {
            show_high_scores();
            return title_screen();
        }
        if (key >= '0' && key <= '9') {
            putchar(key);
            seed = seed * 10 + (key - '0');
        }
    }

    if (seed == 0) {
        rng_state = 7777;
        maze_seed = 0;
        puts("");
        puts("  Random maze!");
    } else {
        rng_state = (unsigned int)seed * 257 + 1;
        maze_seed = seed;
    }

    score = 0;
    level = 1;
    energy = 500;

    puts("");
    puts("  Press any key...");
    fgetc_cons();
    return 0;
}

/* ============================================================
 * GAME OVER SCREEN
 * ============================================================ */

void gameover_screen(void)
{
    unsigned char pos;
    char name[NAME_LEN];

    cls();
    puts("");
    puts("  ********************************");
    puts("  *        GAME  OVER            *");
    puts("  ********************************");
    puts("");
    puts("  Your energy ran out!");
    printf("  Final score: %d\n", score);
    printf("  Mazes cleared: %d\n", level - 1);
    printf("  Gems this maze: %d / %d\n",
           gems_collected, gems_total);
    if (maze_seed > 0)
        printf("  Last maze: #%d\n", maze_seed);

    pos = hs_qualifies(score);
    if (pos != 255) {
        puts("");
        puts("  ** NEW HIGH SCORE! **");
        get_player_name(name);
        hs_insert(pos, name, score);
        puts("");
        puts("");
        show_high_scores();
    } else {
        puts("");
        puts("  Press any key...");
        fgetc_cons();
    }
}

/* ============================================================
 * START NEXT LEVEL
 * ============================================================ */

void start_next_level(void)
{
    unsigned char new_seed;

    do {
        new_seed = rng();
    } while (new_seed == 0);

    maze_seed = new_seed;
    rng_state = (unsigned int)new_seed * 257 + 1;

    generate_maze();
    place_items();
}

/* ============================================================
 * MAIN GAME LOOP
 * Returns: 0 = back to title, 1 = quit program
 * ============================================================ */

unsigned char game_loop(void)
{
    int key;
    unsigned char nx, ny;
    unsigned char i;
    int bonus;
    int energy_bonus;
    unsigned char hs_pos;
    char hs_buf[NAME_LEN];

    cls();
    draw_map();
    draw_status();

    while (game_running) {
        key = fgetc_cons();

        old_px = px; old_py = py;
        old_gx = ghost_x; old_gy = ghost_y;

        switch (key) {
            case 'w': case 'W':
                nx = px + dx[pdir]; ny = py + dy[pdir];
                if (!is_wall(nx, ny)) {
                    px = nx; py = ny;
                    energy -= 2;
                    ghost_timer++;
                }
                break;

            case 's': case 'S':
                nx = px - dx[pdir]; ny = py - dy[pdir];
                if (!is_wall(nx, ny)) {
                    px = nx; py = ny;
                    energy -= 2;
                    ghost_timer++;
                }
                break;

            case 'a': case 'A':
                pdir = (pdir + 3) & 3;
                energy -= 1;
                break;

            case 'd': case 'D':
                pdir = (pdir + 1) & 3;
                energy -= 1;
                break;

            case 'p': case 'P':
                nx = px + dx[pdir];
                ny = py + dy[pdir];
                if (is_wall(nx, ny) && !is_border(nx, ny)
                    && energy > 50) {
                    maze[ny][nx] = 0;
                    px = nx; py = ny;
                    energy -= 50;
                    ghost_timer++;
                    draw_cell(nx, ny);
                    print_at(1, 23,
                        "* Hedge parted! -50 *              ");
                } else if (is_border(nx, ny)) {
                    print_at(1, 23,
                        "Can't part the border!             ");
                } else if (energy <= 50) {
                    print_at(1, 23,
                        "Not enough energy!                 ");
                } else {
                    print_at(1, 23,
                        "No hedge ahead!                    ");
                }
                break;

            case 'q': case 'Q':
                if (confirm(23)) {
                    game_running = 0;
                    hs_pos = hs_qualifies(score);
                    if (hs_pos != 255) {
                        cls();
                        puts("");
                        printf("  Final score: %d\n", score);
                        puts("");
                        puts("  ** NEW HIGH SCORE! **");
                        get_player_name(hs_buf);
                        hs_insert(hs_pos, hs_buf, score);
                        puts("");
                        puts("");
                        show_high_scores();
                    }
                    return 0;
                }
                print_at(1, 23,
                    "                                       ");
                continue;

            default:
                continue;
        }

        /* Check gem pickup */
        for (i = 0; i < NUM_GEMS; i++) {
            if (!gem_taken[i] && px == gem_x[i]
                && py == gem_y[i]) {
                gem_taken[i] = 1;
                gems_collected++;
                maze[gem_y[i]][gem_x[i]] &= ~CELL_GEM;
                print_at(1, 23,
                    "** GEM FOUND! **                   ");
            }
        }

        /* Check exit */
        if (px == exit_x && py == exit_y) {
            if (gems_collected >= gems_total) {
                /* All gems - full bonus */
                bonus = 100;
                energy_bonus = (int)gems_total * ENERGY_PER_GEM;
                score += bonus;
                energy += energy_bonus;

                level_complete_screen(bonus, energy_bonus);

                level++;
                start_next_level();

                cls();
                draw_map();
                draw_status();
                continue;
            } else {
                /* Partial - offer early exit */
                if (early_exit_prompt()) {
                    /* Player chose to leave early */
                    bonus = (int)gems_collected * 15;
                    energy_bonus = (int)gems_collected
                                   * ENERGY_PER_GEM;
                    score += bonus;
                    energy += energy_bonus;

                    level_complete_screen(bonus, energy_bonus);

                    level++;
                    start_next_level();

                    cls();
                    draw_map();
                    draw_status();
                    continue;
                } else {
                    /* Player chose to stay */
                    cls();
                    draw_map();
                    draw_status();
                    print_at(1, 23,
                        "Back to the maze!                  ");
                    continue;
                }
            }
        }

        /* Move ghost every 3 player moves */
        if (ghost_timer >= 3) {
            move_ghost();
            ghost_timer = 0;
        }

        /* Ghost collision */
        if (px == ghost_x && py == ghost_y) {
            energy -= 50;
            print_at(1, 23,
                "!! GHOST !! -50 energy                 ");
            nx = px - dx[pdir]; ny = py - dy[pdir];
            if (!is_wall(nx, ny)) { px = nx; py = ny; }
        }

        /* Check energy */
        if (energy <= 0) {
            gameover_screen();
            return 0;
        }

        /* Partial map redraw */
        update_map();
        draw_status();
    }

    return 0;
}

/* ============================================================
 * MAIN
 * ============================================================ */

int main(void)
{
    init_high_scores();

    while (1) {
        if (title_screen()) break;
        generate_maze();
        place_items();
        if (game_loop()) break;
    }

    cls();
    puts("");
    puts("  Thanks for playing Sultan's Rogue!");
    printf("  Final score: %d\n", score);
    puts("");
    return 0;
}