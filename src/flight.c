#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0xCE, 0x53, 0x23, 0xAE, 0x6E, 0x13, 0x4D, 0xC0, 0xB2, 0x17, 0x8D, 0x7C, 0xB4, 0xA4, 0xD9, 0x7B }
PBL_APP_INFO(MY_UUID,
             "Flight", "Emile Victor",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);

Window window;
TextLayer zuluHeader;
TextLayer zuluTime;
TextLayer localHeader;
TextLayer localTime;
TextLayer flightTimeHeader;
TextLayer flightTime;
bool timerHasStarted = false;
PblTm initialFlightTime;



void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t)
{
  static char zuluHeaderText[] = "zulu";
  static char zuluTimeText[] = "00:00:00";
  static char localHeaderText[] = "local";
  static char localTimeText[] = "00:00:00";
  static char flightTimeHeaderText[] = "flight time";
  static char flightTimeText[25];
  strcpy(flightTimeText,"xx");

  int GMTOffset = 10;



  (void) t;
  (void)ctx;


  //Do the GMT conversion

  PblTm gmtTime;
  memcpy(&gmtTime,t->tick_time,sizeof(gmtTime));

  int convertingHour = t->tick_time->tm_hour - GMTOffset;
  if (convertingHour < 0)
  {
    convertingHour = 24 + convertingHour;
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
      strcat(flightTimeText, " minutes");

  }

  //FLight time text
  text_layer_set_text(&flightTime, flightTimeText);

}


void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "Flight");
  window_stack_push(&window, true /* Animated */);
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
