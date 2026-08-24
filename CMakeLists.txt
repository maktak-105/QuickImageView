cmake_minimum_required(VERSION 3.20)
project(QuickImageView VERSION 2.1.0 LANGUAGES CXX)

set(WEBP_BUILD_ANIM_UTILS OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_CWEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_DWEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_GIF2WEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_IMG2WEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_VWEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_WEBPINFO OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_LIBWEBPMUX OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_WEBPMUX OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/libwebp-1.6.0 third_party/libwebp-build EXCLUDE_FROM_ALL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(QuickImageView WIN32 core/native/main.cpp core/native/resource.rc)
set_target_properties(QuickImageView PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/binary"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/dist/binary"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/dist/binary"
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/dist/binary"
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/dist/binary"
)
target_compile_definitions(QuickImageView PRIVATE UNICODE _UNICODE NOMINMAX)
target_link_libraries(QuickImageView PRIVATE windowscodecs ole32 user32 gdi32 shlwapi shell32 comdlg32 uxtheme webp)

if (MINGW)
  target_link_options(QuickImageView PRIVATE
    -municode
    -static
    -static-libgcc
    -static-libstdc++
  )
endif()

enable_testing()
add_test(NAME smoke_self_test COMMAND QuickImageView --self-test)
