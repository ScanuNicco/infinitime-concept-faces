#pragma once

// Small helpers shared by the Concept Faces watch faces.

#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <lvgl/lvgl.h>

#include "components/ble/SimpleWeatherService.h"
#include "components/fs/FS.h"
#include "components/settings/Settings.h"

namespace ConceptFaces {

  constexpr int32_t trigScale = 32767; // _lv_trigo_sin(90)

  inline bool FileExists(Pinetime::Controllers::FS& fs, const char* path) {
    lfs_file f = {};
    if (fs.FileOpen(&f, path, LFS_O_RDONLY) < 0) {
      return false;
    }
    fs.FileClose(&f);
    return true;
  }

  inline bool AllExist(Pinetime::Controllers::FS& fs, std::initializer_list<const char*> paths) {
    for (const char* p : paths) {
      if (!FileExists(fs, p)) {
        return false;
      }
    }
    return true;
  }

  // Load a font from the resource pack (path like "/fonts/x.bin"), or nullptr if it isn't installed.
  inline lv_font_t* LoadFont(Pinetime::Controllers::FS& fs, const char* path) {
    if (!FileExists(fs, path)) {
      return nullptr;
    }
    char lvPath[48];
    snprintf(lvPath, sizeof(lvPath), "F:%s", path);
    return lv_font_load(lvPath);
  }

  // Free a loaded font. Call after lv_obj_clean(), when nothing uses it any more.
  inline void FreeFont(lv_font_t*& font) {
    if (font != nullptr) {
      lv_font_free(font);
      font = nullptr;
    }
  }

  inline const lv_font_t* Or(const lv_font_t* font, const lv_font_t* fallback) {
    return font != nullptr ? font : fallback;
  }

  inline lv_obj_t* Rect(lv_obj_t* parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, lv_color_t colour, lv_coord_t radius = 0) {
    lv_obj_t* o = lv_obj_create(parent, nullptr);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_local_bg_color(o, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, colour);
    lv_obj_set_style_local_radius(o, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, radius);
    lv_obj_set_style_local_border_width(o, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_click(o, false);
    return o;
  }

  inline lv_obj_t* Background(lv_color_t colour) {
    return Rect(lv_scr_act(), 0, 0, LV_HOR_RES, LV_VER_RES, colour);
  }

  inline lv_obj_t* Circle(lv_coord_t cx, lv_coord_t cy, lv_coord_t r, lv_color_t colour) {
    return Rect(lv_scr_act(), cx - r, cy - r, 2 * r, 2 * r, colour, LV_RADIUS_CIRCLE);
  }

  inline lv_obj_t* Label(const lv_font_t* font, lv_color_t colour, lv_obj_t* parent = nullptr) {
    lv_obj_t* l = lv_label_create(parent != nullptr ? parent : lv_scr_act(), nullptr);
    lv_obj_set_style_local_text_font(l, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, font);
    lv_obj_set_style_local_text_color(l, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, colour);
    return l;
  }

  // An image from the resource pack (path like "/images/x.bin").
  inline lv_obj_t* Image(const char* path, lv_coord_t x, lv_coord_t y) {
    char lvPath[48];
    snprintf(lvPath, sizeof(lvPath), "F:%s", path);
    lv_obj_t* img = lv_img_create(lv_scr_act(), nullptr);
    lv_img_set_src(img, lvPath); // LVGL copies the path
    lv_obj_set_pos(img, x, y);
    return img;
  }

  // Point at distance r from (cx, cy), angle in degrees clockwise from 12 o'clock. Integer maths only.
  inline lv_point_t Polar(lv_coord_t cx, lv_coord_t cy, int32_t r, int32_t degrees) {
    const int16_t a = static_cast<int16_t>(((degrees % 360) + 360) % 360);
    const int32_t sin = _lv_trigo_sin(a);
    const int32_t cos = _lv_trigo_sin(static_cast<int16_t>((a + 90) % 360));
    return {static_cast<lv_coord_t>(cx + r * sin / trigScale), static_cast<lv_coord_t>(cy - r * cos / trigScale)};
  }

  // A straight line (clock hand, connector...). Keep it as a member: LVGL keeps a pointer to the points.
  struct Line {
    lv_obj_t* obj = nullptr;
    lv_point_t points[2] {};

    void Create(lv_color_t colour, lv_style_int_t width, bool rounded = true) {
      obj = lv_line_create(lv_scr_act(), nullptr);
      lv_obj_set_style_local_line_width(obj, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, width);
      lv_obj_set_style_local_line_color(obj, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, colour);
      lv_obj_set_style_local_line_rounded(obj, LV_LINE_PART_MAIN, LV_STATE_DEFAULT, rounded);
    }

    void Set(lv_point_t from, lv_point_t to) {
      points[0] = from;
      points[1] = to;
      lv_line_set_points(obj, points, 2);
    }

    // A clock hand from (cx, cy): `back` pixels behind the centre to `length` pixels out.
    void Hand(lv_coord_t cx, lv_coord_t cy, int32_t degrees, int32_t length, int32_t back = 0) {
      Set(Polar(cx, cy, -back, degrees), Polar(cx, cy, length, degrees));
    }
  };

  // A ring of `count` tick marks, `length` long, ending `inset` pixels inside a circle of `size`
  // pixels centred at (cx, cy). The first tick is at 12 o'clock.
  inline lv_obj_t* Ticks(lv_coord_t cx, lv_coord_t cy, lv_coord_t size, uint16_t count, lv_style_int_t length, lv_style_int_t width,
                         lv_color_t colour) {
    lv_obj_t* m = lv_linemeter_create(lv_scr_act(), nullptr);
    lv_linemeter_set_scale(m, static_cast<uint16_t>(360 - 360 / count), count);
    lv_linemeter_set_angle_offset(m, 180);
    lv_obj_set_size(m, size, size);
    lv_obj_set_pos(m, cx - size / 2, cy - size / 2);
    lv_obj_set_style_local_bg_opa(m, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_obj_set_style_local_border_width(m, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_pad_all(m, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 0);
    lv_obj_set_style_local_scale_width(m, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, length);
    lv_obj_set_style_local_scale_end_line_width(m, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, width);
    lv_obj_set_style_local_scale_end_color(m, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, colour);
    lv_obj_set_style_local_line_width(m, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, width);
    lv_obj_set_style_local_line_color(m, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, colour);
    return m;
  }

  // A temperature in the unit the user chose in Settings.
  inline int Degrees(const Pinetime::Controllers::SimpleWeatherService::Temperature& t, const Pinetime::Controllers::Settings& settings) {
    return settings.GetWeatherFormat() == Pinetime::Controllers::Settings::WeatherFormat::Imperial ? t.Fahrenheit() : t.Celsius();
  }

  inline uint8_t Hour12(uint8_t h) {
    return h % 12 == 0 ? 12 : h % 12;
  }

  constexpr const char* degreeSign = "\xC2\xB0";
}
