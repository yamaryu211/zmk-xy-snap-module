# ZMK XY Snap Module
# SPDX-License-Identifier: MIT

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZMK_XY_SNAP_INPUT_BEHAVIOR src/zmk_xy_snap_input_behavior.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_XY_SNAP_INPUT_PROCESSOR src/zmk_xy_snap_input_processor.c)

# 常にヘッダーファイルを含める
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# デフォルトでinput-processorを有効にする
if(NOT DEFINED CONFIG_ZMK_XY_SNAP_INPUT_PROCESSOR)
    zephyr_library_sources(src/zmk_xy_snap_input_processor.c)
endif()