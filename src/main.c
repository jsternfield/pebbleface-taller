#include "pebble.h"

static Window *window;
static Layer* canvas;

#define TOTAL_TIME_DIGITS 4
static GBitmap* time_digits_images[TOTAL_TIME_DIGITS];
static BitmapLayer* time_digits_layers[TOTAL_TIME_DIGITS];

static GPoint mImageSizeBig = { .x = 30, .y = 154 };
static GPoint mImageSizeSm = { .x = 30, .y = 77 };

static GPoint mHourPoint1 = { .x = 2, .y = 7 };
static GPoint mHourPoint2 = { .x = 34, .y = 7 };
static GPoint mMinutePoint1 = { .x = 76, .y = 7 };
static GPoint mMinutePoint2 = { .x = 108, .y = 7 };

static GFont mFont;
static char* mDayText = NULL;
static int mDayTextSize = sizeof("WEDNESDAY ");
static GPoint mDayPoint = { .x = 40, .y = 90 };

const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_0,
  RESOURCE_ID_IMAGE_NUM_1,
  RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3,
  RESOURCE_ID_IMAGE_NUM_4,
  RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6,
  RESOURCE_ID_IMAGE_NUM_7,
  RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};

static void render(Layer *layer, GContext* ctx) 
{
    graphics_context_set_text_color(ctx, GColorWhite);

    graphics_draw_text(ctx, mDayText, mFont, GRect(mDayPoint.x, mDayPoint.y, 144, 168), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin, bool isBig)
{
    GBitmap *old_image = *bmp_image;
    *bmp_image = gbitmap_create_with_resource(resource_id);

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

        set_container_image(&time_digits_images[0], time_digits_layers[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], mHourPoint1, true);
        set_container_image(&time_digits_images[1], time_digits_layers[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], mHourPoint2, true);

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
        set_container_image(&time_digits_images[2], time_digits_layers[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], mMinutePoint1, false);
        set_container_image(&time_digits_images[3], time_digits_layers[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], mMinutePoint2, false);
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