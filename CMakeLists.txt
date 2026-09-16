cmake_minimum_required(VERSION 3.16)

project(PalmVeinSystem
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)


# ============================================================
# Qt
# ============================================================

find_package(Qt6 REQUIRED COMPONENTS
    Widgets
)


# ============================================================
# OpenCV
# ============================================================

find_package(OpenCV REQUIRED)


# ============================================================
# libcamera
# ============================================================

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBCAMERA REQUIRED
    libcamera
)


# ============================================================
# Executable
# ============================================================

add_executable(PalmVeinSystem

    # --------------------------------------------------------
    # Main
    # --------------------------------------------------------

    src/main.cpp


    # --------------------------------------------------------
    # MainWindow
    # --------------------------------------------------------

    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.ui


    # --------------------------------------------------------
    # CapturePage
    # --------------------------------------------------------

    src/ui/CapturePage.cpp
    src/ui/CapturePage.h
    src/ui/CapturePage.ui


    # --------------------------------------------------------
    # ShowImagePage
    # --------------------------------------------------------

    src/ui/ShowImagePage.cpp
    src/ui/ShowImagePage.h
    src/ui/ShowImagePage.ui


    # --------------------------------------------------------
    # Camera
    # --------------------------------------------------------

    src/camera/CameraManager.cpp
    src/camera/CameraManager.h
)


# ============================================================
# Include directories
# ============================================================

target_include_directories(PalmVeinSystem PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}/src

    ${OpenCV_INCLUDE_DIRS}

    ${LIBCAMERA_INCLUDE_DIRS}
)

target_compile_definitions(PalmVeinSystem PRIVATE
    QT_NO_KEYWORDS
)


# ============================================================
# Library directories
# ============================================================

target_link_directories(PalmVeinSystem PRIVATE

    ${LIBCAMERA_LIBRARY_DIRS}
)


# ============================================================
# Link libraries
# ============================================================

target_link_libraries(PalmVeinSystem PRIVATE

    Qt6::Widgets

    ${OpenCV_LIBS}

    ${LIBCAMERA_LIBRARIES}
)