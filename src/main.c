#include <pebble.h>

#define REPEAT_INTERVAL_MS 50
#define APP_KEY_MSG_TYPE 0
#define APP_KEY_MESSAGE = 100

static Window *s_menu_window, *s_counter_window, *s_message_window;
static TextLayer *s_text_layer, *s_message_text_layer, *s_header_layer, *s_body_layer, *s_iob_layer;
static char s_menu_text[32], s_body_text[32], s_numeric_text[8], s_message_text[32], s_iob_text[32], s_float_text[32];
static MenuLayer *s_menu_layer;
static ActionBarLayer *s_action_bar;
static float s_num_counter ;
static int s_int_key, treatmentKey;
static int menuline_count;
static int floatMultiplier = 1000 ;
static GBitmap *s_icon_plus, *s_icon_minus, *s_icon_check;
AppMessageResult appsync_err_openerr = APP_MSG_OK;



enum {
  MSG_TYPE_READY,
  MSG_TYPE_CONFIG,
  MSG_TYPE_MESSAGE,
  MSG_TYPE_QUIT,
} ;

typedef struct {
  char name[50] ;  // Name of line
  int key;  // initial value
  int initValue;  // initial value
  float inc; // increment
  float iob; // iob
} MenuInfo;

static MenuInfo menuLine;

MenuInfo menu_array[5];

static void request_pin() {
  APP_LOG(APP_LOG_LEVEL_INFO, "request pin");
  s_int_key++;
  int s_ret_key = s_int_key + 3 ;
  int s_ret_counter = s_num_counter * floatMultiplier;
  APP_LOG(APP_LOG_LEVEL_INFO, "request pin %i cnt %i", s_ret_key, s_ret_counter);
  DictionaryIterator *iter;
  appsync_err_openerr = app_message_outbox_begin(&iter);
 	if (appsync_err_openerr == 0 ) {
    dict_write_uint32(iter, s_ret_key, s_ret_counter);
    dict_write_end(iter);
    app_message_outbox_send();
  }
 
}

void setLine(DictionaryIterator *treatment) {

  if ( strcmp(dict_find(treatment, treatmentKey)->value->cstring,"") != 0 ) {
    menuline_count++ ;

    menuLine.key = treatmentKey / 100 ; 
    strncpy(menuLine.name, dict_find(treatment, treatmentKey)->value->cstring, 50);
    strncpy(s_numeric_text, dict_find(treatment, treatmentKey + 1)->value->cstring, 8);
    menuLine.initValue = atoi(s_numeric_text)  ;
    strncpy(s_numeric_text, dict_find(treatment, treatmentKey + 2)->value->cstring, 8);
    menuLine.inc = (float) atoi(s_numeric_text) / floatMultiplier;
    strncpy(s_numeric_text, dict_find(treatment, treatmentKey + 3)->value->cstring, 8);
    menuLine.iob = (float) atoi(s_numeric_text) / floatMultiplier;
    menu_array[menuline_count-1] = menuLine;
  
    APP_LOG(APP_LOG_LEVEL_INFO, "line %s ", menuLine.name);
  }
}

void set_config(DictionaryIterator *data) {
  menuline_count = 0 ; 
  Tuple *tuple = dict_find(data, 100);
  if (tuple) {  
      treatmentKey = 100;
      setLine(data); 
 }
  
  tuple = dict_find(data, 200);
  if (tuple) {  
      treatmentKey = 200;
      setLine(data); 
  }
  
  tuple = dict_find(data, 300);
  if (tuple) {  
      treatmentKey = 300;
      setLine(data); 
  }
  
  tuple = dict_find(data, 400);
  if (tuple) {  
      treatmentKey = 400;
      setLine(data); 
  }

  tuple = dict_find(data, 500);
  if (tuple) {  
      treatmentKey = 500;
      setLine(data); 
  }

 //  APP_LOG(APP_LOG_LEVEL_INFO, "float str %s", atof(s_numeric_text));
 
  
  window_stack_pop(false);
  if (menuline_count == 1 ) {
    // got to input window if only 1 menu item
    s_int_key = 0 ;
    s_num_counter = menu_array[s_int_key].initValue ;
    window_stack_push(s_counter_window, false);
  } else {
    // show menu
    window_stack_push(s_menu_window, false);
  }
}

void show_message(DictionaryIterator *data) {
  Tuple *tuple = dict_find(data, 100);
  if (tuple) {  
    strncpy(s_message_text, tuple->value->cstring, sizeof(s_menu_text));
    window_stack_push(s_message_window, false);
  }
}
    
      
static void in_received_handler(DictionaryIterator *received, void *context) {
  int msg_type = dict_find(received, APP_KEY_MSG_TYPE)->value->uint8;
  
  APP_LOG(APP_LOG_LEVEL_INFO, "msg_type %d", msg_type);
  if (msg_type == MSG_TYPE_READY) {
    APP_LOG(APP_LOG_LEVEL_INFO, "js ready");
  }
  
  if (msg_type == MSG_TYPE_CONFIG) {
    APP_LOG(APP_LOG_LEVEL_INFO, "received config" );
    set_config(received); 
  }

  if (msg_type == MSG_TYPE_MESSAGE) {
    show_message(received);
  }

  if (msg_type == MSG_TYPE_QUIT) {
    window_stack_pop_all(false);
  }
}

static uint16_t get_sections_count_callback(struct MenuLayer *menulayer, uint16_t section_index, void *callback_context) {
  return menuline_count;
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
  APP_LOG(APP_LOG_LEVEL_INFO, "%s", s_menu_text);
    
  window_stack_push(s_counter_window, false);
}

static void draw_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
    menu_cell_basic_draw(ctx, cell_layer,  menu_array[cell_index->row].name, NULL, NULL);
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
  text_layer_set_text(s_header_layer, menu_array[s_int_key].name);
  if ( menu_array[s_int_key].inc < 1 ) {
    snprintf(s_body_text, sizeof(s_body_text), "%d.%d", (int) s_num_counter , (int) (s_num_counter * 10) % 10 );
  } else {
    snprintf(s_body_text, sizeof(s_body_text), "%d", (int) s_num_counter);
  }
  text_layer_set_text(s_body_layer, s_body_text);
}

static void increment_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_num_counter = s_num_counter + menu_array[s_int_key].inc ;
  update_counter();
}

static void decrement_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_num_counter <= menu_array[s_int_key].inc  ) {
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

static void message_window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_INFO, "message load");

  Layer *window_layer = window_get_root_layer(window);

  int width = layer_get_frame(window_layer).size.w ;
  int height = layer_get_frame(window_layer).size.h / 2;

  s_message_text_layer = text_layer_create(GRect(0, height , width, 60));
  text_layer_set_font(s_message_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(s_message_text_layer, GColorClear);
  text_layer_set_text_alignment(s_message_text_layer, GTextAlignmentCenter);
  text_layer_set_text(s_message_text_layer, s_message_text);
  layer_add_child(window_layer, text_layer_get_layer(s_message_text_layer));

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
  
  s_iob_layer = text_layer_create(GRect(0, 15, width, 60));
  text_layer_set_font(s_iob_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(s_iob_layer, GColorClear);
  text_layer_set_text_alignment(s_iob_layer, GTextAlignmentCenter);
  if ( menu_array[s_int_key].iob != -1 ) {
    if ( (int) menu_array[s_int_key].iob != menu_array[s_int_key].iob ) {
      snprintf(s_float_text, sizeof(s_float_text), "%d.%d", (int) menu_array[s_int_key].iob, (int) (menu_array[s_int_key].iob * 100) % 100 );
    } else {
      snprintf(s_float_text, sizeof(s_float_text), "%d",(int) menu_array[s_int_key].iob);
    }
    snprintf(s_iob_text, sizeof(s_iob_text), "IOB: %s" , s_float_text);
  } else {
    strcpy(s_iob_text, "" );
  }
  
  text_layer_set_text(s_iob_layer, s_iob_text);
  layer_add_child(window_layer, text_layer_get_layer(s_iob_layer));
  
  s_header_layer = text_layer_create(GRect(0, height - 30, width, 60));
  text_layer_set_font(s_header_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(s_header_layer, GColorClear);
  text_layer_set_text_alignment(s_header_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_header_layer));

  s_body_layer = text_layer_create(GRect(0, height , width, 60));
  text_layer_set_font(s_body_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_background_color(s_body_layer, GColorClear);
  text_layer_set_text_alignment(s_body_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_body_layer));

  update_counter();
}

static void message_window_unload(Window *window) {
  text_layer_destroy(s_message_text_layer);
}

static void menu_window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
}

static void counter_window_unload(Window *window) {
  text_layer_destroy(s_header_layer);
  text_layer_destroy(s_body_layer);
}

static void init() {
  s_icon_plus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_PLUS_WHITE);
  s_icon_minus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_MINUS_WHITE);
  s_icon_check = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CHECK_WHITE);

  s_message_window = window_create();
  window_set_window_handlers(s_message_window, (WindowHandlers){
    .load = message_window_load,
    .unload = message_window_unload,
  });

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
  strncpy(s_message_text, "Loading...", sizeof(s_menu_text));
  window_stack_push(s_message_window, false);
  
  app_message_register_inbox_received(in_received_handler);
  app_message_open(2048, 2048);
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
