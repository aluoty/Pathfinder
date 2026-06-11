#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#define GRID_ROWS 10
#define GRID_COLS 10
#define MIN_CELL 30.0f
#define MAX_CELL 80.0f
#define DEFAULT_CELL 50.0f
#define MIN_STEP_MS 50
#define MAX_STEP_MS 800
#define DEFAULT_STEP_MS 200
#define PANEL_HEIGHT 130

#define EMPTY 0
#define WALL 1
#define PATH 3

// High-performance Heap Node
typedef struct {
    int x;
    int y;
    int g;
    int f;
} Node;

// Fixed-allocation Priority Queue Matrix structure
typedef struct {
    Node data[GRID_ROWS * GRID_COLS * 2];
    int size;
} PriorityQueue;

// --- Priority Queue Operations ---
void pq_push(PriorityQueue *pq, Node node) {
    int i = pq->size++;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (pq->data[p].f < node.f || (pq->data[p].f == node.f && pq->data[p].g >= node.g)) break;
        pq->data[i] = pq->data[p];
        i = p;
    }
    pq->data[i] = node;
}

Node pq_pop(PriorityQueue *pq) {
    Node res = pq->data[0];
    Node node = pq->data[--pq->size];
    int i = 0;
    while (i * 2 + 1 < pq->size) {
        int left = i * 2 + 1;
        int right = i * 2 + 2;
        int child = left;
        if (right < pq->size && (pq->data[right].f < pq->data[left].f || 
           (pq->data[right].f == pq->data[left].f && pq->data[right].g > pq->data[left].g))) {
            child = right;
        }
        if (node.f < pq->data[child].f || (node.f == pq->data[child].f && node.g >= pq->data[child].g)) break;
        pq->data[i] = pq->data[child];
        i = child;
    }
    pq->data[i] = node;
    return res;
}

// --- Global Context State Variables ---
uint8_t grid[GRID_ROWS][GRID_COLS];
bool visited[GRID_ROWS][GRID_COLS];
int parent_x[GRID_ROWS][GRID_COLS];
int parent_y[GRID_ROWS][GRID_COLS];
int dist[GRID_ROWS][GRID_COLS];

int player_x = 0, player_y = 0;
int goal_x = GRID_ROWS - 1, goal_y = GRID_COLS - 1;

bool use_astar = true;
bool ai_done = false;
float cell_size = DEFAULT_CELL;
float grid_offset_x = 0.0f;
float grid_offset_y = 0.0f;
int step_delay_ms = DEFAULT_STEP_MS;
double last_step_time = 0.0;

PriorityQueue pq;

// Manhattan Distance Heuristic Calculation Engine
int get_heuristic(int x, int y) {
    return abs(x - goal_x) + abs(y - goal_y);
}

void init_ai(void) {
    pq.size = 0;
    ai_done = false;
    
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            visited[r][c] = false;
            parent_x[r][c] = -1;
            parent_y[r][c] = -1;
            dist[r][c] = 1e9; 
            // Wipes old visual path calculations but preserves WALL structures completely
            if (grid[r][c] == PATH) grid[r][c] = EMPTY;
        }
    }

    // Anchor the pathfinder to dynamically update from wherever the player is standing
    dist[player_x][player_y] = 0;
    Node start = { player_x, player_y, 0, get_heuristic(player_x, player_y) };
    pq_push(&pq, start);
    last_step_time = GetTime();
}

void draw_path(void) {
    int cx = goal_x;
    int cy = goal_y;
    while (cx != player_x || cy != player_y) {
        if (grid[cx][cy] != WALL) grid[cx][cy] = PATH;
        int px = parent_x[cx][cy];
        int py = parent_y[cx][cy];
        if (px == -1 || py == -1) break;
        cx = px;
        cy = py;
    }
}

void step_ai(void) {
    if (ai_done || pq.size == 0) {
        ai_done = true;
        return;
    }

    Node cur = pq_pop(&pq);

    if (visited[cur.x][cur.y]) return;
    visited[cur.x][cur.y] = true;

    if (cur.x == goal_x && cur.y == goal_y) {
        ai_done = true;
        draw_path();
        return;
    }

    const int DX[4] = {1, -1, 0, 0};
    const int DY[4] = {0, 0, 1, -1};

    for (int d = 0; d < 4; d++) {
        int nx = cur.x + DX[d];
        int ny = cur.y + DY[d];

        if (nx < 0 || ny < 0 || nx >= GRID_ROWS || ny >= GRID_COLS) continue;
        if (grid[nx][ny] == WALL) continue;

        int new_g = cur.g + 1;
        if (new_g < dist[nx][ny]) {
            dist[nx][ny] = new_g;
            int f = use_astar ? (new_g + get_heuristic(nx, ny)) : new_g;
            
            Node neighbor = { nx, ny, new_g, f };
            pq_push(&pq, neighbor);
            
            parent_x[nx][ny] = cur.x;
            parent_y[nx][ny] = cur.y;
        }
    }
}

void toggle_wall_at_mouse(void) {
    Vector2 mouse = GetMousePosition();
    int col = (int)((mouse.x - grid_offset_x) / cell_size);
    int row = (int)((mouse.y - grid_offset_y) / cell_size);

    if (row >= 0 && col >= 0 && row < GRID_ROWS && col < GRID_COLS) {
        if ((row == player_x && col == player_y) || (row == goal_x && col == goal_y)) return;
        grid[row][col] = (grid[row][col] == WALL) ? EMPTY : WALL;
        init_ai(); 
    }
}

Color get_cell_color(int r, int c) {
    if (r == player_x && c == player_y) return (Color){ 0, 100, 255, 255 };
    if (r == goal_x && c == goal_y)      return (Color){ 220, 40, 40, 255 };
    if (grid[r][c] == WALL)              return (Color){ 20, 20, 20, 255 };
    if (grid[r][c] == PATH)              return (Color){ 255, 220, 0, 255 };
    if (visited[r][c])                   return (Color){ 0, 210, 210, 255 };
    return (Color){ 245, 245, 245, 255 };
}

int main(void) {
    int initial_width = (int)(GRID_COLS * DEFAULT_CELL);
    int initial_height = (int)(GRID_ROWS * DEFAULT_CELL) + PANEL_HEIGHT;

    InitWindow(initial_width, initial_height, "Pathfinder — A* vs Dijkstra");
    SetTargetFPS(60);

    for(int r=0; r<GRID_ROWS; r++) {
        for(int c=0; c<GRID_COLS; c++) grid[r][c] = EMPTY;
    }
    init_ai();

    while (!WindowShouldClose()) {
        // Dynamic Window Resizing Layout Math
        int current_w = GetScreenWidth();
        int current_h = GetScreenHeight();
        float grid_area_h = (float)current_h - PANEL_HEIGHT;
        float cell_w = (float)current_w / GRID_COLS;
        float cell_h = grid_area_h / GRID_ROWS;
        cell_size = (cell_w < cell_h) ? cell_w : cell_h;
        if (cell_size < MIN_CELL) cell_size = MIN_CELL;
        if (cell_size > MAX_CELL) cell_size = MAX_CELL;

        float grid_w = GRID_COLS * cell_size;
        float grid_h = GRID_ROWS * cell_size;
        grid_offset_x = ((float)current_w - grid_w) * 0.5f;
        grid_offset_y = (grid_area_h - grid_h) * 0.5f;
        if (grid_offset_y < 0.0f) grid_offset_y = 0.0f;

        // Interactive Mouse Controls
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) toggle_wall_at_mouse();

        // Player movement does not restart an in-progress search (use R to reset)
        if (IsKeyPressed(KEY_UP)    || IsKeyPressed(KEY_W)) { if (player_x > 0 && grid[player_x-1][player_y] != WALL) player_x--; }
        if (IsKeyPressed(KEY_DOWN)  || IsKeyPressed(KEY_S)) { if (player_x < GRID_ROWS-1 && grid[player_x+1][player_y] != WALL) player_x++; }
        if (IsKeyPressed(KEY_LEFT)  || IsKeyPressed(KEY_A)) { if (player_y > 0 && grid[player_x][player_y-1] != WALL) player_y--; }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) { if (player_y < GRID_COLS-1 && grid[player_x][player_y+1] != WALL) player_y++; }

        if (IsKeyPressed(KEY_M)) { use_astar = !use_astar; init_ai(); }
        if (IsKeyPressed(KEY_R)) { init_ai(); }
        if (IsKeyPressed(KEY_C)) { 
            for(int r=0; r<GRID_ROWS; r++) {
                for(int c=0; c<GRID_COLS; c++) grid[r][c] = EMPTY;
            }
            init_ai();
        }

        if (IsKeyPressed(KEY_LEFT_BRACKET))  { step_delay_ms = (step_delay_ms + 50 > MAX_STEP_MS) ? MAX_STEP_MS : step_delay_ms + 50; }
        if (IsKeyPressed(KEY_RIGHT_BRACKET)) { step_delay_ms = (step_delay_ms - 50 < MIN_STEP_MS) ? MIN_STEP_MS : step_delay_ms - 50; }

        // Time Engine Tick Synchronization Loop (handles natural automation step progression)
        double current_time = GetTime();
        if (!ai_done && (current_time - last_step_time) >= ((double)step_delay_ms / 1000.0)) {
            step_ai();
            last_step_time = current_time;
        }

        SetWindowTitle(TextFormat("Pathfinder — %s | cell %dpx | step %dms", 
                       use_astar ? "A*" : "Dijkstra", (int)cell_size, step_delay_ms));

        BeginDrawing();
            ClearBackground((Color){ 47, 79, 79, 255 }); 

            // Render Node Grid Network
            for (int r = 0; r < GRID_ROWS; r++) {
                for (int c = 0; c < GRID_COLS; c++) {
                    float x = grid_offset_x + c * cell_size;
                    float y = grid_offset_y + r * cell_size;
                    DrawRectangleRounded((Rectangle){ x + 1, y + 1, cell_size - 2, cell_size - 2 }, 0.1f, 4, get_cell_color(r, c));
                    DrawRectangleRoundedLines((Rectangle){ x + 1, y + 1, cell_size - 2, cell_size - 2 }, 0.1f, 4, (Color){ 160, 160, 160, 255 });
                }
            }

            // HUD panel anchored to window bottom, laid out from window width
            const float hud_pad = 16.0f;
            const float row_h = 26.0f;
            const float row_gap = 10.0f;
            float panel_y = (float)current_h - PANEL_HEIGHT;
            float content_w = (float)current_w - hud_pad * 2.0f;

            DrawRectangle(0, (int)panel_y, current_w, PANEL_HEIGHT, (Color){ 45, 55, 65, 255 });
            DrawLine(0, (int)panel_y, current_w, (int)panel_y, (Color){ 70, 85, 100, 255 });

            float row0_y = panel_y + hud_pad;
            float mode_w = 110.0f;
            Color mode_color = use_astar ? (Color){ 100, 180, 255, 255 } : (Color){ 180, 130, 255, 255 };
            DrawRectangleRounded((Rectangle){ hud_pad, row0_y, mode_w, row_h }, 0.2f, 4, mode_color);
            const char *mode_text = use_astar ? "A*" : "Dijkstra";
            int mode_tw = MeasureText(mode_text, 14);
            DrawText(mode_text, (int)(hud_pad + (mode_w - mode_tw) * 0.5f), (int)(row0_y + 6), 14, BLACK);

            float delay_label_x = hud_pad + mode_w + 14.0f;
            DrawText("Delay", (int)delay_label_x, (int)(row0_y + 7), 12, LIGHTGRAY);
            float delay_bar_x = delay_label_x + 46.0f;
            float delay_bar_w = content_w - mode_w - 14.0f - 46.0f;
            float speed_fill = ((float)step_delay_ms / MAX_STEP_MS) * delay_bar_w;
            if (speed_fill < 4.0f) speed_fill = 4.0f;
            DrawRectangleRounded((Rectangle){ delay_bar_x, row0_y + 4, delay_bar_w, row_h - 8 }, 0.15f, 4, (Color){ 35, 45, 55, 255 });
            DrawRectangleRounded((Rectangle){ delay_bar_x, row0_y + 4, speed_fill, row_h - 8 }, 0.15f, 4, (Color){ 80, 200, 120, 255 });

            float row1_y = row0_y + row_h + row_gap;
            DrawText("Cell size", (int)hud_pad, (int)(row1_y + 7), 12, LIGHTGRAY);
            float scale_bar_x = hud_pad + 72.0f;
            float scale_bar_w = content_w - 72.0f;
            float scale_fill = ((cell_size - MIN_CELL) / (MAX_CELL - MIN_CELL)) * scale_bar_w;
            if (scale_fill < 4.0f) scale_fill = 4.0f;
            DrawRectangleRounded((Rectangle){ scale_bar_x, row1_y + 4, scale_bar_w, row_h - 8 }, 0.15f, 4, (Color){ 35, 45, 55, 255 });
            DrawRectangleRounded((Rectangle){ scale_bar_x, row1_y + 4, scale_fill, row_h - 8 }, 0.15f, 4, (Color){ 255, 160, 80, 255 });

            float row2_y = row1_y + row_h + row_gap;
            Color legend_colors[5] = {
                { 0, 100, 255, 255 },
                { 220, 40, 40, 255 },
                { 255, 220, 0, 255 },
                { 0, 210, 210, 255 },
                { 20, 20, 20, 255 }
            };
            const char *legend_labels[5] = { "Player", "Goal", "Path", "Visited", "Wall" };
            for (int i = 0; i < 5; i++) {
                float lx = hud_pad + i * 62.0f;
                DrawRectangleRounded((Rectangle){ lx, row2_y, 16, 16 }, 0.2f, 4, legend_colors[i]);
                DrawText(legend_labels[i], (int)(lx + 22), (int)(row2_y + 2), 11, LIGHTGRAY);
            }
            DrawText("WASD: Move  |  M: Algo  |  R: Reset  |  C: Clear  |  [ ]: Speed", (int)hud_pad, (int)(row2_y + 24), 11, (Color){ 180, 180, 180, 255 });

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
