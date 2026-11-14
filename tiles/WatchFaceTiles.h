#pragma once

#include <array>
#include <chrono>
#include <lvgl/lvgl.h>

#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/fs/FS.h"
#include "components/settings/Settings.h"
#include "utility/DirtyValue.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      // Tiles: the time as four big digits on dark red tiles, like a flip clock.
      // Uses the fonts /fonts/tiles_digits.bin and /fonts/tiles_text.bin from the resource pack.
      class WatchFaceTiles : public Screen {
      public:
        WatchFaceTiles(Controllers::DateTime& dateTime, Controllers::Settings& settings, Controllers::FS& filesystem);
        ~WatchFaceTiles() override;

        void Refresh() override;

        static bool IsAvailable(Controllers::FS& filesystem);

      private:
        Controllers::DateTime& dateTime;
        Controllers::Settings& settings;

        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>> currentMinute {};

        lv_font_t* fontDigits = nullptr;
        lv_font_t* fontText = nullptr;

        lv_obj_t* date;
        lv_obj_t* weekday;
        lv_obj_t* ampm;
        std::array<lv_obj_t*, 4> tiles;
        std::array<lv_obj_t*, 4> digits;
        lv_task_t* refreshTask;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Tiles> {
      static constexpr WatchFace watchFace = WatchFace::Tiles;
      static constexpr const char* name = "Tiles";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceTiles(controllers.dateTimeController, controllers.settingsController, controllers.filesystem);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& filesystem) {
        return Screens::WatchFaceTiles::IsAvailable(filesystem);
      }
    };
  }
}
