cmake_minimum_required(VERSION 3.20...3.28)
project(MatchMaker)

set(CMAKE_CXX_STANDARD 17)

# Set SDL3 location manually
set(SDL3_DIR "${CMAKE_SOURCE_DIR}/external/SDL3/cmake")
find_package(SDL3 CONFIG REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/external/SDL3/include
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/implot
    ${CMAKE_SOURCE_DIR}/external/Utility
)

# Add source files
add_executable(MatchMaker

# main files
    src/main.cpp
    src/UIConstructor.h
    src/UIConstructor.cpp
    src/MatchMakingSystem.h
    src/MatchMakingSystem.cpp
    src/MM_Elements.h
    src/MM_Elements.cpp
    src/PlayerTrait.h
    src/PlayerTrait.cpp
    
# custom support files
    external/Utility/Logger.h
    external/Utility/Logger.cpp
    external/Utility/RandomGenerator.h
    external/Utility/RandomGenerator.cpp
    external/Utility/Utility.h
    external/Utility/Utility.cpp
    external/Utility/WorldClock.h
    external/Utility/WorldClock.cpp

# imgui
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/backends/imgui_impl_sdl3.cpp
    external/imgui/backends/imgui_impl_sdlrenderer3.cpp

#implot
    external/implot/implot.cpp
    external/implot/implot_demo.cpp
    external/implot/implot_items.cpp
)

# Link libraries
target_link_libraries(MatchMaker SDL3::SDL3)
