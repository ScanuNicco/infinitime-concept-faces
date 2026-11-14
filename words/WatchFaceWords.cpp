#include "WatchFaceWords.h"

#include <cstdio>
#include <cstring>

#include "../common/FaceKit.h"

using namespace Pinetime::Applications::Screens;
using namespace ConceptFaces;

extern lv_font_t jetbrains_mono_bold_20;

namespace {
  constexpr const char* lit = "ffffff";
  constexpr const char* dateLit = "ffd60a";
  constexpr const char* dim = "333333";
  constexpr char fillers[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHKLMNPRSTUVWXYZ0123456789";

  constexpr const char* hourWords[] = {"TWELVE", "ONE", "TWO", "THREE", "FOUR", "FIVE",
                                       "SIX", "SEVEN", "EIGHT", "NINE", "TEN", "ELEVEN"};

  struct Rng {
    uint32_t state;

    uint32_t Next() {
      state ^= state << 13;
      state ^= state >> 17;
      state ^= state << 5;
      return state;
    }
  };

  // One row of the grid: lit words placed among filler letters.
  struct Row {
    char letters[11] = {};
    bool on[10] = {};
    const char* colour = lit;

    void Fill(Rng& rng) {
      for (int i = 0; i < 10; i++) {
        letters[i] = fillers[rng.Next() % (sizeof(fillers) - 1)];
        on[i] = false;
      }
    }

    void Put(int at, const char* word) {
      for (int i = 0; word[i] != '\0' && at + i < 10; i++) {
        letters[at + i] = word[i];
        on[at + i] = true;
      }
    }

    // Place words at a random spot, keeping them in order with at least one letter between them.
    void PutWords(Rng& rng, const char* a, const char* b = nullptr) {
      const int la = static_cast<int>(strlen(a));
      const int lb = b != nullptr ? static_cast<int>(strlen(b)) : 0;
      const int room = 10 - la - (b != nullptr ? lb + 1 : 0);
      const int start = room > 0 ? static_cast<int>(rng.Next() % static_cast<uint32_t>(room + 1)) : 0;
      Put(start, a);
      if (b != nullptr) {
        const int gapRoom = 10 - start - la - 1 - lb;
        Put(start + la + 1 + (gapRoom > 0 ? static_cast<int>(rng.Next() % static_cast<uint32_t>(gapRoom + 1)) : 0), b);
      }
    }

    // LVGL recolour text: "#rrggbb LETTERS#" runs.
    void Render(lv_obj_t* label) const {
      char text[10 * 12 + 1];
      size_t n = 0;
      for (int i = 0; i < 10;) {
        int j = i;
        while (j < 10 && on[j] == on[i]) {
          j++;
        }
        n += static_cast<size_t>(snprintf(text + n, sizeof(text) - n, "#%s %.*s#", on[i] ? colour : dim, j - i, letters + i));
        i = j;
      }
      lv_label_set_text(label, text);
    }
  };
}

WatchFaceWords::WatchFaceWords(Controllers::DateTime& dateTime) : dateTime {dateTime} {
  Background(LV_COLOR_BLACK);
  for (int r = 0; r < rows; r++) {
    lines[r] = Label(&jetbrains_mono_bold_20, LV_COLOR_WHITE);
    lv_label_set_recolor(lines[r], true);
    // the monospace font is 12 px wide; spacing spreads the 10 letters over the screen
    lv_obj_set_style_local_text_letter_space(lines[r], LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 11);
    lv_obj_set_pos(lines[r], 8, static_cast<lv_coord_t>(14 + r * 31));
  }
  refreshTask = lv_task_create(RefreshTaskCallback, 1000, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceWords::~WatchFaceWords() {
  lv_task_del(refreshTask);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceWords::Refresh() {
  currentMinute = std::chrono::time_point_cast<std::chrono::minutes>(dateTime.CurrentDateTime());
  if (!currentMinute.IsUpdated()) {
    return;
  }
  const int minute = dateTime.Minutes();
  int hour = dateTime.Hours();
  const int five = minute / 5; // word clocks count in five-minute steps
  Rng rng {static_cast<uint32_t>(hour * 60 + minute) * 2654435761u + 1u};

  Row grid[rows];
  for (auto& row : grid) {
    row.Fill(rng);
  }
  grid[0].PutWords(rng, "IT", "IS");

  static constexpr const char* minuteWords[] = {nullptr, "FIVE", "TEN", "QUARTER", "TWENTY", "TWENTYFIVE", "HALF"};
  if (five > 6) {
    hour++; // "TWENTY TO FOUR"
  }
  if (five != 0) {
    grid[1].PutWords(rng, minuteWords[five <= 6 ? five : 12 - five]);
    grid[2].PutWords(rng, five <= 6 ? "PAST" : "TO");
  }
  grid[3].PutWords(rng, hourWords[hour % 12]);
  if (five == 0) {
    grid[4].PutWords(rng, "OCLOCK");
  }

  char day[12];
  snprintf(day, sizeof(day), "%d", dateTime.Day());
  char year[12];
  snprintf(year, sizeof(year), "%04d", dateTime.Year() % 10000);
  grid[5].colour = dateLit;
  grid[5].PutWords(rng, day, dateTime.MonthShortToString());
  grid[6].colour = dateLit;
  grid[6].PutWords(rng, dateTime.DayOfWeekShortToString(), year);

  for (int r = 0; r < rows; r++) {
    grid[r].Render(lines[r]);
  }
}
