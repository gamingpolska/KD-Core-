cmake_minimum_required(VERSION 3.20)
project(KDCore VERSION 2.0.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "KD Core jest aplikacją natywną dla Windows.")
endif()

enable_language(RC)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/resources")
configure_file(resources/KD.ico "${CMAKE_CURRENT_BINARY_DIR}/resources/KD.ico" COPYONLY)
configure_file(resources/KD_Core_Icon_Master.png "${CMAKE_CURRENT_BINARY_DIR}/resources/KD_Core_Icon_Master.png" COPYONLY)
configure_file(resources/KD_Core_512.png "${CMAKE_CURRENT_BINARY_DIR}/resources/KD_Core_512.png" COPYONLY)
configure_file(resources/KD_Core_64.png "${CMAKE_CURRENT_BINARY_DIR}/resources/KD_Core_64.png" COPYONLY)
configure_file(app.rc "${CMAKE_CURRENT_BINARY_DIR}/app.rc" COPYONLY)

add_executable(KDCore WIN32
    src/main.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/app.rc"
)

target_include_directories(KDCore PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_features(KDCore PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(KDCore PRIVATE /W4 /utf-8 /permissive- /EHsc)
endif()

target_link_libraries(KDCore PRIVATE
    gdiplus
    comdlg32
    comctl32
    shell32
    advapi32
    shlwapi
    ole32
    oleaut32
    uuid
    pdh
    dxgi
    powrprof
    dwmapi
    uxtheme
)

if(NOT MSVC)
    target_compile_options(KDCore PRIVATE -municode)
    target_link_options(KDCore PRIVATE -municode -static)
endif()

set_target_properties(KDCore PROPERTIES
    OUTPUT_NAME "KDCore"
    WIN32_EXECUTABLE TRUE
)
