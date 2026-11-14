#pragma once

#include <array>
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
#include "../common/FaceKit.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      // Constellation: the hour and minute are two stars joined to the centre like a constellation,
      // on a starry sky, with the weather above and the date below.
      // Font /fonts/constellation_text.bin. The starry sky is drawn by a draw layer (see FaceKit.h).
      class WatchFaceConstellation : public Screen {
      public:
        WatchFaceConstellation(Controllers::DateTime& dateTime,
                               Controllers::Settings& settings,
                               Controllers::SimpleWeatherService* weather,
                               Controllers::FS& filesystem);
        ~WatchFaceConstellation() override;

        void Refresh() override;

        static bool IsAvailable(Controllers::FS& filesystem);

      private:
        struct Star {
          uint8_t x;
          uint8_t y;
          uint8_t size; // 0: a point, 1: two points, 2: a small cross (brighter)
        };

        static lv_design_res_t Draw(lv_obj_t* layer, const lv_area_t* clip, lv_design_mode_t mode);

        Controllers::DateTime& dateTime;
        Controllers::Settings& settings;
        Controllers::SimpleWeatherService* weather;

        Utility::DirtyValue<uint8_t> minute {};
        Utility::DirtyValue<std::optional<Controllers::SimpleWeatherService::CurrentWeather>> currentWeather {};

        lv_font_t* font = nullptr;
        std::array<Star, 70> stars;
        ConceptFaces::Line toHour;
        ConceptFaces::Line toMinute;
        lv_obj_t* hourStar;
        lv_obj_t* minuteStar;
        lv_obj_t* conditions;
        lv_obj_t* date;
        lv_task_t* refreshTask;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Constellation> {
      static constexpr WatchFace watchFace = WatchFace::Constellation;
      static constexpr const char* name = "Constellation";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceConstellation(controllers.dateTimeController,
                                                   controllers.settingsController,
                                                   controllers.weatherController,
                                                   controllers.filesystem);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& filesystem) {
        return Screens::WatchFaceConstellation::IsAvailable(filesystem);
      }
    };
  }
}
