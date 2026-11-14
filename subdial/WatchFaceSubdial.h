#pragma once

#include <array>
#include <lvgl/lvgl.h>

#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/fs/FS.h"
#include "utility/DirtyValue.h"
#include "../common/FaceKit.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      // Subdial: a dress-watch face with a small-seconds sub-dial, a date window and the weekday.
      // Font: /fonts/subdial_text.bin.
      class WatchFaceSubdial : public Screen {
      public:
        WatchFaceSubdial(Controllers::DateTime& dateTime, Controllers::FS& filesystem);
        ~WatchFaceSubdial() override;

        void Refresh() override;

        static bool IsAvailable(Controllers::FS& filesystem);

      private:
        Controllers::DateTime& dateTime;
        Utility::DirtyValue<uint8_t> second {};
        Utility::DirtyValue<uint8_t> minute {};

        lv_font_t* font = nullptr;
        std::array<ConceptFaces::Line, 10> marks; // every hour but 3 (date) and 6 (sub-dial)
        ConceptFaces::Line hourHand;
        ConceptFaces::Line minuteHand;
        ConceptFaces::Line secondHand;
        lv_obj_t* weekday;
        lv_obj_t* date;
        lv_task_t* refreshTask;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Subdial> {
      static constexpr WatchFace watchFace = WatchFace::Subdial;
      static constexpr const char* name = "Subdial";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceSubdial(controllers.dateTimeController, controllers.filesystem);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& filesystem) {
        return Screens::WatchFaceSubdial::IsAvailable(filesystem);
      }
    };
  }
}
