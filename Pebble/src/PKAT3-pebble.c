#include <pebble.h>

static Window *s_main_window;
static BitmapLayer *s_choice_layer;
static GBitmap *s_choice_bitmap;
static MenuLayer *s_choice_menu;
static TextLayer *s_result_layer;

static int s_choice = CHOICE_WAITING;
static int s_win_counter, s_game_counter;

static void ui_show_weapon_selector(bool show) {
  layer_set_hidden(menu_layer_get_layer(s_choice_menu), !show);
  layer_set_hidden(text_layer_get_layer(s_result_layer), show);

  if(show) {
    gbitmap_destroy(s_choice_bitmap);
    s_choice_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UNKNOWN);
    bitmap_layer_set_bitmap(s_choice_layer, s_choice_bitmap);
  }
}

static void ui_update_weapon(int row) {
  gbitmap_destroy(s_choice_bitmap);

  switch(row) {
    case 0:
      // Chose rock
      s_choice = CHOICE_ROCK;
      s_choice_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ROCK);
      break;
    case 1:
      // Chose paper
      s_choice = CHOICE_PAPER;
      s_choice_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PAPER);
      break;
    case 2:
      // Chose scissors
      s_choice = CHOICE_SCISSORS;
      s_choice_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SCISSORS);
      break;
  }
  
  bitmap_layer_set_bitmap(s_choice_layer, s_choice_bitmap);
}

static void timer_handler(void *context) {
  // Allow user to choose once more
  ui_show_weapon_selector(true);

  s_choice = CHOICE_WAITING;
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
    // Update UI with choice
    ui_update_weapon(cell_index->row);
    
    // Lock in choice
    menu_layer_reload_data(s_choice_menu);
  }
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_choice_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UNKNOWN);
  GRect bitmap_bounds = gbitmap_get_bounds(s_choice_bitmap);

  // Initialize to 'unknown' state
  const GEdgeInsets choice_layer_insets = {.bottom = bounds.size.h - bitmap_bounds.size.h};
  s_choice_layer = bitmap_layer_create(grect_inset(bounds, choice_layer_insets));
  bitmap_layer_set_bitmap(s_choice_layer, s_choice_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_choice_layer));

  // MenuLayer for selection - half the screen
  const GEdgeInsets menu_insets = {.top = bitmap_bounds.size.h};
  s_choice_menu = menu_layer_create(grect_inset(bounds, menu_insets));  
  menu_layer_set_callbacks(s_choice_menu, NULL, (MenuLayerCallbacks) {
    .draw_row = draw_row_callback,
    .get_cell_height = get_cell_height_callback,
    .get_num_rows = get_num_rows_callback,
    .select_click = select_click_callback,
  });
  menu_layer_set_click_config_onto_window(s_choice_menu, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_choice_menu));

  // TextLayer to show result of matches
  const GEdgeInsets result_insets = {.top = 90};
  s_result_layer = text_layer_create(grect_inset(bounds, result_insets));
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
}

static void deinit(void) {

}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
