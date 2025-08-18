cmake_minimum_required(VERSION 3.10)
project(pid_tools)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

add_subdirectory(${CMAKE_SOURCE_DIR}/include/glfw)

# Add the PID simulation executable
add_executable(pid_sim src/pid_sim.cpp)
add_executable(pid_plot src/pid_plot.cpp
    include/imgui/imgui.cpp 
    include/imgui/imgui_draw.cpp 
    include/imgui/imgui_tables.cpp 
    include/imgui/imgui_widgets.cpp
    include/imgui/backends/imgui_impl_glfw.cpp 
    include/imgui/backends/imgui_impl_opengl3.cpp
    include/implot/implot.cpp 
    include/implot/implot_items.cpp
)
add_executable(pid_bc_compare src/pid_bc_compare.cpp
    include/imgui/imgui.cpp 
    include/imgui/imgui_draw.cpp 
    include/imgui/imgui_tables.cpp 
    include/imgui/imgui_widgets.cpp
    include/imgui/backends/imgui_impl_glfw.cpp 
    include/imgui/backends/imgui_impl_opengl3.cpp
    include/implot/implot.cpp 
    include/implot/implot_items.cpp
    ../libs/libfixmath/libfixmath/fix16.c
)

# Include the Controls library headers
target_include_directories(pid_sim PRIVATE
    ${CMAKE_SOURCE_DIR}/../Libs/Controls/include
    ${CMAKE_SOURCE_DIR}/../Libs
)

target_include_directories(pid_plot PRIVATE
    ${CMAKE_SOURCE_DIR}/../Libs/Controls/include
    ${CMAKE_SOURCE_DIR}/../Libs
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
    ${CMAKE_SOURCE_DIR}/include/implot
    ${CMAKE_SOURCE_DIR}/include/glfw/include
)

target_include_directories(pid_bc_compare PRIVATE
    ${CMAKE_SOURCE_DIR}/../Libs/Controls/include
    ${CMAKE_SOURCE_DIR}/../Libs
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/include/imgui
    ${CMAKE_SOURCE_DIR}/include/imgui/backends
    ${CMAKE_SOURCE_DIR}/include/implot
    ${CMAKE_SOURCE_DIR}/include/glfw/include
)

find_package(OpenGL REQUIRED)
# find_package(PkgConfig REQUIRED)

target_link_libraries(pid_plot PRIVATE OpenGL::GL glfw)
target_link_libraries(pid_bc_compare PRIVATE OpenGL::GL glfw)