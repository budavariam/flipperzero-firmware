// #include <furi.h>
// #include <gui/gui.h>
// #include <input/input.h>
// #include <stdlib.h>
// #include <stdio.h>
// #include <unistd.h>

// typedef enum {
//     EventTypeTick,
//     EventTypeKey,
// } EventType;

// typedef struct {
//     EventType type;
//     InputEvent input;
// } PluginEvent;

// typedef struct {
// 	unsigned univ[128][64];
// } PluginState;

// static void render_callback(Canvas* const canvas, void* ctx) {
//     const PluginState* plugin_state = acquire_mutex((ValueMutex*)ctx, 25);
//     if(plugin_state == NULL) {
//         return;
//     }
//     // border around the edge of the screen
//     canvas_draw_frame(canvas, 0, 0, 128, 64);

//     canvas_set_font(canvas, FontPrimary);
//     canvas_draw_str_aligned(
//         canvas, 10, 10, AlignRight, AlignBottom, plugin_state->univ[1][1] == 0 ? "0" : "1" );

//     canvas_draw_dot(canvas, 80, 60);

//     // for (int x = 0; x < w; x++) for (int y = 0; y < h; y++) canvas_draw_dot(canvas, x, y)

//     release_mutex((ValueMutex*)ctx, plugin_state);
// }

// static void input_callback(InputEvent* input_event, FuriMessageQueue* event_queue) {
//     furi_assert(event_queue);

//     PluginEvent event = {.type = EventTypeKey, .input = *input_event};
//     furi_message_queue_put(event_queue, &event, FuriWaitForever);
// }

// static void game_of_life_state_init(PluginState* const plugin_state) {
//     int w = 128;
//     int h = 64;
//     for (int x = 0; x < w; x++) 
//         for (int y = 0; y < h; y++) 
//             plugin_state->univ[y][x] = rand() < RAND_MAX / 10 ? 1 : 0;
// }

// int32_t game_of_life_app() {
//     FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(PluginEvent));

//     PluginState* plugin_state = malloc(sizeof(PluginState));

//     game_of_life_state_init(plugin_state);

//     ValueMutex state_mutex;
//     if(!init_mutex(&state_mutex, plugin_state, sizeof(PluginState))) {
//         FURI_LOG_E("game_of_life", "cannot create mutex\r\n");
//         free(plugin_state);
//         return 255;
//     }

//     // Set system callbacks
//     ViewPort* view_port = view_port_alloc();
//     view_port_draw_callback_set(view_port, render_callback, &state_mutex);
//     view_port_input_callback_set(view_port, input_callback, event_queue);

//     // Open GUI and register view_port
//     Gui* gui = furi_record_open("gui");
//     gui_add_view_port(gui, view_port, GuiLayerFullscreen);

//     PluginEvent event;
//     for(bool processing = true; processing;) {
//         FuriStatus event_status = furi_message_queue_get(event_queue, &event, 100);

//         PluginState* plugin_state = (PluginState*)acquire_mutex_block(&state_mutex);

//         if(event_status == FuriStatusOk) {
//             // press events
//             if(event.type == EventTypeKey) {
//                 if(event.input.type == InputTypePress) {
//                     switch(event.input.key) {
//                     case InputKeyUp:
//                         // plugin_state->y--;
//                         break;
//                     case InputKeyDown:
//                         // plugin_state->y++;
//                         break;
//                     case InputKeyRight:
//                         // plugin_state->x++;
//                         break;
//                     case InputKeyLeft:
//                         // plugin_state->x--;
//                         break;
//                     case InputKeyOk:
//                     case InputKeyBack:
//                         processing = false;
//                         break;
//                     }
//                 }
//             }
//         } else {
//             FURI_LOG_D("game_of_life", "FuriMessageQueue: event timeout");
//             // event timeout
//         }

//         view_port_update(view_port);
//         release_mutex(&state_mutex, plugin_state);
//     }

//     view_port_enabled_set(view_port, false);
//     gui_remove_view_port(gui, view_port);
//     furi_record_close("gui");
//     view_port_free(view_port);
//     furi_message_queue_free(event_queue);
//     delete_mutex(&state_mutex);

//     return 0;
// }

// // void show(void *u, int w, int h)
// // {
// // 	int (*univ)[w] = u;
// // 	// printf("");
// // 	for (int y = 0; y < h; y++) {
// // 		for (int x = 0; x < w; x++) printf(univ[y][x] ? "X" : " ");
// //         printf("\n");
// // 	}
// // 	printf("---\n");
// // 	fflush(stdout);
// // }

// // void evolve(void *u, int w, int h)
// // {
// //     // inspiration: https://rosettacode.org/wiki/Conway%27s_Game_of_Life#C
// // 	unsigned (*univ)[w] = u;
// // 	unsigned new[h][w];
 
// // 	for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
// // 		int n = 0;
// // 		for (int y1 = y - 1; y1 <= y + 1; y1++)
// // 			for (int x1 = x - 1; x1 <= x + 1; x1++)
// // 				if (univ[(y1 + h) % h][(x1 + w) % w])
// // 					n++;
 
// // 		if (univ[y][x]) n--;
// // 		new[y][x] = (n == 3 || (n == 2 && univ[y][x]));
// // 	}
// // 	for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) univ[y][x] = new[y][x];
// // }