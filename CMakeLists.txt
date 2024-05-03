cmake_minimum_required(VERSION 3.15)
project(audioio VERSION 0.1.0)

add_executable(audioio)
set_property(TARGET audioio PROPERTY CXX_STANDARD 20)

# Init dependencies
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(init)
init()

# Add include directories
target_include_directories(audioio PRIVATE src ${nano_signal_slot_SOURCE_DIR})

# Add src subdirs
add_subdirectory(src)

# Link libs
target_link_libraries(audioio PRIVATE PortAudio ftxui::screen ftxui::dom ftxui::component)