#pragma once

#include <array>
#include <chrono>
#include <optional>
#include <lvgl/lvgl.h>

#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "displayapp/screens/Screen.h"
#include "components/battery/BatteryController.h"
#include "components/ble/SimpleWeatherService.h"
#include "components/datetime/DateTimeController.h"
#include "components/fs/FS.h"
#include "components/settings/Settings.h"
#include "utility/DirtyValue.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      // Outlook: large time and date, then the weather and battery, and the next days' forecast.
      // Fonts: /fonts/outlook_{time,temp,text}.bin.
      class WatchFaceOutlook : public Screen {
      public:
        WatchFaceOutlook(Controllers::DateTime& dateTime,
                         Controllers::Settings& settings,
                         const Controllers::Battery& battery,
                         Controllers::SimpleWeatherService* weather,
                         Controllers::FS& filesystem);
        ~WatchFaceOutlook() override;

        void Refresh() override;

        static bool IsAvailable(Controllers::FS& filesystem);

      private:
        void UpdateWeather();

        Controllers::DateTime& dateTime;
        Controllers::Settings& settings;
        const Controllers::Battery& battery;
        Controllers::SimpleWeatherService* weather;

        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::minutes>> currentMinute {};
        Utility::DirtyValue<std::optional<Controllers::SimpleWeatherService::CurrentWeather>> currentWeather {};
        Utility::DirtyValue<std::optional<Controllers::SimpleWeatherService::Forecast>> forecast {};
        Utility::DirtyValue<uint8_t> batteryPercent {};

        lv_font_t* fontTime = nullptr;
        lv_font_t* fontTemp = nullptr;
        lv_font_t* fontText = nullptr;

        lv_obj_t* time;
        lv_obj_t* date;
        lv_obj_t* weekday;
        lv_obj_t* icon;
        lv_obj_t* temperature;
        lv_obj_t* batteryText;
        lv_obj_t* batteryFill;
        std::array<lv_obj_t*, 3> forecastIcons;
        lv_point_t curve[9];
        lv_task_t* refreshTask;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Outlook> {
      static constexpr WatchFace watchFace = WatchFace::Outlook;
      static constexpr const char* name = "Outlook";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceOutlook(controllers.dateTimeController,
                                             controllers.settingsController,
                                             controllers.batteryController,
                                             controllers.weatherController,
                                             controllers.filesystem);
      }

      static bool IsAvailable(Pinetime::Controllers::FS& filesystem) {
        return Screens::WatchFaceOutlook::IsAvailable(filesystem);
      }
    };
  }
}
