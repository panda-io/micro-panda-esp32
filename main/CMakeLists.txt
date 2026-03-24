# IDF runs this file twice: once in script mode (to collect requirements) and
# once in build mode. add_custom_command and execute_process for side effects
# must only run in build mode.
if(NOT CMAKE_SCRIPT_MODE_FILE)
    # Run mpd gen during CMake configure to ensure out/esp32.c exists on first run.
    # -C points to this file's directory (where mpd.yaml lives) regardless of
    # what directory CMake/IDF is running from.
    execute_process(
        COMMAND mpd -C ${CMAKE_CURRENT_LIST_DIR} gen esp32
        RESULT_VARIABLE MPD_GEN_RESULT)
    if(NOT MPD_GEN_RESULT EQUAL 0)
        message(FATAL_ERROR "mpd gen esp32 failed")
    endif()

    # Rebuild out/esp32.c incrementally whenever src/main.mpd changes.
    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_LIST_DIR}/out/esp32.c
        COMMAND mpd -C ${CMAKE_CURRENT_LIST_DIR} gen esp32
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/src/main.mpd
                ${CMAKE_CURRENT_LIST_DIR}/mpd.yaml
        COMMENT "Generating C from Micro-Panda sources (mpd gen esp32)")
endif()

idf_component_register(
    SRCS    "out/esp32.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_ledc esp_driver_gpio)
