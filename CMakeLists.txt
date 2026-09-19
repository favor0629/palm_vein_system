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
# lgpio
# ============================================================

find_path(LGPIO_INCLUDE_DIR
    NAMES lgpio.h
    REQUIRED
)

find_file(LGPIO_LIBRARY
    NAMES liblgpio.so.1
    PATHS
        /usr/lib/aarch64-linux-gnu
        /lib/aarch64-linux-gnu
    NO_DEFAULT_PATH
    REQUIRED
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

    src/ui/page/CapturePage.cpp
    src/ui/page/CapturePage.h
    src/ui/page/CapturePage.ui


    # --------------------------------------------------------
    # ShowImagePage
    # --------------------------------------------------------

    src/ui/page/ShowImagePage.cpp
    src/ui/page/ShowImagePage.h
    src/ui/page/ShowImagePage.ui


    # --------------------------------------------------------
    # Camera
    # --------------------------------------------------------

    src/camera/CameraManager.cpp
    src/camera/CameraManager.h


    # --------------------------------------------------------
    # GPIO
    # --------------------------------------------------------

    src/driver/pwm/GpioChip.hpp


    # --------------------------------------------------------
    # PWM driver
    # --------------------------------------------------------

    src/driver/pwm/GpioChip.cpp
    src/driver/pwm/GpioChip.hpp
    src/driver/pwm/IPwmChannel.hpp
    src/driver/pwm/PwmTypes.hpp
    src/driver/pwm/HardwarePwmChannel.cpp
    src/driver/pwm/HardwarePwmChannel.hpp
    src/driver/pwm/SoftwarePwmChannel.cpp
    src/driver/pwm/SoftwarePwmChannel.hpp
    src/driver/pwm/PwmController.cpp
    src/driver/pwm/PwmController.hpp


    # --------------------------------------------------------
    # Infrared lamp
    # --------------------------------------------------------

    src/lamp/InfraredLampController.cpp
    src/lamp/InfraredLampController.hpp


    # --------------------------------------------------------
    #  tools
    # --------------------------------------------------------
    src/tools/fvia.cpp
    src/tools/fvia.hpp
)


# ============================================================
# Include directories
# ============================================================

target_include_directories(PalmVeinSystem PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}/src

    ${OpenCV_INCLUDE_DIRS}

    ${LIBCAMERA_INCLUDE_DIRS}

    ${LGPIO_INCLUDE_DIR}
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

    ${LGPIO_LIBRARY}
)