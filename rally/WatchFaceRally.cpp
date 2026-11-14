#include "WatchFaceRally.h"

#include <algorithm>

using namespace Pinetime::Applications::Screens;
using namespace ConceptFaces;

extern lv_font_t jetbrains_mono_bold_20;

namespace {
  constexpr const char* fontPath = "/fonts/rally_digits.bin";

  constexpr lv_coord_t cx = 120;
  constexpr lv_coord_t cy = 120;
  constexpr int bandHalf = 116;  // the band runs round the very edge of the screen
  constexpr int numbersHalf = 86;
  constexpr int ticksHalf = 110; // bezel ticks end on this square, inside the band
  constexpr int dialHalf = 58;
  constexpr int dialNumbersHalf = 45;
  constexpr int half = 16;       // segments per gauge

  const lv_color_t green = lv_color_hex(0x8cc63f);
  const lv_color_t yellow = lv_color_hex(0xf0e400);
  const lv_color_t dimSteps = lv_color_hex(0x262d12);
  const lv_color_t white = lv_color_hex(0xeeeeee);
  const lv_color_t dimBattery = lv_color_hex(0x262626);
  const lv_color_t dialColour = lv_color_hex(0xc8d34f); // the inner dial's yellow-green
  const lv_color_t handDark = lv_color_hex(0x5c5c5c);
  const lv_color_t minorTick = lv_color_hex(0x9a9a9a);

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

}

bool WatchFaceRally::IsAvailable(Pinetime::Controllers::FS& filesystem) {
  return FileExists(filesystem, fontPath);
}

// Draws the band and all the ticks, clipped to the part of the screen LVGL is redrawing.
lv_design_res_t WatchFaceRally::Draw(lv_obj_t* layer, const lv_area_t* clip, lv_design_mode_t mode) {
  if (mode != LV_DESIGN_DRAW_MAIN) {
    return NotDrawing(mode);
  }
  const WatchFaceRally* face = Owner<WatchFaceRally>(layer);
  // band: segments 0-15 (right half) battery from 6 o'clock up, 16-31 (left half) steps from 6 o'clock up
  for (int i = 0; i < segments; i++) {
    lv_color_t c;
    if (i < half) {
      c = (half - 1 - i) < face->batteryLit ? white : dimBattery;
    } else {
      const int k = i - half;
      c = k < face->stepsLit ? (k < half * 6 / 10 ? green : yellow) : dimSteps;
    }
    DrawLine(clip, face->band[i], face->band[i + 1], c, 6);
  }
  for (const Tick& t : face->ticks) {
    DrawLine(clip, t.from, t.to, t.major ? LV_COLOR_WHITE : minorTick, t.width);
  }
  return LV_DESIGN_RES_OK;
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

  // Precompute the band and the ticks; the draw layer just draws them.
  // band: point i is i/32 of the way round the edge, clockwise from 12 o'clock
  for (int i = 0; i <= segments; i++) {
    band[i] = AlongSquare(i, segments, bandHalf);
  }
  size_t n = 0;
  for (int m = 1; m < 60; m++) { // bezel: every minute, longer and bolder every five
    const bool major = m % 5 == 0;
    const int r = SquareRadius(ticksHalf, m * 6);
    ticks[n++] = {Polar(cx, cy, r - (major ? 14 : 8), m * 6), Polar(cx, cy, r, m * 6), static_cast<uint8_t>(major ? 3 : 2), major};
  }
  for (int h = 0; h < 12; h++) { // across the inner dial's outline
    const int r = SquareRadius(dialHalf - 1, h * 30);
    ticks[n++] = {Polar(cx, cy, r - 4, h * 30), Polar(cx, cy, r + 3, h * 30), 2, false};
  }
  layer = DrawLayer(Draw, this);
  Rect(lv_scr_act(), cx - 3, 0, 6, 14, lv_color_hex(0xff3b30)); // red mark at 60

  // bezel numbers; the date sits where 15 would be
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
  bool bandChanged = false;
  if (stepSegments.IsUpdated()) {
    stepsLit = stepSegments.Get();
    bandChanged = true;
  }

  // battery: right half, from 6 o'clock (segment 15) up to 12 (segment 0)
  const uint8_t pct = battery.PercentRemaining() > 100 ? 100 : battery.PercentRemaining();
  batterySegments = (pct * half + 50) / 100;
  if (batterySegments.IsUpdated()) {
    batteryLit = batterySegments.Get();
    bandChanged = true;
  }
  if (bandChanged) {
    lv_obj_invalidate(layer); // redrawn only when the step or battery level changes
  }
}
