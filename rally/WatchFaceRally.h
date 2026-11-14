#pragma once

#include <array>
#include <chrono>
#include <lvgl/lvgl.h>

#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/fs/FS.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"
#include "utility/DirtyValue.h"
#include "../common/FaceKit.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      // Rally: a small analog clock inside a numbered bezel. The track around the edge fills from
      // green to yellow as you walk towards your daily step goal. Font: /fonts/rally_digits.bin.
      class WatchFaceRally : public Screen {
      public:
        WatchFaceRally(Controllers::DateTime& dateTime,
                       Controllers::Settings& settings,
                       Controllers::MotionController& motion,
                       Controllers::FS& filesystem);
        ~WatchFaceRally() override;

        void Refresh() override;

        static bool IsAvailable(Controllers::FS& filesystem);

      private:
        static constexpr int segments = 32;

        Controllers::DateTime& dateTime;
        Controllers::Settings& settings;
        Controllers::MotionController& motion;

        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>> currentMinute {};
        Utility::DirtyValue<int> litSegments {};

        lv_font_t* font = nullptr;
        std::array<ConceptFaces::Line, segments> track;
        ConceptFaces::Line hourHand;
        ConceptFaces::Line minuteHand;
        lv_obj_t* date;
        lv_task_t* refreshTask;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Rally> {
      static constexpr WatchFace watchFace = WatchFace::Rally;
      static constexpr const char* name = "Rally";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceRally(controllers.dateTimeController,
                                           controllers.settingsController,
                                           controllers.motionController,
                                           controllers.filesystem);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& filesystem) {
        return Screens::WatchFaceRally::IsAvailable(filesystem);
      }
    };
  }
}
