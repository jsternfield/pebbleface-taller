#include <pebble.h>
#include <pebble-owm-weather/owm-weather.h>
#include <pebble-events/pebble-events.h>

#include "main.h"


static Window *window;
static Layer* canvas;

static GBitmap* time_digits_images[TOTAL_TIME_DIGITS];
static BitmapLayer* time_digits_layers[TOTAL_TIME_DIGITS];

const int DIGIT_RESOURCE_IDS[] = {
  RESOURCE_ID_0,
  RESOURCE_ID_1,
  RESOURCE_ID_2,
  RESOURCE_ID_3,
  RESOURCE_ID_4,
  RESOURCE_ID_5,
  RESOURCE_ID_6,
  RESOURCE_ID_7,
  RESOURCE_ID_8,
  RESOURCE_ID_9
};

const int DIGIT_RESOURCE_IDS_S[] = {
  RESOURCE_ID_0S,
  RESOURCE_ID_1S,
  RESOURCE_ID_2S,
  RESOURCE_ID_3S,
  RESOURCE_ID_4S,
  RESOURCE_ID_5S,
  RESOURCE_ID_6S,
  RESOURCE_ID_7S,
  RESOURCE_ID_8S,
  RESOURCE_ID_9S
};

static void render(Layer *layer, GContext* ctx) 
{
    graphics_context_set_text_color(ctx, GColorWhite);

    GPoint dayPoint = mBigHour ? mDayPoint : mDayPointAlt;
    graphics_draw_text(ctx, mDayText, mFont, GRect(dayPoint.x, dayPoint.y, SCREEN_W2, SCREEN_H), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin, bool isBig)
{
    GBitmap *old_image = *bmp_image;
    *bmp_image = isBig ? gbitmap_create_with_resource(DIGIT_RESOURCE_IDS[resource_id]) : gbitmap_create_with_resource(DIGIT_RESOURCE_IDS_S[resource_id]);

    GRect frame = isBig ? GRect(origin.x, origin.y, mImageSizeBig.x, mImageSizeBig.y) : GRect(origin.x, origin.y, mImageSizeSm.x, mImageSizeSm.y);
  
    bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
    layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);
    gbitmap_destroy(old_image);
}

unsigned short get_display_hour(unsigned short hour)
{
  if (clock_is_24h_style())
  {
    return hour;
  }

  unsigned short display_hour = hour % 12;

  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{
    if (units_changed & DAY_UNIT)
    {
        // Get the day and convert text uppercase
        strftime(mDayText, mDayTextSize, "%A", tick_time);
        for (int i = 0; mDayText[i] != 0; i++)
        {
            if (mDayText[i] >= 'a' && mDayText[i] <= 'z')
            {
                mDayText[i] -= 0x20;
            }
        }
    }

    if (units_changed & HOUR_UNIT)
    {
        unsigned short display_hour = get_display_hour(tick_time->tm_hour);

        GPoint hourPoint = mBigHour ? GPoint(DIGIT_X1, DIGIT_Y) : GPoint(DIGIT_X1_S-SCREEN_W2-BHOFFSET, DIGIT_Y);;
        set_container_image(&time_digits_images[0], time_digits_layers[0], display_hour/10, hourPoint, mBigHour);
        hourPoint.x = mBigHour ? DIGIT_X2 : DIGIT_X2_S-SCREEN_W2-BHOFFSET;
        set_container_image(&time_digits_images[1], time_digits_layers[1], display_hour%10, hourPoint, mBigHour);

        if (!clock_is_24h_style())
        {
            if (display_hour / 10 == 0)
            {
                layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), true);
            }
            else
            {
                layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), false);
            }
        }
    }
    if (units_changed & MINUTE_UNIT)
    {
        GPoint minutePoint = mBigHour ? GPoint(DIGIT_X1_S, DIGIT_Y) : GPoint(SCREEN_W2+DIGIT_X1-BHOFFSET, DIGIT_Y);
        set_container_image(&time_digits_images[2], time_digits_layers[2], tick_time->tm_min/10, minutePoint, !mBigHour);
        minutePoint.x = mBigHour ? DIGIT_X2_S : SCREEN_W2+DIGIT_X2-BHOFFSET;
        set_container_image(&time_digits_images[3], time_digits_layers[3], tick_time->tm_min%10, minutePoint, !mBigHour);
    }	
}

static void load_settings()
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "load_settings");
    if (persist_exists(0))
    {
        mBigHour = persist_read_bool(0);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "load_settings - setting found: %d", mBigHour);
    }
    else
    {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "load_settings - setting missing");
        persist_write_bool(0, true);
    }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context)
{
    // Read preferences
    Tuple* big_hour_t = dict_find(iter, MESSAGE_KEY_BigHour);
    if (big_hour_t)
    {
        mBigHour = big_hour_t->value->int32 == 1;
        
        // Save the setting.
        APP_LOG(APP_LOG_LEVEL_DEBUG, "load_settings - setting set: %d", mBigHour);
        persist_write_bool(0, mBigHour);
        
        // Force a redraw with updated settings.
        time_t now = time(NULL);
        struct tm *tick_time = localtime(&now);
        handle_tick(tick_time, DAY_UNIT + HOUR_UNIT + MINUTE_UNIT);
    }
}

static void init(void)
{
    memset(&time_digits_layers, 0, sizeof(time_digits_layers));
    memset(&time_digits_images, 0, sizeof(time_digits_images));

    const int inbound_size = 64;
    const int outbound_size = 64;
    app_message_open(inbound_size, outbound_size);  

    window = window_create();
    if (window == NULL)
    {
        return;
    }

    window_set_background_color(window, GColorBlack);
    
    canvas = layer_create(GRect(0, 0, 144, 168));
	layer_set_update_proc(canvas, (LayerUpdateProc) render);
	layer_add_child(window_get_root_layer(window), canvas);
	
    window_stack_push(window, true);
    Layer *window_layer = window_get_root_layer(window);
    
    load_settings();
    
    // Open AppMessage connection
    app_message_register_inbox_received(inbox_received_handler);
    app_message_open(128, 128);
    
    //owm_weather_init(5ba77aab84470992ddc7e49e4985aeab);
    //events_app_message_open();
    //owm_weather_fetch();
 
    // Create time and date layers
    GRect dummy_frame = { {0, 0}, {0, 0} };

    for (int i = 0; i < TOTAL_TIME_DIGITS; i++)
    {
        time_digits_layers[i] = bitmap_layer_create(dummy_frame);
        layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers[i]));
    }
    
    mFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_18));
    mDayText = malloc(sizeof("WEDNESDAY "));

    // Avoids a blank screen on watch start.
    time_t now = time(NULL);
    struct tm *tick_time = localtime(&now);
    handle_tick(tick_time, DAY_UNIT + HOUR_UNIT + MINUTE_UNIT);

    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
}


static void deinit(void)
{
  tick_timer_service_unsubscribe();

  for (int i = 0; i < TOTAL_TIME_DIGITS; i++)
  {
    layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers[i]));
    gbitmap_destroy(time_digits_images[i]);
    bitmap_layer_destroy(time_digits_layers[i]);
  }

	  window_destroy(window);
}

int main(void)
{
  init();
  app_event_loop();
  deinit();
}