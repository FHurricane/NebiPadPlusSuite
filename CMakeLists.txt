cmake_minimum_required(VERSION 3.21)

project(NebiPadPlus VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Svg PrintSupport Multimedia MultimediaWidgets Charts Network)
find_package(Threads REQUIRED)
find_package(Qt6 6.2 QUIET COMPONENTS WebEngineWidgets)
find_package(Qt6 6.2 QUIET COMPONENTS Pdf)
find_package(podofo QUIET CONFIG HINTS "/opt/homebrew/opt/podofo/lib/cmake/podofo")
if(TARGET podofo::podofo)
    set(NEBIPAD_PODOFO_TARGET podofo::podofo)
elseif(TARGET podofo::podofo_shared)
    set(NEBIPAD_PODOFO_TARGET podofo::podofo_shared)
endif()

if(APPLE)
    enable_language(OBJCXX)
    set_source_files_properties(src/speechbackend_mac.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    set(SPEECH_PLATFORM_SOURCE src/speechbackend_mac.mm)
else()
    set(SPEECH_PLATFORM_SOURCE)
endif()

if(WIN32)
    enable_language(RC)
endif()

qt_add_executable(NebiPadPlus
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/editor.cpp
    src/editor.h
    src/textencoding.cpp
    src/textencoding.h
    src/language.cpp
    src/language.h
    src/languagehighlighter.cpp
    src/languagehighlighter.h
    src/lspclient.cpp
    src/lspclient.h
    src/recentfiles.cpp
    src/recentfiles.h
    src/localization.cpp
    src/localization.h
    src/welcomewindow.cpp
    src/welcomewindow.h
    src/suitewindow.cpp
    src/suitewindow.h
    src/imageeditdialog.cpp
    src/imageeditdialog.h
    src/audioanalysis.cpp
    src/audioanalysis.h
    src/dictationcontroller.cpp
    src/dictationcontroller.h
    src/dictationoverlay.cpp
    src/dictationoverlay.h
    src/recordingcontroller.cpp
    src/recordingcontroller.h
    src/audioringbuffer.cpp
    src/audioringbuffer.h
    src/recordingdsp.cpp
    src/recordingdsp.h
    src/audiolinks.cpp
    src/audiolinks.h
    src/savepaths.cpp
    src/savepaths.h
    src/speechbackend.h
    src/pdfpodofo.cpp
    src/pdfpodofo.h
    src/googledriveclient.cpp
    src/googledriveclient.h
    src/googlecloudmenu.cpp
    src/googlecloudmenu.h
    src/speechbackend_whisper.cpp
    ${SPEECH_PLATFORM_SOURCE}
    assets/icons.qrc
    assets/brand.qrc
)

target_link_libraries(NebiPadPlus PRIVATE Qt6::Widgets Qt6::Svg Qt6::PrintSupport Qt6::Multimedia Qt6::MultimediaWidgets Qt6::Charts Qt6::Network Threads::Threads)
if(TARGET Qt6::WebEngineWidgets)
    target_link_libraries(NebiPadPlus PRIVATE Qt6::WebEngineWidgets)
    target_compile_definitions(NebiPadPlus PRIVATE NEBIPAD_HAS_WEBENGINE=1)
endif()
if(NEBIPAD_PODOFO_TARGET)
    target_link_libraries(NebiPadPlus PRIVATE ${NEBIPAD_PODOFO_TARGET})
    target_compile_definitions(NebiPadPlus PRIVATE NEBIPAD_HAS_PODOFO=1)
endif()
if(TARGET Qt6::Pdf)
    target_link_libraries(NebiPadPlus PRIVATE Qt6::Pdf)
    target_compile_definitions(NebiPadPlus PRIVATE NEBIPAD_HAS_QTPDF=1)
endif()

if(APPLE)
    set_source_files_properties(assets/NebiPad.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(NebiPadPlus PRIVATE assets/NebiPad.icns)
    target_link_libraries(NebiPadPlus PRIVATE "-framework Speech" "-framework AVFoundation")
    # A developer build may reuse a bundle previously processed by macdeployqt.
    # Remove those copied Qt binaries before relinking; otherwise the new
    # executable loads Homebrew Qt while the old Cocoa plugin loads bundled Qt.
    add_custom_command(TARGET NebiPadPlus PRE_LINK
        COMMAND ${CMAKE_COMMAND} -E remove_directory
                "$<TARGET_BUNDLE_DIR:NebiPadPlus>/Contents/Frameworks"
        COMMAND ${CMAKE_COMMAND} -E remove_directory
                "$<TARGET_BUNDLE_DIR:NebiPadPlus>/Contents/PlugIns"
        COMMENT "Removing stale deployed Qt libraries before relinking")
elseif(WIN32)
    target_sources(NebiPadPlus PRIVATE assets/NebiPad.rc)
    target_include_directories(NebiPadPlus PRIVATE assets)
endif()

set_target_properties(NebiPadPlus PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "NebiPad+"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.nebipad.plus"
    MACOSX_BUNDLE_ICON_FILE "NebiPad.icns"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/assets/Info.plist"
)

include(GNUInstallDirs)
install(TARGETS NebiPadPlus
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    install(FILES assets/nebipadplus.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES assets/nebipad-logo-512.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps RENAME nebipadplus.png)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(audioringbuffer_test tests/audioringbuffer_test.cpp src/audioringbuffer.cpp)
    target_include_directories(audioringbuffer_test PRIVATE src)
    target_link_libraries(audioringbuffer_test PRIVATE Threads::Threads)
    add_test(NAME audioringbuffer_test COMMAND audioringbuffer_test)

    add_executable(recordingdsp_test tests/recordingdsp_test.cpp src/recordingdsp.cpp)
    target_include_directories(recordingdsp_test PRIVATE src)
    add_test(NAME recordingdsp_test COMMAND recordingdsp_test)

    add_executable(savepaths_test tests/savepaths_test.cpp src/savepaths.cpp)
    target_include_directories(savepaths_test PRIVATE src)
    target_link_libraries(savepaths_test PRIVATE Qt6::Core)
    add_test(NAME savepaths_test COMMAND savepaths_test)

    add_executable(audiolinks_test tests/audiolinks_test.cpp src/audiolinks.cpp)
    target_include_directories(audiolinks_test PRIVATE src)
    target_link_libraries(audiolinks_test PRIVATE Qt6::Core)
    add_test(NAME audiolinks_test COMMAND audiolinks_test)

    add_executable(audioanalysis_test tests/audioanalysis_test.cpp src/audioanalysis.cpp)
    target_include_directories(audioanalysis_test PRIVATE src)
    target_link_libraries(audioanalysis_test PRIVATE Qt6::Core)
    add_test(NAME audioanalysis_test COMMAND audioanalysis_test)

    add_executable(textencoding_test tests/textencoding_test.cpp src/textencoding.cpp)
    target_include_directories(textencoding_test PRIVATE src)
    target_link_libraries(textencoding_test PRIVATE Qt6::Core)
    add_test(NAME textencoding_test COMMAND textencoding_test)

    add_executable(language_catalog_test tests/language_catalog_test.cpp src/language.cpp)
    target_include_directories(language_catalog_test PRIVATE src)
    target_link_libraries(language_catalog_test PRIVATE Qt6::Core)
    add_test(NAME language_catalog_test COMMAND language_catalog_test)

    add_executable(recentfiles_test tests/recentfiles_test.cpp src/recentfiles.cpp)
    target_include_directories(recentfiles_test PRIVATE src)
    target_link_libraries(recentfiles_test PRIVATE Qt6::Core)
    add_test(NAME recentfiles_test COMMAND recentfiles_test)

    add_executable(brand_test tests/brand_test.cpp assets/brand.qrc)
    target_link_libraries(brand_test PRIVATE Qt6::Widgets Qt6::Svg)
    add_test(NAME brand_test COMMAND brand_test)
    set_tests_properties(brand_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_executable(welcome_test tests/welcome_test.cpp
        src/welcomewindow.cpp src/welcomewindow.h
        src/localization.cpp src/localization.h
        src/recentfiles.cpp src/recentfiles.h
        assets/brand.qrc assets/icons.qrc)
    target_include_directories(welcome_test PRIVATE src)
    target_link_libraries(welcome_test PRIVATE Qt6::Widgets Qt6::Svg)
    add_test(NAME welcome_test COMMAND welcome_test)
    set_tests_properties(welcome_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_executable(lspclient_test tests/lspclient_test.cpp src/lspclient.cpp src/lspclient.h)
    target_include_directories(lspclient_test PRIVATE src)
    target_link_libraries(lspclient_test PRIVATE Qt6::Core)
    add_test(NAME lspclient_test COMMAND lspclient_test)

    add_executable(taborder_test tests/taborder_test.cpp)
    target_link_libraries(taborder_test PRIVATE Qt6::Widgets)
    add_test(NAME taborder_test COMMAND taborder_test)
    set_tests_properties(taborder_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_executable(menuvisibility_test
        tests/menuvisibility_test.cpp
        src/mainwindow.cpp src/mainwindow.h
        src/editor.cpp src/editor.h
        src/textencoding.cpp src/textencoding.h
        src/language.cpp src/language.h
        src/languagehighlighter.cpp src/languagehighlighter.h
        src/lspclient.cpp src/lspclient.h
        src/recentfiles.cpp src/recentfiles.h
        src/audioanalysis.cpp src/audioanalysis.h
        src/dictationcontroller.cpp src/dictationcontroller.h
        src/dictationoverlay.cpp src/dictationoverlay.h
        src/recordingcontroller.cpp src/recordingcontroller.h
        src/audioringbuffer.cpp src/audioringbuffer.h
        src/recordingdsp.cpp src/recordingdsp.h
        src/audiolinks.cpp src/audiolinks.h
        src/savepaths.cpp src/savepaths.h
        src/googledriveclient.cpp src/googledriveclient.h
        src/googlecloudmenu.cpp src/googlecloudmenu.h
        src/speechbackend.h src/speechbackend_whisper.cpp ${SPEECH_PLATFORM_SOURCE}
        assets/icons.qrc
    )
    target_include_directories(menuvisibility_test PRIVATE src)
    target_link_libraries(menuvisibility_test PRIVATE Qt6::Widgets Qt6::Svg Qt6::PrintSupport Qt6::Multimedia Qt6::Network Threads::Threads)
    if(APPLE)
        target_link_libraries(menuvisibility_test PRIVATE "-framework Speech" "-framework AVFoundation")
    endif()
    add_test(NAME menuvisibility_test COMMAND menuvisibility_test)
    set_tests_properties(menuvisibility_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    add_executable(suitewindow_test tests/suitewindow_test.cpp src/suitewindow.cpp src/suitewindow.h src/imageeditdialog.cpp src/imageeditdialog.h src/recentfiles.cpp src/recentfiles.h src/pdfpodofo.cpp src/pdfpodofo.h src/audioanalysis.cpp src/audioanalysis.h src/dictationcontroller.cpp src/dictationcontroller.h src/dictationoverlay.cpp src/dictationoverlay.h src/recordingcontroller.cpp src/recordingcontroller.h src/audioringbuffer.cpp src/audioringbuffer.h src/recordingdsp.cpp src/recordingdsp.h src/audiolinks.cpp src/audiolinks.h src/googledriveclient.cpp src/googledriveclient.h src/googlecloudmenu.cpp src/googlecloudmenu.h src/speechbackend.h src/speechbackend_whisper.cpp ${SPEECH_PLATFORM_SOURCE} assets/icons.qrc)
    target_include_directories(suitewindow_test PRIVATE src)
    target_link_libraries(suitewindow_test PRIVATE Qt6::Widgets Qt6::Svg Qt6::PrintSupport Qt6::Multimedia Qt6::MultimediaWidgets Qt6::Charts Qt6::Network Threads::Threads)
    if(APPLE)
        target_link_libraries(suitewindow_test PRIVATE "-framework Speech" "-framework AVFoundation")
    endif()
    if(TARGET Qt6::WebEngineWidgets)
        target_link_libraries(suitewindow_test PRIVATE Qt6::WebEngineWidgets)
        target_compile_definitions(suitewindow_test PRIVATE NEBIPAD_HAS_WEBENGINE=1)
    endif()
    if(TARGET Qt6::Pdf)
        target_link_libraries(suitewindow_test PRIVATE Qt6::Pdf)
        target_compile_definitions(suitewindow_test PRIVATE NEBIPAD_HAS_QTPDF=1)
    endif()
    if(NEBIPAD_PODOFO_TARGET)
        target_link_libraries(suitewindow_test PRIVATE ${NEBIPAD_PODOFO_TARGET})
        target_compile_definitions(suitewindow_test PRIVATE NEBIPAD_HAS_PODOFO=1)
    endif()
    add_test(NAME suitewindow_test COMMAND suitewindow_test)
    set_tests_properties(suitewindow_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
