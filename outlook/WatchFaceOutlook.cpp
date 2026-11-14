#include "WatchFaceOutlook.h"

#include <cstdio>

#include "../common/FaceKit.h"

using namespace Pinetime::Applications::Screens;
using namespace ConceptFaces;

extern lv_font_t jetbrains_mono_bold_20;
extern lv_font_t jetbrains_mono_42;
extern lv_font_t jetbrains_mono_76;

namespace {
  constexpr const char* timeFont = "/fonts/outlook_time.bin";
  constexpr const char* tempFont = "/fonts/outlook_temp.bin";
  constexpr const char* textFont = "/fonts/outlook_text.bin";

  const lv_color_t yellow = lv_color_hex(0xdddd00);
  const lv_color_t darkYellow = lv_color_hex(0x4a4a00);
  const lv_color_t grey = lv_color_hex(0x8a8a8a);

  // layout of the lower half
  constexpr lv_coord_t dividerX = 84;
  constexpr lv_coord_t tempX = 94;
  constexpr lv_coord_t batteryX = 188;
  constexpr lv_coord_t batteryY = 196;
  constexpr lv_coord_t batteryW = 28;
  constexpr lv_coord_t batteryH = 14;

  // Image for a weather icon, following InfiniTime's own choice of symbols (WeatherSymbols.cpp).
  // Names must match WEATHER_ICONS in tools/make_images.py.
  const char* WeatherName(Pinetime::Controllers::SimpleWeatherService::Icons icon, bool night) {
    using Icons = Pinetime::Controllers::SimpleWeatherService::Icons;
    switch (icon) {
      case Icons::Sun:
        return night ? "moon" : "sun";
      case Icons::CloudsSun:
        return night ? "cloudmoon" : "cloudsun";
      case Icons::Clouds:
        return "cloud";
      case Icons::BrokenClouds:
        return "brokenclouds";
      case Icons::CloudShowerHeavy:
        return "showers";
      case Icons::CloudSunRain:
        return night ? "moonrain" : "sunrain";
      case Icons::Thunderstorm:
        return "bolt";
      case Icons::Snow:
        return "snow";
      case Icons::Smog:
        return "smog";
      default:
        return "unknown";
    }
  }

  void ShowWeatherImage(lv_obj_t* img, const char* prefix, const char* name) {
    char path[48];
    snprintf(path, sizeof(path), "F:/images/%s%s.bin", prefix, name);
    lv_img_set_src(img, path); // LVGL copies the path
  }

  bool IsNight(int minutes, const std::optional<Pinetime::Controllers::SimpleWeatherService::CurrentWeather>& w) {
    int sunrise = 6 * 60;
    int sunset = 18 * 60;
    if (w.has_value() && w->sunrise >= 0 && w->sunset > w->sunrise) {
      sunrise = w->sunrise;
      sunset = w->sunset;
    }
    return minutes < sunrise || minutes >= sunset;
  }
}

bool WatchFaceOutlook::IsAvailable(Pinetime::Controllers::FS& filesystem) {
  return AllExist(filesystem, {timeFont, tempFont, textFont, "/images/outlook_wx_sun.bin", "/images/outlook_wxs_sun.bin"});
}

WatchFaceOutlook::WatchFaceOutlook(Controllers::DateTime& dateTime,
                                   Controllers::Settings& settings,
                                   const Controllers::Battery& battery,
                                   Controllers::SimpleWeatherService* weather,
                                   Controllers::FS& filesystem)
  : dateTime {dateTime}, settings {settings}, battery {battery}, weather {weather} {
  fontTime = LoadFont(filesystem, timeFont);
  fontTemp = LoadFont(filesystem, tempFont);
  fontText = LoadFont(filesystem, textFont);
  const lv_font_t* text = Or(fontText, &jetbrains_mono_bold_20);

  Background(LV_COLOR_BLACK);
  time = Label(Or(fontTime, &jetbrains_mono_76), LV_COLOR_WHITE);
  date = Label(text, yellow);
  weekday = Label(text, LV_COLOR_WHITE);

  // an unbroken arc from edge to edge: y = 146 + (x - 120)^2 / 600
  for (int i = 0; i < 13; i++) {
    const int x = i * 20;
    arc[i] = {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(146 + (x - 120) * (x - 120) / 600)};
  }
  lv_obj_t* line = lv_line_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_line_width(line, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, 3);
  lv_obj_set_style_local_line_color(line, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, grey);
  lv_obj_set_style_local_line_rounded(line, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, true);
  lv_line_set_points(line, arc, 13);

  // weather icon | temperature, battery and forecast
  icon = lv_img_create(lv_scr_act(), nullptr);
  for (lv_coord_t y = 178; y < 236; y += 8) {
    Rect(lv_scr_act(), dividerX, y, 2, 4, LV_COLOR_WHITE); // dashed divider
  }
  temperature = Label(Or(fontTemp, &jetbrains_mono_42), LV_COLOR_WHITE);

  // battery: percentage over a yellow battery with + and - marks; the yellow part shrinks as it drains
  batteryText = Label(text, LV_COLOR_WHITE);
  Rect(lv_scr_act(), batteryX, batteryY, batteryW, batteryH, darkYellow, 3);
  batteryFill = Rect(lv_scr_act(), batteryX, batteryY, batteryW, batteryH, yellow, 3);
  Rect(lv_scr_act(), batteryX + batteryW, batteryY + 4, 3, batteryH - 8, yellow, 1);
  Rect(lv_scr_act(), batteryX + 5, batteryY + 6, 7, 2, LV_COLOR_BLACK);  // +
  Rect(lv_scr_act(), batteryX + 7, batteryY + 4, 2, 6, LV_COLOR_BLACK);
  Rect(lv_scr_act(), batteryX + 17, batteryY + 6, 7, 2, LV_COLOR_BLACK); // -

  for (size_t i = 0; i < forecastIcons.size(); i++) {
    forecastIcons[i] = lv_img_create(lv_scr_act(), nullptr);
  }
  for (size_t i = 0; i < forecastDashes.size(); i++) {
    forecastDashes[i] = Rect(lv_scr_act(), 0, 0, 6, 2, yellow);
  }

  refreshTask = lv_task_create(RefreshTaskCallback, 1000, LV_TASK_PRIO_MID, this);
  Refresh();
  UpdateWeather();
}

WatchFaceOutlook::~WatchFaceOutlook() {
  lv_task_del(refreshTask);
  lv_obj_clean(lv_scr_act());
  FreeFont(fontTime);
  FreeFont(fontTemp);
  FreeFont(fontText);
}

void WatchFaceOutlook::Refresh() {
  currentMinute = std::chrono::time_point_cast<std::chrono::minutes>(dateTime.CurrentDateTime());
  const bool minuteChanged = currentMinute.IsUpdated();
  if (minuteChanged) {
    const uint8_t h = settings.GetClockType() == Controllers::Settings::ClockType::H12 ? Hour12(dateTime.Hours()) : dateTime.Hours();
    lv_label_set_text_fmt(time, "%d:%02d", h, dateTime.Minutes());
    lv_obj_align(time, nullptr, LV_ALIGN_IN_TOP_MID, 0, 2);
    lv_label_set_text_fmt(date, "%d %s %d", dateTime.Day(), dateTime.MonthShortToString(), dateTime.Year());
    lv_obj_align(date, nullptr, LV_ALIGN_IN_TOP_MID, 0, 96);
    lv_label_set_text(weekday, dateTime.DayOfWeekToString());
    lv_obj_align(weekday, nullptr, LV_ALIGN_IN_TOP_MID, 0, 117);
  }

  batteryPercent = battery.PercentRemaining();
  if (batteryPercent.IsUpdated()) {
    const uint8_t pct = batteryPercent.Get() > 100 ? 100 : batteryPercent.Get();
    lv_label_set_text_fmt(batteryText, "%d%%", pct);
    lv_obj_align(batteryText, nullptr, LV_ALIGN_IN_TOP_LEFT, batteryX + 2, 172);
    lv_obj_set_width(batteryFill, static_cast<lv_coord_t>(batteryH / 2 + pct * (batteryW - batteryH / 2) / 100));
  }

  if (weather != nullptr) {
    currentWeather = weather->Current();
    forecast = weather->GetForecast();
  }
  if (currentWeather.IsUpdated() || forecast.IsUpdated() || minuteChanged) {
    UpdateWeather(); // also on the minute: the icon switches between day and night
  }
}

void WatchFaceOutlook::UpdateWeather() {
  const auto& w = currentWeather.Get();
  const bool night = IsNight(dateTime.Hours() * 60 + dateTime.Minutes(), w);
  if (w.has_value()) {
    ShowWeatherImage(icon, "outlook_wx_", WeatherName(w->iconId, night));
    lv_label_set_text_fmt(temperature, "%d%s", Degrees(w->temperature, settings), degreeSign);
  } else {
    lv_label_set_text_static(temperature, "--");
  }
  lv_obj_set_hidden(icon, !w.has_value());
  // centre the 44 px icon in the space left of the divider
  lv_obj_set_pos(icon, (dividerX - 44) / 2, 180);
  lv_obj_align(temperature, nullptr, LV_ALIGN_IN_TOP_LEFT, tempX, 164);

  // the next days, joined by short dashes, under the temperature
  const auto& f = forecast.Get();
  lv_coord_t x = tempX;
  for (size_t i = 0; i < forecastIcons.size(); i++) {
    const size_t day = i + 1; // tomorrow onwards
    const bool has = f.has_value() && day < f->nbDays && f->days[day].has_value();
    if (has) {
      ShowWeatherImage(forecastIcons[i], "outlook_wxs_", WeatherName(f->days[day]->iconId, false));
    }
    lv_obj_set_hidden(forecastIcons[i], !has);
    lv_obj_set_pos(forecastIcons[i], x, 214);
    x = static_cast<lv_coord_t>(x + 20 + 4);
    if (i < forecastDashes.size()) {
      lv_obj_set_hidden(forecastDashes[i], !has);
      lv_obj_set_pos(forecastDashes[i], x, 224);
      x = static_cast<lv_coord_t>(x + 6 + 4);
    }
  }
}
