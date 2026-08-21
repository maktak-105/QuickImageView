cmake_minimum_required(VERSION 3.20)
project(QuickImageView VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(QuickImageView WIN32 core/native/main.cpp)
target_compile_definitions(QuickImageView PRIVATE UNICODE _UNICODE NOMINMAX)
target_link_libraries(QuickImageView PRIVATE windowscodecs ole32 user32 gdi32 shlwapi shell32 comdlg32)

if (MINGW)
  target_link_options(QuickImageView PRIVATE -municode)
endif()

enable_testing()
add_test(NAME smoke_self_test COMMAND QuickImageView --self-test)
