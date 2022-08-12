#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>


typedef enum {
    GameStateLife,
    GameStateGameOver,
} GameState;

#define WIDTH 31
#define HEIGHT 15

typedef struct {
    uint8_t universe[HEIGHT][WIDTH];
    uint8_t iteration;
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
    for(uint8_t y = 0; y < HEIGHT; y++) {
        for(uint8_t x = 0; x < WIDTH; x++) {
            uint8_t state = game_of_life_state->universe[y][x];
            if (state != 0) {
                canvas_draw_box(canvas, 2+4*x,2+4*y, 4, 4);
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
        char buffer[15];
        snprintf(buffer, sizeof(buffer), "Iteration: %u", game_of_life_state->iteration);
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

static bool game_of_life_evolve(void *u)
{
    // inspiration: https://rosettacode.org/wiki/Conway%27s_Game_of_Life#C
	uint8_t (*univ)[WIDTH] = u;
	uint8_t new[HEIGHT][WIDTH];
    uint8_t cntAlive = 0;

	for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t x = 0; x < WIDTH; x++) {
		    uint8_t n = 0;
            for (uint8_t y1 = y - 1; y1 <= y + 1; y1++) {
                for (uint8_t x1 = x - 1; x1 <= x + 1; x1++)
                    if (univ[(y1 + HEIGHT) % HEIGHT][(x1 + WIDTH) % WIDTH]) {
                        n++;
                    }
            }
            if (univ[y][x]) {
                n--;
            }
		    new[y][x] = (n == 3 || (n == 2 && univ[y][x]));
        }
    }
	for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t x = 0; x < WIDTH; x++) {
            if (new[y][x] > 0) {
                cntAlive++;
            } 
            univ[y][x] = new[y][x];
        }
    }
    return cntAlive == 0;
}

static void game_of_life_init_game(GameOfLifeState* const game_of_life_state) {
    uint8_t disp[HEIGHT][WIDTH];
    for (uint8_t y=0; y<HEIGHT; y++) {
        for (uint8_t x=0; x<WIDTH; x++) {
            disp[y][x] = rand() % 2;
        }
    }
    memcpy(&game_of_life_state->universe[0][0], &disp[0][0], WIDTH*HEIGHT*sizeof(disp[0][0]));
    game_of_life_state->state = GameStateLife;
    game_of_life_state->iteration = 0;
}

static void game_of_life_process_game_step(GameOfLifeState* const game_of_life_state) {
    if(game_of_life_state->state == GameStateGameOver) {
        return;
    }

    bool isOver = game_of_life_evolve(game_of_life_state->universe);
    if (isOver) {
        game_of_life_state->state = GameStateGameOver;
    }
    game_of_life_state->iteration = game_of_life_state->iteration+1 % 256 ;
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
    furi_timer_start(timer, furi_kernel_get_tick_frequency() / 2); // the larger the number the slower it is

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
                        game_of_life_state->state = GameStateGameOver;
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
