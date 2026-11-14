#include "WatchFaceSubdial.h"

using namespace Pinetime::Applications::Screens;
using namespace ConceptFaces;

extern lv_font_t jetbrains_mono_bold_20;

namespace {
  constexpr const char* fontPath = "/fonts/subdial_text.bin";
  constexpr lv_coord_t cx = 120;
  constexpr lv_coord_t cy = 120;
  constexpr lv_coord_t subX = 120; // small-seconds sub-dial
  constexpr lv_coord_t subY = 172;
  constexpr lv_coord_t subHalf = 24;

  const lv_color_t dial = lv_color_hex(0x1e1f21);
  const lv_color_t mark = lv_color_hex(0x9a9a9a);
  const lv_color_t silver = lv_color_hex(0xdedede);
  const lv_color_t yellow = lv_color_hex(0xffd60a);
  const lv_color_t green = lv_color_hex(0x30d158);
}

bool WatchFaceSubdial::IsAvailable(Pinetime::Controllers::FS& filesystem) {
  return FileExists(filesystem, fontPath);
}

WatchFaceSubdial::WatchFaceSubdial(Controllers::DateTime& dateTime, Controllers::FS& filesystem) : dateTime {dateTime} {
  font = LoadFont(filesystem, fontPath);
  const lv_font_t* text = Or(font, &jetbrains_mono_bold_20);

  Background(dial);
  size_t next = 0;
  for (int h = 0; h < 12; h++) {
    if (h == 3 || h == 6) {
      continue;
    }
    const int inner = h == 0 ? 96 : 104;
    marks[next].Create(mark, h == 0 ? 4 : 2, false);
    marks[next].Set(Polar(cx, cy, inner, h * 30), Polar(cx, cy, 116, h * 30));
    next++;
  }

  weekday = Label(text, yellow);

  // date window at three o'clock
  lv_obj_t* window = Rect(lv_scr_act(), 190, cy - 11, 30, 22, dial, 2);
  lv_obj_set_style_local_border_width(window, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_border_color(window, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, silver);
  date = Label(text, LV_COLOR_WHITE, window);

  // small-seconds sub-dial at six o'clock
  lv_obj_t* sub = Rect(lv_scr_act(), subX - subHalf, subY - subHalf, 2 * subHalf, 2 * subHalf, lv_color_hex(0x161718), 4);
  lv_obj_set_style_local_border_width(sub, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_border_color(sub, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x4a4a4a));
  Ticks(subX, subY, 2 * subHalf - 4, 12, 3, 1, lv_color_hex(0x5a5a5a));
  secondHand.Create(green, 2);
  Circle(subX, subY, 2, green);

  hourHand.Create(silver, 4);
  minuteHand.Create(silver, 3);
  Circle(cx, cy, 6, silver);
  Circle(cx, cy, 3, dial);

  refreshTask = lv_task_create(RefreshTaskCallback, 250, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceSubdial::~WatchFaceSubdial() {
  lv_task_del(refreshTask);
  lv_obj_clean(lv_scr_act());
  FreeFont(font);
}

void WatchFaceSubdial::Refresh() {
  minute = dateTime.Minutes();
  if (minute.IsUpdated()) {
    const int h = dateTime.Hours() % 12;
    const int m = minute.Get();
    hourHand.Hand(cx, cy, h * 30 + m / 2, 56, 8);
    minuteHand.Hand(cx, cy, m * 6, 96, 8);
    lv_label_set_text(weekday, dateTime.DayOfWeekToString());
    lv_obj_align(weekday, nullptr, LV_ALIGN_IN_TOP_MID, 0, 50);
    lv_label_set_text_fmt(date, "%d", dateTime.Day());
    lv_obj_align(date, nullptr, LV_ALIGN_CENTER, 0, 1);
  }
  second = dateTime.Seconds();
  if (second.IsUpdated()) {
    secondHand.Hand(subX, subY, second.Get() * 6, subHalf - 4);
  }
}
