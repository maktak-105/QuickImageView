cmake_minimum_required(VERSION 3.20)
project(QuickImageView VERSION 4.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Quick QuickControls2 QuickTest Test)
qt_standard_project_setup(REQUIRES 6.10)
enable_testing()

qt_add_executable(QuickImageView WIN32
  src/app/qt_main.cpp
  src/app/qt_app_controller.cpp
  src/app/qt_app_controller.h
  src/app/image_engine.cpp
  src/app/image_engine.h
  src/app/quick_image_provider.cpp
  src/app/quick_image_provider.h
  src/app/native_file_dialog.cpp
  src/app/native_file_dialog.h
  src/app/window_theme.cpp
  src/app/window_theme.h
  src/app/QuickImageView.rc
)
set_target_properties(QuickImageView PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/dist"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/dist"
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/dist"
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/dist"
)
target_compile_definitions(QuickImageView PRIVATE QUICKIMAGEVIEW_VERSION="${PROJECT_VERSION}")
target_link_libraries(QuickImageView PRIVATE Qt6::Quick Qt6::QuickControls2 windowscodecs ole32 shell32 advapi32 uuid)

qt_add_qml_module(QuickImageView
  URI QuickImageView
  VERSION 1.0
  QML_FILES
    src/ui/Main.qml
    src/ui/AboutDialog.qml
    src/ui/HelpView.qml
    src/ui/InfoWindow.qml
    src/ui/SettingsDialog.qml
    src/ui/ReplaceImageDialog.qml
)
qt_add_resources(QuickImageView "QuickImageViewEmbeddedResources"
  PREFIX "/"
  BASE "${CMAKE_CURRENT_SOURCE_DIR}"
  FILES
    assets/maktak105-V04-01.jpg
    src/app/help/help.md
    src/app/help/help_jp.md
)

add_executable(qiv_controller_tests
  tests/qt/tst_qt_app_controller.cpp
  src/app/qt_app_controller.cpp
  src/app/qt_app_controller.h
  src/app/image_engine.cpp
  src/app/image_engine.h
  src/app/quick_image_provider.cpp
  src/app/quick_image_provider.h
  src/app/native_file_dialog.cpp
  src/app/native_file_dialog.h
)
target_compile_definitions(qiv_controller_tests PRIVATE QUICKIMAGEVIEW_VERSION="${PROJECT_VERSION}")
target_include_directories(qiv_controller_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/app")
target_link_libraries(qiv_controller_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Test windowscodecs ole32 shell32 advapi32 uuid)
add_test(NAME qiv_controller_tests COMMAND qiv_controller_tests)

# The QML tests use the real controller (as the global appController) with a temporary settings file.
add_executable(qiv_qml_tests
  tests/qml/qml_tests.cpp
  src/app/qt_app_controller.cpp
  src/app/qt_app_controller.h
  src/app/image_engine.cpp
  src/app/image_engine.h
  src/app/quick_image_provider.cpp
  src/app/quick_image_provider.h
  src/app/native_file_dialog.cpp
  src/app/native_file_dialog.h
)
target_include_directories(qiv_qml_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/app")
target_link_libraries(qiv_qml_tests PRIVATE Qt6::QuickTest Qt6::Quick Qt6::QuickControls2 Qt6::Gui windowscodecs ole32 shell32 advapi32 uuid)
target_compile_definitions(qiv_qml_tests PRIVATE
  QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/qml"
  QUICKIMAGEVIEW_VERSION="${PROJECT_VERSION}"
)
add_test(NAME qiv_qml_tests COMMAND qiv_qml_tests -platform offscreen)
set_tests_properties(qiv_qml_tests PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
)
