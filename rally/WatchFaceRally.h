#pragma once

#include <array>
#include <lvgl/lvgl.h>

#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "displayapp/screens/Screen.h"
#include "components/battery/BatteryController.h"
#include "components/datetime/DateTimeController.h"
#include "components/fs/FS.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"
#include "utility/DirtyValue.h"
#include "../common/FaceKit.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      // Rally: a small analog clock inside a numbered, ticked bezel. A band round the edge of the
      // screen shows your steps (left half, green then yellow near the goal) and the battery (right
      // half, white). Font /fonts/rally_digits.bin; tick marks are 1-bit images
      // /images/rally_{ticks_minor,ticks_major,dial_ticks}.bin.
      class WatchFaceRally : public Screen {
      public:
        WatchFaceRally(Controllers::DateTime& dateTime,
                       Controllers::Settings& settings,
                       Controllers::MotionController& motion,
                       const Controllers::Battery& battery,
                       Controllers::FS& filesystem);
        ~WatchFaceRally() override;

        void Refresh() override;

        static bool IsAvailable(Controllers::FS& filesystem);

      private:
        static constexpr int segments = 32; // round the whole edge; 16 per gauge

        Controllers::DateTime& dateTime;
        Controllers::Settings& settings;
        Controllers::MotionController& motion;
        const Controllers::Battery& battery;

        Utility::DirtyValue<uint8_t> minute {};
        Utility::DirtyValue<uint8_t> second {};
        Utility::DirtyValue<int> stepSegments {};
        Utility::DirtyValue<int> batterySegments {};

        lv_font_t* font = nullptr;
        std::array<ConceptFaces::Line, segments> band;
        ConceptFaces::Line hourBase;
        ConceptFaces::Line hourBlade;
        ConceptFaces::Line hourTip;
        ConceptFaces::Line minuteHand;
        ConceptFaces::Line minuteTail;
        ConceptFaces::Line secondHand;
        lv_obj_t* minuteWeight;
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
                                           controllers.batteryController,
                                           controllers.filesystem);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& filesystem) {
        return Screens::WatchFaceRally::IsAvailable(filesystem);
      }
    };
  }
}
