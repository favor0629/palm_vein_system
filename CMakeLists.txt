cmake_minimum_required(VERSION 3.16)

project(
    PalmVeinSystem
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

find_package(PkgConfig REQUIRED)

find_package(OpenCV REQUIRED)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

pkg_check_modules(
    LIBCAMERA
    REQUIRED
    IMPORTED_TARGET
    libcamera
)

add_executable(
    PalmVeinSystem

    src/main.cpp

    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.ui

    src/camera/CameraManager.cpp
    src/camera/CameraManager.h
)

target_include_directories(
    PalmVeinSystem
    PRIVATE

    ${OpenCV_INCLUDE_DIRS}

    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    PalmVeinSystem

    PRIVATE

    PkgConfig::LIBCAMERA

    ${OpenCV_LIBS}

    Qt6::Widgets
)

target_compile_options(
    PalmVeinSystem
    PRIVATE

    -Wall
    -Wextra
    -Wpedantic
)