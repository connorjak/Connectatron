project(imgui) #VERSION 1.84) #(CMake error with version not allowed unless cmp0048 is set to new)

set(_imgui_Sources
    imconfig.h
    imgui.cpp
    imgui.h
    imgui.natvis
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_internal.h
    imgui_widgets.cpp
    imgui_tables.cpp
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
    LICENSE.txt
    ImguiFileDialog.cpp
    ImguiFileDialogConfig.h
    ImguiFileDialog.h
    misc/cpp/imgui_stdlib.cpp
    misc/cpp/imgui_stdlib.h
)

add_library(${PROJECT_NAME} STATIC ${_imgui_Sources})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#target_compile_definitions(${PROJECT_NAME} PUBLIC "ImDrawIdx=unsigned int")
target_compile_definitions(${PROJECT_NAME} PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${_imgui_Sources})

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "external")

