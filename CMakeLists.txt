# ZMK XY Snap Input Behavior Listener
# SPDX-License-Identifier: MIT

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZMK_XY_SNAP_INPUT_BEHAVIOR src/zmk_xy_snap_input_behavior.c)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)