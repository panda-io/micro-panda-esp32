Regenerate C from Micro-Panda sources and build the ESP32 firmware with ninja.

Steps:
1. Run `mpd -C $PROJECT_PATH/main gen esp32` to regenerate `out/esp32.c`
2. Run `~/.espressif/tools/ninja/1.12.1/ninja -C $PROJECT_PATH/build` to compile

If `mpd gen` fails, show the compiler error and stop — do not run ninja.
If ninja fails, show the C compiler errors clearly, grouped by file and function.
Report the final binary size from the ninja output.
