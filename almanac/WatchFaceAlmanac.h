#pragma once

#include <optional>
#include <lvgl/lvgl.h>

#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "displayapp/screens/Screen.h"
#include "components/ble/SimpleWeatherService.h"
#include "components/datetime/DateTimeController.h"
#include "components/fs/FS.h"
#include "components/settings/Settings.h"
#include "utility/DirtyValue.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      // Almanac: the time as two rulers, hours above and minutes below, with the current value
      // large in the middle. Weather above, date below. Fonts: /fonts/almanac_{big,small}.bin.
      class WatchFaceAlmanac : public Screen {
      public:
        WatchFaceAlmanac(Controllers::DateTime& dateTime,
                         Controllers::Settings& settings,
                         Controllers::SimpleWeatherService* weather,
                         Controllers::FS& filesystem);
        ~WatchFaceAlmanac() override;

        void Refresh() override;

        static bool IsAvailable(Controllers::FS& filesystem);

      private:
        struct Ruler {
          lv_obj_t* before;
          lv_obj_t* current;
          lv_obj_t* after;
        };

        void SetRuler(Ruler& ruler, int value, int count, int first, bool twoDigits, lv_coord_t y);

        Controllers::DateTime& dateTime;
        Controllers::Settings& settings;
        Controllers::SimpleWeatherService* weather;

        Utility::DirtyValue<uint8_t> minute {};
        Utility::DirtyValue<std::optional<Controllers::SimpleWeatherService::CurrentWeather>> currentWeather {};

        lv_font_t* fontBig = nullptr;
        lv_font_t* fontSmall = nullptr;
        Ruler hours;
        Ruler minutes;
        lv_obj_t* icon;
        lv_obj_t* temperature;
        lv_obj_t* date;
        lv_task_t* refreshTask;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Almanac> {
      static constexpr WatchFace watchFace = WatchFace::Almanac;
      static constexpr const char* name = "Almanac";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceAlmanac(controllers.dateTimeController,
                                             controllers.settingsController,
                                             controllers.weatherController,
                                             controllers.filesystem);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& filesystem) {
        return Screens::WatchFaceAlmanac::IsAvailable(filesystem);
      }
    };
  }
}
