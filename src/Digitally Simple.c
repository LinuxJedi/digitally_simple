/* vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 * Drizzle Client & Protocol Library
 *
 * Copyright (C) 2015 Andrew Hutchings <andrew@linuxjedi.co.uk>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 *     * The names of its contributors may not be used to endorse or
 * promote products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <pebble.h>

// Comment out to enable logging
//#undef APP_LOG
//#define APP_LOG(...)

static Window *window;
static TextLayer *time_layer;
static TextLayer *seconds_layer;
static TextLayer *date_layer;
static TextLayer *battery_icon_layer;
static TextLayer *battery_percent_layer;
static TextLayer *bluetooth_icon_layer;

static void handle_tick(struct tm* tick_time, TimeUnits units_changed)
{
  static char seconds[4];
  static char time[6];
  static char date[24];
  static bool first_second= true;

  if (units_changed & SECOND_UNIT)
  {
    strftime(seconds, sizeof(seconds), "%S", tick_time);
    text_layer_set_text(seconds_layer, seconds);
  }
  if ((units_changed & MINUTE_UNIT) || first_second)
  {
    strftime(time, sizeof(time), "%H:%M", tick_time);
    text_layer_set_text(time_layer, time);
  }
  if ((units_changed & DAY_UNIT) || first_second)
  {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Day changed");
    strftime(date, sizeof(date), "%A\n%F", tick_time);
    text_layer_set_text(date_layer, date);
  }
  first_second= false;
}

static void handle_bluetooth(bool connected) {
  static char bt_text[4];
  static bool bt_connected= true;
  if (connected)
  {
    strcpy(bt_text, "\xef\x84\x96");
    if (!bt_connected)
    {
      bt_connected = true;
      vibes_double_pulse();
    }
  }
  else
  {
    strcpy(bt_text, "");
    if (bt_connected)
    {
      bt_connected = false;
      vibes_long_pulse();
    }
  }
  text_layer_set_text(bluetooth_icon_layer, bt_text);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Bluetooth change: connected=%i", connected);
}

static void handle_battery(BatteryChargeState charge_state)
{
  static char battery_icon[4];
  static char battery_text[5];
  if (charge_state.charge_percent > 80)
  {
    strcpy(battery_icon, "\xef\x84\x93");
  }
  else if (charge_state.charge_percent > 50 && charge_state.charge_percent <= 80)
  {
    strcpy(battery_icon, "\xef\x84\x94");
  }
  else if (charge_state.charge_percent > 20 && charge_state.charge_percent <= 50)
  {
    strcpy(battery_icon, "\xef\x84\x95");
  }
  else
  {
    strcpy(battery_icon, "\xef\x84\x92");
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Battery change: %i remaining", charge_state.charge_percent);
  snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  text_layer_set_text(battery_icon_layer, battery_icon);
  text_layer_set_text(battery_percent_layer, battery_text);
}

static void window_load(Window *window)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Watchface init");
  Layer *window_layer= window_get_root_layer(window);
  window_set_background_color(window, GColorBlack);
  GFont time_font= fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_38));
  GFont seconds_font= fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_16));
  GFont date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DATE_22));
  GFont status_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_STATUS_12));
  GFont icon_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ICONS_14));

  // Hour / Minute
  time_layer= text_layer_create(GRect(2, 38, 118, 55));
  text_layer_set_text_alignment(time_layer, GTextAlignmentLeft);
  text_layer_set_font(time_layer, time_font);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_background_color(time_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(time_layer));

  // Seconds
  seconds_layer= text_layer_create(GRect(120, 60, 30, 20));
  text_layer_set_text_alignment(seconds_layer, GTextAlignmentLeft);
  text_layer_set_font(seconds_layer, seconds_font);
  text_layer_set_text_color(seconds_layer, GColorWhite);
  text_layer_set_background_color(seconds_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(seconds_layer));

  // Date
  date_layer = text_layer_create(GRect(2, 100, 142, 50));
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  text_layer_set_font(date_layer, date_font);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_background_color(date_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(date_layer));

  // Battery Percentage
  battery_percent_layer = text_layer_create(GRect(85, 1, 33, 15));
  text_layer_set_font(battery_percent_layer, status_font);
  text_layer_set_text_color(battery_percent_layer, GColorWhite);
  text_layer_set_background_color(battery_percent_layer, GColorClear);
  text_layer_set_text_alignment(battery_percent_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(battery_percent_layer));

  // Battery Icon
  battery_icon_layer = text_layer_create(GRect(120, 1, 15, 15));
  text_layer_set_font(battery_icon_layer, icon_font);
  text_layer_set_text_color(battery_icon_layer, GColorWhite);
  text_layer_set_background_color(battery_icon_layer, GColorClear);
  text_layer_set_text_alignment(battery_icon_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(battery_icon_layer));

  // Bluetooth Icon
  bluetooth_icon_layer = text_layer_create(GRect(2, 1, 15, 15));
  text_layer_set_font(bluetooth_icon_layer, icon_font);
  text_layer_set_text_color(bluetooth_icon_layer, GColorWhite);
  text_layer_set_background_color(bluetooth_icon_layer, GColorClear);
  text_layer_set_text_alignment(bluetooth_icon_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(bluetooth_icon_layer));

  // Events
  tick_timer_service_subscribe(SECOND_UNIT|MINUTE_UNIT|DAY_UNIT, &handle_tick);
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(&handle_bluetooth);
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_tick(current_time, SECOND_UNIT);
  handle_battery(battery_state_service_peek());
  handle_bluetooth(bluetooth_connection_service_peek());
}

static void window_unload(Window *window)
{
  text_layer_destroy(time_layer);
  text_layer_destroy(seconds_layer);
  text_layer_destroy(date_layer);
  text_layer_destroy(battery_percent_layer);
  text_layer_destroy(battery_icon_layer);
  text_layer_destroy(bluetooth_icon_layer);
}

static void init(void)
{
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void)
{
  window_destroy(window);
}

int main(void)
{
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
