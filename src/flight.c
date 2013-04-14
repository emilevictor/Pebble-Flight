#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0xCE, 0x53, 0x23, 0xAE, 0x6E, 0x13, 0x4D, 0xC0, 0xB2, 0x17, 0x8D, 0x7C, 0xB4, 0xA4, 0xD9, 0x7B }
PBL_APP_INFO(MY_UUID,
             "Flight", "Emile Victor",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;
TextLayer zuluHeader;
TextLayer zuluTime;
TextLayer localHeader;
TextLayer localTime;
TextLayer flightTimeHeader;
TextLayer flightTime;
bool timerHasStarted = false;
PblTm initialFlightTime;
BmpContainer background_image_container;
bool hasVibed = false;



void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t)
{
  static char zuluHeaderText[] = "zulu";
  static char zuluTimeText[] = "00:00:00";
  static char localHeaderText[] = "local";
  static char localTimeText[] = "00:00:00";
  static char flightTimeHeaderText[] = "flight time";
  static char flightTimeText[25];
  strcpy(flightTimeText,"xx");

  //Change me to change GMT offset.
  int GMTOffset = -11;
  bool addHalfHourOffset = true;

  //Set the header text for static headers.
  text_layer_set_text(&localHeader, localHeaderText);
  text_layer_set_text(&zuluHeader, zuluHeaderText);
  text_layer_set_text(&flightTimeHeader, flightTimeHeaderText);



  (void) t;
  (void)ctx;


  //Do the GMT conversion

  PblTm gmtTime;
  memcpy(&gmtTime,t->tick_time,sizeof(gmtTime));

  int convertingHour = t->tick_time->tm_hour - GMTOffset;
  if (convertingHour < 0)
  {
    convertingHour = 24 + convertingHour;
  } else if (convertingHour > 24)
  {
    convertingHour = convertingHour - 24;
  }

  if (addHalfHourOffset)
  {
    if ((t->tick_time->tm_min + 30)>60)
    {
      convertingHour += 1;
      if (convertingHour < 0)
      {
        convertingHour = 24 + convertingHour;
      } else if (convertingHour > 24)
      {
        convertingHour = convertingHour - 24;
      }
      gmtTime.tm_min = (t->tick_time->tm_min + 30)-60;
    } else {
      gmtTime.tm_min = t->tick_time->tm_min + 30;
    }
  }
  gmtTime.tm_hour = convertingHour;


  
  string_format_time(zuluTimeText, sizeof(zuluTimeText), "%T", &gmtTime);
  text_layer_set_text(&zuluTime, zuluTimeText);

  //Local time
  string_format_time(localTimeText, sizeof(localTimeText), "%T", t->tick_time);
  text_layer_set_text(&localTime, localTimeText);




  //If the initial starting time hasn't been set:
  if (!timerHasStarted)
  {
    memcpy(&initialFlightTime,t->tick_time,sizeof(initialFlightTime));
    timerHasStarted = true;
    //initialFlightTime = t->tick_time;
  } else {
    int totalElapsedMinutes = 0;
    PblTm localInitialFlightTime = initialFlightTime;
    //calculate minutes since start.
    if (initialFlightTime.tm_hour != t->tick_time->tm_hour)
      {
        //Check minutes up to end of hour, add one to the hour, check if the times are still different.
        int numberOfMinutesToNextHour = 60 - initialFlightTime.tm_min;
        totalElapsedMinutes = totalElapsedMinutes + numberOfMinutesToNextHour;
        localInitialFlightTime.tm_hour = localInitialFlightTime.tm_hour + 1;

        //If they are still different, take the new hour, get the difference between current and newly modified hour and multiply by 60.
        if (localInitialFlightTime.tm_hour != t->tick_time->tm_hour)
        {
          totalElapsedMinutes += (t->tick_time->tm_hour - localInitialFlightTime.tm_hour)*60;

        }

        totalElapsedMinutes += t->tick_time->tm_min; 


      } else {

        totalElapsedMinutes += t->tick_time->tm_min - initialFlightTime.tm_min;
      }
      t->tick_time->tm_min = totalElapsedMinutes;
      string_format_time(flightTimeText, sizeof(flightTimeText), "%M", t->tick_time);
      if (totalElapsedMinutes == 1)
      {
        strcat(flightTimeText, " minute");
      } else if (totalElapsedMinutes == 0)
      {
        strcpy(flightTimeText, "--Ascent--");
      } else {
        strcat(flightTimeText, " minutes");
      }

      //At the 30 minute mark, alert the pilot to check tanks.
      if ((totalElapsedMinutes%30 == 0)&&(totalElapsedMinutes != 0)&&(hasVibed == false))
      {
        vibes_long_pulse();
        hasVibed = true;
      } else if ((totalElapsedMinutes%30 == 0)&&(totalElapsedMinutes != 0)) {
        hasVibed = true;
      } else {
        hasVibed = false;
      }
      

  }

  //FLight time text
  text_layer_set_text(&flightTime, flightTimeText);

}


void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "Flight");
  window_stack_push(&window, true /* Animated */);

  resource_init_current_app(&APP_RESOURCES);

  window_set_background_color(&window, GColorBlack);

  bmp_init_container(RESOURCE_ID_IMAGE_BACKGROUND, &background_image_container);

  layer_add_child(&window.layer, &background_image_container.layer.layer);


  //Local header
  text_layer_init(&localHeader, window.layer.frame);
  layer_set_frame(&localHeader.layer, GRect(15,9,114-8,168-8));
  text_layer_set_text_color(&localHeader, GColorWhite);
  text_layer_set_background_color(&localHeader, GColorClear);
  text_layer_set_font(&localHeader, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(&window.layer, &localHeader.layer);

  //Local text
  text_layer_init(&localTime, window.layer.frame);
  layer_set_frame(&localTime.layer, GRect(15,25,114-8,168-8));
  text_layer_set_text_color(&localTime, GColorWhite);
  text_layer_set_background_color(&localTime, GColorClear);
  text_layer_set_font(&localTime, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(&window.layer, &localTime.layer);

  //Zulu header
  text_layer_init(&zuluHeader, window.layer.frame);
  layer_set_frame(&zuluHeader.layer, GRect(15,50,114-8,168-8));
  text_layer_set_text_color(&zuluHeader, GColorWhite);
  text_layer_set_background_color(&zuluHeader, GColorClear);
  text_layer_set_font(&zuluHeader, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(&window.layer, &zuluHeader.layer);

  //Zulu text
  text_layer_init(&zuluTime, window.layer.frame);
  layer_set_frame(&zuluTime.layer, GRect(15,65,114-8,168-8));
  text_layer_set_text_color(&zuluTime, GColorWhite);
  text_layer_set_background_color(&zuluTime, GColorClear);
  text_layer_set_font(&zuluTime, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(&window.layer, &zuluTime.layer);

  //Elapsed header
  text_layer_init(&flightTimeHeader, window.layer.frame);
  layer_set_frame(&flightTimeHeader.layer, GRect(15,90,114-8,168-8));
  text_layer_set_text_color(&flightTimeHeader, GColorWhite);
  text_layer_set_background_color(&flightTimeHeader, GColorClear);
  text_layer_set_font(&flightTimeHeader, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(&window.layer, &flightTimeHeader.layer);

  //Elapsed text
  text_layer_init(&flightTime, window.layer.frame);
  layer_set_frame(&flightTime.layer, GRect(15,105,114-8,168-8));
  text_layer_set_text_color(&flightTime, GColorWhite);
  text_layer_set_background_color(&flightTime, GColorClear);
  text_layer_set_font(&flightTime, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(&window.layer, &flightTime.layer);
}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,

    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }

  };
  app_event_loop(params, &handlers);
}
