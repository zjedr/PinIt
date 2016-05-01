#include <pebble.h>

#define MENU_TEXT_GAP 14
#define REPEAT_INTERVAL_MS 50

static Window *s_menu_window, *s_counter_window;
static TextLayer *s_text_layer, *s_error_text_layer, *s_counter_text_layer, *s_header_layer, *s_body_layer, *s_label_layer;
static char s_text_buffer[64], s_menu_text[32], s_body_text[32] ;
static MenuLayer *s_menu_layer;
static ActionBarLayer *s_action_bar;
static float s_num_counter ;
static int s_int_key;
static GBitmap *s_icon_plus, *s_icon_minus, *s_icon_check;AppMessageResult appsync_err_openerr = APP_MSG_OK;
bool inConnected ;

enum {
  AppKeyReady,
  AppKey1,
  AppKey2,
  AppKey3,
  AppKeyNoInternet,
  AppKeyQuit,
} AppKey;

enum {
  PERSIST_VALUE // Persistent storage key for wakeup_id
};

typedef struct {
  char name[16];  // Name of line
  int key;       // key no
  int initValue;  // initial value
  float inc; // increment
} MenuInfo;

MenuInfo menu_array[] = {
  {"Insulin", 1, 1, 0.5},
  {"Carbs", 2, 10, 5},
  {"Protein", 3, 50, 5}
};


static void request_pin() {
  APP_LOG(APP_LOG_LEVEL_INFO, "request pin");
  s_int_key++;
  s_num_counter = s_num_counter * 10;
  DictionaryIterator *iter;
  appsync_err_openerr = app_message_outbox_begin(&iter);
 	if (appsync_err_openerr == 0 ) {
    dict_write_uint32(iter, s_int_key, s_num_counter);
    dict_write_end(iter);
    app_message_outbox_send();
  }
 
}


static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *tuple = dict_find(iter, AppKeyReady);
  if (tuple) {
    inConnected = true ;
    APP_LOG(APP_LOG_LEVEL_INFO, "js ready");
    window_stack_push(s_menu_window, false);
  }
  
  tuple = dict_find(iter, AppKey1);
  if (tuple) {
    APP_LOG(APP_LOG_LEVEL_INFO, "key 1: %s", tuple->value->cstring );
  }

  tuple = dict_find(iter, AppKey2);
  if (tuple) {
    APP_LOG(APP_LOG_LEVEL_INFO, "key 2: %s", tuple->value->cstring );
  }
  tuple = dict_find(iter, AppKey3);
  if (tuple) {
    APP_LOG(APP_LOG_LEVEL_INFO, "key 3: %s", tuple->value->cstring );
  }
  tuple = dict_find(iter, AppKeyNoInternet);
  if (tuple) {
    inConnected = false ;
    APP_LOG(APP_LOG_LEVEL_INFO, "key 4" );
    APP_LOG(APP_LOG_LEVEL_INFO, "data: %s", tuple->value->cstring );
  }

  tuple = dict_find(iter, AppKeyQuit);
  if (tuple) {
    window_stack_pop_all(false);
  }
}

static uint16_t get_sections_count_callback(struct MenuLayer *menulayer, uint16_t section_index, void *callback_context) {
  int count = sizeof(menu_array) / sizeof(MenuInfo);
  return count;
}

#ifdef PBL_ROUND
static int16_t get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  return 60;
}
#endif

static void select_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index, void *callback_context) {

  s_int_key = menu_array[cell_index->row].key - 1 ;
  s_num_counter = menu_array[s_int_key].initValue ;
  snprintf(s_menu_text, sizeof(s_menu_text),"selected %i ", s_int_key );
  APP_LOG(APP_LOG_LEVEL_INFO, s_menu_text);
    
  window_stack_push(s_counter_window, false);
}

static void draw_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
  char* name = menu_array[cell_index->row].name;

  int text_gap_size = MENU_TEXT_GAP - strlen(name);

  snprintf(s_menu_text, sizeof(s_menu_text), "%s%*s", PBL_IF_ROUND_ELSE("", name), PBL_IF_ROUND_ELSE(0, text_gap_size), "");
  menu_cell_basic_draw(ctx, cell_layer, PBL_IF_ROUND_ELSE(name, s_menu_text), PBL_IF_ROUND_ELSE(s_menu_text, NULL), NULL);
}

static void menu_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = get_sections_count_callback,
    .get_cell_height = PBL_IF_ROUND_ELSE(get_cell_height_callback, NULL),
    .draw_row = draw_row_handler,
    .select_click = select_callback
  }); 
  menu_layer_set_click_config_onto_window(s_menu_layer,	window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

}

static void update_counter() {
  static char s_body_text[18];
  text_layer_set_text(s_header_layer, menu_array[s_int_key].name);
  int show_int = s_num_counter ;
  if (menu_array[s_int_key].inc < 1 ) {
    float s_num_dec = s_num_counter - show_int ;
    int show_dec = s_num_dec * 10;
    snprintf(s_body_text, sizeof(s_body_text), "%d.%d", show_int, show_dec);
  } else {
    snprintf(s_body_text, sizeof(s_body_text), "%d", show_int);
  }
  text_layer_set_text(s_body_layer, s_body_text);
}

static void increment_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "+");
  s_num_counter = s_num_counter + menu_array[s_int_key].inc ;
  update_counter();
}

static void decrement_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "-");
  if (s_num_counter <= 0) {
    // Keep the counter at zero
    return;
  }

  s_num_counter = s_num_counter - menu_array[s_int_key].inc ;
  update_counter();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Request pin");
  request_pin();
}

static void click_config_provider(void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "click config");
  window_single_repeating_click_subscribe(BUTTON_ID_UP, REPEAT_INTERVAL_MS, increment_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, REPEAT_INTERVAL_MS, decrement_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_SELECT, REPEAT_INTERVAL_MS, select_click_handler);
}

static void counter_window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_INFO, "counter load");

  Layer *window_layer = window_get_root_layer(window);

  s_action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(s_action_bar, window);
  action_bar_layer_set_click_config_provider(s_action_bar, click_config_provider);

  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_UP, s_icon_plus);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_DOWN, s_icon_minus);
  action_bar_layer_set_icon(s_action_bar, BUTTON_ID_SELECT, s_icon_check);

  int width = layer_get_frame(window_layer).size.w - ACTION_BAR_WIDTH - 3;
  int height = layer_get_frame(window_layer).size.h / 2;
  
  s_header_layer = text_layer_create(GRect(0, height - 30, width, 60));
  text_layer_set_font(s_header_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_background_color(s_header_layer, GColorClear);
  text_layer_set_text_alignment(s_header_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_header_layer));

  s_body_layer = text_layer_create(GRect(0, height , width, 60));
  text_layer_set_font(s_body_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(s_body_layer, GColorClear);
  text_layer_set_text_alignment(s_body_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_body_layer));

  update_counter();
}

static void menu_window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  text_layer_destroy(s_error_text_layer);
}

static void counter_window_unload(Window *window) {
  text_layer_destroy(s_header_layer);
  text_layer_destroy(s_body_layer);
}

static void init() {
  s_icon_plus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_PLUS_WHITE);
  s_icon_minus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_MINUS_WHITE);
  s_icon_check = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CHECK_WHITE);

  s_menu_window = window_create();
  window_set_window_handlers(s_menu_window, (WindowHandlers){
    .load = menu_window_load,
    .unload = menu_window_unload,
  });

  s_counter_window = window_create();
  window_set_window_handlers(s_counter_window, (WindowHandlers){
    .load = counter_window_load,
    .unload = counter_window_unload,
  });

 window_stack_push(s_menu_window, false);
  
  app_message_register_inbox_received(in_received_handler);
  app_message_open(256, 256);
}

static void deinit() {
  window_destroy(s_menu_window);
  window_destroy(s_counter_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
