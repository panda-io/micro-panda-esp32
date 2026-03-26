Run a ninja build with verbose compiler output to diagnose errors in the ESP32 firmware.

Steps:
1. Run `mpd -C $PROJECT_PATH/main gen esp32` to regenerate `out/esp32.c`
2. Run `~/.espressif/tools/ninja/1.12.1/ninja -C $PROJECT_PATH/build -v 2>&1` for verbose output

For each C compiler error:
- Show the error message and the offending line from `$PROJECT_PATH/main/out/esp32.c`
- Trace back to the corresponding Micro-Panda source (the C function name tells you the module: `string__foo` → `string.mpd`, `gpio__bar` → `src/gpio.mpd`, `main__baz` → `src/main.mpd`)
- Suggest a fix in the `.mpd` source or in the compiler (`micro-panda-dart/`)

After diagnosing, ask whether to apply fixes or just report.
