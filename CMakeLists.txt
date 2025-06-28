# ZMK XY Snap Module
# SPDX-License-Identifier: MIT

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZMK_XY_SNAP_INPUT_PROCESSOR behaviors/zmk_xy_snap_input_processor.c)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include) 