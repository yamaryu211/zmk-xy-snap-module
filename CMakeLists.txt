# Zephyrモジュールとしての設定
zephyr_module()

# ライブラリの設定
zephyr_library()
zephyr_library_sources(src/zmk_xy_snap_input_processor.c)
zephyr_include_directories(include)
zephyr_library_link_libraries(zephyr zmk)