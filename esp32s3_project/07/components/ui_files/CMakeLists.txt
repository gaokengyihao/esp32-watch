set(ui_src_dirs
    "src"
    "src/fonts"
    "src/images"
    "src/screens")

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/src/components")
    list(APPEND ui_src_dirs "src/components")
endif()

idf_component_register(
    SRC_DIRS      ${ui_src_dirs}
    INCLUDE_DIRS  "src"
    REQUIRES      lvgl)
