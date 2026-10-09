cmake_minimum_required(VERSION 3.22...4.1)

set(APP_NAME HorrorGame)
set(HG_EDITOR_NAME HorrorGameEditor)
set(HG_BUNDLE_ID "com.constantinemanychev.horrorgame")

project(${APP_NAME})

option(HG_CORE_ONLY "Build only the engine independent core library, tools and tests" OFF)

if(HG_CORE_ONLY)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  include(cmake/HorrorGameCore.cmake)
  return()
endif()

if(XCODE)
  set(CMAKE_XCODE_GENERATE_TOP_LEVEL_PROJECT_ONLY TRUE)
endif()

set(_is_axmol_embed FALSE)
set(_AX_USE_PREBUILT FALSE)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

include(AXGameEngineOptions)
include(AXGameEngineSetup)

include(cmake/HorrorGameCore.cmake)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.25)
  set_target_properties(${_AX_CORE_LIB} PROPERTIES SYSTEM TRUE)
  if(TARGET ImGui)
    set_target_properties(ImGui PROPERTIES SYSTEM TRUE)
  endif()
endif()

file(GLOB_RECURSE HG_RUNTIME_FILES CONFIGURE_DEPENDS
  Source/Runtime/*.h
  Source/Runtime/*.cpp
)

add_library(hg_runtime STATIC ${HG_RUNTIME_FILES})
target_link_libraries(hg_runtime PUBLIC hg_core ${_AX_CORE_LIB})
target_include_directories(hg_runtime PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source")
set_target_properties(hg_runtime PROPERTIES FOLDER "HorrorGame")
hg_configure_warnings(hg_runtime)

set(content_folder "${CMAKE_CURRENT_SOURCE_DIR}/Content")

if(APPLE)
  ax_mark_multi_resources(common_content_files RES_TO "Resources" FOLDERS ${content_folder})
elseif(WINDOWS)
  ax_mark_multi_resources(common_content_files RES_TO "Content" FOLDERS ${content_folder})
endif()

set(HG_PLATFORM_HEADERS)
set(HG_PLATFORM_SOURCES)

if(ANDROID)
  list(APPEND HG_PLATFORM_SOURCES proj.android/app/jni/main.cpp)
elseif(LINUX)
  list(APPEND HG_PLATFORM_SOURCES proj.linux/main.cpp ${common_content_files})
elseif(WINDOWS)
  list(APPEND HG_PLATFORM_HEADERS proj.win32/main.h proj.win32/resource.h)
  list(APPEND HG_PLATFORM_SOURCES proj.win32/main.cpp proj.win32/game.rc ${common_content_files})
elseif(APPLE)
  if(IOS)
    list(APPEND HG_PLATFORM_HEADERS
      proj.ios_mac/ios/GameAppController.h
      proj.ios_mac/ios/GameViewController.h
      proj.ios_mac/ios/GameSceneDelegate.h
    )
    set(APP_UI_RES
      proj.ios_mac/ios/targets/ios/LaunchScreenBackground.png
      proj.ios_mac/ios/targets/ios/LaunchScreen.storyboard
      proj.ios_mac/ios/targets/ios/Images.xcassets
    )
    list(APPEND HG_PLATFORM_SOURCES
      proj.ios_mac/ios/main.mm
      proj.ios_mac/ios/GameAppController.mm
      proj.ios_mac/ios/GameViewController.mm
      proj.ios_mac/ios/GameSceneDelegate.mm
      proj.ios_mac/ios/Prefix.pch
      ${APP_UI_RES}
    )
  elseif(MACOSX)
    set(APP_UI_RES proj.ios_mac/mac/Icon.icns)
    list(APPEND HG_PLATFORM_SOURCES
      proj.ios_mac/mac/main.cpp
      proj.ios_mac/mac/Prefix.pch
      ${APP_UI_RES}
    )
  endif()
  list(APPEND HG_PLATFORM_SOURCES ${common_content_files})
endif()

file(GLOB HG_APP_FILES CONFIGURE_DEPENDS Source/App/*.h Source/App/*.cpp)

function(hg_add_application target)
  cmake_parse_arguments(opt "" "BUNDLE_ID" "FILES" ${ARGN})

  set(APP_NAME ${target})
  set(target_files ${HG_APP_FILES} ${opt_FILES} ${HG_PLATFORM_HEADERS} ${HG_PLATFORM_SOURCES})

  if(NOT ANDROID)
    add_executable(${target} ${target_files})
  else()
    add_library(${target} SHARED ${target_files})
    if(NOT BUILD_SHARED_LIBS)
      target_link_libraries(${target} -Wl,--whole-archive cpp_android_spec -Wl,--no-whole-archive)
    endif()
    config_android_shared_libs("dev.axmol.lib" "${CMAKE_CURRENT_SOURCE_DIR}/proj.android/app/src")
  endif()

  target_link_libraries(${target} hg_runtime)
  target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source/App" "${CMAKE_CURRENT_SOURCE_DIR}/Source")
  set_target_properties(${target} PROPERTIES FOLDER "HorrorGame")
  hg_configure_warnings(${target})

  ax_setup_app_config(${target})

  if((NOT APPLE) AND (NOT WINRT))
    ax_get_resource_path(APP_RES_DIR ${target})
    ax_sync_target_res(${target} LINK_TO ${APP_RES_DIR} FOLDERS ${content_folder} SYM_LINK 1)
    if(WINDOWS AND (NOT (CMAKE_GENERATOR MATCHES "Ninja")))
      set_property(TARGET ${target} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${content_folder}")
    endif()
  endif()

  ax_setup_app_props(${target})

  if(APPLE)
    set_target_properties(${target} PROPERTIES RESOURCE "${APP_UI_RES}")
    set_xcode_property(${target} INSTALL_PATH "\$(LOCAL_APPS_DIR)")
    set_xcode_property(${target} PRODUCT_BUNDLE_IDENTIFIER "${opt_BUNDLE_ID}")
    if(MACOSX)
      set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/proj.ios_mac/mac/Info.plist")
    elseif(IOS)
      set_target_properties(${target} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/proj.ios_mac/ios/targets/ios/Info.plist")
      set_xcode_property(${target} ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon")
      set_xcode_property(${target} TARGETED_DEVICE_FAMILY "1,2")
    endif()
  elseif(WINDOWS)
    ax_sync_target_dlls(${target})
  endif()
endfunction()

file(GLOB_RECURSE HG_GAME_FILES CONFIGURE_DEPENDS Source/Game/*.h Source/Game/*.cpp)
hg_add_application(${APP_NAME} FILES ${HG_GAME_FILES} BUNDLE_ID ${HG_BUNDLE_ID})

if(NOT DEFINED BUILD_ENGINE_DONE AND WINDOWS)
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${APP_NAME})
endif()

option(HG_BUILD_EDITOR "Build the level editor on desktop platforms" ON)

if(HG_BUILD_EDITOR AND AX_ENABLE_EXT_IMGUI AND (LINUX OR MACOSX OR (WINDOWS AND NOT WINRT)))
  file(GLOB_RECURSE HG_EDITOR_FILES CONFIGURE_DEPENDS Source/Editor/*.h Source/Editor/*.cpp)
  hg_add_application(${HG_EDITOR_NAME} FILES ${HG_EDITOR_FILES} BUNDLE_ID "${HG_BUNDLE_ID}.editor")
  target_compile_definitions(${HG_EDITOR_NAME} PRIVATE HG_CONTENT_SOURCE_DIR="${content_folder}")
endif()
