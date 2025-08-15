#-------------------------------------------------
#  Copyright 2025 ESRI
#
#  All rights reserved under the copyright laws of the United States
#  and applicable international laws, treaties, and conventions.
#
#  You may freely redistribute and use this sample code, with or
#  without modification, provided you include the original copyright
#  notice and use restrictions.
#
#  See the Sample code usage restrictions document for further information.
#-------------------------------------------------
cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(arcTest LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(IOS)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "16.0" CACHE STRING "Minimum iOS deployment version" FORCE)
elseif(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS deployment version" FORCE)
endif()

find_package(Qt6 COMPONENTS REQUIRED Core Widgets Multimedia Positioning Sensors OpenGLWidgets WebSockets SerialPort)
if (Qt6Core_VERSION VERSION_LESS 6.8.2)
  message(FATAL_ERROR "This version of the ArcGIS Maps SDK for Qt requires at least Qt 6.8.2")
endif()
find_package(ArcGISRuntime 200.7.0 COMPONENTS REQUIRED Cpp)

set(SOURCE_FILES
  main.cpp
  ArcTest.cpp
  ArcTest.h
  resources.qrc)

# Add Windows resource file for icon
if(WIN32)
  set(SOURCE_FILES ${SOURCE_FILES} app_icon.rc)
endif()

qt_add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Set application properties for different platforms
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    MACOSX_BUNDLE_BUNDLE_NAME "ArcGIS Ground Station"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
    MACOSX_BUNDLE_COPYRIGHT "Copyright © 2025 Your Company. All rights reserved."
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourcompany.arctest"
    MACOSX_BUNDLE_ICON_FILE "app_icon.icns"
)

# Copy required dynamic libraries to the build folder as a post-build step.
if(DEFINED ArcGISRuntime_LIBRARIES)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${ArcGISRuntime_LIBRARIES}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
  ArcGISRuntime::Cpp
  Qt6::Core
  Qt6::Widgets
  Qt6::Multimedia
  Qt6::Positioning
  Qt6::Sensors
  Qt6::OpenGLWidgets
  Qt6::WebSockets
  Qt6::SerialPort)

# To integrate the toolkit, copy the `toolkitwidgets` and the `common` subdirectories from the toolkit
# into your project's directory. Then uncomment the following lines to add it to your project.
# See https://github.com/Esri/arcgis-maps-sdk-toolkit-qt for details
# add_subdirectory(toolkitwidgets)
# target_link_libraries(${PROJECT_NAME} PRIVATE libtoolkitwidgets)
