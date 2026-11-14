#include "WatchFaceTiles.h"

#include <cstdio>

using namespace Pinetime::Applications::Screens;

extern lv_font_t jetbrains_mono_bold_20;
extern lv_font_t jetbrains_mono_76;

namespace {
  constexpr const char* digitsFont = "/fonts/tiles_digits.bin";
  constexpr const char* textFont = "/fonts/tiles_text.bin";

  const lv_color_t background = lv_color_hex(0xc4161c);
  const lv_color_t tileColour = lv_color_hex(0x6e0b0f);
  const lv_color_t digitColour = lv_color_hex(0xffeaea);
  const lv_color_t dateColour = lv_color_hex(0xffcfcf);

  constexpr lv_coord_t tileW = 76;
  constexpr lv_coord_t tileH = 74;
  constexpr lv_coord_t gap = 8;
  constexpr lv_coord_t tilesX = (240 - 2 * tileW - gap) / 2;
  constexpr lv_coord_t tilesY = 58;

  bool FileExists(Pinetime::Controllers::FS& fs, const char* path) {
    lfs_file f = {};
    if (fs.FileOpen(&f, path, LFS_O_RDONLY) < 0) {
      return false;
    }
    fs.FileClose(&f);
    return true;
  }

  // Load a font from the resource pack, or return nullptr if it isn't installed.
  lv_font_t* LoadFont(Pinetime::Controllers::FS& fs, const char* path) {
    if (!FileExists(fs, path)) {
      return nullptr;
    }
    char lvPath[48];
    snprintf(lvPath, sizeof(lvPath), "F:%s", path);
    return lv_font_load(lvPath);
  }

  lv_obj_t* Rect(lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, lv_color_t colour, lv_coord_t radius) {
    lv_obj_t* o = lv_obj_create(lv_scr_act(), nullptr);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_local_bg_color(o, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, colour);
    lv_obj_set_style_local_radius(o, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, radius);
    lv_obj_set_style_local_border_width(o, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_click(o, false);
    return o;
  }

  lv_obj_t* Label(const lv_font_t* font, lv_color_t colour) {
    lv_obj_t* l = lv_label_create(lv_scr_act(), nullptr);
    lv_obj_set_style_local_text_font(l, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font);
    lv_obj_set_style_local_text_color(l, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, colour);
    return l;
  }
}

bool WatchFaceTiles::IsAvailable(Pinetime::Controllers::FS& filesystem) {
  return FileExists(filesystem, digitsFont) && FileExists(filesystem, textFont);
}

WatchFaceTiles::WatchFaceTiles(Controllers::DateTime& dateTime, Controllers::Settings& settings, Controllers::FS& filesystem)
  : dateTime {dateTime}, settings {settings} {
  fontDigits = LoadFont(filesystem, digitsFont);
  fontText = LoadFont(filesystem, textFont);
  const lv_font_t* digitsOrFallback = fontDigits != nullptr ? fontDigits : &jetbrains_mono_76;
  const lv_font_t* textOrFallback = fontText != nullptr ? fontText : &jetbrains_mono_bold_20;

  Rect(0, 0, LV_HOR_RES, LV_VER_RES, background, 0);

  date = Label(textOrFallback, dateColour);
  weekday = Label(textOrFallback, LV_COLOR_WHITE);
  ampm = Label(textOrFallback, dateColour);

  for (size_t i = 0; i < tiles.size(); i++) {
    const lv_coord_t x = tilesX + static_cast<lv_coord_t>(i % 2) * (tileW + gap);
    const lv_coord_t y = tilesY + static_cast<lv_coord_t>(i / 2) * (tileH + gap);
    tiles[i] = Rect(x, y, tileW, tileH, tileColour, 6);
    digits[i] = Label(digitsOrFallback, digitColour);
    // the hinge of the flip card, drawn over the digit
    Rect(x, y + tileH / 2 - 1, tileW, 2, background, 0);
  }

  refreshTask = lv_task_create(RefreshTaskCallback, 1000, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceTiles::~WatchFaceTiles() {
  lv_task_del(refreshTask);
  lv_obj_clean(lv_scr_act());
  // fonts can only be freed once nothing uses them any more
  if (fontDigits != nullptr) {
    lv_font_free(fontDigits);
  }
  if (fontText != nullptr) {
    lv_font_free(fontText);
  }
}

void WatchFaceTiles::Refresh() {
  currentMinute = std::chrono::time_point_cast<std::chrono::minutes>(dateTime.CurrentDateTime());
  if (!currentMinute.IsUpdated()) {
    return;
  }

  uint8_t hours = dateTime.Hours();
  const bool twelveHour = settings.GetClockType() == Controllers::Settings::ClockType::H12;
  if (twelveHour) {
    lv_label_set_text_static(ampm, hours < 12 ? "AM" : "PM");
    hours = hours % 12 == 0 ? 12 : hours % 12;
  }
  lv_obj_set_hidden(ampm, !twelveHour);
  lv_obj_align(ampm, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, -4);

  const uint8_t minutes = dateTime.Minutes();
  const char values[4] = {static_cast<char>('0' + hours / 10),
                          static_cast<char>('0' + hours % 10),
                          static_cast<char>('0' + minutes / 10),
                          static_cast<char>('0' + minutes % 10)};
  for (size_t i = 0; i < digits.size(); i++) {
    const char text[2] = {values[i], '\0'};
    lv_label_set_text(digits[i], text);
    lv_obj_align(digits[i], tiles[i], LV_ALIGN_CENTER, 0, 2);
  }

  lv_label_set_text_fmt(date, "%d %s %d", dateTime.Day(), dateTime.MonthShortToString(), dateTime.Year());
  lv_obj_align(date, nullptr, LV_ALIGN_IN_TOP_MID, 0, 6);
  lv_label_set_text(weekday, dateTime.DayOfWeekToString());
  lv_obj_align(weekday, nullptr, LV_ALIGN_IN_TOP_MID, 0, 30);
}
