set(HG_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
cmake_path(NORMAL_PATH HG_ROOT_DIR)

set(HG_THIRDPARTY_DIR "${HG_ROOT_DIR}/axmol/3rdparty")

if(NOT EXISTS "${HG_THIRDPARTY_DIR}/rapidjson/document.h")
  message(FATAL_ERROR "Axmol submodule is missing. Run: git submodule update --init --depth 1")
endif()

option(HG_WARNINGS_AS_ERRORS "Treat warnings as errors in project code" OFF)

function(hg_configure_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    if(HG_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wno-unused-parameter)
    if(HG_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endfunction()

file(GLOB_RECURSE HG_CORE_FILES CONFIGURE_DEPENDS
  "${HG_ROOT_DIR}/Source/Core/*.h"
  "${HG_ROOT_DIR}/Source/Core/*.cpp"
)

add_library(hg_core STATIC ${HG_CORE_FILES})
target_compile_features(hg_core PUBLIC cxx_std_20)
target_include_directories(hg_core PUBLIC "${HG_ROOT_DIR}/Source")
target_include_directories(hg_core SYSTEM PRIVATE "${HG_THIRDPARTY_DIR}")
set_target_properties(hg_core PROPERTIES FOLDER "HorrorGame")
hg_configure_warnings(hg_core)

if(NOT ANDROID AND NOT IOS AND NOT TVOS AND NOT WASM)
  file(GLOB HG_TOOL_FILES CONFIGURE_DEPENDS "${HG_ROOT_DIR}/Tools/hg_tool/*.h" "${HG_ROOT_DIR}/Tools/hg_tool/*.cpp")
  add_executable(hg_tool ${HG_TOOL_FILES})
  target_link_libraries(hg_tool PRIVATE hg_core)
  set_target_properties(hg_tool PROPERTIES FOLDER "HorrorGame" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/hg_tool")
  hg_configure_warnings(hg_tool)

  option(HG_BUILD_TESTS "Build unit tests" ON)

  if(HG_BUILD_TESTS)
    file(GLOB_RECURSE HG_TEST_FILES CONFIGURE_DEPENDS
      "${HG_ROOT_DIR}/Tests/*.h"
      "${HG_ROOT_DIR}/Tests/*.cpp"
    )

    add_executable(hg_tests ${HG_TEST_FILES} "${HG_ROOT_DIR}/Tools/hg_tool/ContentCheck.cpp")
    target_link_libraries(hg_tests PRIVATE hg_core)
    target_include_directories(hg_tests PRIVATE "${HG_ROOT_DIR}/Tools/hg_tool")
    target_include_directories(hg_tests SYSTEM PRIVATE "${HG_THIRDPARTY_DIR}/doctest")
    target_compile_definitions(hg_tests PRIVATE HG_CONTENT_DIR="${HG_ROOT_DIR}/Content")
    set_target_properties(hg_tests PROPERTIES FOLDER "HorrorGame" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/hg_tests")
    hg_configure_warnings(hg_tests)

    enable_testing()
    add_test(NAME hg_tests COMMAND hg_tests)
    add_test(NAME content_validation COMMAND hg_tool validate "${HG_ROOT_DIR}/Content")
  endif()
endif()
