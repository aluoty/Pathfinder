# Pathfinder

A visual pathfinding demo comparing **A\*** and **Dijkstra** on a 10×10 grid. Built with [raylib](https://www.raylib.com/).

## Build

Requires raylib and a C compiler.

```bash
make
./pathfinder
```

## Controls

| Input | Action |
|-------|--------|
| **W** / **↑** | Move player up |
| **S** / **↓** | Move player down |
| **A** / **←** | Move player left |
| **D** / **→** | Move player right |
| **Left click** | Toggle wall on a cell |
| **M** | Switch between A\* and Dijkstra (restarts search) |
| **R** | Reset search |
| **C** | Clear grid (removes all walls) |
| **[** | Slow down step delay |
| **]** | Speed up step delay |

Moving the player during an in-progress search does not restart the algorithm. Use **R** to run a new search from the current position.

Resize the window to change cell size automatically.

## Legend

| Color | Meaning |
|-------|---------|
| Blue | Player (start) |
| Red | Goal |
| Yellow | Final path |
| Cyan | Visited cells |
| Black | Walls |
