cmake_minimum_required(VERSION 3.16)
project(arm_neon_array_processor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(bench_cli main.cpp)
target_compile_options(bench_cli PRIVATE -O3)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm")
  target_compile_options(bench_cli PRIVATE -march=armv8-a+simd)
endif()

option(BUILD_GUI "Build ImGui/ImPlot GUI" ON)
if(BUILD_GUI)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GLFW REQUIRED glfw3)

  include(FetchContent)
  FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b)
  FetchContent_Declare(implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG v0.16)
  FetchContent_MakeAvailable(imgui implot)

  add_library(imgui_lib
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp)
  target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${implot_SOURCE_DIR}
    ${GLFW_INCLUDE_DIRS})

  add_executable(bench_gui gui.cpp)
  target_link_libraries(bench_gui PRIVATE imgui_lib ${GLFW_LIBRARIES} GL)
  target_compile_definitions(bench_gui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GL3W)
  target_compile_options(bench_gui PRIVATE -O3)
endif()
