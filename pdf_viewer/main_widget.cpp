// deduplicate database code
// refactor database to use prepared statements
// make sure jsons exported by previous sioyek versions can be imported
// change find_closest_*_index and argminf to use the fact that the list is sorted and speed up the search (not important if there are not a ridiculous amount of highlight/bookmarks)
// make sure database migrations goes smoothly. Test with database files from previous sioyek versions.
// touch epub controls
// better tablet button handling, the current method is setting dependent
// continue high quality tts on ios and android when the app is minimized
// make sure pop_current_widget is called on all show_filtered_select_menus
// batch the todos
// make the action of download and clipboard paper configurable
// factorize click, scroll, etc. handling code
// add keyboard commands to control pinned portals
// bug: the result of ai queries is not uploaded to servers
// probably we need to convert onTextChanged to onDisplayTextChanged in more qml places

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <set>
#include <unordered_map>
#include <optional>
#include <memory>
#include <cctype>
#include <qpainterpath.h>
#include <qabstractitemmodel.h>
#include <qapplication.h>
#include <qboxlayout.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qdrag.h>
#include <qmenu.h>
#include <qdesktopservices.h>
#include <qtemporarydir.h>
#include <qtemporaryfile.h>

#ifndef SIOYEK_QT6
#include <qdesktopwidget.h>
#endif

#include <qkeyevent.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qtableview.h>
#include <qopenglfunctions.h>
#include <qpushbutton.h>
#include <qsortfilterproxymodel.h>
#include <qstringlistmodel.h>
#include <qtextedit.h>
#include <qtimer.h>
#include <qtreeview.h>
#include <qwindow.h>
#include <qstandardpaths.h>
#include <qprocess.h>
#include <qguiapplication.h>
#include <qmimedata.h>
#include <qscreen.h>
#include <QGestureEvent>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qlocalsocket.h>
#include <qbytearray.h>
#include <qscrollbar.h>
#include <qtexttospeech.h>
#include <qwidget.h>
#include <qjsengine.h>
#include <qqmlengine.h>
#include <qtextdocumentfragment.h>
#include <qmenubar.h>
#include <qstylehints.h>
#include <qhttpmultipart.h>
#include <qstringconverter.h>
#include <qmediaplayer.h>
#include <qaudiooutput.h>
#include <qregion.h>
#include <qtextdocument.h>

//#include "main_widget.moc"

#include <mupdf/fitz.h>

#include "input.h"
#include "database.h"
#include "book.h"
#include "utils.h"
#include "ui.h"
#include "pdf_renderer.h"
#include "document.h"
#include "document_view.h"
#include "pdf_view_opengl_widget.h"
#include "config.h"
#include "utf8.h"
#include "synctex/synctex_parser.h"
#include "path.h"
#include "background_tasks.h"
#include "touchui/TouchMarkSelector.h"
#include "checksum.h"
#include "touchui/TouchSettings.h"
#include "network_manager.h"
#include "status_string.h"

#include "main_widget.h"

#ifdef SIOYEK_ANDROID
#include <QtCore/private/qandroidextras_p.h>
#endif



extern "C" {
    #include <fzf/fzf.h>
}

#ifdef Q_OS_MACOS
extern "C" void changeTitlebarColor(WId, double, double, double, double);
extern "C" void hideWindowTitleBar(WId);
#endif

#ifdef SIOYEK_IOS
// extern "C" void iosTestFunc(NSString* text);

extern "C" void iosResumeFunc();
extern "C" void iosPauseFunc();
extern "C" AVSpeechSynthesizer* createSpeechSynthesizer();
extern "C" void iosPlayTextToSpeechInBackground(NSString* text, NSString* voiceName, double rate);
extern "C" int getLastSpokenWordLocation();
extern "C" int iosStopReading();

#endif

extern std::string APPLICATION_VERSION;

const std::wstring SERVER_SYMBOL = L"🌐";

extern int next_window_id;
extern std::map<std::wstring, std::wstring> SHELL_BOOKMARK_COMMANDS;

extern float SERVER_AND_LOCAL_DOCUMENT_MISMATCH_THRESHOLD;
extern bool SHOULD_USE_MULTIPLE_MONITORS;
extern bool MULTILINE_MENUS;
extern bool SORT_BOOKMARKS_BY_LOCATION;
extern bool SORT_HIGHLIGHTS_BY_LOCATION;
extern bool FLAT_TABLE_OF_CONTENTS;
extern bool HOVER_OVERVIEW;
extern bool WHEEL_ZOOM_ON_CURSOR;
extern float MOVE_SCREEN_PERCENTAGE;
extern std::wstring INVERSE_SEARCH_COMMAND;
// extern std::wstring PAPER_SEARCH_URL;
// extern std::wstring PAPER_SEARCH_URL_PATH;
// extern std::wstring PAPER_SEARCH_TILE_PATH;
// extern std::wstring PAPER_SEARCH_CONTRIB_PATH;
extern bool FUZZY_SEARCHING;
extern bool AUTO_RENAME_DOWNLOADED_PAPERS;
extern bool SHOW_DOCUMENTATION_IN_WIDGET;

extern float VISUAL_MARK_NEXT_PAGE_FRACTION;
extern float VISUAL_MARK_NEXT_PAGE_THRESHOLD;
extern float SMALL_PIXMAP_SCALE;
// extern std::wstring EXECUTE_COMMANDS[26];
extern float HIGHLIGHT_COLORS[26 * 3];
extern int STATUS_BAR_FONT_SIZE;

extern float VERTICAL_MOVE_AMOUNT;
extern float HORIZONTAL_MOVE_AMOUNT;

extern std::wstring TITLEBAR_FORMAT;
extern Path standard_data_path;
extern Path documentation_path;
extern Path default_config_path;
extern Path default_keys_path;
extern Path sioyek_js_path;
extern Path sioyek_access_token_path;
extern Path cached_tts_path;
extern std::vector<Path> user_config_paths;
extern std::vector<Path> user_keys_paths;
extern Path database_file_path;
extern Path tutorial_path;
extern Path last_opened_file_address_path;
extern Path sioyek_json_data_path;
extern Path auto_config_path;
extern std::wstring ITEM_LIST_PREFIX;
extern std::wstring SEARCH_URLS[26];
extern std::wstring MIDDLE_CLICK_SEARCH_ENGINE;
extern std::wstring SHIFT_MIDDLE_CLICK_SEARCH_ENGINE;
extern float DISPLAY_RESOLUTION_SCALE;
extern float STATUS_BAR_COLOR[3];
extern float STATUS_BAR_TEXT_COLOR[3];
extern int MAIN_WINDOW_SIZE[2];
extern int HELPER_WINDOW_SIZE[2];
extern int MAIN_WINDOW_MOVE[2];
extern int HELPER_WINDOW_MOVE[2];
extern float TOUCHPAD_SENSITIVITY;
extern int SINGLE_MAIN_WINDOW_SIZE[2];
extern int SINGLE_MAIN_WINDOW_MOVE[2];
extern float OVERVIEW_SIZE[2];
extern float OVERVIEW_OFFSET[2];
extern bool IGNORE_WHITESPACE_IN_PRESENTATION_MODE;
extern std::vector<MainWidget*> windows;
extern bool SHOW_DOC_PATH;
extern bool SINGLE_CLICK_SELECTS_WORDS;
extern std::wstring SHIFT_CLICK_COMMAND;
extern std::wstring CONTROL_CLICK_COMMAND;
extern std::wstring COMMAND_CLICK_COMMAND;
extern std::wstring COMMAND_RIGHT_CLICK_COMMAND;
extern std::wstring SHIFT_RIGHT_CLICK_COMMAND;
extern std::wstring SHIFT_MIDDLE_CLICK_COMMAND;
extern std::wstring CONTROL_MIDDLE_CLICK_COMMAND;
extern std::wstring COMMAND_MIDDLE_CLICK_COMMAND;
extern std::wstring ALT_MIDDLE_CLICK_COMMAND;
extern std::wstring CONTROL_RIGHT_CLICK_COMMAND;
extern std::wstring ALT_CLICK_COMMAND;
extern std::wstring ALT_RIGHT_CLICK_COMMAND;
extern Path local_database_file_path;
extern Path global_database_file_path;
extern Path downloaded_papers_path;
extern std::map<std::wstring, std::wstring> ADDITIONAL_COMMANDS;
extern std::map<std::wstring, std::wstring> ADDITIONAL_MACROS;
extern bool HIGHLIGHT_MIDDLE_CLICK;
extern std::wstring STARTUP_COMMANDS;
extern float CUSTOM_BACKGROUND_COLOR[3];
extern float CUSTOM_TEXT_COLOR[3];
extern float HYPERDRIVE_SPEED_FACTOR;
extern float SMOOTH_SCROLL_SPEED;
extern float SMOOTH_SCROLL_DRAG;
extern bool SUPER_FAST_SEARCH;
extern bool INCREMENTAL_SEARCH;
extern bool CASE_SENSITIVE_SEARCH;
extern bool SMARTCASE_SEARCH;
extern bool SHOULD_HIGHLIGHT_LINKS;
extern float SCROLL_VIEW_SENSITIVITY;
extern std::wstring STATUS_BAR_FORMAT;
extern std::wstring RIGHT_STATUS_BAR_FORMAT;
extern bool INVERTED_HORIZONTAL_SCROLLING;
extern bool TOC_JUMP_ALIGN_TOP;
extern bool AUTOCENTER_VISUAL_SCROLL;
// extern bool ALPHABETIC_LINK_TAGS;
extern bool NUMERIC_TAGS;
extern bool VIMTEX_WSL_FIX;
extern float RULER_AUTO_MOVE_SENSITIVITY;
extern float TTS_RATE;
extern std::wstring HOLD_MIDDLE_CLICK_COMMAND;
extern float FREETEXT_BOOKMARK_FONT_SIZE;
extern bool USE_RULER_TO_HIGHLIGHT_SYNCTEX_LINE;
extern std::wstring VOLUME_DOWN_COMMAND;
extern std::wstring VOLUME_UP_COMMAND;
extern int DOCUMENTATION_FONT_SIZE;
extern ScratchPad global_scratchpad;
extern int NUM_CACHED_PAGES;
extern bool IGNORE_SCROLL_EVENTS;
extern bool DONT_FOCUS_IF_SYNCTEX_RECT_IS_VISIBLE;
extern bool USE_SYSTEM_THEME;
extern bool USE_CUSTOM_COLOR_FOR_DARK_SYSTEM_THEME;
extern bool ALLOW_MAIN_VIEW_SCROLL_WHILE_IN_OVERVIEW;
extern bool DEBUG;
extern bool AUTO_LOGIN_ON_STARTUP;
extern bool FANCY_UI_MENUS;

extern float AUTO_BOOKMARK_VERTICAL_MARGIN;
extern float AUTO_BOOKMARK_HORIZONTAL_MARGIN;

extern int COLOR_MODE;

extern std::wstring TTS_VOICE;
extern std::wstring PAPERS_FOLDER_PATH;
extern bool SHOW_RIGHT_CLICK_CONTEXT_MENU;
extern std::wstring CONTEXT_MENU_ITEMS;
extern std::wstring CONTEXT_MENU_ITEMS_FOR_SELECTED_TEXT;
extern std::wstring CONTEXT_MENU_ITEMS_FOR_LINKS;
extern std::wstring CONTEXT_MENU_ITEMS_FOR_HIGHLIGHTS;
extern std::wstring CONTEXT_MENU_ITEMS_FOR_BOOKMARKS;
extern std::wstring CONTEXT_MENU_ITEMS_FOR_OVERVIEW;
extern bool RIGHT_CLICK_CONTEXT_MENU;
extern float SMOOTH_MOVE_MAX_VELOCITY;
extern int DOCUMENT_LOCATION_MISMATCH_STRATEGY;
extern int NUM_PAGE_COLUMNS;
extern Path python_api_base_path;

extern float MENU_SCREEN_WDITH_RATIO;
extern float MENU_SCREEN_HEIGHT_RATIO;

extern bool AUTOMATICALLY_UPDATE_CHECKSUM_WHEN_DOCUMENT_IS_CHANGED;
extern bool SAVE_EXTERNALLY_EDITED_TEXT_ON_FOCUS;
extern std::wstring EXTERNAL_TEXT_EDITOR_COMMAND;
extern bool FORCE_CUSTOM_LINE_ALGORITHM;

extern std::wstring RIGHT_CLICK_COMMAND;
extern std::wstring MIDDLE_CLICK_COMMAND;
extern int MAX_TAB_COUNT;
extern std::wstring RESIZE_COMMAND;
extern std::wstring BACK_RECT_TAP_COMMAND;
extern std::wstring BACK_RECT_HOLD_COMMAND;
extern std::wstring FORWARD_RECT_TAP_COMMAND;
extern std::wstring FORWARD_RECT_HOLD_COMMAND;
extern std::wstring TOP_CENTER_TAP_COMMAND;
extern std::wstring TOP_CENTER_HOLD_COMMAND;
extern std::wstring VISUAL_MARK_NEXT_TAP_COMMAND;
extern std::wstring VISUAL_MARK_NEXT_HOLD_COMMAND;
extern std::wstring VISUAL_MARK_PREV_TAP_COMMAND;
extern std::wstring VISUAL_MARK_PREV_HOLD_COMMAND;
extern bool AUTOMATICALLY_DOWNLOAD_MATCHING_PAPER_NAME;
extern std::wstring TABLET_PEN_CLICK_COMMAND;
extern std::wstring TABLET_PEN_DOUBLE_CLICK_COMMAND;
extern bool ALLOW_HORIZONTAL_DRAG_WHEN_DOCUMENT_IS_SMALL;
extern float PAGE_SPACE_X;
extern float PAGE_SPACE_Y;
extern Path sioyek_temp_text_path;

extern std::wstring MIDDLE_LEFT_RECT_TAP_COMMAND;
extern std::wstring MIDDLE_LEFT_RECT_HOLD_COMMAND;
extern std::wstring MIDDLE_RIGHT_RECT_TAP_COMMAND;
extern std::wstring MIDDLE_RIGHT_RECT_HOLD_COMMAND;

extern UIRect PORTRAIT_EDIT_PORTAL_UI_RECT;
extern UIRect LANDSCAPE_EDIT_PORTAL_UI_RECT;

extern UIRect PORTRAIT_BACK_UI_RECT;
extern UIRect PORTRAIT_FORWARD_UI_RECT;
extern UIRect LANDSCAPE_BACK_UI_RECT;
extern UIRect LANDSCAPE_FORWARD_UI_RECT;
extern UIRect PORTRAIT_VISUAL_MARK_PREV;
extern UIRect PORTRAIT_VISUAL_MARK_NEXT;
extern UIRect LANDSCAPE_VISUAL_MARK_PREV;
extern UIRect LANDSCAPE_VISUAL_MARK_NEXT;
extern UIRect PORTRAIT_MIDDLE_LEFT_UI_RECT;
extern UIRect PORTRAIT_MIDDLE_RIGHT_UI_RECT;
extern UIRect LANDSCAPE_MIDDLE_LEFT_UI_RECT;
extern UIRect LANDSCAPE_MIDDLE_RIGHT_UI_RECT;

extern bool PAPER_DOWNLOAD_CREATE_PORTAL;
extern bool ALIGN_LINK_DEST_TO_TOP;
extern bool USE_KEYBOARD_POINT_SELECTION;

extern bool SCROLLBAR;
extern bool STATUSBAR;
extern bool STATUSBAR_HANDLES_WHEEL_EVENTS;
extern bool AUTOMATICALLY_INDEX_DOCUMENT_FOR_FULLTEXT_SEARCH;
extern bool AUTOMATICALLY_UPLOAD_PORTAL_DESTINATION_FOR_SYNCED_DOCUMENTS;
extern bool SNAP_DRAGGING;
extern bool TOUCH_MODE;
extern std::unordered_map<std::wstring, std::wstring> STATUS_BAR_COMMANDS;
extern std::unordered_map<std::wstring, std::wstring> STATUS_BAR_WHEEL_UP_COMMANDS;
extern std::unordered_map<std::wstring, std::wstring> STATUS_BAR_WHEEL_DOWN_COMMANDS;

const int MAX_SCROLLBAR = 10000;

extern int RELOAD_INTERVAL_MILISECONDS;

const unsigned int INTERVAL_TIME = 200;

#ifdef Q_OS_MACOS
extern float MACOS_TITLEBAR_COLOR[3];
extern float MACOS_DARK_TITLEBAR_COLOR[3];
extern bool MACOS_HIDE_TITLEBAR;
#endif

MainWidget* get_window_with_window_id(int window_id) {
    for (auto window : windows) {
        if (window->get_window_id() == window_id) return window;
    }
    return nullptr;
}

bool MainWidget::main_document_view_has_document()
{
    return (main_document_view != nullptr) && (doc() != nullptr);
}


template<typename T>
void set_filtered_select_menu(
    MainWidget* main_widget,
     bool fuzzy,
     bool multiline,
     std::vector<std::vector<std::wstring>> columns,
    std::vector<T> values,
    int selected_index,
    std::function<void(T*)> on_select,
    std::function<void(T*)> on_delete,
    std::function<void(T*)> on_edit = nullptr
) {
    if (columns.size() > 1) {

        if (TOUCH_MODE) {

            // we will set the parent of model to be the widget in the constructor,
            // and it will delete the model when it is freed, so this is not a memory leak
            QStandardItemModel* model = create_table_model(columns);

            auto widget = new TouchFilteredSelectWidget<T>(
                fuzzy, model, values, selected_index,
                [&, main_widget, on_select = std::move(on_select)](T* val) {
                    if (val) {
                        on_select(val);
                    }
                    main_widget->pop_current_widget();
                },
                [&, on_delete = std::move(on_delete)](T* val) {
                    if (val) {
                        on_delete(val);
                    }
                }, main_widget);

            widget->set_filter_column_index(-1);
            main_widget->set_current_widget(widget);
        }
        else {
            auto w = new FilteredSelectTableWindowClass<T>(
                fuzzy,
                multiline,
                columns,
                values,
                selected_index,
                [on_select = std::move(on_select)](T* val) {
                    if (val) {
                        on_select(val);
                    }
                },
                main_widget,
                    [on_delete = std::move(on_delete)](T* val) {
                    if (val && on_delete) {
                        on_delete(val);
                    }
                });
            w->set_filter_column_index(-1);
            if (on_edit) {
                w->set_on_edit_function(on_edit);
            }
            main_widget->set_current_widget(w);
        }
    }
    else {

        if (TOUCH_MODE) {
            // when will this be released?
            auto widget = new TouchFilteredSelectWidget<T>(
                fuzzy, columns[0], values, selected_index,
                [&, main_widget, on_select = std::move(on_select)](T* val) {
                    if (val) {
                        on_select(val);
                    }
                    main_widget->pop_current_widget();
                },
                [&, on_delete = std::move(on_delete)](T* val) {
                    if (val) {
                        on_delete(val);
                    }
                }, main_widget);
            main_widget->set_current_widget(widget);
        }
        else {

            std::vector<std::wstring> empty_column;
            main_widget->set_current_widget(new FilteredSelectWindowClass<T>(
                fuzzy,
                columns.size() > 0 ? columns[0] : empty_column,
                values,
                [on_select = std::move(on_select)](T* val) {
                    if (val) {
                        on_select(val);
                    }
                },
                main_widget,
                    [on_delete = std::move(on_delete)](T* val) {
                    if (val) {
                        on_delete(val);
                    }
                },
                    selected_index));
        }
    }
}

bool MainWidget::is_current_document_available_on_server() {
    return sioyek_network_manager->is_document_available_on_server(doc());
}

void MainWidget::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
#ifdef SIOYEK_IOS
    opengl_widget->resize(resize_event->size());
#endif

    main_window_width = size().width();
    main_window_height = size().height();

    if (scratchpad) {
        scratchpad->on_view_size_change(
            resize_event->size().width(),
            resize_event->size().height()
        );
        invalidate_render();
    }
    if (main_document_view->get_is_auto_resize_mode()) {
        // when a document is newly opened, we fit it to the page width
        // and make sure the top of the page is aligned with the top of the window
        main_document_view->fit_to_page_width();
        main_document_view->set_offset_y(
            main_window_height / 2 / main_document_view->get_zoom_level()
        );
    }

    int status_bar_height = get_status_bar_height();

    if (text_command_line_edit_container != nullptr) {
        text_command_line_edit_container->move(0, 0);
        text_command_line_edit_container->resize(main_window_width, status_bar_height);
    }

    if (status_label != nullptr) {
        status_label->move(0, main_window_height - status_bar_height);
        status_label->resize(main_window_width, status_bar_height);
        if (should_show_status_label()) {
            status_label->show();
        }
    }

    if ((doc() != nullptr) && (main_document_view->get_zoom_level() == 0)) {
        main_document_view->fit_to_page_width();
        update_current_history_index();
    }

    if ((current_widget_stack.size() > 0)) {
        for (auto w : current_widget_stack) {
            QCoreApplication::postEvent(w, resize_event->clone());
        }
    }

    if (text_selection_buttons_) {
        QCoreApplication::postEvent(get_text_selection_buttons(), resize_event->clone());
    }
    if (search_buttons_) {
        QCoreApplication::postEvent(get_search_buttons(), resize_event->clone());
    }
    if (highlight_buttons_) {
        QCoreApplication::postEvent(get_highlight_buttons(), resize_event->clone());
    }
    if (draw_controls_) {
        QCoreApplication::postEvent(get_draw_controls(), resize_event->clone());
    }
    if (RESIZE_COMMAND.size() > 0) {
        execute_macro_if_enabled(RESIZE_COMMAND);
    }

}

bool MainWidget::handle_visible_object_cursor_update(AbsoluteDocumentPos abs_mpos){
    std::optional<VisibleObjectIndex>& selected_object_index = main_document_view->selected_object_index;
    if (selected_object_index.has_value() && (selected_object_index->object_type == VisibleObjectType::Bookmark)) {
        //BookMark& selected_bookmark = doc()->get_bookmarks()[selected_object_index->index];
        BookMark* selected_bookmark = doc()->get_bookmark_with_uuid(selected_object_index->uuid);
        if (selected_bookmark && selected_bookmark->is_freetext()) {
            set_mouse_cursor_for_side_resize(selected_bookmark->get_resize_side_containing_point(abs_mpos));
            return true;
        }
    }
    else if (main_document_view->is_pinned_portal_selected()) {
        //Portal& selected_portal = doc()->get_portals()[selected_object_index->index];
        Portal* selected_portal = doc()->get_portal_with_uuid(selected_object_index->uuid);
        if (selected_portal) {
            set_mouse_cursor_for_side_resize(selected_portal->get_resize_side_containing_point(abs_mpos));
        }
        return true;
    }
    return false;
}

void MainWidget::mouseMoveEvent(QMouseEvent* mouse_event) {
    if (is_pinching){
        // no need to handle move events when a pinch to zoom is in progress
        return;
    }

    if (is_rotated()) {
        // we don't handle mouse events while document is rotated because proper
        // handling would increase the code complexity too much to be worth it
        return;
    }

    if (should_draw(false) && main_document_view->is_drawing) {
        main_document_view->handle_drawing_move(mouse_event->pos(), -1.0f);
        validate_render();
        return;
    }

    WindowPos mpos(mouse_event->pos());
    AbsoluteDocumentPos abs_mpos = main_document_view->get_window_abspos(mpos);
    NormalizedWindowPos normal_mpos = mpos.to_window_normalized(main_document_view);

    // we start moving visible objects when the mouse has moved for some distance after clicking
    // so we can distinguish between single-click selects and mouse drags
    if (main_document_view->visible_object_move_data && !main_document_view->visible_object_move_data->is_moving) {
        if ((main_document_view->visible_object_move_data->initial_mouse_position - abs_mpos).manhattan() > 5) {
            main_document_view->visible_object_move_data->is_moving = true;
        }
    }

    if (main_document_view->freehand_drawing_move_data) {
        // update temp drawings of document view
        main_document_view->moving_drawings.clear();
        main_document_view->moving_pixmaps.clear();
        main_document_view->move_selected_drawings(
            abs_mpos,
            main_document_view->moving_drawings,
            main_document_view->moving_pixmaps
        );
        validate_render();
        return;
    }

    if (rect_select_mode) {
        if (rect_select_begin.has_value()) {
            rect_select_end = abs_mpos;
            AbsoluteRect selected_rect(
                rect_select_begin.value(),
                rect_select_end.value()
            );
            main_document_view->set_selected_rectangle(selected_rect);

            validate_render();
        }
        return;
    }


    if (TOUCH_MODE) {
        if (selection_begin_indicator) {
            selection_begin_indicator->update_pos();
            selection_end_indicator->update_pos();
        }
        update_highlight_buttons_position();
        if (is_pressed) {
            update_position_buffer();
        }
        if (was_last_mouse_down_in_ruler_next_rect || was_last_mouse_down_in_ruler_prev_rect) {
            WindowPos current_window_pos(mouse_event->pos());
            int distance = current_window_pos.manhattan(ruler_moving_last_window_pos);
            ruler_moving_last_window_pos = current_window_pos;
            ruler_moving_distance_traveled += distance;
            int num_next = ruler_moving_distance_traveled /
                static_cast<int>(std::max(RULER_AUTO_MOVE_SENSITIVITY, 1.0f));
            if (num_next > 0) {
                ruler_moving_distance_traveled = 0;
            }

            for (int i = 0; i < num_next; i++) {

                if (was_last_mouse_down_in_ruler_next_rect) {
                    main_document_view->move_visual_mark_next();
                }
                else {
                    main_document_view->move_visual_mark_prev();
                }

                invalidate_render();
            }
            return;

        }
    }

    if (main_document_view->visible_object_move_data) {
        main_document_view->visible_object_move_data->handle_move(this);
        validate_render();
    }

    // if the mouse has moved too much when pressing middle mouse button, we assume that
    // the user wants to drag instead of smart jump
    if (QGuiApplication::mouseButtons() & Qt::MouseButton::MiddleButton) {

        if (main_document_view->is_window_point_in_overview(normal_mpos)) {
            if (!overview_touch_move_data.has_value()) {
                OverviewTouchMoveData touch_move_data;
                touch_move_data.original_mouse_normalized_pos = normal_mpos;
                touch_move_data.overview_original_pos_absolute = main_document_view->get_overview_page()->get_absolute_pos();
                overview_touch_move_data = touch_move_data;
            }
        }
        else {
            if (!main_document_view->visible_object_move_data.has_value()) {
                if ((mpos.manhattan(last_mouse_down_window_pos)) > 50) {
                    start_dragging();
                }
            }
        }
    }


    if (main_document_view->handle_visible_object_resize_mouse_move(abs_mpos)){
        validate_render();
        return;
    }
    if (main_document_view->handle_visible_object_scroll_mouse_move(abs_mpos)){
        validate_render();
        return;
    }

    if (overview_resize_data) {
        // if we are resizing overview page, set the selected side of the overview window to the mosue position
        fvec2 offset_diff = normal_mpos - overview_resize_data->original_normal_mouse_pos;
        main_document_view->set_overview_side_pos(
            overview_resize_data->side_index,
            overview_resize_data->original_rect,
            offset_diff);
        validate_render();
        return;
    }

    if (overview_move_data) {
        fvec2 offset_diff = normal_mpos - overview_move_data->original_normal_mouse_pos;
        fvec2 new_offsets = overview_move_data.value().original_offsets + offset_diff;
        main_document_view->set_overview_offsets(new_offsets);
        validate_render();
        return;
    }

    if (overview_touch_move_data && main_document_view->get_overview_page()) {
        // in touch mode, instead of moving the overview itself, we move the document inside the overview
        NormalizedWindowPos current_pos = normal_mpos;
        AbsoluteDocumentPos current_overview_absolute_pos = main_document_view->window_pos_to_overview_pos(current_pos).to_absolute(doc());
        /* current_pos = overview_touch_move_data->original_mouse_normalized_pos; */
        AbsoluteDocumentPos original_absolute_pos = main_document_view->window_pos_to_overview_pos(
                overview_touch_move_data->original_mouse_normalized_pos).to_absolute(doc());

        float absdiff_y = current_overview_absolute_pos.y - original_absolute_pos.y;
        float absdiff_x = current_overview_absolute_pos.x - original_absolute_pos.x;

        float new_absolute_y = overview_touch_move_data->overview_original_pos_absolute.y - absdiff_y;
        float new_absolute_x = overview_touch_move_data->overview_original_pos_absolute.x + absdiff_x;

        OverviewState new_overview_state = main_document_view->get_overview_page().value();
        new_overview_state.absolute_offset_y = new_absolute_y;
        new_overview_state.absolute_offset_x = new_absolute_x;

        set_overview_page(new_overview_state, false);
        validate_render();

    }

    std::optional<PdfLink> link = {};
    if (!is_scratchpad_mode()){
        if (main_document_view->is_window_point_in_overview(normal_mpos)) {
            link = doc()->get_link_in_pos(main_document_view->window_pos_to_overview_pos(normal_mpos));
            if (link || dv()->get_overview_download_rect().contains(mpos)) {
                setCursor(Qt::PointingHandCursor);
            }
            else {
                OverviewSide border_side;
                if (dv()->is_window_point_in_overview_border(normal_mpos, &border_side)) {
                    set_mouse_cursor_for_side_resize(border_side);
                }
                else {
                    setCursor(Qt::ArrowCursor);
                }
            }

            return;
        }
        else {
            if (handle_visible_object_cursor_update(abs_mpos)) return;
        }

        if (main_document_view && (link = main_document_view->get_link_in_pos(mpos))) {
            // show hand cursor when hovering over links
            setCursor(Qt::PointingHandCursor);

            // if hover_overview config is set, we show an overview of links while hovering over them
            if (HOVER_OVERVIEW) {
                dv()->set_overview_link(link.value());
                invalidate_render();
            }
        }
        else {
            setCursor(Qt::ArrowCursor);
            if (HOVER_OVERVIEW) {
                set_overview_page({}, false);
                invalidate_render();
                //            invalidate_render();
            }
        }
    }

    if (should_drag()) {
        fvec2 diff_doc = (mpos - last_mouse_down_window_pos) / dv()->get_zoom_level();
        diff_doc[1] = -diff_doc[1]; // higher document y positions correspond to lower window positions

        if (horizontal_scroll_locked) {
            diff_doc.values[0] = 0;
        }
        if (is_dragging_snapped) {
            // initially we assume the user is trying to move vertically and therefore
            // dragging is snapped to vertical direction, until the users moves sufficiently
            // in the horizontal direction
            float abs_x = std::abs(diff_doc.values[0]);
            float abs_y = std::abs(diff_doc.values[1]);

            if (((abs_x + abs_y > 20) && (abs_x / abs_y > 0.6f)) || ((abs_x + abs_y > 10 && (abs_x / abs_y > 2)))) {
                last_mouse_down_document_virtual_offset = dv()->get_virtual_offset();
                last_mouse_down_window_pos = mpos;
                diff_doc.values[0] = 0;
                diff_doc.values[1] = 0;
                is_dragging_snapped = false;
            }
            else {
                diff_doc.values[0] = 0;
            }
        }

        if (!ALLOW_HORIZONTAL_DRAG_WHEN_DOCUMENT_IS_SMALL) {
            set_drag_value_on_small_documents(diff_doc);
        }
        else{
            auto new_pos = last_mouse_down_document_virtual_offset + diff_doc;
            dv()->set_virtual_pos(new_pos, true);
        }


        validate_render();
    }

    if (main_document_view->is_selecting) {

        // When selecting, we occasionally update selected text
        //todo: maybe have a timer event that handles this periodically
        int msecs_since_last_text_select = last_text_select_time.msecsTo(QTime::currentTime());
        if (msecs_since_last_text_select > 16 || msecs_since_last_text_select < 0) {

            dv()->selection_begin = last_mouse_down;
            dv()->selection_end = abs_mpos;
            //fz_point selection_begin = { last_mouse_down.x(), last_mouse_down.y()};
            //fz_point selection_end = { document_x, document_y };

            main_document_view->get_text_selection(dv()->selection_begin,
                dv()->selection_end,
                main_document_view->is_word_selecting,
                main_document_view->selected_character_rects,
                dv()->selected_text);
            main_document_view->selected_text_is_dirty = false;

            validate_render();
            last_text_select_time = QTime::currentTime();
        }
    }
}

void MainWidget::set_drag_value_on_small_documents(fvec2& diff_doc){
    float current_page_width = doc()->get_page_width(get_current_page_number());
    float current_offset = dv()->get_offset_x();

    float view_hwidth_document = dv()->get_view_width() / dv()->get_zoom_level() / 2;

    float min_valid_new_offset_x = std::min(-(current_drag_max_annotation_x - view_hwidth_document), 0.0f);
    float max_valid_new_offset_x = std::max(-(current_drag_min_annotation_x + view_hwidth_document), 0.0f);

    if (dv()->is_two_page_mode()) {
        current_page_width += (current_page_width + PAGE_SPACE_X) * (NUM_PAGE_COLUMNS - 1);
    }

    if ((current_page_width > 0) && ((dv()->get_zoom_level() * current_page_width) < width())) {
        bool ignore = false;
        if (diff_doc[0] < 0 && current_offset < min_valid_new_offset_x){
            diff_doc[0] = 0;
            ignore = true;
        }
        else if (diff_doc[0] > 0 && current_offset > max_valid_new_offset_x){
            diff_doc[0] = 0;
            ignore = true;
        }

        auto new_pos = last_mouse_down_document_virtual_offset + diff_doc;

        if (current_offset <= max_valid_new_offset_x && new_pos.x >= max_valid_new_offset_x){
            new_pos.x = max_valid_new_offset_x;
        }
        else if (current_offset >= min_valid_new_offset_x && new_pos.x <= min_valid_new_offset_x){
            new_pos.x = min_valid_new_offset_x;
        }
        if (!ignore){
            dv()->set_virtual_pos(new_pos, true);
        }
    }
    else {
        auto new_pos = last_mouse_down_document_virtual_offset + diff_doc;
        dv()->set_virtual_pos(new_pos, true);
    }



}

void MainWidget::persist(bool persist_drawings) {
    main_document_view->persist(persist_drawings);

    // write the address of the current document in a file so that the next time
    // we launch the application, we open this document
    if (main_document_view->get_document()) {
        std::ofstream last_path_file(last_opened_file_address_path.get_path_utf8());

        //std::string encoded_file_name_str = utf8_encode(main_document_view->get_document()->get_path());
        std::string encoded_file_name_str = utf8_encode(get_current_tabs_file_names());
        last_path_file << encoded_file_name_str.c_str() << std::endl;
        last_path_file.close();
    }
}
void MainWidget::closeEvent(QCloseEvent* close_event) {
    handle_close_event();
}

MainWidget::MainWidget(MainWidget* other) :
    MainWidget(
        other->mupdf_context,
        other->pdf_renderer,
        other->db_manager,
        other->document_manager,
        other->config_manager,
        other->command_manager,
        other->input_handler,
        other->checksummer,
        other->sioyek_network_manager,
        other->background_task_manager,
        other->background_bookmark_renderer,
        other->should_quit) {

}

MainWidget::MainWidget(fz_context* mupdf_context,
    PdfRenderer* pdf_renderer,
    DatabaseManager* db_manager,
    DocumentManager* document_manager,
    ConfigManager* config_manager,
    CommandManager* command_manager,
    InputHandler* input_handler,
    CachedChecksummer* checksummer,
    SioyekNetworkManager* sioyek_network_manager_,
    BackgroundTaskManager* task_manager,
    BackgroundBookmarkRenderer* bookmark_renderer,
    bool* should_quit_ptr,
    QWidget* parent) :
#ifdef SIOYEK_MOBILE
    QQuickWidget(parent),
#else
    QMainWindow(parent),
#endif
    mupdf_context(mupdf_context),
    pdf_renderer(pdf_renderer),
    db_manager(db_manager),
    document_manager(document_manager),
    config_manager(config_manager),
    input_handler(input_handler),
    checksummer(checksummer),
    sioyek_network_manager(sioyek_network_manager_),
    should_quit(should_quit_ptr),
    background_task_manager(task_manager),
    background_bookmark_renderer(bookmark_renderer),
    command_manager(command_manager)
{
    //main_widget->quickWindow()->setGraphicsApi(QSGRendererInterface::OpenGL);
    //quickWindow()->setGraphicsApi(QSGRendererInterface::OpenGL);
    window_id = next_window_id;
    next_window_id++;


    setMouseTracking(true);
    setAcceptDrops(true);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_AcceptTouchEvents);

#ifdef SIOYEK_MOBILE
    setWindowFlag(Qt::MaximizeUsingFullscreenGeometryHint, true);
#endif


    central_widget = new QWidget(this);
    central_widget->setMouseTracking(true);

    inverse_search_command = INVERSE_SEARCH_COMMAND;
    //pdf_renderer = new PdfRenderer(4, should_quit_ptr, mupdf_context);
    //pdf_renderer->set_num_cached_pages(NUM_CACHED_PAGES);
    //pdf_renderer->start_threads();


    //scratchpad = new ScratchPad();
    scratchpad = &global_scratchpad;

    main_document_view = new DocumentView(db_manager, document_manager, checksummer);
    opengl_widget = new PdfViewOpenGLWidget(main_document_view, pdf_renderer, document_manager, false, this);

    QFont label_font = QFont(get_status_font_face_name());
    label_font.setStyleHint(QFont::TypeWriter);

    status_label = new QWidget(this);
    status_label->setFont(label_font);
    status_label->setStyleSheet(get_status_stylesheet());

    status_label_left = new StatusLabelLineEdit();
    status_label_left->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    status_label_left->setStyleSheet(get_status_stylesheet());
    status_label_left->setFont(label_font);


    QHBoxLayout* right_status_container_layout = new QHBoxLayout();

    status_label_right = new QLabel();
    status_label_right->setStyleSheet(get_status_stylesheet());
    status_label_right->setFont(label_font);

    server_actions_button = new QPushButton(sioyek_network_manager->get_login_status_string(doc()));
    server_actions_button->setCursor(Qt::PointingHandCursor);

    resume_to_server_position_button = new QPushButton("RESUME");
    resume_to_server_position_button->setToolTip("Resume to server location");
    resume_to_server_position_button->setCursor(Qt::PointingHandCursor);
    resume_to_server_position_button->setStyleSheet(get_status_button_stylesheet());

    // we don't want the statusbar buttons to steal the input focus when clicked
    resume_to_server_position_button->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    server_actions_button->setFocusPolicy(Qt::FocusPolicy::NoFocus);


    right_status_container_layout->addWidget(resume_to_server_position_button);
    right_status_container_layout->addWidget(status_label_right);
    right_status_container_layout->addWidget(server_actions_button);

    resume_to_server_position_button->hide();

    QHBoxLayout* status_label_layout = new QHBoxLayout();
    status_label_layout->setContentsMargins(0, 0, 0, 0);

    status_label_layout->addWidget(status_label_left, 1);
    status_label_layout->addLayout(right_status_container_layout);
    //status_label_layout->addWidget(status_label_right);

    status_label->setLayout(status_label_layout);

    opengl_widget->stackUnder(status_label);

    // automatically open the helper window in second monitor
    int num_screens = QGuiApplication::screens().size();

    text_command_line_edit_container = new QWidget(this);
    text_command_line_edit_container->setStyleSheet(get_status_stylesheet());

    QHBoxLayout* text_command_line_edit_container_layout = new QHBoxLayout();

    text_command_line_edit_label = new QLabel(this);
    text_command_line_edit = new MyLineEdit(this);

    text_command_line_edit_label->setFont(label_font);
    text_command_line_edit->setFont(label_font);

    text_command_line_edit_label->setStyleSheet(get_status_stylesheet());
    text_command_line_edit->setStyleSheet(get_status_stylesheet());

    text_command_line_edit_container_layout->addWidget(text_command_line_edit_label);
    text_command_line_edit_container_layout->addWidget(text_command_line_edit);
    text_command_line_edit_container_layout->setContentsMargins(10, 0, 10, 0);

    text_command_line_edit_container->setLayout(text_command_line_edit_container_layout);
    text_command_line_edit_container->hide();

    //QObject::connect(dynamic_cast<MyLineEdit*>(text_command_line_edit), &MyLineEdit::next_suggestion, this, &MainWidget::on_next_text_suggestion);
    //QObject::connect(dynamic_cast<MyLineEdit*>(text_command_line_edit), &MyLineEdit::prev_suggestion, this, &MainWidget::on_prev_text_suggestion);

    on_command_done = [&](std::string command_name, std::string query_text) {
        if (query_text.size() > 0 && (query_text.back() == '?' || query_text[0] == '?')) {
            show_command_documentation(QString::fromStdString(command_name));
        }
        else {
            bool is_numeric = false;
            int page_number = QString::fromStdString(command_name).toInt(&is_numeric);
            if (is_numeric) {
                if (main_document_view) {
                    main_document_view->goto_page(page_number - 1);
                    invalidate_render();
                }
            }
            else {
                std::unique_ptr<Command> command = this->command_manager->get_command_with_name(this, command_name);
                this->command_manager->update_command_last_use(command_name);
                handle_command_types(std::move(command), 0);
            }
        }
    };

    // some commands need to be notified when the text in text input is changed
    // for example when editing bookmarks we update the bookmark in real time as
    // the bookmark text is being edited
    QObject::connect(text_command_line_edit, &QLineEdit::textEdited, [&](const QString& txt) {
        handle_command_text_change(txt);
        });

    // when pdf renderer's background threads finish rendering a page or find a new search result
    // we need to update the ui
    QObject::connect(pdf_renderer, &PdfRenderer::render_advance, this, &MainWidget::invalidate_render);
    QObject::connect(pdf_renderer, &PdfRenderer::search_advance, this, &MainWidget::invalidate_ui);

    QObject::connect(pdf_renderer->get_bookmark_renderer(), &BackgroundBookmarkRenderer::bookmark_rendered, this, &MainWidget::invalidate_render);

    QObject::connect(resume_to_server_position_button, &QPushButton::clicked, [&]() {
        handle_resume_to_server_location();
        });

    // when screen is rotated, we may be below the minimum zoom level for the new orientation
    // so we check for that here and adjust to screen width if that is the case
    QObject::connect(screen(), &QScreen::orientationChanged, [&](Qt::ScreenOrientation orientation){
            if (TOUCH_MODE && doc()){
                int current_page_number = main_document_view->get_current_page_number();
                float min_zoom_level = main_document_view->get_view_width() / doc()->get_page_width(current_page_number);
                if (main_document_view->get_zoom_level() < min_zoom_level){
                    main_document_view->fit_to_page_width();
                    invalidate_render();
                }
            }
            });

    dynamic_cast<StatusLabelLineEdit*>(status_label_left)->on_click = [&]() {

        if (TOUCH_MODE){
            // we don't want to handle clicks on the status label in touch mode
            return;
        }

        std::wstring status_part_name = get_status_part_name_under_cursor();
        if (status_part_name.size() > 0 && STATUS_BAR_COMMANDS.find(status_part_name) != STATUS_BAR_COMMANDS.end()) {
            execute_macro_if_enabled(STATUS_BAR_COMMANDS[status_part_name]);
        }
        };
    //QObject::connect(status_label_left, &QWidget::cursorPositionChanged, [&](int a, int b) {
    //    qDebug() << "something";
    //    });

    QObject::connect(server_actions_button, &QPushButton::clicked, [&]() {
        handle_server_actions_button_pressed();
        });

    QObject::connect(&external_command_edit_watcher, &QFileSystemWatcher::fileChanged, [&]() {

        // hack: this should not be necessary (after all, how could we be receiving fileChanged events
        // if external_command_edit_watcher.files().size() == 0?) but for some reason when editing files
        // using vim they somehow get unregistered from the file watcher
        if (external_command_edit_watcher.files().size() == 0) {
            QString path_qstring = QString::fromStdWString(sioyek_temp_text_path.get_path());
            external_command_edit_watcher.addPath(path_qstring);
        }

        if (text_command_line_edit->isVisible()) {
            is_external_file_edited = true;
            QFile file(QString::fromStdWString(sioyek_temp_text_path.get_path()));

            if (file.open(QIODeviceBase::ReadOnly)) {
                QString content = QString::fromUtf8(file.readAll());
                text_command_line_edit->setText(content);
                text_command_line_edit->textEdited(content);

            }

            file.close();
        }
        });
        
    // we check periodically to see if the ui needs updating
    // this is done so that thousands of search results only trigger
    // a few rerenders
    // todo: make interval time configurable
    validation_interval_timer = new QTimer(this);
    validation_interval_timer->setInterval(INTERVAL_TIME);

    network_timer = new QTimer(this);
    network_timer->setInterval(60000);

    connect(validation_interval_timer, &QTimer::timeout, [&]() {
        handle_validation_interval_timeout();
        });

    connect(network_timer, &QTimer::timeout, [&]() {
        handle_periodic_network_operations();
        });

    validation_interval_timer->start();
    network_timer->start();


    scroll_bar = new QScrollBar(this);
    QVBoxLayout* layout = new QVBoxLayout;
    QHBoxLayout* hlayout = new QHBoxLayout;


    hlayout->addWidget(opengl_widget);
    hlayout->addWidget(scroll_bar);

    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    opengl_widget->setAttribute(Qt::WA_TransparentForMouseEvents);
    layout->addLayout(hlayout);

#ifdef SIOYEK_ANDROID
     setLayout(layout);
#elif !defined(SIOYEK_IOS)
    central_widget->setLayout(layout);
    opengl_widget->stackUnder(status_label);
    setCentralWidget(central_widget);
#endif

    scroll_bar->setMinimum(0);
    scroll_bar->setMaximum(MAX_SCROLLBAR);

    scroll_bar->connect(scroll_bar, &QScrollBar::actionTriggered, [this](int action) {
        int value = scroll_bar->value();
        if (main_document_view_has_document()) {
            float offset = doc()->max_y_offset() * value / static_cast<float>(scroll_bar->maximum());
            main_document_view->set_offset_y(offset);
            validate_render();
        }
        });


    if (!SCROLLBAR) {
        scroll_bar->hide();
    }

    if (SHOULD_HIGHLIGHT_LINKS) {
        main_document_view->set_highlight_links(true, false);
    }

    grabGesture(Qt::TapAndHoldGesture, Qt::DontStartGestureOnChildren);
    grabGesture(Qt::PinchGesture, Qt::DontStartGestureOnChildren | Qt::ReceivePartialGestures);
    QObject::connect((QGuiApplication*)QGuiApplication::instance(), &QGuiApplication::applicationStateChanged, [&](Qt::ApplicationState state) {
        if ((state == Qt::ApplicationState::ApplicationSuspended) || (state == Qt::ApplicationState::ApplicationInactive)) {
#ifdef SIOYEK_MOBILE
            persist(true);
#endif
        }
#ifdef SIOYEK_ANDROID
        if (state == Qt::ApplicationState::ApplicationActive) {
            if (!pending_intents_checked) {
                pending_intents_checked = true;
                check_pending_intents("");
            }
        }
#endif

        });

#ifdef Q_OS_MACOS
    // only apply titlebar menu if the user has specifically changed it in settings
    ensure_titlebar_colors_match_color_mode();

    if (MACOS_HIDE_TITLEBAR) {
        hideWindowTitleBar(winId());
    }
    menu_bar = create_main_menu_bar();
    setMenuBar(menu_bar);
    menu_bar->stackUnder(text_command_line_edit_container);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    set_color_mode_to_system_theme();
#endif

    setFocus();
    // auto selector = new AndroidSelector(this);
    // selector->show();
    //set_current_widget(selector);

#ifdef SIOYEK_IOS
    QDesktopServices::setUrlHandler("file", this, "handle_ios_files");
    QObject::connect((QGuiApplication*)QGuiApplication::instance(), &QApplication::applicationStateChanged, [&](Qt::ApplicationState state){
        on_ios_application_state_changed(state);
    });
#endif
}

void MainWidget::handle_periodic_network_operations() {
    //if (sioyek_network_manager->should_sync_location()) {
    //    sync_current_file_location_to_servers();
    //}
}

MainWidget::~MainWidget() {
    background_task_manager->delete_tasks_with_parent(this);

    if (is_reading) {
        is_reading = false;
        get_tts()->stop();
    }
    validation_interval_timer->stop();
    network_timer->stop();
    remove_self_from_windows();

    if (windows.size() == 0) {
        *should_quit = true;
        pdf_renderer->join_threads();
    }

    if (tts) {
        delete tts;
    }

    if (sync_js_engine != nullptr) {
        sync_js_engine->collectGarbage();
        delete sync_js_engine;
    }

    for (int i = 0; i < available_async_engines.size(); i++) {
        available_async_engines[i]->collectGarbage();
        delete available_async_engines[i];
    }

    // todo: use a reference counting pointer for document so we can delete main_doc
    // and helper_doc in DocumentView's destructor, not here.
    // ideally this function should just become:
    //		delete main_document_view;
    //		if(helper_document_view != main_document_view) delete helper_document_view;
    if (main_document_view != nullptr) {
        //Document* main_doc = main_document_view->get_document();
        //if (main_doc != nullptr) delete main_doc;

        //Document* helper_doc = nullptr;
        //if (helper_document_view != nullptr) {
        //    helper_doc = helper_document_view->get_document();
        //}
        //if (helper_doc != nullptr && helper_doc != main_doc) delete helper_doc;
    }

    if (helper_document_view_ != nullptr && helper_document_view_ != main_document_view) {
        delete helper_document_view();
        helper_document_view_ = nullptr;
    }
}

void MainWidget::handle_validation_interval_timeout(){

        focus_on_high_quality_text_being_read();

        if (download_checksum_when_ready.has_value() && (sioyek_network_manager->status == ServerStatus::LoggedIn)){
            std::string checksum = download_checksum_when_ready.value();
            download_checksum_when_ready = {};
            download_and_open(checksum, 0);
        }

        remove_finished_shell_bookmarks();

        if (doc()) {
            if (doc()->super_fast_search_index_is_new()) {
                on_super_fast_search_index_computed();
            }
        }

        if (main_document_view->scheduled_portal_update) {
            update_link_with_opened_book_state(main_document_view->scheduled_portal_update->portal, main_document_view->scheduled_portal_update->state, true);
            main_document_view->scheduled_portal_update = {};

        }
        if (sioyek_network_manager->status != last_server_status) {
            last_server_status = sioyek_network_manager->status;
            invalidate_ui();
        }

        cleanup_expired_pending_portals();
        manage_last_document_checksum();
        if (TOUCH_MODE && selection_begin_indicator) {
            selection_begin_indicator->update_pos();
            selection_end_indicator->update_pos();
        }
        if (recently_updated_portal.has_value() &&
            (recently_updated_portal->last_modification_time.msecsTo(QDateTime::currentDateTime()) > 1000)) {
            on_portal_edited(recently_updated_portal->uuid);
            recently_updated_portal = {};
        }

        if (doc()) {
            if (doc()->get_should_reload_annotations()) {
                doc()->reload_annotations_on_new_checksum();
                validate_render();
            }
        }

        if (main_document_view->has_synctex_timed_out()) {
            is_render_invalidated = true;
        }

        if (is_render_invalidated) {
            validate_render();
        }

        if (doc() && (!doc()->get_valid())) {
            validate_render();
            doc()->validate();
        }

        if (doc() && dv() && dv()->get_overview_page()) {
            Document* overview_doc = dv()->get_overview_page()->doc;
            if (overview_doc && (!overview_doc->get_valid())) {
                validate_render();
                overview_doc->validate();
            }
        }

        else if (is_ui_invalidated) {
            validate_ui();
        }
        if (doc() && doc()->annotations_are_freshly_loaded) {
            doc()->annotations_are_freshly_loaded = false;
            sync_annotations_with_server();
        }

        if (QGuiApplication::mouseButtons() & Qt::MouseButton::MiddleButton) {
            if ((last_middle_down_time.msecsTo(QTime::currentTime()) > 200) && (!is_middle_click_being_used())) {
                if ((!middle_click_hold_command_already_executed) && (!main_document_view->is_moving_annotations())) {
                    execute_macro_if_enabled(HOLD_MIDDLE_CLICK_COMMAND);
                    middle_click_hold_command_already_executed = true;
                    invalidate_render();
                }
            }
        }

        // detect if the document file has changed and if so, reload the document
        if (main_document_view != nullptr) {
            Document* doc = nullptr;
            if ((doc = main_document_view->get_document()) != nullptr) {

                // Wait until a safe amount of time has passed since the last time the file was updated on the filesystem
                // this is because LaTeX software frequently puts PDF files in an invalid state while it is being made in
                // multiple passes.
                if ((doc->get_milies_since_last_document_update_time() > (doc->get_milies_since_last_edit_time())) &&
                    (doc->get_milies_since_last_edit_time() > RELOAD_INTERVAL_MILISECONDS)) {

                    if (is_doc_valid(this->mupdf_context, utf8_encode(doc->get_path()))) {
                        doc->reload();
                        this->pdf_renderer->clear_cache();
                        this->on_document_changed();
                        invalidate_render();
                    }
                }
            }
        }
}


std::wstring MainWidget::get_status_string(bool is_right) {

    if (is_right) {
        if (!right_status_string_generator.has_value()) {
            right_status_string_generator = std::move(compile_status_string(QString::fromStdWString(RIGHT_STATUS_BAR_FORMAT), this));
        }
        return (*right_status_string_generator)().first.toStdWString();
    }
    else {
        if (!left_status_string_generator.has_value()) {
            left_status_string_generator = std::move(compile_status_string(QString::fromStdWString(STATUS_BAR_FORMAT), this));
        }
        auto [str, ids] = (*left_status_string_generator)();
        last_status_string_ids = ids;
        return str.toStdWString();
    }
}

std::wstring MainWidget::get_title_string() {
    if (!titlebar_generator.has_value()) {
        titlebar_generator = std::move(compile_status_string(QString::fromStdWString(TITLEBAR_FORMAT), this));
    }
    auto [str, ids] = (*titlebar_generator)();
    return str.toStdWString();
}

void MainWidget::handle_escape() {

    if (pop_current_widget()) {
        return;
    }

    // add high escape priority to overview and search, if any of them are escaped, do not escape any further
    if (main_document_view) {
        bool should_return = false;
        if (main_document_view->get_overview_page()) {
            set_overview_page({}, false);
            should_return = true;
        }
        else if (main_document_view->get_is_searching(nullptr)) {
            if (pending_command_instance){
                pending_command_instance->on_cancel();
            }
            main_document_view->cancel_search();
            if (search_buttons_){
                get_search_buttons()->hide();
            }
            hide_command_line_edit();
            should_return = true;
        }
        if (should_return) {
            validate_render();
            setFocus();
            return;
        }
    }

    clear_selection_indicators();
    typing_location = {};
    if (pending_command_instance) {
        pending_command_instance->on_cancel();
    }
    hide_command_line_edit();
    text_suggestion_index = 0;
    main_document_view->set_pending_portal({});
    synchronize_pending_link();


    pending_command_instance = nullptr;
    main_document_view->clear_selected_object();
    //current_pending_command = {};


    bool was_line_select = dv()->line_select_mode;
    if (main_document_view) {
        main_document_view->handle_escape();
        opengl_widget->handle_escape();
    }

    if (main_document_view) {
        bool done_anything = false;
        if (main_document_view->get_overview_page()) {
            done_anything = true;
        }
        if (main_document_view->selected_character_rects.size() > 0) {
            done_anything = true;
        }

        set_overview_page({}, false);
        if (!was_line_select) {
            main_document_view->clear_selected_text();
        }

        //main_document_view->ruler
        if (!done_anything) {
            main_document_view->exit_ruler_mode();
            main_document_view->clear_underline();
        }
    }
    //if (opengl_widget) opengl_widget->set_should_draw_vertical_line(false);


    clear_selected_rect();

    validate_render();
    setFocus();
}

void MainWidget::keyPressEvent(QKeyEvent* kevent) {
    if (TOUCH_MODE) {
        if (kevent->key() == Qt::Key_Back) {
            if (current_widget_stack.size() > 0) {
                pop_current_widget();
            }
            else if (selection_begin_indicator) {
                clear_selection_indicators();
            }
        }
    }
    key_event(false, kevent, kevent->isAutoRepeat());
}

void MainWidget::keyReleaseEvent(QKeyEvent* kevent) {
    key_event(true, kevent, kevent->isAutoRepeat());
}

void MainWidget::validate_render() {

    if (smooth_scroll_mode) {
        if (main_document_view_has_document()) {
            float secs = static_cast<float>(-QTime::currentTime().msecsTo(last_speed_update_time)) / 1000.0f;

            if (secs > 0.1f) {
                secs = 0.0f;
            }

            if (!main_document_view->get_overview_page()) {
                float current_offset = main_document_view->get_offset_y();
                main_document_view->set_offset_y(current_offset + smooth_scroll_speed * secs);
            }
            else {
                OverviewState state = main_document_view->get_overview_page().value();
                //opengl_widget->get_overview_offsets(&overview_offset_x, &overview_offset_y);
                //opengl_widget->set_overview_offsets(overview_offset_x, overview_offset_y + smooth_scroll_speed * secs);
                state.absolute_offset_y += smooth_scroll_speed * secs;
                set_overview_page(state, false);
            }
            float accel = SMOOTH_SCROLL_DRAG;
            if (smooth_scroll_speed > 0) {
                smooth_scroll_speed -= secs * accel;
                if (smooth_scroll_speed < 0) smooth_scroll_speed = 0;

            }
            else {
                smooth_scroll_speed += secs * accel;
                if (smooth_scroll_speed > 0) smooth_scroll_speed = 0;
            }


            last_speed_update_time = QTime::currentTime();
        }
    }
    if (main_document_view->is_moving()) {
        auto current_time = QTime::currentTime();
        float secs = current_time.msecsTo(last_speed_update_time) / 1000.0f;

        main_document_view->velocity_tick(secs, horizontal_scroll_locked);

        if (!main_document_view->is_moving()) {
            validation_interval_timer->setInterval(INTERVAL_TIME);
        }
        last_speed_update_time = current_time;

    }
    if (!isVisible()) {
        return;
    }

    if (main_document_view){
        float status_label_height = status_label->isVisible() ? status_label->height() : 0;
        main_document_view->handle_validate_render(status_label_height);
    }


    bool should_update_portal = false;
    if (main_document_view && main_document_view->get_document() && is_helper_visible()) {
        std::optional<Portal> link = main_document_view->find_closest_portal();
        if (!(link == last_dispplayed_portal)) {
            should_update_portal = true;
            last_dispplayed_portal = link;
        }

        if (link) {
            helper_document_view()->goto_portal(&link.value());
        }
        else {
            helper_document_view()->set_null_document();
        }
    }
    validate_ui();
    update();
    if (opengl_widget != nullptr) {
        opengl_widget->update();
    }

    if (is_helper_visible() && (should_update_portal || helper_opengl_widget()->hasFocus() || helper_opengl_widget()->is_helper_waiting_for_render)) {
        helper_opengl_widget()->update();
    }

    is_render_invalidated = false;
    if (smooth_scroll_mode && (smooth_scroll_speed != 0)) {
        is_render_invalidated = true;
    }
    if (main_document_view->is_moving()) {
        is_render_invalidated = true;
        if (!hasFocus()) { // stop scrolling if the windows doesn't have focus
            set_fixed_velocity(0);
        }
    }
}

void MainWidget::validate_ui() {

    if (should_show_status_label() && !status_label->isVisible()) {
        status_label->show();
    }
    if ((!should_show_status_label()) && status_label->isVisible()) {
        status_label->hide();
    }

    status_label_left->setText(QString::fromStdWString(get_status_string(false)));
    status_label_right->setText(QString::fromStdWString(get_status_string(true)));

    if (TITLEBAR_FORMAT.size() > 0) {
        std::wstring new_titlebar_string = get_title_string();
        if (new_titlebar_string != last_titlebar_string) {
            last_titlebar_string = new_titlebar_string;
            setWindowTitle(QString::fromStdWString(new_titlebar_string));
        }
    }

    server_actions_button->setText(sioyek_network_manager->get_login_status_string(doc()));
    is_ui_invalidated = false;
}

void MainWidget::on_config_file_changed(ConfigManager* new_config) {
    QFont label_font = QFont(get_status_font_face_name());
    label_font.setStyleHint(QFont::TypeWriter);

    status_label->setStyleSheet(get_status_stylesheet());
    status_label->setFont(label_font);
    text_command_line_edit_container->setStyleSheet(get_status_stylesheet());
    text_command_line_edit->setFont(label_font);

    status_label_left->setStyleSheet(get_status_stylesheet());
    status_label_right->setStyleSheet(get_status_stylesheet());
    status_label_left->setFont(label_font);
    status_label_right->setFont(label_font);

    text_command_line_edit_label->setStyleSheet(get_status_stylesheet());
    text_command_line_edit->setStyleSheet(get_status_stylesheet());
    //status_label->setStyleSheet(get_status_stylesheet());

    int status_bar_height = get_status_bar_height();
    status_label->move(0, main_window_height - status_bar_height);
    status_label->resize(size().width(), status_bar_height);

    if (current_widget_stack.size() > 0) {
        BaseSelectorWidget* selector_widget = dynamic_cast<BaseSelectorWidget*>(current_widget_stack.back());
        if (selector_widget) {
            selector_widget->on_config_file_changed();
        }
    }

    //text_command_line_edit_container->setStyleSheet("background-color: black; color: white; border: none;");
}

void MainWidget::toggle_mouse_drag_mode() {
    this->mouse_drag_mode = !this->mouse_drag_mode;
}

void MainWidget::do_synctex_forward_search(const Path& pdf_file_path, const Path& latex_file_path, int line, int column) {
#ifndef SIOYEK_MOBILE

    std::wstring latex_file_path_with_redundant_dot = add_redundant_dot_to_path(latex_file_path.get_path());

    std::string latex_file_string = latex_file_path.get_path_utf8();
    std::string latex_file_with_redundant_dot_string = utf8_encode(latex_file_path_with_redundant_dot);
    std::string pdf_file_string = pdf_file_path.get_path_utf8();

    synctex_scanner_p scanner = synctex_scanner_new_with_output_file(pdf_file_string.c_str(), nullptr, 1);

    int stat = synctex_display_query(scanner, latex_file_string.c_str(), line, column, 0);
    int target_page = -1;

    if (stat <= 0) {
        stat = synctex_display_query(scanner, latex_file_with_redundant_dot_string.c_str(), line, column, 0);
    }

    if (stat > 0) {
        synctex_node_p node;

        std::vector<DocumentRect> highlight_rects;

        while ((node = synctex_scanner_next_result(scanner))) {
            int page = synctex_node_page(node);
            target_page = page - 1;

            float x = synctex_node_box_visible_h(node);
            float y = synctex_node_box_visible_v(node);
            float w = synctex_node_box_visible_width(node);
            float h = synctex_node_box_visible_height(node);

            highlight_rects.push_back(DocumentRect({x, y, x + w, y - h}, target_page));

            break; // todo: handle this properly
        }
        if (target_page != -1) {

            if ((main_document_view->get_document() == nullptr) ||
                (pdf_file_path.get_path() != main_document_view->get_document()->get_path())) {

                open_document(pdf_file_path);

            }

            if (!USE_RULER_TO_HIGHLIGHT_SYNCTEX_LINE) {

                std::optional<AbsoluteRect> line_rect_absolute = doc()->get_page_intersecting_rect(highlight_rects[0]);
                if (line_rect_absolute){
                    DocumentRect line_rect = line_rect_absolute->to_document(doc());
                    bool should_recenter = true;
                    if (DONT_FOCUS_IF_SYNCTEX_RECT_IS_VISIBLE){
                        NormalizedWindowRect line_window_rect = line_rect.to_window_normalized(main_document_view);
                        if (line_window_rect.is_visible()){
                            should_recenter = false;
                        }
                    }

                    main_document_view->set_synctex_highlights({ line_rect });
                    if (highlight_rects.size() == 0) {
                        main_document_view->goto_page(target_page);
                    }
                    else {
                        if (should_recenter){
                            main_document_view->goto_offset_within_page(target_page, highlight_rects[0].rect.y0);
                        }
                    }
                }
            }
            else {
                if (highlight_rects.size() > 0) {
                    main_document_view->focus_rect(highlight_rects[0]);
                }
            }
        }
    }
    else {
        open_document(pdf_file_path);
    }
    synctex_scanner_free(scanner);
#endif
}

void MainWidget::update_link_with_opened_book_state(Portal lnk, const OpenedBookState& new_state, bool async) {
    std::wstring docpath = main_document_view->get_document()->get_path();
    Document* link_owner = document_manager->get_document(docpath);

    lnk.dst.book_state = new_state;

    if (link_owner) {
        link_owner->update_portal(lnk);
    }

    if (async) {
        background_task_manager->add_task([this, lnk, new_state]() {
            db_manager->update_portal(lnk.uuid, new_state.offset_x, new_state.offset_y, new_state.zoom_level);
            }, this);
    }
    else {
        db_manager->update_portal(lnk.uuid, new_state.offset_x, new_state.offset_y, new_state.zoom_level);
    }
    set_recently_updated_portal(lnk.uuid);

    portal_to_edit = {};
}

void MainWidget::set_recently_updated_portal(const std::string& uuid) {
    if (recently_updated_portal.has_value()) {
        if (recently_updated_portal->uuid == uuid) {
            recently_updated_portal->last_modification_time = QDateTime::currentDateTime();
            return;
        }
        else {
            on_portal_edited(recently_updated_portal->uuid);
        }
    }
    RecentlyUpdatedPortalState s;
    s.uuid = uuid;
    s.last_modification_time = QDateTime::currentDateTime();
    recently_updated_portal = s;
}

void MainWidget::update_closest_link_with_opened_book_state(const OpenedBookState& new_state) {
    std::optional<Portal> closest_link = main_document_view->find_closest_portal();
    if (closest_link) {
        update_link_with_opened_book_state(closest_link.value(), new_state);
    }
}

void MainWidget::invalidate_render() {
    invalidate_ui();
    is_render_invalidated = true;
}

void MainWidget::invalidate_ui() {
    is_render_invalidated = true;
}

void MainWidget::open_document(const PortalViewState& lvs) {
    DocumentViewState dvs;
    auto path = checksummer->get_path(lvs.document_checksum);
    if (path) {
        dvs.book_state = lvs.book_state;
        dvs.document_path = path.value();
        open_document(dvs);
    }
}

void MainWidget::open_document_with_hash(const std::string& path, std::optional<float> offset_x, std::optional<float> offset_y, std::optional<float> zoom_level) {
    std::optional<std::wstring> maybe_path = checksummer->get_path(path);
    if (maybe_path) {
        Path path(maybe_path.value());
        open_document(path, offset_x, offset_y, zoom_level);
    }
}

void MainWidget::open_document(const Path& path, std::optional<float> offset_x, std::optional<float> offset_y, std::optional<float> zoom_level, std::string downloaded_checksum) {
    if (doc()) {
        if (resume_to_server_position_button->isVisible()) {
            resume_to_server_position_button->hide();
        }
        sync_current_file_location_to_servers();
    }

    opengl_widget->clear_all_selections();

    //save the previous document state
    if (main_document_view && doc()) {
        bool should_sync_drawings = doc()->get_drawings_are_dirty();
        main_document_view->persist(true);
        perform_sync_operations_when_document_is_closed(false, should_sync_drawings);
    }

    if (main_document_view->get_view_width() > main_window_width) {
        main_window_width = main_document_view->get_view_width();
    }

    main_document_view->on_view_size_change(main_window_width, main_window_height);
    main_document_view->open_document(path.get_path(), true, {}, false, downloaded_checksum);

    if (downloaded_checksum.size() > 0) {
        // if this documents is downloaded from the server, it must be synced
        doc()->set_is_synced(true);
    }

    on_open_document(path.get_path());

    if (doc()) {
        document_manager->add_tab(doc()->get_path());
        //doc()->set_only_for_portal(false);
    }

    bool has_document = main_document_view_has_document();

    if (has_document) {
        //setWindowTitle(QString::fromStdWString(path.get_path()));
        if (path.filename().has_value()) {
            setWindowTitle(QString::fromStdWString(path.filename().value()));
        }
        else {
            setWindowTitle(QString::fromStdWString(path.get_path()));
        }

    }

    if ((path.get_path().size() > 0) && (!has_document)) {
        show_error_message(L"Could not open file1: " + path.get_path());
    }

    if (offset_x) {
        main_document_view->set_offset_x(offset_x.value());
    }
    if (offset_y) {
        main_document_view->set_offset_y(offset_y.value());
    }

    if (zoom_level) {
        main_document_view->set_zoom_level(zoom_level.value(), true);
    }

    show_password_prompt_if_required();

    if (main_document_view_has_document()) {
        update_scrollbar();
    }

    deselect_document_indices();
    invalidate_render();

}

void MainWidget::open_document_at_location(const Path& path_,
    int page,
    std::optional<float> x_loc,
    std::optional<float> y_loc,
    std::optional<float> zoom_level,
    bool should_push_state)
{
    //save the previous document state
    if (main_document_view) {
        main_document_view->persist();
    }
    std::wstring path = path_.get_path();

    open_document(path, true, {}, true);
    bool has_document = main_document_view_has_document();

    if (has_document && should_push_state) {
        //setWindowTitle(QString::fromStdWString(path));
        push_state();
    }

    if ((path.size() > 0) && (!has_document)) {
        show_error_message(L"Could not open file2: " + path);
    }
    else {
        main_document_view->on_view_size_change(main_window_width, main_window_height);

        AbsoluteDocumentPos absolute_pos = DocumentPos{ page, x_loc.value_or(0), y_loc.value_or(0) }.to_absolute(doc());

        if (x_loc) {
            main_document_view->set_offset_x(absolute_pos.x);
        }
        main_document_view->set_offset_y(absolute_pos.y);

        if (zoom_level) {
            main_document_view->set_zoom_level(zoom_level.value(), true);
        }
    }


    show_password_prompt_if_required();
}

void MainWidget::open_document(const DocumentViewState& state)
{
    open_document(state.document_path, state.book_state.offset_x, state.book_state.offset_y, state.book_state.zoom_level);
}


bool MainWidget::is_waiting_for_symbol() {
    return ((pending_command_instance != nullptr) &&
        pending_command_instance->next_requirement(this).has_value() &&
        (pending_command_instance->next_requirement(this).value().type == RequirementType::Symbol));
}

bool MainWidget::handle_command_types(std::unique_ptr<Command> new_command, int num_repeats, std::wstring* result) {

    if (new_command == nullptr) {
        return false;
    }

    if (new_command) {
        new_command->set_num_repeats(num_repeats);
        if (new_command->pushes_state()) {
            push_state();
        }
        advance_command(std::move(new_command), result);
        update_scrollbar();
    }
    return true;

}

void MainWidget::handle_text_edit_return_pressed() {
    if (text_command_line_edit_container->isVisible()) {
        text_command_line_edit_container->hide();
        setFocus();
        handle_pending_text_command(text_command_line_edit->text().toStdWString());
        return;
    }
}

void MainWidget::key_event(bool released, QKeyEvent* kevent, bool is_auto_repeat) {

    if (released && (!is_auto_repeat)) {
        set_last_performed_command({});
    }
    if (typing_location.has_value()) {

        if (released == false) {
            if (kevent->key() == Qt::Key::Key_Escape) {
                handle_escape();
                return;
            }

            bool should_focus = false;
            if (kevent->key() == Qt::Key::Key_Return) {
                typing_location.value().next_char();
            }
            else if (kevent->key() == Qt::Key::Key_Backspace) {
                typing_location.value().backspace();
            }
            else if (kevent->text().size() > 0) {
                char c = kevent->text().at(0).unicode();
                should_focus = typing_location.value().advance(c);
            }

            int page = typing_location.value().page;
            DocumentRect character_rect = DocumentRect(rect_from_quad(typing_location.value().character->quad), page);
            std::optional<DocumentRect> wrong_rect = {};

            if (typing_location.value().previous_character) {
                wrong_rect = DocumentRect(rect_from_quad(typing_location.value().previous_character->character->quad), page);
            }

            if (should_focus) {
                main_document_view->set_offset_y(typing_location.value().focus_offset());
            }
            main_document_view->set_typing_rect(character_rect, wrong_rect);

        }
        validate_render();
        return;

    }


    if (released == false) {

#ifdef SIOYEK_ANDROID
        if (kevent->key() == Qt::Key::Key_VolumeDown) {
            execute_macro_if_enabled(VOLUME_DOWN_COMMAND);
        }
        if (kevent->key() == Qt::Key::Key_VolumeUp) {
            execute_macro_if_enabled(VOLUME_UP_COMMAND);
        }
        if (kevent->key() == Qt::Key::Key_MediaTogglePlayPause){
            // handle the tablet button
            execute_macro_if_enabled(TABLET_PEN_CLICK_COMMAND);
        }
        if (kevent->key() == Qt::Key::Key_MediaNext){
            // handle the tablet button
            execute_macro_if_enabled(TABLET_PEN_DOUBLE_CLICK_COMMAND);
        }
#endif


        if (kevent->key() == Qt::Key::Key_Escape) {
            handle_escape();
        }

        if (kevent->key() == Qt::Key::Key_Return || kevent->key() == Qt::Key::Key_Enter) {
            handle_text_edit_return_pressed();
        }

        std::vector<int> ignored_codes = {
            Qt::Key::Key_Shift,
            Qt::Key::Key_Control,
            Qt::Key::Key_Meta,
            Qt::Key::Key_Alt
        };
        if (std::find(ignored_codes.begin(), ignored_codes.end(), kevent->key()) != ignored_codes.end()) {
            return;
        }
        if (is_waiting_for_symbol()) {

            char symb = get_symbol(kevent->key(), kevent->modifiers() & Qt::ShiftModifier, pending_command_instance->special_symbols());
            if (symb) {
                pending_command_instance->set_symbol_requirement(symb);
                advance_command(std::move(pending_command_instance));
            }
            validate_render();
            return;
        }
        int num_repeats = 0;

        bool is_meta_pressed = is_platform_meta_pressed(kevent);
        bool is_control_pressed =  is_platform_control_pressed(kevent);

        std::unique_ptr<Command> commands = input_handler->handle_key(this,
            kevent,
            kevent->modifiers() & Qt::ShiftModifier,
            is_control_pressed,
            is_meta_pressed,
            kevent->modifiers() & Qt::AltModifier,
            &num_repeats);

        if (commands) {
            if (last_performed_command && last_performed_command->is_holdable() && commands->get_name() == last_performed_command->get_name()) {
                last_performed_command->on_key_hold();
            }
            else {
                handle_command_types(std::move(commands), num_repeats);
                validate_render();
            }
        }
        //for (auto& command : commands) {
        //    handle_command_types(std::move(command), num_repeats);
        //}
    }

}

void MainWidget::handle_right_click(WindowPos click_pos, bool down, bool is_shift_pressed, bool is_control_pressed, bool is_command_pressed, bool is_alt_pressed) {

    AbsoluteDocumentPos click_abspos = click_pos.to_absolute(dv());

    if (is_scratchpad_mode()){
        return;
    }

    if (is_shift_pressed || is_control_pressed || is_command_pressed || is_alt_pressed) {
        return;
    }

    if (SHOW_RIGHT_CLICK_CONTEXT_MENU && down && show_contextual_context_menu()) {
        return;
    }
    if (RIGHT_CLICK_COMMAND.size() > 0) {
        execute_macro_if_enabled(RIGHT_CLICK_COMMAND);
        return;
    }

    if (is_rotated()) {
        return;
    }

    if ((down == true) && main_document_view->get_overview_page()) {
        set_overview_page({}, false);
        //main_document_view->set_line_index(-1);
        invalidate_render();
        return;
    }

    if (down==true){
        std::optional<VisibleObjectIndex> visible_object_under_cursor = main_document_view->get_visible_object_at_pos(click_abspos);
        if (visible_object_under_cursor.has_value() && visible_object_under_cursor->object_type == VisibleObjectType::Bookmark){
            BookMark* bookmark = doc()->get_bookmark_with_uuid(visible_object_under_cursor->uuid);
            if (main_document_view->handle_right_click_bookmark(click_pos, bookmark)){
                invalidate_render();
                return;
            }

        }

    }

    if ((main_document_view->get_document() != nullptr) && (opengl_widget != nullptr)) {

        // disable visual mark and overview window when we are in synctex mode
        // because we probably don't need them (we are editing our own document after all)
        // we can always use middle click to jump to a destination which is probably what we
        // need anyway
        if (down == true && (!this->synctex_mode)) {
            if (pending_command_instance && (pending_command_instance->get_name() == "goto_mark")) {
                return_to_last_visual_mark();
                return;
            }

            if (overview_under_pos(click_pos)) {
                invalidate_render();
                return;
            }

            if (visual_scroll_mode) {
                if (is_in_middle_right_rect(click_pos)) {
                    overview_to_definition();
                    return;
                }
            }

            visual_mark_under_pos(click_pos);

        }
        else {
            if (this->synctex_mode) {
                if (down == false) {
                    synctex_under_pos(click_pos);
                }
            }
        }

    }

}

void MainWidget::download_and_portal_to_highlighted_overview_paper() {
    auto paper_name = main_document_view->get_overview_paper_name();
    auto source_rect = main_document_view->get_overview_source_rect();
    if (paper_name && source_rect) {
        download_and_portal(paper_name->toStdWString(), source_rect->center());
    }
}

bool MainWidget::handle_left_press_touch_mode(WindowPos click_pos) {
    was_last_mouse_down_in_ruler_next_rect = false;
    was_last_mouse_down_in_ruler_prev_rect = false;

    last_press_point = mapFromGlobal(QCursor::pos());
    last_press_msecs = QDateTime::currentMSecsSinceEpoch();
    main_document_view->velocity_x = 0;
    main_document_view->velocity_y = 0;
    is_pressed = true;

    if (current_widget_stack.size() > 0 && (dynamic_cast<AndroidSelector*>(current_widget_stack.back()))) {
        return true;
    }

    int window_width = width();
    int window_height = height();

    NormalizedWindowPos nwp = main_document_view->window_to_normalized_window_pos(click_pos);

    if (main_document_view->is_ruler_mode()) {
        if (screen()->orientation() == Qt::PortraitOrientation) {
            if (PORTRAIT_VISUAL_MARK_NEXT.enabled && PORTRAIT_VISUAL_MARK_NEXT.contains(nwp)) {
                main_document_view->move_visual_mark_next();
                was_last_mouse_down_in_ruler_next_rect = true;
                ruler_moving_last_window_pos = click_pos;

            }
            else if (PORTRAIT_VISUAL_MARK_PREV.enabled && PORTRAIT_VISUAL_MARK_PREV.contains(nwp)) {
                main_document_view->move_visual_mark_prev();
                was_last_mouse_down_in_ruler_prev_rect = true;
                ruler_moving_last_window_pos = click_pos;
            }
        }
        else {
            if (LANDSCAPE_VISUAL_MARK_NEXT.enabled && LANDSCAPE_VISUAL_MARK_NEXT.contains(nwp)) {
                main_document_view->move_visual_mark_next();
                was_last_mouse_down_in_ruler_next_rect = true;
                ruler_moving_last_window_pos = click_pos;
            }
            else if (LANDSCAPE_VISUAL_MARK_PREV.enabled && LANDSCAPE_VISUAL_MARK_PREV.contains(nwp)) {
                main_document_view->move_visual_mark_prev();
                was_last_mouse_down_in_ruler_prev_rect = true;
                ruler_moving_last_window_pos = click_pos;
            }

        }
    }
    return false;

}

bool MainWidget::handle_left_release_touch_mode(WindowPos click_pos) {
    is_pressed = false;
    QPoint current_pos = mapFromGlobal(QCursor::pos());
    qint64 current_time = QDateTime::currentMSecsSinceEpoch();
    QPointF vel;
    if (((current_pos - last_press_point).manhattanLength() < 10) && ((current_time - last_press_msecs) < 500)) {
        if (handle_quick_tap(click_pos)) {
            stop_dragging();
            invalidate_render();
            return true;
        }
    }
    else if (is_flicking(&vel)) {
        main_document_view->velocity_x = -vel.x();
        main_document_view->velocity_y = vel.y();
        if (is_dragging_snapped) {
            main_document_view->velocity_x = 0;
        }
        if (main_document_view->is_moving()) {
            validation_interval_timer->setInterval(0);
        }
        last_speed_update_time = QTime::currentTime();
    }
    return false;
}

bool MainWidget::handle_left_click_point_select(AbsoluteDocumentPos abs_doc_pos) {
    if (pending_command_instance) {
        pending_command_instance->set_point_requirement(abs_doc_pos);
        advance_command(std::move(pending_command_instance));
    }

    main_document_view->is_selecting = false;
    this->point_select_mode = false;
    main_document_view->clear_selected_rectangle();
    return true;
}

void MainWidget::MainWidget::handle_rect_selection_point_press(AbsoluteDocumentPos abs_doc_pos) {
    if (rect_select_end.has_value()) {
        //clicked again after selecting, we should clear the selected rectangle
        clear_selected_rect();
    }
    else {
        rect_select_begin = abs_doc_pos;
    }
}

void MainWidget::handle_rect_selection_point_release(AbsoluteDocumentPos abs_doc_pos) {
    if (rect_select_begin.has_value() && rect_select_end.has_value()) {
        rect_select_end = abs_doc_pos;
        AbsoluteRect selected_rectangle = AbsoluteRect(rect_select_begin.value(), rect_select_end.value());
        main_document_view->set_selected_rectangle(selected_rectangle);

        // is pending rect command
        if (pending_command_instance) {
            pending_command_instance->set_rect_requirement(selected_rectangle);
            advance_command(std::move(pending_command_instance));
        }

        this->rect_select_mode = false;
        this->rect_select_begin = {};
        this->rect_select_end = {};
    }
}

void MainWidget::handle_overview_download_button_click(AbsoluteDocumentPos abs_doc_pos) {
    last_mouse_down = abs_doc_pos;
    download_and_portal_to_highlighted_overview_paper();
    close_overview();
}


bool MainWidget::handle_overview_click(WindowPos click_pos, AbsoluteDocumentPos abs_doc_pos) {
    NormalizedWindowPos click_normalized_window_pos = click_pos.to_window_normalized(dv());
    bool is_in_overview = main_document_view->is_window_point_in_overview(click_normalized_window_pos);
    bool is_in_download = dv()->get_overview_download_rect().contains(click_pos);
    if (!TOUCH_MODE && is_in_overview && is_in_download) {
        handle_overview_download_button_click(abs_doc_pos);
        return true;
    }

    OverviewSide border_index = static_cast<OverviewSide>(-1);
    if (!is_in_download && main_document_view->is_window_point_in_overview_border(click_normalized_window_pos, &border_index)) {
        OverviewResizeData resize_data;
        resize_data.original_normal_mouse_pos = click_normalized_window_pos;
        resize_data.original_rect = main_document_view->get_overview_rect();
        resize_data.side_index = border_index;
        overview_resize_data = resize_data;
        return true;
    }
    if (is_in_overview) {
        float original_offset_x, original_offset_y;

        if (TOUCH_MODE) {
            OverviewTouchMoveData touch_move_data;
            //touch_move_data.original_mouse_offset_y = doc()->document_to_absolute_pos(opengl_widget->window_pos_to_overview_pos({ normal_x, normal_y })).y;
            touch_move_data.original_mouse_normalized_pos = click_normalized_window_pos;
            float overview_offset_y = main_document_view->get_overview_page()->absolute_offset_y;
            float overview_offset_x = main_document_view->get_overview_page()->absolute_offset_x;
            touch_move_data.overview_original_pos_absolute = AbsoluteDocumentPos{ overview_offset_x, overview_offset_y };
            overview_touch_move_data = touch_move_data;
        }
        else {
            OverviewMoveData move_data;
            main_document_view->get_overview_offsets(&original_offset_x, &original_offset_y);
            move_data.original_normal_mouse_pos = click_normalized_window_pos;
            move_data.original_offsets = fvec2{ original_offset_x, original_offset_y };
            overview_move_data = move_data;
        }

        return true;
    }
    return false;
}

bool MainWidget::handle_visible_object_resize_finish() {
    auto& visible_object_resize_data = main_document_view->visible_object_resize_data;

    if (visible_object_resize_data->type == VisibleObjectType::Bookmark) {
        update_bookmark_with_uuid(visible_object_resize_data->object_uuid);
        visible_object_resize_data = {};
        return true;
    }
    if (visible_object_resize_data->type == VisibleObjectType::PinnedPortal) {
        update_portal_with_uuid(visible_object_resize_data->object_uuid);
        visible_object_resize_data = {};
        return true;
    }
    return false;
}

void MainWidget::start_dragging(){
    int page = get_current_page_number();
    auto [min_x, max_x] = doc()->get_min_max_annot_x_for_page(page);
    current_drag_min_annotation_x = min_x;
    current_drag_max_annotation_x = max_x;

    // when the user starts dragging while the document is moving with velocity, we should reset the interval
    if (validation_interval_timer->interval() == 0){
        validation_interval_timer->setInterval(INTERVAL_TIME);
    }

    is_dragging = true;
}

void MainWidget::stop_dragging(){
    is_dragging = false;
}

void MainWidget::handle_left_click(WindowPos click_pos, bool down, bool is_shift_pressed, bool is_control_pressed, bool is_command_pressed, bool is_alt_pressed) {
    AbsoluteDocumentPos abs_doc_pos = main_document_view->get_window_abspos(click_pos);

    if (is_rotated()) {
        // we don't support selection, etc. when document is rotated
        return;
    }
    if (is_shift_pressed || is_control_pressed || is_alt_pressed || is_command_pressed) {
        // these will be handled on mouseReleaseEvent
        return;
    }

    if (main_document_view->selected_freehand_drawings) {
        if (main_document_view->handle_freehand_drawing_click_event(abs_doc_pos)) {
            return;
        }
    }

    if (TOUCH_MODE) {
        if (down) {
            if (handle_left_press_touch_mode(click_pos)) return;
        }
        else {
            if (handle_left_release_touch_mode(click_pos)) return;
        }
    }

    NormalizedWindowPos click_normalized_window_pos = main_document_view->window_to_normalized_window_pos(click_pos);

    if (point_select_mode && (down == false)) {
        if (handle_left_click_point_select(abs_doc_pos)) return;
    }
    if (rect_select_mode) {
        if (down == true) {
            handle_rect_selection_point_press(abs_doc_pos);
        }
        else {
            handle_rect_selection_point_release(abs_doc_pos);
        }
        return;
    }
    else {
        if (down == true) {
            clear_selected_rect();
        }
    }

    if (down == true) {

        if (handle_overview_click(click_pos, abs_doc_pos)) return;

        auto visible_object = main_document_view->get_visible_object_at_pos(abs_doc_pos);

        last_mouse_down = abs_doc_pos;
        last_mouse_down_window_pos = click_pos;
        last_mouse_down_document_virtual_offset = dv()->get_virtual_offset();

        if (visible_object) {
            if (main_document_view->handle_visible_object_click(click_pos, abs_doc_pos, visible_object)) {
                invalidate_render();
                return;
            }
        }


        dv()->selection_begin = abs_doc_pos;


        if (!TOUCH_MODE) {
            main_document_view->selected_character_rects.clear();
        }

        if ((!TOUCH_MODE) && (!mouse_drag_mode)) {
            main_document_view->is_selecting = true;
            if (SINGLE_CLICK_SELECTS_WORDS) {
                main_document_view->is_word_selecting = true;
            }
        }
        else {
            start_dragging();
            if (SNAP_DRAGGING) {
                is_dragging_snapped = true;
            }
        }
    }
    else {
        dv()->selection_end = abs_doc_pos;

        main_document_view->is_selecting = false;
        stop_dragging();

        if (main_document_view->visible_object_resize_data) {
            if (handle_visible_object_resize_finish()) return;
        }

        if (main_document_view->visible_object_scroll_data) {
            main_document_view->visible_object_scroll_data = {};
            //return;
        }

        bool was_resizing_overview =
            overview_move_data.has_value() ||
            overview_resize_data.has_value() ||
            overview_touch_move_data.has_value();

        if (overview_touch_move_data) {
            on_overview_move_end();
        }
        int overview_move_distance = 0;
        if (overview_move_data){
            WindowPos original_mouse_pos = overview_move_data->original_normal_mouse_pos.to_window(main_document_view);
            overview_move_distance = click_pos.manhattan(original_mouse_pos);
        }

        overview_move_data = {};
        overview_touch_move_data = {};
        overview_resize_data = {};

        float dist = manhattan_distance(fvec2(last_mouse_down), fvec2(abs_doc_pos));
        if ((!was_resizing_overview) && (!TOUCH_MODE) && (!mouse_drag_mode) && (dist > 5)) {

            main_document_view->get_text_selection(last_mouse_down,
                abs_doc_pos,
                main_document_view->is_word_selecting,
                main_document_view->selected_character_rects,
                dv()->selected_text);
            main_document_view->selected_text_is_dirty = false;

            //opengl_widget->set_control_character_rect(control_rect);
            main_document_view->is_word_selecting = false;
        }
        else {
            // clear the potential drag candidate as we are clicking rather than dragging
            if (main_document_view->visible_object_move_data && !main_document_view->visible_object_move_data->is_moving) {
                main_document_view->visible_object_move_data = {};
            }

            if (!TOUCH_MODE) {
                if ((overview_move_distance > 10) && (dist > 5)) {
                    // don't click on overview links when we are moving them
                    return;
                }
                handle_click(click_pos);
                main_document_view->clear_selected_text();
            }
            else {
                int distance = abs(click_pos.x - last_mouse_down_window_pos.x) + abs(click_pos.y - last_mouse_down_window_pos.y);
                if (distance < 20 && (!was_hold_gesture)) { // we don't want to accidentally click on links when moving the document
                    handle_click(click_pos);
                }
            }

        }
        validate_render();
    }
}


void MainWidget::push_state(bool update) {

    if (!main_document_view_has_document()) return; // we don't add empty document to history

    DocumentViewState dvs = main_document_view->get_state();

    //if (history.size() > 0) { // this check should always be true
    //	history[history.size() - 1] = dvs;
    //}
    //// don't add the same place in history multiple times
    //// todo: we probably don't need this check anymore
    //if (history.size() > 0) {
    //	DocumentViewState last_history = history.back();
    //	if (last_history == dvs) return;
    //}

    // delete all history elements after the current history point
    history.erase(history.begin() + (1 + current_history_index), history.end());
    if (!((history.size() > 0) && (history.back() == dvs))) {
        history.push_back(dvs);
    }
    if (update) {
        current_history_index = static_cast<int>(history.size() - 1);
    }
}

void MainWidget::next_state() {
    if (current_widget_stack.size() > 0 && dynamic_cast<SioyekDocumentationTextBrowser*>(current_widget_stack.back())) {
        // if we are showing documentation, use history navigation commands to navigate the documentation 
        SioyekDocumentationTextBrowser* doc_browser = dynamic_cast<SioyekDocumentationTextBrowser*>(current_widget_stack.back());
        doc_browser->forward();
    }

    if (current_history_index < (static_cast<int>(history.size()) - 1)) {
        update_current_history_index();
        current_history_index++;
        if (current_history_index + 1 < history.size()) {
            set_main_document_view_state(history[current_history_index + 1]);
        }

    }
}

void MainWidget::prev_state() {
    if (current_widget_stack.size() > 0 && dynamic_cast<SioyekDocumentationTextBrowser*>(current_widget_stack.back())) {
        // if we are showing documentation, use history navigation commands to navigate the documentation 
        SioyekDocumentationTextBrowser* doc_browser = dynamic_cast<SioyekDocumentationTextBrowser*>(current_widget_stack.back());
        doc_browser->backward();
    }
    if (current_history_index >= 0) {
        update_current_history_index();

        /*
        Goto previous history
        In order to edit a link, we set the link to edit and jump to the link location, when going back, we
        update the link with the current location of document, therefore, we must check to see if a link
        is being edited and if so, we should update its destination position
        */
        if (portal_to_edit) {

            //std::wstring link_document_path = checksummer->get_path(link_to_edit.value().dst.document_checksum).value();
            std::wstring link_document_path = history[current_history_index].document_path;
            Document* link_owner = document_manager->get_document(link_document_path);

            OpenedBookState state = main_document_view->get_state().book_state;
            portal_to_edit.value().dst.book_state = state;

            if (link_owner) {
                link_owner->update_portal(portal_to_edit.value());
            }

            db_manager->update_portal(portal_to_edit->uuid, state.offset_x, state.offset_y, state.zoom_level);
            set_recently_updated_portal(portal_to_edit->uuid);

            portal_to_edit = {};
        }

        if (current_history_index == (history.size() - 1)) {
            if (!(history[history.size() - 1] == main_document_view->get_state())) {
                push_state(false);
            }
        }
        if (history[current_history_index] == main_document_view->get_state()) {
            current_history_index--;
        }
        if (current_history_index >= 0) {
            DocumentViewState new_state = history[current_history_index];
            // save the current document in the list of opened documents
            if (doc() && doc()->get_path() != new_state.document_path) {
                persist();
            }
            set_main_document_view_state(new_state);
            current_history_index--;
        }
    }
}

void MainWidget::update_current_history_index() {
    if (main_document_view_has_document()) {
        int index_to_update = current_history_index + 1;
        if (index_to_update < history.size()) {
            DocumentViewState current_state = main_document_view->get_state();
            history[index_to_update] = current_state;
        }
    }
}

void MainWidget::set_main_document_view_state(DocumentViewState new_view_state) {

    if ((!main_document_view_has_document()) || (main_document_view->get_document()->get_path() != new_view_state.document_path)) {
        open_document(new_view_state.document_path, &this->is_ui_invalidated);

        //setwindowtitle(qstring::fromstdwstring(new_view_state.document_path));
    }

    main_document_view->on_view_size_change(main_window_width, main_window_height);
    main_document_view->set_book_state(new_view_state.book_state);
}

void MainWidget::handle_click(WindowPos click_pos) {


    if (!main_document_view_has_document()) {
        return;
    }
    if (is_scratchpad_mode()){
        return;
    }

    auto [normal_x, normal_y] = click_pos.to_window_normalized(main_document_view);
    AbsoluteDocumentPos mouse_abspos = click_pos.to_absolute(main_document_view);

    if (main_document_view->is_window_point_in_overview({ normal_x, normal_y })) {
        auto [doc_page, doc_x, doc_y] = main_document_view->window_pos_to_overview_pos({ normal_x, normal_y });
        auto link = main_document_view->get_document()->get_link_in_pos(doc_page, doc_x, doc_y);
        if (link) {
            handle_link_click(link.value());
        }
        else {
            // try to update the target paper name
            NormalizedWindowPos normal_click_pos = click_pos.to_window_normalized(main_document_view);
            DocumentPos overview_doc_pos = main_document_view->window_pos_to_overview_pos(normal_click_pos);
            main_document_view->update_overview_highlighted_paper_with_position(overview_doc_pos);
        }
        return;
    }

    auto link = main_document_view->get_link_in_pos(click_pos);

    std::string selected_uuid = main_document_view->get_highlight_uuid_in_pos(click_pos);
    if (selected_uuid.size() > 0) {
        main_document_view->set_selected_highlight_uuid(selected_uuid);
    }
    else if ((selected_uuid = doc()->get_bookmark_uuid_at_pos(mouse_abspos)).size() > 0) {
        bool was_bookmark_anchor_link_click = false;
        BookMark* bookmark = doc()->get_bookmark_with_uuid(selected_uuid);

        if (bookmark) {
            if (BookMark::should_be_displayed_as_markdown(QString::fromStdWString(bookmark->description))) {
                QString anchor_string = main_document_view->get_markdown_bookmark_anchor_text_under_pos(QPoint(click_pos.x, click_pos.y));
                if (anchor_string.size() > 0) {
                    was_bookmark_anchor_link_click = true;
                    if (!anchor_string.startsWith("sioyek://")) {
                        open_web_url(anchor_string.toStdWString());
                    }
                    else {
                        QString url_encoded = anchor_string.mid(9);
                        QString decoded = QString(QUrl::fromPercentEncoding(url_encoded.toUtf8()));
                        if (decoded.size() > 1) {
                            if (decoded[0] == '\"' && decoded[decoded.size() - 1] == '\"') {
                                decoded = decoded.mid(1, decoded.size() - 2);
                            }
                            decoded = decoded.trimmed();
                            push_state();
                            main_document_view->perform_fuzzy_search(decoded.toStdWString());
                        }
                    }
                }
            }
        }
        if (!was_bookmark_anchor_link_click) {
            main_document_view->set_selected_bookmark_uuid(selected_uuid);
        }
    }
    else if ((selected_uuid = doc()->get_pinned_portal_uuid_at_pos(mouse_abspos)).size() > 0) {
        main_document_view->set_selected_portal_uuid(selected_uuid, true);
    }
    else if ((selected_uuid = doc()->get_icon_portal_uuid_at_pos(mouse_abspos)).size() > 0) {
        main_document_view->set_selected_portal_uuid(selected_uuid);
    }
    else {
        main_document_view->clear_selected_object();
    }

    if (main_document_view->selected_object_index.has_value() && main_document_view->selected_object_index->object_type == VisibleObjectType::Portal) {
        Portal* portal = doc()->get_portal_with_uuid(main_document_view->selected_object_index->uuid);

        if (portal) {

            push_state();
            if (document_manager->get_document_with_checksum(portal->dst.document_checksum)) {
                open_document(portal->dst);
            }
            else if (sioyek_network_manager->is_checksum_available_on_server(portal->dst.document_checksum)) {
                sioyek_network_manager->download_file_with_hash(this, QString::fromStdString(portal->dst.document_checksum), [this, portal](QString path) {
                    //void open_document(const std::wstring& doc_path, bool* invalid_flag, bool load_prev_state = true, std::optional<OpenedBookState> prev_state = {}, bool foce_load_dimensions = false);
                    open_document(path.toStdWString(), false, portal->dst.book_state);
                    });
            }
        }


        return;
    }


    if (TOUCH_MODE && (main_document_view->get_selected_highlight_uuid().size() > 0)) {
        show_highlight_buttons();
    }


    if (link.has_value()) {
        handle_link_click(link.value());
        return;
    }
    else {
        if (!TOUCH_MODE) {
            if (main_document_view) {
                 main_document_view->exit_ruler_mode();
                 main_document_view->clear_underline();
            }
            //if (opengl_widget) opengl_widget->set_should_draw_vertical_line(false);
        }
    }

}



void MainWidget::mouseReleaseEvent(QMouseEvent* mevent) {

    bool is_shift_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier);
    bool is_control_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ControlModifier);
    bool is_command_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::MetaModifier);
    bool is_alt_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::AltModifier);

    if (!TOUCH_MODE && current_widget_stack.size() > 0) {
        pop_current_widget();
        return;
    }

    if (main_document_view->is_drawing) {
        main_document_view->finish_drawing(mevent->pos());
        invalidate_render();
        return;
    }

    if (handle_annotation_move_finish()) {
        return;
    }

    if (TOUCH_MODE) {
        was_last_mouse_down_in_ruler_next_rect = false;
        was_last_mouse_down_in_ruler_prev_rect = false;
        pdf_renderer->no_rerender = false;
    }

    if (is_rotated()) {
        return;
    }

    main_document_view->visible_object_scroll_data = {};

    if (mevent->button() == Qt::MouseButton::LeftButton) {

        if (is_shift_pressed) {
            execute_macro_if_enabled(SHIFT_CLICK_COMMAND);
        }
        else if (is_control_pressed) {
            execute_macro_if_enabled(CONTROL_CLICK_COMMAND);
        }
        else if (is_command_pressed) {
            execute_macro_if_enabled(COMMAND_CLICK_COMMAND);
        }
        else if (is_alt_pressed) {
            execute_macro_if_enabled(ALT_CLICK_COMMAND);
        }
        else {
            handle_left_click({ mevent->pos().x(), mevent->pos().y() }, false, is_shift_pressed, is_control_pressed, is_command_pressed, is_alt_pressed);
            was_hold_gesture = false;
            if (is_select_highlight_mode && (main_document_view->selected_character_rects.size() > 0)) {
                add_highlight_to_current_document(dv()->selection_begin, dv()->selection_end, select_highlight_type);
                main_document_view->clear_selected_text();
            }
            if (main_document_view->selected_character_rects.size() > 0) {
                copy_to_clipboard(main_document_view->get_selected_text(), true);
            }
        }

    }

    if (mevent->button() == Qt::MouseButton::RightButton) {
        if (is_shift_pressed) {
            execute_macro_if_enabled(SHIFT_RIGHT_CLICK_COMMAND);
        }
        else if (is_control_pressed) {
            execute_macro_if_enabled(CONTROL_RIGHT_CLICK_COMMAND);
        }
        else if (is_command_pressed) {
            execute_macro_if_enabled(COMMAND_RIGHT_CLICK_COMMAND);
        }
        else if (is_alt_pressed) {
            execute_macro_if_enabled(ALT_RIGHT_CLICK_COMMAND);
        }
        else {
            handle_right_click({ mevent->pos().x(), mevent->pos().y() }, false, is_shift_pressed, is_control_pressed, is_command_pressed, is_alt_pressed);
        }
    }

    if (mevent->button() == Qt::MouseButton::MiddleButton) {


        if (overview_touch_move_data.has_value()) {
            on_overview_move_end();
            overview_touch_move_data = {};
            return;
        }
        if (!is_dragging) {

            if (HIGHLIGHT_MIDDLE_CLICK
                && main_document_view->selected_character_rects.size() > 0
                && !(main_document_view && main_document_view->get_overview_page())) {

                add_highlight_to_current_document(dv()->selection_begin, dv()->selection_end, select_highlight_type);
                main_document_view->clear_selected_text();

                validate_render();
            }
            else {
                if (last_middle_down_time.msecsTo(QTime::currentTime()) > 200) {
                }
                else {
                    if (is_shift_pressed) {
                        execute_macro_if_enabled(SHIFT_MIDDLE_CLICK_COMMAND);
                    }
                    else if (is_control_pressed) {
                        execute_macro_if_enabled(CONTROL_MIDDLE_CLICK_COMMAND);
                    }
                    else if (is_command_pressed) {
                        execute_macro_if_enabled(COMMAND_MIDDLE_CLICK_COMMAND);
                    }
                    else if (is_alt_pressed) {
                        execute_macro_if_enabled(ALT_MIDDLE_CLICK_COMMAND);
                    }
                    else {
                        execute_macro_if_enabled(MIDDLE_CLICK_COMMAND);
                        //smart_jump_under_pos({ mevent->pos().x(), mevent->pos().y() });
                        //handle_right_click({ mevent->pos().x(), mevent->pos().y() }, false, is_shift_pressed, is_control_pressed, is_command_pressed, is_alt_pressed);
                    }
                }
            }
        }
        else {
            stop_dragging();

        }
    }

}

void MainWidget::mouseDoubleClickEvent(QMouseEvent* mevent) {
    if (!doc()) return;

    if (!TOUCH_MODE) {
        if (mevent->button() == Qt::MouseButton::LeftButton) {
            main_document_view->is_selecting = true;
            if (SINGLE_CLICK_SELECTS_WORDS) {
                main_document_view->is_word_selecting = false;
            }
            else {
                main_document_view->is_word_selecting = true;
            }
        }

        WindowPos click_pos = { mevent->pos().x(), mevent->pos().y() };
        AbsoluteDocumentPos mouse_abspos = main_document_view->window_to_absolute_document_pos(click_pos);
        std::string bookmark_uuid = doc()->get_bookmark_uuid_at_pos(mouse_abspos);
        std::string highlight_uuid = main_document_view->get_highlight_uuid_in_pos(click_pos);

        if (bookmark_uuid.size() > 0) {
            main_document_view->set_selected_bookmark_uuid(bookmark_uuid);
            handle_command_types(command_manager->get_command_with_name(this, "edit_selected_bookmark"), 0);
            return;
        }
        if (highlight_uuid.size() > 0) {
            main_document_view->set_selected_highlight_uuid(highlight_uuid);
            handle_command_types(command_manager->get_command_with_name(this, "add_annot_to_highlight"), 0);
        }
    }
}

void MainWidget::mousePressEvent(QMouseEvent* mevent) {
    bool is_shift_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier);
    bool is_control_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ControlModifier);
    bool is_command_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::MetaModifier);
    bool is_alt_pressed = QGuiApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::AltModifier);

    if (!TOUCH_MODE && current_widget_stack.size() > 0) {
        return;
    }

    if (should_draw(false) && (mevent->button() == Qt::MouseButton::LeftButton)) {
        main_document_view->start_drawing();
        return;
    }

    if (mevent->button() == Qt::MouseButton::LeftButton) {
        handle_left_click({ mevent->pos().x(), mevent->pos().y() }, true, is_shift_pressed, is_control_pressed, is_command_pressed, is_alt_pressed);
    }

    if (mevent->button() == Qt::MouseButton::RightButton) {
        handle_right_click({ mevent->pos().x(), mevent->pos().y() }, true, is_shift_pressed, is_control_pressed, is_command_pressed, is_alt_pressed);
    }

    if (mevent->button() == Qt::MouseButton::MiddleButton) {
        //if (MIDDLE_CLICK_COMMAND.size() > 0) {
        //    execute_macro_if_enabled(MIDDLE_CLICK_COMMAND);
        //    return;
        //}
        last_middle_down_time = QTime::currentTime();
        middle_click_hold_command_already_executed = false;
        last_mouse_down_window_pos = WindowPos{ mevent->pos().x(), mevent->pos().y() };
        last_mouse_down_document_virtual_offset = dv()->get_virtual_offset();

        AbsoluteDocumentPos abs_mpos = dv()->window_to_absolute_document_pos(last_mouse_down_window_pos);
        if (main_document_view->is_pinned_portal_selected()) {
            main_document_view->begin_portal_scroll(last_mouse_down_window_pos);
        }
        else if (!main_document_view->visible_object_move_data.has_value()) {
            auto visible_object_index = main_document_view->get_visible_object_at_pos(abs_mpos);
            if (visible_object_index.has_value()) {
                visible_object_index->handle_move_begin(main_document_view, abs_mpos);
            }
        }
    }

    if (mevent->button() == Qt::MouseButton::XButton1) {
        handle_command_types(command_manager->get_command_with_name(this, "prev_state"), 0);
        invalidate_render();
    }

    if (mevent->button() == Qt::MouseButton::XButton2) {
        handle_command_types(command_manager->get_command_with_name(this, "next_state"), 0);
        invalidate_render();
    }
}

bool MainWidget::is_mouse_cursor_in_overview(){
    WindowPos cursor_pos(mapFromGlobal(QCursor::pos()));
    return dv()->is_window_point_in_overview(cursor_pos.to_window_normalized(dv()));
}

bool MainWidget::is_mouse_cursor_in_statusbar() {
    auto cursor_pos = status_label->mapFromGlobal(QCursor::pos());

    if (status_label->isVisible()) {
        QRect rect = status_label->rect();
        return status_label->rect().contains(cursor_pos);
    }
    return false;
}

std::wstring MainWidget::get_status_part_name_under_cursor() {
    QPoint mouse_pos = mapFromGlobal(QCursor::pos());
    int cursor_pos = status_label_left->cursorPositionAt(mouse_pos);
    QString text = status_label_left->text();
    if (cursor_pos >= 0 && cursor_pos < last_status_string_ids.size()) {
        //qDebug() << text.at(cursor_pos);
        int type = last_status_string_ids[cursor_pos];
        if (type >= 0 && type < STATUS_STRING_PARTS.size()) {
            std::wstring status_part = STATUS_STRING_PARTS[type].toStdWString();
            return status_part;
        }
    }
    return L"";
}

void MainWidget::wheelEvent(QWheelEvent* wevent) {

    if (IGNORE_SCROLL_EVENTS) return;

    float vertical_move_amount = VERTICAL_MOVE_AMOUNT * TOUCHPAD_SENSITIVITY;
    float horizontal_move_amount = HORIZONTAL_MOVE_AMOUNT * TOUCHPAD_SENSITIVITY;

    if (main_document_view_has_document()) {
        main_document_view->disable_auto_resize_mode();
    }

    bool is_control_pressed = QApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier) ||
        QApplication::queryKeyboardModifiers().testFlag(Qt::MetaModifier);

    bool is_shift_pressed = QApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier);
    bool is_visual_mark_mode = main_document_view->is_ruler_mode() && visual_scroll_mode;


#ifdef SIOYEK_QT6
    int x = wevent->position().x();
    int y = wevent->position().y();
#else
    int x = wevent->pos().x();
    int y = wevent->pos().y();
#endif

    WindowPos mouse_window_pos = { x, y };
    AbsoluteDocumentPos mouse_abs_pos = mouse_window_pos.to_absolute(dv());
    auto [normal_x, normal_y] = main_document_view->window_to_normalized_window_pos(mouse_window_pos);

#ifdef SIOYEK_QT6
    int num_repeats = abs(wevent->angleDelta().y() / 120);
    float num_repeats_f_y = abs(wevent->angleDelta().y() / 120.0);
    float num_repeats_f_x = abs(wevent->angleDelta().x() / 120.0);
    float zoom_factor = 1.0f + num_repeats_f_y * (ZOOM_INC_FACTOR - 1.0f);
    if (std::abs(num_repeats_f_x) > std::abs(num_repeats_f_y)){
        num_repeats_f_y = 0;
    }
    else{
        num_repeats_f_x = 0;
    }
#else
    int num_repeats = abs(wevent->delta() / 120);
    float num_repeats_f = abs(wevent->delta() / 120.0);
#endif

    if (num_repeats == 0) {
        num_repeats = 1;
    }

    bool is_touchpad = wevent->pointingDevice()->pointerType() == QPointingDevice::PointerType::Finger;
    bool is_in_overview = is_mouse_cursor_in_overview();

    if (STATUSBAR_HANDLES_WHEEL_EVENTS && is_mouse_cursor_in_statusbar()) {
        std::wstring status_part = get_status_part_name_under_cursor();
        const std::unordered_map<std::wstring, std::wstring>& command_map = wevent->angleDelta().y() > 0 ? STATUS_BAR_WHEEL_UP_COMMANDS : STATUS_BAR_WHEEL_DOWN_COMMANDS;
        
        if (command_map.find(status_part) != command_map.end()) {
            const std::wstring command = command_map.find(status_part)->second;
            execute_macro_if_enabled(command);
        }

        return;
    }

    std::optional<VisibleObjectIndex> object_under_cursor = main_document_view->get_visible_object_at_pos(mouse_abs_pos);
    if ((!is_in_overview) && object_under_cursor.has_value() && (!main_document_view->visible_object_move_data.has_value())) {
        if ((object_under_cursor->object_type == VisibleObjectType::Bookmark) &&
            main_document_view->selected_object_index.has_value() &&
            (main_document_view->selected_object_index->object_type == VisibleObjectType::Bookmark) &&
            (main_document_view->selected_object_index->uuid == object_under_cursor->uuid)) {
            float amount = -VERTICAL_MOVE_AMOUNT * wevent->angleDelta().y() / 120.0f;
            std::string bookmark_uuid = object_under_cursor->uuid;
            scroll_bookmark_with_uuid(bookmark_uuid, amount);
            validate_render();
            return;
        }
        if (
            (object_under_cursor->object_type == VisibleObjectType::PinnedPortal) &&
            main_document_view->selected_object_index.has_value() &&
            (main_document_view->selected_object_index->object_type == VisibleObjectType::PinnedPortal) &&
            (main_document_view->selected_object_index->uuid == object_under_cursor->uuid)) {

            //Portal* portal = doc()->get_portals()[object_under_cursor->index];
            Portal* portal = doc()->get_portal_with_uuid(object_under_cursor->uuid);
            if (portal) {
                if (is_control_pressed) {
                    //float amount = 72.0 * -VERTICAL_MOVE_AMOUNT * wevent->angleDelta().y() / 360;
                    if (wevent->angleDelta().y() > 0) {
                        portal->dst.book_state.zoom_level *= zoom_factor;
                    }
                    else {
                        portal->dst.book_state.zoom_level /= zoom_factor;
                    }
                }
                else if (is_shift_pressed) {
                    float amount = 72.0 * -VERTICAL_MOVE_AMOUNT * wevent->angleDelta().y() / 360;
                    portal->dst.book_state.offset_x += amount;
                }
                else {
                    float amount = 72.0 * -VERTICAL_MOVE_AMOUNT * wevent->angleDelta().y() / 360;
                    portal->dst.book_state.offset_y += amount;
                }

                main_document_view->schedule_update_link_with_opened_book_state(*portal, portal->dst.book_state);
                validate_render();
                return;
            }
        }
    }
    if ( (!is_shift_pressed) && (!is_control_pressed)) {

        if (main_document_view->get_overview_page()) {
            if (main_document_view->is_window_point_in_overview({ normal_x, normal_y })) {
                if (is_touchpad){
                    if (wevent->angleDelta().y() > 0) {
                        main_document_view->scroll_overview_vertical(-72.0f * vertical_move_amount * num_repeats_f_y);
                    }
                    if (wevent->angleDelta().y() < 0) {
                        main_document_view->scroll_overview_vertical(72.0f * vertical_move_amount * num_repeats_f_y);
                    }
                }
                else{
                    if (wevent->angleDelta().y() > 0) {
                        main_document_view->scroll_overview(-1);
                    }
                    if (wevent->angleDelta().y() < 0) {
                        main_document_view->scroll_overview(1);
                    }

                }
            }
            else {
                if (ALLOW_MAIN_VIEW_SCROLL_WHILE_IN_OVERVIEW) {
                    if (wevent->angleDelta().y() > 0) {
                        move_vertical(-72.0f * vertical_move_amount * num_repeats_f_y);
                    }
                    else {
                        move_vertical(72.0f * vertical_move_amount * num_repeats_f_y);
                    }
                    update_scrollbar();
                }
                else {
                    if (wevent->angleDelta().y() > 0) {
                        goto_ith_next_overview(-1);
                    }
                    if (wevent->angleDelta().y() < 0) {
                        goto_ith_next_overview(1);
                    }
                }
            }
            validate_render();
        }
        else {

            if (wevent->angleDelta().y() > 0) {

                if (is_visual_mark_mode) {
                    if (main_document_view->get_overview_page()) {
                        if (!goto_ith_next_overview(1)) {
                            move_visual_mark_command(-num_repeats);
                        }
                        return;
                    }
                    else {
                        /* move_visual_mark_command(-num_repeats); */
                        move_ruler_prev();
                        validate_render();
                        return;
                    }

                }
                else if (main_document_view->is_presentation_mode()) {
                    main_document_view->goto_page(main_document_view->get_center_page_number() - num_repeats);
                    invalidate_render();
                }
                else {
                    move_vertical(-72.0f * vertical_move_amount * num_repeats_f_y);
                    update_scrollbar();
                    // return;
                }
            }
            if (wevent->angleDelta().y() < 0) {

                if (is_visual_mark_mode) {
                    if (main_document_view->get_overview_page()) {
                        if (!goto_ith_next_overview(-1)) {
                            move_visual_mark_command(num_repeats);
                        }
                        return;
                    }
                    else {
                        /* move_visual_mark_command(num_repeats); */
                        move_ruler_next();
                        validate_render();
                        return;
                    }
                }
                else if (main_document_view->is_presentation_mode()) {
                    main_document_view->goto_page(main_document_view->get_center_page_number() + num_repeats);
                    invalidate_render();
                }
                else {
                    move_vertical(72.0f * vertical_move_amount * num_repeats_f_y);
                    update_scrollbar();
                    // return;
                }
            }

            float inverse_factor = INVERTED_HORIZONTAL_SCROLLING ? -1.0f : 1.0f;

            if (wevent->angleDelta().x() > 0) {
                move_horizontal(72.0f * horizontal_move_amount * num_repeats_f_x * inverse_factor);
                return;
            }
            if (wevent->angleDelta().x() < 0) {
                move_horizontal(-72.0f * horizontal_move_amount * num_repeats_f_x * inverse_factor);
                return;
            }
        }
    }

    if (is_control_pressed) {
        if (main_document_view->get_overview_page() && main_document_view->is_window_point_in_overview({ normal_x, normal_y })) {
            if (wevent->angleDelta().y() > 0) {
                main_document_view->zoom_in_overview();
            }
            else if (wevent->angleDelta().y() < 0) {
                main_document_view->zoom_out_overview();
            }
            validate_render();
            return;
        }
        else {
            float scroll_zoom_factor = 1.0f + num_repeats_f_y * (SCROLL_ZOOM_INC_FACTOR - 1.0f);
            zoom(mouse_window_pos, scroll_zoom_factor, wevent->angleDelta().y() > 0);
            return;
        }
    }
    if (is_shift_pressed) {
        float inverse_factor = INVERTED_HORIZONTAL_SCROLLING ? -1.0f : 1.0f;

        bool is_macos = false;
#ifdef Q_OS_MACOS
        is_macos = true;
#endif
        if (is_macos){
            if (is_touchpad){
                if (wevent->angleDelta().y() > 0) {
                    move_horizontal(-72.0f * horizontal_move_amount * num_repeats_f_y * inverse_factor);
                    return;
                }
                if (wevent->angleDelta().y() < 0) {
                    move_horizontal(72.0f * horizontal_move_amount * num_repeats_f_y * inverse_factor);
                    return;
                }
            }
            else{
                if (wevent->angleDelta().x() > 0) {
                    move_horizontal(-72.0f * horizontal_move_amount * num_repeats_f_x * inverse_factor);
                    return;
                }
                if (wevent->angleDelta().x() < 0) {
                    move_horizontal(72.0f * horizontal_move_amount * num_repeats_f_x * inverse_factor);
                    return;
                }
            }
        }
        else{
            if (wevent->angleDelta().y() > 0) {
                move_horizontal(-72.0f * horizontal_move_amount * num_repeats_f_y * inverse_factor);
                return;
            }
            if (wevent->angleDelta().y() < 0) {
                move_horizontal(72.0f * horizontal_move_amount * num_repeats_f_y * inverse_factor);
                return;
            }
        }


    }
}

void MainWidget::show_mark_selector() {
    TouchMarkSelector* mark_selector = new TouchMarkSelector(this);

    set_current_widget(mark_selector);
    QObject::connect(mark_selector, &TouchMarkSelector::onMarkSelected, [&](QString mark) {

        if (mark.size() > 0 && pending_command_instance) {
            char symbol = mark.toStdString().at(0);
            pending_command_instance->set_symbol_requirement(symbol);
            advance_command(std::move(pending_command_instance));
            invalidate_render();
        }

        pop_current_widget();
        });
}

void MainWidget::show_textbar(const std::wstring& command_name, bool is_password, const std::wstring& initial_value) {
    QString init = "";
    text_suggestion_index = 0;

    if (initial_value.size() > 0) {
        init = QString::fromStdWString(initial_value);
    }
    if (TOUCH_MODE) {

        TouchTextEdit* edit_widget = new TouchTextEdit(QString::fromStdWString(command_name), init, is_password, this);

        QObject::connect(edit_widget, &TouchTextEdit::confirmed, [&](QString text) {
            pop_current_widget();
            //setFocus();
            handle_pending_text_command(text.toStdWString());
            });

        QObject::connect(edit_widget, &TouchTextEdit::cancelled, [&]() {
            pop_current_widget();
            });

        edit_widget->resize(width() * 0.8, height() * 0.8);
        edit_widget->move(width() * 0.1, height() * 0.1);
        push_current_widget(edit_widget);
        show_current_widget();
    }
    else {
        text_command_line_edit->clear();
        if (init.size() > 0) {
            text_command_line_edit->setText(init);
        }
        text_command_line_edit_label->setText(QString::fromStdWString(command_name));
        text_command_line_edit_container->show();
        text_command_line_edit->setFocus();
        if (initial_value.size() > 0) {
            text_command_line_edit->selectAll();
        }
        if (is_password) {
            text_command_line_edit->setEchoMode(QLineEdit::EchoMode::Password);
        }
        else {
            text_command_line_edit->setEchoMode(QLineEdit::EchoMode::Normal);
        }
    }
}

bool MainWidget::helper_window_overlaps_main_window() {

    QRect main_window_rect = get_main_window_rect();
    QRect helper_window_rect = get_helper_window_rect();
    QRect intersection = main_window_rect.intersected(helper_window_rect);
    return intersection.width() > 50;
}

void MainWidget::toggle_window_configuration() {

    QWidget* helper_window = get_top_level_widget(helper_opengl_widget());
    QWidget* main_window = get_top_level_widget(opengl_widget);

    if (is_helper_visible()){
        apply_window_params_for_one_window_mode();
    }
    else {
        apply_window_params_for_two_window_mode();
        if (helper_window_overlaps_main_window()) {
            helper_window->activateWindow();
        }
        else {
            main_window->activateWindow();
        }


    }
}

void MainWidget::toggle_two_window_mode() {

    //main_widget.resize(window_width, window_height);

    QWidget* helper_window = get_top_level_widget(helper_opengl_widget());

    if (!is_helper_visible()) {

        QPoint pos = helper_opengl_widget()->pos();
        QSize size = helper_opengl_widget()->size();
        helper_window->show();
        helper_window->resize(size);
        helper_window->move(pos);

        if (!helper_window_overlaps_main_window()) {
            activateWindow();
        }
    }
    else {
        helper_window->hide();
    }
}

std::optional<PaperNameWithRects> MainWidget::get_paper_name_under_cursor(bool use_last_hold_point) {
    QPoint mouse_pos;
    if (use_last_hold_point) {
        mouse_pos = last_hold_point;
    }
    else {
        mouse_pos = mapFromGlobal(cursor_pos());
    }
    WindowPos window_pos = { mouse_pos.x(), mouse_pos.y() };
    auto normal_pos = main_document_view->window_to_normalized_window_pos(window_pos);

    if (main_document_view->is_window_point_in_overview(normal_pos)) {
        DocumentPos docpos = main_document_view->window_pos_to_overview_pos(normal_pos);
        return main_document_view->get_document()->get_paper_name_at_position(docpos);
    }
    else {
        DocumentPos doc_pos = main_document_view->window_to_document_pos(window_pos);
        return dv()->get_direct_paper_name_under_pos(doc_pos);
    }
}

void MainWidget::smart_jump_under_pos(WindowPos pos) {
    if ((!main_document_view_has_document()) || main_document_view->scratchpad) {
        return;
    }


    Qt::KeyboardModifiers modifiers = QGuiApplication::queryKeyboardModifiers();
    bool is_shift_pressed = modifiers.testFlag(Qt::ShiftModifier);

    auto [normal_x, normal_y] = main_document_view->window_to_normalized_window_pos(pos);

    // if overview page is open and we middle click on a paper name, search it in a search engine
    if (main_document_view->is_window_point_in_overview({ normal_x, normal_y })) {
        DocumentPos docpos = main_document_view->window_pos_to_overview_pos({ normal_x, normal_y });
        std::optional<PaperNameWithRects> paper_name = main_document_view->get_document()->get_paper_name_at_position(docpos);
        if (paper_name) {
            handle_search_paper_name(paper_name->paper_name, is_shift_pressed);
        }
        return;
    }

    auto docpos = main_document_view->window_to_document_pos(pos);

    fz_stext_page* stext_page = main_document_view->get_document()->get_stext_with_page_number(docpos.page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);

    TextUnderPointerInfo text_under_pos_info = dv()->find_location_of_text_under_pointer(docpos);
    if ((text_under_pos_info.candidates.size() > 0) && (text_under_pos_info.candidates[0].reference_type != ReferenceType::None)){
        DocumentPos candid_docpos = text_under_pos_info.candidates[0].get_docpos(main_document_view);
        long_jump_to_destination(candid_docpos.page, candid_docpos.y);
    }
    else {
        std::optional<PaperNameWithRects> paper_name_on_pointer = main_document_view->get_document()->get_paper_name_at_position(flat_chars, docpos);
        if (paper_name_on_pointer) {
            handle_search_paper_name(paper_name_on_pointer->paper_name, is_shift_pressed);
        }
    }

}

void MainWidget::visual_mark_under_pos(WindowPos pos) {
    //float doc_x, doc_y;
    //int page;
    DocumentPos document_pos = main_document_view->window_to_document_pos(pos);
    if (document_pos.page != -1) {
        //opengl_widget->set_should_draw_vertical_line(true);
        int container_line_index = main_document_view->get_line_index_of_pos(document_pos);

        if (container_line_index == -1) {
            main_document_view->set_line_index(main_document_view->get_line_index_of_vertical_pos(), -1);
        }
        else {
            fz_pixmap* pixmap = main_document_view->get_document()->get_small_pixmap(document_pos.page);
            std::vector<unsigned int> hist = get_max_width_histogram_from_pixmap(pixmap);
            std::vector<unsigned int> line_locations;
            std::vector<unsigned int> _;
            get_line_begins_and_ends_from_histogram(hist, _, line_locations);
            int small_doc_x = static_cast<int>(document_pos.x * SMALL_PIXMAP_SCALE);
            int small_doc_y = static_cast<int>(document_pos.y * SMALL_PIXMAP_SCALE);
            int best_vertical_loc = find_best_vertical_line_location(pixmap, small_doc_x, small_doc_y);
            //int best_vertical_loc = line_locations[find_nth_larger_element_in_sorted_list(line_locations, static_cast<unsigned int>(small_doc_y), 2)];
            float best_vertical_loc_doc_pos = best_vertical_loc / SMALL_PIXMAP_SCALE;
            WindowPos window_pos = main_document_view->document_to_window_pos_in_pixels_uncentered(DocumentPos{ document_pos.page, 0, best_vertical_loc_doc_pos });
            auto [abs_doc_x, abs_doc_y] = main_document_view->window_to_absolute_document_pos(window_pos);
            main_document_view->set_vertical_line_pos(abs_doc_y);
            main_document_view->set_line_index(container_line_index, document_pos.page);
        }
        validate_render();

        if (is_reading) {
            read_current_line();
        }
    }
}

void MainWidget::open_overview_to_portal(Document* dst_doc, Portal portal){
    dst_doc->open(true);
    dst_doc->load_page_dimensions(true);
    OverviewState overview;
    overview.doc = dst_doc;
    overview.absolute_offset_y = portal.dst.book_state.offset_y;
    overview.absolute_offset_x = portal.dst.book_state.offset_x;
    overview.overview_type = "portal";
    overview.source_portal = portal;
    if (portal.dst.book_state.zoom_level > 0) {
        overview.zoom_level = portal.dst.book_state.zoom_level;
    }

    set_overview_page(overview, true);
}

bool MainWidget::overview_under_pos(WindowPos pos) {

    std::optional<PdfLink> link;
    dv()->smart_view_candidates.clear();
    dv()->index_into_candidates = 0;

    //std::string portal_uuid = -1;
    Portal* portal = main_document_view->get_portal_under_window_pos(pos);
    if (portal) {
        Document* dst_doc = document_manager->get_document_with_checksum(portal->dst.document_checksum);
        if (dst_doc) {
            main_document_view->set_selected_portal_uuid(portal->uuid);

            open_overview_to_portal(dst_doc, *portal);

            invalidate_render();
            return true;
        }
        else if (sioyek_network_manager->is_checksum_available_on_server(portal->dst.document_checksum)) { // check if the document is available on server
            sioyek_network_manager->download_file_with_hash(this, QString::fromStdString(portal->dst.document_checksum),
                [this, portal_v=*portal](QString path) {
                Document* downloaded_dst_doc = document_manager->get_document(path.toStdWString());
                if (downloaded_dst_doc) {
                    main_document_view->set_selected_portal_uuid(portal_v.uuid);
                    open_overview_to_portal(downloaded_dst_doc, portal_v);
                    invalidate_render();
                }
                });
            return true;
        }

    }

    if (main_document_view && (link = main_document_view->get_link_in_pos(pos))) {
        if (QString::fromStdString(link.value().uri).startsWith("http")) {
            // can't open overview to web links
            return false;
        }
        else {
            dv()->set_overview_link(link.value());
            on_overview_source_updated();
            //main_document_view->fit_overview_width();
            return true;
        }
    }

    DocumentPos docpos = main_document_view->window_to_document_pos(pos);

    TextUnderPointerInfo reference_info = dv()->find_location_of_text_under_pointer(docpos);
    if ((reference_info.candidates.size() > 0) && (reference_info.candidates[0].reference_type != ReferenceType::None)) {
        int pos_page = main_document_view->window_to_document_pos(pos).page;

        main_document_view->smart_view_candidates = reference_info.candidates;
        DocumentPos first_candid_pos = reference_info.candidates[0].get_docpos(main_document_view);

        dv()->set_overview_position(
            first_candid_pos.page,
            first_candid_pos.y,
            reference_type_string(reference_info.candidates[0].reference_type),
            reference_info.candidates[0].get_highlight_rects()
        );
        on_overview_source_updated();
        return true;
    }

    return false;
}

void MainWidget::set_synctex_mode(bool mode) {
    if (mode) {
        set_overview_page({}, false);
    }
    this->synctex_mode = mode;
}

void MainWidget::toggle_synctex_mode() {
    this->set_synctex_mode(!this->synctex_mode);
}

void MainWidget::start_creating_rect_portal(AbsoluteDocumentPos location) {

    Portal new_portal;
    new_portal.src_offset_y = location.y;
    new_portal.src_offset_x = location.x;

    main_document_view->set_pending_portal(main_document_view->get_document()->get_path(), new_portal);

    synchronize_pending_link();
    refresh_all_windows();
    validate_render();
}

void MainWidget::handle_portal() {
    if (!main_document_view_has_document()) return;

    if (main_document_view->is_pending_link_source_filled()) {
        auto [source_path, pl] = main_document_view->current_pending_portal.value();
        pl.dst = main_document_view->get_checksum_state();

        if (source_path.has_value()) {
            add_portal(source_path.value(), pl);
        }

        main_document_view->set_pending_portal({});
    }
    else {
        // if an overview is opened, add the overview as a pinned portal
        if (dv()->get_overview_page().has_value()){
            pin_current_overview_as_portal();
            close_overview();
        }
        else{
            main_document_view->set_pending_portal(main_document_view->get_document()->get_path(),
                Portal::with_src_offset(main_document_view->get_offset_y()));
        }
    }

    synchronize_pending_link();
    refresh_all_windows();
    validate_render();
}

void MainWidget::handle_pending_text_command(std::wstring text) {
    if (pending_command_instance) {
        pending_command_instance->set_text_requirement(text);
        advance_command(std::move(pending_command_instance));
    }
}

void MainWidget::toggle_fullscreen() {
    if (isFullScreen()) {
        if (is_helper_visible()){
            helper_opengl_widget()->setWindowState(Qt::WindowState::WindowMaximized);
        }
        setWindowState(Qt::WindowState::WindowMaximized);
    }
    else {
        if (is_helper_visible()){
            helper_opengl_widget()->setWindowState(Qt::WindowState::WindowFullScreen);
        }
        setWindowState(Qt::WindowState::WindowFullScreen);
    }
}


void MainWidget::complete_pending_link(const PortalViewState& destination_view_state) {
    Portal& pl = main_document_view->current_pending_portal.value().second;
    pl.dst = destination_view_state;
    std::string uuid = doc()->add_portal(pl);
    on_new_portal_added(uuid);

    main_document_view->set_pending_portal({});
}

void MainWidget::long_jump_to_destination(int page, float offset_y) {
    long_jump_to_destination({ page, main_document_view->get_offset_x(), offset_y });
}


void MainWidget::long_jump_to_destination(float abs_offset_y) {

    auto [page, _, offset_y] = main_document_view->get_document()->absolute_to_page_pos(
        { main_document_view->get_offset_x(), abs_offset_y });
    long_jump_to_destination(page, offset_y);
}

void MainWidget::long_jump_to_destination(DocumentPos pos) {
    AbsoluteDocumentPos abs_pos = pos.to_absolute(doc());

    if (ALIGN_LINK_DEST_TO_TOP) {
        abs_pos.y += get_align_to_top_offset();

    }

    if (!main_document_view->is_pending_link_source_filled()) {
        push_state();
        main_document_view->set_offsets(pos.x, abs_pos.y);
        //main_document_view->goto_offset_within_page({ pos.page, pos.x, pos.y });
    }
    else {
        // if we press the link button and then click on a pdf link, we automatically link to the
        // link's destination


        PortalViewState dest_state;
        dest_state.document_checksum = main_document_view->get_document()->get_checksum();
        dest_state.book_state.offset_x = abs_pos.x;
        dest_state.book_state.offset_y = abs_pos.y;
        dest_state.book_state.zoom_level = main_document_view->get_zoom_level();

        complete_pending_link(dest_state);
    }
    invalidate_render();
}

void MainWidget::set_current_widget(QWidget* new_widget) {
    for (auto widget : current_widget_stack) {
        widget->hide();
        widget->deleteLater();
    }
    current_widget_stack.clear();
    current_widget_stack.push_back(new_widget);
    
    if (!TOUCH_MODE) {
        if (new_widget) {
            new_widget->stackUnder(status_label);
            if (menu_bar) {
                menu_bar->stackUnder(new_widget);
            }
        }
    }
    //if (current_widget != nullptr) {
    //    current_widget->hide();
    //    garbage_widgets.push_back(current_widget);
    //}
    //current_widget = new_widget;

    //if (garbage_widgets.size() > 2) {
    //    delete garbage_widgets[0];
    //    garbage_widgets.erase(garbage_widgets.begin());
    //}
}

void MainWidget::push_current_widget(QWidget* new_widget) {
    if (current_widget_stack.size() > 0) {
        current_widget_stack.back()->hide();
    }
    current_widget_stack.push_back(new_widget);
}

bool MainWidget::pop_current_widget(bool canceled) {

    bool popped = false;

    if (current_widget_stack.size() > 0) {
        // if (dynamic_cast<AudioUI*>(current_widget_stack.back())){
        //     if (!is_reading){
        //         stop_tts_service();
        //     }
        // }
        current_widget_stack.back()->hide();
        current_widget_stack.back()->deleteLater();
        current_widget_stack.pop_back();
        popped = true;
    }
    if (current_widget_stack.size() > 0) {
        current_widget_stack.back()->show();
    }
    else {
        setFocus();
    }
    return popped;
}


void MainWidget::toggle_visual_scroll_mode() {
    visual_scroll_mode = !visual_scroll_mode;
}

std::optional<std::wstring> MainWidget::get_current_file_name() {
    if (main_document_view) {
        if (main_document_view->get_document()) {
            return main_document_view->get_document()->get_path();
        }
    }
    return {};
}

CommandManager* MainWidget::get_command_manager() {
    return command_manager;
}

void MainWidget::toggle_dark_mode() {
    if (COLOR_MODE == ColorMode::Dark) {
        COLOR_MODE = ColorMode::Light;
    }
    else {
        COLOR_MODE = ColorMode::Dark;
    }

    //main_document_view->toggle_dark_mode();
    ensure_titlebar_colors_match_color_mode();

    if (helper_opengl_widget_) {
        //helper_document_view_->toggle_dark_mode();
        helper_opengl_widget_->update();
    }
    config_manager->handle_set_color_palette(this, main_document_view->get_current_color_mode());
}

void MainWidget::toggle_custom_color_mode() {
    if (COLOR_MODE == ColorMode::Custom) {
        COLOR_MODE = ColorMode::Light;
    }
    else {
        COLOR_MODE = ColorMode::Custom;
    }
    //main_document_view->toggle_custom_color_mode();
    ensure_titlebar_colors_match_color_mode();

    if (helper_opengl_widget_) {
        //helper_document_view_->toggle_custom_color_mode();
        helper_opengl_widget_->update();
    }
    config_manager->handle_set_color_palette(this, main_document_view->get_current_color_mode());
}


void MainWidget::execute_command(std::wstring command, std::wstring text, bool wait) {
    qDebug() << "executing command: " << QString::fromStdWString(command);

    std::wstring file_path = main_document_view->get_document()->get_path();
    QString qfile_path = QString::fromStdWString(file_path);
    std::vector<std::wstring> path_parts;
    split_path(file_path, path_parts);
    std::wstring file_name = path_parts.back();
    QString qfile_name = QString::fromStdWString(file_name);

    QString qtext = QString::fromStdWString(command);

    qtext.arg(qfile_path);

#ifdef SIOYEK_QT6
    QStringList command_parts_ = qtext.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
#else
    QStringList command_parts_ = qtext.split(QRegExp("\\s+"), QString::SkipEmptyParts);
#endif

    QStringList command_parts;
    while (command_parts_.size() > 0) {
        if ((command_parts_.size() <= 1) || (!command_parts_.at(0).endsWith("\\"))) {
            command_parts.append(command_parts_.at(0));
            command_parts_.pop_front();
        }
        else {
            QString first_part = command_parts_.at(0);
            QString second_part = command_parts_.at(1);
            QString new_command_part = first_part.left(first_part.size() - 1) + " " + second_part;
            command_parts_.pop_front();
            command_parts_.replace(0, new_command_part);
        }
    }
    if (command_parts.size() > 0) {
        QString command_name = command_parts[0];
        QStringList command_args;

        command_parts.takeFirst();

        QPoint mouse_pos_ = mapFromGlobal(cursor_pos());
        WindowPos mouse_pos = { mouse_pos_.x(), mouse_pos_.y() };
        DocumentPos mouse_pos_document = main_document_view->window_to_document_pos(mouse_pos);

        for (int i = 0; i < command_parts.size(); i++) {
            // lagacy number macros, now replaced with names ones
            command_parts[i].replace("%1", qfile_path);
            command_parts[i].replace("%2", qfile_name);
            command_parts[i].replace("%3", QString::fromStdWString(main_document_view->get_selected_text()));
            command_parts[i].replace("%4", QString::number(get_current_page_number()));
            command_parts[i].replace("%5", QString::fromStdWString(text));

            // new named macros
            command_parts[i].replace("%{file_path}", qfile_path);
            command_parts[i].replace("%{file_name}", qfile_name);
            command_parts[i].replace("%{selected_text}", QString::fromStdWString(main_document_view->get_selected_text()));
            std::wstring current_selected_text = main_document_view->get_selected_text();

            if (current_selected_text.size() > 0) {
                auto selection_begin_document = main_document_view->get_document()->absolute_to_page_pos(dv()->selection_begin);
                command_parts[i].replace("%{selection_begin_document}",
                    QString::number(selection_begin_document.page) + " " + QString::number(selection_begin_document.x) + " " + QString::number(selection_begin_document.y));
                auto selection_end_document = main_document_view->get_document()->absolute_to_page_pos(dv()->selection_end);
                command_parts[i].replace("%{selection_end_document}",
                    QString::number(selection_end_document.page) + " " + QString::number(selection_end_document.x) + " " + QString::number(selection_end_document.y));
            }
            command_parts[i].replace("%{page_number}", QString::number(get_current_page_number()));
            command_parts[i].replace("%{command_text}", QString::fromStdWString(text));


            command_parts[i].replace("%{mouse_pos_window}", QString::number(mouse_pos.x) + " " + QString::number(mouse_pos.y));
            //command_parts[i].replace("%{mouse_pos_window}", QString("%1 %2").arg(mouse_pos.x, mouse_pos.y));
            command_parts[i].replace("%{mouse_pos_document}", QString::number(mouse_pos_document.page) + " " + QString::number(mouse_pos_document.x) + " " + QString::number(mouse_pos_document.y));
            //command_parts[i].replace("%{mouse_pos_document}", QString("%1 %2 %3").arg(mouse_pos_document.page, mouse_pos_document.x, mouse_pos_document.y));
            if (command_parts[i].indexOf("%{paper_name}") != -1) {
                std::optional<PaperNameWithRects> maybe_paper_name = get_paper_name_under_cursor();
                if (maybe_paper_name) {
                    command_parts[i].replace("%{paper_name}", maybe_paper_name->paper_name);
                }
            }

            command_parts[i].replace("%{sioyek_path}", QCoreApplication::applicationFilePath());
            command_parts[i].replace("%{local_database}", QString::fromStdWString(local_database_file_path.get_path()));
            command_parts[i].replace("%{shared_database}", QString::fromStdWString(global_database_file_path.get_path()));

            int selected_rect_page = -1;
            std::optional<DocumentRect> selected_rect_document = main_document_view->get_selected_rect_document();
            if (selected_rect_document) {
                selected_rect_page = selected_rect_document->page;
                QString format_string = "%1,%2,%3,%4,%5";
                QString rect_string = format_string
                    .arg(QString::number(selected_rect_page))
                    .arg(QString::number(selected_rect_document->rect.x0))
                    .arg(QString::number(selected_rect_document->rect.y0))
                    .arg(QString::number(selected_rect_document->rect.x1))
                    .arg(QString::number(selected_rect_document->rect.y1));
                command_parts[i].replace("%{selected_rect}", rect_string);
            }


            std::wstring selected_line_text;
            if (main_document_view) {
                selected_line_text = main_document_view->get_selected_line_text().value_or(L"");
                command_parts[i].replace("%{zoom_level}", QString::number(main_document_view->get_zoom_level()));
                DocumentPos docpos = main_document_view->get_offsets().to_document(doc());
                command_parts[i].replace("%{offset_x}", QString::number(main_document_view->get_offset_x()));
                command_parts[i].replace("%{offset_y}", QString::number(main_document_view->get_offset_y()));
                command_parts[i].replace("%{offset_x_document}", QString::number(docpos.x));
                command_parts[i].replace("%{offset_y_document}", QString::number(docpos.y));
            }

            if (selected_line_text.size() > 0) {
                command_parts[i].replace("%6", QString::fromStdWString(selected_line_text));
                command_parts[i].replace("%{line_text}", QString::fromStdWString(selected_line_text));
            }

            std::wstring command_parts_ = command_parts[i].toStdWString();
            command_args.push_back(command_parts[i]);
        }

        run_command(command_name.toStdWString(), command_args, wait);
    }

}

void MainWidget::handle_search_paper_name(QString paper_name, bool is_shift_pressed) {
    if (paper_name.size() > 5) {
        char type;
        if (is_shift_pressed) {
            type = SHIFT_MIDDLE_CLICK_SEARCH_ENGINE[0];
        }
        else {
            type = MIDDLE_CLICK_SEARCH_ENGINE[0];
        }
        if ((type >= 'a') && (type <= 'z')) {
            search_custom_engine(paper_name.toStdWString(), SEARCH_URLS[type - 'a']);
        }
    }

}
void MainWidget::move_vertical(float amount) {
    if (main_document_view->on_vertical_scroll()){
        // hide the link/text labels when we move
        hide_command_line_edit();
    }

    if (dv()->is_scratchpad()) {
        dv()->move_document(0, amount);
        validate_render();
        return;
    }

    if (!smooth_scroll_mode) {
        dv()->move_document(0, amount);
        validate_render();
    }
    else {
        smooth_scroll_speed += amount * SMOOTH_SCROLL_SPEED;
        validate_render();
    }
}

void MainWidget::zoom(WindowPos pos, float zoom_factor, bool zoom_in) {
    dv()->last_smart_fit_page = {};
    if (zoom_in) {
        if (WHEEL_ZOOM_ON_CURSOR) {
            dv()->zoom_in_cursor(pos, zoom_factor);
        }
        else {
            dv()->zoom_in(zoom_factor);
        }
    }
    else {
        if (WHEEL_ZOOM_ON_CURSOR) {
            dv()->zoom_out_cursor(pos, zoom_factor);
        }
        else {
            dv()->zoom_out(zoom_factor);
        }
    }
    validate_render();
}

bool MainWidget::move_horizontal(float amount, bool force) {
    if (!horizontal_scroll_locked) {
        bool ret = main_document_view->move_document(amount, 0, force);
        validate_render();
        return ret;
    }
    return true;
}

std::optional<std::string> MainWidget::get_last_opened_file_checksum() {

    std::vector<std::wstring> opened_docs_hashes;
    std::wstring current_checksum = L"";
    if (main_document_view_has_document()) {
        current_checksum = utf8_decode(main_document_view->get_document()->get_checksum());
    }

    db_manager->select_opened_books_path_values(opened_docs_hashes);

    size_t index = 0;
    while (index < opened_docs_hashes.size()) {
        if (opened_docs_hashes[index] == current_checksum) {
            index++;
        }
        else {
            return utf8_encode(opened_docs_hashes[index]);
        }
    }

    return {};
}
void MainWidget::get_window_params_for_one_window_mode(int* main_window_size, int* main_window_move) {
    if (SINGLE_MAIN_WINDOW_SIZE[0] >= 0) {
        main_window_size[0] = SINGLE_MAIN_WINDOW_SIZE[0];
        main_window_size[1] = SINGLE_MAIN_WINDOW_SIZE[1];
        main_window_move[0] = SINGLE_MAIN_WINDOW_MOVE[0];
        main_window_move[1] = SINGLE_MAIN_WINDOW_MOVE[1];
    }
    else {
#ifdef SIOYEK_QT6
        int window_width = QGuiApplication::primaryScreen()->geometry().width();
        int window_height = QGuiApplication::primaryScreen()->geometry().height();
#else
        int window_width = QApplication::desktop()->screenGeometry(0).width();
        int window_height = QApplication::desktop()->screenGeometry(0).height();
#endif

        main_window_size[0] = window_width;
        main_window_size[1] = window_height;
        main_window_move[0] = 0;
        main_window_move[1] = 0;
    }
}
void MainWidget::get_window_params_for_two_window_mode(int* main_window_size, int* main_window_move, int* helper_window_size, int* helper_window_move) {
    if (MAIN_WINDOW_SIZE[0] >= 0) {
        main_window_size[0] = MAIN_WINDOW_SIZE[0];
        main_window_size[1] = MAIN_WINDOW_SIZE[1];
        main_window_move[0] = MAIN_WINDOW_MOVE[0];
        main_window_move[1] = MAIN_WINDOW_MOVE[1];
        helper_window_size[0] = HELPER_WINDOW_SIZE[0];
        helper_window_size[1] = HELPER_WINDOW_SIZE[1];
        helper_window_move[0] = HELPER_WINDOW_MOVE[0];
        helper_window_move[1] = HELPER_WINDOW_MOVE[1];
    }
    else {
#ifdef SIOYEK_QT6
        int num_screens = QGuiApplication::screens().size();
#else
        int num_screens = QApplication::desktop()->numScreens();
#endif
        int main_window_width = get_current_monitor_width();
        int main_window_height = get_current_monitor_height();
        if (num_screens > 1) {
#ifdef SIOYEK_QT6
            int second_window_width = QGuiApplication::screens().at(1)->geometry().width();
            int second_window_height = QGuiApplication::screens().at(1)->geometry().height();
#else
            int second_window_width = QApplication::desktop()->screenGeometry(1).width();
            int second_window_height = QApplication::desktop()->screenGeometry(1).height();
#endif
            main_window_size[0] = main_window_width;
            main_window_size[1] = main_window_height;
            main_window_move[0] = 0;
            main_window_move[1] = 0;
            helper_window_size[0] = second_window_width;
            helper_window_size[1] = second_window_height;
            helper_window_move[0] = main_window_width;
            helper_window_move[1] = 0;
        }
        else {
            main_window_size[0] = main_window_width / 2;
            main_window_size[1] = main_window_height;
            main_window_move[0] = 0;
            main_window_move[1] = 0;
            helper_window_size[0] = main_window_width / 2;
            helper_window_size[1] = main_window_height;
            helper_window_move[0] = main_window_width / 2;
            helper_window_move[1] = 0;
        }
    }
}

void MainWidget::apply_window_params_for_one_window_mode(bool force_resize) {

    QWidget* main_window = get_top_level_widget(opengl_widget);

    int main_window_width = get_current_monitor_width();

    int main_window_size[2];
    int main_window_move[2];

    get_window_params_for_one_window_mode(main_window_size, main_window_move);

    bool should_maximize = main_window_width == main_window_size[0];

    if (should_maximize) {
        main_window->move(main_window_move[0], main_window_move[1]);
        main_window->hide();
        if (force_resize) {
            main_window->resize(main_window_size[0], main_window_size[1]);
        }
        main_window->showMaximized();
    }
    else {
        main_window->move(main_window_move[0], main_window_move[1]);
        main_window->resize(main_window_size[0], main_window_size[1]);
    }


    if (helper_opengl_widget_ != nullptr) {
        helper_opengl_widget_->hide();
        helper_opengl_widget_->move(0, 0);
        helper_opengl_widget_->resize(main_window_size[0], main_window_size[1]);
    }
}

void MainWidget::apply_window_params_for_two_window_mode() {
    QWidget* main_window = get_top_level_widget(opengl_widget);
    QWidget* helper_window = get_top_level_widget(helper_opengl_widget());

#ifdef Q_OS_MACOS
    if (MACOS_HIDE_TITLEBAR) {
        hideWindowTitleBar(helper_window->winId());
    }
#endif
    //int main_window_width = QApplication::desktop()->screenGeometry(0).width();
    int main_window_width = get_current_monitor_width();

    int main_window_size[2];
    int main_window_move[2];
    int helper_window_size[2];
    int helper_window_move[2];

    get_window_params_for_two_window_mode(main_window_size, main_window_move, helper_window_size, helper_window_move);

    bool should_maximize = main_window_width == main_window_size[0];


    if (helper_opengl_widget_ != nullptr) {
        helper_window->move(helper_window_move[0], helper_window_move[1]);
        helper_window->resize(helper_window_size[0], helper_window_size[1]);

        helper_window->show();
    }

    if (should_maximize) {
        main_window->hide();
        main_window->showMaximized();
    }
    else {
        main_window->move(main_window_move[0], main_window_move[1]);
        main_window->resize(main_window_size[0], main_window_size[1]);
    }
}

QRect MainWidget::get_main_window_rect() {
    QPoint main_window_pos = pos();
    QSize main_window_size = size();
    return QRect(main_window_pos, main_window_size);
}

QRect MainWidget::get_helper_window_rect() {
    QPoint helper_window_pos = helper_opengl_widget_->pos();
    QSize helper_window_size = helper_opengl_widget_->size();
    return QRect(helper_window_pos, helper_window_size);
}

void MainWidget::open_document(const std::wstring& doc_path,
    bool load_prev_state,
    std::optional<OpenedBookState> prev_state,
    bool force_load_dimensions) {
    opengl_widget->clear_all_selections();

    if (main_document_view) {
        main_document_view->persist();
    }
    on_open_document(doc_path);

    main_document_view->open_document(doc_path, load_prev_state, prev_state, force_load_dimensions);

    if (doc()) {
        document_manager->add_tab(doc()->get_path());
        //doc()->set_only_for_portal(false);
    }

    std::optional<std::wstring> filename = Path(doc_path).filename();
    if (filename) {
        setWindowTitle(QString::fromStdWString(filename.value()));
    }

    if (SCROLLBAR) {
        update_scrollbar();
    }
}

// #ifndef Q_OS_MACOS
void MainWidget::dragEnterEvent(QDragEnterEvent* e)
{
    e->acceptProposedAction();

}

void MainWidget::dragMoveEvent(QDragMoveEvent* e)
{
    e->acceptProposedAction();

}

void MainWidget::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        auto urls = event->mimeData()->urls();
        std::wstring path = urls.at(0).toLocalFile().toStdWString();
        // ignore file:/// at the beginning of the URL
//#ifdef Q_OS_WIN
//        path = path.substr(8, path.size() - 8);
//#else
//        path = path.substr(7, path.size() - 7);
//#endif
        //handle_args(QStringList() << QApplication::applicationFilePath() << QString::fromStdWString(path));
        push_state();
        open_document(path, &is_render_invalidated);
    }
}
// #endif


bool MainWidget::is_rotated() {
    return main_document_view->is_rotated();
}

void MainWidget::show_password_prompt_if_required() {
    if (main_document_view && (main_document_view->get_document() != nullptr)) {
        if (main_document_view->get_document()->needs_authentication()) {
            if ((pending_command_instance == nullptr) || (pending_command_instance->get_name() != "enter_password")) {
                handle_command_types(command_manager->get_command_with_name(this, "enter_password"), 1);
            }
        }
    }
}

void MainWidget::on_new_paper_added(const std::wstring& file_path) {
    if (main_document_view->is_pending_link_source_filled()) {
        PortalViewState dst_view_state;

        dst_view_state.book_state.offset_x = 0;
        dst_view_state.book_state.offset_y = 0;
        dst_view_state.book_state.zoom_level = 1;
        Document* new_doc = document_manager->get_document(file_path);
        new_doc->open(false, "", true);
        PagelessDocumentRect first_page_rect = new_doc->get_page_rect_no_cache(0);
        document_manager->free_document(new_doc);

        dst_view_state.document_checksum = checksummer->get_checksum(file_path);

        if (helper_document_view_) {
            float helper_view_width = helper_document_view_->get_view_width();
            float helper_view_height = helper_document_view_->get_view_height();
            float zoom_level = helper_view_width / first_page_rect.width();
            dst_view_state.book_state.zoom_level = zoom_level;
            dst_view_state.book_state.offset_y = -std::abs(-helper_view_height / zoom_level / 2 + first_page_rect.height() / 2);
        }
        complete_pending_link(dst_view_state);
        invalidate_render();
    }
}
void MainWidget::handle_link_click(const PdfLink& link) {
    if (link.uri.substr(0, 4).compare("http") == 0) {
        open_web_url(utf8_decode(link.uri));
        return;
    }

    if (link.uri.substr(0, 4).compare("file") == 0) {
        QString path_uri;
        if (link.uri.substr(0, 7) == "file://") {
            path_uri = QString::fromStdString(link.uri.substr(7, link.uri.size() - 7)); // skip file://
        }
        else {
            path_uri = QString::fromStdString(link.uri.substr(5, link.uri.size() - 5)); // skip file:
        }
        auto parts = path_uri.split('#');
        std::wstring path_part = parts.at(0).toStdWString();
        auto docpath = doc()->get_path();
        Path linked_file_path = Path(doc()->get_path()).file_parent().slash(path_part);
        int page = 0;
        if (parts.size() > 1) {
            if (parts.at(1).startsWith("nameddest")) {
                QString standard_uri = QString::fromStdString(link.uri);
                if (standard_uri.startsWith("file:") && !(standard_uri.startsWith("file://"))) {
                    standard_uri = "file://" + standard_uri.mid(5);
                }

                Document* linked_doc = document_manager->get_document(linked_file_path.get_path());
                if (!linked_doc->doc) {
                    linked_doc->open();
                }

                if (linked_doc && linked_doc->doc) {
                    ParsedUri parsed_uri = parse_uri(mupdf_context, linked_doc->doc, standard_uri.toStdString());
                    page = parsed_uri.page - 1;
                    push_state();
                    open_document_at_location(linked_file_path, page, parsed_uri.x, parsed_uri.y, {});
                    return;
                }
            }
            else {
                std::string page_string = parts.at(1).toStdString();
                page_string = page_string.substr(5, page_string.size() - 5);
                page = QString::fromStdString(page_string).toInt() - 1;
            }
        }
        push_state();
        open_document_at_location(linked_file_path, page, {}, {}, {});
        return;
    }

    auto [page, offset_x, offset_y] = parse_uri(mupdf_context, doc()->doc, link.uri);

    // convert one indexed page to zero indexed page
    page--;

    if (main_document_view->is_presentation_mode()) {
        goto_page_with_page_number(page);
    }
    else {
        handle_goto_link_with_page_and_offset(page, offset_y);
    }
}

void MainWidget::save_auto_config() {
    std::wofstream outfile(auto_config_path.get_path_utf8());
    outfile << get_serialized_configuration_string();
#ifndef SIOYEK_MOBILE
    outfile << L"\n";
    config_manager->serialize_auto_configs(outfile);
#endif
    outfile.close();
}

std::wstring MainWidget::get_serialized_configuration_string() {
    float overview_size[2];
    float overview_offset[2];
    main_document_view->get_overview_offsets(&overview_offset[0], &overview_offset[1]);
    main_document_view->get_overview_size(&overview_size[0], &overview_size[1]);

    QString overview_config = "overview_size %1 %2\noverview_offset %3 %4\n";
    std::wstring overview_config_string = overview_config.arg(QString::number(overview_size[0]),
        QString::number(overview_size[1]),
        QString::number(overview_offset[0]),
        QString::number(overview_offset[1])).toStdWString();
    return overview_config_string + get_window_configuration_string();
}
std::wstring MainWidget::get_window_configuration_string() {

    QString config_string_multi = "main_window_size    %1 %2\nmain_window_move     %3 %4\nhelper_window_size    %5 %6\nhelper_window_move     %7 %8";
    QString config_string_single = "single_main_window_size    %1 %2\nsingle_main_window_move     %3 %4";

    QString main_window_size_w = QString::number(size().width());
    QString main_window_size_h = QString::number(size().height());
    QString helper_window_size_w = QString::number(-1);
    QString helper_window_size_h = QString::number(-1);
    QString main_window_move_x = QString::number(pos().x());
    QString main_window_move_y = QString::number(pos().y());
    QString helper_window_move_x = QString::number(-1);
    QString helper_window_move_y = QString::number(-1);

    if (is_helper_visible()) {
        helper_window_size_w = QString::number(helper_opengl_widget_->size().width());
        helper_window_size_h = QString::number(helper_opengl_widget_->size().height());
        helper_window_move_x = QString::number(helper_opengl_widget_->pos().x());
        helper_window_move_y = QString::number(helper_opengl_widget_->pos().y());
        return (config_string_multi.arg(main_window_size_w,
            main_window_size_h,
            main_window_move_x,
            main_window_move_y,
            helper_window_size_w,
            helper_window_size_h,
            helper_window_move_x,
            helper_window_move_y).toStdWString());
    }
    else {
        return (config_string_single.arg(main_window_size_w,
            main_window_size_h,
            main_window_move_x,
            main_window_move_y).toStdWString());
    }
}

void MainWidget::upload_drawings(bool wait_for_send) {
    std::optional<std::string> checksum = doc()->get_checksum_fast();
    if (checksum) {
        QNetworkReply* reply = sioyek_network_manager->upload_drawings(this, checksum.value(), doc()->get_drawings_file_path(), []() {

            });
        if (reply && wait_for_send) {
            block_for_send(reply);
        }
    }
}

void MainWidget::perform_sync_operations_when_document_is_closed(bool wait_for_send, bool sync_drawings) {
    if (is_logged_in() && doc() && doc()->get_is_synced()) {
        sync_current_file_location_to_servers(wait_for_send);
        if (sync_drawings) {
            upload_drawings(wait_for_send);
        }
    }
}

void MainWidget::handle_close_event() {

    bool should_sync_drawings = false;
    if (doc()){
        should_sync_drawings = doc()->get_drawings_are_dirty();
    }

    save_auto_config();
#ifndef SIOYEK_ANDROID
    persist(true);
#endif

    perform_sync_operations_when_document_is_closed(true, should_sync_drawings);


    // we need to delete this here (instead of destructor) to ensure that application
    // closes immediately after the main window is closed
    if (helper_opengl_widget_){
        delete helper_opengl_widget_;
        helper_opengl_widget_ = nullptr;
    }
}

Document* MainWidget::doc() {
    return main_document_view->get_document();
}

void MainWidget::return_to_last_visual_mark() {
    main_document_view->goto_vertical_line_pos();
    //opengl_widget->set_should_draw_vertical_line(true);
    pending_command_instance = nullptr;
    validate_render();
}

void MainWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMaximized()) {
            //int width = size().width();
            //int height = size().height();
            //main_window_width = get_current_monitor_width();
            //main_window_height = get_current_monitor_height();
        }
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (event->type() == QEvent::ThemeChange) {
      set_color_mode_to_system_theme();
    }
#endif

    QWidget::changeEvent(event);
}

void MainWidget::move_ruler_next(){
    if (FORCE_CUSTOM_LINE_ALGORITHM) {
        move_visual_mark(1);
        return;
    }
    main_document_view->move_visual_mark_next();
    if (is_reading || high_quality_play_state.has_value()) {
        read_current_line();
    }
}

void MainWidget::move_ruler_prev(){
    if (FORCE_CUSTOM_LINE_ALGORITHM) {
        move_visual_mark(-1);
        return;
    }
    main_document_view->move_visual_mark_prev();
    if (is_reading || high_quality_play_state.has_value()) {
        read_current_line();
    }
}

AbsoluteRect MainWidget::move_visual_mark(int offset) {
    if ((!main_document_view->is_ruler_mode()) || (main_document_view->get_overview_page().has_value())){
        dv()->handle_vertical_move(offset);
        return fz_empty_rect;
    }
    else{
        AbsoluteRect ruler_rect = main_document_view->move_visual_mark(offset);

        if (is_reading || high_quality_play_state.has_value()) {
            read_current_line();
        }
        if (AUTOCENTER_VISUAL_SCROLL) {
            return_to_last_visual_mark();
        }
        main_document_view->clear_underline();
        return ruler_rect;
    }
}


std::wstring MainWidget::get_current_page_label() {
    return doc()->get_page_label(main_document_view->get_center_page_number());
}

int MainWidget::get_current_page_number() const {
    return main_document_view->get_current_page_number();
}

void MainWidget::set_inverse_search_command(const std::wstring& new_command) {
    inverse_search_command = new_command;
}

void MainWidget::focusInEvent(QFocusEvent* ev) {
    int index = -1;
    for (size_t i = 0; i < windows.size(); i++) {
        if (windows[i] == this) {
            index = i;
            break;
        }
    }
    if (index > 0) {
        std::swap(windows[0], windows[index]);
    }
}

void MainWidget::toggle_statusbar() {
    execute_macro_if_enabled(L"toggleconfig_statusbar");
}

void MainWidget::toggle_titlebar() {

    Qt::WindowFlags flags = windowFlags();
    if (flags.testFlag(Qt::WindowTitleHint)) {
        flags |= Qt::CustomizeWindowHint;
        flags &= ~Qt::WindowContextHelpButtonHint;
        flags &= ~Qt::WindowSystemMenuHint;
        flags &= ~Qt::WindowMinMaxButtonsHint;
        flags &= ~Qt::WindowCloseButtonHint;
        flags &= ~Qt::WindowTitleHint;
        setWindowFlags(flags);

    }
    else {
        flags &= ~Qt::CustomizeWindowHint;
        flags |= Qt::WindowContextHelpButtonHint;
        flags |= Qt::WindowSystemMenuHint;
        flags |= Qt::WindowMinMaxButtonsHint;
        flags |= Qt::WindowCloseButtonHint;
        flags |= Qt::WindowTitleHint;
        setWindowFlags(flags);
    }
    show();
}



int MainWidget::get_current_monitor_width() {
    if (this->window()->windowHandle() != nullptr) {
        return this->window()->windowHandle()->screen()->geometry().width();
    }
    else {
#ifdef SIOYEK_QT6
        return QGuiApplication::primaryScreen()->geometry().width();
#else
        return QApplication::desktop()->screenGeometry(0).width();
#endif
    }
}

int MainWidget::get_current_monitor_height() {
    if (this->window()->windowHandle() != nullptr) {
        return this->window()->windowHandle()->screen()->geometry().height();
    }
    else {
#ifdef SIOYEK_QT6
        return QGuiApplication::primaryScreen()->geometry().height();
#else
        return QApplication::desktop()->screenGeometry(0).height();
#endif
    }
}

void MainWidget::reload(bool flush) {
    pdf_renderer->delete_old_pages(flush, true);
    if (doc()) {
        doc()->reload();
    }
}


std::wstring MainWidget::synctex_under_pos(WindowPos position) {
    std::wstring res = L"";
#ifndef SIOYEK_MOBILE
    auto [page, doc_x, doc_y] = main_document_view->window_to_document_pos(position);
    std::wstring docpath = main_document_view->get_document()->get_path();
    std::string docpath_utf8 = utf8_encode(docpath);
    synctex_scanner_p scanner = synctex_scanner_new_with_output_file(docpath_utf8.c_str(), nullptr, 1);

    int stat = synctex_edit_query(scanner, page + 1, doc_x, doc_y);

    if (stat > 0) {
        synctex_node_p node;
        while ((node = synctex_scanner_next_result(scanner))) {
            int line = synctex_node_line(node);
            int column = synctex_node_column(node);
            if (column < 0) column = 0;
            int tag = synctex_node_tag(node);
            const char* file_name = synctex_scanner_get_name(scanner, tag);
            QString new_path;
#ifdef Q_OS_WIN
            // the path returned by synctex is formatted in unix style, for example it is something like this
            // in windows: d:/some/path/file.pdf
            // this doesn't work with Vimtex for some reason, so here we have to convert the path separators
            // to windows style and make sure the driver letter is capitalized
            QDir file_path = QDir(file_name);
            new_path = QDir::toNativeSeparators(file_path.absolutePath());
            new_path[0] = new_path[0].toUpper();
            if (VIMTEX_WSL_FIX) {
                new_path = file_name;
            }

#else
            new_path = file_name;
#endif

            std::string line_string = std::to_string(line);
            std::string column_string = std::to_string(column);

            if (inverse_search_command.size() > 0) {
#ifdef Q_OS_WIN
                QString command = QString::fromStdWString(inverse_search_command).arg(new_path, line_string.c_str(), column_string.c_str());
#else
                QString command = QString::fromStdWString(inverse_search_command).arg(file_name, line_string.c_str(), column_string.c_str());
#endif
                QStringList args = QProcess::splitCommand(command);
                QProcess::startDetached(args[0], args.mid(1));
            }
            else {
                show_error_message(L"inverse_search_command is not set in prefs_user.config");
            }

        }

    }
    synctex_scanner_free(scanner);

#endif
    return res;
}

void MainWidget::set_status_message(std::wstring new_status_string) {
    custom_status_message = new_status_string;
}

void MainWidget::remove_self_from_windows() {
    for (size_t i = 0; i < windows.size(); i++) {
        if (windows[i] == this) {
            windows.erase(windows.begin() + i);
            break;
        }
    }
}



void MainWidget::add_portal(std::wstring source_path, Portal new_link) {
    if (source_path == main_document_view->get_document()->get_path()) {
        std::string uuid = main_document_view->get_document()->add_portal(new_link);
        on_new_portal_added(uuid);
    }
    else if (document_manager->get_cached_document(source_path)){
        // if the source of the portal is not the current document
        // we should add it to the loaded document if exists
        Document* source_doc = document_manager->get_cached_document(source_path).value();
        source_doc->add_portal(new_link);
        on_new_portal_added(new_link.uuid);
    }
    else {
        //const std::unordered_map<std::wstring, Document*> cached_documents = document_manager->get_cached_documents();
        Document* doc = document_manager->get_document(source_path);
        std::string uuid = doc->add_portal(new_link, false);
        on_new_portal_added(uuid);

        if (new_link.is_visible()) {
            std::string uuid = utf8_encode(new_uuid());
            bool success = db_manager->insert_visible_portal(checksummer->get_checksum(source_path),
                new_link.dst.document_checksum,
                new_link.dst.book_state.offset_x,
                new_link.dst.book_state.offset_y,
                new_link.dst.book_state.zoom_level,
                new_link.src_offset_x.value(),
                new_link.src_offset_y,
                utf8_decode(uuid));
            if (success) {
                on_new_portal_added(uuid);
            }
        }
        else {
            std::string uuid = utf8_encode(new_uuid());
            bool success = db_manager->insert_portal(checksummer->get_checksum(source_path),
                new_link.dst.document_checksum,
                new_link.dst.book_state.offset_x,
                new_link.dst.book_state.offset_y,
                new_link.dst.book_state.zoom_level,
                new_link.src_offset_y,
                utf8_decode(uuid));
            if (success) {
                on_new_portal_added(uuid);
            }
        }
    }
}

void MainWidget::toggle_scrollbar() {
    execute_macro_if_enabled(L"toggleconfig_scrollbar");
}

void MainWidget::update_scrollbar() {
    if (main_document_view_has_document()) {
        
        if (doc()->num_pages() > 0){
            scroll_bar->setSingleStep(std::max(MAX_SCROLLBAR / doc()->num_pages() / 10, 1));
            scroll_bar->setPageStep(MAX_SCROLLBAR / doc()->num_pages());
        }
        else {
            scroll_bar->setSingleStep(1);
            scroll_bar->setPageStep(10);
        }

        float offset = main_document_view->get_offset_y();
        int scroll = static_cast<int>(MAX_SCROLLBAR * offset / doc()->max_y_offset());
        scroll_bar->setValue(scroll);
    }
}


void MainWidget::goto_overview() {
    if (main_document_view->get_overview_page()) {
        OverviewState overview = main_document_view->get_overview_page().value();
        if (overview.doc != nullptr && (overview.doc != doc())) {
            std::optional<Portal> closest_link_ = main_document_view->get_target_portal(false);
            if (closest_link_) {
                push_state();
                open_document(closest_link_.value().dst);
            }
        }
        else {
            std::optional<DocumentPos> maybe_overview_position = main_document_view->get_overview_position();
            if (maybe_overview_position.has_value()) {
                long_jump_to_destination(maybe_overview_position->page, maybe_overview_position->y);
            }
        }
        set_overview_page({}, false);

    }
}


void MainWidget::reset_highlight_links() {
    if (SHOULD_HIGHLIGHT_LINKS) {
        main_document_view->set_highlight_links(true, false);
    }
    else {
        main_document_view->set_highlight_links(false, false);
    }
}

void MainWidget::set_rect_select_mode(bool mode) {
    if (!USE_KEYBOARD_POINT_SELECTION) {
        rect_select_mode = mode;
    }
    if (draw_controls_) {
        if (mode) {
            draw_controls_->hide();
        }
        else {
            draw_controls_->show();
        }
    }
    if (mode == true) {
        if (USE_KEYBOARD_POINT_SELECTION) {
            handle_command_types(std::make_unique<KeyboardSelectPointCommand>(this, std::move(pending_command_instance)), 0);
        }
        else {
            main_document_view->set_selected_rectangle(AbsoluteRect());
        }
    }
    invalidate_render();
}

void MainWidget::set_point_select_mode(bool mode) {

    point_select_mode = mode;
    if (mode == true) {
        if (USE_KEYBOARD_POINT_SELECTION) {
            handle_command_types(std::make_unique<KeyboardSelectPointCommand>(this, std::move(pending_command_instance)), 0);
        }
        else {
            main_document_view->set_selected_rectangle(AbsoluteRect());
        }
    }
}

void MainWidget::clear_selected_rect() {
    main_document_view->clear_selected_rectangle();
    //rect_select_mode = false;
    //rect_select_begin = {};
    //rect_select_end = {};
}

bool CharacterAddress::backspace() {
    if (previous_character) {
        CharacterAddress& prev = *previous_character;

        this->page = prev.page;
        this->block = prev.block;
        this->line = prev.line;
        this->doc = prev.doc;
        this->character = prev.character;
        delete previous_character;
        this->previous_character = nullptr;
        return false;
    }
    else {
        return false;
    }
}

bool CharacterAddress::advance(char c) {
    if (!previous_character) {
        if (character->c == c) {
            return next_char();
        }
        else {
            previous_character = new CharacterAddress();
            previous_character->page = page;
            previous_character->block = block;
            previous_character->line = line;
            previous_character->doc = doc;
            previous_character->character = character;
            next_char();
            return false;
        }
    }
    return false;
}

bool CharacterAddress::next_char() {
    if (character->next) {
        character = character->next;
        return false;
    }
    else {
        next_line();
        return true;
    }
}

bool CharacterAddress::next_line() {
    if (line->next) {
        line = line->next;
        character = line->first_char;
        return false;
    }
    else {
        next_block();
        return true;
    }
}

bool CharacterAddress::next_block() {
    if (block->next) {
        block = block->next;
        line = block->u.t.first_line;
        character = line->first_char;
        return false;
    }
    else {
        next_page();
        return true;
    }
}

bool CharacterAddress::next_page() {
    if (page < doc->num_pages() - 1) {
        page = page + 1;
        block = doc->get_stext_with_page_number(page)->first_block;
        line = block->u.t.first_line;
        character = line->first_char;
        return true;
    }
    return false;
}

float CharacterAddress::focus_offset() {
    return doc->document_to_absolute_y(page, character->quad.ll.y);
}

void MainWidget::set_mark_in_current_location(char symbol) {
    // it is a global mark, we delete other marks with the same symbol from database and add the new mark
    std::string uuid;
    if (isupper(symbol)) {
        db_manager->delete_mark_with_symbol(symbol);
        // we should also delete the cached marks
        document_manager->delete_global_mark(symbol);
        uuid = main_document_view->add_mark(symbol);
    }
    else {
        uuid = main_document_view->add_mark(symbol);
        validate_render();
    }
    on_mark_added(uuid, symbol);
}

void MainWidget::goto_mark(char symbol) {
    if (symbol == '`' || symbol == '\'') {
        return_to_last_visual_mark();
    }
    else if (isupper(symbol)) { // global mark
        std::vector<std::pair<std::string, float>> mark_vector;
        db_manager->select_global_mark(symbol, mark_vector);
        if (mark_vector.size() > 0) {
            assert(mark_vector.size() == 1); // we can not have more than one global mark with the same name
            std::wstring doc_path = checksummer->get_path(mark_vector[0].first).value();
            open_document(doc_path, 0.0f, mark_vector[0].second);
        }

    }
    else {
        main_document_view->goto_mark(symbol);
    }
}

void MainWidget::advance_command(std::unique_ptr<Command> new_command, std::wstring* result) {
    if (new_command) {
        if (!new_command->next_requirement(this).has_value()) {
            new_command->run();
            if (result) {
                std::optional<std::wstring> command_result = new_command->get_result();
                if (command_result.has_value() && (result != nullptr)) {
                    *result = command_result.value();
                }
                //*result = new_command->get_result()
            }
            set_last_performed_command(std::move(new_command));
        }
        else {
            pending_command_instance = std::move(new_command);


            Requirement next_requirement = pending_command_instance->next_requirement(this).value();
            if (next_requirement.type == RequirementType::Text) {
                show_textbar(utf8_decode(next_requirement.name), false, pending_command_instance->get_text_default_value());
            }
            else if (next_requirement.type == RequirementType::Password) {
                show_textbar(utf8_decode(next_requirement.name), true, pending_command_instance->get_text_default_value());
            }
            else if (next_requirement.type == RequirementType::Symbol) {
                invalidate_ui();
                if (TOUCH_MODE) {
                    show_mark_selector();
                }
            }
            else if (next_requirement.type == RequirementType::File || next_requirement.type == RequirementType::Folder) {
                std::wstring file_name;
                if (next_requirement.type == RequirementType::File) {
                    file_name = select_command_file_name(pending_command_instance->get_name());
                }
                else{
                    file_name = select_command_folder_name();
                }
#ifdef SIOYEK_ANDROID

                if (file_name.size() > 0 && QString::fromStdWString(file_name).startsWith("content://")) {
                    if (!QString::fromStdWString(file_name).startsWith("content://com.android.providers.media.documents")){
                        qDebug() << "sioyek: trying to convert " << file_name;
                        file_name = android_file_uri_from_content_uri(QString::fromStdWString(file_name)).toStdWString();
                        qDebug() << "sioyek: result was " << file_name;
                    }
                }
#endif
                if (file_name.size() > 0) {
                    pending_command_instance->set_file_requirement(file_name);
                    advance_command(std::move(pending_command_instance));
                }
            }
            else if (next_requirement.type == RequirementType::Rect) {
                set_rect_select_mode(true);
                validate_render();
            }
            else if (next_requirement.type == RequirementType::Point) {
                set_point_select_mode(true);
                validate_render();
            }
            else if (next_requirement.type == RequirementType::Generic) {
                pending_command_instance->handle_generic_requirement();
            }

            if (pending_command_instance) {
                pending_command_instance->pre_perform();
                invalidate_render();
            }

        }
    }
}

void MainWidget::add_search_term(const std::wstring& term) {
    auto res = std::find(search_terms.begin(), search_terms.end(), term);
    if (res != search_terms.end()) {

        int index = res - search_terms.begin();

        for (int i = index; i < search_terms.size() - 1; i++) {
            std::swap(search_terms[i], search_terms[i + 1]);
        }
    }
    else {
        search_terms.push_back(term);
    }

    if (search_terms.size() > 100) {
        search_terms.pop_front();
    }
}


void MainWidget::perform_search(std::wstring text, bool is_regex, bool is_incremental) {

    if (!is_incremental) {
        add_search_term(text);
        // When searching, the start position before search is saved in a mark named '0'
        main_document_view->add_mark('/');
    }


    int range_begin, range_end;
    std::wstring search_term;
    std::optional<std::pair<int, int>> search_range = {};
    if (parse_search_command(text, &range_begin, &range_end, &search_term)) {
        // we assuem the user-entered range is 1-indexed so we convert it to
        // 0 indexed here
        range_begin -= 1;
        range_end -= 1;
        search_range = std::make_pair(range_begin, range_end);
    }

    if (search_term.size() > 0) {
        // in mupdf RTL documents are reversed, so we reverse the search string
        //todo: better (or any!) handling of mixed RTL and LTR text
        if ((!SUPER_FAST_SEARCH) && is_rtl(search_term[0])) {
            search_term = reverse_wstring(search_term);
        }
    }

    SearchCaseSensitivity case_sens = SearchCaseSensitivity::CaseInsensitive;
    if (CASE_SENSITIVE_SEARCH) case_sens = SearchCaseSensitivity::CaseSensitive;
    if (SMARTCASE_SEARCH) case_sens = SearchCaseSensitivity::SmartCase;
    main_document_view->search_text(pdf_renderer, search_term, case_sens, is_regex, search_range);

    if (is_incremental) {
        goto_search_result(1);
    }
}

void MainWidget::overview_to_definition() {
    if (!main_document_view->get_overview_page()) {
        std::vector<SmartViewCandidate> candidates = main_document_view->find_line_definitions();

        if (candidates.size() > 0) {
            DocumentPos first_docpos = candidates[0].get_docpos(main_document_view);
            AbsoluteDocumentPos first_abspos = first_docpos.to_absolute(doc());
            dv()->smart_view_candidates = candidates;
            dv()->index_into_candidates = 0;
            //dv()->set_overview_position(first_docpos.page, first_docpos.y, reference_type_string(candidates[0].reference_type));
            OverviewState overview_state;
            overview_state.absolute_offset_x = 0;
            overview_state.absolute_offset_y = first_abspos.y;
            overview_state.doc = candidates[0].doc;
            overview_state.highlight_rects = candidates[0].get_highlight_rects();
            overview_state.overview_type = reference_type_string(candidates[0].reference_type);

            set_overview_page(overview_state, true);
            //dv()->set_overview_highlights(candidates[0].highlight_rects);
            on_overview_source_updated();
        }
    }
    else {
        set_overview_page({}, false);
    }
}

void MainWidget::portal_to_definition() {
    std::vector<SmartViewCandidate> defpos = main_document_view->find_line_definitions();
    if (defpos.size() > 0) {
        //AbsoluteDocumentPos abspos = doc()->document_to_absolute_pos(defpos[0].first, true);
        AbsoluteDocumentPos abspos = defpos[0].get_abspos(main_document_view);
        Portal link;
        link.dst.document_checksum = doc()->get_checksum();
        link.dst.book_state.offset_x = abspos.x;
        link.dst.book_state.offset_y = abspos.y;
        link.dst.book_state.zoom_level = main_document_view->get_zoom_level();
        link.src_offset_y = main_document_view->get_ruler_pos();
        std::string uuid = doc()->add_portal(link, true);
        on_new_portal_added(uuid);
    }
}

void MainWidget::move_visual_mark_command(int amount) {
    main_document_view->move_ruler(amount);
    validate_render();
}

void MainWidget::show_current_widget() {
    if (current_widget_stack.size() > 0) {
        current_widget_stack.back()->show();
        // we want to show statusbar in touch mode when other windows are visible
        // so when we show a widget, we need to invalidate the ui so the statusbar
        // is displayed if it is needed
        invalidate_ui();
    }
}

void MainWidget::handle_goto_portal_list() {
    std::vector<std::wstring> option_names;
    std::vector<std::wstring> option_location_strings;
    std::vector<Portal> portals;

    if (!doc()) return;

    if (SORT_BOOKMARKS_BY_LOCATION) {
        portals = main_document_view->get_document()->get_sorted_portals();
    }
    else {
        portals = main_document_view->get_document()->get_portals();
    }

    for (auto portal : portals) {
        std::wstring portal_type_string = L"[*]";
        if (!portal.is_visible()) {
            portal_type_string = L"[.]";
        }

        option_names.push_back(ITEM_LIST_PREFIX + L" " + portal_type_string + L" " +  checksummer->get_path(portal.dst.document_checksum).value_or(L"[ERROR]"));
        //option_locations.push_back(bookmark.y_offset);
        auto [page, _, __] = main_document_view->get_document()->absolute_to_page_pos({ 0, portal.src_offset_y });
        option_location_strings.push_back(get_page_formatted_string(page + 1));
    }

    int closest_portal_index = main_document_view->get_document()->find_closest_portal_index(portals, main_document_view->get_offset_y());

    set_filtered_select_menu<Portal>(this, FUZZY_SEARCHING, MULTILINE_MENUS, { option_names, option_location_strings }, portals, closest_portal_index,
        [&](Portal* portal) {
            pending_command_instance->set_generic_requirement(portal->src_offset_y);
            advance_command(std::move(pending_command_instance));
            pop_current_widget();

        },
        [&](Portal* portal) {
            std::string uuid = portal->uuid;
            std::optional<Portal> deleted_portal = doc()->delete_portal_with_uuid(uuid);
            if (deleted_portal) {
                on_portal_deleted(deleted_portal.value(), doc()->get_checksum());
            }
        },
            [&](Portal* portal) {
                portal_to_edit = *portal;
                open_document(portal->dst);
                pop_current_widget();
                invalidate_render();
        }
        );

    show_current_widget();
}

void MainWidget::handle_goto_bookmark() {
    //std::vector<std::wstring> option_names;
    std::vector<QString> option_location_strings;
    std::vector<BookMark> bookmarks;

    if (!doc()) return;

    if (SORT_BOOKMARKS_BY_LOCATION) {
        bookmarks = main_document_view->get_document()->get_sorted_bookmarks();
    }
    else {
        bookmarks = main_document_view->get_document()->get_bookmarks();
    }

    for (auto bookmark : bookmarks) {
        //option_names.push_back(ITEM_LIST_PREFIX + L" " + bookmark.description);
        //option_locations.push_back(bookmark.y_offset);
        auto [page, _, __] = main_document_view->get_document()->absolute_to_page_pos({ 0, bookmark.get_y_offset()});
        option_location_strings.push_back(QString::fromStdWString(get_page_formatted_string(page + 1)));
    }

    int closest_bookmark_index = main_document_view->get_document()->find_closest_bookmark_index(bookmarks, main_document_view->get_offset_y());

    auto handle_select_fn = [&](BookMark bm) {
        if (pending_command_instance) {
            pending_command_instance->set_generic_requirement(bm.get_y_offset());
        }

        advance_command(std::move(pending_command_instance));
        pop_current_widget();
        };

    auto handle_delete_fn = [&](BookMark bm) {
        delete_current_document_bookmark_with_bookmark(&bm);
        };

    auto handle_edit_fn = [&](BookMark bm) {
        main_document_view->set_selected_bookmark_uuid(bm.uuid);
        pop_current_widget();
        handle_command_types(command_manager->get_command_with_name(this, "edit_selected_bookmark"), 0);
        };

    if (TOUCH_MODE || (!FANCY_UI_MENUS)) {
        std::vector<std::wstring> option_names;
        std::vector<std::wstring> option_location_wstrings;
        //std::vector<BookMark> bookmarks;

        for (auto bookmark : bookmarks) {
            option_names.push_back(ITEM_LIST_PREFIX + L" " + bookmark.description);
            auto [page, _, __] = main_document_view->get_document()->absolute_to_page_pos({ 0, bookmark.get_y_offset() });
            option_location_wstrings.push_back(get_page_formatted_string(page + 1));
        }

        set_filtered_select_menu<BookMark>(this, FUZZY_SEARCHING, MULTILINE_MENUS, { option_names, option_location_wstrings }, bookmarks, closest_bookmark_index,
            [&, handle_select_fn](BookMark* bm) {
                handle_select_fn(*bm);
            },
            [&, handle_delete_fn](BookMark* bm) {
                handle_delete_fn(*bm);
            },
            [&, handle_edit_fn](BookMark* bm) {
                handle_edit_fn(*bm);
            }
        );
        show_current_widget();
    }
    else {
        BookmarkSelectorWidget* bookmark_widget = BookmarkSelectorWidget::from_bookmarks(
            std::move(bookmarks), this, std::move(option_location_strings));
        bookmark_widget->set_selected_index(closest_bookmark_index);

        bookmark_widget->set_select_fn([&, bookmark_widget, handle_select_fn](int index) {
            BookMark bm = bookmark_widget->bookmark_model->bookmarks[index];
            handle_select_fn(bm);

            });

        bookmark_widget->set_delete_fn([&, bookmark_widget, handle_delete_fn](int index) {
            BookMark bm = bookmark_widget->bookmark_model->bookmarks[index];
            handle_delete_fn(bm);
            });

        bookmark_widget->set_edit_fn([&, bookmark_widget, handle_edit_fn](int index) {
            BookMark bm = bookmark_widget->bookmark_model->bookmarks[index];
            handle_edit_fn(bm);
            });

        set_current_widget(bookmark_widget);
        show_current_widget();

    }
}

void MainWidget::handle_goto_bookmark_global() {
    std::vector<std::pair<std::string, BookMark>> global_bookmarks;
    db_manager->global_select_bookmark(global_bookmarks);

    auto handle_select_fn = [&](QString checksum, float offset_y) {
        if (checksum.startsWith("SERVER://")) {
            download_and_open(checksum.mid(9).toStdString(), offset_y);
        }
        else {
            if (pending_command_instance) {
                QString file_path = QString::fromStdWString(document_manager->get_path_from_hash(checksum.toStdString()).value_or(L""));
                pending_command_instance->set_generic_requirement(QList<QVariant>() << file_path << offset_y);
            }
            advance_command(std::move(pending_command_instance));
            pop_current_widget();
        }
        };

    auto handle_delete_fn = [&](std::string uuid) {
        delete_global_bookmark(uuid);
        };


    std::vector<BookMark> bookmarks;
    std::vector<QString> file_names;
    std::vector<QString> file_checksums;

    for (const auto& desc_bm_pair : global_bookmarks) {
        std::string checksum = desc_bm_pair.first;
        bool is_remote = false;
        std::optional<std::wstring> path = checksummer->get_path(checksum);
        if (!path.has_value() && sioyek_network_manager->is_checksum_available_on_server(checksum)) {
            is_remote = true;
            path = L"SERVER://" + utf8_decode(checksum);
        }
        if (path) {
            BookMark bm = desc_bm_pair.second;
            std::wstring file_name = is_remote ? SERVER_SYMBOL  : Path(path.value()).filename().value_or(L"");
            bookmarks.push_back(bm);
            file_names.push_back(QString::fromStdWString(path.value_or(L"")));
            file_checksums.push_back(QString::fromStdString(checksum));
        }
    }
    if (TOUCH_MODE || (!FANCY_UI_MENUS)) {
        std::vector<std::wstring> descs;
        std::vector<std::wstring> file_names_wstring;
        std::vector<std::pair<BookState, std::string>> book_states;
        for (int i = 0; i < bookmarks.size(); i++) {
            descs.push_back(bookmarks[i].description);
            file_names_wstring.push_back(file_names[i].toStdWString());
            BookState book_state;
            book_state.document_path = file_checksums[i].toStdWString();
            book_state.offset_y = bookmarks[i].get_y_offset();
            book_states.push_back(std::make_pair(book_state, bookmarks[i].uuid));
        }

        set_filtered_select_menu<std::pair<BookState, std::string>>(this, FUZZY_SEARCHING, MULTILINE_MENUS, { descs, file_names_wstring }, book_states, -1,
            [&, handle_select_fn](std::pair<BookState, std::string>* book_state) {
                QString path = QString::fromStdWString(book_state->first.document_path);

                handle_select_fn(path, book_state->first.offset_y);

            },
            [&, handle_delete_fn](std::pair<BookState, std::string>* book_state) {
                handle_delete_fn(book_state->second);
            }
        );
        show_current_widget();
    }
    else {
        BookmarkSelectorWidget* bookmark_widget = BookmarkSelectorWidget::from_bookmarks(
            std::move(bookmarks), this, std::move(file_names), std::move(file_checksums)
        );

        bookmark_widget->set_select_fn([&, bookmark_widget, handle_select_fn](int index) {
            QString path = bookmark_widget->bookmark_model->checksums[index];
            BookMark bm = bookmark_widget->bookmark_model->bookmarks[index];
            QString file_path = bookmark_widget->bookmark_model->documents[index];
            handle_select_fn(path, bm.get_y_offset());
            });

        bookmark_widget->set_delete_fn(
            [&, bookmark_widget, handle_delete_fn](int index) {
                BookMark bm = bookmark_widget->bookmark_model->bookmarks[index];
                handle_delete_fn(bm.uuid);
            }
        );

        set_current_widget(bookmark_widget);
        show_current_widget();
    }

    //set_filtered_select_menu<BookState>(this, FUZZY_SEARCHING, MULTILINE_MENUS, { descs, file_names }, book_states, -1,
    //    [&](BookState* book_state) {
    //        QString path = QString::fromStdWString(book_state->document_path);
    //        if (path.startsWith("SERVER://")) {
    //            download_and_open(path.mid(9).toStdString(), book_state->offset_y);
    //        }
    //        else {
    //            if (pending_command_instance) {
    //                pending_command_instance->set_generic_requirement(QList<QVariant>() << QString::fromStdWString(book_state->document_path) << book_state->offset_y);
    //            }
    //            advance_command(std::move(pending_command_instance));
    //        }
    //    },
    //    [&](BookState* book_state) {
    //        delete_global_bookmark(book_state->uuid);
    //    }
    //    );
    //show_current_widget();
}

std::string MainWidget::add_highlight_to_current_document(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type) {
    std::string uuid = main_document_view->add_highlight_(selection_begin, selection_end, type);
    on_new_highlight_added(uuid);
    return uuid;
}

std::wstring MainWidget::handle_add_highlight(char symbol) {
    if (main_document_view->selected_character_rects.size() > 0) {
        std::string uuid = add_highlight_to_current_document(dv()->selection_begin, dv()->selection_end, symbol);
        main_document_view->clear_selected_text();
        return utf8_decode(uuid);
    }
    else {
        change_selected_highlight_type(symbol);
        return utf8_decode(main_document_view->get_selected_highlight_uuid());
    }
}

void MainWidget::change_selected_highlight_type(char new_type) {
    std::string selected_highlight_uuid = main_document_view->get_selected_highlight_uuid();
    if (selected_highlight_uuid.size() > 0) {
        doc()->update_highlight_type(selected_highlight_uuid, new_type);
        on_highlight_type_edited(selected_highlight_uuid);
    }
}

char MainWidget::get_current_selected_highlight_type() {
    std::string selected_highlight_uuid = main_document_view->get_selected_highlight_uuid();
    if (selected_highlight_uuid.size() > 0) {
        Highlight* highlight = doc()->get_highlight_with_uuid(selected_highlight_uuid);
        if (highlight) {
            return highlight->type;
        }
    }
    return 'a';
}

void MainWidget::handle_goto_highlight() {
    std::vector<Highlight> highlights;

    if (SORT_HIGHLIGHTS_BY_LOCATION) {
        highlights = doc()->get_highlights_sorted();
    }
    else {
        highlights = doc()->get_highlights();
    }

    std::vector<QString> page_numbers;
    page_numbers.reserve(highlights.size());

    for (auto hl : highlights) {
        page_numbers.push_back(QString::number(hl.selection_begin.to_document(doc()).page));
    }

    int closest_highlight_index = doc()->find_closest_highlight_index(highlights, main_document_view->get_offset_y());


    auto handle_select_fn = [&](Highlight hl) {
        if (pending_command_instance) {
            pending_command_instance->set_generic_requirement(hl.selection_begin.y);
        }
        advance_command(std::move(pending_command_instance));
        pop_current_widget();
        };

    auto handle_edit_fn = [&](Highlight hl) {
        main_document_view->set_selected_highlight_uuid(hl.uuid);
        pop_current_widget();

        std::unique_ptr<Command> cmd = command_manager->get_command_with_name(this, "edit_selected_highlight");
        cmd->pre_perform();
        advance_command(std::move(cmd));
        };

    auto handle_delete_fn = [&](Highlight hl) {
        delete_current_document_highlight(&hl);
        invalidate_render();
        };

    if (TOUCH_MODE) {
        HighlightModel* highlights_model = new HighlightModel(std::move(highlights), std::move(page_numbers), {}, this);

        TouchDelegateListView* lv = new TouchDelegateListView(highlights_model, true, "TouchHighlightsView", { std::make_pair("_colorMap", get_color_mapping()), std::make_pair("_selected_index", closest_highlight_index)}, this);
        lv->list_view->proxy_model->set_is_highlight(true);
        lv->list_view->proxy_model->setFilterKeyColumn(-1);

        lv->set_select_fn([&, highlights_model, handle_select_fn](int index) {
            Highlight hl = highlights_model->highlights[index];
            handle_select_fn(hl);
            });

        lv->set_delete_fn(
            [&, highlights_model, handle_delete_fn](int index) {
                Highlight hl = highlights_model->highlights[index];
                handle_delete_fn(hl);
            }
        );

        set_current_widget(lv);
        show_current_widget();

    }
    else {
        if (FANCY_UI_MENUS) {
            HighlightSelectorWidget* highlight_selector_widget = HighlightSelectorWidget::from_highlights(std::move(highlights), this, std::move(page_numbers));
            highlight_selector_widget->set_selected_index(closest_highlight_index);

            highlight_selector_widget->set_select_fn(
                [&, highlight_selector_widget, handle_select_fn](int index) {
                    Highlight hl = highlight_selector_widget->highlight_model->highlights[index];
                    handle_select_fn(hl);
                }
            );

            highlight_selector_widget->set_delete_fn(
                [&, highlight_selector_widget, handle_delete_fn](int index) {
                    Highlight hl = highlight_selector_widget->highlight_model->highlights[index];
                    handle_delete_fn(hl);
                }
            );
            highlight_selector_widget->set_edit_fn(
                [&, highlight_selector_widget, handle_edit_fn](int index) {
                    Highlight hl = highlight_selector_widget->highlight_model->highlights[index];
                    handle_edit_fn(hl);
                }
            );

            set_current_widget(highlight_selector_widget);
            show_current_widget();
        }
        else {
            std::vector<std::wstring> option_names;
            std::vector<std::wstring> option_location_wstrings;

            for (auto highlight : highlights) {
                option_names.push_back(ITEM_LIST_PREFIX + L" " + highlight.description);
                int page = highlight.selection_begin.to_document(doc()).page;
                option_location_wstrings.push_back(get_page_formatted_string(page + 1));
            }

            set_filtered_select_menu<Highlight>(this, FUZZY_SEARCHING, MULTILINE_MENUS, { option_names, option_location_wstrings }, highlights, closest_highlight_index,
                [&, handle_select_fn](Highlight* hl) {
                    handle_select_fn(*hl);
                },
                [&, handle_delete_fn](Highlight* hl) {
                    handle_delete_fn(*hl);
                },
                [&, handle_edit_fn](Highlight* hl) {
                    handle_edit_fn(*hl);
                }
            );
            show_current_widget();

        }
    }
}

void MainWidget::handle_goto_highlight_global() {
    std::vector<std::pair<std::string, Highlight>> global_highlights;
    db_manager->global_select_highlight(global_highlights);

    auto handle_select_fn = [&](float offset_y, std::string checksum) {
        if (checksum.size() > 0) {
            if (QString::fromStdString(checksum).startsWith("SERVER://")) {
                download_and_open(QString::fromStdString(checksum).mid(9).toStdString(), offset_y);

            }
            else {
                if (pending_command_instance) {
                    QString file_path = QString::fromStdWString(checksummer->get_path(checksum).value_or(L""));
                    pending_command_instance->set_generic_requirement(
                        QList<QVariant>() << file_path << offset_y);
                }
                advance_command(std::move(pending_command_instance));
            }
            pop_current_widget();
        }
    };

    auto handle_delete_fn = [&](std::string uuid) {
            delete_highlight_with_uuid(uuid);
        };

    std::vector<QString> file_names;
    std::vector<QString> file_checksums;
    std::vector<Highlight> highlights;

    for (auto [checksum, hl] : global_highlights) {

        QString file_name = QString::fromStdWString(checksummer->get_path(checksum).value_or(L""));
        if (file_name.size() == 0) {
            if (sioyek_network_manager->is_checksum_available_on_server(checksum)) {
                file_name = QString::fromStdWString(SERVER_SYMBOL);
                checksum = "SERVER://" + checksum;
            }
            else {
                continue;
                //file_name = QString::fromStdString(checksum);
            }
        }

        highlights.push_back(hl);
        file_checksums.push_back(QString::fromStdString(checksum));
        file_names.push_back(file_name);
    }

    if (TOUCH_MODE) {
        //HighlightSelectorWidget* highlight_selector_widget = HighlightSelectorWidget::from_highlights(std::move(highlights), this, std::move(file_names), std::move(file_checksums));
        HighlightModel* highlights_model = new HighlightModel(std::move(highlights), std::move(file_names), std::move(file_checksums), this);

        TouchDelegateListView* lv = new TouchDelegateListView(highlights_model, true, "TouchHighlightsView", { std::make_pair("_colorMap", get_color_mapping())}, this);
        lv->list_view->proxy_model->set_is_highlight(true);
        lv->list_view->proxy_model->setFilterKeyColumn(-1);

        lv->set_select_fn(
            [&, highlights_model, handle_select_fn](int index) {
                Highlight hl = highlights_model->highlights[index];
                std::string checksum = highlights_model->checksums[index].toStdString();
                handle_select_fn(hl.selection_begin.y, checksum);
            });

        lv->set_delete_fn([&, highlights_model, handle_delete_fn](int index) {
            Highlight hl = highlights_model->highlights[index];
            handle_delete_fn(hl.uuid);
            });


        set_current_widget(lv);
        show_current_widget();
    }
    else {
        if (FANCY_UI_MENUS) {
            HighlightSelectorWidget* highlight_selector_widget = HighlightSelectorWidget::from_highlights(std::move(highlights), this, std::move(file_names), std::move(file_checksums));

            highlight_selector_widget->set_select_fn(
                [&, highlight_selector_widget, handle_select_fn](int index) {
                    Highlight hl = highlight_selector_widget->highlight_model->highlights[index];
                    std::string checksum = highlight_selector_widget->highlight_model->checksums[index].toStdString();
                    handle_select_fn(hl.selection_begin.y, checksum);
                });

            highlight_selector_widget->set_delete_fn([&, highlight_selector_widget, handle_delete_fn](int index) {
                Highlight hl = highlight_selector_widget->highlight_model->highlights[index];
                handle_delete_fn(hl.uuid);
                });

            set_current_widget(highlight_selector_widget);
            show_current_widget();
        }
        else {
            std::vector<std::wstring> descs;
            std::vector<std::wstring> file_names_wstring;
            std::vector<std::pair<BookState, std::string>> book_states;
            for (int i = 0; i < highlights.size(); i++) {
                descs.push_back(highlights[i].description);
                file_names_wstring.push_back(file_names[i].toStdWString());
                BookState book_state;
                book_state.document_path = file_checksums[i].toStdWString();
                book_state.offset_y = highlights[i].selection_begin.y;
                book_states.push_back(std::make_pair(book_state, highlights[i].uuid));
            }

            set_filtered_select_menu<std::pair<BookState, std::string>>(this, FUZZY_SEARCHING, MULTILINE_MENUS, { descs, file_names_wstring }, book_states, -1,
                [&, handle_select_fn](std::pair<BookState, std::string>* book_state) {
                    handle_select_fn(book_state->first.offset_y, book_state->second);

                },
                [&, handle_delete_fn](std::pair<BookState, std::string>* book_state) {
                    handle_delete_fn(book_state->second);
                }
            );
            show_current_widget();

        }
    }
}

void MainWidget::handle_goto_toc() {

    if (!main_document_view || !main_document_view->get_document()){
        return;
    }
    if (main_document_view->get_document()->has_toc()) {
        if (TOUCH_MODE) {
            std::vector<std::wstring> flat_toc;
            std::vector<DocumentPos> current_document_toc_pages;
            get_flat_toc(main_document_view->get_document()->get_toc(), flat_toc, current_document_toc_pages);
            std::vector<std::wstring> page_strings;
            for (int i = 0; i < current_document_toc_pages.size(); i++) {
                page_strings.push_back(("[ " + QString::number(current_document_toc_pages[i].page + 1) + " ]").toStdWString());
            }
            int closest_toc_index = current_document_toc_pages.size() - 1;
            int current_page = get_current_page_number();
            for (int i = 0; i < current_document_toc_pages.size(); i++) {
                if (current_document_toc_pages[i].page > current_page) {
                    closest_toc_index = i - 1;
                    break;
                }
            }
            if (closest_toc_index == -1) {
                closest_toc_index = 0;
            }
            QAbstractItemModel* model = create_table_model(flat_toc, page_strings);

            set_current_widget(new TouchFilteredSelectWidget<DocumentPos>(FUZZY_SEARCHING, model, current_document_toc_pages, closest_toc_index, [&](DocumentPos* page_value) {
                if (page_value && pending_command_instance) {
                    pending_command_instance->set_generic_requirement(page_value->page);
                    advance_command(std::move(pending_command_instance));
                    invalidate_render();
                }
                pop_current_widget();
                }, [&](DocumentPos* page) {}, this));
            show_current_widget();
        }
        else {

            if (FLAT_TABLE_OF_CONTENTS) {
                std::vector<std::wstring> flat_toc;
                std::vector<DocumentPos> current_document_toc_pages;
                get_flat_toc(main_document_view->get_document()->get_toc(), flat_toc, current_document_toc_pages);
                set_current_widget(new FilteredSelectWindowClass<DocumentPos>(FUZZY_SEARCHING, flat_toc, current_document_toc_pages, [&](DocumentPos* page_value) {
                    if (page_value && pending_command_instance) {
                        pending_command_instance->set_generic_requirement(page_value->page);
                        advance_command(std::move(pending_command_instance));

                    }
                pop_current_widget();
                    }, this));
                show_current_widget();
            }
            else {

                std::vector<int> selected_index = main_document_view->get_current_chapter_recursive_index();
                //if (!TOUCH_MODE) {
                set_current_widget(new FilteredTreeSelect<int>(FUZZY_SEARCHING, main_document_view->get_document()->get_toc_model(),
                    [&](const std::vector<int>& indices) {
                        TocNode* toc_node = get_toc_node_from_indices(main_document_view->get_document()->get_toc(),
                        indices);
                if (toc_node && pending_command_instance) {
                    if (std::isnan(toc_node->y)) {
                        pending_command_instance->set_generic_requirement(toc_node->page);
                        advance_command(std::move(pending_command_instance));
                    }
                    else {
                        pending_command_instance->set_generic_requirement(QList<QVariant>() << toc_node->page << toc_node->x << toc_node->y);
                        advance_command(std::move(pending_command_instance));
                    }
                }
                pop_current_widget();
                    }, this, selected_index));
                show_current_widget();
            }
        }

    }
    else {
        show_error_message(L"This document doesn't have a table of contents");
    }
}

void MainWidget::handle_open_all_docs() {


    std::vector<std::pair<std::wstring, std::wstring>> pairs;
    db_manager->get_prev_path_hash_pairs(pairs);

    // show the most recent files first 
    std::reverse(pairs.begin(), pairs.end());

    std::vector<std::string> hashes;
    std::vector<std::wstring> paths;

    for (auto [path, hash] : pairs) {
        hashes.push_back(utf8_encode(hash));
        paths.push_back(path);
    }


    set_filtered_select_menu<std::string>(this, FUZZY_SEARCHING, MULTILINE_MENUS, { paths }, hashes, -1,
        [&](std::string* doc_hash) {
            if ((doc_hash->size() > 0) && (pending_command_instance)) {
                pending_command_instance->set_generic_requirement(QList<QVariant>() << QString::fromStdString(*doc_hash));
                advance_command(std::move(pending_command_instance));
            }
        },
        [&](std::string* doc_hash) {
            db_manager->delete_opened_book(*doc_hash);
        }
        );

    show_current_widget();
}

std::vector<OpenedBookInfo> MainWidget::get_all_opened_books(bool include_server_books, bool force_full_path) {
    std::vector<OpenedBookInfo> res;
    db_manager->select_opened_books(res);
    for (int i = 0; i < res.size(); i++) {
        std::wstring path = document_manager->get_path_from_hash(res[i].checksum).value_or(L"");
        if (path.size() > 0) {
            if (SHOW_DOC_PATH || force_full_path) {
                res[i].file_name = QString::fromStdWString(path);
            }
            else {
                res[i].file_name = QString::fromStdWString(Path(path).filename().value_or(L""));
            }
        }
    }

    auto new_end = std::remove_if(res.begin(), res.end(), [](OpenedBookInfo& info) {
        return info.file_name.size() == 0;
        });
    res.erase(new_end, res.end());

    if ((sioyek_network_manager->status == ServerStatus::LoggedIn) && include_server_books) {
        std::vector<std::string> local_file_hashes;
        for (auto info : res) {
            local_file_hashes.push_back(info.checksum);
        }
        std::vector<OpenedBookInfo> server_opened_books = sioyek_network_manager->get_excluded_opened_files(local_file_hashes);

        auto middle_index = res.size();

        for (auto& server_book : server_opened_books) {
            server_book.checksum = "SERVER://" + server_book.checksum;
            res.push_back(server_book);
        }

        auto last = res.end();
        // at the time of this commit, inplace_merge on android seems to be wrong, so we
        // sort the entire array instead, we should just be using the inplace_merge when
        // it is fixed
#ifndef SIOYEK_MOBILE
        std::inplace_merge(res.begin(), res.begin() + middle_index, last, [](const OpenedBookInfo& lhs, const OpenedBookInfo& rhs) {
            return lhs.last_access_time > rhs.last_access_time;
            });
#else
        std::sort(res.begin(), res.end(), [](const OpenedBookInfo& lhs, const OpenedBookInfo& rhs) {
            return lhs.last_access_time > rhs.last_access_time;
            });
#endif
    }
    return res;
}

void MainWidget::handle_open_prev_doc() {
    auto handle_select_fn = [&](std::string checksum, float offset_y) {
        if ((checksum.size() > 0) && (pending_command_instance)) {
            QString doc_hash_qstring = QString::fromStdString(checksum);
            if (doc_hash_qstring.startsWith("SERVER://")) {
                doc_hash_qstring = doc_hash_qstring.mid(9);
                download_and_open(doc_hash_qstring.toStdString(), offset_y);
            }
            else {
                pending_command_instance->set_generic_requirement(QList<QVariant>() << QString::fromStdString(checksum));
                advance_command(std::move(pending_command_instance));
            }
        }
        pop_current_widget();
        };

    auto handle_delete_fn = [&](std::string checksum) {
        QString doc_hash_qstring = QString::fromStdString(checksum);
        if (doc_hash_qstring.startsWith("SERVER://")) {
            sioyek_network_manager->delete_file_from_server(this, doc_hash_qstring.mid(9).toStdString(), []() {});
        }
        else {
            db_manager->delete_opened_book(checksum);
        }
        };

    if (TOUCH_MODE || (!FANCY_UI_MENUS)) {
        std::vector<std::wstring> opened_docs_names;
        std::vector<std::wstring> opened_docs_actual_names;
        std::vector<OpenedBookInfo> opened_docs = get_all_opened_books();
        std::vector<OpenedBookInfo> opened_docs_instances;

        std::wstring current_path = L"";

        if (doc()) {
            current_path = doc()->get_path();
        }

        for (const auto& opened_doc : opened_docs) {
            if (QString::fromStdString(opened_doc.checksum).startsWith("SERVER://")) {
                opened_docs_names.push_back(L"[" + SERVER_SYMBOL + L"] " + opened_doc.file_name.toStdWString());
                //opened_docs_hashes.push_back(opened_doc.checksum);
                opened_docs_actual_names.push_back(opened_doc.document_title.toStdWString());
                opened_docs_instances.push_back(opened_doc);
            }
            else {

                std::optional<std::wstring> path = checksummer->get_path(opened_doc.checksum);
                if (path) {
                    if (path == current_path) continue;

                    if (SHOW_DOC_PATH) {
                        opened_docs_names.push_back(path.value_or(L"<ERROR>"));
                    }
                    else {
#ifdef SIOYEK_ANDROID
                        std::wstring path_value = path.value();
                        if (path_value.substr(0, 10) == L"content://") {
                            path_value = android_file_name_from_uri(QString::fromStdWString(path_value)).toStdWString();
                        }
                        opened_docs_names.push_back(Path(path.value()).filename_no_ext().value_or(L"<ERROR>"));
#else
                        opened_docs_names.push_back(Path(path.value()).filename().value_or(L"<ERROR>"));
#endif
                    }
                    //opened_docs_hashes.push_back(opened_doc.checksum);
                    opened_docs_actual_names.push_back(opened_doc.document_title.toStdWString());
                    opened_docs_instances.push_back(opened_doc);
                }
            }
        }

        set_filtered_select_menu<OpenedBookInfo>(this, FUZZY_SEARCHING, MULTILINE_MENUS, { opened_docs_names, opened_docs_actual_names }, opened_docs_instances, -1,
            [&, handle_select_fn](OpenedBookInfo* info) {
                handle_select_fn(info->checksum, info->offset_y);
            },
            [&, handle_delete_fn](OpenedBookInfo* info) {
                handle_delete_fn(info->checksum);
            }
        );

        make_current_menu_columns_equal();
        show_current_widget();
    }
    else {
        std::vector<OpenedBookInfo> opened_documents = get_all_opened_books();
        DocumentSelectorWidget* selector_widget = DocumentSelectorWidget::from_documents(std::move(opened_documents), this);

        selector_widget->set_select_fn([&, selector_widget, handle_select_fn](int index) {
            OpenedBookInfo info = selector_widget->document_model->opened_documents[index];
            handle_select_fn(info.checksum, info.offset_y);
            });

        selector_widget->set_delete_fn([&, selector_widget, handle_delete_fn](int index) {
            OpenedBookInfo info = selector_widget->document_model->opened_documents[index];
            handle_delete_fn(info.checksum);
            });

        set_current_widget(selector_widget);
        show_current_widget();
    }

}

MainWidget* MainWidget::handle_new_window() {
    MainWidget* new_widget = new MainWidget(mupdf_context,
        pdf_renderer,
        db_manager,
        document_manager,
        config_manager,
        command_manager,
        input_handler,
        checksummer,
        sioyek_network_manager,
        background_task_manager,
        background_bookmark_renderer,
        should_quit);
    new_widget->open_document(main_document_view->get_state());
    new_widget->show();
    new_widget->apply_window_params_for_one_window_mode();
    new_widget->execute_macro_if_enabled(STARTUP_COMMANDS);
    new_widget->run_startup_js();

    windows.push_back(new_widget);
    return new_widget;
}

std::optional<PdfLink> MainWidget::get_selected_link(const std::wstring& text) {
    std::vector<PdfLink> visible_page_links;
    if ((!NUMERIC_TAGS) || is_string_numeric(text)) {

        int link_index = 0;

        if (!NUMERIC_TAGS) {
            link_index = get_index_from_tag(utf8_encode(text), true);
        }
        else {
            link_index = std::stoi(text);
        }

        main_document_view->get_visible_links(visible_page_links);
        if ((link_index >= 0) && (link_index < static_cast<int>(visible_page_links.size()))) {
            return visible_page_links[link_index];
        }
        return {};
    }
    return {};
}

void MainWidget::handle_overview_link(const std::wstring& text) {

    auto selected_link_ = get_selected_link(text);
    if (selected_link_) {
        PdfLink pdf_link;
        pdf_link.rects = selected_link_->rects;
        pdf_link.uri = selected_link_->uri;
        pdf_link.source_page = selected_link_->source_page;
        dv()->set_overview_link(pdf_link);
    }
    reset_highlight_links();
}

void MainWidget::handle_portal_to_link(const std::wstring& text) {

    auto selected_link_ = get_selected_link(text);
    if (selected_link_) {
        PdfLink pdf_link = selected_link_.value();
        ParsedUri parsed_uri = parse_uri(mupdf_context, doc()->doc, pdf_link.uri);

        AbsoluteDocumentPos src_abspos = DocumentPos {pdf_link.source_page, 0, pdf_link.rects[0].y0}.to_absolute(doc());
        AbsoluteDocumentPos dst_abspos = DocumentPos {parsed_uri.page-1, parsed_uri.x, parsed_uri.y}.to_absolute(doc());

        Portal portal;
        portal.dst.document_checksum = doc()->get_checksum();
        portal.dst.book_state.offset_x = dst_abspos.x;
        portal.dst.book_state.offset_y = dst_abspos.y;
        portal.dst.book_state.zoom_level = main_document_view->get_zoom_level();
        portal.src_offset_y = src_abspos.y;
        std::string uuid = doc()->add_portal(portal, true);
        on_new_portal_added(uuid);

    }
    reset_highlight_links();
}

void MainWidget::handle_open_link(const std::wstring& text, bool copy) {

    auto selected_link_ = get_selected_link(text);
    if (selected_link_) {
        //auto [selected_page, selected_link] = selected_link_.value();
        PdfLink link = selected_link_.value();

        if (copy) {
            copy_to_clipboard(utf8_decode(link.uri));
        }
        else {
            if (QString::fromStdString(link.uri).startsWith("http")) {
                open_web_url(utf8_decode(link.uri));
            }
            else {
                auto [page, offset_x, offset_y] = parse_uri(mupdf_context, doc()->doc, link.uri);
                if (main_document_view->is_presentation_mode()) {
                    goto_page_with_page_number(page - 1);
                }
                else {
                    handle_goto_link_with_page_and_offset(page - 1, offset_y);
                }
            }
        }
    }
    reset_highlight_links();
}

void MainWidget::handle_keys_user_all() {
    std::vector<Path> keys_paths = input_handler->get_all_user_keys_paths();
    std::vector<std::wstring> keys_paths_wstring;
    for (auto path : keys_paths) {
        keys_paths_wstring.push_back(path.get_path());
    }

    set_current_widget(new FilteredSelectWindowClass<std::wstring>(
        FUZZY_SEARCHING,
        keys_paths_wstring,
        keys_paths_wstring,
        [&](std::wstring* path) {
            if (path) {
                open_file(*path, true);
            }
        },
        this));
    show_current_widget();
}

void MainWidget::sync_annotations_with_server() {
    sioyek_network_manager->sync_document_annotations_to_server(this, doc(), [this]() {invalidate_render(); });
}


void MainWidget::handle_prefs_user_all() {
    std::vector<Path> prefs_paths = config_manager->get_all_user_config_files();
    std::vector<std::wstring> prefs_paths_wstring;
    for (auto path : prefs_paths) {
        prefs_paths_wstring.push_back(path.get_path());
    }

    set_current_widget(new FilteredSelectWindowClass<std::wstring>(
        FUZZY_SEARCHING,
        prefs_paths_wstring,
        prefs_paths_wstring,
        [&](std::wstring* path) {
            if (path) {
                open_file(*path, true);
            }
        },
        this));
    show_current_widget();
}

void MainWidget::handle_portal_to_overview() {
    std::optional<Portal> new_portal = main_document_view->create_portal_to_overview();
    if (new_portal.has_value()) {
        add_portal(main_document_view->get_document()->get_path(), new_portal.value());
    }
}

void MainWidget::handle_goto_window() {
    std::vector<std::wstring> window_names;
    std::vector<int> window_ids;
    for (int i = 0; i < windows.size(); i++) {
        window_names.push_back(windows[i]->windowTitle().toStdWString());
        window_ids.push_back(i);
    }
    set_current_widget(new FilteredSelectWindowClass<int>(FUZZY_SEARCHING, window_names,
        window_ids,
        [&](int* window_id) {
            if (*window_id < windows.size()) {
                windows[*window_id]->raise();
                windows[*window_id]->activateWindow();
            }
        },
        this));
    show_current_widget();
}

void MainWidget::handle_toggle_smooth_scroll_mode() {
    smooth_scroll_mode = !smooth_scroll_mode;

    if (smooth_scroll_mode) {
        validation_interval_timer->setInterval(16);
    }
    else {
        validation_interval_timer->setInterval(INTERVAL_TIME);
    }
}


void MainWidget::handle_overview_to_portal() {
    if (main_document_view->get_overview_page()) {
        set_overview_page({}, false);
    }
    else {

        OverviewState overview_state;
        std::optional<Portal> portal_ = main_document_view->get_target_portal(false);
        if (portal_) {
            Portal portal = portal_.value();
            auto destination_path = checksummer->get_path(portal.dst.document_checksum);
            if (destination_path) {
                Document* doc = document_manager->get_document(destination_path.value());
                if (doc) {
                    doc->open(true);
                    overview_state.absolute_offset_y = portal.dst.book_state.offset_y;
                    overview_state.doc = doc;
                    set_overview_page(overview_state, true);
                }
            }
        }
    }
}

void MainWidget::handle_toggle_typing_mode() {
    if (!typing_location.has_value()) {
        int page = main_document_view->get_center_page_number();
        CharacterAddress charaddr;
        charaddr.doc = main_document_view->get_document();
        charaddr.page = page - 1;
        charaddr.next_page();

        main_document_view->set_typing_rect(DocumentRect(rect_from_quad(charaddr.character->quad), charaddr.page), {});

        typing_location = std::move(charaddr);
        main_document_view->set_offset_y(typing_location.value().focus_offset());

    }
}

void MainWidget::handle_delete_highlight_under_cursor() {
    QPoint mouse_pos = mapFromGlobal(cursor_pos());
    WindowPos window_pos = WindowPos{ mouse_pos.x(), mouse_pos.y() };
    std::string sel_highlight = main_document_view->get_highlight_uuid_in_pos(window_pos);
    if (sel_highlight.size() > 0) {
        if (main_document_view->get_selected_highlight_uuid() == sel_highlight) {
            main_document_view->clear_selected_object();
        }
        delete_current_document_highlight_with_uuid(sel_highlight);
    }
}

std::optional<Highlight> MainWidget::handle_delete_selected_highlight() {
    std::string selected_highlight_uuid = main_document_view->get_selected_highlight_uuid();
    std::optional<Highlight> deleted_highlight = {};
    if (selected_highlight_uuid.size() > 0) {
        main_document_view->set_selected_highlight_uuid("");
        deleted_highlight = delete_current_document_highlight_with_uuid(selected_highlight_uuid);
    }
    validate_render();
    return deleted_highlight;
}

std::optional<BookMark> MainWidget::handle_delete_selected_bookmark() {
    std::string selected_bookmark_uuid = main_document_view->get_selected_bookmark_uuid();
    std::optional<BookMark> deleted_bookmark = {};
    if (selected_bookmark_uuid.size() > 0) {
        main_document_view->set_selected_bookmark_uuid("");
        deleted_bookmark = delete_current_document_bookmark(selected_bookmark_uuid);
    }
    validate_render();
    return deleted_bookmark;
}

std::optional<Portal> MainWidget::handle_delete_selected_portal() {
    std::string selected_portal_uuid = main_document_view->get_selected_portal_uuid();
    std::optional<Portal> deleted_portal = {};
    if (selected_portal_uuid.size() > 0) {
        main_document_view->clear_selected_object();
        deleted_portal = doc()->delete_portal_with_uuid(selected_portal_uuid);
        if (deleted_portal.has_value()) {
            on_portal_deleted(deleted_portal.value(), doc()->get_checksum());
        }
        //push_deleted_portal(deleted_portal);
    }
    validate_render();
    return deleted_portal;
}

void MainWidget::synchronize_pending_link() {
    for (auto window : windows) {
        if (window != this) {
            window->main_document_view->set_pending_portal(main_document_view->current_pending_portal);
        }
    }
    refresh_all_windows();
}

void MainWidget::refresh_all_windows() {
    for (auto window : windows) {
        window->invalidate_ui();
    }
}


int MainWidget::num_visible_links() {
    std::vector<PdfLink> visible_page_links;
    main_document_view->get_visible_links(visible_page_links);
    return visible_page_links.size();
}

bool MainWidget::event(QEvent* event) {
    QTabletEvent* te = dynamic_cast<QTabletEvent*>(event);
    QKeyEvent* ke = dynamic_cast<QKeyEvent*>(event);

    if (event->type() == QEvent::WindowActivate) {
        if (SAVE_EXTERNALLY_EDITED_TEXT_ON_FOCUS && is_external_file_edited) {
            if (text_command_line_edit->isVisible()) {
                handle_text_edit_return_pressed();
                is_external_file_edited = false;
            }
        }
    }

    if (ke && (ke->type() == QEvent::KeyPress)) {
        // Apparently Qt doesn't send keyPressEvent for tab and backtab anymore, so we have to
        // manually handle them here.
        // todo: make sure this doesn't cause problems on linux and mac
        if (((ke->key() == Qt::Key_Tab) && (ke->modifiers() == 0)) || ((ke->key() == Qt::Key_Backtab) && (ke->modifiers() == Qt::ShiftModifier))) {
            if (event->isAccepted()) {
                key_event(false, ke);
            }
        }
    }

    if (event->type() == QEvent::WindowActivate) {
        if (is_scratchpad_mode()) {
            scratchpad->on_view_size_change(width(), height());
        }
    }
    if ((should_draw(true)) && te) {
        handle_pen_drawing_event(te);
        event->accept();
        return true;
    }

    //if (event->type() == QEvent::TabletEVe)
    if (TOUCH_MODE) {

        if (event->type() == QEvent::TouchUpdate) {
            // when performing pinch to zoom, Qt only fires PinchGesture event when
            // the finger that initiated the pinch is moved (the first finger to touch the screen)
            // therefore, if the user tries to pinch to zoom using the second finger, the render is not
            // updated in PinchGesture handling code. Here we manually update the render if we are pinching
            // even when the other finger is moved, which results in a much smoother zooming.
            if (is_pinching) {
                validate_render();
            }
        }

        if (event->type() == QEvent::Gesture) {
            auto gesture = (static_cast<QGestureEvent*>(event));

            if (gesture->gesture(Qt::TapAndHoldGesture)) {
                was_hold_gesture = true;
                main_document_view->velocity_x = 0;
                main_document_view->velocity_y = 0;

                if (was_last_mouse_down_in_ruler_next_rect) {
                    return true;
                }

                if ((mapFromGlobal(QCursor::pos()) - last_press_point).manhattanLength() > 10) {
                    return QWidget::event(event);
                }

                // only show menu when there are no other widgets
                if (current_widget_stack.size() > 0) {
                    return true;
                }

                last_hold_point = mapFromGlobal(QCursor::pos());
                WindowPos window_pos = WindowPos{ last_hold_point.x(), last_hold_point.y() };
                AbsoluteDocumentPos hold_abspos = main_document_view->window_to_absolute_document_pos(window_pos);
                if (doc()) {
                    std::string bookmark_uuid = doc()->get_bookmark_uuid_at_pos(hold_abspos);

                    if (bookmark_uuid.size() > 0) {
                        main_document_view->begin_bookmark_move(bookmark_uuid, hold_abspos);
                        main_document_view->set_selected_bookmark_uuid(bookmark_uuid);
                        show_touch_buttons({ L"Delete", L"Edit" }, {}, [this](int index, std::wstring name) {

                            std::string selected_bookmark_uuid = main_document_view->get_selected_bookmark_uuid();
                            if (selected_bookmark_uuid.size() > 0) {
                                if (name == L"Delete") {
                                    delete_current_document_bookmark(selected_bookmark_uuid);
                                    main_document_view->set_selected_bookmark_uuid("");
                                    pop_current_widget();
                                    invalidate_render();
                                }
                                else {
                                    pop_current_widget();
                                    handle_command_types(command_manager->get_command_with_name(this, "edit_selected_bookmark"), 0);
                                    return;
                                }
                            }
                            });
                        //show_touch_delete_button();
                        return true;
                    }

                    std::string portal_uuid = doc()->get_icon_portal_uuid_at_pos(hold_abspos);
                    if (portal_uuid.size()) {
                        main_document_view->begin_portal_move(portal_uuid, hold_abspos, false);
                        main_document_view->set_selected_portal_uuid(portal_uuid);
                        show_touch_buttons({ L"Delete" }, {}, [this](int index, std::wstring name) {
                            if (main_document_view->get_selected_portal_uuid().size() > 0) {
                                std::string del_uuid = main_document_view->get_selected_portal_uuid();
                                main_document_view->clear_selected_object();
                                //std::string uuid = doc()->get_portals()[del_index].uuid;
                                std::optional<Portal> deleted_portal = doc()->delete_portal_with_uuid(del_uuid);
                                if (deleted_portal) {
                                    on_portal_deleted(deleted_portal.value(), doc()->get_checksum());
                                }
                                pop_current_widget();
                                invalidate_render();
                            }
                            });
                        return true;
                    }
                }
                //opengl_widget->last_selected_block

                QTapAndHoldGesture* tapgest = static_cast<QTapAndHoldGesture*>(gesture->gesture(Qt::TapAndHoldGesture));
                if (tapgest->state() == Qt::GestureFinished) {

                    stop_dragging();

                    if (is_in_middle_left_rect(window_pos)) {
                        if (execute_macro_if_enabled(MIDDLE_LEFT_RECT_HOLD_COMMAND)) {
                            invalidate_render();
                            return true;
                        }
                    }
                    if (is_in_middle_right_rect(window_pos)) {
                        if (execute_macro_if_enabled(MIDDLE_RIGHT_RECT_HOLD_COMMAND)) {
                            invalidate_render();
                            return true;
                        }
                    }
                    if (is_in_back_rect(window_pos)) {
                        handle_command_types(command_manager->create_macro_command(this, "", BACK_RECT_HOLD_COMMAND), 0);
                        invalidate_render();
                        return true;
                    }
                    if (is_in_forward_rect(window_pos)) {
                        handle_command_types(command_manager->create_macro_command(this, "", FORWARD_RECT_HOLD_COMMAND), 0);
                        invalidate_render();
                        return true;
                    }
                    if (is_in_edit_portal_rect(window_pos)) {
                        handle_command_types(command_manager->create_macro_command(this, "", TOP_CENTER_HOLD_COMMAND), 0);
                        invalidate_render();
                        return true;
                    }
                    if ((!main_document_view->is_ruler_mode()) && is_in_visual_mark_next_rect(window_pos)) {
                        if (execute_macro_if_enabled(VISUAL_MARK_NEXT_HOLD_COMMAND)) {
                            return true;
                        }
                    }
                    if ((!main_document_view->is_ruler_mode()) && is_in_visual_mark_prev_rect(window_pos)) {
                        if (execute_macro_if_enabled(VISUAL_MARK_PREV_HOLD_COMMAND)) {
                            return true;
                        }
                    }

                    overview_under_pos(last_hold_point);
                    update_touch_overview_buttons(dv()->overview_page);
                    // show_touch_main_menu();

                    return true;
                }
            }
            if (gesture->gesture(Qt::PinchGesture)) {
                pdf_renderer->no_rerender = true;
                QPinchGesture* pinch = static_cast<QPinchGesture*>(gesture->gesture(Qt::PinchGesture));
                if (pinch->state() == Qt::GestureStarted) {
                    is_pinching = true;
                }
                if ((pinch->state() == Qt::GestureFinished) || (pinch->state() == Qt::GestureCanceled)) {
                    is_pinching = false;
                    stop_dragging();
                }
                float scale = pinch->scaleFactor();

                if ((pinch->scaleFactor() >= 1 && pinch->lastScaleFactor() >= 1)
                    || (pinch->scaleFactor() <= 1 && pinch->lastScaleFactor() <= 1)
                    ){

                    if (main_document_view->get_overview_page()){
                        main_document_view->zoom_overview(scale);
                    }
                    else if (main_document_view->selected_object_index.has_value() && main_document_view->selected_object_index->object_type == VisibleObjectType::PinnedPortal) {
                        // todo: this is not testes, I should test this on a touch screen
                        Portal* portal = doc()->get_portal_with_uuid(main_document_view->selected_object_index->uuid);
                        if (portal) {
                            portal->dst.book_state.zoom_level *= scale;
                            main_document_view->schedule_update_link_with_opened_book_state(*portal, portal->dst.book_state);
                        }
                    }
                    else{
                        dv()->set_zoom_level(dv()->get_zoom_level() * scale, true);
                    }
                }
#ifdef SIOYEK_IOS
                validate_render();
#endif
                return true;
            }

            return QWidget::event(event);

        }
    }

    return QWidget::event(event);
}

void MainWidget::start_mobile_selection_under_point(AbsoluteDocumentPos abspoint) {
    if (!doc()) return;

    fz_stext_char* character_under = main_document_view->get_closest_character_to_cusrsor(abspoint);

    // WindowPos last_hold_point_window_pos = { last_hold_point.x(), last_hold_point.y() };
    // DocumentPos last_hold_point_document_pos = main_document_view->window_to_document_pos(last_hold_point_window_pos);
    DocumentPos last_hold_point_document_pos = abspoint.to_document(doc());

    if (character_under) {
        int current_page = last_hold_point_document_pos.page;
        AbsoluteRect centered_rect = DocumentRect(rect_from_quad(character_under->quad), current_page).to_absolute(doc());
        main_document_view->selected_character_rects.push_back(centered_rect);

        AbsoluteDocumentPos begin_abspos;
        begin_abspos.x = centered_rect.x0;
        begin_abspos.y = (centered_rect.y0 + centered_rect.y1) / 2;
        //begin_abspos.y = rect.y0;

        AbsoluteDocumentPos end_abspos;
        end_abspos.x = centered_rect.x1;
        end_abspos.y = (centered_rect.y1 + centered_rect.y0) / 2;

        WindowPos begin_window_pos = begin_abspos.to_window(main_document_view);
        WindowPos end_window_pos = end_abspos.to_window(main_document_view);

        int selection_indicator_size = 40;
        QPoint begin_pos;
        begin_pos.setX(begin_window_pos.x - selection_indicator_size);
        begin_pos.setY(begin_window_pos.y - selection_indicator_size);

        QPoint end_pos;
        end_pos.setX(end_window_pos.x);
        end_pos.setY(end_window_pos.y);

        selection_begin_indicator = new SelectionIndicator(this, true, this, begin_abspos);
        selection_end_indicator = new SelectionIndicator(this, false, this, end_abspos);
        //        text_selection_buttons = new TextSelectionButtons(this);


        //        int window_width = width();
        //        int window_height = height();
        float pixel_ratio = QGuiApplication::primaryScreen()->devicePixelRatio();

        //        begin_pos = begin_pos / pixel_ratio;
        //        end_pos = end_pos / pixel_ratio;

        //        float real_height = pixel_ratio * window_height;
        //        float real_width = pixel_ratio * window_width;

        selection_begin_indicator->move(begin_pos);
        selection_end_indicator->move(end_pos);

        //        selection_begin_indicator->move(QPoint(0, 0));
        //        selection_end_indicator->move(QPoint(20, 20));

        selection_begin_indicator->resize(selection_indicator_size, selection_indicator_size);
        selection_end_indicator->resize(selection_indicator_size, selection_indicator_size);

        selection_begin_indicator->raise();
        selection_end_indicator->raise();

        selection_begin_indicator->show();
        selection_end_indicator->show();
        get_text_selection_buttons()->show();

        invalidate_render();
    }
    //    selected_character
    //    smart_jump_under_pos(window_pos);

    //    main_document_view->window_to_absolute_document_pos();
}

void MainWidget::update_mobile_selection() {
    AbsoluteDocumentPos begin_absolute = selection_begin_indicator->get_docpos().to_absolute(doc());
    AbsoluteDocumentPos end_absolute = selection_end_indicator->get_docpos().to_absolute(doc());

    main_document_view->get_text_selection(begin_absolute,
        end_absolute,
        false,
        main_document_view->selected_character_rects,
        dv()->selected_text);
    main_document_view->selected_text_is_dirty = false;
    validate_render();
}

void MainWidget::clear_selection_indicators() {
    if (selection_begin_indicator) {
        selection_begin_indicator->hide();
        selection_end_indicator->hide();
        get_text_selection_buttons()->hide();
        delete selection_begin_indicator;
        delete selection_end_indicator;
        //delete text_selection_buttons;
        selection_begin_indicator = nullptr;
        selection_end_indicator = nullptr;
        //text_selection_buttons = nullptr;
    }
}

bool MainWidget::handle_quick_tap(WindowPos click_pos) {
    // returns true if we double clicked or clicked on a location that executes a command
    // in either case, we should not do anything else corresponding to the single tap event
    QTime now = QTime::currentTime();
    NormalizedWindowPos normalized_click_pos = click_pos.to_window_normalized(main_document_view);

    // if we perform a quick tap, then we are not moving or resizing
    // this prevents bugs e.g. when we tap on a rect that is supposed to show the touch
    // mode menu and it overlaps a portal or bookmark
    main_document_view->visible_object_move_data = {};
    main_document_view->visible_object_resize_data = {};

    if ((last_quick_tap_time.msecsTo(now) < 200) && (mapFromGlobal(QCursor::pos()) - last_quick_tap_position).manhattanLength() < 20) {
        if (handle_double_tap(last_quick_tap_position)) {
            return true;
        }
    }

    if (main_document_view->is_showing_rect_hints()) {
        main_document_view->hide_rect_hints();
    }

    if (current_widget_stack.size() > 0) {
        bool is_tap_in_overview = main_document_view->is_window_point_in_overview(normalized_click_pos);
        if (!is_tap_in_overview){
            bool should_return = does_current_widget_consume_quicktap_event();
            pop_current_widget();
            if (should_return){
                return false;
            }
        }
    }

    if (main_document_view->overview_page.has_value() && main_document_view->is_window_point_in_overview(normalized_click_pos)){

        DocumentPos overview_doc_pos = main_document_view->window_pos_to_overview_pos(normalized_click_pos);
        main_document_view->update_overview_highlighted_paper_with_position(overview_doc_pos);
    }

    if (is_in_middle_left_rect(click_pos)) {
        if (execute_macro_if_enabled(MIDDLE_LEFT_RECT_TAP_COMMAND)) {
            return true;
        }
    }
    if (is_in_middle_right_rect(click_pos)) {
        if (execute_macro_if_enabled(MIDDLE_RIGHT_RECT_TAP_COMMAND)) {
            return true;
        }
    }
    if (is_in_back_rect(click_pos)) {
        if (execute_macro_if_enabled(BACK_RECT_TAP_COMMAND)) {
            return true;
        }
    }
    if (is_in_forward_rect(click_pos)) {
        if (execute_macro_if_enabled(FORWARD_RECT_TAP_COMMAND)) {
            return true;
        }
    }

    if (is_in_edit_portal_rect(click_pos)) {
        if (execute_macro_if_enabled(TOP_CENTER_TAP_COMMAND)) {
            return true;
        }
    }

    if ((!main_document_view->is_ruler_mode()) && is_in_visual_mark_next_rect(click_pos)) {
        if (execute_macro_if_enabled(VISUAL_MARK_NEXT_TAP_COMMAND)) {
            return true;
        }
    }
    if ((!main_document_view->is_ruler_mode()) && is_in_visual_mark_prev_rect(click_pos)) {
        if (execute_macro_if_enabled(VISUAL_MARK_PREV_TAP_COMMAND)) {
            return true;
        }
    }

    if (TOUCH_MODE && main_document_view->get_overview_page()) {
        // in touch mode, quick tapping outside the overview window should close it
        auto window_pos = mapFromGlobal(QCursor::pos());
        NormalizedWindowPos nwp = main_document_view->window_to_normalized_window_pos({ window_pos.x(), window_pos.y() });

        if (!main_document_view->is_window_point_in_overview({ nwp.x, nwp.y })) {
            set_overview_page({}, false);
        }
    }


    main_document_view->clear_selected_text();
    clear_selection_indicators();
    main_document_view->clear_selected_object();
    clear_highlight_buttons();
    clear_search_buttons();
    main_document_view->cancel_search();
    stop_dragging();

    //if (current_widget != nullptr) {
    //    delete current_widget;
    //    current_widget = nullptr;
    //}
    text_command_line_edit_container->hide();

    last_quick_tap_position = mapFromGlobal(QCursor::pos());
    last_quick_tap_time = now;
    return false;
}

//void MainWidget::applicationStateChanged(Qt::ApplicationState state){
//    qDebug() << "application state changed\n";
//    if ((state == Qt::ApplicationState::ApplicationSuspended) || (state == Qt::ApplicationState::ApplicationInactive)){
//        persist();
//    }
//}

void MainWidget::android_handle_visual_mode() {
    //	last_hold_point
    if (main_document_view->is_ruler_mode()) {
        //opengl_widget->set_should_draw_vertical_line(false);
        main_document_view->exit_ruler_mode();
        main_document_view->clear_underline();
    }
    else {

        WindowPos pos;
        pos.x = last_hold_point.x();
        pos.y = last_hold_point.y();

        visual_mark_under_pos(pos);
    }
}

void MainWidget::update_position_buffer() {
    QPoint pos = QCursor::pos();
    QTime time = QTime::currentTime();

    position_buffer.push_back(std::make_pair(time, pos));

    if (position_buffer.size() > 5) {
        position_buffer.pop_front();
    }
}

bool MainWidget::is_flicking(QPointF* out_velocity) {
    // we never flick the background when a widget is being shown on top
    if (current_widget_stack.size() > 0) {
        return false;
    }
    if (main_document_view->get_overview_page()) {
        return false;
    }
    if (main_document_view->visible_object_scroll_data) {
        return false;
    }

    std::vector<float> speeds;
    std::vector<float> dts;
    std::vector<QPointF> velocities;
    if (position_buffer.size() == 0) {
        *out_velocity = QPointF(0, 0);
        return false;
    }
    for (int i = 0; i < position_buffer.size() - 1; i++) {
        float dt = (static_cast<float>(position_buffer[i].first.msecsTo(position_buffer[i + 1].first)) / 1000.0f);
        //        dts.push_back(dt);
        if (dt < (1.0f / 120.0f)) {
            dt = 1.0f / 120.0f;
        }

        if (dt != 0) {
            QPointF velocity = (position_buffer[i + 1].second - position_buffer[i].second).toPointF() / dt;
            velocities.push_back(velocity);
            speeds.push_back(sqrt(QPointF::dotProduct(velocity, velocity)));
        }
    }
    if (speeds.size() == 0) {
        *out_velocity = QPointF(0, 0);
        return false;
    }
    float average_speed = compute_average<float>(speeds);

    if (out_velocity) {
        *out_velocity = 2 * compute_average<QPointF>(velocities);
    }

    if (average_speed > 500.0f) {
        return true;
    }
    else {
        return false;
    }
}

bool MainWidget::handle_double_tap(QPoint pos) {
    WindowPos position;
    position.x = pos.x();
    position.y = pos.y();

    if (main_document_view->is_ruler_mode()) {
        NormalizedWindowPos nmp = main_document_view->window_to_normalized_window_pos(position);
        // don't want to accidentally smart jump when quickly moving the ruler
        if (screen()->orientation() == Qt::PortraitOrientation) {
            if (PORTRAIT_VISUAL_MARK_NEXT.contains(nmp) || PORTRAIT_VISUAL_MARK_PREV.contains(nmp)) {
                return false;
            }
        }
        else {
            if (LANDSCAPE_VISUAL_MARK_NEXT.contains(nmp) || LANDSCAPE_VISUAL_MARK_PREV.contains(nmp)) {
                return false;
            }
        }
    }
    overview_under_pos(position);
    /* smart_jump_under_pos(position); */
    return true;
}


void MainWidget::show_highlight_buttons() {
    //highlight_buttons = new HighlightButtons(this);
    get_highlight_buttons()->highlight_buttons->setHighlightType(get_current_selected_highlight_type());
    update_highlight_buttons_position();
    get_highlight_buttons()->show();
}

void MainWidget::clear_highlight_buttons() {
    if (highlight_buttons_) {
        get_highlight_buttons()->hide();
        //delete highlight_buttons;
        //highlight_buttons = nullptr;
    }
}

void MainWidget::handle_touch_highlight() {

    AbsoluteDocumentPos begin_abspos = selection_begin_indicator->get_docpos().to_absolute(doc());
    AbsoluteDocumentPos end_abspos = selection_end_indicator->get_docpos().to_absolute(doc());

    add_highlight_to_current_document(begin_abspos, end_abspos, select_highlight_type);

    invalidate_render();
}

void MainWidget::show_search_buttons() {
    get_search_buttons()->show();
}

void MainWidget::clear_search_buttons() {

    if (search_buttons_){
        get_search_buttons()->hide();
    }
}

void MainWidget::restore_default_config() {
    config_manager->restore_default();
    config_manager->restore_defaults_in_memory();
    //config_manager->deserialize(default_config_path, auto_config_path, user_config_paths);
    invalidate_render();
}


void MainWidget::persist_config() {
    config_manager->persist_config();
}

int MainWidget::get_current_colorscheme_index() {
    auto colormode = main_document_view->get_current_color_mode();
    if (colormode == ColorPalette::Normal) {
        return 0;
    }
    if (colormode == ColorPalette::Dark) {
        return 1;
    }
    if (colormode == ColorPalette::Custom) {
        return 2;
    }
    return -1;
}

void MainWidget::set_dark_mode() {
    if (main_document_view->get_current_color_mode() != ColorPalette::Dark) {
        COLOR_MODE = ColorMode::Dark;
        config_manager->handle_set_color_palette(this, ColorPalette::Dark);
    }
}

void MainWidget::set_light_mode() {
    if (main_document_view->get_current_color_mode() != ColorPalette::Normal) {
        COLOR_MODE = ColorMode::Light;
        config_manager->handle_set_color_palette(this, ColorPalette::Normal);
    }
}

void MainWidget::set_custom_color_mode() {
    if (main_document_view->get_current_color_mode() != ColorPalette::Custom) {
        COLOR_MODE = ColorMode::Custom;
        config_manager->handle_set_color_palette(this, ColorPalette::Custom);
    }
}


void MainWidget::set_color_mode_to_system_theme() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (USE_SYSTEM_THEME) {
        QStyleHints *style_hints =  QGuiApplication::styleHints();
        if (style_hints->colorScheme() == Qt::ColorScheme::Light){
            set_light_mode();
        }
        if (style_hints->colorScheme() == Qt::ColorScheme::Dark){
            if (USE_CUSTOM_COLOR_FOR_DARK_SYSTEM_THEME){
                set_custom_color_mode();
            }
            else{
                set_dark_mode();
            }
        }
#ifdef Q_OS_MACOS
        ensure_titlebar_colors_match_color_mode();
#endif

    }
#endif
}

void MainWidget::update_highlight_buttons_position() {
    std::string selected_highlight_uuid = main_document_view->get_selected_highlight_uuid();
    if (selected_highlight_uuid.size() > 0) {
        Highlight* hl = doc()->get_highlight_with_uuid(selected_highlight_uuid);
        if (hl) {
            AbsoluteDocumentPos hlpos;
            hlpos.x = hl->selection_begin.x;
            hlpos.y = hl->selection_begin.y;
            DocumentPos docpos = doc()->absolute_to_page_pos_uncentered(hlpos);

            WindowPos windowpos = main_document_view->document_to_window_pos_in_pixels_uncentered(docpos);
            get_highlight_buttons()->move(get_highlight_buttons()->pos().x(), windowpos.y - get_highlight_buttons()->height());
        }
    }
}


float MainWidget::get_align_to_top_offset() {
    if (ALIGN_LINK_DEST_TO_TOP) {
        return main_document_view->get_view_height() / 2.1f / main_document_view->get_zoom_level();
    }
    return 0;
}

void MainWidget::open_documentation_file_for_name(QString doctype, QString name) {
    std::string nameddest_link = "file://sioyek_documentation.pdf#nameddest=" + doctype.toStdString() + ":" + name.toStdString();
    Document* documentation_document = document_manager->get_document(documentation_path.get_path());

    if (!documentation_document->doc) {
        documentation_document->open();
    }

    ParsedUri parsed_uri = parse_uri(mupdf_context, documentation_document->doc, nameddest_link);
    int page = parsed_uri.page - 1;
    push_state();
    open_document_at_location(documentation_path.get_path(), page, 0, parsed_uri.y, {}, false);

    if (ALIGN_LINK_DEST_TO_TOP) {
        main_document_view->scroll_mid_to_top();
    }

    invalidate_render();
}

void MainWidget::show_documentation_with_title(QString doctype, QString title) {


    load_sioyek_documentation();

    if (!SHOW_DOCUMENTATION_IN_WIDGET) {

        const QJsonObject& command_name_to_file_name_map = sioyek_documentation_json_document["command_name_to_file_name_map"].toObject();
        const QJsonObject& config_name_to_file_name_map = sioyek_documentation_json_document["config_name_to_file_name_map"].toObject();

        QString file_title;
        if (doctype == "command") {
            file_title = command_name_to_file_name_map[title].toString();
        }
        else {
            file_title = config_name_to_file_name_map[title].toString();
        }
        open_documentation_file_for_name(doctype, file_title);
    }
    else {
        SioyekDocumentationTextBrowser* text_edit = new SioyekDocumentationTextBrowser(this);
        text_edit->setStyleSheet("QTextBrowser{" + get_status_stylesheet(false, DOCUMENTATION_FONT_SIZE) + "border-radius: 4px; padding: 10px;}\n" + get_scrollbar_stylesheet());
        text_edit->setTextInteractionFlags(Qt::LinksAccessibleByMouse);

        int w = width() * MENU_SCREEN_WDITH_RATIO;
        int h = height() * MENU_SCREEN_HEIGHT_RATIO;
        text_edit->setReadOnly(true);
        text_edit->move(width() / 2 - w / 2, height() / 2 - h / 2);
        text_edit->resize(w, h);

        QString documentation_url = "";
        QString doc;

        const QJsonObject& command_title_to_documentation_map = sioyek_documentation_json_document["command_title_to_documentation_map"].toObject();
        const QJsonObject& config_title_to_documentation_map = sioyek_documentation_json_document["config_title_to_documentation_map"].toObject();
        //for (auto key : command_title_to_documentation_map.keys()) {
        //    qDebug() << key;
        //    qDebug() << (key == title);
        //}

        if (doctype == "command") {
            if (command_title_to_documentation_map.contains(title)) {
                doc = command_title_to_documentation_map[title].toString();
            }
        }
        else if (doctype == "config") {
            if (config_title_to_documentation_map.contains(title)) {
                doc = config_title_to_documentation_map[title].toString();
            }
        }

        text_edit->setSource(documentation_url, QTextDocument::ResourceType::MarkdownResource);

        text_edit->setMarkdown(doc);
        push_current_widget(text_edit);
        text_edit->show();
    }


}

void MainWidget::show_command_documentation(QString command_name) {
    if (SHOW_DOCUMENTATION_IN_WIDGET) {
        SioyekDocumentationTextBrowser* text_edit = new SioyekDocumentationTextBrowser(this);
        text_edit->setStyleSheet(get_status_stylesheet(false, DOCUMENTATION_FONT_SIZE));
        text_edit->setTextInteractionFlags(Qt::LinksAccessibleByMouse);

        int w = width() / 2;
        int h = height() / 2;
        text_edit->setReadOnly(true);
        text_edit->move(width() / 2 - w / 2, height() / 2 - h / 2);
        text_edit->resize(w, h);

        QString documentation_url = "";
        QString doucmentation_name;
        auto doc = get_command_documentation(command_name, &documentation_url, &doucmentation_name);

        text_edit->setSource(documentation_url, QTextDocument::ResourceType::MarkdownResource);

        text_edit->setMarkdown(doc);
        push_current_widget(text_edit);
        text_edit->show();
    }
    else {
        QString documentation_url = "";
        QString doucmentation_name;
        auto doc = get_command_documentation(command_name, &documentation_url, &doucmentation_name);
        QString doctype = documentation_url.startsWith("conf") ? "config" : "command";
        open_documentation_file_for_name(doctype, doucmentation_name);
    }
}


bool MainWidget::show_contextual_context_menu(QString default_context_menu) {
    auto p = mapFromGlobal(QCursor::pos());
    WindowPos window_pos = WindowPos{
        p.x(), p.y()
    };
    AbsoluteDocumentPos abs_pos = window_pos.to_absolute(main_document_view);
    NormalizedWindowPos normal_pos = window_pos.to_window_normalized(main_document_view);


    if (main_document_view->get_overview_page()) {
        if (main_document_view->is_window_point_in_overview({ normal_pos.x, normal_pos.y })) {
            if (CONTEXT_MENU_ITEMS_FOR_OVERVIEW.size() > 0) {
                show_context_menu(QString::fromStdWString(CONTEXT_MENU_ITEMS_FOR_OVERVIEW));
                return true;
            }
        }
    }
    else if (dv()->is_pos_inside_selected_text(window_pos)) {
        if (CONTEXT_MENU_ITEMS_FOR_SELECTED_TEXT.size() > 0) {
            show_context_menu(QString::fromStdWString(CONTEXT_MENU_ITEMS_FOR_SELECTED_TEXT));
            return true;
        }
    }
    else {
        std::string sel_highlight = main_document_view->get_highlight_uuid_in_pos(window_pos);
        std::string sel_bookmark = doc()->get_bookmark_uuid_at_pos(abs_pos);

        if (sel_highlight.size() > 0) {
            main_document_view->set_selected_highlight_uuid(sel_highlight);
            if (CONTEXT_MENU_ITEMS_FOR_HIGHLIGHTS.size() > 0) {
                show_context_menu(QString::fromStdWString(CONTEXT_MENU_ITEMS_FOR_HIGHLIGHTS));
                return true;
            }
        }
        if (sel_bookmark.size() > 0) {
            main_document_view->set_selected_bookmark_uuid(sel_bookmark);
            if (CONTEXT_MENU_ITEMS_FOR_BOOKMARKS.size() > 0) {
                show_context_menu(QString::fromStdWString(CONTEXT_MENU_ITEMS_FOR_BOOKMARKS));
                return true;
            }
        }
    }
    if (default_context_menu.size() > 0) {
        show_context_menu(default_context_menu);
        return true;
    }
    return false;
}

void MainWidget::show_context_menu(QString menu_string) {
    // since the context menu overrides the main widget's event loop, the validate_render
    // call below is necessary to ensure that changes due to showing the context menu
    // (e.g. the right clicked highlight being selected) are displayed
    validate_render();

    auto menu = parse_menu_string(this, "menu", menu_string);
    show_recursive_context_menu(std::move(menu));
}

QMenu* MainWidget::get_menu_from_items(std::unique_ptr<MenuItems> items, QWidget* parent) {
    QMenu* contextMenu = new QMenu(QString::fromStdWString(items->name), parent);
    QStringList command_names;

    std::vector<QAction*> actions;
    for (auto&& subitem : items->items) {

        if (std::holds_alternative<std::unique_ptr<Command>>(subitem)) {
            std::unique_ptr<Command> subcommand = std::get<std::unique_ptr<Command>>(std::move(subitem));
            std::string command_name = subcommand->get_human_readable_name();

            QAction* action = new QAction(QString::fromStdString(command_name), contextMenu);
            actions.push_back(action);
            connect(action, &QAction::triggered, [&, command_name, subcommand = std::move(subcommand)]() mutable {
                //execute_macro_if_enabled(command_name.toStdWString());
                handle_command_types(std::move(subcommand), 0);
                invalidate_render();
            });
            contextMenu->addAction(action);
        }
        else {
            std::unique_ptr<MenuItems> submenu_ = std::get<std::unique_ptr<MenuItems>>(std::move(subitem));
            QMenu* submenu = get_menu_from_items(std::move(submenu_), contextMenu);
            contextMenu->addMenu(submenu);

        }
    }
    return contextMenu;
}

void MainWidget::show_recursive_context_menu(std::unique_ptr<MenuItems> items) {
    // since the context menu overrides the main widget's event loop, the validate_render
    // call below is necessary to ensure that changes due to showing the context menu
    // (e.g. the right clicked highlight being selected) are displayed
    validate_render();

    context_menu_right_click_pos = QCursor::pos();

    QMenu* context_menu = get_menu_from_items(std::move(items), this);

    context_menu->exec(QCursor::pos());
    context_menu_right_click_pos = {};

    delete context_menu;
}


void MainWidget::handle_documentation_search() {

    load_sioyek_documentation();
    db_manager->index_documentation(sioyek_documentation_json_document);

    auto search_widget = DocumentationSearchWidget::create(this);
    search_widget->set_select_fn([&, search_widget](int index) {
            FulltextSearchResult result = search_widget->result_model->search_results[index];
            QStringList parts = QString::fromStdWString(result.document_title).split("/");

            pop_current_widget();
            if (parts[0] == "command") {
                show_documentation_with_title("command", parts[1]);
            }
            else if (parts[0] == "config") {

                show_documentation_with_title("config", parts[1]);
            }
            //qDebug() << result.document_title;
        });

    set_current_widget(search_widget);
    show_current_widget();
}

void MainWidget::handle_fulltext_search(std::wstring maybe_file_checksum) {

    auto search_widget = FulltextSearchWidget::create(this, maybe_file_checksum);
    search_widget->set_select_fn([&, search_widget](int index) {
            FulltextSearchResult result = search_widget->result_model->search_results[index];
            std::wstring document_path = document_manager->get_path_from_hash(result.document_checksum).value_or(L"");

            QString snippet = QString::fromStdWString(result.snippet);
            int first_index = snippet.indexOf("SIOYEK_MATCH_BEGIN");
            int last_index = snippet.lastIndexOf("SIOYEK_MATCH_END");

            if (first_index != -1 && last_index != -1) {
                snippet = snippet.mid(first_index, last_index - first_index);
            }

            snippet = snippet.replace("SIOYEK_MATCH_BEGIN", "");
            snippet = snippet.replace("SIOYEK_MATCH_END", "");
            
            if (snippet.startsWith("...")) {
                snippet = snippet.mid(3);
            }

            if (snippet.endsWith("...")) {
                snippet = snippet.mid(0, snippet.size() - 3);
            }

            if (document_path.size() > 0) {
                open_document_at_location(Path(document_path), result.page, {}, {}, {});
                main_document_view->focus_page_text(result.page, snippet.toStdWString());
                invalidate_render();
            }
            pop_current_widget();
            //qDebug() << "selected " << result.snippet;
        });

    set_current_widget(search_widget);
    show_current_widget();
}

void MainWidget::index_current_document_for_fulltext_search(bool async) {
    bool super_fast_search_index_is_ready = doc()->is_super_fast_index_ready();

    if (!super_fast_search_index_is_ready) {
        show_error_message(L"Super fast search index is not ready.");
    }
    else {
        if (async) {
            Document* current_document = doc();
            background_task_manager->add_task([this, current_document]() {
                std::string document_checksum = current_document->get_checksum();
                db_manager->index_document(document_checksum, current_document->get_super_fast_index(), current_document->get_super_fast_page_begin_indices());
                }, this);
        }
        else {
            std::string document_checksum = doc()->get_checksum();
            db_manager->index_document(document_checksum, doc()->get_super_fast_index(), doc()->get_super_fast_page_begin_indices());
        }
    }
}


void MainWidget::free_renderer_resources_for_current_document() {
    pdf_renderer->free_all_resources_for_document(doc()->get_path());

}

void MainWidget::handle_debug_command() {
}

std::vector<WindowRect> MainWidget::get_largest_empty_rects() {
    const int REDUCE_FACTOR = 8;
#ifdef SIOYEK_OPENGL_BACKEND
    QImage image = opengl_widget->grabFramebuffer();
#else
    QPixmap pixmap(size());
    render(&pixmap, QPoint(), QRegion(rect()));
    QImage image = pixmap.toImage();
#endif
    image = image.scaled(image.width() / REDUCE_FACTOR, image.height() / REDUCE_FACTOR);

    std::unordered_map<int, int> counts;

    QRgb mode_color = image.pixel(QPoint(0, 0));
    int mode_count = 1;

    for (int j = 0; j < image.height(); j++) {
        for (int i = 0; i < image.width(); i++) {
            QRgb pixel = image.pixel(QPoint(i, j));

            if (counts.find(pixel) == counts.end()) {
                counts[pixel] = 1;
            }
            else {
                counts[pixel] += 1;
            }
            if (counts[pixel] > mode_count) {
                mode_count = counts[pixel];
                mode_color = pixel;
            }
        }
    }

    std::vector<std::vector<bool>> binary_matrix;
    for (int j = 0; j < image.height(); j++) {
        std::vector<bool> row;
        for (int i = 0; i < image.width(); i++) {
            QRgb pixel = image.pixel(QPoint(i, j));
            if (pixel == mode_color) {
                row.push_back(true);
            }
            else {
                row.push_back(false);
            }
        }
        binary_matrix.push_back(row);
    }

    std::vector<MaximumRectangleResult> largest_rects = maximum_rectangle(binary_matrix);


    std::vector<WindowRect> window_rects;
    for (auto largest_rect : largest_rects) {
        WindowRect window_rect;
        window_rect.x0 = largest_rect.begin_col * REDUCE_FACTOR + AUTO_BOOKMARK_HORIZONTAL_MARGIN;
        window_rect.x1 = largest_rect.end_col * REDUCE_FACTOR - AUTO_BOOKMARK_HORIZONTAL_MARGIN;
        window_rect.y0 = largest_rect.begin_row * REDUCE_FACTOR + AUTO_BOOKMARK_VERTICAL_MARGIN;
        window_rect.y1 = largest_rect.end_row * REDUCE_FACTOR - AUTO_BOOKMARK_VERTICAL_MARGIN;
        window_rects.push_back(window_rect);
    }
    return window_rects;
}

void MainWidget::show_command_menu() {
    std::vector<QString> command_names;
    std::vector<QStringList> command_keybinds;
    std::unordered_map<std::string, std::vector<std::string>> command_key_mappings = input_handler->get_command_key_mappings();

    // also add key mapping to aliased commands
    for (auto [original_name, alias] : command_manager->command_aliases) {
        auto original_it = command_key_mappings.find(original_name);
        auto alias_it = command_key_mappings.find(alias);
        if ((original_it == command_key_mappings.end()) && (alias_it != command_key_mappings.end())) {
            command_key_mappings[original_name] = command_key_mappings[alias];
        }
        if ((original_it != command_key_mappings.end()) && (alias_it == command_key_mappings.end())) {
            command_key_mappings[alias] = command_key_mappings[original_name];
        }
    }

    QStringList all_command_names = command_manager->get_all_command_names();
    for (int i = 0; i < all_command_names.size(); i++) {
        QStringList keybindings;
        auto iterator = command_key_mappings.find(all_command_names[i].toStdString());
        if (iterator != command_key_mappings.end()) {
            for (int j = 0; j < iterator->second.size(); j++) {
                keybindings.append(QString::fromStdString(iterator->second[j]).toHtmlEscaped());
            }
        }
        command_names.push_back(all_command_names[i]);
        command_keybinds.push_back(keybindings);
    }

    CommandSelectorWidget* command_selector_widget = CommandSelectorWidget::from_commands(command_names, command_keybinds, this);
    command_selector_widget->set_select_fn([&, command_selector_widget](int index){

        std::string query = command_selector_widget->line_edit->text().toStdString();
        bool is_numeric = false;
        QString::fromStdString(query).toInt(&is_numeric);
        if (is_numeric){
            int page_number = QString::fromStdString(query).toInt();
            main_document_view->goto_page(page_number - 1);
            // open_document_at_location(doc()->get_path(), page_number, {}, {}, {});
            invalidate_render();
            pop_current_widget();
            setFocus();
        }
        else{
            QString command_name = command_selector_widget->get_command_with_index(index);
            pop_current_widget();
            setFocus();
            on_command_done(command_name.toStdString(), query);
        }

        });
    set_current_widget(command_selector_widget);
    show_current_widget();
}

void MainWidget::add_chunk_to_bookmark(Document* document, std::string bookmark_uuid, QString chunk) {
    int bookmark_index = document->get_bookmark_index_with_uuid(bookmark_uuid);
    if (bookmark_index >= 0) {
        BookMark& bm = document->get_bookmarks()[bookmark_index];
        bm.description += chunk.toStdWString();

        invalidate_render();
    }
}

bool MainWidget::ensure_super_fast_search_index() {
    if (doc()->get_super_fast_index().size() > 0) {
        return true;
    }

    if (!SUPER_FAST_SEARCH) {
        show_error_message(L"This feature requires super_fast_search to be enabled.");
    }
    else if (doc()->get_is_indexing()) {
        show_error_message(L"Super fast index is not ready yet.");
    }
    else {
        show_error_message(L"Super fast index not available for unknown reason.");
    }


    return false;
}

void MainWidget::handle_bookmark_summarize_query(std::wstring bookmark_uuid_) {
    if (!ensure_super_fast_search_index()) {
        return;
    }
    const std::wstring& index = doc()->get_super_fast_index();
    int first_page_end_index = doc()->get_super_fast_page_begin_indices()[1] - 1;

    std::string bookmark_uuid = utf8_encode(bookmark_uuid_);
    int ind = doc()->get_bookmark_index_with_uuid(bookmark_uuid);
    doc()->get_bookmarks()[ind].description += L"\n\n";
    sioyek_network_manager->summarize(this,  index, first_page_end_index, [this, bookmark_uuid, document=doc()](QString chunk) {
        add_chunk_to_bookmark(document, bookmark_uuid, chunk);
        },
        [this, bookmark_uuid, document=doc()]() {
            //int bookmark_index = document->get_bookmark_index_with_uuid(bookmark_uuid);
            BookMark* bm = document->get_bookmark_with_uuid(bookmark_uuid);
            if (bm) {
                bm->description = replace_verbatim_links(bm->description);
                document->update_bookmark_text(bookmark_uuid, bm->description, bm->font_size);
                on_bookmark_edited(bm->uuid);
            }
        });
}

void MainWidget::handle_bookmark_ask_query(std::wstring query, std::wstring bookmark_uuid_) {
    const std::wstring& index = doc()->get_super_fast_index();
    std::string bookmark_uuid = utf8_encode(bookmark_uuid_);
    int ind = doc()->get_bookmark_index_with_uuid(bookmark_uuid);
    doc()->get_bookmarks()[ind].description += L"\n\n";
    sioyek_network_manager->semantic_ask(this, QString::fromStdWString(query), index, [this, bookmark_uuid, document=doc()](QString chunk) {
        add_chunk_to_bookmark(document, bookmark_uuid, chunk);
        },
        [this, bookmark_uuid, document=doc()]() {
            //int bookmark_index = document->get_bookmark_index_with_uuid(bookmark_uuid);
            BookMark* bm = document->get_bookmark_with_uuid(bookmark_uuid);
            if (bm) {
                document->update_bookmark_text(bookmark_uuid, bm->description, bm->font_size);
                on_bookmark_edited(bm->uuid);
            }
        });
}

std::optional<ShellOutputBookmark> MainWidget::get_shell_output_bookmark_with_uuid(std::string uuid){
    for (int i = 0; i < shell_output_bookmarks.size(); i++){
        if (shell_output_bookmarks[i].uuid == uuid){
            return shell_output_bookmarks[i];
        }
    }
    return {};
}

void MainWidget::remove_finished_shell_bookmark_with_index(int index){
    if (index >= 0 && index < shell_output_bookmarks.size()){
        ShellOutputBookmark& shell_output_bookmark = shell_output_bookmarks[index];
        shell_output_bookmark.watcher->removePath(shell_output_bookmark.output_file->fileName());
        shell_output_bookmark.watcher->deleteLater();
        shell_output_bookmark.output_file->remove();
        shell_output_bookmark.output_file->deleteLater();

        if (shell_output_bookmark.document_content_file){
            shell_output_bookmark.document_content_file->remove();
            shell_output_bookmark.document_content_file->deleteLater();
        }

        if (shell_output_bookmark.image_file){
            shell_output_bookmark.image_file->remove();
            shell_output_bookmark.image_file->deleteLater();
        }

        shell_output_bookmark.process->deleteLater();
        shell_output_bookmarks.erase(shell_output_bookmarks.begin() + index);
    }
}

void MainWidget::remove_finished_shell_bookmarks(){
#ifndef SIOYEK_MOBILE
    std::vector<int> indices_to_delete;
    for (int i = 0; i < shell_output_bookmarks.size(); i++){
        if (!is_process_still_running(shell_output_bookmarks[i].pid)){
            on_bookmark_shell_output_updated(shell_output_bookmarks[i].uuid, shell_output_bookmarks[i].output_file->fileName());
            indices_to_delete.push_back(i);
        }
    }
    for (int j = indices_to_delete.size() - 1; j >= 0; j--){
        int index = indices_to_delete[j];
        remove_finished_shell_bookmark_with_index(index);
    }
#endif
}

void MainWidget::on_bookmark_shell_output_updated(std::string bookmark_uuid, QString file_path) {
    QFile file(file_path);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        QString content = stream.readAll();
        file.close();
        BookMark* bm = doc()->get_bookmark_with_uuid(bookmark_uuid);
        if (bm) {
            std::optional<ShellOutputBookmark> shell_output_data = get_shell_output_bookmark_with_uuid(bookmark_uuid);
            if (shell_output_data) {
                bm->description = (shell_output_data->style_string + " " + content).toStdWString();
                doc()->update_bookmark_text(bookmark_uuid, bm->description, bm->font_size);
                on_bookmark_edited(bm->uuid);
                invalidate_render();
            }
        }
    }
}

void MainWidget::handle_bookmark_shell_command(QString text, std::string pending_uuid, QString text_arg){
#ifndef SIOYEK_MOBILE

    if (text.startsWith("#shell")){
        text = text.mid(QString("#shell").size() + 1); // also skip the space
    }

    QTemporaryFile* file_content_temp_file = nullptr;
    QTemporaryFile* image_temp_file = nullptr;
    QString style_string = "";
    if (text.startsWith("#") && (!text.startsWith("#shell"))){
        int first_space_index = text.indexOf(" ");
        if (first_space_index >= 0){
            style_string = text.mid(0, first_space_index).trimmed();
            text = text.mid(first_space_index + 1);
        }
    }

    if (text.indexOf("%{text}") != -1){
        text = text.replace("%{text}", text_arg);
    }
    if (text.indexOf("%{document_text_file}") != -1){
        file_content_temp_file = new QTemporaryFile();
        file_content_temp_file->open();
        if (doc()->get_super_fast_index().size() > 0){
            file_content_temp_file->write(utf8_encode(doc()->get_super_fast_index()).c_str());
            text = text.replace("%{document_text_file}", file_content_temp_file->fileName());
        }
    }
    if (text.indexOf("%{current_page_begin_index}") != -1){
        int current_page = main_document_view->get_current_page_number();
        int page_begin_index = doc()->get_super_fast_page_begin_indices()[current_page];
        text = text.replace("%{current_page_begin_index}", QString::number(page_begin_index));
    }
    if (text.indexOf("%{current_page_end_index}") != -1){
        int next_page = main_document_view->get_current_page_number() + 1;

        if (next_page < doc()->get_super_fast_page_begin_indices().size()){
            int page_begin_index = doc()->get_super_fast_page_begin_indices()[next_page];
            text = text.replace("%{current_page_end_index}", QString::number(page_begin_index));
        }
        else{
            text = text.replace("%{current_page_end_index}", QString::number(doc()->get_super_fast_index().size()));
        }
    }
    if (text.indexOf("%{bookmark_image_file}") != -1){
        BookMark* bm = doc()->get_bookmark_with_uuid(pending_uuid);
        if (bm){
            std::optional<AbsoluteRect> rect = bm->get_rectangle();
            if (rect){
                WindowRect window_rect = rect->to_window(dv());
                QRect window_qrect = window_rect.to_qrect();
                QPixmap pixmap;

                float ratio = QGuiApplication::primaryScreen()->devicePixelRatio();
                pixmap = QPixmap(static_cast<int>(window_qrect.width() * ratio), static_cast<int>(window_qrect.height() * ratio));
                pixmap.setDevicePixelRatio(ratio);

                render(&pixmap, QPoint(), QRegion(window_qrect));

                image_temp_file = new QTemporaryFile();
                image_temp_file->open();
                image_temp_file->setFileName(image_temp_file->fileName() + ".png");
                pixmap.save(image_temp_file->fileName());

                text = text.replace("%{bookmark_image_file}", image_temp_file->fileName());
            }
        }

    }
    if (text.indexOf("%{selected_text}") != -1){
        text = text.replace("%{selected_text}", QString::fromStdWString(main_document_view->get_selected_text()));
    }

    // QString command = text.mid(QString("#shell").size()).trimmed();
    QStringList command_parts = QProcess::splitCommand(text);
    if (command_parts.size() > 0){

        QString command_name = command_parts.at(0);
        QStringList command_args = command_parts.mid(1);
        // qint64 pid = -1; 
        // QProcess::startDetached(command_name, command_args, QString(), &pid);


        QTemporaryFile* temp_file = new QTemporaryFile();
        temp_file->open();
        QFileSystemWatcher* watcher = new QFileSystemWatcher();
        watcher->addPath(temp_file->fileName());
        connect(watcher, &QFileSystemWatcher::fileChanged, [this, pending_uuid](const QString& path) {

                on_bookmark_shell_output_updated(pending_uuid, path);
                });

        qint64 pid = -1;
        QProcess* process = new QProcess();
        process->setProgram(command_name);
        process->setArguments(command_args);
        process->setStandardOutputFile(temp_file->fileName());
        process->startDetached(&pid);

        ShellOutputBookmark shell_output_bookmark;
        shell_output_bookmark.pid = pid;
        shell_output_bookmark.uuid = pending_uuid;
        shell_output_bookmark.output_file = std::move(temp_file);
        shell_output_bookmark.watcher = watcher;
        shell_output_bookmark.process = process;
        shell_output_bookmark.document_content_file = file_content_temp_file;
        shell_output_bookmark.image_file = image_temp_file;
        shell_output_bookmark.style_string = style_string;

        shell_output_bookmarks.push_back(shell_output_bookmark);

    }
#endif
}

std::wstring MainWidget::handle_freetext_bookmark_perform(const std::wstring& text, const std::string& pending_uuid) {
    std::wstring result = L"";
    if (text.size() > 0) {
        std::string uuid = doc()->add_pending_bookmark(pending_uuid, text);
        on_new_bookmark_added(uuid);
        result = utf8_decode(uuid);
        main_document_view->set_selected_bookmark_uuid("");
        QString qtext = QString::fromStdWString(text);

        if (text.size() > 2 && text.substr(0, 2) == L"? ") {
            handle_bookmark_ask_query(text.substr(2, text.size()-2), result);
        }
        else if (qtext.startsWith("#summarize")) {
            handle_bookmark_summarize_query(result);
        }
        else if (qtext.startsWith("#shell")) {
            handle_bookmark_shell_command(qtext, pending_uuid);
        }
        else if (QString::fromStdWString(text).startsWith("@")){
            // the text after the @ and before the first space is the command name
            QString command_name = qtext.mid(1).split(" ").at(0);
            QString text_arg = qtext.mid(1 + command_name.size()).trimmed();
            if (SHELL_BOOKMARK_COMMANDS.find(command_name.toStdWString()) != SHELL_BOOKMARK_COMMANDS.end()){
                QString command_string = QString::fromStdWString(SHELL_BOOKMARK_COMMANDS[command_name.toStdWString()]);
                QString equivalent_shell_command = "#shell " + command_string;
                handle_bookmark_shell_command(equivalent_shell_command, pending_uuid, text_arg);
            }
        }
    }
    else {
        doc()->undo_pending_bookmark(pending_uuid);
        result = L"";
    }

    clear_selected_rect();
    invalidate_render();
    return result;
}

void MainWidget::focus_on_high_quality_text_being_read() {
    if ((media_player != nullptr) && (media_player->isPlaying()) && high_quality_play_state.has_value()) {
        float current_time = static_cast<float>(media_player->position()) / 1000.0f;
        int index = -1;
        for (int i = 0; i < high_quality_play_state->timestamps.size(); i++) {
            if (high_quality_play_state->timestamps[i] > current_time) {
                index = i;
                break;
            }
        }
        if (index >= 0) {
            PagelessDocumentRect rect_to_focus = high_quality_play_state->line_rects[index];
            if (!(rect_to_focus == high_quality_play_state->last_focused_rect)) {
                main_document_view->focus_rect(DocumentRect(rect_to_focus, high_quality_play_state->page_number));
                high_quality_play_state->last_focused_rect = rect_to_focus;
                invalidate_render();
            }
        }

    }
}

void MainWidget::export_command_names(std::wstring file_path){
    QFile output_file(QString::fromStdWString(file_path));
    if (output_file.open(QIODeviceBase::WriteOnly)){
        QStringList command_names = command_manager->get_all_command_names();
        for (auto command_name : command_names){
            output_file.write((command_name + "\n").toUtf8());
        }

        output_file.close();
    }
}

void MainWidget::export_config_names(std::wstring file_path){
    QFile output_file(QString::fromStdWString(file_path));
    if (output_file.open(QIODeviceBase::WriteOnly)){
        std::vector<Config*> configs = config_manager->get_configs();
        //QStringList command_names = command_manager->get_all_command_names();
        for (auto config : configs){
            output_file.write((QString::fromStdWString(config->name) + "\n").toUtf8());
        }

        output_file.close();
    }
}

void MainWidget::export_default_config_file(std::wstring file_path){
    load_sioyek_documentation();

    QFile output_file(QString::fromStdWString(file_path));
    if (output_file.open(QIODeviceBase::WriteOnly)){
        std::vector<Config*> configs = config_manager->get_configs();
        for (auto config : configs){
            QString config_name = QString::fromStdWString(config->name);
            if (config->default_value_string.size() > 0){
                output_file.write((config_name + " " + QString::fromStdWString(config->default_value_string) + "\n\n").toUtf8());
            }
            else{
                output_file.write(("# " + config_name + " " + QString::fromStdWString(config->default_value_string) + "\n\n").toUtf8());
            }
        }

        output_file.close();
    }
}

void MainWidget::download_paper_under_cursor(bool use_last_touch_pos) {
    if (is_scratchpad_mode()){
        return;
    }
    ensure_internet_permission();

    QPoint mouse_pos;
    if (use_last_touch_pos) {
        mouse_pos = last_hold_point;
    }
    else {
        mouse_pos = mapFromGlobal(cursor_pos());
    }
    WindowPos pos(mouse_pos.x(), mouse_pos.y());
    DocumentPos doc_pos = pos.to_document(main_document_view);
    // DocumentPos doc_pos = main_document_view->get_document_pos_under_window_pos(pos);
    std::optional<QString> paper_name = dv()->get_paper_name_under_pos(doc_pos, true);


    if (paper_name) {
        QString bib_text = clean_bib_item(paper_name.value());
        std::string pending_portal_handle = "";

        if (get_default_paper_download_finish_action() == PaperDownloadFinishedAction::Portal) {
            AbsoluteDocumentPos source_position;
            if (main_document_view->get_overview_page() && main_document_view->get_overview_source_rect())  {
                source_position = main_document_view->get_overview_source_rect()->center();
            }
            else {
                source_position = doc_pos.to_absolute(doc());
            }

            pending_portal_handle = main_document_view->create_pending_download_portal(source_position, bib_text.toStdWString());
        }
        if (TOUCH_MODE) {
            show_text_prompt(bib_text.toStdWString(), [this, pending_portal_handle](std::wstring text) {
                download_paper_with_name(text, {}, pending_portal_handle);
                });
        }
        else {
            download_paper_with_name(bib_text.toStdWString(), {}, pending_portal_handle);
        }
    }
}

void MainWidget::on_paper_downloaded(QNetworkReply* reply) {
    QByteArray pdf_data = reply->readAll();
    QString header = reply->header(QNetworkRequest::ContentTypeHeader).toString();

    if ((pdf_data.size() == 0) || (!header.startsWith("application/pdf"))) {
        return;
    }

    QString file_name = reply->url().fileName();
    if (!file_name.endsWith(".pdf") && header.startsWith("application/pdf")) {
        file_name = file_name + ".pdf";
    }

    if (AUTO_RENAME_DOWNLOADED_PAPERS && (!reply->property("sioyek_actual_paper_name").isNull())) {
        QString detected_file_name = get_file_name_from_paper_name(reply->property("sioyek_actual_paper_name").toString());

        if (detected_file_name.size() > 0) {
            QString extension = file_name.split(".").back();
            file_name = detected_file_name + "." + extension;
        }
    }
    //reply->property("sioyek_paper_name").toString().replace("/", "_")

    PaperDownloadFinishedAction finish_action = get_paper_download_action_from_string(reply->property("sioyek_finish_action").toString());
    QString path = QString::fromStdWString(downloaded_papers_path.slash(file_name.toStdWString()).get_path());
    QDir dir;
    dir.mkpath(QString::fromStdWString(downloaded_papers_path.get_path()));

    QFile file(path);
    bool opened = file.open(QIODeviceBase::WriteOnly);
    if (opened) {
        file.write(pdf_data);
        file.close();
        if (finish_action == PaperDownloadFinishedAction::Portal) {
            //std::string checksum = this->checksummer->get_checksum(path.toStdWString());

            std::string new_portal_uuid = this->main_document_view->finish_pending_download_portal(
                reply->property("sioyek_paper_name").toString().toStdWString(),
                path.toStdWString()
            );

            if (new_portal_uuid.size() > 0){
                on_new_portal_added(new_portal_uuid);
            }

        }
        else {
#ifdef SIOYEK_MOBILE
            // todo: maybe show a dialog asking the user if they want to open the downloaded document
            push_state();
            open_document(path.toStdWString());
#else
            MainWidget* new_window = handle_new_window();
            new_window->open_document(path.toStdWString());
#endif
        }
    }

}

void MainWidget::read_current_line() {

    if (high_quality_play_state.has_value()) {
        handle_stop_reading();
        handle_start_reading_high_quality();
    }
    else {

        if (!word_by_word_reading){
            if (is_reading){
                is_reading = false;
                prev_tts_state = "Stopped";
                get_tts()->stop();
            }
        }

        std::wstring selected_line_text = main_document_view->get_selected_line_text().value_or(L"");
        //std::wstring text = main_document_view->get_selected_line_text().value_or(L"");
        tts_text.clear();
        tts_corresponding_line_rects.clear();
        tts_corresponding_char_rects.clear();

        int page_number = main_document_view->get_vertical_line_page();
        AbsoluteRect ruler_rect = main_document_view->get_ruler_rect().value_or(fz_empty_rect);
        int maximum_size = get_tts()->get_maximum_tts_text_size();
        int index_into_page = doc()->get_page_text_and_line_rects_after_rect(
            page_number,
            maximum_size,
            ruler_rect,
            tts_text,
            tts_corresponding_line_rects,
            tts_corresponding_char_rects);


        //fz_stext_page* stext_page = doc()->get_stext_with_page_number(page_number);
        //std::vector<fz_stext_char*> flat_chars;
        //get_flat_chars_from_stext_page(stext_page, flat_chars, true);

        get_tts()->set_rate(TTS_RATE);
        if (word_by_word_reading) {
            int page_offset = doc()->get_page_offset_into_super_fast_index(page_number);
            int current_offset_into_document = page_offset + index_into_page;
            if (tts_text.size() > 0) {
                get_tts()->say(QString::fromStdWString(tts_text), current_offset_into_document);
            }
        }
        else {
            get_tts()->say(QString::fromStdWString(selected_line_text));
        }

        // last_page_read = page_number;
        // last_index_into_page_read = index_into_page;

        is_reading = true;
    }
}

void MainWidget::handle_start_reading() {

    int line_index = main_document_view->get_line_index();
    if (line_index == -1) {
        show_error_message(L"You must select a line first (e.g. by right clicking on a line)");
        return;
    }

    is_reading = true;
    read_current_line();
    if (TOUCH_MODE) {
        AudioUI * audio_ui_widget = new AudioUI(this);
        set_current_widget(audio_ui_widget);
        show_current_widget();
    }
}

void MainWidget::handle_stop_reading() {
    if (high_quality_play_state.has_value()) {
        if (media_player) {
            media_player->stop();
        }
        high_quality_play_state = {};
    }
    else {
        is_reading = false;

        get_tts()->stop();

        if (TOUCH_MODE) {
            pop_current_widget();
        }
    }
}

void MainWidget::handle_toggle_reading() {
    if (is_reading){
        handle_stop_reading();
    }
    else{
        handle_start_reading();
    }
}

void MainWidget::handle_play() {
    // I think qt's current implementation of pause/resume is a little buggy
    // for example it sometimes does not fire sayingWord events after being resumed
    // so for now we just start reading from start of the current line instead of an actual resume
    handle_start_reading();

    //if (tts_has_pause_resume_capability) {
    //    is_reading = true;
    //    if (get_tts()->state() == QTextToSpeech::Paused) {
    //        get_tts()->resume();
    //    }
    //    else {
    //        move_visual_mark(1);
    //        invalidate_render();
    //    }
    //}
    //else {
    //    handle_start_reading();
    //}
}

void MainWidget::handle_pause() {
    is_reading = false;
    get_tts()->pause();
}
bool MainWidget::should_show_status_label() {
    float prog;
    if (is_network_manager_running()) {
        return true;
    }

    if (TOUCH_MODE) {
        if (pending_command_instance && pending_command_instance->next_requirement(this)->type == RequirementType::Point){
            return true;
        }
        if (current_widget_stack.size() > 0 || main_document_view->get_is_searching(&prog) || main_document_view->is_pending_link_source_filled()) {
            return true;
        }
        return false;
    }
    else {
        return STATUSBAR || main_document_view->get_is_searching(&prog);
    }
}

bool MainWidget::is_in_middle_left_rect(WindowPos pos) {
    NormalizedWindowPos nwp = main_document_view->window_to_normalized_window_pos(pos);
    if (screen()->orientation() == Qt::PortraitOrientation) {
        return PORTRAIT_MIDDLE_LEFT_UI_RECT.contains(nwp);
    }
    else {
        return LANDSCAPE_MIDDLE_LEFT_UI_RECT.contains(nwp);
    }
}

bool MainWidget::is_in_middle_right_rect(WindowPos pos) {
    NormalizedWindowPos nwp = main_document_view->window_to_normalized_window_pos(pos);
    if (screen()->orientation() == Qt::PortraitOrientation) {
        return PORTRAIT_MIDDLE_RIGHT_UI_RECT.contains(nwp);
    }
    else {
        return LANDSCAPE_MIDDLE_RIGHT_UI_RECT.contains(nwp);
    }
}

bool MainWidget::is_in_back_rect(WindowPos pos) {
    NormalizedWindowPos nwp = main_document_view->window_to_normalized_window_pos(pos);
    if (screen()->orientation() == Qt::PortraitOrientation) {
        return PORTRAIT_BACK_UI_RECT.contains(nwp);
    }
    else {
        return LANDSCAPE_BACK_UI_RECT.contains(nwp);
    }
}

bool MainWidget::is_in_forward_rect(WindowPos pos) {
    NormalizedWindowPos nwp = main_document_view->window_to_normalized_window_pos(pos);
    if (screen()->orientation() == Qt::PortraitOrientation) {
        return PORTRAIT_FORWARD_UI_RECT.contains(nwp);
    }
    else {
        return LANDSCAPE_FORWARD_UI_RECT.contains(nwp);
    }
}
bool MainWidget::is_in_visual_mark_next_rect(WindowPos pos) {

    NormalizedWindowPos nwp = main_document_view->window_to_normalized_window_pos(pos);
    if (screen()->orientation() == Qt::PortraitOrientation) {
        return PORTRAIT_VISUAL_MARK_NEXT.contains(nwp);
    }
    else {
        return LANDSCAPE_VISUAL_MARK_NEXT.contains(nwp);
    }
}

bool MainWidget::is_in_visual_mark_prev_rect(WindowPos pos) {

    NormalizedWindowPos nwp = main_document_view->window_to_normalized_window_pos(pos);
    if (screen()->orientation() == Qt::PortraitOrientation) {
        return PORTRAIT_VISUAL_MARK_PREV.contains(nwp);
    }
    else {
        return LANDSCAPE_VISUAL_MARK_PREV.contains(nwp);
    }
}

bool MainWidget::is_in_edit_portal_rect(WindowPos pos) {
    NormalizedWindowPos nwp = main_document_view->window_to_normalized_window_pos(pos);
    if (screen()->orientation() == Qt::PortraitOrientation) {
        return PORTRAIT_EDIT_PORTAL_UI_RECT.contains(nwp);
    }
    else {
        return LANDSCAPE_EDIT_PORTAL_UI_RECT.contains(nwp);
    }
}

void MainWidget::goto_page_with_label(std::wstring label) {
    int page = doc()->get_page_number_with_label(label);
    if (page > -1) {
        main_document_view->goto_page(page);
    }
}

void MainWidget::on_configs_changed(std::vector<std::string>* config_names) {
    bool should_reflow = false;
    bool should_invalidate_render = false;
    for (int i = 0; i < config_names->size(); i++){
        std::wstring confname = QString::fromStdString((*config_names)[i]).toStdWString();
        Config* conf = config_manager->get_mut_config_with_name(confname);
        if (conf->alias_for.size() > 0) {
            (*config_names)[i] = utf8_encode(conf->alias_for);
        }
        if (conf->on_change){
            conf->on_change.value()(this);
        }
    }
    // return;

    for (int i = 0; i < config_names->size(); i++) {
        QString confname = QString::fromStdString((*config_names)[i]);

        // if we are setting the dark/custom variant a color config which matches the current color mode
        // then we should also update the current value of that config
        if (confname.startsWith("DARK_") && (main_document_view->get_current_color_mode() == ColorPalette::Dark)) {
            Config* this_config = config_manager->get_mut_config_with_name(confname.toStdWString());
            Config* base_config = config_manager->get_mut_config_with_name(confname.mid(5).toStdWString());
            int n_channels = base_config->config_type == ConfigType::Color3 ? 3 : 4;
            if (base_config) {
                for (int c = 0; c < n_channels; c++) {
                    ((float*)base_config->value)[c] = ((float*)this_config->value)[c];
                }
            }
        }

        if (confname.startsWith("CUSTOM_") && (main_document_view->get_current_color_mode() == ColorPalette::Custom)) {
            Config* this_config = config_manager->get_mut_config_with_name(confname.toStdWString());
            Config* base_config = config_manager->get_mut_config_with_name(confname.mid(7).toStdWString());
            int n_channels = base_config->config_type == ConfigType::Color3 ? 3 : 4;
            if (base_config) {
                for (int c = 0; c < n_channels; c++) {
                    ((float*)base_config->value)[c] = ((float*)this_config->value)[c];
                }
            }
        }

        if (confname.startsWith("epub")) {
            should_reflow = true;
        }

        if (confname.startsWith("page_space")) {
            if (confname == "page_space_x") main_document_view->set_page_space_x(PAGE_SPACE_X);
            if (confname == "page_space_y") main_document_view->set_page_space_y(PAGE_SPACE_Y);
            main_document_view->fill_cached_virtual_rects(true);
        }

        if (confname == "scrollbar") {
            Config* this_config = config_manager->get_mut_config_with_name(confname.toStdWString());
            bool enabled = *static_cast<bool*>(this_config->get_value());
            if (enabled && !scroll_bar->isVisible()) {
                scroll_bar->show();
            }
            if (!enabled && scroll_bar->isVisible()) {
                scroll_bar->hide();
            }

        }
        if (confname == "statusbar") {
            if (!should_show_status_label()) {
                status_label->hide();
            }
            else {
                status_label->show();
            }
        }
        if (confname == "tts_voice") {
            if (tts) {
                tts->set_voice(TTS_VOICE);
            }
        }

    }
    if (should_reflow) {
        bool flag = false;
        pdf_renderer->delete_old_pages(true, true);
        int new_page = doc()->reflow(get_current_page_number());
        main_document_view->goto_page(new_page);
    }
    if (should_invalidate_render) {
        pdf_renderer->clear_cache();
    }
}

void MainWidget::on_config_changed(std::string config_name, bool should_save) {
    std::vector<std::string> config_names;
    config_names.push_back(config_name);

    for (auto window : windows){
        window->on_configs_changed(&config_names);
    }

    if (should_save) {
        auto conf = config_manager->get_mut_config_with_name(utf8_decode(config_name));
        if (conf) {
            conf->is_auto = true;
            save_auto_config();
        }
    }
}

void MainWidget::handle_undo_marked_data() {
    if (main_document_view->marked_data_rects.size() > 0) {
        main_document_view->marked_data_rects.pop_back();
    }
}

void MainWidget::handle_add_marked_data() {
    std::deque<AbsoluteRect> local_selected_rects;
    std::wstring local_selected_text;

    main_document_view->get_text_selection(dv()->selection_begin,
        dv()->selection_end,
        main_document_view->is_word_selecting,
        local_selected_rects,
        local_selected_text);
    if (local_selected_rects.size() > 0) {

        DocumentRect begin_docrect = local_selected_rects[0].to_document(doc());
        DocumentRect end_docrect = local_selected_rects[local_selected_rects.size() - 1].to_document(doc());

        MarkedDataRect begin_rect;
        begin_rect.rect = begin_docrect;
        begin_rect.type = 0;
        main_document_view->marked_data_rects.push_back(begin_rect);

        MarkedDataRect end_rect;
        end_rect.rect = end_docrect;
        end_rect.type = 1;
        main_document_view->marked_data_rects.push_back(end_rect);

        invalidate_render();
    }
}

void MainWidget::handle_remove_marked_data() {
    main_document_view->marked_data_rects.clear();
}


void MainWidget::handle_export_marked_data() {
}

void MainWidget::handle_goto_random_page() {
    int num_pages = doc()->num_pages();
    int random_page = rand() % num_pages;
    main_document_view->goto_page(random_page);
    invalidate_render();
}

void MainWidget::show_download_paper_menu(
    const std::vector<std::wstring>& paper_names,
    const std::vector<std::wstring>& download_urls,
    std::wstring paper_name,
    PaperDownloadFinishedAction action) {


    // force it to be a double column layout. the second column will asynchronously be filled with
    // file sizes
    std::vector<std::wstring> right_names(paper_names.size());
    std::vector<std::pair<std::wstring, std::wstring>> values;
    for (int i = 0; i < paper_names.size(); i++) {
        values.push_back(std::make_pair(paper_names[i], download_urls[i]));
    }

    show_current_widget();
}


bool MainWidget::is_network_manager_running(bool* is_downloading, std::wstring* message) {
    if (sioyek_network_manager->network_manager_ == nullptr) {
        return false;
    }

    auto children = sioyek_network_manager->network_manager_->findChildren<QNetworkReply*>();
    bool running = false;

    for (int i = 0; i < children.size(); i++) {
        if (children.at(i)->isRunning()) {
            running = true;
            if (is_downloading) {
                bool downloading = !children.at(i)->property("sioyek_downloading").isNull();

                if (!children.at(i)->property("sioyek_network_message").isNull()) {
                    *message = children.at(i)->property("sioyek_network_message").toString().toStdWString();
                }

                *is_downloading = downloading;
                return running;
            }
        }
    }
    return running;
}

QString MainWidget::get_network_status_string() {
    auto children = findChildren<QNetworkReply*>();
    for (int i = 0; i < children.size(); i++) {
        if (children.at(i)->isRunning()) {
            QVariant status_message = children.at(i)->property("sioyek_network_status_string");
            if (!status_message.isNull()) {
                return status_message.toString();
            }
        }
    }
    return "";
}

void MainWidget::exit_freehand_drawing_mode() {
    freehand_drawing_mode = DrawingMode::None;
    handle_drawing_ui_visibilty();
}

void MainWidget::toggle_freehand_drawing_mode() {
    set_hand_drawing_mode(freehand_drawing_mode != DrawingMode::Drawing);
}

void MainWidget::set_pen_drawing_mode(bool enabled) {
    if (enabled) {
        freehand_drawing_mode = DrawingMode::PenDrawing;
    }
    else {
        freehand_drawing_mode = DrawingMode::None;
    }
    handle_drawing_ui_visibilty();
}

void MainWidget::set_hand_drawing_mode(bool enabled) {
    if (enabled) {
        freehand_drawing_mode = DrawingMode::Drawing;
    }
    else {
        freehand_drawing_mode = DrawingMode::None;
    }
    handle_drawing_ui_visibilty();
}

void MainWidget::toggle_pen_drawing_mode() {
    set_pen_drawing_mode(freehand_drawing_mode != DrawingMode::PenDrawing);
}

void MainWidget::handle_undo_drawing() {
    doc()->undo_freehand_drawing();
    invalidate_render();
}

void MainWidget::set_freehand_thickness(float val) {
    main_document_view->freehand_thickness = val;
}

void MainWidget::handle_pen_drawing_event(QTabletEvent* te) {

    if (te->type() == QEvent::TabletPress) {
        main_document_view->start_drawing();
    }

    if (te->type() == QEvent::TabletRelease) {
        main_document_view->finish_drawing(te->pos());
        invalidate_render();
    }

    if (te->type() == QEvent::TabletMove) {
        if (main_document_view->is_drawing) {
            main_document_view->handle_drawing_move(te->pos(), te->pressure());
            validate_render();
        }

    }
}

void MainWidget::delete_freehand_drawings(AbsoluteRect rect) {
    if (main_document_view->scratchpad) {
        scratchpad->delete_intersecting_objects(rect);
        set_rect_select_mode(false);
        clear_selected_rect();
        invalidate_render();
    }
    else {
        DocumentRect page_rect = rect.to_document(doc());
        doc()->delete_page_intersecting_drawings(page_rect.page, rect, main_document_view->visible_drawing_mask);
        set_rect_select_mode(false);
        clear_selected_rect();
        invalidate_render();
    }
}

void MainWidget::select_freehand_drawings(AbsoluteRect rect) {
    main_document_view->select_freehand_drawings(rect);

    set_rect_select_mode(false);
    clear_selected_rect();
    invalidate_render();
}

void MainWidget::hande_turn_on_all_drawings() {
    for (int i = 0; i < 26; i++) {
        main_document_view->visible_drawing_mask[i] = true;
    }
}

void MainWidget::hande_turn_off_all_drawings() {
    for (int i = 0; i < 26; i++) {
        main_document_view->visible_drawing_mask[i] = false;
    }
}

void MainWidget::handle_toggle_drawing_mask(char symbol) {

    if (symbol >= 'a' && symbol <= 'z') {
        main_document_view->visible_drawing_mask[symbol - 'a'] = !main_document_view->visible_drawing_mask[symbol - 'a'];
    }
}

std::string MainWidget::get_current_mode_string() {
    std::string res;
    res += (main_document_view->is_ruler_mode()) ? "r" : "R";
    res += main_document_view->is_line_select_mode() ? "l" : "L";
    res += (synctex_mode) ? "x" : "X";
    //res += (is_select_highlight_mode) ? "h" : "H";
    res += (freehand_drawing_mode == DrawingMode::Drawing) ? "q" : "Q";
    res += (freehand_drawing_mode == DrawingMode::PenDrawing) ? "e" : "E";
    res += (mouse_drag_mode) ? "d" : "D";
    res += (main_document_view->is_presentation_mode()) ? "p" : "P";
    res += (main_document_view->get_overview_page()) ? "o" : "O";
    res += main_document_view->scratchpad ? "s" : "S";
    res += (main_document_view->get_is_searching(nullptr)) ? "f" : "F";
    res += (is_menu_focused()) ? "m" : "M";
    res += main_document_view->selected_object_index.has_value() ? "h" : "H";

    if (main_document_view) {
        res += (main_document_view->selected_character_rects.size() > 0) ? "t" : "T";
    }
    else {
        res += "T";
    }
    return res;

}

void MainWidget::handle_drawing_ui_visibilty() {
    if (!TOUCH_MODE && (draw_controls_ == nullptr)) {
        return;
    }

    if (freehand_drawing_mode == DrawingMode::None) {
        get_draw_controls()->hide();
    }
    else {
        get_draw_controls()->show();
        get_draw_controls()->controls_ui->set_pen_size(main_document_view->freehand_thickness);
    }
}


void MainWidget::handle_toggle_text_mark() {
    main_document_view->should_show_text_selection_marker = true;
    main_document_view->toggle_text_mark();
}


void MainWidget::handle_goto_loaded_document() {
    // opens a list of currently loaded documents. This is basically sioyek's "tab" feature
    // the user can "unload" a document by pressing the delete key while it is highlighted in the list

    std::vector<std::wstring> loaded_document_paths_ = document_manager->get_tabs();
    //std::vector<std::wstring> loaded_document_paths = get_path_unique_prefix(loaded_document_paths_);
    //std::vector<std::wstring> loaded_document_paths = get_path_unique_prefix(loaded_document_paths_);
    std::vector<std::wstring> detected_paper_names;

    std::vector<OpenedBookInfo> opened_books = get_all_opened_books(false, true);
    std::map<QString, QString> path_to_title_map;
    for (auto opened_book : opened_books) {
        path_to_title_map[opened_book.file_name] = opened_book.document_title;
    }

    for (auto path : loaded_document_paths_) {
        Document* loaded_document = document_manager->get_document(path);
        if (loaded_document && loaded_document->doc) {
            detected_paper_names.push_back(loaded_document->detect_paper_name());
        }
        else {
            auto it = path_to_title_map.find(QString::fromStdWString(path));
            if (it != path_to_title_map.end()) {
                detected_paper_names.push_back(it->second.toStdWString());
            }
            else {
                detected_paper_names.push_back(L"[" + path + L"]");
            }
        }
    }

    std::wstring current_document_path = L"";

    if (doc()) {
        current_document_path = doc()->get_path();
    }


    auto loc = std::find(loaded_document_paths_.begin(), loaded_document_paths_.end(), current_document_path);
    int index = -1;
    if (loc != loaded_document_paths_.end()) {
        index = loc - loaded_document_paths_.begin();
    }

    std::vector<QString> loaded_document_paths_qstrings;
    std::vector<QString> detected_paper_names_qstrings;
    std::vector<QString> document_path_qstring;
    for (int i = 0; i < loaded_document_paths_.size(); i++) {
        loaded_document_paths_qstrings.push_back(QString::fromStdWString(loaded_document_paths_[i]));
        detected_paper_names_qstrings.push_back(QString::fromStdWString(detected_paper_names[i]));
        document_path_qstring.push_back(QString::fromStdWString(loaded_document_paths_[i]));
    }

    auto widget = ItemWithDescriptionSelectorWidget::from_items(
        std::move(detected_paper_names_qstrings),
        std::move(loaded_document_paths_qstrings),
        std::move(document_path_qstring),
        this);

    widget->set_select_fn([this, widget](int index) {
        QString path = widget->item_model->metadatas[index];
        if (pending_command_instance) {
            pending_command_instance->set_generic_requirement(path);
            advance_command(std::move(pending_command_instance));
            pop_current_widget();
        }
        });
    widget->set_delete_fn([this, widget](int index) {
        std::wstring path = widget->item_model->metadatas[index].toStdWString();
        std::optional<Document*> doc_to_delete = document_manager->get_cached_document(path);
        if (!doc_to_delete) {
            document_manager->remove_tab(path);
        }
        for (auto window : windows) {
            if (window->doc() && window->doc()->get_path() == path) {
                if (window != this) {
                    window->close();
                }
            }
        }
        if (doc_to_delete && (doc_to_delete.value() != doc())) {
            document_manager->remove_tab(path);
            free_document(doc_to_delete.value());
        }
        else if (doc_to_delete) {
            // removing the current document, close the document
            main_document_view->set_null_document();
            document_manager->remove_tab(path);
            free_document(doc_to_delete.value());
        }
        });

    set_current_widget(widget);
    show_current_widget();

    //set_filtered_select_menu<std::wstring>(this, true,
    //    MULTILINE_MENUS,
    //    { loaded_document_paths, detected_paper_names },
    //    loaded_document_paths_,
    //    index,
    //    [&](std::wstring* path) {
    //        if (pending_command_instance) {
    //            pending_command_instance->set_generic_requirement(QString::fromStdWString(*path));
    //            advance_command(std::move(pending_command_instance));
    //        }
    //        //open_document(*path);
    //    },
    //    [&](std::wstring* path) {
    //        std::optional<Document*> doc_to_delete = document_manager->get_cached_document(*path);
    //        if (!doc_to_delete) {
    //            document_manager->remove_tab(*path);
    //        }
    //        for (auto window : windows) {
    //            if (window->doc() && window->doc()->get_path() == *path) {
    //                if (window != this) {
    //                    window->close();
    //                }
    //            }
    //        }
    //        if (doc_to_delete && (doc_to_delete.value() != doc())) {
    //            document_manager->remove_tab(*path);
    //            free_document(doc_to_delete.value());
    //        }
    //    }
    //    );
    //make_current_menu_columns_equal();

    show_current_widget();
}

bool MainWidget::execute_macro_if_enabled(std::wstring macro_command_string, QLocalSocket* result_socket) {

    std::unique_ptr<Command> command = command_manager->create_macro_command(this, "", macro_command_string);
    command->set_result_socket(result_socket);

    if (is_macro_command_enabled(command.get())) {
        handle_command_types(std::move(command), 0);
        invalidate_render();

        return true;
    }

    return false;
}

bool MainWidget::ensure_internet_permission() {

#ifdef SIOYEK_ANDROID
    //    qDebug() << "entered";
    auto internet_permission_status = QtAndroidPrivate::checkPermission("android.permission.INTERNET").result();
    //    qDebug() << "checked";
    //    qDebug() << internet_permission_status;
    if (internet_permission_status == QtAndroidPrivate::Denied) {
        //        qDebug() << "was denied";
        internet_permission_status = QtAndroidPrivate::requestPermission("android.permission.INTERNET").result();

        if (internet_permission_status == QtAndroidPrivate::Denied) {
            qDebug() << "Could not get internet permission\n";
        }
    }

#endif
    return true;
}

void MainWidget::change_selected_bookmark_text(const std::wstring& new_text) {
    std::string selected_bookmark_uuid = main_document_view->get_selected_bookmark_uuid();
    if (selected_bookmark_uuid.size() > 0) {
        BookMark* selected_bookmark = doc()->get_bookmark_with_uuid(selected_bookmark_uuid);
        if (selected_bookmark) {
            if (new_text.size() > 0) {
                float new_font_size = selected_bookmark->font_size;
                doc()->update_bookmark_text(selected_bookmark_uuid, new_text, new_font_size);
                on_bookmark_edited(selected_bookmark->uuid);
            }
            else {
                delete_current_document_bookmark(selected_bookmark_uuid);
            }
        }
    }
}

void MainWidget::update_highlight_annot_with_uuid(const std::string& uuid, const std::wstring& new_annot) {
    doc()->update_highlight_add_text_annotation(uuid, new_annot);
    on_highlight_annotation_edited(uuid);
}

std::optional<BookMark> MainWidget::delete_current_document_bookmark(const std::string& uuid) {
    std::optional<BookMark> deleted_bookmark = doc()->delete_bookmark_with_uuid(uuid);
    if (deleted_bookmark) {
        on_bookmark_deleted(deleted_bookmark.value(), doc()->get_checksum());
    }
    return deleted_bookmark;
}

void MainWidget::delete_global_bookmark(const std::string& uuid) {
    std::vector<std::pair<std::string, BookMark>> deleted_bookmark;
    db_manager->select_bookmark_with_uuid(uuid, deleted_bookmark);
    if (deleted_bookmark.size() > 0) {
        if (deleted_bookmark[0].first == doc()->get_checksum()) {
            doc()->delete_bookmark_with_uuid(uuid);
        }
        else {
            db_manager->delete_bookmark(uuid);
        }
        on_bookmark_deleted(deleted_bookmark[0].second, deleted_bookmark[0].first);
    }
}

void MainWidget::change_selected_highlight_text_annot(const std::wstring& new_text) {
    std::string selected_highlight_uuid = main_document_view->get_selected_highlight_uuid();
    if (selected_highlight_uuid.size() > 0) {
        update_highlight_annot_with_uuid(selected_highlight_uuid, new_text);
    }
}

void MainWidget::set_command_textbox_text(const std::wstring& txt) {
    if (TOUCH_MODE) {
        if (current_widget_stack.size() > 0) {
            TouchTextEdit* edit_widget = dynamic_cast<TouchTextEdit*>(current_widget_stack.back());
            if (edit_widget) {
                edit_widget->set_text(txt);
            }

        }

    }
    else {
        text_command_line_edit->setText(QString::fromStdWString(txt));
    }
}

void MainWidget::toggle_pdf_annotations() {

    if (doc()->get_should_render_pdf_annotations()) {
        doc()->set_should_render_pdf_annotations(false);
    }
    else {
        doc()->set_should_render_pdf_annotations(true);
    }

    pdf_renderer->delete_old_pages(true, true);
}

void MainWidget::handle_command_text_change(const QString& new_text) {
    if (pending_command_instance) {
        pending_command_instance->on_text_change(new_text);
        validate_render();
    }
}

void MainWidget::update_selected_bookmark_font_size() {
    std::string selected_bookmark_uuid = main_document_view->get_selected_bookmark_uuid();
    if (selected_bookmark_uuid.size() > 0) {
        BookMark* selected_bookmark = doc()->get_bookmark_with_uuid(selected_bookmark_uuid);

        if (selected_bookmark->font_size != -1) {
            selected_bookmark->font_size = FREETEXT_BOOKMARK_FONT_SIZE;
            pdf_renderer->get_bookmark_renderer()->release_cache();
        }
    }
}

TextToSpeechHandler* MainWidget::get_tts() {
    if (tts) return tts;

#ifdef SIOYEK_ANDROID
    tts = new AndroidTextToSpeechHandler();
#else
    tts = new QtTextToSpeechHandler();
#endif


    if (TTS_VOICE.size() > 0) {
        tts->set_voice(TTS_VOICE);
    }
    //void sayingWord(const QString &word, qsizetype id, qsizetype start, qsizetype length);

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)

    tts_has_pause_resume_capability = tts->is_pausable();
    if (tts->is_word_by_word()) {
        word_by_word_reading = true;
    }
    
    //void aboutToSynthesize(qsizetype id);
    tts->set_word_callback([&](int start, int length) {
        if (is_reading) {
            if (start >= tts_corresponding_line_rects.size()) return;

            PagelessDocumentRect line_being_read_rect = tts_corresponding_line_rects[start];
            PagelessDocumentRect char_being_read_rect = tts_corresponding_char_rects[start];

            int ruler_page = main_document_view->get_vertical_line_page();

            DocumentRect char_being_read_document_rect = DocumentRect(char_being_read_rect, ruler_page);
            NormalizedWindowRect char_being_read_window_rect = char_being_read_document_rect.to_window_normalized(main_document_view);

            int end = start + length;
            if (last_focused_rect.has_value() && (last_focused_rect.value() == line_being_read_rect)) {

                if (char_being_read_window_rect.x0 > 1) {
                    main_document_view->move_visual_mark_next();
                    invalidate_render();
                }
            }

            if (!tts_is_about_to_finish && ((!last_focused_rect.has_value()) || !(last_focused_rect.value() == line_being_read_rect))) {
                last_focused_rect = line_being_read_rect;
                DocumentRect line_being_read_document_rect = DocumentRect(line_being_read_rect, ruler_page);
                WindowRect line_being_read_window_rect = line_being_read_document_rect.to_window(main_document_view);
                NormalizedWindowRect line_being_read_normalized_window_rect = line_being_read_document_rect.to_window_normalized(main_document_view);
                main_document_view->focus_rect(line_being_read_document_rect);

                if (line_being_read_normalized_window_rect.x0 < -1) { // if the next line is out of view
                    move_horizontal(-line_being_read_window_rect.x0);
                }

                invalidate_render();
            }


            if ((tts_text.size() - end) <= 5) {
                tts_is_about_to_finish = true;
            }

        }
        });


    tts->set_state_change_callback([&](QString state) {

        if ((!word_by_word_reading) && is_reading && (state == "Ready") && (prev_tts_state == "Speaking")){
            // when word_by_word_reading is not available, we can't rely on tts_is_about_to_finish
            move_visual_mark(1);
            invalidate_render();

        }
        else{
            if ((state == "Ready") || (state == "Error")) {
                if (is_reading && tts_is_about_to_finish) {
                    tts_is_about_to_finish = false;
                    move_visual_mark(1);
                    invalidate_render();
                }
            }
        }
        prev_tts_state = state;
        });

    tts->set_external_state_change_callback([&](QString state){
        ensure_player_state_(state);
    });

    tts->set_on_app_pause_callback([&](){

        bool is_audio_ui_visible = false;
        if (current_widget_stack.size() > 0){
            AudioUI* audio_ui = dynamic_cast<AudioUI*>(current_widget_stack.back());
            if (audio_ui){
                is_audio_ui_visible = true;
            }
        }

        if (is_reading || is_audio_ui_visible) {
            return get_rest_of_document_pages_text();
        }
        else{
            return QString("");
        }
    });

    tts->set_on_app_resume_callback([&](bool is_playing, bool is_on_rest, int offset){

        handle_app_tts_resume(is_playing, is_on_rest, offset);
    });

#else
    QObject::connect(tts, &QTextToSpeech::stateChanged, [&](QTextToSpeech::State state) {
        if ((state == QTextToSpeech::Ready) || (state == QTextToSpeech::Error)) {
            if (is_reading) {
                move_visual_mark(1);
                //read_current_line();
                invalidate_render();
            }
        }
        });
#endif

#ifdef SIOYEK_ANDROID
        // wait for the tts engine to be initialized
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
#endif

    return tts;
}

void MainWidget::handle_app_tts_resume(bool is_playing, bool is_on_rest, int offset){
    // set_status_message(L"got on resume callback");

    // return;
    if (is_reading && (is_playing == false)){
        is_reading = false;
    }

    if (is_playing){
        ensure_player_state("Ended");

        handle_stop_reading();

        if (doc() && doc()->is_super_fast_index_ready()){
            if (offset > 0){
                focus_on_character_offset_into_document(offset);
            }
        }
        else{
            dv()->on_super_fast_compute_focus_offset = offset;
        }
    }
    else{
        ensure_player_state("Ended");
    }
}

void MainWidget::update_bookmark_with_uuid(const std::string& uuid) {
    if (uuid.size() > 0) {
        //BookMark& bm = doc()->get_bookmarks()[index];
        BookMark* bm = doc()->get_bookmark_with_uuid(uuid);
        if (bm) {
            doc()->update_bookmark_position(uuid, { bm->begin_x, bm->begin_y }, { bm->end_x, bm->end_y });
            on_bookmark_edited(bm->uuid);
        }
    }
}

void MainWidget::update_portal_with_uuid(const std::string& uuid) {
    if (uuid.size() > 0) {
        //Portal& portal = doc()->get_portals()[index];
        Portal* portal = doc()->get_portal_with_uuid(uuid);

        if (portal) {
            if (portal->is_pinned()) {
                doc()->update_portal_src_position(uuid,
                    AbsoluteDocumentPos{ portal->src_offset_x.value(), portal->src_offset_y },
                    AbsoluteDocumentPos{ portal->src_offset_end_x.value(), portal->src_offset_end_y.value() }
                );
            }
            else {
                if (portal->src_offset_end_x.has_value()) {
                    doc()->update_portal_src_position(uuid,
                        AbsoluteDocumentPos{ portal->src_offset_x.value(), portal->src_offset_y },
                        {}
                    );
                }
            }
            //doc()->update_portal(portal);
            on_portal_edited(uuid);
        }
    }
}

void MainWidget::handle_bookmark_move_finish() {
    std::string uuid = main_document_view->visible_object_move_data->index.uuid;
    update_bookmark_with_uuid(uuid);
}

void MainWidget::handle_portal_move_finish() {
    main_document_view->handle_portal_move_finish();
}

void MainWidget::handle_visible_object_move() {
    if (main_document_view->visible_object_move_data.has_value()) {
        auto type = main_document_view->visible_object_move_data->index.object_type;
        if (type == VisibleObjectType::Bookmark) {
            handle_bookmark_move();
        }
        else if (type == VisibleObjectType::Portal || type == VisibleObjectType::PendingPortal) {
            handle_portal_move();
        }
    }
}

void MainWidget::handle_portal_move(){
    main_document_view->handle_portal_move(get_cursor_abspos());
}

void MainWidget::handle_bookmark_move() {
    AbsoluteDocumentPos current_mouse_abspos = get_cursor_abspos();
    main_document_view->handle_bookmark_move(current_mouse_abspos);
}

bool MainWidget::is_middle_click_being_used() {
    return main_document_view->visible_object_move_data.has_value() || overview_touch_move_data.has_value() || is_dragging;
}


bool MainWidget::should_drag() {
    return is_dragging && main_document_view && (!main_document_view->visible_object_move_data.has_value());
}

void MainWidget::show_command_palette() {

    std::vector<std::wstring> command_names;
    std::vector<std::wstring> command_descs;

    for (auto [command_name, command_desc] : command_manager->command_human_readable_names) {
        command_names.push_back(utf8_decode(command_name));
        command_descs.push_back(utf8_decode(command_desc));
    }
    std::vector<std::vector<std::wstring>> columns = { command_descs, command_names };
    //widget->set_filtered_selelect_menu()

    set_filtered_select_menu<std::wstring>(this, true,
        false,
        columns,
        command_names,
        -1,
        [this](std::wstring* s) {
            auto command = this->command_manager->get_command_with_name(this, utf8_encode(*s));
            this->handle_command_types(std::move(command), 0);
        },
        [](std::wstring* s) {
        });
    show_current_widget();
}

TouchTextSelectionButtons* MainWidget::get_text_selection_buttons() {
    if (text_selection_buttons_ == nullptr) {
        text_selection_buttons_ = new TouchTextSelectionButtons(this);
        text_selection_buttons_->hide();
    }

    return text_selection_buttons_;
}

DrawControlsUI* MainWidget::get_draw_controls() {

    if (draw_controls_ == nullptr) {
        draw_controls_ = new DrawControlsUI(this);
        draw_controls_->hide();
    }

    draw_controls_->controls_ui->set_scratchpad_mode(is_scratchpad_mode());
    return draw_controls_;
}

SearchButtons* MainWidget::get_search_buttons() {

    if (search_buttons_ == nullptr) {
        search_buttons_ = new SearchButtons(this);
        search_buttons_->hide();
    }

    return search_buttons_;
}

HighlightButtons* MainWidget::get_highlight_buttons() {

    if (highlight_buttons_ == nullptr) {
        highlight_buttons_ = new HighlightButtons(this);
        highlight_buttons_->hide();
    }

    return highlight_buttons_;
}

bool MainWidget::goto_ith_next_overview(int i) {
    std::optional<OverviewState> state = main_document_view->get_ith_next_overview(i);
    if (state.has_value()){
        set_overview_page(state.value(), true);
        invalidate_render();
        on_overview_source_updated();
        return true;
    }
    return false;
}

void MainWidget::on_overview_source_updated() {
    invalidate_render();
    if (current_widget_stack.size() > 0 && dynamic_cast<TouchGenericButtons*>(current_widget_stack.back())) {
        if (dv()->index_into_candidates >= 0 && dv()->index_into_candidates < dv()->smart_view_candidates.size()) {
            auto& current_candidate = dv()->smart_view_candidates[dv()->index_into_candidates];
            show_touch_buttons_for_overview_type(reference_type_string(current_candidate.reference_type));
        }
    }
    dv()->fit_overview_width();

    //if (index_into_candidates >= 0 && index_into_candidates < smart_view_candidates.size()) {
    //    main_document_view->set_overview_highlights(smart_view_candidates[index_into_candidates].highlight_rects);
    //}
}

AbsoluteDocumentPos MainWidget::get_cursor_abspos() {
    QPoint current_mouse_window_point = mapFromGlobal(QCursor::pos());
    WindowPos current_mouse_window_pos = { current_mouse_window_point.x(), current_mouse_window_point.y() };
    return main_document_view->window_to_absolute_document_pos(current_mouse_window_pos);
}

void MainWidget::cleanup_expired_pending_portals() {
    std::vector<int> indices_to_delete;

    if ((main_document_view->pending_download_portals.size() > 0) && (current_widget_stack.size() == 0)) {
        if (sioyek_network_manager->network_manager_ == nullptr) {
            return;
        }

        auto children_ = findChildren<QNetworkReply*>() + sioyek_network_manager->network_manager_->findChildren<QNetworkReply*>();

        for (int i = 0; i < main_document_view->pending_download_portals.size(); i++) {
            auto paper_name = main_document_view->pending_download_portals[i].paper_name;
            bool still_pending = false;
            //network_manager.
            for (int i = 0; i < children_.size(); i++) {
                auto paper_name_prop = children_[i]->property("sioyek_paper_name");
                if ((!paper_name_prop.isNull()) && paper_name_prop.toString().toStdWString() == paper_name) {
                    still_pending = true;
                }
            }
            if (!still_pending) {
                if (main_document_view->pending_download_portals[i].marked) {
                    indices_to_delete.push_back(i);
                }
                else {
                    main_document_view->pending_download_portals[i].marked = true;
                }
            }
        }
    }
    if (indices_to_delete.size() > 0) {
        //update_pending_portal_indices_after_removed_indices(indices_to_delete);
        for (int i = indices_to_delete.size() - 1; i >= 0; i--) {
            main_document_view->pending_download_portals.erase(main_document_view->pending_download_portals.begin() + indices_to_delete[i]);
        }
        // update_opengl_pending_download_portals();
    }

}

void MainWidget::close_overview() {
    set_overview_page({}, false);
}

void MainWidget::handle_overview_to_ruler_portal() {
    std::optional<OverviewState> state = main_document_view->overview_to_ruler_portal(&is_render_invalidated);
    if (state.has_value()){
        set_overview_page(state.value(), true);
        invalidate_render();
    }

}

void MainWidget::handle_goto_ruler_portal(std::string tag) {
    std::vector<Portal> portals = main_document_view->get_ruler_portals();
    int index = 0;
    if (tag.size() > 0) {
        index = get_index_from_tag(tag);
    }

    if (portals.size() > 0 && (index < portals.size())) {
        open_document(portals[index].dst);
    }
}


void MainWidget::show_touch_buttons(std::vector<std::wstring> buttons, std::vector<std::wstring> tips, std::function<void(int, std::wstring)> on_select, bool top) {

    if (current_widget_stack.size() > 0 && dynamic_cast<TouchGenericButtons*>(current_widget_stack.back())) {
        pop_current_widget();
    }

    TouchGenericButtons* generic_buttons = new TouchGenericButtons(buttons, tips, top, this);
    QObject::connect(generic_buttons, &TouchGenericButtons::buttonPressed, [this, on_select](int index, std::wstring name) {
        on_select(index, name);
        });
    push_current_widget(generic_buttons);
    show_current_widget();
}


void MainWidget::smart_jump_to_selected_text() {
    if (dv()->selected_text.size() != 0) {
        int page = -1;
        float offset;
        AbsoluteRect source_rect;
        std::wstring source_text;
        if (dv()->find_location_of_selected_text(&page, &offset, &source_rect, &source_text) != ReferenceType::None) {
            long_jump_to_destination(page, offset);
        }
    }
}

void MainWidget::download_selected_text() {
    if (dv()->selected_text.size() != 0) {
        int page = -1;
        float offset;
        AbsoluteRect source_rect;
        std::wstring source_text;
        if (dv()->find_location_of_selected_text(&page, &offset, &source_rect, &source_text) != ReferenceType::None) {
            auto bib_item_ = doc()->get_page_bib_with_reference(page, source_text);
            if (bib_item_) {
                auto [bib_item_text, bib_item_rect] = bib_item_.value();
                QString paper_name = get_paper_name_from_reference_text(bib_item_text);
                AbsoluteDocumentPos source_pos = source_rect.center();
                //source_pos.x = (source_rect.x1 + source_rect.x1) / 2;
                //source_pos.y = (source_rect.y0 + source_rect.y1) /2 ;
                show_text_prompt(paper_name.toStdWString(), [this, source_pos](std::wstring confirmed_paper_name) {
                    download_and_portal(confirmed_paper_name, source_pos);
                    });
            }
        }
    }
}

void MainWidget::download_and_portal(std::wstring unclean_paper_name, AbsoluteDocumentPos source_pos) {

    std::wstring cleaned_paper_name = clean_bib_item(QString::fromStdWString(unclean_paper_name)).toStdWString();
    std::string pending_portal_handle = main_document_view->create_pending_download_portal(source_pos, cleaned_paper_name);
    download_paper_with_name(cleaned_paper_name, PaperDownloadFinishedAction::Portal, pending_portal_handle);
}

void MainWidget::show_text_prompt(std::wstring initial_value, std::function<void(std::wstring)> on_select) {
    auto new_widget = new TouchTextEdit("Enter text", QString::fromStdWString(initial_value), false, this);

    QObject::connect(new_widget, &TouchTextEdit::confirmed, [this, on_select](QString text) {
        on_select(text.toStdWString());
        pop_current_widget();
        });

    QObject::connect(new_widget, &TouchTextEdit::cancelled, [this]() {
        pop_current_widget();
        });
    set_current_widget(new_widget);
    show_current_widget();
}

void MainWidget::on_set_enum_config_value(std::string config_name, std::wstring config_value) {
    config_manager->deserialize_config(config_name, config_value);
    pop_current_widget();
    invalidate_render();
}

void MainWidget::show_touch_buttons_for_overview_type(std::string type) {
    std::vector<std::wstring> button_icons;
    std::vector<std::wstring> button_names;

    button_icons = { L"qrc:/icons/go-to-file.svg" };
    button_names = { L"Go" };

    if (type == "reference" || type == "reflink") {
        button_icons.push_back(L"qrc:/icons/paper-download.svg");
        button_names.push_back(L"Download");
    }

    if (dv()->smart_view_candidates.size() > 1) {
        button_icons.insert(button_icons.begin(), L"qrc:/icons/next.svg");
        button_names.insert(button_names.begin(), L"Prev");
        button_icons.insert(button_icons.end(), L"qrc:/icons/previous.svg");
        button_names.insert(button_names.end(), L"Next");
    }
    show_touch_buttons(
        button_icons,
        button_names,
        [this](int index, std::wstring name) {
            QString name_qstring = QString::fromStdWString(name);

            if (name_qstring.endsWith("next.svg")) {
                goto_ith_next_overview(-1);
                invalidate_render();
            }
            if (name_qstring.endsWith("previous.svg")) {
                goto_ith_next_overview(1);
                invalidate_render();
            }
            if (name_qstring.endsWith("go-to-file.svg")) {
                goto_overview();
                invalidate_render();
            }
            if (name_qstring.endsWith("paper-download.svg")) {
                //execute_macro_if_enabled(L"download_overview_paper");
                auto command = command_manager->get_command_with_name(this, "download_overview_paper");
                handle_command_types(std::move(command), 1);
            }
        });
}

void MainWidget::update_touch_overview_buttons(const std::optional<OverviewState>& overview) {
    if (TOUCH_MODE) {
        if (overview) {
            //if (!main_document_view->get_overview_page().has_value()) {
            //    // show the overview buttons when a new overview is displayed
                show_touch_buttons_for_overview_type(overview->overview_type.value_or(""));
            //}
        }
        else {
            if (current_widget_stack.size() > 0) {
                if (dynamic_cast<TouchGenericButtons*>(current_widget_stack.back())) {
                    pop_current_widget();
                }
            }
        }
    }
}

void MainWidget::set_overview_page(std::optional<OverviewState> overview, bool should_update_buttons) {

    // if (!overview){
    //     main_document_view->set_overview_highlights({});
    // }
    // else {
    //     main_document_view->set_overview_highlights(overview->highlight_rects);
    // }

    if (should_update_buttons) {
        update_touch_overview_buttons(overview);
    }
    main_document_view->set_overview_page(overview);

}

QJSEngine* MainWidget::take_js_engine(bool async) {
    //std::lock_guard guard(available_engine_mutex);
    if (!async) {
        if (sync_js_engine != nullptr) {
            return sync_js_engine;
        }

        auto js_engine = new QJSEngine();
        js_engine->installExtensions(QJSEngine::ConsoleExtension);

        QJSValue sioyek_object = js_engine->newQObject(this);
        js_engine->setObjectOwnership(this, QJSEngine::CppOwnership);

        js_engine->globalObject().setProperty("sioyek_api", sioyek_object);
        export_javascript_api(*js_engine, false);
        sync_js_engine = js_engine;
        return sync_js_engine;

    }
    available_engine_mutex.lock();

    while (num_js_engines > 4 && (available_async_engines.size() == 0)) {
        available_engine_mutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        available_engine_mutex.lock();
    }

    if (available_async_engines.size() > 0) {
        auto res = available_async_engines.back();
        available_async_engines.pop_back();
        available_engine_mutex.unlock();
        return res;
    }

    auto js_engine = new QJSEngine();
    js_engine->installExtensions(QJSEngine::ConsoleExtension);
    num_js_engines++;

    available_engine_mutex.unlock();

    QJSValue sioyek_object = js_engine->newQObject(this);
    js_engine->setObjectOwnership(this, QJSEngine::CppOwnership);

    js_engine->globalObject().setProperty("sioyek_api", sioyek_object);
    export_javascript_api(*js_engine, true);
    return js_engine;
}

void MainWidget::release_async_js_engine(QJSEngine* engine) {
    std::lock_guard guard(available_engine_mutex);
    available_async_engines.push_back(engine);
}

QJSValue MainWidget::export_javascript_api(QJSEngine& engine, bool is_async) {

    QJSValue res = engine.newObject();
    engine.globalObject().setProperty("sioyek", res);

    QStringList command_names = command_manager->get_all_command_names();
    QString all_command_eval_string = "[";
    for (int i = 0; i < command_names.size(); i++) {
        if (command_names[i] == "import") continue;
        all_command_eval_string += "\"" + command_names[i] + "\"";

        if (i < command_names.size() - 1) {
            all_command_eval_string += ",";
        }

    }
    all_command_eval_string += "]";
    engine.evaluate("__all_command_names=" + all_command_eval_string);

    //engine.globalObject().setProperty("__all_command_names", command_names);


    if (is_async) {
        engine.evaluate("\
            for (let i = 0; i < __all_command_names.length; i++){\
                let cname = __all_command_names[i];\
                sioyek[cname] = (...args)=>{\
                    let arg_strings = args.map((arg) => {return '' + arg;});\
                    return sioyek_api.run_macro_on_main_thread(cname, arg_strings);\
                };\
                sioyek['$' + cname] = (...args)=>{\
                    let arg_strings = args.map((arg) => {return '' + arg;});\
                    return sioyek_api.run_macro_on_main_thread(cname, arg_strings, false);\
                };\
            }\
        ");
    }
    else {
        engine.evaluate("__sioyek_keybind_function_index=0;\
                        function addHook(eventType, codeString){\
                            sioyek_api.register_hook_function(eventType, codeString);\
                        }\
                        function addKeybind(keybind, callable){\
                            let backtrace = __get_stacktrace();\
                            let line = new Error().stack;\
                            if (typeof(callable) == 'string'){\
                                sioyek_api.register_string_keybind(keybind, callable, backtrace[0], backtrace[1]);\
                            }\
                            else{\
                                let name = '__sioyek_keybind_function_' + __sioyek_keybind_function_index;\
                                __sioyek_keybind_function_index++;\
                                this[name] = callable;\
                                let hasWarning = sioyek_api.register_function_keybind(keybind, name, backtrace[0], backtrace[1]);\
                                return name;\
                            }\
                        }\
                        function addKeybindAsync(keybind, callable){\
                            let backtrace = __get_stacktrace();\
                            if (typeof(callable) !== 'string'){console.log('Error in ' + backtrace[0] + ':' + backtrace[1] + ': async function should be a string, if you are passing a raw function, you can convert it to a string by surrounding it with `.'); return;}\
                            sioyek_api.register_function_keybind_async(keybind, callable, backtrace[0], backtrace[1]);\
                        }\
                        ");
        engine.evaluate("\
            for (let i = 0; i < __all_command_names.length; i++){\
                let cname = __all_command_names[i];\
                sioyek[cname] = (...args)=>{\
                    let arg_strings = args.map((arg) => {return '' + arg;});\
                    let args_string = arg_strings.join(',');\
                    if (args_string.length > 0) {return sioyek_api.execute_macro_sync(cname, arg_strings);}\
                    else {return sioyek_api.execute_macro_sync(cname);}\
                    console.log('happened');\
                }\
            }\
        ");
    }


    return res;
}

bool remove_file(QString path) {
    QFile file(path);
    file.setPermissions(QFile::WriteOwner | QFile::ReadOwner | QFile::ExeOwner | QFile::WriteUser | QFile::ReadUser | QFile::ExeUser | QFile::WriteGroup | QFile::ReadGroup | QFile::ExeGroup | QFile::WriteOther | QFile::ReadOther | QFile::ExeOther);
    return file.remove();
}

void MainWidget::export_python_api() {
#ifndef SIOYEK_MOBILE
    QString res;
    QString INDENT = "    ";

    res += "class SioyekBase:\n\n";

    QStringList command_names = command_manager->get_all_command_names();
    for (auto command_name : command_names) {
        QString command_name_ = command_name;
        if (command_name_ == "import") {
            command_name_ = "import_";
        }

        if (command_name.size() > 0 && command_name[0] != '_') {

            auto command = command_manager->get_command_with_name(this, command_name.toStdString());
            res += INDENT + "def " + command_name_ + "(self";
            auto requirement = command->next_requirement(this);
            if (requirement) {
                res += ", *args, focus=False, wait=True, window_id=None):\n";
                res += INDENT + INDENT;
                res += "return self.run_command(\"" + command_name + "\", text=args, focus=focus, wait=wait, window_id=window_id)\n\n";
            }
            else {
                res += ", focus=False, wait=True, window_id=None):\n";
                res += INDENT + INDENT;
                res += "return self.run_command(\"" + command_name + "\", text=None, focus=focus, wait=wait, window_id=window_id)\n\n";
            }
        }

    }
    // ensure python_api_base_path/src/sioyek folder structure exists using Qt
    QString sioyek_python_lib_path = QString::fromStdWString(python_api_base_path.slash(L"src").slash(L"sioyek").get_path());
    QDir dir(sioyek_python_lib_path);
    bool dir_exists = false;

    if (!dir.exists()) {
        dir_exists = dir.mkpath(".");
    }
    else {
        dir_exists = true;
    }

    if (dir_exists) {
        // copy qrc:/python_api/LICENSE.txt and qrc:/python_api/pyproject.toml to python_api_base_path
        QString license_path = QString::fromStdWString(python_api_base_path.slash(L"LICENSE.txt").get_path());
        remove_file(license_path);
        QFile::copy(":/python_api/LICENSE.txt", license_path);

        QString readme_path = QString::fromStdWString(python_api_base_path.slash(L"README.md").get_path());
        remove_file(readme_path);
        QFile::copy(":/python_api/README.md", readme_path);

        QString pyproject_path = QString::fromStdWString(python_api_base_path.slash(L"pyproject.toml").get_path());
        remove_file(pyproject_path);
        QFile::copy(":/python_api/pyproject.toml", pyproject_path);

        QString sioyekpy = QString::fromStdWString(python_api_base_path.slash(L"src").slash(L"sioyek").slash(L"sioyek.py").get_path());
        remove_file(sioyekpy);
        QFile::copy(":/python_api/src/sioyek/sioyek.py", sioyekpy);

        //QString sioyekpy2 = QString::fromStdWString(python_api_base_path.slash(L"src").slash(L"sioyek").slash(L"sioyek2.py").get_path());
        //QFile::remove(sioyekpy2);
        //QFile::copy(":/python_api/src/sioyek/sioyek.py", sioyekpy2);

        QString init_path = QString::fromStdWString(python_api_base_path.slash(L"src").slash(L"sioyek").slash(L"__init__.py").get_path());
        remove_file(init_path);
        QFile::copy(":/python_api/src/sioyek/__init__.py", init_path);


        char* python_interpreter_path = std::getenv("SIOYEK_PYTHON_INTERPRETER_PATH");
        if (python_interpreter_path == nullptr) {
            show_error_message(L"You should set SIOYEK_PYTHON_INTERPRETER_PATH environment variables for export to work");
            return;
        }
        QString base_path = QString::fromStdWString(python_api_base_path.slash(L"src").slash(L"sioyek").slash(L"base.py").get_path());

        QFile output(base_path);

        if (output.open(QIODevice::WriteOnly)) {
            output.write(res.toUtf8());
        }
        output.close();


        //QDesktopServices::openUrl(QString::fromStdWString(python_api_base_path.get_path()));
        std::string command = std::string(python_interpreter_path) + " -m pip install " + python_api_base_path.get_path_utf8();
        std::system(command.c_str());
    }


#endif
}

bool MainWidget::execute_macro_from_origin(std::wstring macro_command_string, QLocalSocket* origin) {
    return execute_macro_if_enabled(macro_command_string, origin);
}

void MainWidget::show_custom_option_list(std::vector<std::wstring> options) {
    std::vector<std::vector<std::wstring>> values = { options };
    set_filtered_select_menu<std::wstring>(this, false, true, values, options, -1, [this](std::wstring* val) {
        //selected_option = *val;
        pending_command_instance->set_generic_requirement(QString::fromStdWString(*val));
        advance_command(std::move(pending_command_instance));
        },
        [](std::wstring* val) {

        });
    show_current_widget();
}

void MainWidget::on_socket_deleted(QLocalSocket* deleted_socket) {
    if (!(*should_quit)) {

        for (auto pc : commands_being_performed) {
            if (pc->result_socket == deleted_socket) {
                pc->set_result_socket(nullptr);
            }
        }
    }
}

void MainWidget::set_state(QJsonObject state) {

    if (state.contains("zoom_level")) {
        float new_zoom_level = state["zoom_level"].toDouble();
        main_document_view->set_zoom_level(new_zoom_level, true);
    }

    int new_page_number = -1;
    if (state.contains("page_number")) {
        new_page_number = state["page_number"].toInt();
        main_document_view->goto_page(new_page_number);
    }

    if (state.contains("document_path")) {
        main_document_view->open_document(state["document_path"].toString().toStdWString(), &this->is_render_invalidated);
    }

    if (state.contains("document_checksum")) {
        std::optional<std::wstring> path = document_manager->get_path_from_hash(state["document_checksum"].toString().toStdString());
        if (path.has_value()) {
            main_document_view->open_document(path.value(), &this->is_render_invalidated);
        }
    }

    if (state.contains("x_offset")) {
        float x_offset = state["x_offset"].toDouble();
        main_document_view->set_offset_x(x_offset);
    }

    if (state.contains("y_offset")) {
        float y_offset = state["y_offset"].toDouble();
        main_document_view->set_offset_y(y_offset);
    }

    if (state.contains("x_offset_in_page")) {
        float x_offset = state["x_offset_in_page"].toDouble();
        main_document_view->set_offset_x(x_offset);
    }

    if (state.contains("y_offset_in_page")) {
        float y_offset = state["y_offset_in_page"].toDouble();
        int page_number = new_page_number >= 0 ? new_page_number : main_document_view->get_center_page_number();
        main_document_view->goto_offset_within_page(page_number, y_offset);
    }

    if (state.contains("window_width")) {
        int new_width = state["window_width"].toInt();
        resize(new_width, height());
    }

    if (state.contains("window_height")) {
        int new_height = state["window_height"].toInt();
        resize(width(), new_height);
    }


    invalidate_render();

}

QJsonObject MainWidget::get_json_state() {
    QJsonObject result;
    if (doc()) {
        result["document_path"] = QString::fromStdWString(doc()->get_path());
        result["document_checksum"] = QString::fromStdString(doc()->get_checksum());

        int current_page = get_current_page_number();
        result["page_number"] = get_current_page_number();
        bool is_searching = main_document_view->get_is_searching(nullptr);
        result["searching"] = is_searching;
        if (is_searching) {
            int num_results = main_document_view->get_num_search_results();
            result["num_search_results"] = num_results;
        }
        float offset_x = main_document_view->get_offset_x();
        float offset_y =  main_document_view->get_offset_y();
        result["x_offset"] = offset_x;
        result["y_offset"] = offset_y;

        std::optional<AbsoluteRect> selected_rect_abs = main_document_view->get_selected_rect_absolute();
        if (selected_rect_abs) {
            int selected_rect_page;
            PagelessDocumentRect  selected_rect_doc = main_document_view->get_selected_rect_document()->rect;

            QJsonObject absrect_json;
            QJsonObject docrect_json;

            absrect_json["x0"] = selected_rect_abs->x0;
            absrect_json["x1"] = selected_rect_abs->x1;
            absrect_json["y0"] = selected_rect_abs->y0;
            absrect_json["y1"] = selected_rect_abs->y1;

            docrect_json["x0"] = selected_rect_doc.x0;
            docrect_json["x1"] = selected_rect_doc.x1;
            docrect_json["y0"] = selected_rect_doc.y0;
            docrect_json["y1"] = selected_rect_doc.y1;
            docrect_json["page"] = selected_rect_page;

            result["selected_rect_absolute"] = absrect_json;
            result["selected_rect_document"] = docrect_json;
        }


        AbsoluteDocumentPos abspos = { offset_x, offset_y };
        DocumentPos docpso = doc()->absolute_to_page_pos_uncentered(abspos);

        result["x_offset_in_page"] = docpso.x;
        result["y_offset_in_page"] = docpso.y;

        result["zoom_level"] = main_document_view->get_zoom_level();

        result["selected_text"] = QString::fromStdWString(main_document_view->get_selected_text());
        result["window_id"] = window_id;
        result["window_width"] = width();
        result["window_height"] = height();

        std::vector<std::wstring> loaded_document_paths = document_manager->get_loaded_document_paths();
        QJsonArray loaded_documents;
        for (auto docpath : loaded_document_paths) {
            loaded_documents.push_back(QString::fromStdWString(docpath));
        }

        result["loaded_documents"] = loaded_documents;

        if (main_document_view->get_overview_page()) {
            QJsonObject overview_state_json;
            OverviewState overview_state = main_document_view->get_overview_page().value();
            overview_state_json["y_offset"] = overview_state.absolute_offset_y;
            AbsoluteDocumentPos overview_abspos = { 0, overview_state.absolute_offset_y };
            Document* overview_doc = overview_state.doc ? overview_state.doc : doc();
            DocumentPos overview_docpos = overview_doc->absolute_to_page_pos_uncentered(overview_abspos);
            overview_state_json["target_page"] = overview_docpos.page;
            overview_state_json["y_offset_in_page"] = overview_docpos.y;
            overview_state_json["document_path"] = QString::fromStdWString(overview_doc->get_path());
            result["overview"] = overview_state_json;
        }
        if (dv()->smart_view_candidates.size() > 0) {
            QJsonArray smart_view_candidates_json;

            for (auto candid : dv()->smart_view_candidates) {
                QJsonObject candid_json_object;
                Document* candid_document = candid.doc ? candid.doc : doc();
                candid_json_object["document_path"] = QString::fromStdWString(candid_document->get_path());

                AbsoluteRect source_absolute_rect = candid.source_rect;
                int source_page = -1;
                DocumentRect source_page_rect = source_absolute_rect.to_document(doc());

                candid_json_object["source_absolute_rect"] = rect_to_json(source_absolute_rect);
                candid_json_object["source_document_rect"] = rect_to_json(source_page_rect.rect);
                candid_json_object["source_page"] = source_page;
                candid_json_object["source_text"] = QString::fromStdWString(candid.source_text);

                DocumentPos target_docpos = candid.get_docpos(main_document_view);
                AbsoluteDocumentPos target_abspos = candid.get_abspos(main_document_view);

                candid_json_object["target_document_x"] = target_docpos.x;
                candid_json_object["target_document_y"] = target_docpos.y;
                candid_json_object["target_document_page"] = target_docpos.page;

                candid_json_object["target_absolute_x"] = target_abspos.x;
                candid_json_object["target_absolute_y"] = target_abspos.y;

                smart_view_candidates_json.push_back(candid_json_object);
            }

            result["smart_view_candidates"] = smart_view_candidates_json;
        }

        
    }

    return result;
}

QJsonArray MainWidget::get_all_json_states() {
    QJsonArray result;
    for (auto window : windows) {
        result.append(window->get_json_state());
    }
    return result;
}

void MainWidget::screenshot(std::wstring file_path) {
    QPixmap pixmap(size());
    render(&pixmap, QPoint(), QRegion(rect()));
    pixmap.save(QString::fromStdWString(file_path));
}

void MainWidget::framebuffer_screenshot(std::wstring file_path) {
#ifdef SIOYEK_OPENGL_BACKEND
    QImage image = opengl_widget->grabFramebuffer();
    QPixmap pixmap = QPixmap::fromImage(image);
    pixmap.save(QString::fromStdWString(file_path));
#endif

    //QPixmap pixmap(size());
    //render(&pixmap, QPoint(), QRegion(rect()));
    //pixmap.save(QString::fromStdWString(file_path));
}

bool MainWidget::is_render_ready(){
    return  (!is_render_invalidated) && (!pdf_renderer->is_busy());
}

bool MainWidget::is_index_ready(){
    return !doc()->get_is_indexing();
}

bool MainWidget::is_search_ready() {
    return !pdf_renderer->is_search_busy();
}

void MainWidget::advance_waiting_command(std::string waiting_command_name) {
    //if ()
    if (pending_command_instance && (pending_command_instance->get_name().find(waiting_command_name) != -1)) {
        pending_command_instance->set_generic_requirement("");
        advance_command(std::move(pending_command_instance));
    }

}

void MainWidget::handle_select_current_search_match() {
    if (main_document_view->select_current_search_match()) {
        handle_stop_search();
    }
}

void MainWidget::handle_select_ruler_text() {
    dv()->select_ruler_text();
}

void MainWidget::handle_stop_search() {
    main_document_view->cancel_search();
    if (TOUCH_MODE) {
        get_search_buttons()->hide();
    }
}

int MainWidget::get_window_id() {
    return window_id;
}

void MainWidget::add_command_being_performed(Command* new_command) {
    commands_being_performed.push_back(new_command);
}

void MainWidget::remove_command_being_performed(Command* new_command) {
    auto index = std::find(commands_being_performed.begin(), commands_being_performed.end(), new_command);
    if (index != commands_being_performed.end()) {
        commands_being_performed.erase(index);
    }
}

QJsonObject MainWidget::get_json_annotations() {

    QJsonObject annots;
    annots["bookmarks"] = doc()->get_bookmarks_json();
    annots["highlights"] = doc()->get_highlights_json();
    annots["portals"] = doc()->get_portals_json();
    annots["marks"] = doc()->get_marks_json();
    return annots;
}

QString MainWidget::handle_action_in_menu(std::wstring action) {

    BaseSelectorWidget* selector_widget = nullptr;
    MyLineEdit* my_line_edit = nullptr;

    if (current_widget_stack.size() > 0) {
        selector_widget = dynamic_cast<BaseSelectorWidget*>(current_widget_stack.back());
    }
    my_line_edit = dynamic_cast<MyLineEdit*>(focusWidget());

    if (selector_widget) {
        if (action == L"down") {
            selector_widget->simulate_move_down();
        }
        if (action == L"up") {
            selector_widget->simulate_move_up();
        }
        if (action == L"page_down") {
            selector_widget->simulate_page_down();
        }
        if (action == L"page_up") {
            selector_widget->simulate_page_up();
        }
        if (action == L"menu_begin") {
            selector_widget->simulate_home();
        }
        if (action == L"menu_end") {
            selector_widget->simulate_end();
        }
        if (action == L"menu_close") {
            selector_widget->simulate_move_left();
        }
        if (action == L"menu_expand") {
            selector_widget->simulate_move_right();
        }
        if (action == L"select") {
            selector_widget->simulate_select();
        }
        if (action == L"get") {
            return selector_widget->get_selected_item();
        }
    }
    if (my_line_edit) {
        if ((!selector_widget) && action == L"select") {
            QKeyEvent* enter_event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
            QApplication::sendEvent(my_line_edit, enter_event);
        }
        if (action == L"cursor_backward") {
            my_line_edit->cursorBackward(false);
        }
        else if (action == L"cursor_forward") {
            my_line_edit->cursorForward(false);
        }
        else if (action == L"select_backward") {
            my_line_edit->cursorBackward(true);
        }
        else if (action == L"select_forward") {
            my_line_edit->cursorForward(true);
        }
        else if (action == L"move_word_backward") {
            my_line_edit->cursorWordBackward(false);
        }
        else if (action == L"move_word_forward") {
            my_line_edit->cursorWordForward(false);
        }
        else if (action == L"select_word_backward") {
            my_line_edit->cursorWordBackward(true);
        }
        else if (action == L"select_word_forward") {
            my_line_edit->cursorWordForward(true);
        }
        else if (action == L"move_to_end") {
            my_line_edit->end(false);
        }
        else if (action == L"select_to_end") {
            my_line_edit->end(true);
        }
        else if (action == L"move_to_begin") {
            my_line_edit->home(false);
        }
        else if (action == L"select_all") {
            my_line_edit->selectAll();
        }
        else if (action == L"select_to_begin") {
            my_line_edit->home(true);
        }
        else if (action == L"delete_to_end") {
            my_line_edit->end(true);
            my_line_edit->del();
        }
        else if (action == L"delete_to_begin") {
            my_line_edit->home(true);
            my_line_edit->del();
        }
        else if (action == L"delete_next_word") {
            my_line_edit->cursorWordForward(true);
            my_line_edit->del();
        }
        else if (action == L"delete_prev_word") {
            my_line_edit->cursorWordBackward(true);
            my_line_edit->del();
        }
        else if (action == L"delete_next_char") {
            my_line_edit->cursorForward(true);
            my_line_edit->del();
        }
        else if (action == L"delete_prev_char") {
            my_line_edit->cursorBackward(true);
            my_line_edit->del();
        }
        else if (action == L"next_suggestion" || action == L"down") {
            on_next_text_suggestion();
        }
        else if (action == L"prev_suggestion" || action == L"up") {
            on_prev_text_suggestion();
        }


    }
    return "";
}

std::wstring MainWidget::handle_synctex_to_ruler() {
    std::optional<NormalizedWindowRect> ruler_rect = main_document_view->get_ruler_window_rect();
    fz_irect ruler_irect = main_document_view->normalized_to_window_rect(ruler_rect.value());

    WindowPos mid_window_pos;
    mid_window_pos.x = (ruler_irect.x0 + ruler_irect.x1) / 2;
    mid_window_pos.y = (ruler_irect.y0 + ruler_irect.y1) / 2;

    return synctex_under_pos(mid_window_pos);
}

void MainWidget::show_touch_main_menu() {

    set_current_widget(new AndroidSelector(this));
    show_current_widget();
}

void MainWidget::show_touch_page_select() {

    int current_page = dv()->get_center_page_number();
    int num_pages = doc()->num_pages();
    set_current_widget(new PageSelectorUI(this, current_page, num_pages));
    show_current_widget();
}

void MainWidget::show_touch_highlight_type_select() {
    SelectHighlightTypeUI* new_widget = new SelectHighlightTypeUI(this);

    set_current_widget(new_widget);
    show_current_widget();
}

void MainWidget::highlight_type_color_clicked(int index) {
    this->select_highlight_type = 'a' + index;
    pop_current_widget();
    invalidate_ui();

}

void MainWidget::show_touch_settings_menu() {

    TouchSettings* config_menu = new TouchSettings(this);

    pop_current_widget();
    set_current_widget(config_menu);
    show_current_widget();

}

void MainWidget::free_document(Document* doc) {

    if ((helper_document_view_ != nullptr) && helper_document_view_->get_document() == doc) {
        helper_document_view()->set_null_document();
    }

    document_manager->free_document(doc);
}

bool MainWidget::is_helper_visible() {

    if (helper_document_view_ == nullptr) return false;

    if (helper_document_view() && helper_opengl_widget()) {
        return helper_opengl_widget()->isVisible();
    }
    return false;
}

std::wstring MainWidget::get_current_tabs_file_names() {
    std::wstring res;
    std::vector<std::wstring> file_names = document_manager->get_tabs();
    std::wstring current_doc_path;
    if (doc()) {
        current_doc_path = doc()->get_path();
    }

    res += current_doc_path;
    int begin_index = 0;
    if (file_names.size() > MAX_TAB_COUNT) {
        begin_index = file_names.size() - MAX_TAB_COUNT;
    }

    for (int i = begin_index; i < file_names.size(); i++) {
        if (file_names[i] == current_doc_path) continue;
        res += L"\n" + file_names[i];
    }

    return res;
}

void MainWidget::open_tabs(const std::vector<std::wstring>& tabs) {
    for (auto tab : tabs) {
        document_manager->add_tab(tab);
    }
}

void MainWidget::handle_goto_tab(const std::wstring& path) {

    // if there is a window with that tab, raise the window
    for (auto window : windows) {
        if (window->doc()) {
            if (window->doc()->get_path() == path) {
                window->raise();
                window->activateWindow();
                return;
            }
        }
    }

    push_state();
    open_document(path);
}

void MainWidget::handle_rename(std::wstring new_name) {
    Document* document_to_be_freed = doc();
    std::wstring path_to_be_freed = document_to_be_freed->get_path();

    std::vector<DocumentView*> document_views_referencing_doc;
    std::vector<MainWidget*> document_views_referencing_doc_widgets;
    std::vector<bool> document_view_is_helper;
    get_document_views_referencing_doc(path_to_be_freed, document_views_referencing_doc, document_views_referencing_doc_widgets, document_view_is_helper);

    std::vector<DocumentViewState> view_states;
    for (auto document_view : document_views_referencing_doc) {
        view_states.push_back(document_view->get_state());
        document_view->set_null_document();
    }

    pdf_renderer->free_all_resources_for_document(path_to_be_freed);
    free_document(document_to_be_freed);

    QFile old_qfile(QString::fromStdWString(path_to_be_freed));

    QFileInfo file_info(old_qfile);
    QString file_extension = file_info.fileName().split(".").back();
    QString new_file_name = QString::fromStdWString(new_name) + "." + file_extension;
    QString new_file_path = file_info.dir().filePath(new_file_name);

    if (old_qfile.rename(QString::fromStdWString(path_to_be_freed), new_file_path)) {
        db_manager->update_file_name(path_to_be_freed, new_file_path.toStdWString());
        document_views_open_path(document_views_referencing_doc, document_views_referencing_doc_widgets, document_view_is_helper, new_file_path.toStdWString());
        update_renamed_document_in_history(path_to_be_freed, new_file_path.toStdWString());
        restore_document_view_states(document_views_referencing_doc, view_states);
    }
    else {

        document_views_open_path(document_views_referencing_doc, document_views_referencing_doc_widgets, document_view_is_helper, path_to_be_freed);
        restore_document_view_states(document_views_referencing_doc, view_states);
        show_error_message(L"Could not rename the file maybe it is opened in another window or you don't have permission to rename it");
    }

}

void MainWidget::get_document_views_referencing_doc(std::wstring doc_path, std::vector<DocumentView*>& document_views, std::vector<MainWidget*>& corresponding_widgets, std::vector<bool>& is_helper){

    for (auto window : windows) {
        if (window->doc()) {
            if (window->doc()->get_path() == doc_path) {
                document_views.push_back(window->main_document_view);
                corresponding_widgets.push_back(window);
                is_helper.push_back(false);
            }
            if (window->helper_document_view_ && window->helper_document_view()->get_document() && (window->helper_document_view()->get_document()->get_path() == doc_path)) {
                document_views.push_back(window->main_document_view);
                corresponding_widgets.push_back(window);
                is_helper.push_back(true);
            }
        }
    }
}

void MainWidget::restore_document_view_states(const std::vector<DocumentView*>& document_views,const std::vector<DocumentViewState>& states){
    // assumes the length of vectors are equal

    for (int i = 0; i < document_views.size(); i++) {
        document_views[i]->set_book_state(states[i].book_state);
    }
}

void MainWidget::document_views_open_path(const std::vector<DocumentView*>& document_views, const std::vector<MainWidget*>& main_widgets, const std::vector<bool> is_helpers, std::wstring new_path) {

    for (int i = 0; i < document_views.size(); i++) {
        if (is_helpers[i]) {
            document_views[i]->open_document(new_path, &main_widgets[i]->is_render_invalidated);
        }
        else{
            main_widgets[i]->open_document(new_path);
        }
    }
}

void MainWidget::update_renamed_document_in_history(std::wstring old_path, std::wstring new_path){

    for (int i = 0; i < history.size(); i++) {
        if (history[i].document_path == old_path) {
            history[i].document_path = new_path;
        }
    }
}

void MainWidget::maximize_window() {
    showMaximized();
}

void MainWidget::handle_semantic_search_extractive(const std::wstring& query, bool has_tried_already) {

    const std::wstring& index = doc()->get_super_fast_index();

    sioyek_network_manager->semantic_search_extractive(this, QString::fromStdWString(query), index, [&, has_tried_already, query, document=doc()](QJsonObject resp) {
        if (document != doc()) return;

        QString status = resp["status"].toString();

        if (status == "NO_INDEX") {
            const std::wstring& local_index = document->get_super_fast_index();
            if (has_tried_already == false) {
                sioyek_network_manager->upload_document_index(this, local_index, [this, has_tried_already, query](QJsonObject res) {
                    handle_semantic_search_extractive(query, true);
                    });
            }
        }
        else {
            int range_begin = resp["start_index_in_document"].toInt();
            int range_end = resp["end_index_in_document"].toInt();

            if (range_begin >= 0 && range_end >= 0) {
                int page = -1;
                SearchResult current_result;
                current_result.begin_index_in_page = document->absolute_to_page_index(range_begin, page);
                current_result.end_index_in_page = document->absolute_to_page_index(range_end, page);
                current_result.page = page;

                main_document_view->set_search_results({ current_result });
                invalidate_render();
            }
        }
        });
}

void MainWidget::handle_semantic_search(const std::wstring& query, bool has_tried_already) {

    const std::wstring& index = doc()->get_super_fast_index();

    sioyek_network_manager->semantic_search(this, QString::fromStdWString(query), index, [&, has_tried_already, query, document=doc()](QJsonObject resp) {
        if (document != doc()) return;

        QString status = resp["status"].toString();

        if (status == "NO_INDEX") {
            const std::wstring& local_index = doc()->get_super_fast_index();
            if (has_tried_already == false) {
                sioyek_network_manager->upload_document_index(this, local_index, [this, has_tried_already, document, query](QJsonObject res) {
                    if (doc() != document) return;
                    handle_semantic_search(query, true);
                    });
            }
        }
        else {
            std::vector<SearchResult> search_results;

            QJsonArray highlights_json = resp["highlights"].toArray();
            for (int i = highlights_json.size()-1; i >= 0; i--) {
                SearchResult current_result;

                QJsonArray range_tuple_json = highlights_json.at(i).toArray();
                float range_begin = range_tuple_json.at(0).toInt();
                float range_end = range_tuple_json.at(1).toInt();

                int page = -1;
                current_result.begin_index_in_page = doc()->absolute_to_page_index(range_begin, page);
                current_result.end_index_in_page = doc()->absolute_to_page_index(range_end, page);
                current_result.page = page;

                search_results.push_back(current_result);

            }
            main_document_view->set_search_results(std::move(search_results));
            invalidate_render();
        }
        });
}

void MainWidget::run_command_with_name(std::string command_name, bool should_pop_current_widget) {
    auto command = command_manager->get_command_with_name(this, command_name);

    if (should_pop_current_widget) {
        pop_current_widget();
    }

    handle_command_types(std::move(command), 0);
}

QStringListModel* MainWidget::get_new_command_list_model() {
    return new QStringListModel(command_manager->get_all_command_names());
}

void MainWidget::add_password(std::wstring path, std::string password) {
    if (doc()){
        doc()->reload(password);
    }
    pdf_renderer->add_password(path, password);
}


int MainWidget::current_document_page_count(){
    if (doc()) {
        return doc()->num_pages();
    }
    return -1;
}

void MainWidget::goto_page_with_page_number(int page_number) {
    main_document_view->goto_page(page_number);
}

void MainWidget::goto_search_result(int nth_next_result, bool overview) {
    main_document_view->goto_search_result(nth_next_result, overview);
}

void MainWidget::set_should_highlight_words(bool should_highlight_words) {
    main_document_view->set_should_highlight_words(should_highlight_words);
}

void MainWidget::toggle_highlight_links() {
    main_document_view->toggle_highlight_links();
}

void MainWidget::set_highlight_links(bool should_highlight, bool should_show_numbers) {
    main_document_view->set_highlight_links(should_highlight, should_show_numbers);
}

void MainWidget::rotate_clockwise() {
    main_document_view->rotate_clockwise();
}

void MainWidget::rotate_counterclockwise() {
    main_document_view->rotate_counterclockwise();
}

void MainWidget::toggle_fastread() {
    main_document_view->toggle_fastread_mode();
}

void MainWidget::export_json(std::wstring json_file_path){
    db_manager->export_json(json_file_path, checksummer);
}

void MainWidget::import_json(std::wstring json_file_path){
    db_manager->import_json(json_file_path, checksummer);
}

bool MainWidget::does_current_widget_consume_quicktap_event(){
    if (current_widget_stack.size() > 0){
        /* return true; */
        if (dynamic_cast<TouchGenericButtons*>(current_widget_stack.back())){
            return false;
        }
        return true;
    }

    return false;
}

void MainWidget::initialize_helper(){
    helper_document_view_ = new DocumentView(db_manager, document_manager, checksummer);
    helper_opengl_widget_ = new PdfViewOpenGLWidget(helper_document_view_, pdf_renderer, document_manager, true);
#ifdef Q_OS_WIN
    // seems to be required only on windows though. TODO: test this on macos.
    // weird hack, should not be necessary but application crashes without it when toggling window configuration
    helper_opengl_widget_->show();
    helper_opengl_widget_->hide();
#endif
#ifdef Q_OS_MACOS
    QWidget* helper_window = get_top_level_widget(helper_opengl_widget_);
    if (MACOS_HIDE_TITLEBAR) {
        hideWindowTitleBar(helper_window->winId());
    }
    helper_opengl_widget_->show();
    helper_opengl_widget_->hide();
#endif

    set_color_mode_to_system_theme();

    helper_opengl_widget_->register_on_link_edit_listener([this](OpenedBookState state) {
            this->update_closest_link_with_opened_book_state(state);
            });
}

PdfViewOpenGLWidget* MainWidget::helper_opengl_widget(){

    if (helper_opengl_widget_ == nullptr){
        initialize_helper();
    }

    return helper_opengl_widget_;

}

DocumentView* MainWidget::helper_document_view(){

    if (helper_document_view_ == nullptr){
        initialize_helper();
    }

    return helper_document_view_;
}

void MainWidget::hide_command_line_edit(){
    text_command_line_edit->setText("");
    text_command_line_edit_container->hide();
    text_suggestion_index = 0;
    pending_command_instance = {};
    setFocus();
}

void MainWidget::deselect_document_indices(){
    main_document_view->set_selected_highlight_uuid("");
    main_document_view->set_selected_bookmark_uuid("");
    main_document_view->clear_selected_object();
}

QString MainWidget::run_macro_on_main_thread(QString macro_string, QStringList args, bool wait_for_result, int target_window_id) {
    MainWidget* target = this;
    if (target_window_id != -1) {
        target = get_window_with_window_id(target_window_id);
        if (target == nullptr) return "";
    }

    bool is_done = false;
    std::wstring result;
    if (wait_for_result) {
        QMetaObject::invokeMethod(target,
            "execute_macro_and_return_result",
            Qt::QueuedConnection,
            Q_ARG(QString, macro_string),
            Q_ARG(bool*, &is_done),
            Q_ARG(std::wstring*, &result),
            Q_ARG(std::optional<QStringList>, args)
        );
        while (!is_done) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        };
        return QString::fromStdWString(result);
    }
    else {
        QMetaObject::invokeMethod(target,
            "execute_macro_and_return_result",
            Qt::QueuedConnection,
            Q_ARG(QString, macro_string),
            Q_ARG(bool*, nullptr),
            Q_ARG(std::wstring*, nullptr),
            Q_ARG(std::optional<QStringList>, args)
        );
        return "";
    }
}

QString MainWidget::read_text_file(QString path) {
    bool is_done = false;
    QString res;

    QMetaObject::invokeMethod(this, [&, path]() {
        QFile file(path);
        if (file.open(QIODeviceBase::ReadOnly)) {
            res = QString::fromUtf8(file.readAll());
        }
        is_done = true;
        });

    while (!is_done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    };
    return res;
}

QByteArray MainWidget::perform_network_request(QString url, QString method, QString json_data){
    bool is_done = false;
    QByteArray res;

    QMetaObject::invokeMethod(this, [&, url]() {
            QNetworkRequest req;
            req.setUrl(url);
            if (json_data.size() > 0) {
                req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            }
            QNetworkReply* reply = nullptr;
            if (method == "get") {
                reply = sioyek_network_manager->get_network_manager()->get(req);
            }
            else if (method == "post") {
                reply = sioyek_network_manager->get_network_manager()->post(req, json_data.toUtf8());
            }

            if (reply) {
                reply->setProperty("sioyek_js_extension", "true");
                QObject::connect(reply, &QNetworkReply::finished, [&, reply]() {
                    //res = QString::fromUtf8(reply->readAll());
                    reply->deleteLater();
                    res = reply->readAll();
                    is_done = true;
                    });
            }
        });
    while (!is_done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    };
    return res;
}

void MainWidget::execute_macro_and_return_result(QString macro_string, bool* is_done, std::wstring* result, std::optional<QStringList> args) {
    std::unique_ptr<Command> command;
    if (args.has_value()) {
        command = command_manager->create_macro_command_with_args(this, "", macro_string, args.value());
    }
    else {
        command = command_manager->create_macro_command(this, "", macro_string.toStdWString());
    }
    if (is_done != nullptr) {
        command->set_result_mutex(is_done, result);
    }

    if (is_macro_command_enabled(command.get())) {
        handle_command_types(std::move(command), 0);
        invalidate_render();

    }
    else {
        if (is_done != nullptr) {
            *is_done = true;
        }
    }

}

void MainWidget::run_javascript_command(std::wstring javascript_code, std::optional<std::wstring> entry_point, bool is_async){

    QString content = QString::fromStdWString(javascript_code);
    if (entry_point.has_value()) {
        content += "\n" + QString::fromStdWString(entry_point.value()) + "()";
    }

    if (is_async) {
        std::thread ext_thread = std::thread([&, content]() {
            QJSEngine* engine = take_js_engine(true);
            auto res = engine->evaluate(content);
            release_async_js_engine(engine);
            });
        ext_thread.detach();
    }
    else {
        QJSEngine* engine = take_js_engine(false);
        QStringList stack_trace;
        auto res = engine->evaluate(content, QString(), 1, &stack_trace);
        //release_js_engine(engine);
        if (stack_trace.size() > 0) {
            for (auto line : stack_trace) {
                qDebug() << line;
            }
        }
    }

}

void MainWidget::load_sioyek_documentation(){
    if (sioyek_documentation_json_document.isNull()){
        QFile documentation_json_file(":/data/sioyek_documentation.json");
        if (documentation_json_file.open(QFile::ReadOnly)){
            sioyek_documentation_json_document = QJsonDocument().fromJson(documentation_json_file.readAll());
            documentation_json_file.close();
        }
    }
}

QString MainWidget::get_config_documentation_with_title(QString config, QString title) {
    load_sioyek_documentation();

    const QJsonObject& config_title_to_documentation_map = sioyek_documentation_json_document["config_title_to_documentation_map"].toObject();
    QJsonArray related_commands = sioyek_documentation_json_document["config_related_commands_map"][title].toArray();
    QJsonArray related_configs = sioyek_documentation_json_document["config_related_configs_map"][title].toArray();
    QString relateds = get_related_command_and_configs_string(related_commands, related_configs);

    if (config_title_to_documentation_map.value(title).isString()) {
        QString doc_string = "##" + config_title_to_documentation_map.value(title).toString().trimmed();

        if (config.size() > 0) {
            QString current_value_string = QString::fromStdWString(
                config.toStdWString() + L" " + config_manager->get_config_value_string(config.toStdWString())
                );
            QString res = doc_string + "\n\n" + "<hr/>\n\n#### current value:\n\n`" + current_value_string + "`\n\n";
            res += "[open config file in text editor](changeconfig-" + config + ")\n\n";
            res += "[temporarily change in sioyek](setconfig-" + config + ")\n\n";
            res += "[permanently change in sioyek](setsaveconfig-" + config + ")\n\n";

            return res + relateds;
        }
        else {
            return doc_string + relateds;
        }
    }

    return "";
}

QString MainWidget::get_related_command_and_configs_string(QJsonArray related_commands, QJsonArray related_configs) {
    QString result = "";

    const QJsonObject& command_name_to_title_map = sioyek_documentation_json_document["command_name_to_title_map"].toObject();
    const QJsonObject& config_name_to_title_map = sioyek_documentation_json_document["config_name_to_title_map"].toObject();

    if (related_commands.size() > 0) {
        result += "\n\nrelated commands: ";
        for (int i = 0; i < related_commands.size(); i++) {
            result += "[" + related_commands[i].toString() + "](commands.md#" + command_name_to_title_map[related_commands[i].toString()].toString() + ")";
            if (i < related_commands.size() - 1) {
                result += ", ";
            }
        }
    }

    if (related_configs.size() > 0) {
        result += "\n\nrelated configs: ";
        for (int i = 0; i < related_configs.size(); i++) {
            result += "[" + related_configs[i].toString() + "](configs.md#" + config_name_to_title_map[related_configs[i].toString()].toString() + ")";
            if (i < related_configs.size() - 1) {
                result += ", ";
            }
        }
    }
    return result;
}

QString MainWidget::get_command_documentation_with_title(QString title) {
    load_sioyek_documentation();

    const QJsonObject& command_title_to_documentation_map = sioyek_documentation_json_document["command_title_to_documentation_map"].toObject();

    QJsonArray related_commands = sioyek_documentation_json_document["command_related_commands_map"][title].toArray();
    QJsonArray related_configs = sioyek_documentation_json_document["command_related_configs_map"][title].toArray();

    if (command_title_to_documentation_map.value(title).isString()) {
        QString relateds = get_related_command_and_configs_string(related_commands, related_configs);
        return command_title_to_documentation_map.value(title).toString() + relateds;
    }

    return "";
}

QString get_config_command_prefix(QString config_command_name) {
    if (config_command_name.startsWith("setconfig_")) return "setconfig_";
    if (config_command_name.startsWith("toggleconfig_")) return "toggleconfig_";
    if (config_command_name.startsWith("saveconfig_")) return "saveconfig_";
    if (config_command_name.startsWith("setsaveconfig_")) return "setsaveconfig_";
    return "";
}

QString MainWidget::get_command_documentation(QString command_name, QString* out_url, QString* out_file_name){
    load_sioyek_documentation();

    const QJsonObject& command_name_to_title_map = sioyek_documentation_json_document["command_name_to_title_map"].toObject();
    const QJsonObject& command_title_to_documentation_map = sioyek_documentation_json_document["command_title_to_documentation_map"].toObject();
    const QJsonObject& config_name_to_title_map = sioyek_documentation_json_document["config_name_to_title_map"].toObject();

    int config_command_prefix_size = get_config_command_prefix(command_name).size();
    if (config_command_prefix_size > 0) {
        QString config_name = command_name.mid(config_command_prefix_size);
        const QJsonObject& config_title_to_documentation_map = sioyek_documentation_json_document["config_title_to_documentation_map"].toObject();
        const QJsonObject& config_name_to_file_name_map = sioyek_documentation_json_document["config_name_to_file_name_map"].toObject();
        const QJsonObject& config_related_commands_map = sioyek_documentation_json_document["config_related_commands_map"].toObject();
        const QJsonObject& config_related_configs_map = sioyek_documentation_json_document["config_related_configs_map"].toObject();

        if (config_name_to_title_map.value(config_name).isString()) {
            QString documentation_title = config_name_to_title_map.value(config_name).toString();
            if (out_url) *out_url = "configs.md#" + documentation_title;
            if (out_file_name) *out_file_name = config_name_to_file_name_map.value(config_name).toString();
            return get_config_documentation_with_title(config_name, documentation_title);
        }
        else if (!config_name_to_file_name_map.value(config_name).isNull()) {
            if (out_url) *out_url = "configs.md#" + config_name_to_file_name_map.value(config_name).toString();
            if (out_file_name) *out_file_name = config_name_to_file_name_map.value(config_name).toString();
        }
    }

    if (command_name_to_title_map.value(command_name).isString()){
        QString documentation_title = command_name_to_title_map.value(command_name).toString();
        const QJsonObject& command_name_to_file_name_map = sioyek_documentation_json_document["command_name_to_file_name_map"].toObject();
        const QJsonObject& command_related_commands_map = sioyek_documentation_json_document["command_related_commands_map"].toObject();
        const QJsonObject& command_related_configs_map = sioyek_documentation_json_document["command_related_configs_map"].toObject();

        if (command_title_to_documentation_map.value(documentation_title).isString()) {
            if (out_url) *out_url = "commands.md#" + documentation_title;
            if (out_file_name) *out_file_name = command_name_to_file_name_map.value(command_name).toString();
            //return command_title_to_documentation_map.value(documentation_title).toString();
            return get_command_documentation_with_title(documentation_title);
        }
    }

    return "";
}

void MainWidget::print_undocumented_configs(){
    load_sioyek_documentation();
    std::vector<Config*> all_configs = config_manager->get_configs();
    std::vector<QRegularExpression> regex_config_titles; // some documentation config titles are regexes e.g.: "highlight_type_[a-z]"


    for (auto key : sioyek_documentation_json_document["config_name_to_title_map"].toObject().keys()) {
        if ((key.indexOf("[") >= 0)){
            regex_config_titles.push_back(QRegularExpression(key));
        }
        else if (key.indexOf("*") >= 0) {
            key = key.replace("*", "[a-z_]*");
            regex_config_titles.push_back(QRegularExpression(key));
        }
    }

    for (auto& config : all_configs) {
        QString config_name = QString::fromStdWString(config->name);
        if (sioyek_documentation_json_document["config_name_to_title_map"][config_name].toString().size() == 0) {
            // don't print configs like highlight_type_a to highlight_type_z
            if (config_name.size() > 2 && config_name[config_name.size() - 2] == '_') {
                continue;
            }
            if ((config_name.indexOf("visual_mark") >= 0) || (config_name.indexOf("vertical_line") >= 0)) {
                // old names replaced with ruler_ commands
                continue;
            }

            bool found_regex = false;
            for (auto regex : regex_config_titles) {
                if (regex.match(config_name).hasMatch()) {
                    found_regex = true;
                }
            }
            if (found_regex) {
                continue;
            }
            qDebug() << config_name;

        }
    }

}

void MainWidget::print_undocumented_commands(){
    load_sioyek_documentation();

    auto all_commands = command_manager->get_all_command_names();
    for (auto& command : all_commands) {
        if (sioyek_documentation_json_document["command_name_to_title_map"][command].toString().size() == 0) {
            if (command.indexOf("config_") == -1) {
                qDebug() << command;
            }
        }
    }
}

void MainWidget::print_non_default_configs(){
    auto configs = config_manager->get_configs();
    for (auto conf : configs){
        if (conf->has_changed_from_default()){
            qDebug() << "___________";
            qDebug() << "name: " << conf->name;
            qDebug() << "default: " << conf->default_value_string;
            qDebug() << "current: " << conf->get_current_string();
        }
    }
}

void MainWidget::set_text_prompt_text(QString text) {

    if (!TOUCH_MODE) {
        text_command_line_edit->setText(text);
    }
    else {
        if (current_widget_stack.size() > 0) {
            TouchTextEdit* text_edit = dynamic_cast<TouchTextEdit*>(current_widget_stack.back());
            text_edit->set_text(text.toStdWString());
        }
    }
}

DocumentView* MainWidget::dv() {
    if (main_document_view->scratchpad) {
        return scratchpad;
    }
    else{
        return main_document_view;
    }
}

bool MainWidget::should_draw(bool originated_from_pen) {

    if (rect_select_mode) return false;
    if (main_document_view && (main_document_view->freehand_drawing_move_data || main_document_view->selected_freehand_drawings)) return false;

    if (TOUCH_MODE){
        if (opengl_widget && main_document_view->scratchpad) {
            return originated_from_pen;
        }

        if (freehand_drawing_mode == DrawingMode::Drawing) {
            return true;
        }

        if (freehand_drawing_mode == DrawingMode::PenDrawing && originated_from_pen) {
            return true;
        }
    }
    else{
        if (opengl_widget && main_document_view->scratchpad) {
            return true;
        }

        if (freehand_drawing_mode == DrawingMode::Drawing) {
            return true;
        }

        if (freehand_drawing_mode == DrawingMode::PenDrawing && originated_from_pen) {
            return true;
        }
    }


    return false;
}

bool MainWidget::is_scratchpad_mode(){
    return main_document_view->scratchpad != nullptr;
}

void MainWidget::toggle_scratchpad_mode(){
    if (main_document_view->scratchpad) {
        main_document_view->scratchpad = nullptr;
    }
    else {
        scratchpad->on_view_size_change(width(), height());
        main_document_view->scratchpad = scratchpad;
    }

    if (draw_controls_) {
        if (main_document_view->scratchpad) {
            draw_controls_->controls_ui->set_scratchpad_mode(true);
        }
        else {
            draw_controls_->controls_ui->set_scratchpad_mode(false);
        }
    }
}

void MainWidget::add_pixmap_to_scratchpad(QPixmap pixmap) {
    scratchpad->add_pixmap(pixmap);
}

void MainWidget::save_scratchpad() {
    auto scratchpad_file_name = doc()->get_scratchpad_file_path();
    scratchpad->save(scratchpad_file_name);
}

void MainWidget::load_scratchpad() {
    auto scratchpad_file_name = doc()->get_scratchpad_file_path();
    scratchpad->load(scratchpad_file_name);
}

void MainWidget::clear_scratchpad() {
    scratchpad->clear();
    invalidate_render();
}

void MainWidget::show_draw_controls() {
    get_draw_controls()->show();
}

PaperDownloadFinishedAction MainWidget::get_default_paper_download_finish_action() {
    if (PAPER_DOWNLOAD_CREATE_PORTAL) {
        return PaperDownloadFinishedAction::Portal;
    }

    return PaperDownloadFinishedAction::OpenInNewWindow;
}


void MainWidget::set_tag_prefix(std::wstring prefix) {

    main_document_view->set_tag_prefix(prefix);
}

void MainWidget::clear_tag_prefix() {
    main_document_view->clear_tag_prefix();
}

QPoint MainWidget::cursor_pos() {
    if (context_menu_right_click_pos) {
        return context_menu_right_click_pos.value();
    }
    return QCursor::pos();
}


void MainWidget::clear_current_page_drawings() {
    int page_number = get_current_page_number();
    doc()->delete_all_page_drawings(page_number);
}

void MainWidget::clear_current_document_drawings() {
    doc()->delete_all_drawings();
}

void MainWidget::handle_goto_link_with_page_and_offset(int page, float y_offset) {
    long_jump_to_destination(page, y_offset);
    //if (ALIGN_LINK_DEST_TO_TOP) {
    //    float top_offset = (main_document_view->get_view_height() / main_document_view->get_zoom_level()) / 2.0f;
    //    main_document_view->move_absolute(0, top_offset);
    //}
}

QString MainWidget::execute_macro_sync(QString macro, QStringList args) {
    std::unique_ptr<Command> command = command_manager->create_macro_command_with_args(this, "", macro, args);
    std::wstring result;

    if (is_macro_command_enabled(command.get())) {
        handle_command_types(std::move(command), 0, &result);
        invalidate_render();

        return QString::fromStdWString(result);
    }

    return "";
}

void MainWidget::set_variable(QString name, QVariant var) {
    js_variables[name] = var;
}

QVariant MainWidget::get_variable(QString name) {
    return js_variables[name];
}

void MainWidget::on_next_text_suggestion() {
    if (pending_command_instance) {
        bool this_has_value = pending_command_instance->get_text_suggestion(text_suggestion_index).has_value();
        bool next_has_value = pending_command_instance->get_text_suggestion(text_suggestion_index + 1).has_value();
        if (!this_has_value && !next_has_value) return;

        text_suggestion_index++;
        set_current_text_suggestion();
    }
}

void MainWidget::on_prev_text_suggestion() {
    if (pending_command_instance) {
        bool this_has_value = pending_command_instance->get_text_suggestion(text_suggestion_index).has_value();
        bool next_has_value = pending_command_instance->get_text_suggestion(text_suggestion_index - 1).has_value();
        if (!this_has_value && !next_has_value) return;

        text_suggestion_index--;
        set_current_text_suggestion();
    }
}

void MainWidget::set_current_text_suggestion() {
    if (pending_command_instance) {
        std::optional<std::wstring> suggestion = pending_command_instance->get_text_suggestion(text_suggestion_index);
        if (suggestion) {
            text_command_line_edit->setText(QString::fromStdWString(suggestion.value()));
        }
        else {
            text_command_line_edit->setText("");
        }

    }
}

std::optional<std::wstring> MainWidget::get_search_suggestion_with_index(int index) {
    if (index >= 0 || (-index > search_terms.size())) {
        return {};
    }
    else {
        return search_terms[search_terms.size() + index];
    }
}

bool MainWidget::is_menu_focused() {
    if (dynamic_cast<MyLineEdit*>(focusWidget())) {
        return true;
    }
    return false;
}


void MainWidget::ensure_player_state(QString state) {
    if (current_widget_stack.size() > 0) {
        AudioUI* audio_ui = dynamic_cast<AudioUI*>(current_widget_stack.back());
        if (audio_ui) {
            if (state == "Ready") {
                audio_ui->buttons->set_playing();
                is_reading = true;
            }
            if (state == "Ended") {
                audio_ui->buttons->set_paused();
                is_reading = false;
            }
        }
    }
}

void MainWidget::ensure_player_state_(QString state) {
    QMetaObject::invokeMethod(this,
        "ensure_player_state",
        Qt::QueuedConnection,
        Q_ARG(QString, state)
    );
}

QString MainWidget::get_rest_of_document_pages_text() {
    int page_number = main_document_view->get_vertical_line_page();

    // last_pause_rest_of_document_page = page_number + 1;
#ifdef SIOYEK_ANDROID
    int page_offset = doc()->get_page_offset_into_super_fast_index(page_number + 1);
    return QString::number(page_offset) + " " + doc()->get_rest_of_document_pages_text(page_number + 1).left(100000);
#else
    return doc()->get_rest_of_document_pages_text(page_number + 1).left(100000);
#endif
}

void MainWidget::focus_on_character_offset_into_document(int character_offset_into_document) {
    main_document_view->focus_on_character_offset_into_document(character_offset_into_document);
    invalidate_render();
}

void MainWidget::handle_move_smooth_hold(bool down) {

    float max_velocity = down ? -SMOOTH_MOVE_MAX_VELOCITY : SMOOTH_MOVE_MAX_VELOCITY;

    if (down) {
        main_document_view->velocity_y -= (main_document_view->velocity_y - max_velocity) / 5.0f;
    }
    else {
        main_document_view->velocity_y += (max_velocity - main_document_view->velocity_y) / 5.0f;
    }

    validation_interval_timer->setInterval(0);
    last_speed_update_time = QTime::currentTime();
}

void MainWidget::handle_toggle_two_page_mode() {
    main_document_view->toggle_two_page();
    if (NUM_CACHED_PAGES < 6) {
        if (main_document_view->is_two_page_mode()) {
            pdf_renderer->set_num_cached_pages(NUM_CACHED_PAGES * 2);
        }
        else {
            pdf_renderer->set_num_cached_pages(NUM_CACHED_PAGES);
        }
    }
}


void MainWidget::ensure_zero_interval_timer(){
    validation_interval_timer->setInterval(INTERVAL_TIME);
}


void MainWidget::set_last_performed_command(std::unique_ptr<Command> command) {
    if (last_performed_command) {
        last_performed_command->perform_up();
    }

    last_performed_command = std::move(command);
}

void MainWidget::make_current_menu_columns_equal() {
    if (!TOUCH_MODE && current_widget_stack.size() > 0) {
        FilteredSelectTableWindowClass<std::string>* widget = dynamic_cast<FilteredSelectTableWindowClass<std::string>*>(current_widget_stack.back());
        FilteredSelectTableWindowClass<std::wstring>* widget2 = dynamic_cast<FilteredSelectTableWindowClass<std::wstring>*>(current_widget_stack.back());
        if (widget) {
            widget->set_equal_columns();
        }
        if (widget2) {
            widget2->set_equal_columns();
        }
    }
}

void MainWidget::set_highlighted_tags(std::vector<std::string> tags) {
    main_document_view->set_highlighted_tags(tags);
}

AbsoluteDocumentPos MainWidget::get_mouse_abspos() {
    auto pos = mapFromGlobal(QCursor::pos());
    WindowPos window_pos = { pos.x(), pos.y() };
    AbsoluteDocumentPos abspos = window_pos.to_absolute(main_document_view);
    return abspos;
}

bool MainWidget::handle_annotation_move_finish(){

    if (main_document_view->visible_object_move_data && main_document_view->visible_object_move_data->is_moving) {
        main_document_view->visible_object_move_data->handle_move_end(this);
        main_document_view->visible_object_move_data = {};
        stop_dragging();
        main_document_view->is_selecting = false;
        return true;
    }

    if (main_document_view->freehand_drawing_move_data) {
        main_document_view->handle_freehand_drawing_move_finish(get_cursor_abspos());
        invalidate_render();
        main_document_view->is_selecting = false;
        return true;
    }

    return false;
}

void MainWidget::set_fixed_velocity(float vel) {
    main_document_view->velocity_y = vel;
    main_document_view->is_velocity_fixed = true;
    if (vel == 0) {
        main_document_view->is_velocity_fixed = false;
        if (validation_interval_timer->interval() == 0){
            validation_interval_timer->setInterval(INTERVAL_TIME);
        }
    }
}

void MainWidget::create_menu_from_menu_node(
    QMenu* parent,
    MenuNode* items,
    std::unordered_map<std::string,
    std::vector<std::string>>& command_key_mappings) {

    if (items->children.size() == 0) {
        // this is a command

        std::string command = items->name.toStdString();
        if (command[0] == '-' && command.size() == 1) {
            parent->addSeparator();
            return;
        }
        std::string human_readable_name;

        if (items->doc.size() > 0) {
            human_readable_name = items->doc.toStdString();
        }
        else {
            auto cmd = command_manager->get_command_with_name(this, command);
            if (cmd){
                human_readable_name = cmd->get_human_readable_name();
            }
        }

        std::vector<std::string> key_mappings;

        if (command_key_mappings.find(command) != command_key_mappings.end()){
            key_mappings = command_key_mappings[command];
        }

        QString action_menu_name = QString::fromStdString(human_readable_name);

        if (key_mappings.size() > 0){
            action_menu_name += " ( " + translate_key_mapping_to_macos(QString::fromStdString(key_mappings[0])) + " ) ";
        }

        auto command_action = parent->addAction(action_menu_name);
        connect(command_action, &QAction::triggered, [&, command](){

            auto cmd = command_manager->get_command_with_name(this, command);
            if (cmd) {
                handle_command_types(std::move(cmd), 0);
            }
            else {
                execute_macro_if_enabled(utf8_decode(command));
            }
        });

    }
    else {
        auto menu = parent->addMenu(items->name);
        for (auto&& child : items->children) {
            create_menu_from_menu_node(menu, child, command_key_mappings);
        }
    }
}

QMenuBar* MainWidget::create_main_menu_bar(){
    std::vector<MenuNode*> top_level_menus = get_top_level_menu_nodes();

    auto command_key_mappings = input_handler->get_command_key_mappings();
    QMenuBar* menu_bar = new QMenuBar(this);
    for (auto top_level_menu : top_level_menus) {
        auto parent_menu = menu_bar->addMenu(top_level_menu->name);
        for (auto child : top_level_menu->children) {
            create_menu_from_menu_node(parent_menu, child, command_key_mappings);
        }
    }

    QMenu* help_menu = menu_bar->addMenu("Help");
    QAction* donate_action = help_menu->addAction("Donate");
    donate_action->setShortcut(QKeySequence(Qt::Key_Meta | Qt::Key_PageDown));


    connect(donate_action, &QAction::triggered, [&](){
        execute_macro_if_enabled(L"donate");
    });

    for (auto top_level_menu : top_level_menus) {
        delete_menu_nodes(top_level_menu);
    }

    return menu_bar;
}

void MainWidget::delete_menu_nodes(MenuNode* items) {
    for (auto child : items->children) {
        delete_menu_nodes(child);
    }
    delete items;
}

bool MainWidget::is_ruler_mode(){
    return main_document_view->is_ruler_mode();
}

void MainWidget::register_hook_function(QString type, QString name) {
    if (type == "add_bookmark") {
        add_bookmark_hook_function_name = name;
    }
    else if (type == "delete_bookmark") {
        delete_bookmark_hook_function_name = name;
    }
    else if (type == "edit_bookmark") {
        edit_bookmark_hook_function_name = name;
    }
    else if (type == "add_highlight") {
        add_highlight_hook_function_name = name;
    }
    else if (type == "delete_highlight") {
        delete_highlight_hook_function_name = name;
    }
    else if (type == "highlight_annotation_changed") {
        highlight_annotation_changed_hook_function_name = name;
    }
    else if (type == "highlight_type_changed") {
        highlight_type_changed_hook_function_name = name;
    }
    else if (type == "add_mark") {
        add_mark_hook_function_name = name;
    }
    else if (type == "add_portal") {
        add_portal_hook_function_name = name;
    }
    else if (type == "delete_portal") {
        delete_portal_hook_function_name = name;
    }
    else if (type == "edit_portal") {
        edit_portal_hook_function_name = name;
    }
    else if (type == "open_document") {
        open_document_hook_function_name = name;
    }
    else if (type == "open_new_document") {
        open_new_document_hook_function_name = name;
    }
}

bool MainWidget::register_function_keybind(QString keybind, QString function_name, QString file_name, int line_number){

    return input_handler->add_keybind(
        keybind.toStdWString(),
        L"{jscall}" + function_name.toStdWString(),
        file_name.toStdWString(),
        line_number
    );
}

void MainWidget::register_function_keybind_async(QString keybind, QString code, QString file_name, int line_number){
    input_handler->add_keybind(
        keybind.toStdWString(),
        L"{jsasync}" + code.toStdWString(),
        file_name.toStdWString(),
        line_number
    );
}

void MainWidget::register_string_keybind(QString keybind, QString commands_string, QString file_name, int line_number){
    input_handler->add_keybind(
        keybind.toStdWString(),
        commands_string.toStdWString(),
        file_name.toStdWString(),
        line_number
    );
}

void MainWidget:: run_startup_js(bool first_run) {
#ifndef SIOYEK_MOBILE
    QString js_path_qstring = QString::fromStdWString(sioyek_js_path.get_path());
    QFile sioyek_js(js_path_qstring);

    if (sioyek_js.exists()) {
        sioyek_js.open(QIODeviceBase::ReadOnly);
        auto js_engine = take_js_engine(false);
        QString prelude = "let __first_run = %{FIRST_RUN};\n\
            if (!__first_run){addKeybind = ()=>{}; addKeybindAsync = ()=>{}}\n\
            function __get_stacktrace(){\n\
                let lines = new Error().stack.split('\\n');\n\
                let line = lines[lines.length-1];\n\
                let lineNumber =  (0 + line.split(':')[1]) - %{PRELUDE_LINES};\n\
                return ['"+ js_path_qstring + "', lineNumber];\
            }\
            ";
        int n_lines = prelude.count('\n');
        prelude = prelude.replace("%{PRELUDE_LINES}", QString::number(n_lines));
        prelude = prelude.replace("%{FIRST_RUN}", first_run ? "true" : "false");
        QString file_data = QString::fromUtf8(sioyek_js.readAll());
        js_engine->evaluate(prelude + file_data);
        sioyek_js.close();
    }
#endif
}

void MainWidget::open_external_text_editor() {
    if (EXTERNAL_TEXT_EDITOR_COMMAND.size() == 0) {
        show_error_message(L"You should configure external_text_editor_command in you configs file");
        return;
    }

    QString path_qstring = QString::fromStdWString(sioyek_temp_text_path.get_path());
    QFile external_file(path_qstring);

    external_file.open(QIODeviceBase::WriteOnly);
    external_file.write(text_command_line_edit->text().toUtf8());
    external_file.close();

    std::wstring command = QString::fromStdWString(EXTERNAL_TEXT_EDITOR_COMMAND)
        .replace("%{file}", QString::fromStdWString(sioyek_temp_text_path.get_path()))
        .replace("%{line}", QString::number(1))
        .toStdWString();
    execute_command(command);
    is_external_file_edited = false;
    if (external_command_edit_watcher.files().size() == 0) {
        external_command_edit_watcher.addPath(path_qstring);
    }
}

void MainWidget::on_bookmark_deleted(const BookMark& bookmark, const std::string& document_checksum){
    DeletedObject deleted_bookmark_object;
    deleted_bookmark_object.document_checksum = document_checksum;
    deleted_bookmark_object.object = bookmark;
    deleted_objects.push_back(deleted_bookmark_object);

    if (delete_bookmark_hook_function_name) {
        call_async_js_function_with_args(delete_bookmark_hook_function_name.value(), QJsonArray() << QString::fromStdString(bookmark.uuid));
    }

    // if we have deleted a bookmark which shows the output of a command
    // we need to kill the process and remove it from shell_output_bookmarks
    for (int i = 0; i < shell_output_bookmarks.size(); i++){
        if (shell_output_bookmarks[i].uuid == bookmark.uuid) {
            kill_process(shell_output_bookmarks[i].pid);
            remove_finished_shell_bookmark_with_index(i);
            break;
        }
    }

    sync_deleted_annot("bookmark", bookmark.uuid);
}

void MainWidget::on_highlight_deleted(const Highlight& hl, const std::string& document_checksum){

    DeletedObject deleted_highlight_object;
    deleted_highlight_object.document_checksum = document_checksum;
    deleted_highlight_object.object = hl;
    deleted_objects.push_back(deleted_highlight_object);

    if (delete_highlight_hook_function_name) {
        call_async_js_function_with_args(delete_highlight_hook_function_name.value(), QJsonArray() << QString::fromStdString(hl.uuid));
    }
    sync_deleted_annot("highlight", hl.uuid);
}

void MainWidget::sync_deleted_annot(const std::string& annot_type, const std::string& uuid) {
    sioyek_network_manager->sync_deleted_annot(this, doc(), annot_type, uuid);
}


void MainWidget::delete_highlight_with_uuid(const std::string& uuid) {
    //db_manager->delete_highlight(uuid);
    std::vector<std::pair<std::string, Highlight>> deleted_highlight;
    db_manager->select_highlight_with_uuid(uuid, deleted_highlight);

    if (deleted_highlight.size() > 0) {
        if (deleted_highlight[0].first == doc()->get_checksum()) {
            doc()->delete_highlight(deleted_highlight[0].second);
        }
        else {
            db_manager->delete_highlight(uuid);
        }

        on_highlight_deleted(deleted_highlight[0].second, deleted_highlight[0].first);
    }
}

std::optional<Highlight> MainWidget::delete_current_document_highlight_with_uuid(const std::string& uuid) {
    std::optional<Highlight> deleted_highlight = doc()->delete_highlight_with_uuid(uuid);
    if (deleted_highlight.has_value()) {
        on_highlight_deleted(deleted_highlight.value(), doc()->get_checksum());
    }
    return deleted_highlight;
}

void MainWidget::delete_current_document_highlight(Highlight* hl) {
    std::string uuid = hl->uuid;
    std::optional<Highlight> deleted_highlight = doc()->delete_highlight(*hl);
    if (deleted_highlight) {
        on_highlight_deleted(deleted_highlight.value(), doc()->get_checksum());
    }
}

void MainWidget::delete_current_document_bookmark_with_bookmark(BookMark* bm) {
    delete_current_document_bookmark(bm->uuid);
}

void MainWidget::on_bookmark_edited(const std::string& uuid) {
    if (edit_bookmark_hook_function_name) {
        call_js_function_with_bookmark_arg_with_uuid(edit_bookmark_hook_function_name.value(), uuid);
    }
    sync_edited_annot("bookmark", uuid);
}

//void MainWidget::call_js_function_with_args(const QString& function_name) {
void MainWidget::call_async_js_function_with_args(const QString& code, QJsonArray args){
    //auto engine = take_js_engine(true);
    //QJSValue func = engine->evaluate(function_name);
    std::thread ext_thread = std::thread([&, code, args]() {
        QJSEngine* engine = take_js_engine(true);
        //auto jsargs = engine->toScriptValue(args);
        auto func = engine->evaluate(code);
        QJSValueList js_args;
        for (auto arg : args) {
            if (arg.isArray()) {
                js_args.push_back(engine->toScriptValue(arg.toArray()));
            }
            else if (arg.isBool()) {
                js_args.push_back(engine->toScriptValue(arg.toBool()));
            }
            else if (arg.isDouble()) {
                js_args.push_back(engine->toScriptValue(arg.toDouble()));
            }
            else if (arg.isObject()) {
                js_args.push_back(engine->toScriptValue(arg.toObject()));
            }
            else if (arg.isString()) {
                js_args.push_back(engine->toScriptValue(arg.toString()));
            }
        }
        func.call(js_args);
        release_async_js_engine(engine);
        });
    ext_thread.detach();

}

void MainWidget::call_js_function_with_bookmark_arg_with_uuid(const QString& function_name, const std::string& uuid) {
    int bookmark_index = doc()->get_bookmark_index_with_uuid(uuid);
    if (bookmark_index >= 0 && bookmark_index < doc()->get_bookmarks().size()) {
        BookMark bookmark = doc()->get_bookmarks()[bookmark_index];
        call_async_js_function_with_args(function_name, QJsonArray() << bookmark.to_json(""));
    }
}

void MainWidget::call_js_function_with_highlight_arg_with_uuid(const QString& function_name, const std::string& uuid) {
    int highlight_index = doc()->get_highlight_index_with_uuid(uuid);
    if (highlight_index >= 0 && highlight_index < doc()->get_highlights().size()) {
        Highlight highlight = doc()->get_highlights()[highlight_index];

        call_async_js_function_with_args(function_name, QJsonArray() << highlight.to_json(""));
    }
}

void MainWidget::call_js_function_with_portal_arg_with_uuid(const QString& function_name, const std::string& uuid) {
    int portal_index = doc()->get_portal_index_with_uuid(uuid);
    if (portal_index >= 0 && portal_index < doc()->get_portals().size()) {
        Portal portal = doc()->get_portals()[portal_index];
        call_async_js_function_with_args(function_name, QJsonArray() << portal.to_json(""));
    }
}

void MainWidget::on_new_bookmark_added(const std::string& uuid) {
    if (add_bookmark_hook_function_name) {
        call_js_function_with_bookmark_arg_with_uuid(add_bookmark_hook_function_name.value(), uuid);
    }

    sync_newly_added_annot("bookmark", uuid);

    doc()->update_last_local_edit_time();
}

void MainWidget::on_new_portal_added(const std::string& uuid) {
    if (add_portal_hook_function_name) {
        call_js_function_with_portal_arg_with_uuid(add_bookmark_hook_function_name.value(), uuid);
    }
    sync_newly_added_annot("portal", uuid);
    if (AUTOMATICALLY_UPLOAD_PORTAL_DESTINATION_FOR_SYNCED_DOCUMENTS) {
        int portal_index = doc()->get_portal_index_with_uuid(uuid);
        if (portal_index >= 0) {
            Portal& portal = doc()->get_portals()[portal_index];
            if (!sioyek_network_manager->is_checksum_available_on_server(portal.dst.document_checksum)) {
                std::optional<std::wstring> document_path = document_manager->get_path_from_hash(portal.dst.document_checksum);
                if (document_path) {
                    sioyek_network_manager->upload_file(
                        QApplication::instance(),
                        QString::fromStdWString(document_path.value()),
                        QString::fromStdString(portal.dst.document_checksum), [network_manager=sioyek_network_manager]() {
                            network_manager->update_user_files_hash_set();
                        });
                }
            }
        }
    }

    doc()->update_last_local_edit_time();
}

void MainWidget::on_portal_deleted(const Portal& deleted_portal, const std::string& document_checksum) {

    DeletedObject deleted_portal_object;
    deleted_portal_object.document_checksum = document_checksum;
    deleted_portal_object.object = deleted_portal;
    deleted_objects.push_back(deleted_portal_object);

    if (delete_portal_hook_function_name) {
        call_async_js_function_with_args(delete_portal_hook_function_name.value(),
            QJsonArray() << QString::fromStdString(deleted_portal.uuid));
    }
    sync_deleted_annot("portal", deleted_portal.uuid);
}

void MainWidget::on_portal_edited(const std::string& uuid) {
    if (edit_portal_hook_function_name) {
        call_async_js_function_with_args(edit_portal_hook_function_name.value(),
            QJsonArray() << QString::fromStdString(uuid));
    }
    sync_edited_annot("portal", uuid);
}

void MainWidget::on_mark_added(const std::string& uuid, char type) {
    if (add_mark_hook_function_name) {

        QString type_string = QString(QChar(type));
        call_async_js_function_with_args(add_mark_hook_function_name.value(), QJsonArray() << QString::fromStdString(uuid) << type_string);
    }
}

void MainWidget::sync_newly_added_annot(const std::string& annot_type, const std::string& uuid) {
    if (is_current_document_available_on_server()) {
        //int highlight_index = doc()->get_highlight_index_with_uuid(uuid);
        const Annotation* annot = doc()->get_annot_with_uuid(annot_type, uuid);
        std::optional<std::string> checksum = doc()->get_checksum_fast();
        if ((annot != nullptr) && checksum) {
            sioyek_network_manager->upload_annot(this,
                QString::fromStdString(checksum.value()),
                *annot,
                [&, uuid, this, annot_type, document=doc()]() { // on success
                    if (document != doc()) return;
                    std::vector<std::string> uuids = { uuid };
                    doc()->set_annots_to_synced_with_type(annot_type, uuids);
                },
                [&, uuid, this]() { // on failure
                }
            );
        }
    }
}

void MainWidget::on_new_highlight_added(const std::string& uuid) {
    if (add_highlight_hook_function_name) {
        call_js_function_with_highlight_arg_with_uuid(add_highlight_hook_function_name.value(), uuid);
    }
    sync_newly_added_annot("highlight", uuid);

    doc()->update_last_local_edit_time();
}

void MainWidget::on_highlight_annotation_edited(const std::string& uuid) {
    if (highlight_annotation_changed_hook_function_name) {
        call_js_function_with_highlight_arg_with_uuid(highlight_annotation_changed_hook_function_name.value(), uuid);
    }
    sync_edited_annot("highlight", uuid);
}

void MainWidget::on_highlight_type_edited(const std::string& uuid) {
    if (highlight_type_changed_hook_function_name) {
        call_js_function_with_highlight_arg_with_uuid(highlight_type_changed_hook_function_name.value(), uuid);
    }
    sync_edited_annot("highlight", uuid);
}

void MainWidget::sync_edited_annot(const std::string& annot_type, const std::string& uuid) {
    if (is_current_document_available_on_server()) {
        const Annotation* annot = doc()->get_annot_with_uuid(annot_type, uuid);
        if (annot) {
            std::optional<std::string> doc_checksum = doc()->get_checksum_fast();
            if (doc_checksum.has_value()) {
                sioyek_network_manager->upload_annot(this,
                    QString::fromStdString(doc_checksum.value()),
                    *annot,
                    []() {},
                    []() {}
                );
            }
        }
    }
    doc()->update_last_local_edit_time();
}

void MainWidget::on_open_document(const std::wstring& path) {
    if (open_new_document_hook_function_name) {
        if (!checksummer->get_checksum_fast(path).has_value()) {
            call_async_js_function_with_args(open_new_document_hook_function_name.value(),
                QJsonArray() << QString::fromStdWString(path));
        }
    }
    if (open_document_hook_function_name) {
        call_async_js_function_with_args(open_document_hook_function_name.value(),
            QJsonArray() << QString::fromStdWString(path));
    }

    handle_sync_open_document();
}

void MainWidget::handle_sync_open_document() {
    if (sioyek_network_manager->ACCESS_TOKEN.size() > 0) {
        // check if the server's document location is different from the local location
        if (doc() && doc()->get_checksum_fast()) {
            sioyek_network_manager->get_opened_book_data_from_checksum(this, QString::fromStdString(doc()->get_checksum_fast().value()), [&, document=doc()](QJsonObject obj) {
                if (document != doc()) return;

                if (obj["status"] == "OK") {
                    float server_offset_y = obj["result"].toObject()["offset_y"].toDouble();
                    if (std::abs(server_offset_y - main_document_view->get_offset_y()) > SERVER_AND_LOCAL_DOCUMENT_MISMATCH_THRESHOLD) {
                        handle_server_document_location_mismatch(main_document_view->get_offset_y(), server_offset_y);
                    }
                }
                else if (obj["status"] == "DELETED") {
                    doc()->set_is_synced(false);
                }

                });
        }
    }
}

void MainWidget::handle_server_document_location_mismatch(float local_offset_y, float server_offset_y) {
    if (DOCUMENT_LOCATION_MISMATCH_STRATEGY == DocumentLocationMismatchStrategy::Local) return;
    if (DOCUMENT_LOCATION_MISMATCH_STRATEGY == DocumentLocationMismatchStrategy::Server) {
        long_jump_to_destination(server_offset_y);
    }
    if (DOCUMENT_LOCATION_MISMATCH_STRATEGY == DocumentLocationMismatchStrategy::Ask) {
        int res = show_option_buttons(L"Do you want to move to server location?", { L"Yes", L"No" });
        if (res == 0) {
            long_jump_to_destination(server_offset_y);
        }
    }
    if (DOCUMENT_LOCATION_MISMATCH_STRATEGY == DocumentLocationMismatchStrategy::ShowButton) {
        if (main_document_view) {
            if (doc() && doc()->get_checksum_fast()) {
                sioyek_network_manager->set_last_server_location(doc()->get_checksum_fast().value(), server_offset_y);
                resume_to_server_position_button->show();
            }
        }
    }

}


void VisibleObjectMoveData::handle_move(MainWidget* widget){
    if (is_moving) {
        if (index.object_type == VisibleObjectType::Bookmark) {
            widget->handle_bookmark_move();
        }
        else if ((index.object_type == VisibleObjectType::Portal) || (index.object_type == VisibleObjectType::PendingPortal) || (index.object_type == VisibleObjectType::PinnedPortal)) {
            widget->handle_portal_move();
        }
    }
}

void VisibleObjectMoveData::handle_move_end(MainWidget* widget){
    if (is_moving) {
        if (index.object_type == VisibleObjectType::Bookmark) {
            widget->handle_bookmark_move_finish();
        }
        else if ((index.object_type == VisibleObjectType::Portal) || (index.object_type == VisibleObjectType::PendingPortal)) {
            widget->handle_portal_move_finish();
        }
    }
}

void MainWidget::handle_login(std::wstring email, std::wstring password) {
    sioyek_network_manager->login(email, password);
}

void MainWidget::upload_current_file() {
    if (!doc()) return;

    doc()->set_is_synced(true);
    float offset_y = main_document_view->get_offset_y();

    sioyek_network_manager->upload_file(
        this,
        QString::fromStdWString(doc()->get_path()),
        QString::fromStdString(doc()->get_checksum()),
        [&, offset_y, document=doc()]() {
            sioyek_network_manager->sync_document_annotations_to_server(this, document, [this]() {invalidate_render(); });
            //sync_annotations_with_server();
            sync_document_location_to_servers(document, offset_y, false);
        }
    );
}


void MainWidget::update_current_document_checksum(std::string checksum) {
    //todo:
    qDebug() << "update_current_document_checksum not implemented";
    assert(false);
}
 
void MainWidget::auto_login() {
    if (AUTO_LOGIN_ON_STARTUP) {
        sioyek_network_manager->load_access_token();
    }
}

void MainWidget::on_server_hashes_loaded() {

}

#ifdef SIOYEK_IOS
void MainWidget::handle_ios_files(const QUrl& url){
    qDebug() << "handle_ios_files called with: " << url;
    std::wstring path = url.toLocalFile().toStdWString();
    push_state();
    open_document(path, &is_render_invalidated);
}

void MainWidget::on_ios_application_state_changed(Qt::ApplicationState state){

    if (state == Qt::ApplicationState::ApplicationActive){

        if (ios_was_suspended){
            ios_was_suspended = false;
            on_ios_resume();
        }
    }

    if (state == Qt::ApplicationState::ApplicationInactive){
    }

    if (state == Qt::ApplicationState::ApplicationSuspended){
        ios_was_suspended = true;
        if (is_reading){
            on_ios_suspend_while_reading();
        }
    }

    last_ios_application_state = state;
}

void MainWidget::on_ios_suspend_while_reading(){
    auto tts_handler = dynamic_cast<QtTextToSpeechHandler*>(tts);
    double tts_rate = tts_handler->tts->rate();
    QString tts_voice_name = tts_handler->tts->voice().name();

    handle_stop_reading();
    int page_number  = main_document_view->get_vertical_line_page();
    AbsoluteRect ruler_rect = main_document_view->get_ruler_rect().value_or(fz_empty_rect);
    std::wstring current_page_text;

    int index_into_page = doc()->get_page_text_and_line_rects_after_rect(
        page_number,
        ruler_rect,
        current_page_text,
        tts_corresponding_line_rects,
        tts_corresponding_char_rects);
    int index_into_document = index_into_page + doc()->get_page_offset_into_super_fast_index(page_number);


    QString rest_text = QString::fromStdWString(current_page_text) + get_rest_of_document_pages_text();
    ios_tts_begin_index_into_document = index_into_document;

    iosPlayTextToSpeechInBackground(
                rest_text.toNSString(),
                tts_voice_name.toNSString(),
                tts_rate);
}

void MainWidget::on_ios_resume(){
    iosStopReading();
    int last_spoken_word_location = getLastSpokenWordLocation();
    int last_spoken_word_index_into_document = last_spoken_word_location + ios_tts_begin_index_into_document;
    focus_on_character_offset_into_document(last_spoken_word_index_into_document);

}

#endif


QNetworkReply* MainWidget::download_paper_with_url(std::wstring paper_url, bool use_archive_url, PaperDownloadFinishedAction action) {
    QNetworkReply* reply = sioyek_network_manager->download_paper_with_url(paper_url, use_archive_url, action);
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        on_paper_downloaded(reply);
        });
    return reply;
}


void MainWidget::sync_current_file_location_to_servers(bool wait_for_send) {
    return sync_document_location_to_servers(doc(), main_document_view->get_offset_y(), wait_for_send);
}

void MainWidget::sync_document_location_to_servers(Document* document, float offset_y, bool wait_for_send) {
    QDateTime current_datetime = QDateTime::currentDateTime();

    if (document && document->get_checksum_fast()) {
        QNetworkReply* reply = sioyek_network_manager->sync_file_location(
            QString::fromStdString(document->get_checksum_fast().value()),
            QString::fromStdWString(document->detect_paper_name()),
            current_datetime.toString(Qt::DateFormat::ISODate),
            offset_y
        );

        if (wait_for_send) {
            block_for_send(reply);
        }
    }
}


void MainWidget::handle_open_server_only_file() {

    std::vector<std::string> local_checksums;
    db_manager->get_all_local_checksums(local_checksums);

    std::vector<std::vector<std::wstring>> values;
    std::vector<OpenedBookInfo> keys;

    values.push_back({});

    for (auto&  opened_file_info : sioyek_network_manager->get_excluded_opened_files(local_checksums)) {
        //values.push_back({});
        values.back().push_back(opened_file_info.file_name.toStdWString());
        keys.push_back(opened_file_info);
    }

    set_filtered_select_menu<OpenedBookInfo>(this, false, true, values, keys, -1, [this](OpenedBookInfo* val) {

        download_and_open(val->checksum, val->offset_y);
        },
        [](OpenedBookInfo* val) {

        });
    show_current_widget();
}

void MainWidget::download_and_open(std::string checksum, float offset_y) {

    sioyek_network_manager->download_file_with_hash(this, QString::fromStdString(checksum), [&, checksum, offset_y](QString file_path) {
        checksummer->set_checksum(checksum, file_path.toStdWString());

        std::optional<float> offset_x = {};
        push_state();
        open_document(file_path.toStdWString(), offset_x, offset_y, {}, checksum);
        //doc()->annotations_are_freshly_loaded = true;
        invalidate_render();
        });
}

void MainWidget::manage_last_document_checksum() {
    if (doc()) {
        if (doc()->checksum_is_new && sioyek_network_manager->server_hashes_loaded) {
            doc()->checksum_is_new = false;
            on_checksum_computed();
        }
        //if (last_document_checksum.doc != doc()) {
        //    last_document_checksum.checksum = doc()->get_checksum_fast();
        //    last_document_checksum.doc = doc();
        //}
        //else {
        //    if (!last_document_checksum.checksum.has_value()) {
        //        last_document_checksum.checksum = doc()->get_checksum_fast();
        //        if (last_document_checksum.checksum.has_value()) {
        //            on_checksum_computed();
        //        }
        //    }
        //}
    }

}

void MainWidget::on_checksum_computed() {
    //std::optional<std::string> checksum = doc()->get_checksum_fast();
    //if (checksum) {
    //    if (sioyek_network_manager->is_checksum_available_on_server(checksum.value())) {
    //        doc()->set_is_synced(true);
    //    }
    //    handle_sync_open_document();
    //    sync_annotations_with_server();
    //}
}


void MainWidget::handle_resume_to_server_location() {
    if (doc() && doc()->get_checksum_fast()) {
        auto res = sioyek_network_manager->last_server_location.find(doc()->get_checksum_fast().value());
        if (res != sioyek_network_manager->last_server_location.end()) {
            long_jump_to_destination(res->second);
            resume_to_server_position_button->hide();
            invalidate_render();
        }
    }
}

void MainWidget::handle_server_actions_button_pressed() {
    if (sioyek_network_manager->status == ServerStatus::LoggedIn) {
        if (!is_current_document_available_on_server()) {
            show_context_menu("logout|upload_current_file");
        }
        else {
            show_context_menu("logout");
        }
    }
    else if ((sioyek_network_manager->status == ServerStatus::NotLoggedIn) || (sioyek_network_manager->status == ServerStatus::InvalidCredentials)) {
        show_context_menu("login");
    }

}

void MainWidget::delete_current_file_from_server() {
    if (doc() && doc()->get_checksum_fast()) {
        doc()->set_is_synced(false);
        sioyek_network_manager->delete_file_with_checksum(QString::fromStdString(doc()->get_checksum_fast().value()));
    }
}

bool MainWidget::is_logged_in() {
    return sioyek_network_manager->status == ServerStatus::LoggedIn;
}

void MainWidget::on_overview_move_end() {
    main_document_view->handle_portal_overview_update();
}

void MainWidget::do_synchronize() {
    doc()->set_is_synced(true);
    handle_sync_open_document();
    sync_annotations_with_server();
}

void MainWidget::synchronize_if_desynchronized() {
    if (doc()) {
        if (is_current_document_available_on_server()) {
            if (!doc()->get_is_synced()) {
                do_synchronize();
            }
        }
    }
}

QMediaPlayer* MainWidget::get_media_player(){
    if (media_player == nullptr) {
        QAudioOutput* audio_output = new QAudioOutput(this);
        audio_output->setVolume(10);
        media_player = new QMediaPlayer(this);
        media_player->setAudioOutput(audio_output);
        QObject::connect(media_player, &QMediaPlayer::mediaStatusChanged, [this](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::MediaStatus::EndOfMedia) {
                //qDebug() << "end of media reached";
                high_quality_play_state = {};
                move_visual_mark(1);
                handle_start_reading_high_quality(true);
            }
            });
    }
    return media_player;
}

void MainWidget::handle_start_reading_high_quality(bool should_preload) {
    std::vector<PagelessDocumentRect> line_rects;
    std::vector<PagelessDocumentRect> char_rects;
    int current_page_number = get_current_page_number();
    int line_number = main_document_view->get_line_index();
    HighQualityPlayState play_state;
    play_state.doc = doc();
    play_state.page_number = current_page_number;
    play_state.start_line = line_number;
    high_quality_play_state = play_state;
    float rate = (TTS_RATE + 2) / 2; // todo: platform-specific conversion

    AbsoluteRect ruler_rect = main_document_view->get_ruler_rect().value_or(fz_empty_rect);
    std::wstring dummy_text;
    std::vector<PagelessDocumentRect> rect1, rect2;
    int index_into_page = doc()->get_page_text_and_line_rects_after_rect(
        current_page_number,
        INT_MAX,
        ruler_rect,
        dummy_text,
        rect1,
        rect2);

    std::wstring text;

    doc()->get_page_text_and_line_rects_after_rect(current_page_number, INT_MAX, fz_empty_rect, text, line_rects, char_rects);
    high_quality_play_state->line_rects = line_rects;
    //qDebug() << "page text size : " << text.size();
    index_into_page = text.size() - dummy_text.size();
    //doc()->get_page_text_and_line_rects_after_rect

    sioyek_network_manager->tts(this, text, doc()->get_checksum(), get_current_page_number(), rate, [&, index_into_page](QString file_path, std::vector<float> timestamps) {
        QMediaPlayer* mp = get_media_player();
        mp->setSource(QUrl::fromLocalFile(file_path));
        high_quality_play_state->timestamps = timestamps;
        //media_player->audioTracks().at(0).

        auto seek_to_location = [&, mp, index_into_page, timestamps](bool seekable) {
            if (seekable) {
                QTimer::singleShot(0, [&, mp, timestamps, index_into_page]() {
                    //media_player->setPosition(20 * 1000);
                    if (index_into_page < timestamps.size()) {
                        float time = timestamps[index_into_page];
                        media_player->setPosition(static_cast<int>(time * 1000));
                        media_player->play();
                        if (high_quality_play_state) {
                            high_quality_play_state->is_playing = true;
                        }
                    }
                    });
            }
            };
        if (mp->isSeekable()) {
            seek_to_location(true);
        }
        else {
            QObject::connect(mp, &QMediaPlayer::seekableChanged, seek_to_location);
        }

        });

    if (should_preload) {
        preload_next_page_for_tts(rate);

        //sioyek_network_manager->tts(this, )
    }
}

void MainWidget::preload_next_page_for_tts(float rate) {
    int next_page_number = get_current_page_number() + 1;
    if (next_page_number < doc()->num_pages()) {
        std::vector<PagelessDocumentRect> dummy_next_lines;
        std::vector<PagelessDocumentRect> dummy_next_chars;
        std::wstring next_page_text;
        doc()->get_page_text_and_line_rects_after_rect(next_page_number, INT_MAX, fz_empty_rect, next_page_text, dummy_next_lines, dummy_next_chars);
        sioyek_network_manager->tts(this, next_page_text, doc()->get_checksum(), next_page_number, rate, [](QString path, std::vector<float> timestamps) {});
    }
}

Q_INVOKABLE void MainWidget::update_checksum_impl(std::string old_checksum, std::string new_checksum) {
    this->sioyek_network_manager->update_checksum(this, QString::fromStdWString(doc()->get_path()), QString::fromStdString(old_checksum), QString::fromStdString(new_checksum), [this, document=doc(), old_checksum, new_checksum]() {
        this->sioyek_network_manager->update_user_files_hash_set();
        document->reload_annotations_on_new_checksum();

        this->background_task_manager->add_task([this, old_checksum, new_checksum]() {
            this->document_manager->update_checksum(old_checksum, new_checksum);
            }, this);
        });
}

void MainWidget::on_document_changed() {
    //return;
    if (AUTOMATICALLY_UPDATE_CHECKSUM_WHEN_DOCUMENT_IS_CHANGED) {
        background_task_manager->add_task([this]() {
            std::string old_checksum = doc()->get_checksum();
            std::string new_checksum = compute_checksum(QString::fromStdWString(doc()->get_path()), QCryptographicHash::Md5);
            this->document_manager->update_checksum(old_checksum, new_checksum);
            }, this);

            //QMetaObject::invokeMethod(this,
            //    "update_checksum_impl",
            //    Qt::QueuedConnection,
            //    Q_ARG(std::string, old_checksum),
            //    Q_ARG(std::string, new_checksum)
            //);
            //}, this);
    }
}

bool MainWidget::import_local_database(std::wstring path) {
    return db_manager->import_local(QString::fromStdWString(path));
}

bool MainWidget::import_shared_database(std::wstring path) {
    return db_manager->import_shared(QString::fromStdWString(path));
}

void MainWidget::on_paper_download_begin(QNetworkReply* reply, std::string pending_portal_handle) {

    QObject::connect(reply, &QNetworkReply::downloadProgress, [this, pending_portal_handle](qint64 received, qint64 total) {
        int pending_index = main_document_view->get_pending_portal_index_with_handle(pending_portal_handle);
        float ratio = static_cast<float>(received) / total;
        main_document_view->pending_download_portals[pending_index].downloaded_fraction = ratio;
        invalidate_render();
        });
}


QNetworkReply* MainWidget::download_paper_with_name(std::wstring name, std::optional<PaperDownloadFinishedAction> action, std::string pending_portal_handle) {
    QNetworkReply* reply = sioyek_network_manager->download_paper_with_name(this, name,
        action.value_or(get_default_paper_download_finish_action()),
        [this, pending_portal_handle](QNetworkReply* reply) {
            on_paper_download_begin(reply, pending_portal_handle);
        },
        [this](QNetworkReply* reply) {
            on_paper_downloaded(reply);
        });
    return reply;
}

void MainWidget::on_super_fast_search_index_computed() {
    if (AUTOMATICALLY_INDEX_DOCUMENT_FOR_FULLTEXT_SEARCH) {
        index_current_document_for_fulltext_search(true);
    }

    if (dv() && dv()->on_super_fast_compute_focus_offset){
        focus_on_character_offset_into_document(dv()->on_super_fast_compute_focus_offset.value());
        dv()->on_super_fast_compute_focus_offset = {};
    }

    if (doc()) {
        for (auto [checksum, text] : highlight_text_when_super_fast_index_is_ready) {
            if (doc()->get_checksum() == checksum) {
                main_document_view->perform_fuzzy_search(text.toStdWString());
            }
        }
    }

}


void MainWidget::handle_type_text_into_input(QString txt) {

    QWidget* focus_widget = focusWidget();

    if (focus_widget) {
        QKeySequence seq = QKeySequence::mnemonic(txt);
        QKeyEvent* e = new QKeyEvent(QEvent::KeyPress, seq[0], Qt::NoModifier, txt);
        // send event to focused widget
        QCoreApplication::sendEvent(focus_widget, e);
    }
}

void MainWidget::send_symbol_to_last_command(char symbol) {
    if (pending_command_instance) {
        pending_command_instance->set_symbol_requirement(symbol);
        advance_command(std::move(pending_command_instance));
    }
}

void MainWidget::ensure_titlebar_colors_match_color_mode(){
#ifdef Q_OS_MACOS
    if (main_document_view->get_current_color_mode() == ColorPalette::Normal){
        if (MACOS_TITLEBAR_COLOR[0] >= 0){
            changeTitlebarColor(winId(), MACOS_TITLEBAR_COLOR[0], MACOS_TITLEBAR_COLOR[1], MACOS_TITLEBAR_COLOR[2], 1.0f);
        }
    }
    else{
        if (MACOS_DARK_TITLEBAR_COLOR[0] >= 0){
            changeTitlebarColor(winId(), MACOS_DARK_TITLEBAR_COLOR[0], MACOS_DARK_TITLEBAR_COLOR[1], MACOS_DARK_TITLEBAR_COLOR[2], 1.0f);
        }
    }
#endif
}

MainWidget* MainWidget::get_widget_with_id(int window_id){
    return get_window_with_window_id(window_id);
}

bool MainWidget::is_current_document_fulltext_indexed() {
    return db_manager->is_document_indexed(doc()->get_checksum());
}

void MainWidget::handle_delete_document_from_fulltext_search_index() {
    std::vector<std::string> indexed_checksums = db_manager->get_all_fulltext_indexed_checksums();
    std::vector<std::wstring> corresponding_paths;
    for (int i = 0; i < indexed_checksums.size(); i++) {
        //qDebug() << QString::fromStdString(indexed_checksums[i]);
        corresponding_paths.push_back(
            document_manager->get_path_from_hash(indexed_checksums[i]).value_or(utf8_decode(indexed_checksums[i]))
        );
    }

    std::vector<std::vector<std::wstring>> columns = { corresponding_paths };

    set_filtered_select_menu<std::string>(this, true, false, columns, indexed_checksums, -1,
        [this](std::string* val) {
            std::wstring path = document_manager->get_path_from_hash(*val).value_or(L"");
            if (path.size() > 0) {
                int res = show_option_buttons(L"Are you sure you want to delete " + path + L" from the index?", { L"Yes", L"Cancel" });
                if (res == 0) {
                    db_manager->delete_checksum_from_fulltext_index(utf8_decode(*val));
                }
            }

        },
        [](std::string* val) {

        });
    show_current_widget();

}

void MainWidget::scroll_bookmark_with_uuid(const std::string& bookmark_uuid, int amount) {
    if (bookmark_uuid.size() > 0) {
        BookMark* bookmark = doc()->get_bookmark_with_uuid(bookmark_uuid);
        if (bookmark) {
            std::string uuid = bookmark->uuid;
            float scroll_amount = 72.0f * amount * VERTICAL_MOVE_AMOUNT;
            float current_scroll = dv()->get_bookmark_scroll_amount(uuid);
            float new_scroll = current_scroll + scroll_amount;
            float height = background_bookmark_renderer->get_cached_bookmark_height(uuid);

            if (new_scroll > (height - bookmark->get_rectangle()->height() * dv()->get_zoom_level())) {
                new_scroll = height - bookmark->get_rectangle()->height() * dv()->get_zoom_level();
            }
            dv()->set_bookmark_scroll_amount(uuid, new_scroll);
        }
    }
}

void MainWidget::scroll_selected_bookmark(int amount) {
    std::string bookmark_uuid = main_document_view->get_selected_bookmark_uuid();
    scroll_bookmark_with_uuid(bookmark_uuid, amount);
}

void MainWidget::pin_current_overview_as_portal() {
    std::optional<Portal> new_portal = main_document_view->pin_current_overview_as_portal();
    if (new_portal){
        add_portal(doc()->get_path(), new_portal.value());
    }
}

void MainWidget::set_mouse_cursor_for_side_resize(std::optional<OverviewSide> side){
    if (side) {
        if ((side.value() == OverviewSide::left) || (side.value() == OverviewSide::right)) {
            setCursor(Qt::SizeHorCursor);
            return;
        }
        if ((side.value() == OverviewSide::top) || (side.value() == OverviewSide::bottom)) {
            setCursor(Qt::SizeVerCursor);
            return;
        }
    }
    setCursor(Qt::ArrowCursor);
}

void MainWidget::stop_all_threads(){
    if ((*should_quit) == false){
        *should_quit = true;
        pdf_renderer->join_threads();
        background_task_manager->stop_worker_thread();
    }
}

void MainWidget::restart_all_threads(){
    if ((*should_quit) == true){
        *should_quit = false;
        pdf_renderer->start_threads();
        background_task_manager->start_worker_thread();
    }
}

void MainWidget::set_brightness(float brightness) {
#ifdef SIOYEK_ANDROID
    android_brightness_set(brightness);
#else
#endif
}

#ifdef SIOYEK_ANDROID


void MainWidget::on_android_pause(){
    // todo: we should probably stop all threads here
    validation_interval_timer->stop();
    network_timer->stop();
}

void MainWidget::on_android_resume(){
    validation_interval_timer->start();
    network_timer->start();

    // on mobile we use setWindowFlag(Qt::MaximizeUsingFullscreenGeometryHint, true)
    // to make the app cover the entire screen including the area above the camera notch
    // however, there seems to be a bug on android where this option stops working after
    // the app is minimized and then maximized again (even though the flag is still set)
    // and the weird thing is that when on_android_resume is called, the screen size is still
    // correct. So we just toggle the fullscreen twice to fix this issue, however, this might
    // cause problems when wants to manually resize the window. As a workaround we only do that
    // if the height difference between the screen and the window is less than 100 pixels

    int screen_height = screen()->size().height();
    int window_height = size().height();
    int height_diff = std::abs(screen_height - window_height);

    if (height_diff < 100){
        toggle_fullscreen();
        QTimer::singleShot(10, [&](){
            toggle_fullscreen();
        });
    }

}
#endif

void MainWidget::handle_highlight_text_in_document(std::string document_checksum, QString text_to_highlight) {
    std::optional<std::wstring> maybe_path = document_manager->get_path_from_hash(document_checksum);
    if (maybe_path.has_value()) {
        open_document(maybe_path.value());
        if (doc()->is_super_fast_index_ready()) {
            main_document_view->perform_fuzzy_search(text_to_highlight.toStdWString());
        }
        else {
            highlight_text_when_super_fast_index_is_ready.push_back(std::make_pair(document_checksum, text_to_highlight));
        }
    }
    else {
        windows[0]->download_checksum_when_ready = document_checksum;
    }
}

void MainWidget::show_citers_with_paper_name(std::wstring paper_name) {
    sioyek_network_manager->get_citers_with_name(this, paper_name, [this](QNetworkReply* reply) {
        auto content = reply->readAll();
        QJsonDocument json_doc = QJsonDocument::fromJson(content);
        QJsonObject root_object = json_doc.object();

        //std::vector<QString> citer_titles;
        //std::vector<QString> citer_citations;
        //std::vector<QString> citer_publication_year;
        std::vector<QString> citer_urls;
        std::vector<QString> citer_titles;
        std::vector<QString> citer_descriptions;

        for (auto obj : root_object["citers"].toArray()) {
            QJsonObject citer_props = obj.toObject();
            citer_titles.push_back(citer_props["title"].toString());
            citer_urls.push_back(citer_props["url"].toString());

            QString cite_count =QString::number(citer_props["cited_by_count"].toInt());
            QString publication_year = QString::number(citer_props["publication_year"].toInt());
            QString pulibcation_location = citer_props["publication_location"].toString();
            QString description = pulibcation_location + ", " + publication_year + ", " + cite_count + " citations";
            if (pulibcation_location.size() == 0) {
                description = publication_year + ", " + cite_count + " citations";
            }
            citer_descriptions.push_back(description);
        }

        auto selector_widget = ItemWithDescriptionSelectorWidget::from_items(std::move(citer_titles), std::move(citer_descriptions), std::move(citer_urls), this);
        selector_widget->set_select_fn([this, selector_widget](int index) {
            QString url = selector_widget->item_model->metadatas[index];
            auto reply = download_paper_with_url(url.toStdWString(), false, PaperDownloadFinishedAction::OpenInNewWindow);
            reply->setProperty("sioyek_downloading", true);
            reply->setProperty("sioyek_network_message", "starting download");
            QObject::connect(reply, &QNetworkReply::downloadProgress, [this, reply](qint64 received, qint64 total) {
                if (total > 0) {
                    float ratio = static_cast<float>(received) / total;
                    int percent = static_cast<int>(ratio * 100);
                    reply->setProperty("sioyek_network_message", "downloading ... " + QString::number(percent) + "% (cancel)");
                }
                else {
                    reply->setProperty("sioyek_network_message", "downloading ... " + QString::number(received) + "B (cancel)");

                }
                //main_document_view->pending_download_portals[pending_index].downloaded_fraction = ratio;
                invalidate_ui();
                });
            pop_current_widget();
            invalidate_ui();

            });
        set_current_widget(selector_widget);
        show_current_widget();

        });

}

void MainWidget::show_citers_of_current_paper() {
    std::wstring paper_name = doc()->detect_paper_name();
    show_citers_with_paper_name(paper_name);
}

void MainWidget::push_deleted_object(DeletedObject object) {
    deleted_objects.push_back(object);
}

void MainWidget::undo_delete() {
    if (deleted_objects.size() > 0) {
        DeletedObject& object_to_undo = deleted_objects.back();
        Document* target_doc = document_manager->get_document_with_checksum(object_to_undo.document_checksum);

        if (std::holds_alternative<Highlight>(object_to_undo.object)) {
            Highlight highlight_to_undo = std::get<Highlight>(object_to_undo.object);
            target_doc->add_highlight_with_existing_uuid(highlight_to_undo);
        }
        else if (std::holds_alternative<BookMark>(object_to_undo.object)) {
            BookMark bookmark_to_undo = std::get<BookMark>(object_to_undo.object);
            target_doc->add_bookmark_with_existing_uuid(bookmark_to_undo);
        }
        else if (std::holds_alternative<Portal>(object_to_undo.object)) {
            Portal portal_to_undo = std::get<Portal>(object_to_undo.object);
            target_doc->add_portal_with_existing_uuid(portal_to_undo);
        }

        deleted_objects.pop_back();
    }
}

void MainWidget::push_deleted_highlight(std::optional<Highlight> hl) {
    if (hl) {
        push_deleted_object(DeletedObject{ doc()->get_checksum(),  hl.value() });
    }
}

void MainWidget::push_deleted_bookmark(std::optional<BookMark> bm) {
    if (bm) {
        push_deleted_object(DeletedObject{ doc()->get_checksum(), bm.value() });
    }
}

void MainWidget::push_deleted_portal(std::optional<Portal> portal) {
    if (portal) {
        push_deleted_object(DeletedObject{ doc()->get_checksum(), portal.value() });
    }
}

void MainWidget::toggle_menu_collapse() {
    if (current_widget_stack.size() > 0) {

        auto tree_widget = dynamic_cast<FilteredTreeSelect<int>*>(current_widget_stack.back());
        if (tree_widget) {
            auto tree_view = dynamic_cast<QTreeView*>(tree_widget->get_view());
            if (tree_view) {
                bool is_exanded = false;

                for (int i = 0; i < tree_view->model()->rowCount(); i++) {
                    QModelIndex index = tree_view->model()->index(i, 0);
                    if (tree_view->isExpanded(index)) {
                        is_exanded = true;
                        break;
                    }
                }

                if (is_exanded) {
                    tree_view->collapseAll();
                }
                else {
                    tree_view->expandAll();
                }
            }
        }
    }
}

void MainWidget::goto_offset(float x_offset, float y_offset) {
    main_document_view->set_offsets(x_offset, y_offset, true);
}

Q_INVOKABLE QJsonObject MainWidget::absolute_to_window_rect_json(QJsonObject absolute_rect_json) {

    AbsoluteRect abs_rect;
    abs_rect.x0 = absolute_rect_json["x0"].toDouble();
    abs_rect.x1 = absolute_rect_json["x1"].toDouble();
    abs_rect.y0 = absolute_rect_json["y0"].toDouble();
    abs_rect.y1 = absolute_rect_json["y1"].toDouble();

    WindowRect wind_rect = abs_rect.to_window(main_document_view);
    QJsonObject wind_rect_json;
    wind_rect_json["x0"] = wind_rect.x0;
    wind_rect_json["x1"] = wind_rect.x1;
    wind_rect_json["y0"] = wind_rect.y0;
    wind_rect_json["y1"] = wind_rect.y1;
    return wind_rect_json;
}

void MainWidget::screenshot_js(QString file_path, QJsonObject window_rect_js) {
    WindowRect window_rect;
    window_rect.x0 = window_rect_js["x0"].toDouble();
    window_rect.x1 = window_rect_js["x1"].toDouble();
    window_rect.y0 = window_rect_js["y0"].toDouble();
    window_rect.y1 = window_rect_js["y1"].toDouble();
    QRect window_qrect = QRect(window_rect.x0, window_rect.y0, window_rect.x1 - window_rect.x0, std::abs(window_rect.y1 - window_rect.y0));
    

    if (window_qrect.width() > 0 && window_qrect.height() > 0) {

        QPixmap pixmap(size());
        render(&pixmap, QPoint(), QRegion(rect()));
        pixmap = pixmap.copy(window_qrect);

        pixmap.save(file_path);
    }

}

void MainWidget::framebuffer_screenshot_js(QString file_path, QJsonObject window_rect_js) {
#ifdef SIOYEK_OPENGL_BACKEND
    WindowRect window_rect;
    window_rect.x0 = window_rect_js["x0"].toDouble();
    window_rect.x1 = window_rect_js["x1"].toDouble();
    window_rect.y0 = window_rect_js["y0"].toDouble();
    window_rect.y1 = window_rect_js["y1"].toDouble();
    QRect window_qrect = QRect(window_rect.x0, window_rect.y0, window_rect.x1 - window_rect.x0, std::abs(window_rect.y1 - window_rect.y0));

    if (window_qrect.width() > 0 && window_qrect.height() > 0) {

        QImage image = opengl_widget->grabFramebuffer();
        QPixmap pixmap = QPixmap::fromImage(image);
        pixmap = pixmap.copy(window_qrect);

        pixmap.save(file_path);
    }
#else
    return screenshot_js(file_path, window_rect_js);
#endif
}

