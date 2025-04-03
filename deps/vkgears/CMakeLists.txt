set (VULKAN_SDK "/opt/vulkan/1.2.182.0/x86_64")
set (SHADER_DIR shaders)

include(CheckLibraryExists)
check_library_exists(m sqrtf "" HAVE_LIB_M)
if (HAVE_LIB_M)
    set(EXTRA_LIBS ${EXTRA_LIBS} m)
endif (HAVE_LIB_M)

find_package(PkgConfig)
pkg_check_modules(GLFW3 glfw3)
pkg_check_modules(VULKAN vulkan)

# Find OpenGL library
#include(FindOpenGL)

# Find Vulkan library
include(FindVulkan)
if (NOT Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
    find_program (GLSL_LANG_VALIDATOR NAMES glslangValidator PATHS ${VULKAN_SDK}/bin)
    set(Vulkan_GLSLANG_VALIDATOR_EXECUTABLE "${GLSL_LANG_VALIDATOR}")
endif()

# set defaults for options
set (OPENGL_EXAMPLES_DEFAULT ${OpenGL_OpenGL_FOUND})
set (VULKAN_EXAMPLES_DEFAULT ${Vulkan_FOUND})
if(NOT Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
    set (VULKAN_EXAMPLES_DEFAULT OFF)
endif()
set (OPENGL_EXAMPLES_DEFAULT OFF)

configure_file(config.h.cmake ${CMAKE_BINARY_DIR}/config_vkgears.h @ONLY)

# user configurable options
option(OPENGL_EXAMPLES "Build OpenGL examples" ${OPENGL_EXAMPLES_DEFAULT})
option(VULKAN_EXAMPLES "Build Vulkan examples" ${VULKAN_EXAMPLES_DEFAULT})
option(EXTERNAL_GLFW "Use external GLFW project" OFF)
option(EXTERNAL_GLAD "Use external GLAD project" OFF)

message(STATUS "OPENGL_EXAMPLES = ${OPENGL_EXAMPLES}")
message(STATUS "VULKAN_EXAMPLES = ${VULKAN_EXAMPLES}")
message(STATUS "EXTERNAL_GLFW = ${EXTERNAL_GLFW}")
message(STATUS "EXTERNAL_GLAD = ${EXTERNAL_GLAD}")

if(APPLE)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  find_library(IOKIT_LIBRARY IOKit)
  find_library(COCOA_LIBRARY Cocoa)
  list(APPEND GLFW_LIBS_ALL ${COREFOUNDATION_LIBRARY} ${IOKIT_LIBRARY} ${COCOA_LIBRARY})
endif()

# Support for external glfw
if (EXTERNAL_GLFW)
    include(ExternalProject)
    ExternalProject_Add(
        GLFW
        GIT_REPOSITORY "https://github.com/glfw/glfw.git"
        SOURCE_DIR "${CMAKE_BINARY_DIR}/third_party/glfw3"
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
        CMAKE_ARGS -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF
                   -DGLFW_BUILD_DOCS=OFF -DGLFW_INSTALL=OFF
    )
    set(GLFW_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/third_party/glfw3/include")
    set(GLFW_LIBRARY_DIR "${CMAKE_BINARY_DIR}/GLFW-prefix/src/GLFW-build/src/")
    set(GLFW_LIBRARIES glfw3 pthread dl)
    include_directories(${GLFW_INCLUDE_DIRS})
    link_directories(${GLFW_LIBRARY_DIR})
    list(APPEND GLFW_LIBS_ALL ${GLFW_LIBRARIES} ${EXTRA_LIBS})
else ()
    add_definitions(${GLFW3_CFLAGS})
    list(APPEND GLFW_LIBS_ALL ${GLFW3_LDFLAGS} ${EXTRA_LIBS})
endif ()

# Support for external glad
if (EXTERNAL_GLAD)
    include(ExternalProject)
    ExternalProject_Add(
        GLAD
        GIT_REPOSITORY "https://github.com/Dav1dde/glad.git"
        SOURCE_DIR "${CMAKE_BINARY_DIR}/third_party/glad"
        STEP_TARGETS build
        EXCLUDE_FROM_ALL TRUE
        CMAKE_ARGS -DGLAD_EXPORT=OFF
    )
    set(GLAD_INCLUDE_DIRS "${CMAKE_BINARY_DIR}/GLAD-prefix/src/GLAD-build/include/")
    set(GLAD_LIBRARY_DIR "${CMAKE_BINARY_DIR}/GLAD-prefix/src/GLAD-build/")
    set(GLAD_LIBRARIES glad)
    include_directories(${GLAD_INCLUDE_DIRS})
    link_directories(${GLAD_LIBRARY_DIR})
    list(APPEND OPENGL_LOADER_LIBS ${GLAD_LIBRARIES})
else ()
    list(APPEND OPENGL_LOADER_LIBS ${OPENGL_opengl_LIBRARY})
endif ()

if (OPENGL_EXAMPLES)
    foreach(prog IN ITEMS gl1_gears gl2_gears gl3_gears gl4_gears)
        message("-- Adding: ${prog}")
        add_executable(${prog} src/${prog}.c)
        target_link_libraries(${prog} ${GLFW_LIBS_ALL} ${OPENGL_LOADER_LIBS})
        if (EXTERNAL_GLAD)
            target_compile_definitions(${prog} PRIVATE -DHAVE_GLAD)
        endif ()
        if (EXTERNAL_GLFW)
            add_dependencies(${prog} GLFW-build)
        endif ()
        if (EXTERNAL_GLAD)
            add_dependencies(${prog} GLAD-build)
        endif ()
    endforeach(prog)
endif (OPENGL_EXAMPLES)

if (VULKAN_EXAMPLES)
    file(GLOB_RECURSE VERTEX_SHADER_SOURCE "${SHADER_DIR}/*.v450.vert")
    file(GLOB_RECURSE FRAGMENT_SHADER_SOURCE "${SHADER_DIR}/*.v450.frag")
    set(SHADER_SOURCE ${VERTEX_SHADER_SOURCE} ${FRAGMENT_SHADER_SOURCE})
    source_group("shaders" FILES ${SHADER_SOURCE})

    foreach(shader ${SHADER_SOURCE})
        add_custom_command(OUTPUT ${shader}.spv DEPENDS ${shader}
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V ${shader} -o ${shader}.spv)
        list(APPEND SHADERS ${shader}.spv)
    endforeach(shader)

    include_directories(${CMAKE_BINARY_DIR} ${Vulkan_INCLUDE_DIRS})

    message("-- Adding: vk1_gears")
    add_executable(vk1_gears src/vk1_gears.c ${SHADERS})
    target_link_libraries(vk1_gears ${Vulkan_LIBRARIES} ${GLFW_LIBS_ALL})

    if (EXTERNAL_GLFW)
        add_dependencies(vk1_gears GLFW-build)
    endif ()
    if (EXTERNAL_GLAD)
        add_dependencies(vk1_gears GLAD-build)
    endif ()

    install(FILES ${CMAKE_BINARY_DIR}/deps/vkgears/vk1_gears DESTINATION ${CMAKE_INSTALL_LIBDIR}/hardinfo2/modules
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    install(FILES "${VERTEX_SHADER_SOURCE}.spv" DESTINATION ${CMAKE_INSTALL_LIBDIR}/hardinfo2/modules
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ 
    )
    install(FILES "${FRAGMENT_SHADER_SOURCE}.spv" DESTINATION ${CMAKE_INSTALL_LIBDIR}/hardinfo2/modules
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ 
    )
      
    message(STATUS "vkgears module added - DONE")
endif (VULKAN_EXAMPLES)
