#include "WatchFaceOutlook.h"

#include "displayapp/screens/WeatherSymbols.h"
#include "../common/FaceKit.h"

using namespace Pinetime::Applications::Screens;
using namespace ConceptFaces;

extern lv_font_t jetbrains_mono_bold_20;
extern lv_font_t jetbrains_mono_42;
extern lv_font_t jetbrains_mono_76;
extern lv_font_t fontawesome_weathericons;

namespace {
  constexpr const char* timeFont = "/fonts/outlook_time.bin";
  constexpr const char* tempFont = "/fonts/outlook_temp.bin";
  constexpr const char* textFont = "/fonts/outlook_text.bin";

  const lv_color_t yellow = lv_color_hex(0xffd60a);
  const lv_color_t grey = lv_color_hex(0x5a5a5a);

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
  return AllExist(filesystem, {timeFont, tempFont, textFont});
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

  // a gentle curve under the date: y = 136 + (x - 120)^2 / 900
  for (int i = 0; i < 9; i++) {
    const int x = 12 + i * 27;
    curve[i] = {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(136 + (x - 120) * (x - 120) / 900)};
  }
  lv_obj_t* line = lv_line_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_line_width(line, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, 2);
  lv_obj_set_style_local_line_color(line, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, grey);
  lv_line_set_points(line, curve, 9);

  icon = Label(&fontawesome_weathericons, yellow);
  Rect(lv_scr_act(), 78, 160, 2, 40, grey);
  temperature = Label(Or(fontTemp, &jetbrains_mono_42), LV_COLOR_WHITE);

  // battery: percentage and a small gauge
  batteryText = Label(text, LV_COLOR_WHITE);
  lv_obj_t* outline = Rect(lv_scr_act(), 186, 186, 26, 12, LV_COLOR_BLACK, 2);
  lv_obj_set_style_local_border_width(outline, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 2);
  lv_obj_set_style_local_border_color(outline, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  Rect(lv_scr_act(), 212, 189, 3, 6, LV_COLOR_WHITE, 1);
  batteryFill = Rect(lv_scr_act(), 189, 189, 20, 6, yellow, 1);

  for (size_t i = 0; i < forecastIcons.size(); i++) {
    forecastIcons[i] = Label(&fontawesome_weathericons, yellow);
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
    lv_obj_align(time, nullptr, LV_ALIGN_IN_TOP_MID, 0, 10);
    lv_label_set_text_fmt(date, "%d %s %d", dateTime.Day(), dateTime.MonthShortToString(), dateTime.Year());
    lv_obj_align(date, nullptr, LV_ALIGN_IN_TOP_MID, 0, 92);
    lv_label_set_text(weekday, dateTime.DayOfWeekToString());
    lv_obj_align(weekday, nullptr, LV_ALIGN_IN_TOP_MID, 0, 112);
  }

  batteryPercent = battery.PercentRemaining();
  if (batteryPercent.IsUpdated()) {
    const uint8_t pct = batteryPercent.Get() > 100 ? 100 : batteryPercent.Get();
    lv_label_set_text_fmt(batteryText, "%d%%", pct);
    lv_obj_align(batteryText, nullptr, LV_ALIGN_IN_TOP_LEFT, 186, 160);
    lv_obj_set_width(batteryFill, static_cast<lv_coord_t>(pct * 20 / 100));
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
    lv_label_set_text(icon, Symbols::GetSymbol(w->iconId, night));
    lv_label_set_text_fmt(temperature, "%d%s", Degrees(w->temperature, settings), degreeSign);
  } else {
    lv_label_set_text_static(icon, "");
    lv_label_set_text_static(temperature, "--");
  }
  lv_obj_align(icon, nullptr, LV_ALIGN_IN_TOP_MID, -80, 168);
  lv_obj_align(temperature, nullptr, LV_ALIGN_IN_TOP_LEFT, 92, 154);

  const auto& f = forecast.Get();
  for (size_t i = 0; i < forecastIcons.size(); i++) {
    const size_t day = i + 1; // tomorrow onwards
    if (f.has_value() && day < f->nbDays && f->days[day].has_value()) {
      lv_label_set_text(forecastIcons[i], Symbols::GetSymbol(f->days[day]->iconId, false));
    } else {
      lv_label_set_text_static(forecastIcons[i], "");
    }
    lv_obj_align(forecastIcons[i], nullptr, LV_ALIGN_IN_TOP_MID, static_cast<lv_coord_t>((static_cast<int>(i) - 1) * 50), 208);
  }
}
