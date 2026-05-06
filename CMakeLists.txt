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
option(USE_BUNDLED_THIRD_PARTY "Use local third_party/{imgui,implot}" ON)

if(BUILD_GUI)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GLFW REQUIRED glfw3)
  find_package(OpenGL REQUIRED)

  set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui")
  set(IMPLOT_DIR "${CMAKE_SOURCE_DIR}/third_party/implot")

  if(USE_BUNDLED_THIRD_PARTY AND EXISTS "${IMGUI_DIR}/imgui.cpp" AND EXISTS "${IMPLOT_DIR}/implot.cpp")
    message(STATUS "Using bundled third_party imgui/implot")
  else()
    include(FetchContent)
    message(STATUS "Bundled imgui/implot not found; using FetchContent")
    FetchContent_Declare(imgui
      GIT_REPOSITORY https://github.com/ocornut/imgui.git
      GIT_TAG v1.91.9b)
    FetchContent_Declare(implot
      GIT_REPOSITORY https://github.com/epezent/implot.git
      GIT_TAG v0.16)
    FetchContent_MakeAvailable(imgui implot)
    set(IMGUI_DIR "${imgui_SOURCE_DIR}")
    set(IMPLOT_DIR "${implot_SOURCE_DIR}")
  endif()

  add_library(imgui_lib
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMPLOT_DIR}/implot.cpp
    ${IMPLOT_DIR}/implot_items.cpp)

  target_include_directories(imgui_lib PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMPLOT_DIR}
    ${GLFW_INCLUDE_DIRS})

  add_executable(bench_gui gui.cpp)
  target_link_libraries(bench_gui PRIVATE imgui_lib ${GLFW_LIBRARIES} OpenGL::GL)
  target_compile_options(bench_gui PRIVATE -O3)
endif()
