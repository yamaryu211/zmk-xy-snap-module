# ZMK XY Snap Module
# SPDX-License-Identifier: MIT

zephyr_library()

zephyr_library_sources(src/zmk_xy_snap_input_processor.c)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ZMKのinclude pathを追加
zephyr_include_directories(${CMAKE_SOURCE_DIR}/include)
zephyr_include_directories(${CMAKE_SOURCE_DIR}/zmk/app/include)

# デフォルトでinput-processorを有効にする
if(NOT DEFINED CONFIG_ZMK_XY_SNAP_INPUT_PROCESSOR)
    zephyr_library_sources(src/zmk_xy_snap_input_processor.c)
endif()