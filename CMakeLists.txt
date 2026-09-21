cmake_minimum_required(VERSION 3.20)
project(QuickImageView VERSION 4.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Quick QuickControls2 QuickTest Test)
qt_standard_project_setup(REQUIRES 6.10)
enable_testing()

qt_add_executable(QuickImageViewQt WIN32
  src/app/qt_main.cpp
  src/app/qt_app_controller.cpp
  src/app/qt_app_controller.h
  src/app/image_engine.cpp
  src/app/image_engine.h
  src/app/quick_image_provider.cpp
  src/app/quick_image_provider.h
  src/app/qt_resource.rc
)
set_target_properties(QuickImageViewQt PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/dist"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/dist"
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/dist"
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/dist"
)
target_compile_definitions(QuickImageViewQt PRIVATE QUICKIMAGEVIEW_VERSION="${PROJECT_VERSION}")
target_link_libraries(QuickImageViewQt PRIVATE Qt6::Quick Qt6::QuickControls2 windowscodecs ole32 shell32 advapi32)

qt_add_qml_module(QuickImageViewQt
  URI QuickImageView
  VERSION 1.0
  QML_FILES
    src/ui/Main.qml
    src/ui/AboutDialog.qml
    src/ui/HelpView.qml
    src/ui/InfoWindow.qml
    src/ui/SettingsDialog.qml
)
qt_add_resources(QuickImageViewQt "QuickImageViewEmbeddedResources"
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
)
target_compile_definitions(qiv_controller_tests PRIVATE QUICKIMAGEVIEW_VERSION="${PROJECT_VERSION}")
target_include_directories(qiv_controller_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/app")
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
