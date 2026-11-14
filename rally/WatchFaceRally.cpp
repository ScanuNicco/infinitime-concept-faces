#include "WatchFaceRally.h"

#include <algorithm>
#include <cstdlib>

using namespace Pinetime::Applications::Screens;
using namespace ConceptFaces;

extern lv_font_t jetbrains_mono_bold_20;

namespace {
  constexpr const char* fontPath = "/fonts/rally_digits.bin";
  constexpr lv_coord_t cx = 120;
  constexpr lv_coord_t cy = 120;
  constexpr int trackHalf = 114; // half-size of the square the track runs along
  constexpr int numbersHalf = 96;
  constexpr int dialHalf = 58;
  constexpr int dialNumbersHalf = 46;

  const lv_color_t green = lv_color_hex(0x4cd964);
  const lv_color_t yellow = lv_color_hex(0xffd60a);
  const lv_color_t trackOff = lv_color_hex(0x262626);

  // Point at fraction i/n of the way round the square's perimeter, clockwise from 12 o'clock.
  lv_point_t AlongSquare(int i, int n, int half) {
    const int perimeter = 8 * half;
    int d = (i * perimeter / n + half) % perimeter; // distance from the top-left corner
    const int side = 2 * half;
    if (d < side) {
      return {static_cast<lv_coord_t>(cx - half + d), static_cast<lv_coord_t>(cy - half)};
    }
    d -= side;
    if (d < side) {
      return {static_cast<lv_coord_t>(cx + half), static_cast<lv_coord_t>(cy - half + d)};
    }
    d -= side;
    if (d < side) {
      return {static_cast<lv_coord_t>(cx + half - d), static_cast<lv_coord_t>(cy + half)};
    }
    d -= side;
    return {static_cast<lv_coord_t>(cx - half), static_cast<lv_coord_t>(cy + half - d)};
  }
}

bool WatchFaceRally::IsAvailable(Pinetime::Controllers::FS& filesystem) {
  return FileExists(filesystem, fontPath);
}

WatchFaceRally::WatchFaceRally(Controllers::DateTime& dateTime,
                               Controllers::Settings& settings,
                               Controllers::MotionController& motion,
                               Controllers::FS& filesystem)
  : dateTime {dateTime}, settings {settings}, motion {motion} {
  font = LoadFont(filesystem, fontPath);
  const lv_font_t* digits = Or(font, &jetbrains_mono_bold_20);

  Background(LV_COLOR_BLACK);

  // step track: dim all the way round, segments light up with progress
  for (int i = 0; i < segments; i++) {
    track[i].Create(trackOff, 7, false);
    track[i].Set(AlongSquare(i, segments, trackHalf), AlongSquare(i + 1, segments, trackHalf));
  }

  // bezel numbers 5..60; the date sits where 15 would be
  for (int v = 5; v <= 60; v += 5) {
    if (v == 15) {
      continue;
    }
    lv_obj_t* l = Label(digits, v == 60 ? lv_color_hex(0xff453a) : LV_COLOR_WHITE);
    lv_label_set_text_fmt(l, v == 5 ? "05" : "%d", v);
    const lv_point_t p = OnSquare(cx, cy, numbersHalf, v * 6);
    lv_obj_set_pos(l, p.x - lv_obj_get_width(l) / 2, p.y - lv_obj_get_height(l) / 2);
  }
  lv_obj_t* dateBox = Rect(lv_scr_act(), cx + numbersHalf - 15, cy - 11, 30, 22, LV_COLOR_WHITE, 3);
  date = Label(digits, LV_COLOR_BLACK, dateBox);

  // the inner dial
  lv_obj_t* dial = Rect(lv_scr_act(), cx - dialHalf, cy - dialHalf, 2 * dialHalf, 2 * dialHalf, LV_COLOR_BLACK, 12);
  lv_obj_set_style_local_border_width(dial, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 2);
  lv_obj_set_style_local_border_color(dial, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  for (int h = 1; h <= 12; h++) {
    lv_obj_t* l = Label(digits, lv_color_hex(0xd0d0d0));
    lv_label_set_text_fmt(l, "%d", h);
    const lv_point_t p = OnSquare(cx, cy, dialNumbersHalf, h * 30);
    lv_obj_set_pos(l, p.x - lv_obj_get_width(l) / 2, p.y - lv_obj_get_height(l) / 2);
  }

  hourHand.Create(LV_COLOR_WHITE, 5);
  minuteHand.Create(LV_COLOR_WHITE, 3);
  Circle(cx, cy, 4, LV_COLOR_WHITE);

  refreshTask = lv_task_create(RefreshTaskCallback, 1000, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceRally::~WatchFaceRally() {
  lv_task_del(refreshTask);
  lv_obj_clean(lv_scr_act());
  FreeFont(font);
}

void WatchFaceRally::Refresh() {
  currentMinute = std::chrono::time_point_cast<std::chrono::minutes>(dateTime.CurrentDateTime());
  if (currentMinute.IsUpdated()) {
    const int h = dateTime.Hours() % 12;
    const int m = dateTime.Minutes();
    hourHand.Hand(cx, cy, h * 30 + m / 2, 30, 6);
    minuteHand.Hand(cx, cy, m * 6, 48, 6);
    lv_label_set_text_fmt(date, "%d", dateTime.Day());
    lv_obj_align(date, nullptr, LV_ALIGN_CENTER, 0, 1);
  }

  const uint32_t goal = settings.GetStepsGoal();
  const uint32_t steps = motion.NbSteps();
  const int lit = goal == 0 ? 0 : static_cast<int>(std::min<uint32_t>(steps, goal) * segments / goal);
  litSegments = lit;
  if (litSegments.IsUpdated()) {
    for (int i = 0; i < segments; i++) {
      const lv_color_t c = i < lit ? lv_color_mix(yellow, green, static_cast<uint8_t>(i * 255 / (segments - 1))) : trackOff;
      lv_obj_set_style_local_line_color(track[i].obj, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, c);
    }
  }
}
