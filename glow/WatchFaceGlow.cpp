#include "WatchFaceGlow.h"

#include "displayapp/screens/Symbols.h"

using namespace Pinetime::Applications::Screens;
using namespace ConceptFaces;

extern lv_font_t jetbrains_mono_bold_20;

namespace {
  constexpr const char* fontPath = "/fonts/glow_text.bin";
  constexpr lv_coord_t cx = 120;
  constexpr lv_coord_t cy = 120;
  const lv_color_t cyan = lv_color_hex(0x40e0d0);
  const lv_color_t red = lv_color_hex(0xff2d55);
  const lv_color_t tick = lv_color_hex(0xbfc8cc);
}

bool WatchFaceGlow::IsAvailable(Pinetime::Controllers::FS& filesystem) {
  return FileExists(filesystem, fontPath);
}

WatchFaceGlow::WatchFaceGlow(Controllers::DateTime& dateTime,
                             const Controllers::Battery& battery,
                             const Controllers::Ble& ble,
                             Controllers::FS& filesystem)
  : dateTime {dateTime}, battery {battery}, ble {ble} {
  font = LoadFont(filesystem, fontPath);

  Background(LV_COLOR_BLACK);
  Ticks(cx, cy, 228, 60, 4, 1, lv_color_hex(0x6a7378));
  Ticks(cx, cy, 228, 12, 10, 2, tick);

  date = Label(Or(font, &jetbrains_mono_bold_20), LV_COLOR_WHITE);

  // battery gauge (top left) and Bluetooth (top right)
  lv_obj_t* outline = Rect(lv_scr_act(), 14, 14, 18, 9, LV_COLOR_BLACK, 2);
  lv_obj_set_style_local_border_width(outline, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_border_color(outline, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, tick);
  Rect(lv_scr_act(), 32, 16, 2, 5, tick);
  batteryFill = Rect(lv_scr_act(), 16, 16, 14, 5, cyan, 1);
  bluetooth = Label(&jetbrains_mono_bold_20, tick);
  lv_label_set_text_static(bluetooth, Symbols::bluetooth);
  lv_obj_set_pos(bluetooth, 212, 8);

  hourHand.Create(cyan, 5);
  minuteHand.Create(cyan, 4);
  secondHand.Create(red, 2);
  Circle(cx, cy, 5, red);
  Circle(cx, cy, 2, LV_COLOR_BLACK);

  refreshTask = lv_task_create(RefreshTaskCallback, 250, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceGlow::~WatchFaceGlow() {
  lv_task_del(refreshTask);
  lv_obj_clean(lv_scr_act());
  FreeFont(font);
}

void WatchFaceGlow::Refresh() {
  minute = dateTime.Minutes();
  if (minute.IsUpdated()) {
    const int h = dateTime.Hours() % 12;
    const int m = minute.Get();
    hourHand.Hand(cx, cy, h * 30 + m / 2, 52);
    minuteHand.Hand(cx, cy, m * 6, 92);
    lv_label_set_text_fmt(date, "%s %d %s", dateTime.DayOfWeekShortToString(), dateTime.Day(), dateTime.MonthShortToString());
    lv_obj_align(date, nullptr, LV_ALIGN_IN_LEFT_MID, cx + 14, 0);
  }
  second = dateTime.Seconds();
  if (second.IsUpdated()) {
    secondHand.Hand(cx, cy, second.Get() * 6, 100, 18);
  }

  batteryPercent = battery.PercentRemaining();
  if (batteryPercent.IsUpdated()) {
    const uint8_t pct = batteryPercent.Get() > 100 ? 100 : batteryPercent.Get();
    lv_obj_set_width(batteryFill, static_cast<lv_coord_t>(1 + pct * 13 / 100));
  }
  connected = ble.IsConnected();
  if (connected.IsUpdated()) {
    lv_obj_set_hidden(bluetooth, !connected.Get());
  }
}
