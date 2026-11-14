#pragma once

#include <chrono>
#include <lvgl/lvgl.h>

#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "displayapp/screens/Screen.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/datetime/DateTimeController.h"
#include "components/fs/FS.h"
#include "utility/DirtyValue.h"
#include "../common/FaceKit.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      // Glow: a minimal analog face with cyan hands, a red second hand and a fine tick ring,
      // plus the date and small battery / Bluetooth indicators. Font: /fonts/glow_text.bin.
      class WatchFaceGlow : public Screen {
      public:
        WatchFaceGlow(Controllers::DateTime& dateTime,
                      const Controllers::Battery& battery,
                      const Controllers::Ble& ble,
                      Controllers::FS& filesystem);
        ~WatchFaceGlow() override;

        void Refresh() override;

        static bool IsAvailable(Controllers::FS& filesystem);

      private:
        Controllers::DateTime& dateTime;
        const Controllers::Battery& battery;
        const Controllers::Ble& ble;

        Utility::DirtyValue<uint8_t> second {};
        Utility::DirtyValue<uint8_t> minute {};
        Utility::DirtyValue<uint8_t> batteryPercent {};
        Utility::DirtyValue<bool> connected {};

        lv_font_t* font = nullptr;
        ConceptFaces::Line hourHand;
        ConceptFaces::Line minuteHand;
        ConceptFaces::Line secondHand;
        lv_obj_t* date;
        lv_obj_t* batteryFill;
        lv_obj_t* bluetooth;
        lv_task_t* refreshTask;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Glow> {
      static constexpr WatchFace watchFace = WatchFace::Glow;
      static constexpr const char* name = "Glow";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceGlow(controllers.dateTimeController,
                                          controllers.batteryController,
                                          controllers.bleController,
                                          controllers.filesystem);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& filesystem) {
        return Screens::WatchFaceGlow::IsAvailable(filesystem);
      }
    };
  }
}
