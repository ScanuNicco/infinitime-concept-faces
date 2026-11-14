#pragma once

#include <chrono>
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

      // Horizon: time and date over a dome. The sun travels along the dome from today's sunrise to
      // sunset (from the weather data; 6:00-18:00 without it) and the moon crosses it at night.
      // Uses fonts /fonts/horizon_{time,day,small}.bin. The dome, its dotted arc, the sun and the moon
      // are drawn with LVGL shapes (no images: decoding those from flash made the face slow to open).
      class WatchFaceHorizon : public Screen {
      public:
        WatchFaceHorizon(Controllers::DateTime& dateTime,
                         Controllers::Settings& settings,
                         Controllers::FS& filesystem,
                         Controllers::SimpleWeatherService* weather);
        ~WatchFaceHorizon() override;

        void Refresh() override;

        static bool IsAvailable(Controllers::FS& filesystem);

      private:
        void UpdateTime();
        void UpdateWeather();
        void UpdateSky();

        Controllers::DateTime& dateTime;
        Controllers::Settings& settings;
        Controllers::SimpleWeatherService* weather;

        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>> currentMinute {};
        Utility::DirtyValue<std::optional<Controllers::SimpleWeatherService::CurrentWeather>> currentWeather {};

        lv_font_t* fontTime = nullptr;
        lv_font_t* fontDay = nullptr;
        lv_font_t* fontSmall = nullptr;

        lv_obj_t* date;
        lv_obj_t* weekday;
        lv_obj_t* time;
        static lv_design_res_t DrawDots(lv_obj_t* layer, const lv_area_t* clip, lv_design_mode_t mode);

        lv_obj_t* sunGlow;
        lv_obj_t* sun;
        lv_obj_t* moon;
        lv_obj_t* moonBite;
        lv_obj_t* temperature;
        lv_obj_t* location;
        lv_task_t* refreshTask;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Horizon> {
      static constexpr WatchFace watchFace = WatchFace::Horizon;
      static constexpr const char* name = "Horizon";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceHorizon(controllers.dateTimeController,
                                             controllers.settingsController,
                                             controllers.filesystem,
                                             controllers.weatherController);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& filesystem) {
        return Screens::WatchFaceHorizon::IsAvailable(filesystem);
      }
    };
  }
}
