#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

typedef struct {
    uint8_t x;
    uint8_t y;
} Point;

typedef enum {
    GameStateLife,
    GameStateGameOver,
} GameState;

#define WIDTH 5
#define HEIGHT 5

typedef struct {
    uint16_t universe[HEIGHT][WIDTH];
    uint16_t len;
    GameState state;
} GameOfLifeState;

typedef enum {
    EventTypeTick,
    EventTypeKey,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} GameOfLifeEvent;

static void game_of_life_render_callback(Canvas* const canvas, void* ctx) {
    const GameOfLifeState* game_of_life_state = acquire_mutex((ValueMutex*)ctx, 25);
    if(game_of_life_state == NULL) {
        return;
    }

    // Before the function is called, the state is set with the canvas_reset(canvas)

    // Frame
    canvas_draw_frame(canvas, 0, 0, 128, 64);

    // Points
    for(uint16_t y = 0; y < HEIGHT; y++) {
        for(uint16_t x = 0; x < WIDTH; x++) {
            uint16_t state = game_of_life_state->universe[y][x];
            if (state != 0) {
                canvas_draw_box(canvas, x+1,y+1, 1, 1);
                // canvas_draw_dot(canvas, x+1, y+1);
            }
        }
    }


    // Game Over banner
    if(game_of_life_state->state == GameStateGameOver) {
        // Screen is 128x64 px
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, 34, 20, 62, 24);

        canvas_set_color(canvas, ColorBlack);
        canvas_draw_frame(canvas, 34, 20, 62, 24);

        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 37, 31, "Game Over");

        canvas_set_font(canvas, FontSecondary);
        char buffer[12];
        snprintf(buffer, sizeof(buffer), "Score: %u", game_of_life_state->len - 7);
        canvas_draw_str_aligned(canvas, 64, 41, AlignCenter, AlignBottom, buffer);
    }

    release_mutex((ValueMutex*)ctx, game_of_life_state);
}

static void game_of_life_input_callback(InputEvent* input_event, FuriMessageQueue* event_queue) {
    furi_assert(event_queue);

    GameOfLifeEvent event = {.type = EventTypeKey, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

static void game_of_life_update_timer_callback(FuriMessageQueue* event_queue) {
    furi_assert(event_queue);

    GameOfLifeEvent event = {.type = EventTypeTick};
    furi_message_queue_put(event_queue, &event, 0);
}

// static void game_of_life_evolve(void *u, int w, int h)
// {
//     // inspiration: https://rosettacode.org/wiki/Conway%27s_Game_of_Life#C
// 	unsigned (*univ)[w] = u;
// 	unsigned new[h][w];
 
// 	for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
// 		int n = 0;
// 		for (int y1 = y - 1; y1 <= y + 1; y1++)
// 			for (int x1 = x - 1; x1 <= x + 1; x1++)
// 				if (univ[(y1 + h) % h][(x1 + w) % w])
// 					n++;
 
// 		if (univ[y][x]) n--;
// 		new[y][x] = (n == 3 || (n == 2 && univ[y][x]));
// 	}
// 	for (int y = 0; y < h; y++) {
//         for (int x = 0; x < w; x++) {
//             univ[y][x] = new[y][x];
//         }
//     }
// }

static void game_of_life_init_game(GameOfLifeState* const game_of_life_state) {
    // uint16_t disp[HEIGHT][WIDTH] = {
    //     {0, 0, 0, 0, 0},
    //     {0, 1, 1, 1, 0},
    //     {0, 1, 1, 1, 0},
    //     {0, 1, 1, 1, 0},
    //     {0, 0, 0, 0, 0},
    // };
    uint16_t disp[HEIGHT][WIDTH];
    for (uint16_t y=0; y<HEIGHT; y++) {
        for (uint16_t x=0; x<WIDTH; x++) {
            disp[y][x] = rand() % 2;
            // disp[y][x] = 1;
        }
    }
    memcpy(&game_of_life_state->universe[0][0], &disp[0][0], WIDTH*HEIGHT*sizeof(disp[0][0]));
    game_of_life_state->state = GameStateLife;
}

static void game_of_life_process_game_step(GameOfLifeState* const game_of_life_state) {
    if(game_of_life_state->state == GameStateGameOver) {
        return;
    }

    for(uint16_t y = 0; y < HEIGHT; y++) {
        for(uint16_t x = 0; x < WIDTH; x++) {
            if (game_of_life_state->universe[y][x] == 0) {
                game_of_life_state->universe[y][x] = 1;
            } else {
                game_of_life_state->universe[y][x] = 0;
            }
        }
    }
}

int32_t game_of_life_app(void* p) {
    UNUSED(p);
    srand(DWT->CYCCNT);

    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(GameOfLifeEvent));

    GameOfLifeState* game_of_life_state = malloc(sizeof(GameOfLifeState));
    game_of_life_init_game(game_of_life_state);

    ValueMutex state_mutex;
    if(!init_mutex(&state_mutex, game_of_life_state, sizeof(GameOfLifeState))) {
        FURI_LOG_E("GameOfLife", "cannot create mutex\r\n");
        free(game_of_life_state);
        return 255;
    }

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, game_of_life_render_callback, &state_mutex);
    view_port_input_callback_set(view_port, game_of_life_input_callback, event_queue);

    FuriTimer* timer =
        furi_timer_alloc(game_of_life_update_timer_callback, FuriTimerTypePeriodic, event_queue);
    furi_timer_start(timer, furi_kernel_get_tick_frequency() / 4);

    // Open GUI and register view_port
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    GameOfLifeEvent event;
    for(bool processing = true; processing;) {
        FuriStatus event_status = furi_message_queue_get(event_queue, &event, 100);

        GameOfLifeState* game_of_life_state = (GameOfLifeState*)acquire_mutex_block(&state_mutex);

        if(event_status == FuriStatusOk) {
            // press events
            if(event.type == EventTypeKey) {
                if(event.input.type == InputTypePress) {
                    switch(event.input.key) {
                    case InputKeyUp:
                        break;
                    case InputKeyDown:
                        break;
                    case InputKeyRight:
                        break;
                    case InputKeyLeft:
                        break;
                    case InputKeyOk:
                        if(game_of_life_state->state == GameStateGameOver) {
                            game_of_life_init_game(game_of_life_state);
                        }
                        break;
                    case InputKeyBack:
                        processing = false;
                        break;
                    }
                }
            } else if(event.type == EventTypeTick) {
                game_of_life_process_game_step(game_of_life_state);
            }
        } else {
            // event timeout
        }

        view_port_update(view_port);
        release_mutex(&state_mutex, game_of_life_state);
    }

    furi_timer_free(timer);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    delete_mutex(&state_mutex);
    free(game_of_life_state);

    return 0;
}

// Screen is 128x64 px
// (4 + 4) * 16 - 4 + 2 + 2border == 128
// (4 + 4) * 8 - 4 + 2 + 2border == 64
// Game field from point{x:  0, y: 0} to point{x: 30, y: 14}.
// ┌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┐
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// ╎ ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪ ╎
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// ╎ ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪ ╎
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// ╎ ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪ ╎
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// ╎ ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪ ╎
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// ╎ ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪ ╎
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// ╎ ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪ ╎
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// ╎ ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪   ▪ ╎
// ╎ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ▪ ╎
// └╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┘
