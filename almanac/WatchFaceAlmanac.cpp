#include "WatchFaceAlmanac.h"

#include <cstdio>

#include "displayapp/screens/WeatherSymbols.h"
#include "../common/FaceKit.h"

using namespace Pinetime::Applications::Screens;
using namespace ConceptFaces;

extern lv_font_t jetbrains_mono_bold_20;
extern lv_font_t jetbrains_mono_42;
extern lv_font_t fontawesome_weathericons;

namespace {
  constexpr const char* bigFont = "/fonts/almanac_big.bin";
  constexpr const char* smallFont = "/fonts/almanac_small.bin";
  constexpr int neighbours = 4; // values shown either side of the current one

  const lv_color_t grey = lv_color_hex(0x8a8a8a);
  const lv_color_t dimGrey = lv_color_hex(0x5f5f5f);
}

bool WatchFaceAlmanac::IsAvailable(Pinetime::Controllers::FS& filesystem) {
  return AllExist(filesystem, {bigFont, smallFont});
}

WatchFaceAlmanac::WatchFaceAlmanac(Controllers::DateTime& dateTime,
                                   Controllers::Settings& settings,
                                   Controllers::SimpleWeatherService* weather,
                                   Controllers::FS& filesystem)
  : dateTime {dateTime}, settings {settings}, weather {weather} {
  fontBig = LoadFont(filesystem, bigFont);
  fontSmall = LoadFont(filesystem, smallFont);
  const lv_font_t* big = Or(fontBig, &jetbrains_mono_42);
  const lv_font_t* small = Or(fontSmall, &jetbrains_mono_bold_20);

  Background(LV_COLOR_BLACK);
  icon = Label(&fontawesome_weathericons, LV_COLOR_WHITE);
  temperature = Label(small, grey);
  for (Ruler* r : {&hours, &minutes}) {
    r->before = Label(small, dimGrey);
    r->current = Label(big, LV_COLOR_WHITE);
    r->after = Label(small, dimGrey);
  }
  date = Label(small, grey);

  refreshTask = lv_task_create(RefreshTaskCallback, 1000, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceAlmanac::~WatchFaceAlmanac() {
  lv_task_del(refreshTask);
  lv_obj_clean(lv_scr_act());
  FreeFont(fontBig);
  FreeFont(fontSmall);
}

namespace {
  // Append a number (optionally zero-padded to two digits) and an optional gap to buf.
  size_t Append(char* buf, size_t size, size_t at, int value, bool twoDigits, bool gap) {
    if (at >= size) {
      return at;
    }
    int n = twoDigits ? snprintf(buf + at, size - at, "%02d", value) : snprintf(buf + at, size - at, "%d", value);
    at += static_cast<size_t>(n > 0 ? n : 0);
    if (gap && at + 2 < size) {
      buf[at++] = ' ';
      buf[at++] = ' ';
      buf[at] = '\0';
    }
    return at;
  }
}

// Values cycle through first .. first+count-1 (e.g. 0..23 hours, 1..12 hours, 0..59 minutes).
void WatchFaceAlmanac::SetRuler(Ruler& ruler, int value, int count, int first, bool twoDigits, lv_coord_t y) {
  auto wrap = [count, first](int v) {
    return ((v - first) % count + count) % count + first;
  };
  char before[40] = {};
  char after[40] = {};
  char current[8] = {};
  size_t nb = 0;
  size_t na = 0;
  for (int i = neighbours; i >= 1; i--) {
    nb = Append(before, sizeof(before), nb, wrap(value - i), twoDigits, i > 1);
  }
  for (int i = 1; i <= neighbours; i++) {
    na = Append(after, sizeof(after), na, wrap(value + i), twoDigits, i < neighbours);
  }
  Append(current, sizeof(current), 0, value, twoDigits, false);
  lv_label_set_text(ruler.current, current);
  lv_obj_align(ruler.current, nullptr, LV_ALIGN_IN_TOP_MID, 0, y);
  lv_label_set_text(ruler.before, before);
  lv_obj_align(ruler.before, ruler.current, LV_ALIGN_OUT_LEFT_MID, -10, 2);
  lv_label_set_text(ruler.after, after);
  lv_obj_align(ruler.after, ruler.current, LV_ALIGN_OUT_RIGHT_MID, 10, 2);
}

void WatchFaceAlmanac::Refresh() {
  minute = dateTime.Minutes();
  if (minute.IsUpdated()) {
    if (settings.GetClockType() == Controllers::Settings::ClockType::H12) {
      SetRuler(hours, Hour12(dateTime.Hours()), 12, 1, false, 82);
    } else {
      SetRuler(hours, dateTime.Hours(), 24, 0, true, 82);
    }
    SetRuler(minutes, minute.Get(), 60, 0, true, 122);
    lv_label_set_text_fmt(date,
                          "%s %s %d",
                          Controllers::DateTime::DayOfWeekShortToStringLow(dateTime.DayOfWeek()),
                          Controllers::DateTime::MonthShortToStringLow(dateTime.Month()),
                          dateTime.Day());
    lv_obj_align(date, nullptr, LV_ALIGN_IN_TOP_MID, 0, 176);
  }

  if (weather != nullptr) {
    currentWeather = weather->Current();
  }
  if (currentWeather.IsUpdated()) {
    const auto& w = currentWeather.Get();
    if (w.has_value()) {
      const bool imperial = settings.GetWeatherFormat() == Controllers::Settings::WeatherFormat::Imperial;
      const int now = dateTime.Hours() * 60 + dateTime.Minutes();
      const bool night = w->sunrise >= 0 && w->sunset > w->sunrise ? (now < w->sunrise || now >= w->sunset) : (now < 360 || now >= 1080);
      lv_label_set_text(icon, Symbols::GetSymbol(w->iconId, night));
      lv_label_set_text_fmt(temperature, "%d%s%s", Degrees(w->temperature, settings), degreeSign, imperial ? "F" : "C");
    } else {
      lv_label_set_text_static(icon, "");
      lv_label_set_text_static(temperature, "");
    }
    lv_obj_align(icon, nullptr, LV_ALIGN_IN_TOP_MID, 0, 18);
    lv_obj_align(temperature, nullptr, LV_ALIGN_IN_TOP_MID, 0, 48);
  }
}
