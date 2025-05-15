cmake_minimum_required(VERSION 3.16)
project(sioyek VERSION 2.0.0 LANGUAGES C CXX)

option(USE_SYSTEM_MUPDF "Use system mupdf" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS 3DInput Gui Network OpenGL QuickWidgets Svg TextToSpeech Widgets)
find_package(Qt6 OPTIONAL_COMPONENTS OpenGLWidgets)
# find_package(WrapFreetype 2.2.0 REQUIRED)
# find_package(WrapZLIB 1.0.8)
# find_package(JPEG)
# find_package(WrapHarfbuzz 2.6.0)

qt_standard_project_setup()

qt_add_resources(SOURCES resources.qrc)

qt_add_executable(sioyek WIN32 MACOSX_BUNDLE
    fzf/fzf.c fzf/fzf.h
    microtex/atom/atom_basic.cpp
    microtex/atom/atom_char.cpp
    microtex/atom/atom_impl.cpp
    microtex/atom/atom_matrix.cpp
    microtex/atom/atom_row.cpp
    microtex/atom/atom_space.cpp
    microtex/atom/colors_def.cpp
    microtex/atom/unit_conversion.cpp
    microtex/box/box.cpp
    microtex/box/box_factory.cpp
    microtex/box/box_group.cpp
    microtex/box/box_single.cpp
    microtex/core/core.cpp
    microtex/core/formula.cpp
    microtex/core/formula_def.cpp
    microtex/core/glue.cpp
    microtex/core/localized_num.cpp
    microtex/core/macro.cpp
    microtex/core/macro_def.cpp
    microtex/core/macro_impl.cpp
    microtex/core/parser.cpp
    microtex/fonts/alphabet.cpp
    microtex/fonts/font_basic.cpp
    microtex/fonts/font_info.cpp
    microtex/fonts/fonts.cpp
    microtex/latex.cpp
    microtex/platform/cairo/graphic_cairo.cpp
    microtex/platform/gdi_win/graphic_win32.cpp
    microtex/platform/qt/graphic_qt.cpp
    microtex/platform/skia/graphic_skia.cpp
    microtex/render.cpp
    microtex/res/builtin/formula_mappings.res.cpp
    microtex/res/builtin/symbol_mapping.res.cpp
    microtex/res/builtin/tex_param.res.cpp
    microtex/res/builtin/tex_symbols.res.cpp
    microtex/res/font/bi10.def.cpp
    microtex/res/font/bx10.def.cpp
    microtex/res/font/cmbsy10.def.cpp
    microtex/res/font/cmbx10.def.cpp
    microtex/res/font/cmbxti10.def.cpp
    microtex/res/font/cmex10.def.cpp
    microtex/res/font/cmmi10.def.cpp
    microtex/res/font/cmmi10_unchanged.def.cpp
    microtex/res/font/cmmib10.def.cpp
    microtex/res/font/cmmib10_unchanged.def.cpp
    microtex/res/font/cmr10.def.cpp
    microtex/res/font/cmss10.def.cpp
    microtex/res/font/cmssbx10.def.cpp
    microtex/res/font/cmssi10.def.cpp
    microtex/res/font/cmsy10.def.cpp
    microtex/res/font/cmti10.def.cpp
    microtex/res/font/cmti10_unchanged.def.cpp
    microtex/res/font/cmtt10.def.cpp
    microtex/res/font/dsrom10.def.cpp
    microtex/res/font/eufb10.def.cpp
    microtex/res/font/eufm10.def.cpp
    microtex/res/font/i10.def.cpp
    microtex/res/font/moustache.def.cpp
    microtex/res/font/msam10.def.cpp
    microtex/res/font/msbm10.def.cpp
    microtex/res/font/r10.def.cpp
    microtex/res/font/r10_unchanged.def.cpp
    microtex/res/font/rsfs10.def.cpp
    microtex/res/font/sb10.def.cpp
    microtex/res/font/sbi10.def.cpp
    microtex/res/font/si10.def.cpp
    microtex/res/font/special.def.cpp
    microtex/res/font/ss10.def.cpp
    microtex/res/font/stmary10.def.cpp
    microtex/res/font/tt10.def.cpp
    microtex/res/parser/font_parser.cpp
    microtex/res/parser/formula_parser.cpp
    microtex/res/reg/builtin_font_reg.cpp
    microtex/res/reg/builtin_syms_reg.cpp
    microtex/res/sym/amsfonts.def.cpp
    microtex/res/sym/amssymb.def.cpp
    microtex/res/sym/base.def.cpp
    microtex/res/sym/stmaryrd.def.cpp
    microtex/res/sym/symspecial.def.cpp
    microtex/samples/gtkmm_main.cpp
    microtex/samples/mem_check_main.cpp
    microtex/samples/qt_main.cpp
    microtex/samples/qt_mainwindow.cpp
    microtex/samples/qt_skiatexwidget.cpp
    microtex/samples/qt_texqmlitem.cpp
    microtex/samples/qt_texwidget.cpp
    microtex/samples/win32_main.cpp
    microtex/utils/microtex_utils.cpp
    microtex/utils/string_utils.cpp
    microtex/utils/utf.cpp
    microtex/xml/tinyxml2.cpp
    pdf_viewer/OpenWithApplication.cpp pdf_viewer/OpenWithApplication.h
    pdf_viewer/RunGuard.cpp pdf_viewer/RunGuard.h
    pdf_viewer/background_tasks.cpp pdf_viewer/background_tasks.h
    pdf_viewer/book.cpp pdf_viewer/book.h
    pdf_viewer/checksum.cpp pdf_viewer/checksum.h
    pdf_viewer/config.cpp pdf_viewer/config.h
    pdf_viewer/coordinates.cpp pdf_viewer/coordinates.h
    pdf_viewer/database.cpp pdf_viewer/database.h
    pdf_viewer/document.cpp pdf_viewer/document.h
    pdf_viewer/document_view.cpp pdf_viewer/document_view.h
    pdf_viewer/fts_fuzzy_match.h
    pdf_viewer/input.cpp pdf_viewer/input.h
    pdf_viewer/commands/command_parser.cpp pdf_viewer/commands/command_parser.h
    pdf_viewer/commands/base_commands.cpp pdf_viewer/commands/base_commands.h
    pdf_viewer/commands/annotation_commands.cpp pdf_viewer/commands/annotation_commands.h
    pdf_viewer/commands/misc_commands.cpp pdf_viewer/commands/misc_commands.h
    pdf_viewer/commands/navigation_commands.cpp pdf_viewer/commands/navigation_commands.h
    pdf_viewer/commands/network_commands.cpp pdf_viewer/commands/network_commands.h
    pdf_viewer/main.cpp
    pdf_viewer/main_widget.cpp pdf_viewer/main_widget.h
    pdf_viewer/mysortfilterproxymodel.cpp pdf_viewer/mysortfilterproxymodel.h
    pdf_viewer/network_manager.cpp pdf_viewer/network_manager.h
    pdf_viewer/new_file_checker.cpp pdf_viewer/new_file_checker.h
    pdf_viewer/path.cpp pdf_viewer/path.h
    pdf_viewer/pdf_renderer.cpp pdf_viewer/pdf_renderer.h
    pdf_viewer/pdf_view_opengl_widget.cpp pdf_viewer/pdf_view_opengl_widget.h
    pdf_viewer/rapidfuzz_amalgamated.hpp
    pdf_viewer/sqlite3.c pdf_viewer/sqlite3.h
    pdf_viewer/sqlite3ext.h
    pdf_viewer/status_string.cpp pdf_viewer/status_string.h
    pdf_viewer/touchui/TouchAudioButtons.cpp pdf_viewer/touchui/TouchAudioButtons.h
    pdf_viewer/touchui/TouchCheckbox.cpp pdf_viewer/touchui/TouchCheckbox.h
    pdf_viewer/touchui/TouchConfigMenu.cpp pdf_viewer/touchui/TouchConfigMenu.h
    pdf_viewer/touchui/TouchCopyOptions.cpp pdf_viewer/touchui/TouchCopyOptions.h
    pdf_viewer/touchui/TouchDeleteButton.cpp pdf_viewer/touchui/TouchDeleteButton.h
    pdf_viewer/touchui/TouchDrawControls.cpp pdf_viewer/touchui/TouchDrawControls.h
    pdf_viewer/touchui/TouchGenericButtons.cpp pdf_viewer/touchui/TouchGenericButtons.h
    pdf_viewer/touchui/TouchHighlightButtons.cpp pdf_viewer/touchui/TouchHighlightButtons.h
    pdf_viewer/touchui/TouchListView.cpp pdf_viewer/touchui/TouchListView.h
    pdf_viewer/touchui/TouchMacroEditor.cpp pdf_viewer/touchui/TouchMacroEditor.h
    pdf_viewer/touchui/TouchMainMenu.cpp pdf_viewer/touchui/TouchMainMenu.h
    pdf_viewer/touchui/TouchMarkSelector.cpp pdf_viewer/touchui/TouchMarkSelector.h
    pdf_viewer/touchui/TouchPageSelector.cpp pdf_viewer/touchui/TouchPageSelector.h
    pdf_viewer/touchui/TouchRangeSelectUI.cpp pdf_viewer/touchui/TouchRangeSelectUI.h
    pdf_viewer/touchui/TouchRectangleSelectUI.cpp pdf_viewer/touchui/TouchRectangleSelectUI.h
    pdf_viewer/touchui/TouchSearchButtons.cpp pdf_viewer/touchui/TouchSearchButtons.h
    pdf_viewer/touchui/TouchSettings.cpp pdf_viewer/touchui/TouchSettings.h
    pdf_viewer/touchui/TouchSlider.cpp pdf_viewer/touchui/TouchSlider.h
    pdf_viewer/touchui/TouchTextEdit.cpp pdf_viewer/touchui/TouchTextEdit.h
    pdf_viewer/touchui/TouchChat.cpp pdf_viewer/touchui/TouchChat.h
    pdf_viewer/ui.cpp pdf_viewer/ui.h
    pdf_viewer/utf8.h
    pdf_viewer/utf8/checked.h
    pdf_viewer/utf8/core.h
    pdf_viewer/utf8/unchecked.h
    pdf_viewer/utils.cpp pdf_viewer/utils.h
    ${SOURCES}
)

target_include_directories(sioyek PRIVATE
    microtex
    mupdf/include
    pdf_viewer
)

target_compile_definitions(sioyek PRIVATE
    BUILD_QT
    QT_3DINPUT_LIB
    QT_OPENGLEXTENSIONS_LIB
    QT_OPENGL_LIB
    QT_WIDGETS_LIB
    SQLITE_ENABLE_FTS5
)

target_link_libraries(sioyek PRIVATE
    Qt::3DInput
    Qt::Core
    Qt::Gui
    Qt::GuiPrivate
    Qt::Network
    Qt::OpenGL
    Qt::QuickWidgets
    Qt::Svg
    Qt::TextToSpeech
    Qt::Widgets
)


if(IOS)

    target_compile_definitions(sioyek PRIVATE
        SIOYEK_IOS
        SIOYEK_MOBILE
    )

    target_sources(sioyek PUBLIC
        ios/ios_specific.mm
    )
    target_sources(sioyek PUBLIC
        pdf_viewer/apple_specific.mm
    )

    target_link_libraries(sioyek PRIVATE
        "-framework AVFoundation"
        "-framework MediaPlayer"
        -L${CMAKE_CURRENT_SOURCE_DIR}/mupdf/build/-ios-arm64
        -lcombined
    )
    qt_add_ios_ffmpeg_libraries(sioyek)
endif()

if(NOT ANDROID AND NOT IOS)
    target_include_directories(sioyek PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/zlib
    )
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(sioyek PRIVATE
        LINUX_STANDARD_PATHS
  )
endif()

target_compile_definitions(sioyek PRIVATE
    SIOYEK_QT6
)

target_link_libraries(sioyek PRIVATE
    Qt::OpenGLWidgets
)

# find_package(ZLIB REQUIRED)
# find_package(HarfBuzz REQUIRED)
# find_package(SQLite3 REQUIRED)

if(ANDROID)
    target_compile_definitions(sioyek PRIVATE
        SIOYEK_ANDROID
        SIOYEK_MOBILE
    )

    target_link_libraries(sioyek PRIVATE
        -L${CMAKE_CURRENT_SOURCE_DIR}/libs/
        -lcrypto_3
        -lmupdf_java
        -lssl_3
    )
    set_target_properties(sioyek PROPERTIES ANDROID_EXTRA_LIBS
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/libcrypto_3.so
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/libmupdf_java.so
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/libssl_3.so
    )
    set_property(TARGET sioyek APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android)

    # set_target_properties(sioyek PROPERTIES DISTFILES
    #     ${CMAKE_CURRENT_SOURCE_DIR}/android/build.gradle
    #     ${CMAKE_CURRENT_SOURCE_DIR}/android/gradle.properties
    #     ${CMAKE_CURRENT_SOURCE_DIR}/android/gradle/wrapper/gradle-wrapper.jar
    #     ${CMAKE_CURRENT_SOURCE_DIR}/android/gradle/wrapper/gradle-wrapper.properties
    #     ${CMAKE_CURRENT_SOURCE_DIR}/android/gradlew
    #     ${CMAKE_CURRENT_SOURCE_DIR}/android/gradlew.bat
    #     ${CMAKE_CURRENT_SOURCE_DIR}/android/res/values/libs.xml
    #     ${CMAKE_CURRENT_SOURCE_DIR}/android/AndroidManifest.xml
    # )
endif()

target_compile_definitions(sioyek PRIVATE
    NON_PORTABLE
)

if(NOT ANDROID AND NOT IOS)
    target_sources(sioyek PUBLIC
        pdf_viewer/synctex/synctex_parser.c pdf_viewer/synctex/synctex_parser.h
        pdf_viewer/synctex/synctex_parser_utils.c pdf_viewer/synctex/synctex_parser_utils.h
    )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(sioyek PRIVATE
        /MP
        /bigobj
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(sioyek PRIVATE
        SIOYEK_DEVELOPER
    )
endif()

#add_library(fzf STATIC
#    fzf/fzf.c
#    fzf/fzf.h
#)

if(WIN32)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(sioyek PROPERTIES
            WIN32_EXECUTABLE FALSE
        )

        target_link_libraries(sioyek PRIVATE
            -L${CMAKE_CURRENT_SOURCE_DIR}/mupdf/platform/win32/x64/Debug
            -llibmupdf
            -L${CMAKE_CURRENT_SOURCE_DIR}/zlib
            -lzlib
        )
    else()
        target_link_libraries(sioyek PRIVATE
            -L${CMAKE_CURRENT_SOURCE_DIR}/mupdf/platform/win32/x64/Release
            -llibmupdf
            -Lzlib
            -lzlib
        )
    endif()


    target_compile_definitions(sioyek PRIVATE
        NOMINMAX
        _CRT_NONSTDC_NO_DEPRECATE
        _CRT_SECURE_NO_WARNINGS
    )

    target_link_libraries(sioyek PRIVATE
        opengl32.lib
    )

endif()


if(UNIX AND NOT ANDROID AND NOT APPLE)

    if((CMAKE_BUILD_TYPE STREQUAL "Debug") AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
        target_compile_options(sioyek PRIVATE
            -fstandalone-debug
        )
        target_link_options(sioyek PRIVATE
            -fuse-ld=mold
        )
    endif()

    if((CMAKE_BUILD_TYPE STREQUAL "Debug"))
        # use precompiled headers
        target_precompile_headers(sioyek PUBLIC
            "pch.h"
        )

    endif()



    # target_compile_options(sioyek
    #     -std=c++17
    # )
    if (USE_SYSTEM_MUPDF)
        target_compile_definitions(sioyek PRIVATE
            LINUX_STANDARD_PATHS
            NON_PORTABLE
        )

        target_link_libraries(sioyek PRIVATE
            -ldl
            -lmupdf
            -lmupdf-third
            -lgumbo
            -lharfbuzz
            -lfreetype
            -ljbig2dec
            -ljpeg
            -lmujs
            -lopenjp2
            -lz
        )
    else()
        target_link_libraries(sioyek PRIVATE
            -ldl
            -L${CMAKE_CURRENT_SOURCE_DIR}/mupdf/build/release
            -lmupdf
            -lmupdf-third
            -lmupdf-threads
            -lharfbuzz
            -lz
        )
    endif()

endif()

if(APPLE AND NOT IOS)
    target_sources(sioyek PUBLIC
        pdf_viewer/macos_specific.mm
    )
    target_sources(sioyek PUBLIC
        pdf_viewer/apple_specific.mm
    )

    target_link_libraries(sioyek PRIVATE
        "-framework AppKit"
        "-framework AVFoundation"
        -ldl
        -L${CMAKE_CURRENT_SOURCE_DIR}/mupdf/build/release
        -lmupdf
        -lmupdf-third
        -lmupdf-threads
        -lz
    )

    # target_compile_options(sioyek
    #     -std=c++17
    # )
endif()

# install(TARGETS sioyek
#     BUNDLE DESTINATION .
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
# )
#
# qt_generate_deploy_app_script(
#     TARGET sioyek
#     FILENAME_VARIABLE deploy_script
#     NO_UNSUPPORTED_PLATFORM_ERROR
# )
# install(SCRIPT ${deploy_script})
