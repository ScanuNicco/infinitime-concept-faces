#pragma once

#include <array>
#include <chrono>
#include <lvgl/lvgl.h>

#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "utility/DirtyValue.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      // Words: a word clock. The time is spelled out in white ("IT IS TEN PAST THREE") and the
      // date in yellow, among dim filler letters. Uses only built-in fonts.
      class WatchFaceWords : public Screen {
      public:
        explicit WatchFaceWords(Controllers::DateTime& dateTime);
        ~WatchFaceWords() override;

        void Refresh() override;

      private:
        static constexpr int rows = 7;
        static constexpr int cols = 10;

        Controllers::DateTime& dateTime;
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>> currentMinute {};
        std::array<lv_obj_t*, rows> lines;
        lv_task_t* refreshTask;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Words> {
      static constexpr WatchFace watchFace = WatchFace::Words;
      static constexpr const char* name = "Words";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceWords(controllers.dateTimeController);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
