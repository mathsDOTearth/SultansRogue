# SultansRogue
A top down ASCII Rouge like based on the Amstrad CPC game Sultans Maze

## Files

`Makefile` project makefile   
`maze.c` c code  
`maze.h` function prototypes and constants  
`maze.dsk` - disk image for people who dont want to compile the code

## Game Play

### Keys are:
`A` and `D` to turn left and right.  
`W` and `S` to move forward and backwards.  
`P` part hedge - takes 50 energy  
`Q` to quit  

Collect the gems and exit the maze before your energy runs out. Hitting the ghost will drain 50 energy.

## Change Log
### v0.4
* Full-width maze (39x16).
* Early exit option with partial score/energy.
* Energy bonus = 25 per gem collected on exit.

### v0.3
* High score table (top 10, 8-char names)

### v0.2
* Continuous play through random mazes
* Scoring and bonuses
* Only redraw round player and ghost

### v0.1
* Single 16x16 maze
* maze generation
* game mechanics
