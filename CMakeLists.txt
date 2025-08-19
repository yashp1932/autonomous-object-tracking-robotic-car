cmake_minimum_required(VERSION 3.10)
project(ball_auto_manual)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENCV4 REQUIRED opencv4)

find_path(PIGPIO_INCLUDE_DIR pigpio.h /usr/include /usr/local/include)
find_library(PIGPIO_LIBRARY pigpio /usr/lib /usr/local/lib)
if (NOT PIGPIO_INCLUDE_DIR OR NOT PIGPIO_LIBRARY)
  message(FATAL_ERROR "pigpio not found; install 'sudo apt install libpigpio-dev'")
endif()

include_directories(
  ${OPENCV4_INCLUDE_DIRS}
  ${PIGPIO_INCLUDE_DIR}
)

link_directories(${OPENCV4_LIBRARY_DIRS})

add_executable(ball_auto_manual
  src/ball_auto_manual.cpp
)

target_link_libraries(ball_auto_manual
  ${OPENCV4_LIBRARIES}
  ${PIGPIO_LIBRARY}
  pthread
)