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

      // Noir: long white hands and a red second hand on black, hour marks and the date at three.
      // Font: /fonts/noir_text.bin.
      class WatchFaceNoir : public Screen {
      public:
        WatchFaceNoir(Controllers::DateTime& dateTime, Controllers::FS& filesystem);
        ~WatchFaceNoir() override;

        void Refresh() override;

        static bool IsAvailable(Controllers::FS& filesystem);

      private:
        Controllers::DateTime& dateTime;
        Utility::DirtyValue<uint8_t> second {};
        Utility::DirtyValue<uint8_t> minute {};

        lv_font_t* font = nullptr;
        std::array<ConceptFaces::Line, 10> marks; // 1, 2, 4..11 o'clock
        ConceptFaces::Line twelveLeft;
        ConceptFaces::Line twelveRight;
        ConceptFaces::Line hourHand;
        ConceptFaces::Line minuteHand;
        ConceptFaces::Line secondHand;
        lv_obj_t* date;
        lv_task_t* refreshTask;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Noir> {
      static constexpr WatchFace watchFace = WatchFace::Noir;
      static constexpr const char* name = "Noir";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceNoir(controllers.dateTimeController, controllers.filesystem);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& filesystem) {
        return Screens::WatchFaceNoir::IsAvailable(filesystem);
      }
    };
  }
}
