# ZMK XY Snap Module
# SPDX-License-Identifier: MIT

# Zephyrモジュールとして追加
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZMK_XY_SNAP_INPUT_PROCESSOR src/zmk_xy_snap_input_processor.c)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include) 