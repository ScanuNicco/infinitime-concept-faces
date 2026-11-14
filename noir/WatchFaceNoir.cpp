#include "WatchFaceNoir.h"

using namespace Pinetime::Applications::Screens;
using namespace ConceptFaces;

extern lv_font_t jetbrains_mono_bold_20;

namespace {
  constexpr const char* fontPath = "/fonts/noir_text.bin";
  constexpr lv_coord_t cx = 120;
  constexpr lv_coord_t cy = 120;
  const lv_color_t grey = lv_color_hex(0x8a8a8a);
  const lv_color_t red = lv_color_hex(0xe53935);
}

bool WatchFaceNoir::IsAvailable(Pinetime::Controllers::FS& filesystem) {
  return FileExists(filesystem, fontPath);
}

WatchFaceNoir::WatchFaceNoir(Controllers::DateTime& dateTime, Controllers::FS& filesystem) : dateTime {dateTime} {
  font = LoadFont(filesystem, fontPath);

  Background(LV_COLOR_BLACK);
  // hour marks; twelve gets a double mark, three gives way to the date
  size_t next = 0;
  for (int h = 1; h < 12; h++) {
    if (h == 3) {
      continue;
    }
    marks[next].Create(grey, 2, false);
    marks[next].Set(Polar(cx, cy, 104, h * 30), Polar(cx, cy, 118, h * 30));
    next++;
  }
  twelveLeft.Create(LV_COLOR_WHITE, 2, false);
  twelveLeft.Set({cx - 4, 2}, {cx - 4, 18});
  twelveRight.Create(LV_COLOR_WHITE, 2, false);
  twelveRight.Set({cx + 4, 2}, {cx + 4, 18});

  date = Label(Or(font, &jetbrains_mono_bold_20), LV_COLOR_WHITE);
  Rect(lv_scr_act(), 224, cy - 1, 14, 2, LV_COLOR_WHITE);

  hourHand.Create(LV_COLOR_WHITE, 4);
  minuteHand.Create(LV_COLOR_WHITE, 3);
  secondHand.Create(red, 2);
  Circle(cx, cy, 4, LV_COLOR_WHITE);

  refreshTask = lv_task_create(RefreshTaskCallback, 250, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceNoir::~WatchFaceNoir() {
  lv_task_del(refreshTask);
  lv_obj_clean(lv_scr_act());
  FreeFont(font);
}

void WatchFaceNoir::Refresh() {
  minute = dateTime.Minutes();
  if (minute.IsUpdated()) {
    const int h = dateTime.Hours() % 12;
    const int m = minute.Get();
    hourHand.Hand(cx, cy, h * 30 + m / 2, 62, 12);
    minuteHand.Hand(cx, cy, m * 6, 104, 12);
    lv_label_set_text_fmt(date, "%s %d", dateTime.DayOfWeekShortToString(), dateTime.Day());
    lv_obj_align(date, nullptr, LV_ALIGN_IN_RIGHT_MID, -20, 0);
  }
  second = dateTime.Seconds();
  if (second.IsUpdated()) {
    secondHand.Hand(cx, cy, second.Get() * 6, 104, 22);
  }
}
