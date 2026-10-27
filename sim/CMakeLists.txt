# Linux host simulator. Two SDL windows, both drawn by LVGL 8.3.
#   sudo apt install libsdl2-dev
#   cmake -S sim -B sim/build && cmake --build sim/build
#   ./sim/build/robojoy_ui

cmake_minimum_required(VERSION 3.16)
project(robojoy_ui C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

set(FW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(LVGL_DIR ${FW_DIR}/components/lvgl__lvgl)

file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS ${LVGL_DIR}/src/*.c)
add_library(lvgl STATIC ${LVGL_SOURCES})
target_include_directories(lvgl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LVGL_DIR})
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)
target_link_libraries(lvgl PUBLIC m)

add_executable(robojoy_ui
    src/main.c
    src/sdl_display.c
    src/control_panel.c
    src/hal_link.c
    ${FW_DIR}/main/joy_kb.c
    ${FW_DIR}/main/LVGL_UI/lvgl_main.c
    ${FW_DIR}/main/LVGL_UI/lvgl_robot_control.c
    ${FW_DIR}/main/LVGL_UI/lvgl_robot_select.c
    ${FW_DIR}/main/LVGL_UI/img_battery1.c
    ${FW_DIR}/main/LVGL_UI/img_battery2.c
    ${FW_DIR}/main/LVGL_UI/img_battery3.c
    ${FW_DIR}/main/LVGL_UI/img_signal1.c
    ${FW_DIR}/main/LVGL_UI/img_signal2.c
    ${FW_DIR}/main/LVGL_UI/img_signal3.c
)

target_include_directories(robojoy_ui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${FW_DIR}/main
    ${FW_DIR}/main/LVGL_UI
)
set_source_files_properties(
    src/main.c src/sdl_display.c src/control_panel.c src/hal_link.c
    PROPERTIES COMPILE_OPTIONS "-Wall;-Wextra"
)

if(TARGET SDL2::SDL2)
    target_link_libraries(robojoy_ui PRIVATE lvgl SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    target_link_libraries(robojoy_ui PRIVATE lvgl SDL2::SDL2-static)
else()
    target_include_directories(robojoy_ui PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(robojoy_ui PRIVATE lvgl ${SDL2_LIBRARIES})
endif()
