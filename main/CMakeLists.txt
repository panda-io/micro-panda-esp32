# IDF runs this file twice: once in script mode (to collect requirements) and
# once in build mode. add_custom_command and execute_process for side effects
# must only run in build mode.
if(NOT CMAKE_SCRIPT_MODE_FILE)
    # Root dir is one level up — that's where mpd.yaml and src/ live.
    set(MPD_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

    # Run mpd gen during CMake configure to ensure esp32.c exists on first run.
    execute_process(
        COMMAND mpd -C ${MPD_ROOT} gen esp32
        RESULT_VARIABLE MPD_GEN_RESULT)
    if(NOT MPD_GEN_RESULT EQUAL 0)
        message(FATAL_ERROR "mpd gen esp32 failed")
    endif()

    # Rebuild esp32.c incrementally whenever any source file changes.
    file(GLOB MPD_SOURCES ${MPD_ROOT}/src/*.mpd)
    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_LIST_DIR}/esp32.c
        COMMAND mpd -C ${MPD_ROOT} gen esp32
        DEPENDS ${MPD_SOURCES}
                ${MPD_ROOT}/mpd.yaml
        COMMENT "Generating C from Micro-Panda sources (mpd gen esp32)")
endif()

idf_component_register(
    SRCS "esp32.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_ledc esp_driver_gpio esp_adc esp_driver_i2c esp_driver_spi)
