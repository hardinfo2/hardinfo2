# Hardinfo2 - System Information & Benchmark
# Copyright hardinfo2 project, hwspeedy 2025
# License: GPL2+

message(STATUS "vkgears module being added...")
set (VULKAN_SDK "/opt/vulkan/1.2.182.0/x86_64")

include(CheckLibraryExists)
check_library_exists(m sqrtf "" HAVE_LIB_M)
if (HAVE_LIB_M)
    set(EXTRA_LIBS ${EXTRA_LIBS} m)
endif (HAVE_LIB_M)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(PkgConfig)
pkg_check_modules(VULKAN vulkan)
find_package(Threads REQUIRED)


if(${HARDINFO2_VK_WAYLAND})
  message("-- CK_WAYLAND")
  find_path(WAYLAND_CLIENT_INCLUDE_DIR NAMES wayland-client.h )
  find_path(LIBDECOR_INCLUDE PATH_SUFFIXES libdecor-0 NAMES libdecor.h )
  find_library(WAYLAND_CLIENT_LIBRARY NAMES wayland-client libwayland-client)
  find_library(LIBDECOR_LIBRARY NAMES libdecor-0.so)
  find_library(XKBCOMMON_LIBRARY NAMES xkbcommon)
  set(WAYLAND_SOURCE wsi/wayland.c)
endif()
if(${HARDINFO2_VK_X11})
  message("-- VK_X11")
  find_library(XKBCOMMON_LIBRARY NAMES xkbcommon)
  find_library(XKBCOMMONX11_LIBRARY NAMES xkbcommon-x11)
  find_library(XCB_LIBRARY NAMES xcb)
  set(X11_SOURCE wsi/xcb.c)
endif()

# Find Vulkan library
include(FindVulkan)
if (NOT Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
    find_program (GLSL_LANG_VALIDATOR NAMES glslangValidator PATHS ${VULKAN_SDK}/bin)
    set(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE "${GLSL_LANG_VALIDATOR}")
endif()


file(GLOB_RECURSE VERTEX_SHADER_SOURCE "gear.vert")
file(GLOB_RECURSE FRAGMENT_SHADER_SOURCE "gear.frag")
set(SHADER_SOURCE ${VERTEX_SHADER_SOURCE} ${FRAGMENT_SHADER_SOURCE})
source_group("shaders" FILES ${SHADER_SOURCE})

foreach(shader ${SHADER_SOURCE})
  add_custom_command(OUTPUT ${shader}.spv.h DEPENDS ${shader}
    COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V ${shader} -x -o ${shader}.spv.h)
  list(APPEND SHADERS ${shader}.spv.h)
endforeach(shader)

include_directories(${CMAKE_BINARY_DIR} ${Vulkan_INCLUDE_DIRS} ${LIBDECOR_INCLUDE})

#message(STATUS "Vulkanlib======${Vulkan_LIBRARIES}")

#devenv.set('VK_LOADER_LAYERS_ENABLE', 'VK_LAYER_KHRONOS_validation')
#devenv.set('VK_LAYER_SETTINGS_PATH', meson.current_source_dir())
set(VK_LOADER_LAYERS_ENABLE "VK_LAYER_KHRONOS_validation")
set(VK_LAYER_SETTINGS_PATH "${CMAKE_SOURCE_DIR}")

add_executable(vkgears vkgears.c matrix.c wsi/wsi.c ${WAYLAND_SOURCE} ${X11_SOURCE} ${SHADERS})
target_link_libraries(vkgears ${Vulkan_LIBRARIES} ${EXTRA_LIBS} ${LIBDECOR_LIBRARY} ${WAYLAND_CLIENT_LIBRARY} ${XKBCOMMON_LIBRARY} ${XKBCOMMONX11_LIBRARY} ${XCB_LIBRARY})

install(FILES ${CMAKE_BINARY_DIR}/deps/vkgears/vkgears DESTINATION ${CMAKE_INSTALL_LIBDIR}/hardinfo2/modules
           PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
       )

message(STATUS "vkgears module added - DONE")
