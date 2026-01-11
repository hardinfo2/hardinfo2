if(HARDINFO2_QT5)
  message(STATUS "qgears2 QT5 module being added...")
  find_package(Qt5 REQUIRED Core Gui Widgets OpenGL)
endif()
if(HARDINFO2_QT6)
  message(STATUS "qgears2 QT6 module being added...")
  find_package(Qt6 REQUIRED Core Gui Widgets OpenGL OpenGLWidgets)
endif()
include(FindPkgConfig)

if(HARDINFO2_QT_MOCQT)
  if(HARDINFO2_QT5)
    set(MOC "moc-qt5")
  endif()
  if(HARDINFO2_QT6)
    set(MOC "moc-qt6")
  endif()
else()
  set(MOC "moc")
endif()

if(HARDINFO2_QT5)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/qglgears5.h ${CMAKE_CURRENT_BINARY_DIR}/qglgears.h)
  execute_process(COMMAND ${MOC} -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qgears.cpp)
  execute_process(COMMAND ${MOC} -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qglgears5.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qglgears.cpp)
  execute_process(COMMAND /usr/bin/${MOC} -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qgears.cpp)
  execute_process(COMMAND /usr/bin/${MOC} -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qglgears5.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qglgears.cpp)
  execute_process(COMMAND /usr/lib/qt5/bin/${MOC} -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qgears.cpp)
  execute_process(COMMAND /usr/lib/qt5/bin/${MOC} -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qglgears5.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qglgears.cpp)
  execute_process(COMMAND /usr/lib64/qt5/bin/${MOC} -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qgears.cpp)
  execute_process(COMMAND /usr/lib64/qt5/bin/${MOC} -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qglgears5.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qglgears.cpp)
  execute_process(COMMAND /usr/lib/i386-linux-gnu/qt5/bin/${MOC} -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qgears.cpp)
  execute_process(COMMAND /usr/lib/i386-linux-gnu/qt5/bin/${MOC} -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qglgears5.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qglgears.cpp)
endif()
if(HARDINFO2_QT6)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/qglgears6.h ${CMAKE_CURRENT_BINARY_DIR}/qglgears.h)
  execute_process(COMMAND /usr/lib64/qt6/libexec/moc -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qgears.cpp)
  execute_process(COMMAND /usr/lib64/qt6/libexec/moc -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_BINARY_DIR}/qglgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qglgears.cpp)
  execute_process(COMMAND /usr/lib/qt6/libexec/moc -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/qgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qgears.cpp)
  execute_process(COMMAND /usr/lib/qt6/libexec/moc -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB ${CMAKE_CURRENT_BINARY_DIR}/qglgears.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qglgears.cpp)
endif()

if(HARDINFO2_QT5)
include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  .
  Qt5::OpenGL
  Qt5::Widgets
  Qt5::Gui
  Qt5::Core
)
endif()
if(HARDINFO2_QT6)
include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  .
  Qt6::OpenGL
  Qt6::Widgets
  Qt6::OpenGLWidgets
  Qt6::Gui
  Qt6::Core
)
endif()

add_executable(qgears2
  main.cpp
  qgears.cpp
  commonrenderer.cpp
  qglgears.cpp
  fdclock.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qgears.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/moc_qglgears.cpp
)

set_target_properties(qgears2 PROPERTIES COMPILE_FLAGS "-pipe -O2 -Wall -Wextra -D_REENTRANT -fPIC")

if(HARDINFO2_QT5)
target_link_libraries(qgears2
  GL
  pthread
  Qt5::OpenGL
  Qt5::Widgets
  Qt5::Gui
  Qt5::Core
)
endif()
if(HARDINFO2_QT6)
target_link_libraries(qgears2
  GL
  pthread
  Qt6::OpenGL
  Qt6::Widgets
  Qt6::OpenGLWidgets
  Qt6::Gui
  Qt6::Core
)
endif()

install(FILES ${CMAKE_BINARY_DIR}/deps/qgears2/qgears2 DESTINATION ${CMAKE_INSTALL_LIBDIR}/hardinfo2/modules
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
      
message(STATUS "qgears2 module added - DONE")
