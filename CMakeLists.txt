cmake_minimum_required(VERSION 3.20)
project(QuickImageView VERSION 3.1.2 LANGUAGES CXX)

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

option(QUICKIMAGEVIEW_BUILD_QT "Build the Qt Quick migration preview" ON)

if (QUICKIMAGEVIEW_BUILD_QT)
  find_package(Qt6 6.10 QUIET COMPONENTS Quick QuickControls2 QuickTest Test)

  if (Qt6_FOUND)
    qt_standard_project_setup(REQUIRES 6.10)

    qt_add_executable(QuickImageViewQt WIN32
      core/native/qt_main.cpp
      core/native/qt_app_controller.cpp
      core/native/qt_app_controller.h
      core/native/image_engine.cpp
      core/native/image_engine.h
      core/native/quick_image_provider.cpp
      core/native/quick_image_provider.h
      core/native/qt_resource.rc
    )
    set_target_properties(QuickImageViewQt PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/binary"
      RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/dist/binary"
      RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/dist/binary"
      RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/dist/binary"
      RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/dist/binary"
    )
    target_compile_definitions(QuickImageViewQt PRIVATE QUICKIMAGEVIEW_VERSION="${PROJECT_VERSION}")
    target_link_libraries(QuickImageViewQt PRIVATE Qt6::Quick Qt6::QuickControls2 windowscodecs ole32)
    target_link_libraries(QuickImageViewQt PRIVATE Qt6::Quick Qt6::QuickControls2 windowscodecs ole32 shell32 advapi32)

    qt_add_qml_module(QuickImageViewQt
      URI QuickImageView
      VERSION 1.0
      QML_FILES
        qml/Main.qml
        qml/AboutDialog.qml
        qml/HelpView.qml
        qml/InfoWindow.qml
        qml/SettingsDialog.qml
    )
    qt_add_resources(QuickImageViewQt "QuickImageViewEmbeddedResources"
      PREFIX "/"
      BASE "${CMAKE_CURRENT_SOURCE_DIR}"
      FILES
        assets/maktak105-V04-01.jpg
        resources/help/help.md
        resources/help/help_jp.md
    )

    add_executable(qiv_controller_tests
      tests/qt/tst_qt_app_controller.cpp
      core/native/qt_app_controller.cpp
      core/native/qt_app_controller.h
      core/native/image_engine.cpp
      core/native/image_engine.h
      core/native/quick_image_provider.cpp
      core/native/quick_image_provider.h
    )
    target_compile_definitions(qiv_controller_tests PRIVATE QUICKIMAGEVIEW_VERSION="${PROJECT_VERSION}")
    target_include_directories(qiv_controller_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/core/native")
    target_link_libraries(qiv_controller_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Test windowscodecs ole32)
    target_link_libraries(qiv_controller_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Test windowscodecs ole32 shell32 advapi32)
    add_test(NAME qiv_controller_tests COMMAND qiv_controller_tests)

    add_executable(qiv_qml_tests tests/qml/qml_tests.cpp)
    target_link_libraries(qiv_qml_tests PRIVATE Qt6::QuickTest)
    target_compile_definitions(qiv_qml_tests PRIVATE
      QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/qml"
    )
    add_test(NAME qiv_qml_tests COMMAND qiv_qml_tests -platform offscreen)
    set_tests_properties(qiv_qml_tests PROPERTIES
      ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
    )
  else()
    message(STATUS "Qt 6.10+ was not found. The legacy Win32 target remains available.")
  endif()
endif()
