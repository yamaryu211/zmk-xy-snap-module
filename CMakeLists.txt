# ZMK XY Snap Module
# SPDX-License-Identifier: MIT

zephyr_library()
zephyr_library_sources(rc/zmk_xy_snap_input_behavior.c)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)