get_filename_component(APP_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
idf_component_register(SRCS "tab5_bridge.c" "bsp_tab5.c" "tab5_audio.c" "chex_menu.c"
    INCLUDE_DIRS "${APP_ROOT}/main" "${APP_ROOT}/components/doomgeneric/doomgeneric"
    REQUIRES esp_lcd esp_lcd_touch esp_driver_ppa esp_timer esp_lcd_ili9881c esp_lcd_st7123 esp_lcd_touch_gt911 esp_io_expander_pi4ioe5v6408 vfs usb esp_codec_dev driver fatfs esp_mm usb_host_hid)
