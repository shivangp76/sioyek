cmake_minimum_required(VERSION 3.16)
project(sioyek VERSION 2.0.0 LANGUAGES C CXX)
set(CMAKE_PREFIX_PATH /home/hex/Qt/6.7.2/gcc_64/)
set(CMAKE_MODULE_PATH /home/hex/Qt/6.7.2/gcc_64/lib/cmake/Qt6)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS 3DInput Gui Network OpenGL QuickWidgets Svg TextToSpeech Widgets)
find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS OpenGLWidgets)
find_package(JPEG)
find_package(WrapZLIB 1.0.8)
find_package(WrapFreetype 2.2.0 REQUIRED)
# find_package(WrapHarfbuzz 2.6.0)
# find_package(HarfBuzz REQUIRED)

qt_standard_project_setup()

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
    pdf_viewer/ui.cpp pdf_viewer/ui.h
    pdf_viewer/utf8.h
    pdf_viewer/utf8/checked.h
    pdf_viewer/utf8/core.h
    pdf_viewer/utf8/unchecked.h
    pdf_viewer/utils.cpp pdf_viewer/utils.h
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
    WrapZLIB::WrapZLIB
)
# link mupdf/build/release
target_link_libraries(sioyek PRIVATE
	${CMAKE_SOURCE_DIR}/mupdf/build/release/libmupdf.a
	${CMAKE_SOURCE_DIR}/mupdf/build/release/libmupdf-threads.a
	${CMAKE_SOURCE_DIR}/mupdf/build/release/libmupdf-third.a
	harfbuzz
)


# Resources:
set(resources_resource_files
    "begin.png"
    "data/command_docs.json"
    "data/config_docs.json"
    "data/embedding.npy"
    "data/linear.npy"
    "data/sioyek_documentation.json"
    "end.png"
    "icons/B.svg"
    "icons/B_white.svg"
    "icons/P.svg"
    "icons/P_white.svg"
    "icons/adjust.svg"
    "icons/arrow-begin.svg"
    "icons/arrow-end.svg"
    "icons/bookmark-add.svg"
    "icons/bookmark-g.svg"
    "icons/bookmark.svg"
    "icons/close.svg"
    "icons/copy.svg"
    "icons/document-page-number.svg"
    "icons/draw.svg"
    "icons/erase.svg"
    "icons/finger-index.svg"
    "icons/fit-horizontal-enabled.svg"
    "icons/fit-horizontal.svg"
    "icons/fullscreen-enabled.svg"
    "icons/fullscreen.svg"
    "icons/go-to-file.svg"
    "icons/google-scholar.svg"
    "icons/google.svg"
    "icons/highlight-g.svg"
    "icons/highlight.svg"
    "icons/home.svg"
    "icons/hourglass.svg"
    "icons/jump-right.svg"
    "icons/left.svg"
    "icons/link-enabled.svg"
    "icons/link.svg"
    "icons/load.svg"
    "icons/lock-enabled.svg"
    "icons/lock.svg"
    "icons/move.svg"
    "icons/next.svg"
    "icons/page.svg"
    "icons/paper-download.svg"
    "icons/pause.svg"
    "icons/pen.svg"
    "icons/play.svg"
    "icons/portal-enabled.svg"
    "icons/portal.svg"
    "icons/previous.svg"
    "icons/question.svg"
    "icons/right.svg"
    "icons/ruler-enabled.svg"
    "icons/ruler.svg"
    "icons/save.svg"
    "icons/screenshot.svg"
    "icons/search.svg"
    "icons/select_text.svg"
    "icons/selection-begin.svg"
    "icons/selection-end.svg"
    "icons/stop.svg"
    "icons/swap.svg"
    "icons/table-of-contents.svg"
    "icons/text-selection.svg"
    "icons/tts-enabled.svg"
    "icons/tts.svg"
    "icons/unlink.svg"
    "microtex_resources/.clatexmath-res_root"
    "microtex_resources/RES_README"
    "microtex_resources/SAMPLES.tex"
    "microtex_resources/cyrillic/LICENSE"
    "microtex_resources/cyrillic/cyrillic.map.xml"
    "microtex_resources/cyrillic/language_cyrillic.xml"
    "microtex_resources/cyrillic/mappings_cyrillic.xml"
    "microtex_resources/cyrillic/symbols_cyrillic.xml"
    "microtex_resources/cyrillic/wnbx10.ttf"
    "microtex_resources/cyrillic/wnbx10.xml"
    "microtex_resources/cyrillic/wnbxti10.ttf"
    "microtex_resources/cyrillic/wnbxti10.xml"
    "microtex_resources/cyrillic/wnr10.ttf"
    "microtex_resources/cyrillic/wnr10.xml"
    "microtex_resources/cyrillic/wnss10.ttf"
    "microtex_resources/cyrillic/wnss10.xml"
    "microtex_resources/cyrillic/wnssbx10.ttf"
    "microtex_resources/cyrillic/wnssbx10.xml"
    "microtex_resources/cyrillic/wnssi10.ttf"
    "microtex_resources/cyrillic/wnssi10.xml"
    "microtex_resources/cyrillic/wnti10.ttf"
    "microtex_resources/cyrillic/wnti10.xml"
    "microtex_resources/cyrillic/wntt10.ttf"
    "microtex_resources/cyrillic/wntt10.xml"
    "microtex_resources/fonts/base/cmex10.ttf"
    "microtex_resources/fonts/base/cmmi10.ttf"
    "microtex_resources/fonts/base/cmmib10.ttf"
    "microtex_resources/fonts/euler/eufb10.ttf"
    "microtex_resources/fonts/euler/eufm10.ttf"
    "microtex_resources/fonts/latin/bi10.ttf"
    "microtex_resources/fonts/latin/bx10.ttf"
    "microtex_resources/fonts/latin/cmr10.ttf"
    "microtex_resources/fonts/latin/i10.ttf"
    "microtex_resources/fonts/latin/optional/cmbx10.ttf"
    "microtex_resources/fonts/latin/optional/cmbxti10.ttf"
    "microtex_resources/fonts/latin/optional/cmss10.ttf"
    "microtex_resources/fonts/latin/optional/cmssbx10.ttf"
    "microtex_resources/fonts/latin/optional/cmssi10.ttf"
    "microtex_resources/fonts/latin/optional/cmti10.ttf"
    "microtex_resources/fonts/latin/optional/cmtt10.ttf"
    "microtex_resources/fonts/latin/r10.ttf"
    "microtex_resources/fonts/latin/sb10.ttf"
    "microtex_resources/fonts/latin/sbi10.ttf"
    "microtex_resources/fonts/latin/si10.ttf"
    "microtex_resources/fonts/latin/ss10.ttf"
    "microtex_resources/fonts/latin/tt10.ttf"
    "microtex_resources/fonts/licences/Knuth_License.txt"
    "microtex_resources/fonts/licences/License_for_dsrom.txt"
    "microtex_resources/fonts/licences/OFL.txt"
    "microtex_resources/fonts/maths/cmbsy10.ttf"
    "microtex_resources/fonts/maths/cmsy10.ttf"
    "microtex_resources/fonts/maths/msam10.ttf"
    "microtex_resources/fonts/maths/msbm10.ttf"
    "microtex_resources/fonts/maths/optional/dsrom10.ttf"
    "microtex_resources/fonts/maths/rsfs10.ttf"
    "microtex_resources/fonts/maths/special.ttf"
    "microtex_resources/fonts/maths/stmary10.ttf"
    "microtex_resources/greek/LICENSE"
    "microtex_resources/greek/fcmbipg.ttf"
    "microtex_resources/greek/fcmbipg.xml"
    "microtex_resources/greek/fcmbpg.ttf"
    "microtex_resources/greek/fcmbpg.xml"
    "microtex_resources/greek/fcmripg.ttf"
    "microtex_resources/greek/fcmripg.xml"
    "microtex_resources/greek/fcmrpg.ttf"
    "microtex_resources/greek/fcmrpg.xml"
    "microtex_resources/greek/fcsbpg.ttf"
    "microtex_resources/greek/fcsbpg.xml"
    "microtex_resources/greek/fcsropg.ttf"
    "microtex_resources/greek/fcsropg.xml"
    "microtex_resources/greek/fcsrpg.ttf"
    "microtex_resources/greek/fcsrpg.xml"
    "microtex_resources/greek/fctrpg.ttf"
    "microtex_resources/greek/fctrpg.xml"
    "microtex_resources/greek/greek.map.xml"
    "microtex_resources/greek/language_greek.xml"
    "microtex_resources/greek/mappings_greek.xml"
    "microtex_resources/greek/symbols_greek.xml"
    "pdf_viewer/keys.config"
    "pdf_viewer/prefs.config"
    "pdf_viewer/shaders/compiled_drawing.vertex"
    "pdf_viewer/shaders/compiled_line.fragment"
    "pdf_viewer/shaders/custom_colors.fragment"
    "pdf_viewer/shaders/dark_mode.fragment"
    "pdf_viewer/shaders/debug.fragment"
    "pdf_viewer/shaders/dot.fragment"
    "pdf_viewer/shaders/dot.vertex"
    "pdf_viewer/shaders/highlight.fragment"
    "pdf_viewer/shaders/line.fragment"
    "pdf_viewer/shaders/line.vertex"
    "pdf_viewer/shaders/separator.fragment"
    "pdf_viewer/shaders/simple.fragment"
    "pdf_viewer/shaders/simple.vertex"
    "pdf_viewer/shaders/stencil.fragment"
    "pdf_viewer/shaders/stencil.vertex"
    "pdf_viewer/shaders/unrendered_page.fragment"
    "pdf_viewer/shaders/vertical_bar.fragment"
    "pdf_viewer/shaders/vertical_bar_dark.fragment"
    "pdf_viewer/touchui/GenericListViewDelegate.qml"
    "pdf_viewer/touchui/TouchAudioButtons.qml"
    "pdf_viewer/touchui/TouchButtonGroup.qml"
    "pdf_viewer/touchui/TouchCheckbox.qml"
    "pdf_viewer/touchui/TouchConfigMenu.qml"
    "pdf_viewer/touchui/TouchCopyOptions.qml"
    "pdf_viewer/touchui/TouchDeleteButton.qml"
    "pdf_viewer/touchui/TouchDrawControls.qml"
    "pdf_viewer/touchui/TouchEnumSelector.qml"
    "pdf_viewer/touchui/TouchGenericButtons.qml"
    "pdf_viewer/touchui/TouchHighlightButtons.qml"
    "pdf_viewer/touchui/TouchHighlightsView.qml"
    "pdf_viewer/touchui/TouchListView.qml"
    "pdf_viewer/touchui/TouchMacroEditor.qml"
    "pdf_viewer/touchui/TouchMainMenu.qml"
    "pdf_viewer/touchui/TouchMarkSelector.qml"
    "pdf_viewer/touchui/TouchPageSelector.qml"
    "pdf_viewer/touchui/TouchRangeSelectUI.qml"
    "pdf_viewer/touchui/TouchRectangleSelectUI.qml"
    "pdf_viewer/touchui/TouchSearchButtons.qml"
    "pdf_viewer/touchui/TouchSettings.qml"
    "pdf_viewer/touchui/TouchSlider.qml"
    "pdf_viewer/touchui/TouchSymbolColorSelector.qml"
    "pdf_viewer/touchui/TouchTextEdit.qml"
    "pdf_viewer/touchui/TouchTreeView.qml"
    "python_api/LICENSE.txt"
    "python_api/README.md"
    "python_api/pyproject.toml"
    "python_api/src/sioyek/__init__.py"
    "python_api/src/sioyek/sioyek.py"
    "resources/fonts/ComputerModern/cmu.concrete-bold.ttf"
    "resources/fonts/ComputerModern/cmu.concrete-bolditalic.ttf"
    "resources/fonts/ComputerModern/cmu.concrete-italic.ttf"
    "resources/fonts/ComputerModern/cmu.concrete-roman.ttf"
    "resources/fonts/JetBrainsMono.ttf"
    "tutorial.pdf"
)

qt_add_resources(sioyek "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)

if(IOS)
    target_sources(sioyek PUBLIC
        ios/ios_specific.mm
    )

    target_link_libraries(sioyek PRIVATE
        "-framework AVFoundation"
        "-framework MediaPlayer"
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/mupdf/build/-ios-arm64
        combined
    )
endif()

if(NOT ANDROID AND NOT IOS)
    target_include_directories(sioyek PRIVATE
        zlib
    )
endif()

if((QT_VERSION_MAJOR GREATER 5))
    target_compile_definitions(sioyek PRIVATE
        SIOYEK_QT6
    )

    target_link_libraries(sioyek PRIVATE
        Qt::OpenGLWidgets
    )
endif()

if(NOT ((QT_VERSION_MAJOR GREATER 5)))
    target_link_libraries(sioyek PRIVATE
        openglextensions
    )
endif()

if(ANDROID)
    target_compile_definitions(sioyek PRIVATE
        SIOYEK_ANDROID
        SIOYEK_MOBILE
    )

    target_link_libraries(sioyek PRIVATE
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/libs/
        Qt::CorePrivate
        crypto_3
        mupdf_java
        ssl_3
    )
endif()


if(NOT ANDROID)
    target_sources(sioyek PUBLIC
        pdf_viewer/synctex/synctex_parser.c pdf_viewer/synctex/synctex_parser.h
        pdf_viewer/synctex/synctex_parser_utils.c pdf_viewer/synctex/synctex_parser_utils.h
    )
endif()


if(UNIX AND NOT ANDROID AND NOT APPLE)
	target_compile_options(sioyek PRIVATE
        -std=c++17
    )
endif()

target_compile_definitions(sioyek PRIVATE
SIOYEK_DEVELOPER
)

install(TARGETS sioyek
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET sioyek
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
