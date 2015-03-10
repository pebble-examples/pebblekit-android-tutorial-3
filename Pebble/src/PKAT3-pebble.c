#include <pebble.h>

#define KEY_CHOICE      0
#define CHOICE_ROCK     0
#define CHOICE_PAPER    1
#define CHOICE_SCISSORS 2
#define CHOICE_WAITING  3

#define KEY_RESULT  1
#define RESULT_LOSE 0
#define RESULT_WIN  1
#define RESULT_TIE  2

static Window *s_main_window;
static BitmapLayer *s_choice_layer;
static GBitmap *s_choice_bitmap;
static MenuLayer *s_choice_menu;
static TextLayer *s_result_layer;

static int s_choice = CHOICE_WAITING;
static int s_win_counter, s_game_counter;

static void timer_handler(void *context) {
  // Allow user to choose once more
  layer_set_hidden(menu_layer_get_layer(s_choice_menu), false);
  layer_set_hidden(text_layer_get_layer(s_result_layer), true);

  s_choice = CHOICE_WAITING;
}

static void received_handler(DictionaryIterator *iter, void *context) {
  Tuple *t = dict_read_first(iter);

  while(t != NULL) {
    if(t->key == KEY_RESULT) {
      // Display result to player
      layer_set_hidden(menu_layer_get_layer(s_choice_menu), true);
      layer_set_hidden(text_layer_get_layer(s_result_layer), false);

      // Remember how many games have been played
      s_game_counter++;

      switch(t->value->int32) {
        case RESULT_WIN:
          // Remember how many wins in this session
          s_win_counter++;

          static char s_win_buffer[32];
          snprintf(s_win_buffer, sizeof(s_win_buffer), "You win! (%d of %d)", s_win_counter, s_game_counter);
          text_layer_set_text(s_result_layer, s_win_buffer);
          break;
        case RESULT_LOSE:
          text_layer_set_text(s_result_layer, "You lose!");
          break;
        case RESULT_TIE:
          text_layer_set_text(s_result_layer, "It's a tie!");
          break;
      }

      // Display for 5 seconds
      app_timer_register(5000, timer_handler, NULL);

      // Go back to low-power mode
      app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
    }

    // Finally
    t = dict_read_next(iter);
  }
}

static void send(int key, int msg) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int(iter, key, &msg, sizeof(int), true);
  app_message_outbox_send();
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  switch(cell_index->row) {
    case 0:
      if(s_choice == CHOICE_ROCK) {
        menu_cell_basic_draw(ctx, cell_layer, "> ROCK", NULL, NULL);
      } else {
        menu_cell_basic_draw(ctx, cell_layer, "ROCK", NULL, NULL);
      }
      break;
    case 1:
      if(s_choice == CHOICE_PAPER) {
        menu_cell_basic_draw(ctx, cell_layer, "> PAPER", NULL, NULL);
      } else {
        menu_cell_basic_draw(ctx, cell_layer, "PAPER", NULL, NULL);
      }
      break;
    case 2:
      if(s_choice == CHOICE_SCISSORS) {
        menu_cell_basic_draw(ctx, cell_layer, "> SCISSORS", NULL, NULL);
      } else {
        menu_cell_basic_draw(ctx, cell_layer, "SCISSORS", NULL, NULL);
      }
      break;
  }
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return 25;
}

static uint16_t get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return 3;
}

static void select_click_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  // Only allow choice when waiting for one
  if(s_choice == CHOICE_WAITING) {
    switch(cell_index->row) {
      case 0:
        // Chose rock
        s_choice = CHOICE_ROCK;

        gbitmap_destroy(s_choice_bitmap);
        s_choice_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ROCK);
        bitmap_layer_set_bitmap(s_choice_layer, s_choice_bitmap);
        break;
      case 1:
        // Chose paper
        s_choice = CHOICE_PAPER;

        gbitmap_destroy(s_choice_bitmap);
        s_choice_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PAPER);
        bitmap_layer_set_bitmap(s_choice_layer, s_choice_bitmap);
        break;
      case 2:
        // Chose scissors
        s_choice = CHOICE_SCISSORS;

        gbitmap_destroy(s_choice_bitmap);
        s_choice_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SCISSORS);
        bitmap_layer_set_bitmap(s_choice_layer, s_choice_bitmap);
        break;
    }
    
    // Lock in choice
    send(KEY_CHOICE, s_choice);
    menu_layer_reload_data(s_choice_menu);

    // Prepare for result
    app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
  }
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // Init to unknown
  s_choice_layer = bitmap_layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h - 76));
  s_choice_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UNKNOWN);
  bitmap_layer_set_bitmap(s_choice_layer, s_choice_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_choice_layer));

  // MenuLayer for selection - half the screen
  s_choice_menu = menu_layer_create(GRect(0, 75, window_bounds.size.w, window_bounds.size.h - 75));
  menu_layer_set_callbacks(s_choice_menu, NULL, (MenuLayerCallbacks) {
    .draw_row = draw_row_callback,
    .get_cell_height = get_cell_height_callback,
    .get_num_rows = get_num_rows_callback,
    .select_click = select_click_callback,
  });
  menu_layer_set_click_config_onto_window(s_choice_menu, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_choice_menu));

  // TextLayer to show result of matches
  s_result_layer = text_layer_create(GRect(0, 90, 144, 30));
  text_layer_set_font(s_result_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_result_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_result_layer));
  layer_set_hidden(text_layer_get_layer(s_result_layer), true);
}

static void main_window_unload(Window *window) {
  bitmap_layer_destroy(s_choice_layer);
  gbitmap_destroy(s_choice_bitmap);

  //Finally
  window_destroy(window);
}

static void init(void) {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  app_message_register_inbox_received(received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit(void) {

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
