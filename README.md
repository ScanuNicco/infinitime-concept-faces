# Concept Faces for InfiniTime

Ten watch faces for the [PineTime](https://pine64.org/devices/pinetime/) running
[InfiniTime](https://github.com/InfiniTimeOrg/InfiniTime), adapted from the Pebble watch face
concepts posted in [*Concept watch faces, developers are welcome to develop these*](https://www.reddit.com/r/pebble/comments/6wxhp3/concept_watch_faces_developers_are_welcome_to/)
on r/pebble — thanks to their designer for sharing them. Packaged with an `infinitime-app.json`
manifest so InfiniTime Builder can add them to your firmware.

![All ten faces at 10:08](screenshots/faces.png)

*Top: Tiles, Horizon, Words, Rally, Outlook. Bottom: Glow, Noir, Subdial, Constellation, Almanac.
Screenshots from the InfiniEmu emulator at 10:08 AM, with the watch set to 12-hour time and °F. The
weather (Truckee, CA), step count and battery level are sample data, since the emulator has no phone
connection or sensors.*

## The faces

| Face | | Uses |
|---|---|---|
| **Tiles** | Big light digits on dark red flip-clock tiles; AM/PM in 12-hour mode. | 2 fonts |
| **Horizon** | Time and date over a dome where the sun travels from today's sunrise to sunset (a moon at night), with temperature, high/low and location. | 3 fonts, weather |
| **Words** | A word clock: "IT IS TEN PAST THREE" lit among dim letters, the date in yellow. Five-minute steps. | built-in font |
| **Rally** | A small analog clock inside a ticked, numbered bezel. The band round the edge shows your steps on the left (green, then yellow near your goal) and the battery on the right. | 1 font, steps |
| **Outlook** | Large time and date over an arc; weather icon, temperature, battery, and icons for the next three days. | 3 fonts, 26 images, weather |
| **Glow** | Minimal analog with cyan hands, a red second hand, the date, battery and Bluetooth. | 1 font |
| **Noir** | Long white hands and a red second hand on black, with the day and date at three. | 1 font |
| **Subdial** | A dress-watch face with a small-seconds sub-dial at six, a date window and the weekday. | 1 font |
| **Constellation** | The hour and minute as two stars joined like a constellation, on a starry sky, with the weather and date. | 1 font, weather |
| **Almanac** | The time as two rulers — hours above, minutes below — with the current values large, plus weather and date. | 2 fonts, weather |

Weather (and Horizon's sunrise/sunset) comes from your phone's companion app (Gadgetbridge,
InfiniLink, ...). Temperatures follow the °C/°F choice in the watch's settings, and every face follows
the 12/24-hour setting.

## Install

1. In **InfiniTime Builder**, add `ScanuNicco/infinitime-concept-faces` under **Sources** (new
   installs of the builder have it already).
2. Switch the faces you like on under **Watch faces**, and build.
3. Install the firmware, **then also send the `infinitime-resources-*.zip`** to the watch: all faces
   except Words use custom fonts or images stored on the watch's external flash. Until the resource
   pack is installed they show greyed out in the watch face list.
4. Pick the face on the watch under *Settings > Watch face*.

Requires InfiniTime 1.16 or later. Tested on 1.16.1 in the [InfiniEmu](https://github.com/pipe01/InfiniEmu)
emulator, in 12- and 24-hour mode, by day and night, with and without weather data, and without the
resource pack. All ten together add about 16 KB to the firmware.

## How the resources work

Each face declares its fonts and images in [`infinitime-app.json`](infinitime-app.json), in the same
format as InfiniTime's own `src/resources/fonts.json` and `images.json`. They end up in the
resource pack as `/fonts/<name>.bin` and `/images/<name>.bin`. The faces check for them in
`IsAvailable()`, load fonts with `lv_font_load("F:/fonts/...")` and images with
`lv_img_set_src(img, "F:/images/...")`, and fall back to built-in fonts if something is missing.

Fonts are loaded into the watch's RAM, so each contains only the characters its face can show: from
0.4 KB (Rally) to about 8 KB (Horizon, Outlook). Keep it around there: a face that loaded 16 KB of
fonts crashed the watch in testing.

Images cost no RAM, but LVGL decodes them from the watch's external flash every time that part of
the screen is redrawn, which is slow for large ones: the first versions of Horizon (a full-width
dome image), Constellation (a full-screen starfield) and Rally (full-screen tick images, redrawn
every second around the second hand) were noticeably slow to open. They now draw these with LVGL
shapes, or with a *draw layer* (`common/FaceKit.h`): one object whose drawing function draws many
marks clipped to the area being redrawn, with no images and almost no RAM. Images are kept for
small things only: Outlook's weather icons (one image per icon and size, rather than a font
holding all of them in RAM).

The two scripts in [`tools/`](tools/) generate the manifest (computing each font's characters from
the month, day and weather words it shows) and Outlook's weather icons, and print the coordinate
tables for Horizon's sun path and dotted arc:

    python3 tools/make_manifest.py
    python3 tools/make_images.py

Shared drawing helpers are in [`common/FaceKit.h`](common/FaceKit.h). Clock hands use InfiniTime's
integer trigonometry, so no floating point is needed.

## Credits and license

Designs: adapted from the [concept watch faces on r/pebble](https://www.reddit.com/r/pebble/comments/6wxhp3/concept_watch_faces_developers_are_welcome_to/),
shared by their designer for developers to build. The "pebble" logo on one concept is replaced by the
weekday.

Code: GPL-3.0, like InfiniTime. Fonts: [Barlow Condensed](https://github.com/jpt/barlow), SIL Open
Font License 1.1 (see [`fonts/OFL.txt`](fonts/OFL.txt)). Weather icons:
[Font Awesome Free](https://fontawesome.com) 5.15.4 by Fonticons, Inc., SIL OFL 1.1 (see
[`fonts/FontAwesome-LICENSE.txt`](fonts/FontAwesome-LICENSE.txt)) — the same icons InfiniTime uses.
