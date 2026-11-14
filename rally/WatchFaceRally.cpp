#include "WatchFaceRally.h"

#include <algorithm>

using namespace Pinetime::Applications::Screens;
using namespace ConceptFaces;

extern lv_font_t jetbrains_mono_bold_20;

namespace {
  constexpr const char* fontPath = "/fonts/rally_digits.bin";
  constexpr const char* ticksMinor = "/images/rally_ticks_minor.bin";
  constexpr const char* ticksMajor = "/images/rally_ticks_major.bin";
  constexpr const char* dialTicks = "/images/rally_dial_ticks.bin";

  constexpr lv_coord_t cx = 120;
  constexpr lv_coord_t cy = 120;
  constexpr int bandHalf = 116;  // the band runs round the very edge of the screen
  constexpr int numbersHalf = 86;
  constexpr int dialHalf = 58;   // keep in sync with RALLY_DIAL in tools/make_images.py
  constexpr int dialNumbersHalf = 45;
  constexpr int half = 16;       // segments per gauge

  const lv_color_t green = lv_color_hex(0x8cc63f);
  const lv_color_t yellow = lv_color_hex(0xf0e400);
  const lv_color_t dimSteps = lv_color_hex(0x262d12);
  const lv_color_t white = lv_color_hex(0xeeeeee);
  const lv_color_t dimBattery = lv_color_hex(0x262626);
  const lv_color_t dialColour = lv_color_hex(0xc8d34f); // the inner dial's yellow-green
  const lv_color_t handDark = lv_color_hex(0x5c5c5c);

  // Point at fraction i/n of the way round the square's perimeter, clockwise from 12 o'clock.
  lv_point_t AlongSquare(int i, int n, int sq) {
    const int perimeter = 8 * sq;
    int d = (i * perimeter / n + sq) % perimeter; // distance from the top-left corner
    const int side = 2 * sq;
    if (d < side) {
      return {static_cast<lv_coord_t>(cx - sq + d), static_cast<lv_coord_t>(cy - sq)};
    }
    d -= side;
    if (d < side) {
      return {static_cast<lv_coord_t>(cx + sq), static_cast<lv_coord_t>(cy - sq + d)};
    }
    d -= side;
    if (d < side) {
      return {static_cast<lv_coord_t>(cx + sq - d), static_cast<lv_coord_t>(cy + sq)};
    }
    d -= side;
    return {static_cast<lv_coord_t>(cx - sq), static_cast<lv_coord_t>(cy + sq - d)};
  }

  lv_obj_t* Tinted(const char* path, lv_coord_t x, lv_coord_t y, lv_color_t colour) {
    lv_obj_t* img = Image(path, x, y);
    lv_obj_set_style_local_image_recolor(img, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, colour);
    lv_obj_set_style_local_image_recolor_opa(img, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
    return img;
  }
}

bool WatchFaceRally::IsAvailable(Pinetime::Controllers::FS& filesystem) {
  return AllExist(filesystem, {fontPath, ticksMinor, ticksMajor, dialTicks});
}

WatchFaceRally::WatchFaceRally(Controllers::DateTime& dateTime,
                               Controllers::Settings& settings,
                               Controllers::MotionController& motion,
                               const Controllers::Battery& battery,
                               Controllers::FS& filesystem)
  : dateTime {dateTime}, settings {settings}, motion {motion}, battery {battery} {
  font = LoadFont(filesystem, fontPath);
  const lv_font_t* digits = Or(font, &jetbrains_mono_bold_20);

  Background(LV_COLOR_BLACK);

  // The band: segment i runs clockwise from 12 o'clock. 0-15 (right half) show the battery,
  // filling upwards from 6 o'clock; 16-31 (left half) show the steps, also from 6 o'clock up.
  for (int i = 0; i < segments; i++) {
    band[i].Create(i < half ? dimBattery : dimSteps, 6, false);
    band[i].Set(AlongSquare(i, segments, bandHalf), AlongSquare(i + 1, segments, bandHalf));
  }
  Rect(lv_scr_act(), cx - 3, 0, 6, 14, lv_color_hex(0xff3b30)); // red mark at 60

  // bezel: ticks (images, no RAM) and numbers; the date sits where 15 would be
  if (FileExists(filesystem, ticksMinor)) {
    Tinted(ticksMinor, 0, 0, lv_color_hex(0x9a9a9a));
    Tinted(ticksMajor, 0, 0, LV_COLOR_WHITE);
  }
  for (int v = 5; v <= 60; v += 5) {
    if (v == 15) {
      continue;
    }
    lv_obj_t* l = Label(digits, LV_COLOR_WHITE);
    lv_label_set_text_fmt(l, v == 5 ? "05" : "%d", v);
    const lv_point_t p = OnSquare(cx, cy, numbersHalf, v * 6);
    lv_obj_set_pos(l, p.x - lv_obj_get_width(l) / 2, p.y - lv_obj_get_height(l) / 2);
  }
  lv_obj_t* dateBox = Rect(lv_scr_act(), cx + numbersHalf - 16, cy - 10, 32, 20, LV_COLOR_WHITE, 2);
  date = Label(digits, LV_COLOR_BLACK, dateBox);

  // the inner dial, in the concept's yellow-green
  lv_obj_t* dial = Rect(lv_scr_act(), cx - dialHalf, cy - dialHalf, 2 * dialHalf, 2 * dialHalf, LV_COLOR_BLACK, 12);
  lv_obj_set_style_local_bg_opa(dial, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
  lv_obj_set_style_local_border_width(dial, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 2);
  lv_obj_set_style_local_border_color(dial, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, dialColour);
  if (FileExists(filesystem, dialTicks)) {
    Tinted(dialTicks, cx - dialHalf - 4, cy - dialHalf - 4, dialColour);
  }
  for (int h = 1; h <= 12; h++) {
    lv_obj_t* l = Label(digits, dialColour);
    lv_label_set_text_fmt(l, "%d", h);
    const lv_point_t p = OnSquare(cx, cy, dialNumbersHalf, h * 30);
    lv_obj_set_pos(l, p.x - lv_obj_get_width(l) / 2, p.y - lv_obj_get_height(l) / 2);
  }

  // hands: an arrow-tipped hour hand on a dark base, a long minute hand with a round
  // counterweight, and a thin dark second hand
  secondHand.Create(handDark, 2);
  minuteTail.Create(handDark, 3);
  minuteWeight = Circle(0, 0, 7, handDark);
  hourBase.Create(handDark, 7);
  hourBlade.Create(LV_COLOR_WHITE, 11, false);
  hourTip.Create(LV_COLOR_WHITE, 5);
  minuteHand.Create(LV_COLOR_WHITE, 5);
  Circle(cx, cy, 5, handDark);
  Circle(cx, cy, 2, LV_COLOR_BLACK);

  refreshTask = lv_task_create(RefreshTaskCallback, 250, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceRally::~WatchFaceRally() {
  lv_task_del(refreshTask);
  lv_obj_clean(lv_scr_act());
  FreeFont(font);
}

void WatchFaceRally::Refresh() {
  minute = dateTime.Minutes();
  if (minute.IsUpdated()) {
    const int m = minute.Get();
    const int hourAngle = (dateTime.Hours() % 12) * 30 + m / 2;
    // the hour hand: a dark base, a broad white blade and a narrower tip, like an arrow
    hourBase.Hand(cx, cy, hourAngle, 20, 6);
    hourBlade.Set(Polar(cx, cy, 20, hourAngle), Polar(cx, cy, 36, hourAngle));
    hourTip.Set(Polar(cx, cy, 36, hourAngle), Polar(cx, cy, 44, hourAngle));
    // the minute hand reaches out to the bezel numbers, with a round counterweight behind
    minuteHand.Hand(cx, cy, m * 6, 76);
    minuteTail.Hand(cx, cy, m * 6 + 180, 24);
    const lv_point_t w = Polar(cx, cy, 24, m * 6 + 180);
    lv_obj_set_pos(minuteWeight, w.x - 7, w.y - 7);
    lv_label_set_text_fmt(date, "%d", dateTime.Day());
    lv_obj_align(date, nullptr, LV_ALIGN_CENTER, 0, 1);
  }
  second = dateTime.Seconds();
  if (second.IsUpdated()) {
    secondHand.Hand(cx, cy, second.Get() * 6, 88, 12);
  }

  // steps: left half, from 6 o'clock (segment 16) up to 12 (segment 31); yellow once past 60 %
  const uint32_t goal = settings.GetStepsGoal();
  const uint32_t steps = motion.NbSteps();
  stepSegments = goal == 0 ? 0 : static_cast<int>(std::min<uint32_t>(steps, goal) * half / goal);
  if (stepSegments.IsUpdated()) {
    const int lit = stepSegments.Get();
    for (int k = 0; k < half; k++) {
      const lv_color_t c = k < lit ? (k < half * 6 / 10 ? green : yellow) : dimSteps;
      lv_obj_set_style_local_line_color(band[half + k].obj, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, c);
    }
  }

  // battery: right half, from 6 o'clock (segment 15) up to 12 (segment 0)
  const uint8_t pct = battery.PercentRemaining() > 100 ? 100 : battery.PercentRemaining();
  batterySegments = (pct * half + 50) / 100;
  if (batterySegments.IsUpdated()) {
    const int lit = batterySegments.Get();
    for (int k = 0; k < half; k++) {
      lv_obj_set_style_local_line_color(band[half - 1 - k].obj, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, k < lit ? white : dimBattery);
    }
  }
}
