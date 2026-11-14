#include "WatchFaceConstellation.h"

using namespace Pinetime::Applications::Screens;
using namespace ConceptFaces;

extern lv_font_t jetbrains_mono_bold_20;

namespace {
  constexpr const char* fontPath = "/fonts/constellation_text.bin";
  constexpr lv_coord_t cx = 120;
  constexpr lv_coord_t cy = 122;
  constexpr int hourRadius = 42;
  constexpr int minuteRadius = 74;
  constexpr lv_coord_t hourStarR = 6;
  constexpr lv_coord_t minuteStarR = 8;

  const lv_color_t sky = lv_color_hex(0x0b1838);
  const lv_color_t link = lv_color_hex(0x7088bd);

  // Keep in sync with CONDITIONS in tools/make_manifest.py (the font only has these letters).
  const char* Condition(Pinetime::Controllers::SimpleWeatherService::Icons icon) {
    using Icons = Pinetime::Controllers::SimpleWeatherService::Icons;
    switch (icon) {
      case Icons::Sun:
        return "Clear";
      case Icons::CloudsSun:
      case Icons::Clouds:
      case Icons::BrokenClouds:
        return "Cloudy";
      case Icons::CloudShowerHeavy:
        return "Showers";
      case Icons::CloudSunRain:
        return "Raining";
      case Icons::Thunderstorm:
        return "Storm";
      case Icons::Snow:
        return "Snowing";
      case Icons::Smog:
        return "Misty";
      default:
        return "";
    }
  }

  void Place(lv_obj_t* star, lv_point_t p, lv_coord_t r) {
    lv_obj_set_pos(star, p.x - r, p.y - r);
  }
}

bool WatchFaceConstellation::IsAvailable(Pinetime::Controllers::FS& filesystem) {
  return FileExists(filesystem, fontPath);
}

lv_design_res_t WatchFaceConstellation::Draw(lv_obj_t* layer, const lv_area_t* clip, lv_design_mode_t mode) {
  if (mode != LV_DESIGN_DRAW_MAIN) {
    return NotDrawing(mode);
  }
  static const lv_color_t faint = lv_color_hex(0x6f84b3);
  static const lv_color_t bright = lv_color_hex(0xc3d0ee);
  for (const Star& s : Owner<WatchFaceConstellation>(layer)->stars) {
    const lv_coord_t x = s.x;
    const lv_coord_t y = s.y;
    if (s.size == 2) {
      DrawDot(clip, x, y, 1, bright);
      DrawDot(clip, x - 1, y, 1, bright, LV_OPA_70);
      DrawDot(clip, x + 1, y, 1, bright, LV_OPA_70);
      DrawDot(clip, x, y - 1, 1, bright, LV_OPA_70);
      DrawDot(clip, x, y + 1, 1, bright, LV_OPA_70);
    } else {
      DrawDot(clip, x, y, 1, faint);
      if (s.size == 1) {
        DrawDot(clip, x + 1, y, 1, faint);
      }
    }
  }
  return LV_DESIGN_RES_OK;
}

WatchFaceConstellation::WatchFaceConstellation(Controllers::DateTime& dateTime,
                                               Controllers::Settings& settings,
                                               Controllers::SimpleWeatherService* weather,
                                               Controllers::FS& filesystem)
  : dateTime {dateTime}, settings {settings}, weather {weather} {
  font = LoadFont(filesystem, fontPath);
  const lv_font_t* text = Or(font, &jetbrains_mono_bold_20);

  Background(sky);
  // the same sky every time: stars at pseudo-random places from a fixed seed
  uint32_t rng = 0x5eed1234;
  for (Star& s : stars) {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    const uint32_t size = (rng >> 24) % 100;
    s = {static_cast<uint8_t>(3 + rng % 234), static_cast<uint8_t>(3 + (rng >> 8) % 234), static_cast<uint8_t>(size > 82 ? 2 : (size > 55 ? 1 : 0))};
  }
  DrawLayer(Draw, this);

  conditions = Label(text, LV_COLOR_WHITE);
  date = Label(text, lv_color_hex(0xd6e0f5));

  toHour.Create(link, 2);
  toMinute.Create(link, 2);
  Circle(cx, cy, 3, lv_color_hex(0xbfcde8));
  hourStar = Circle(0, 0, hourStarR, LV_COLOR_WHITE);
  minuteStar = Circle(0, 0, minuteStarR, LV_COLOR_WHITE);

  refreshTask = lv_task_create(RefreshTaskCallback, 1000, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceConstellation::~WatchFaceConstellation() {
  lv_task_del(refreshTask);
  lv_obj_clean(lv_scr_act());
  FreeFont(font);
}

void WatchFaceConstellation::Refresh() {
  minute = dateTime.Minutes();
  if (minute.IsUpdated()) {
    const int h = dateTime.Hours() % 12;
    const int m = minute.Get();
    const lv_point_t centre {cx, cy};
    const lv_point_t hour = Polar(cx, cy, hourRadius, h * 30 + m / 2);
    const lv_point_t min = Polar(cx, cy, minuteRadius, m * 6);
    toHour.Set(centre, hour);
    toMinute.Set(hour, min);
    Place(hourStar, hour, hourStarR);
    Place(minuteStar, min, minuteStarR);

    lv_label_set_text_fmt(date, "%s %d %s", dateTime.MonthShortToString(), dateTime.Day(), dateTime.DayOfWeekShortToString());
    lv_obj_align(date, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, -6);
  }

  if (weather != nullptr) {
    currentWeather = weather->Current();
  }
  if (currentWeather.IsUpdated()) {
    const auto& w = currentWeather.Get();
    if (w.has_value()) {
      lv_label_set_text_fmt(conditions, "%d%s %s", Degrees(w->temperature, settings), degreeSign, Condition(w->iconId));
    } else {
      lv_label_set_text_static(conditions, "");
    }
    lv_obj_align(conditions, nullptr, LV_ALIGN_IN_TOP_MID, 0, 10);
  }
}
