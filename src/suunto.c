#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

//#define DEBUG

#define MY_UUID {0x84, 0x02, 0xAD, 0xE5, 0x73, 0x82, 0x49, 0x7C, 0x9E, 0xAA, 0x38, 0x3D, 0x8C, 0x33, 0x25, 0x34}

PBL_APP_INFO(MY_UUID, "Suunto Core", "Peter Zich", 1, 2, RESOURCE_ID_IMAGE_MENU_ICON, APP_INFO_WATCH_FACE);

Window window;
GRect wFrame;

int largeImageIDs[13] = {
	RESOURCE_ID_IMAGE_LARGE_0,
	RESOURCE_ID_IMAGE_LARGE_1,
	RESOURCE_ID_IMAGE_LARGE_2,
	RESOURCE_ID_IMAGE_LARGE_3,
	RESOURCE_ID_IMAGE_LARGE_4,
	RESOURCE_ID_IMAGE_LARGE_5,
	RESOURCE_ID_IMAGE_LARGE_6,
	RESOURCE_ID_IMAGE_LARGE_7,
	RESOURCE_ID_IMAGE_LARGE_8,
	RESOURCE_ID_IMAGE_LARGE_9,
	RESOURCE_ID_IMAGE_LARGE_COLON,
	RESOURCE_ID_IMAGE_LARGE_AM,
	RESOURCE_ID_IMAGE_LARGE_PM
};
int smallImageIDs[18] = {
	RESOURCE_ID_IMAGE_SMALL_0,
	RESOURCE_ID_IMAGE_SMALL_1,
	RESOURCE_ID_IMAGE_SMALL_2,
	RESOURCE_ID_IMAGE_SMALL_3,
	RESOURCE_ID_IMAGE_SMALL_4,
	RESOURCE_ID_IMAGE_SMALL_5,
	RESOURCE_ID_IMAGE_SMALL_6,
	RESOURCE_ID_IMAGE_SMALL_7,
	RESOURCE_ID_IMAGE_SMALL_8,
	RESOURCE_ID_IMAGE_SMALL_9,
	RESOURCE_ID_IMAGE_SMALL_PERIOD,
	RESOURCE_ID_IMAGE_DAY_SUN,
	RESOURCE_ID_IMAGE_DAY_MON,
	RESOURCE_ID_IMAGE_DAY_TUE,
	RESOURCE_ID_IMAGE_DAY_WED,
	RESOURCE_ID_IMAGE_DAY_THU,
	RESOURCE_ID_IMAGE_DAY_FRI,
	RESOURCE_ID_IMAGE_DAY_SAT
};

#define TOP_ROW_Y 42
#define BOTTOM_ROW_Y 105

BmpContainer tRow[6], bRow[7];
unsigned char tRowC = 0, bRowC = 0;
char tRowS[7], bRowS[8];
int lastDate = -1;

int largeImageConv(char c){
  switch(c){
    case '0':
      return RESOURCE_ID_IMAGE_LARGE_0;
    case '1':
      return RESOURCE_ID_IMAGE_LARGE_1;
    case '2':
      return RESOURCE_ID_IMAGE_LARGE_2;
    case '3':
      return RESOURCE_ID_IMAGE_LARGE_3;
    case '4':
      return RESOURCE_ID_IMAGE_LARGE_4;
    case '5':
      return RESOURCE_ID_IMAGE_LARGE_5;
    case '6':
      return RESOURCE_ID_IMAGE_LARGE_6;
    case '7':
      return RESOURCE_ID_IMAGE_LARGE_7;
    case '8':
      return RESOURCE_ID_IMAGE_LARGE_8;
    case '9':
      return RESOURCE_ID_IMAGE_LARGE_9;
    case ':':
      return RESOURCE_ID_IMAGE_LARGE_COLON;
    case 'a':
      return RESOURCE_ID_IMAGE_LARGE_AM;
    case 'p':
      return RESOURCE_ID_IMAGE_LARGE_PM;
  }
  return -1;
}

int smallImageConv(char c){
  switch(c){
    case '0':
      return RESOURCE_ID_IMAGE_SMALL_0;
    case '1':
      return RESOURCE_ID_IMAGE_SMALL_1;
    case '2':
      return RESOURCE_ID_IMAGE_SMALL_2;
    case '3':
      return RESOURCE_ID_IMAGE_SMALL_3;
    case '4':
      return RESOURCE_ID_IMAGE_SMALL_4;
    case '5':
      return RESOURCE_ID_IMAGE_SMALL_5;
    case '6':
      return RESOURCE_ID_IMAGE_SMALL_6;
    case '7':
      return RESOURCE_ID_IMAGE_SMALL_7;
    case '8':
      return RESOURCE_ID_IMAGE_SMALL_8;
    case '9':
      return RESOURCE_ID_IMAGE_SMALL_9;
    case '.':
      return RESOURCE_ID_IMAGE_SMALL_PERIOD;
    case 'a':
      return RESOURCE_ID_IMAGE_DAY_SUN;
    case 'b':
      return RESOURCE_ID_IMAGE_DAY_MON;
    case 'c':
      return RESOURCE_ID_IMAGE_DAY_TUE;
    case 'd':
      return RESOURCE_ID_IMAGE_DAY_WED;
    case 'e':
      return RESOURCE_ID_IMAGE_DAY_THU;
    case 'f':
      return RESOURCE_ID_IMAGE_DAY_FRI;
    case 'g':
      return RESOURCE_ID_IMAGE_DAY_SAT;
  }
  return -1;
}

void setLine(char *str, BmpContainer *imgs, unsigned char *count, int(*conv)(char), int y, int spacing) {
  int i = 0, width = 0;
  for (i=0; i<*count; i++) {
    layer_remove_from_parent(&(imgs[i]).layer.layer);
    bmp_deinit_container(&imgs[i]);
  }

  *count = strlen(str);

  for (i=0; i<*count; i++) {
    int id = conv(str[i]);
    if (id < 0) id = conv('0');
    bmp_init_container(id, &imgs[i]);
    width += imgs[i].layer.layer.frame.size.w;
    if(i+1<*count) width += spacing;
  }

  int start = (144 - width) / 2;
  for (i=0; i<*count; i++) {
    imgs[i].layer.layer.frame.origin.x = start;
    imgs[i].layer.layer.frame.origin.y = y;
    layer_add_child(&window.layer, &imgs[i].layer.layer);
    start += imgs[i].layer.layer.frame.size.w;
    if(i+1<*count) start += spacing;
  }
}

void formatTime(PblTm *time) {
  char *s = tRowS;

  int h = time->tm_hour;
  if(!clock_is_24h_style()){
    h = h % 12;
    if(h == 0) h = 12;
  }

  int m = time->tm_min;

  if(h >= 10)
    *s++ = '0' + (h / 10);
  *s++ = '0' + (h % 10);
  *s++ = ':';
  *s++ = '0' + (m / 10);
  *s++ = '0' + (m % 10);
  if(!clock_is_24h_style()){
    *s++ = (time->tm_hour < 12 ? 'a' : 'p');
  }
  *s++ = '\0';
}

void formatDate(PblTm *time) {
  char *s = bRowS;

  int m = time->tm_mon + 1;
  int d = time->tm_mday;
  int w = time->tm_wday;

  *s++ = 'a' + w;
  if(m >= 10)
    *s++ = '0' + (m / 10);
  *s++ = '0' + (m % 10);
  *s++ = '.';
  if(d >= 10)
    *s++ = '0' + (d / 10);
  *s++ = '0' + (d % 10);
  *s++ = '.';
  *s++ = '\0';
}

void display_time(PblTm *time) {
  formatTime(time);
  setLine(tRowS, tRow, &tRowC, &largeImageConv, TOP_ROW_Y, 6);

  if(time->tm_mday != lastDate){
    lastDate = time->tm_mday;
    formatDate(time);
    setLine(bRowS, bRow, &bRowC, &smallImageConv, BOTTOM_ROW_Y, 3);
  }
}

void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "Suunto Core");
  window_stack_push(&window, true /* Animated */);
  resource_init_current_app(&APP_RESOURCES);
  window_set_background_color(&window, GColorWhite);
  wFrame = layer_get_frame(&window.layer);

  PblTm t;
  get_time(&t);
  display_time(&t);
#ifdef DEBUG
  app_timer_send_event(ctx, 2000, 1);
#endif
}

void handle_deinit(AppContextRef ctx) {
  (void)ctx;

  setLine("", tRow, &tRowC, &largeImageConv, TOP_ROW_Y, 0);
  setLine("", bRow, &bRowC, &smallImageConv, BOTTOM_ROW_Y, 0);
}

void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)ctx;
  display_time(t->tick_time);
}

#ifdef DEBUG
int db_i = 0;
int db_c = 13;
char *db_str[] = {
  //Test adding and removing characters
  "1:23a",  "a1.2.",
  "12:34a", "b12.34.",
  "1:23p",  "c1.2.",

  //Test other character combinations
  "13:57", "d1.3.",
  "11:11", "e1.4.",
  "1:11",  "f1.4.",
  "23:59", "g1.5.",
  "01234", "01234",
  "5678",  "56789",
  "9:ap",  ".abc",
  "0:00a", "def",
  "0:00p", "g0.0.",

  //Test no characters
  "", ""
};

void debug_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
  (void)ctx;
  (void)handle;
  (void)cookie;
  setLine(db_str[db_i*2], tRow, &tRowC, &largeImageConv, TOP_ROW_Y, 6);
  setLine(db_str[db_i*2+1], bRow, &bRowC, &smallImageConv, BOTTOM_ROW_Y, 3);
  db_i++;
  if(db_i < db_c)
    app_timer_send_event(ctx, 2000, 1);

  lastDate = -1;
}
#endif

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,
    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    }
#ifdef DEBUG
    ,.timer_handler = &debug_timer
#endif
  };
  app_event_loop(params, &handlers);
}
