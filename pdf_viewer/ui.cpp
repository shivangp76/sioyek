#include "ui.h"
#include <qfiledialog.h>

#include <QItemSelectionModel>
#include <QTapGesture>
#include <main_widget.h>
#include <qpainter.h>
#include <qabstracttextdocumentlayout.h>

#include <QQuickView>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQuickWidget>
#include <QFile>
#include <qscrollbar.h>

#include "database.h"
#include "document.h"
#include "document_view.h"
#include "touchui/TouchSlider.h"
#include "touchui/TouchConfigMenu.h"
#include "touchui/TouchSettings.h"

#include "commands/base_commands.h"

extern std::wstring MENU_MATCHED_SEARCH_HIGHLIGHT_STYLE;
extern std::wstring MENU_PRO_HIGHLIGHT_STYLE;
extern bool HIGHLIGHT_PRO_ONLY_COMMANDS;
extern std::wstring DEFAULT_OPEN_FILE_PATH;
extern float DARK_MODE_CONTRAST;
extern float BACKGROUND_COLOR[3];
extern float VISUAL_MARK_NEXT_PAGE_FRACTION;
extern float VISUAL_MARK_NEXT_PAGE_THRESHOLD;
extern float HIGHLIGHT_COLORS[26 * 3];
extern float TTS_RATE;
extern float MENU_SCREEN_WDITH_RATIO;
extern float MENU_SCREEN_HEIGHT_RATIO;
extern bool SHOW_MOST_RECENT_COMMANDS_FIRST;
extern std::wstring EXTERNAL_TEXT_EDITOR_COMMAND;
extern int FONT_SIZE;
extern bool TOUCH_MODE;
extern int DOCUMENTATION_FONT_SIZE;
extern float DEFAULT_TEXT_HIGHLIGHT_COLOR[3];

extern float CHAT_WINDOW_BACKGROUND_COLOR[3];
extern float CHAT_WINDOW_USER_MESSAGE_BACKGROUND_COLOR[3];
extern float CHAT_WINDOW_TEXT_COLOR[3];
extern float CHAT_WINDOW_USER_TEXT_COLOR[3];

extern float UI_TEXT_COLOR[3];
extern float UI_BACKGROUND_COLOR[3];
extern float UI_SELECTED_TEXT_COLOR[3];
extern float UI_SELECTED_BACKGROUND_COLOR[3];

std::wstring select_command_file_name(std::string command_name) {
    if (command_name == "open_document") {
        return select_document_file_name();
    }
    else if (command_name == "source_config") {
        return select_any_file_name();
    }
    else {
        return select_any_file_name();
    }
}

std::wstring select_command_folder_name() {
    QString dir_name = QFileDialog::getExistingDirectory(nullptr, "Select Folder");
    return dir_name.toStdWString();
}

std::wstring select_document_file_name() {
    if (DEFAULT_OPEN_FILE_PATH.size() == 0) {

        QString file_name = QFileDialog::getOpenFileName(nullptr, "Select Document", "", "Documents (*.pdf *.epub *.cbz)");
        return file_name.toStdWString();
    }
    else {

        QFileDialog fd = QFileDialog(nullptr, "Select Document", "", "Documents (*.pdf *.epub *.cbz)");
        fd.setDirectory(QString::fromStdWString(DEFAULT_OPEN_FILE_PATH));
        if (fd.exec()) {

            QString file_name = fd.selectedFiles().first();
            return file_name.toStdWString();
        }
        else {
            return L"";
        }
    }

}

std::wstring select_json_file_name() {
    QString file_name = QFileDialog::getSaveFileName(nullptr, "Select Document", "", "Documents (*.json )");
    return file_name.toStdWString();
}

std::wstring select_any_file_name() {
    QString file_name = QFileDialog::getSaveFileName(nullptr, "Select File", "", "Any (*)");
    return file_name.toStdWString();
}

std::wstring select_any_existing_file_name() {
    QString file_name = QFileDialog::getOpenFileName(nullptr, "Select File", "", "Any (*)");
    return file_name.toStdWString();
}

std::wstring select_new_json_file_name() {
    QString file_name = QFileDialog::getSaveFileName(nullptr, "Select Document", "", "Documents (*.json )");
    return file_name.toStdWString();
}

std::wstring select_new_pdf_file_name() {
    QString file_name = QFileDialog::getSaveFileName(nullptr, "Select Document", "", "Documents (*.pdf )");
    return file_name.toStdWString();
}


std::vector<ConfigFileChangeListener*> ConfigFileChangeListener::registered_listeners;

ConfigFileChangeListener::ConfigFileChangeListener() {
    registered_listeners.push_back(this);
}

ConfigFileChangeListener::~ConfigFileChangeListener() {
    registered_listeners.erase(std::find(registered_listeners.begin(), registered_listeners.end(), this));
}

void ConfigFileChangeListener::notify_config_file_changed(ConfigManager* new_config_manager) {
    for (auto* it : ConfigFileChangeListener::registered_listeners) {
        it->on_config_file_changed(new_config_manager);
    }
}

bool HierarchialSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    // custom behaviour :
#ifdef SIOYEK_QT6
    if (filterRegularExpression().pattern().size() == 0)
#else
    if (filterRegExp().isEmpty() == false)
#endif
    {
        // get source-model index for current row
        QModelIndex source_index = sourceModel()->index(source_row, this->filterKeyColumn(), source_parent);
        if (source_index.isValid())
        {
            // check current index itself :
            QString key = sourceModel()->data(source_index, filterRole()).toString();

#ifdef SIOYEK_QT6
            bool parent_contains = key.contains(filterRegularExpression());
#else
            bool parent_contains = key.contains(filterRegExp());
#endif

            if (parent_contains) return true;

            // if any of children matches the filter, then current index matches the filter as well
            int i, nb = sourceModel()->rowCount(source_index);
            for (i = 0; i < nb; ++i)
            {
                if (filterAcceptsRow(i, source_index))
                {
                    return true;
                }
            }
            return false;
        }
    }
    // parent call for initial behaviour
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

AndroidSelector::AndroidSelector(QWidget* parent) : QWidget(parent) {
    //     layout = new QVBoxLayout();

    main_widget = dynamic_cast<MainWidget*>(parent);
    int current_colorscheme_index = main_widget->get_current_colorscheme_index();
    bool horizontal_locked = main_widget->horizontal_scroll_locked;
    bool fullscreen = main_widget->isFullScreen();
    bool ruler = main_widget->main_document_view->is_ruler_mode();
    bool speaking = main_widget->is_reading;
    bool portaling = main_widget->main_document_view->is_pending_link_source_filled();
    bool fit_mode = main_widget->main_document_view->last_smart_fit_page.has_value();
    bool is_logged_in = main_widget->is_logged_in();
    bool is_current_document_synced = main_widget->is_current_document_available_on_server();
    #ifdef SIOYEK_ANDROID
    float current_brightness = android_brightness_get();
    #else
    float current_brightness = 0.5f;
    #endif

    main_menu = new TouchMainMenu(fit_mode,
        portaling,
        fullscreen,
        ruler,
        speaking,
        horizontal_locked,
        current_colorscheme_index,
        is_logged_in,
        is_current_document_synced,
        current_brightness,
        this);


    //    set_rect_config_button = new QPushButton("Rect Config", this);
    //    goto_page_button = new QPushButton("Goto Page", this);
    //    fullscreen_button = new QPushButton("Fullscreen", this);
    //    select_text_button = new QPushButton("Select Text", this);
    //    open_document_button = new QPushButton("Open New Document", this);
    //    open_prev_document_button = new QPushButton("Open Previous Document", this);
    //    command_button = new QPushButton("Command", this);
    //    visual_mode_button = new QPushButton("Visual Mark Mode", this);
    //    search_button = new QPushButton("Search", this);
    //    set_background_color = new QPushButton("Background Color", this);
    //    set_dark_mode_contrast = new QPushButton("Dark Mode Contrast", this);
    //    set_ruler_mode = new QPushButton("Ruler Mode", this);
    //    restore_default_config_button = new QPushButton("Restore Default Config", this);
    //    toggle_dark_mode_button = new QPushButton("Toggle Dark Mode", this);
    //    ruler_mode_bounds_config_button = new QPushButton("Configure Ruler Mode", this);
    //    test_rectangle_select_ui = new QPushButton("Rectangle Select", this);

    //    layout->addWidget(set_rect_config_button);
    //    layout->addWidget(goto_page_button);
    //    layout->addWidget(fullscreen_button);
    //    layout->addWidget(select_text_button);
    //    layout->addWidget(open_document_button);
    //    layout->addWidget(open_prev_document_button);
    //    layout->addWidget(command_button);
    //    layout->addWidget(visual_mode_button);
    //    layout->addWidget(search_button);
    //    layout->addWidget(set_background_color);
    //    layout->addWidget(set_dark_mode_contrast);
    //    layout->addWidget(set_ruler_mode);
    //    layout->addWidget(restore_default_config_button);
    //    layout->addWidget(toggle_dark_mode_button);
    //    layout->addWidget(ruler_mode_bounds_config_button);
    //    layout->addWidget(test_rectangle_select_ui);


    QObject::connect(main_menu, &TouchMainMenu::fullscreenClicked, [&]() {
        //main_widget->current_widget = {};
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        //deleteLater();
        main_widget->toggle_fullscreen();
        });

    QObject::connect(main_menu, &TouchMainMenu::selectTextClicked, [&]() {
        //main_widget->current_widget = {};
        //deleteLater();
        // assert(main_widget->current_widget_stack.back() == this);
        // main_widget->pop_current_widget();
        // main_widget->handle_mobile_selection();
        // main_widget->invalidate_render();
        main_widget->run_command_with_name("start_mobile_text_selection_at_point", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::openNewDocClicked, [&]() {
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        main_widget->run_command_with_name("open_document");
        });

    QObject::connect(main_menu, &TouchMainMenu::openPrevDocClicked, [&]() {
        main_widget->run_command_with_name("open_prev_doc", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::highlightsClicked, [&]() {
        main_widget->run_command_with_name("goto_highlight", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::tocClicked, [&]() {
        main_widget->run_command_with_name("goto_toc", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::bookmarksClicked, [&]() {
        main_widget->run_command_with_name("goto_bookmark", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::hintClicked, [&]() {
        //main_widget->run_command_with_name("toggle_rect_hints", true);

        // main_widget->toggle_scratchpad_mode();
        main_widget->handle_debug_command();
        main_widget->pop_current_widget();

        main_widget->invalidate_render();

        });

    QObject::connect(main_menu, &TouchMainMenu::commandsClicked, [&]() {
        main_widget->run_command_with_name("command", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::drawingModeClicked, [&]() {
        main_widget->run_command_with_name("toggle_freehand_drawing_mode", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::settingsClicked, [&]() {
        //TouchConfigMenu* config_menu = new TouchConfigMenu(main_widget);
        main_widget->show_touch_settings_menu();

        //TouchSettings* config_menu = new TouchSettings(main_widget);
        //assert(main_widget->current_widget_stack.back() == this);
        //main_widget->pop_current_widget();

        //main_widget->set_current_widget(config_menu);
        //main_widget->show_current_widget();

        });

    QObject::connect(main_menu, &TouchMainMenu::rulerModeClicked, [&, ruler]() {
        if (main_widget->is_ruler_mode()){
            main_widget->android_handle_visual_mode();
            //main_widget->current_widget = {};
            //deleteLater();
            assert(main_widget->current_widget_stack.back() == this);
            //main_widget->pop_current_widget();
            main_widget->invalidate_render();
        }
        else{
            main_widget->run_command_with_name("ruler_under_selected_point", true);
        }

        });

    QObject::connect(main_menu, &TouchMainMenu::searchClicked, [&]() {
        main_widget->run_command_with_name("search", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::brightnessChanged, [&](float brightness) {
        main_widget->set_brightness(brightness);
        //qDebug() << "brightness changed to: " << brightness;
        //main_widget->run_command_with_name("search", true);
        });


    QObject::connect(main_menu, &TouchMainMenu::addBookmarkClicked, [&]() {
        main_widget->run_command_with_name("add_bookmark", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::portalClicked, [&]() {
        main_widget->run_command_with_name("portal", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::deletePortalClicked, [&]() {
        main_widget->run_command_with_name("delete_portal", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::globalBookmarksClicked, [&]() {
        main_widget->run_command_with_name("goto_bookmark_g", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::globalHighlightsClicked, [&]() {
        main_widget->run_command_with_name("goto_highlight_g", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::ttsClicked, [&]() {
        main_widget->run_command_with_name("start_reading", true);
        });

    QObject::connect(main_menu, &TouchMainMenu::horizontalLockClicked, [&]() {
        main_widget->run_command_with_name("toggle_horizontal_scroll_lock");
        });
    QObject::connect(main_menu, &TouchMainMenu::loginClicked, [&]() {
        main_widget->pop_current_widget();
        main_widget->run_command_with_name("login");
        });

    QObject::connect(main_menu, &TouchMainMenu::logoutClicked, [&]() {
        main_widget->pop_current_widget();
        main_widget->run_command_with_name("logout");
        });

    QObject::connect(main_menu, &TouchMainMenu::syncClicked, [&]() {
        main_widget->pop_current_widget();
        main_widget->run_command_with_name("upload_current_file");
        });

    QObject::connect(main_menu, &TouchMainMenu::refreshClicked, [&]() {
        main_widget->pop_current_widget();
        main_widget->run_command_with_name("resync_document");
        });

    //    QObject::connect(set_background_color, &QPushButton::pressed, [&](){

    //        auto command = main_widget->command_manager->get_command_with_name("setconfig_background_color");
    //        main_widget->current_widget = {};
    //        deleteLater();
    //        main_widget->handle_command_types(std::move(command), 0);
    //    });

    //    QObject::connect(set_rect_config_button, &QPushButton::pressed, [&](){

    //        auto command = main_widget->command_manager->get_command_with_name("setconfig_portrait_back_ui_rect");
    //        main_widget->current_widget = {};
    //        deleteLater();
    //        main_widget->handle_command_types(std::move(command), 0);
    //    });

    //    QObject::connect(set_dark_mode_contrast, &QPushButton::pressed, [&](){

    //        main_widget->current_widget = {};
    //        deleteLater();

    //        main_widget->current_widget = new FloatConfigUI(main_widget, &DARK_MODE_CONTRAST, 0.0f, 1.0f);
    //        main_widget->current_widget->show();
    //    });

    //    QObject::connect(set_ruler_mode, &QPushButton::pressed, [&](){

    //        auto command = main_widget->command_manager->get_command_with_name("setconfig_ruler_mode");
    //        main_widget->current_widget = {};
    //        deleteLater();
    //        main_widget->handle_command_types(std::move(command), 0);

    //    });

    //    QObject::connect(restore_default_config_button, &QPushButton::pressed, [&](){

    //        main_widget->current_widget = {};
    //        deleteLater();
    //        main_widget->restore_default_config();

    //    });

    QObject::connect(main_menu, &TouchMainMenu::darkColorschemeClicked, [&]() {
        main_widget->set_dark_mode();
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        //main_widget->pop_current_widget();
        main_widget->invalidate_render();
        });

    QObject::connect(main_menu, &TouchMainMenu::lightColorschemeClicked, [&]() {
        main_widget->set_light_mode();
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        //main_widget->pop_current_widget();
        main_widget->invalidate_render();
        });

    QObject::connect(main_menu, &TouchMainMenu::customColorschemeClicked, [&]() {
        main_widget->set_custom_color_mode();
        //main_widget->current_widget = {};
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        //main_widget->pop_current_widget();
        main_widget->invalidate_render();
        });

    QObject::connect(main_menu, &TouchMainMenu::downloadPaperClicked, [&]() {
        main_widget->pop_current_widget();
        main_widget->download_paper_under_cursor(true);
        main_widget->invalidate_render();
        });


    QObject::connect(main_menu, &TouchMainMenu::fitToPageWidthClicked, [&]() {

        if (main_widget->main_document_view->last_smart_fit_page) {
            main_widget->main_document_view->last_smart_fit_page = {};
            //main_widget->pop_current_widget();
            main_widget->invalidate_render();
        }
        else {
            //main_widget->main_document_view->fit_to_page_width(true);
            main_widget->main_document_view->handle_fit_to_page_width(true);
            int current_page = main_widget->get_current_page_number();
            main_widget->main_document_view->last_smart_fit_page = current_page;

            //main_widget->pop_current_widget();
            main_widget->invalidate_render();
        }
        });
    //    QObject::connect(ruler_mode_bounds_config_button, &QPushButton::pressed, [&](){
    ////        auto command = main_widget->command_manager->get_command_with_name("toggle_dark_mode");
    //        main_widget->current_widget = nullptr;
    //        deleteLater();
    //        RangeConfigUI* config_ui = new RangeConfigUI(main_widget, &VISUAL_MARK_NEXT_PAGE_FRACTION, &VISUAL_MARK_NEXT_PAGE_THRESHOLD);
    //        main_widget->current_widget = config_ui;
    //        main_widget->current_widget->show();
    ////        main_widget->handle_command_types(std::move(command), 0);
    //    });

    QObject::connect(main_menu, &TouchMainMenu::gotoPageClicked, [&]() {
        //main_widget->current_widget = nullptr;
        //deleteLater();
        assert(main_widget->current_widget_stack.back() == this);
        main_widget->pop_current_widget();
        PageSelectorUI* page_ui = new PageSelectorUI(main_widget,
            main_widget->get_current_page_number(),
            main_widget->current_document_page_count());

        main_widget->set_current_widget(page_ui);
        main_widget->show_current_widget();
        //main_widget->current_widget = page_ui;
        //main_widget->current_widget->show();
        });

    //    QObject::connect(test_rectangle_select_ui, &QPushButton::pressed, [&](){
    ////        auto command = main_widget->command_manager->get_command_with_name("toggle_dark_mode");
    ////        main_widget->current_widget = {};
    //        RectangleConfigUI* configui = new RectangleConfigUI(main_widget, &testrect);
    //        main_widget->current_widget = configui;
    //        configui->show();

    //        deleteLater();
    ////        main_widget->handle_command_types(std::move(command), 0);
    //    });

    //     layout->insertStretch(-1, 1);

    //     this->setLayout(layout);
}

void AndroidSelector::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    //float parent_width_in_centimeters = static_cast<float>(parent_width) / logicalDpiX() * 2.54f;
    //float parent_height_in_centimeters = static_cast<float>(parent_height) / logicalDpiY() * 2.54f;
    int ten_cm = static_cast<int>(12 * logicalDpiX() / 2.54f);

    int w = static_cast<int>(parent_width * 0.9f);
    int h = parent_height;

    w = std::min(w, ten_cm);
    h = std::min(h, static_cast<int>(ten_cm * 1.3f));

    main_menu->resize(w, h);
    setFixedSize(w, h);

    //    list_view->setFixedSize(parent_width * 0.9f, parent_height);
    move((parent_width - w) / 2, (parent_height - h) / 2);

}

//TextSelectionButtons::TextSelectionButtons(MainWidget* parent) : QWidget(parent) {

//    QHBoxLayout* layout = new QHBoxLayout();

//    main_widget = parent;
//    copy_button = new QPushButton("Copy");
//    search_in_scholar_button = new QPushButton("Search Scholar");
//    search_in_google_button = new QPushButton("Search Google");
//    highlight_button = new QPushButton("Highlight");

//    QObject::connect(copy_button, &QPushButton::clicked, [&](){
//        copy_to_clipboard(main_widget->selected_text);
//    });

//    QObject::connect(search_in_scholar_button, &QPushButton::clicked, [&](){
//        search_custom_engine(main_widget->selected_text, L"https://scholar.google.com/scholar?&q=");
//    });

//    QObject::connect(search_in_google_button, &QPushButton::clicked, [&](){
//        search_custom_engine(main_widget->selected_text, L"https://www.google.com/search?q=");
//    });

//    QObject::connect(highlight_button, &QPushButton::clicked, [&](){
//        main_widget->handle_touch_highlight();
//    });

//    layout->addWidget(copy_button);
//    layout->addWidget(search_in_scholar_button);
//    layout->addWidget(search_in_google_button);
//    layout->addWidget(highlight_button);

//    this->setLayout(layout);
//}

//void TextSelectionButtons::resizeEvent(QResizeEvent* resize_event) {
//    QWidget::resizeEvent(resize_event);
//    int parent_width = parentWidget()->width();
//    int parent_height = parentWidget()->height();

//    setFixedSize(parent_width, parent_height / 5);
////    list_view->setFixedSize(parent_width * 0.9f, parent_height);
//    move(0, 0);

//}

TouchTextSelectionButtons::TouchTextSelectionButtons(MainWidget* parent) : QWidget(parent) {
    main_widget = parent;
    buttons_ui = new TouchCopyOptions(this);
    this->setAttribute(Qt::WA_NoMousePropagation);

    QObject::connect(buttons_ui, &TouchCopyOptions::copyClicked, [&]() {
        copy_to_clipboard(main_widget->main_document_view->get_selected_text());
        main_widget->clear_selection_indicators();
        });

    QObject::connect(buttons_ui, &TouchCopyOptions::searchClicked, [&]() {
        main_widget->perform_search(main_widget->main_document_view->get_selected_text(), false);
        main_widget->show_search_buttons();
        main_widget->clear_selection_indicators();
        });

    QObject::connect(buttons_ui, &TouchCopyOptions::scholarClicked, [&]() {
        search_custom_engine(main_widget->main_document_view->get_selected_text(), L"https://scholar.google.com/scholar?&q=");
        main_widget->clear_selection_indicators();
        });

    QObject::connect(buttons_ui, &TouchCopyOptions::googleClicked, [&]() {
        search_custom_engine(main_widget->main_document_view->get_selected_text(), L"https://www.google.com/search?q=");
        main_widget->clear_selection_indicators();
        });

    QObject::connect(buttons_ui, &TouchCopyOptions::highlightClicked, [&]() {
        main_widget->handle_touch_highlight();
        main_widget->clear_selection_indicators();
        });

    QObject::connect(buttons_ui, &TouchCopyOptions::downloadClicked, [&]() {
        main_widget->download_selected_text();
        main_widget->clear_selection_indicators();
        });

    QObject::connect(buttons_ui, &TouchCopyOptions::smartJumpClicked, [&]() {
        main_widget->smart_jump_to_selected_text();
        main_widget->clear_selection_indicators();
        });
}

DrawControlsUI::DrawControlsUI(MainWidget* parent) : QWidget(parent) {
    main_widget = parent;
    controls_ui = new TouchDrawControls(parent->main_document_view->freehand_thickness, parent->main_document_view->get_current_freehand_type(), this);
    this->setAttribute(Qt::WA_NoMousePropagation);

    QObject::connect(controls_ui, &TouchDrawControls::exitDrawModePressed, [&]() {
        main_widget->exit_freehand_drawing_mode();
        });

    QObject::connect(controls_ui, &TouchDrawControls::changeColorPressed, [&](int color_index) {
        main_widget->main_document_view->current_freehand_type = 'a' + color_index;
        });

    QObject::connect(controls_ui, &TouchDrawControls::enablePenDrawModePressed, [&]() {
        main_widget->set_pen_drawing_mode(true);
        });

    QObject::connect(controls_ui, &TouchDrawControls::screenshotPressed, [&]() {
        if (main_widget->is_scratchpad_mode()) {
            main_widget->run_command_with_name("copy_drawings_from_scratchpad");
        }
        else {
            main_widget->run_command_with_name("copy_screenshot_to_scratchpad");
        }
        });

    QObject::connect(controls_ui, &TouchDrawControls::saveScratchpadPressed, [&]() {
        main_widget->run_command_with_name("save_scratchpad");
        main_widget->invalidate_render();
        });

    QObject::connect(controls_ui, &TouchDrawControls::loadScratchpadPressed, [&]() {
        main_widget->run_command_with_name("load_scratchpad");
        main_widget->invalidate_render();
        });

    QObject::connect(controls_ui, &TouchDrawControls::movePressed, [&]() {
        main_widget->run_command_with_name("select_freehand_drawings");
        main_widget->invalidate_render();
        });

    QObject::connect(controls_ui, &TouchDrawControls::toggleScratchpadPressed, [&]() {
        main_widget->run_command_with_name("toggle_scratchpad_mode");
        main_widget->invalidate_render();
        });

    QObject::connect(controls_ui, &TouchDrawControls::disablePenDrawModePressed, [&]() {
        main_widget->set_hand_drawing_mode(true);
        });

    QObject::connect(controls_ui, &TouchDrawControls::eraserPressed, [&]() {
        main_widget->run_command_with_name("delete_freehand_drawings");
        });

    QObject::connect(controls_ui, &TouchDrawControls::penSizeChanged, [&](qreal val) {
        main_widget->set_freehand_thickness(val);
        });


}
void DrawControlsUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);

    int pwidth = parentWidget()->width();
    int width = parentWidget()->width() * 3 / 4;
    int height = std::max(parentWidget()->height() / 16, 50);

    controls_ui->move(0, 0);
    controls_ui->resize(width, height);
    move((pwidth - width) / 2, height);
    setFixedSize(width, height);

}

void TouchTextSelectionButtons::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);

    int pwidth = parentWidget()->width();
    int width = parentWidget()->width() * 3 / 4;
    int height = parentWidget()->height() / 16;

    buttons_ui->move(0, 0);
    buttons_ui->resize(width, height);
    move((pwidth - width) / 2, height);
    //    setFixedSize(0, 0);
    setFixedSize(width, height);

}

HighlightButtons::HighlightButtons(MainWidget* parent) : QWidget(parent) {
    main_widget = parent;
    //layout = new QHBoxLayout();

    //delete_highlight_button = new QPushButton("Delete");
    //buttons_widget = new 
    highlight_buttons = new TouchHighlightButtons(main_widget->get_current_selected_highlight_type(), this);

    QObject::connect(highlight_buttons, &TouchHighlightButtons::deletePressed, [&]() {
        main_widget->handle_delete_selected_highlight();
        hide();
        });

    QObject::connect(highlight_buttons, &TouchHighlightButtons::editPressed, [&]() {
        main_widget->run_command_with_name("edit_selected_highlight");
        //main_widget->handle_delete_selected_highlight();
        hide();
        });

    QObject::connect(highlight_buttons, &TouchHighlightButtons::changeColorPressed, [&](int index) {
        //float* color = &HIGHLIGHT_COLORS[3 * index];

        //main_widget->handle_delete_selected_highlight();
        if (index < 26) {
            main_widget->change_selected_highlight_type('a' + index);
        }
        else {
            main_widget->change_selected_highlight_type('A' + index - 26);
        }
        hide();
        main_widget->invalidate_render();
        //main_widget->highlight_buttons = nullptr;
        //deleteLater();
        });

    //layout->addWidget(delete_highlight_button);
    //this->setLayout(layout);
}

void HighlightButtons::resizeEvent(QResizeEvent* resize_event) {

    QWidget::resizeEvent(resize_event);

    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int dpi = physicalDpiY();
    float parent_height_in_centimeters = static_cast<float>(parent_height) / dpi * 2.54f;

    //int w = static_cast<int>(parent_width / 5);
    int w = parent_width;
    int h = static_cast<int>(static_cast<float>(dpi) / 2.54f);
    w = std::max(w, h * 6);

    setFixedSize(w, h);
    highlight_buttons->resize(w, h);
    move((parent_width - w) / 2, parent_height / 5);
}


SearchButtons::SearchButtons(MainWidget* parent) : QWidget(parent) {
    main_widget = parent;
    //layout = new QHBoxLayout(this);

//    delete_highlight_button = new QPushButton("Delete");
//    QObject::connect(delete_highlight_button, &QPushButton::clicked, [&](){
//        main_widget->handle_delete_selected_highlight();
//        hide();
//        main_widget->highlight_buttons = nullptr;
//        deleteLater();
//    });
    //next_match_button = new QPushButton("next");
    //prev_match_button = new QPushButton("prev");
    //goto_initial_location_button = new QPushButton("initial");
    buttons_widget = new TouchSearchButtons(this);

    QObject::connect(buttons_widget, &TouchSearchButtons::nextPressed, [&]() {
        //main_widget->opengl_widget->goto_search_result(1);
        main_widget->run_command_with_name("next_item");
        main_widget->validate_render();
        });

    QObject::connect(buttons_widget, &TouchSearchButtons::previousPressed, [&]() {
        main_widget->run_command_with_name("previous_item");
        //main_widget->opengl_widget->goto_search_result(-1);
        main_widget->validate_render();
        });

    QObject::connect(buttons_widget, &TouchSearchButtons::initialPressed, [&]() {
        main_widget->goto_mark('/');
        main_widget->invalidate_render();
        });


    //layout->addWidget(next_match_button);
    //layout->addWidget(goto_initial_location_button);
    //layout->addWidget(prev_match_button);

    //this->setLayout(layout);
}

void SearchButtons::resizeEvent(QResizeEvent* resize_event) {

    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int width = 2 * parentWidget()->width() / 3;
    int height = parentWidget()->height() / 12;


    buttons_widget->resize(width, height);
    setFixedSize(width, height);
    //layout->update();
    move((parent_width - width) / 2, parent_height - 3 * height / 2);
}

//ConfigUI::ConfigUI(MainWidget* parent) : QQuickWidget(parent){
ConfigUI::ConfigUI(std::string name, MainWidget* parent) : QWidget(parent) {
    main_widget = parent;
    config_name = name;
}

ConfigUI::~ConfigUI() {
    main_widget->on_config_changed(config_name, false);
}

void ConfigUI::on_change() {
    main_widget->on_config_changed(config_name);
}

void ConfigUI::set_should_persist(bool val) {
    this->should_persist = val;
}

void ConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    setFixedSize(2 * parent_width / 3, parent_height / 2);
    move(parent_width / 6, parent_height / 4);
}

Color3ConfigUI::Color3ConfigUI(std::string name, MainWidget* parent, float* config_location_) : ConfigUI(name, parent) {
    color_location = config_location_;
    QColor initial_color = convert_float3_to_qcolor(color_location);
    color_picker = new QColorDialog(initial_color, this);
    color_picker->show();

    connect(color_picker, &QColorDialog::finished, [&]() {
        main_widget->pop_current_widget();
        });

    connect(color_picker, &QDialog::rejected, [&, initial_color=initial_color]() {
        convert_qcolor_to_float3(initial_color, color_location);
        });

    connect(color_picker, &QColorDialog::colorSelected, [&](const QColor& color) {
        convert_qcolor_to_float3(color, color_location);
        main_widget->invalidate_render();
        on_change();

        if (should_persist) {
            main_widget->persist_config();
        }
        });

    connect(color_picker, &QColorDialog::currentColorChanged, [&](const QColor& color) {
        convert_qcolor_to_float3(color, color_location);
        on_change();
        main_widget->invalidate_render();
        });

}

void Color3ConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    setFixedSize(2 * parent_width / 3, parent_height / 2);
    color_picker->resize(width(), height());

    move(parent_width / 6, parent_height / 4);
}

Color4ConfigUI::Color4ConfigUI(std::string name, MainWidget* parent, float* config_location_) : ConfigUI(name, parent) {
    color_location = config_location_;
    QColor initial_color = convert_float4_to_qcolor(color_location);
    color_picker = new QColorDialog(initial_color, this);
    color_picker->setOption(QColorDialog::ShowAlphaChannel, true);

    color_picker->show();

    connect(color_picker, &QColorDialog::colorSelected, [&](const QColor& color) {
        convert_qcolor_to_float4(color, color_location);
        main_widget->invalidate_render();
        on_change();
        if (should_persist) {
            main_widget->persist_config();
        }
        });

    connect(color_picker, &QColorDialog::finished, [&]() {
        main_widget->pop_current_widget();
        });

    connect(color_picker, &QDialog::rejected, [&, initial_color=initial_color]() {
        convert_qcolor_to_float4(initial_color, color_location);
        });

    connect(color_picker, &QColorDialog::currentColorChanged, [&](const QColor& color) {
        convert_qcolor_to_float4(color, color_location);
        on_change();
        main_widget->invalidate_render();
        });
}

MacroConfigUI::MacroConfigUI(std::string name, MainWidget* parent, std::wstring* config_location, std::wstring initial_macro) : ConfigUI(name, parent) {

    macro_editor = new TouchMacroEditor(utf8_encode(initial_macro), this, parent);

    connect(macro_editor, &TouchMacroEditor::macroConfirmed, [&, config_location](std::string macro) {
        //convert_qcolor_to_float4(color, color_location);
        (*config_location) = utf8_decode(macro);
        on_change();
        if (should_persist) {
            main_widget->persist_config();
        }
        main_widget->pop_current_widget();
        });

    QObject::connect(macro_editor, &TouchMacroEditor::canceled, [&]() {
        main_widget->pop_current_widget();
        });


}

FloatConfigUI::FloatConfigUI(std::string name, MainWidget* parent, float* config_location, float min_value_, float max_value_) : ConfigUI(name, parent) {

    min_value = min_value_;
    max_value = max_value_;
    float_location = config_location;

    //int current_value = static_cast<int>((*config_location - min_value) / (max_value - min_value) * 100);
    float current_value = *config_location;

    slider = new TouchSlider(min_value, max_value, current_value, this);
    QObject::connect(slider, &TouchSlider::itemSelected, [&](float val) {
        //float value = min_value + (static_cast<float>(val) / 100.0f) * (max_value - min_value);
        *float_location = val;
        on_change();
        main_widget->invalidate_render();
        main_widget->pop_current_widget();
        });

    QObject::connect(slider, &TouchSlider::canceled, [&]() {
        main_widget->pop_current_widget();
        });


}


IntConfigUI::IntConfigUI(std::string name, MainWidget* parent, int* config_location, int min_value_, int max_value_) : ConfigUI(name, parent) {

    min_value = min_value_;
    max_value = max_value_;
    int_location = config_location;

    int current_value = static_cast<int>((*config_location - min_value) / (max_value - min_value) * 100);
    slider = new TouchSlider(min_value, max_value, current_value, this);
    QObject::connect(slider, &TouchSlider::itemSelected, [&](int val) {
        *int_location = val;
        on_change();
        main_widget->invalidate_render();
        main_widget->pop_current_widget();
        });

    QObject::connect(slider, &TouchSlider::canceled, [&]() {
        main_widget->pop_current_widget();
        });

}


PageSelectorUI::PageSelectorUI(MainWidget* parent, int current, int num_pages) : ConfigUI("", parent) {

    page_selector = new TouchPageSelector(0, num_pages - 1, current, this);

    QObject::connect(page_selector, &TouchPageSelector::pageSelected, [&](int val) {
        main_widget->goto_page_with_page_number(val);
        main_widget->invalidate_render();
        });

}

void PageSelectorUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int w = 2 * parent_width / 3;
    int h = parent_height / 6;
    page_selector->resize(w, h);

    setFixedSize(w, h);
    move((parent_width - w) / 2, parent_height - 2 * h);
}

AudioUI::AudioUI(MainWidget* parent) : ConfigUI("", parent) {

    buttons = new TouchAudioButtons(this);
    buttons->set_rate(TTS_RATE);

    QObject::connect(buttons, &TouchAudioButtons::playPressed, [&]() {
        //main_widget->main_document_view->goto_page(val);
        //main_widget->invalidate_render();
        main_widget->handle_play();
        });

    QObject::connect(buttons, &TouchAudioButtons::stopPressed, [&]() {
        //main_widget->main_document_view->goto_page(val);
        //main_widget->invalidate_render();
        main_widget->handle_stop_reading();
        });

    QObject::connect(buttons, &TouchAudioButtons::pausePressed, [&]() {
        //main_widget->main_document_view->goto_page(val);
        //main_widget->invalidate_render();
        main_widget->handle_pause();
        });


    QObject::connect(buttons, &TouchAudioButtons::rateChanged, [&](qreal rate) {
        TTS_RATE = rate;
        buttons->set_rate(TTS_RATE);
        main_widget->handle_pause();
        main_widget->handle_play();
        });
    //QObject::connect(buttons, &TouchAudioButtons::speedIncreasePressed, [&](){
    //    TTS_RATE += 0.1;
    //    if (TTS_RATE > 1.0f) {
    //        TTS_RATE = 1.0f;
    //    }
    //    buttons->set_rate(TTS_RATE);
    //});

    //QObject::connect(buttons, &TouchAudioButtons::speedDecreasePressed, [&](){
    //    TTS_RATE -= 0.1;
    //    if (TTS_RATE < -1.0f) {
    //        TTS_RATE = -1.0f;
    //    }
    //    buttons->set_rate(TTS_RATE);
    //});
}

void AudioUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int w = parent_width;
    int h = parent_height / 6;
    buttons->resize(w, h);

    setFixedSize(w, h);
    move((parent_width - w) / 2, parent_height - h);
}



BoolConfigUI::BoolConfigUI(std::string name_, MainWidget* parent, bool* config_location, QString qname) : ConfigUI(name_, parent) {
    bool_location = config_location;

    checkbox = new TouchCheckbox(qname, *config_location, this);
    QObject::connect(checkbox, &TouchCheckbox::itemSelected, [&](bool new_state) {
        *bool_location = static_cast<bool>(new_state);
        main_widget->invalidate_render();
        on_change();
        if (should_persist) {
            main_widget->persist_config();
        }

        main_widget->pop_current_widget();
        });

    QObject::connect(checkbox, &TouchCheckbox::canceled, [&]() {
        main_widget->pop_current_widget();
        });
}

void BoolConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int five_cm = static_cast<int>(12 * logicalDpiX() / 2.54f);

    int w = 2 * parent_width / 3;
    int h = parent_height / 2;

    w = std::min(w, five_cm);
    h = std::min(h, five_cm);

    checkbox->resize(w, h);

    setFixedSize(w, h);
    move((parent_width - w) / 2, (parent_height - h) / 2);
}

void MacroConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int w = 2 * parent_width / 3;
    int h = parent_height / 2;
    macro_editor->resize(w, h);

    setFixedSize(w, h);
    move(parent_width / 6, parent_height / 4);
}

void FloatConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int w = 2 * parent_width / 3;
    int h = parent_height / 2;
    slider->resize(w, h);

    setFixedSize(w, h);
    move(parent_width / 6, parent_height / 4);
}

void IntConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();

    int w = 2 * parent_width / 3;
    int h = parent_height / 2;
    slider->resize(w, h);

    setFixedSize(w, h);
    move(parent_width / 6, parent_height / 4);
}


TouchCommandSelector::TouchCommandSelector(bool is_fuzzy, const QStringList& commands, MainWidget* mw) : QWidget(mw) {
    main_widget = mw;
    QStringList touch_commands;
    for (auto com : commands) {
        if (!(com.startsWith("toggleconfig_") || com.startsWith("setconfig_") || com.startsWith("saveconfig_") || com.startsWith("deleteconfig_") || com.startsWith("setsaveconfig_"))) {
            touch_commands.push_back(com);
        }
    }
    list_view = new TouchListView(is_fuzzy, touch_commands, -1, this);

    QObject::connect(list_view, &TouchListView::itemSelected, [&](QString val, int index) {
        //main_widget->current_widget = nullptr;
        main_widget->pop_current_widget();
        main_widget->on_command_done(val.toStdString(), val.toStdString(), false);
        //deleteLater();
        });
}

void TouchCommandSelector::resizeEvent(QResizeEvent* resize_event) {

    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();
    int w = parent_width * 0.8f;
    int h = parent_height * 0.8f;

    list_view->resize(w, h);
    move(parent_width * 0.1f, parent_height * 0.1f);
    resize(w, h);
    QWidget::resizeEvent(resize_event);

    // QWidget::resizeEvent(resize_event);

    // int parent_width = parentWidget()->size().width();
    // int parent_height = parentWidget()->size().height();

    // resize(parent_width * 0.9f, parent_height);
    // move(parent_width * 0.05f, 0);
    // list_view->resize(parent_width * 0.9f, parent_height);
}


RectangleConfigUI::RectangleConfigUI(std::string name, MainWidget* parent, UIRect* config_location) : ConfigUI(name, parent) {

    rect_location = config_location;

    bool current_enabled = config_location->enabled;

    float current_left = config_location->left;
    float current_right = config_location->right;
    float current_top = config_location->top;
    float current_bottom = config_location->bottom;

    //    layout = new QVBoxLayout();

    rectangle_select_ui = new TouchRectangleSelectUI(current_enabled,
        current_left,
        current_top,
        (current_right - current_left) / 2.0f,
        (current_bottom - current_top) / 2.0f,
        this);


    //    int current_value = static_cast<int>((*config_location - min_value) / (max_value - min_value) * 100);
    //    slider = new TouchSlider(0, 100, current_value, this);
    QObject::connect(rectangle_select_ui, &TouchRectangleSelectUI::rectangleSelected, [&](bool enabled, qreal left, qreal right, qreal top, qreal bottom) {

        rect_location->enabled = enabled;
        rect_location->left = left;
        rect_location->right = right;
        rect_location->top = top;
        rect_location->bottom = bottom;

        on_change();
        if (should_persist) {
            main_widget->persist_config();
        }
        main_widget->invalidate_render();
        //main_widget->current_widget = nullptr;
        //deleteLater();
        main_widget->pop_current_widget();
        });

}

//bool RectangleConfigUI::eventFilter(QObject *obj, QEvent *event){
//    if ((obj == parentWidget()) && (event->type() == QEvent::Type::Resize)){

//        QResizeEvent* resize_event = static_cast<QResizeEvent*>(event);
//        int parent_width = resize_event->size().width();
//        int parent_height = resize_event->size().height();

//        bool res = QObject::eventFilter(obj, event);

//        qDebug() << "resizing to " << parent_width << " " << parent_height;
//        rectangle_select_ui->resize(parent_width, parent_height);
//        setFixedSize(parent_width, parent_height);

////        setFixedSize(parent_width, parent_height);
//        move(0, 0);
//        return res;
//    }
//    else{
//        return QObject::eventFilter(obj, event);
//    }
//}

void RectangleConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    move(0, 0);
    //    rectangle_select_ui->resize(resize_event->size().width(), resize_event->size().height());
    setFixedSize(parentWidget()->size());
    rectangle_select_ui->resize(parentWidget()->size());

}

RangeConfigUI::RangeConfigUI(std::string name, MainWidget* parent, float* top_config_location, float* bottom_config_location) : ConfigUI(name, parent) {

    //    range_location = config_location;
    top_location = top_config_location;
    bottom_location = bottom_config_location;

    float current_top = -(*top_location);
    float current_bottom = -(*bottom_location) + 1;


    range_select_ui = new TouchRangeSelectUI(current_top, current_bottom, this);

    // force a resize event in order to have correct sizes
    resize(parent->width(), parent->height());


    QObject::connect(range_select_ui, &TouchRangeSelectUI::rangeSelected, [&](qreal top, qreal bottom) {

        *top_location = -top;
        *bottom_location = -bottom + 1;

        on_change();
        if (should_persist) {
            main_widget->persist_config();
        }
        main_widget->invalidate_render();
        //main_widget->current_widget = nullptr;
        //deleteLater();
        main_widget->pop_current_widget();
        });

    QObject::connect(range_select_ui, &TouchRangeSelectUI::rangeCanceled, [&]() {
        main_widget->invalidate_render();
        //main_widget->current_widget = nullptr;
        //deleteLater();
        main_widget->pop_current_widget();
        });

}

void RangeConfigUI::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    move(0, 0);
    range_select_ui->resize(resize_event->size().width(), resize_event->size().height());

}


QString translate_command_search_string(QString actual_text) {

    if (actual_text.startsWith("+==")) {
        actual_text = "setsaveconfig_" + actual_text.right(actual_text.size() - 3);
    }

    if (actual_text.startsWith("+=")) {
        actual_text = "setsaveconfig_" + actual_text.right(actual_text.size() - 2);
    }


    if (actual_text.startsWith("!")) {
        actual_text = "toggleconfig_" + actual_text.right(actual_text.size() - 1);
    }

    if (actual_text.startsWith("==")) {
        actual_text = "setconfig_" + actual_text.right(actual_text.size() - 2);
    }

    if (actual_text.startsWith("=")) {
        actual_text = "setconfig_" + actual_text.right(actual_text.size() - 1);
    }


    if (actual_text.startsWith("+")) {
        actual_text = "saveconfig_" + actual_text.right(actual_text.size() - 1);
    }


    if (actual_text.startsWith("-")) {
        actual_text = "deleteconfig_" + actual_text.right(actual_text.size() - 1);
    }

    if (actual_text.endsWith("??")){
        // when query ends with ?? we try to open the definition file of the keybind/config
        actual_text = actual_text.left(actual_text.size()-2);
    }

    // when the query starts or ends with ? we open the documentation file for the command/config
    if (actual_text.endsWith("?")){
        actual_text = actual_text.left(actual_text.size()-1);
    }
    if (actual_text.startsWith("?")){
        actual_text = actual_text.right(actual_text.size()-1);
    }
    return actual_text;
}


BaseSelectorWidget::BaseSelectorWidget(QAbstractItemView* item_view, bool fuzzy, QAbstractItemModel* item_model, MainWidget* parent, MySortFilterProxyModel* custom_proxy_model) : QWidget(parent) {

    bool is_tree = dynamic_cast<QTreeView*>(item_view) != nullptr;
    is_fuzzy = fuzzy;

    if (custom_proxy_model == nullptr) {
        proxy_model = new MySortFilterProxyModel(fuzzy, is_tree);
    }
    else {
        proxy_model = custom_proxy_model;
    }
    proxy_model->setParent(this);

    proxy_model->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

    if (item_model) {
        proxy_model->setSourceModel(item_model);
    }

    resize(300, 800);
    QVBoxLayout* layout = new QVBoxLayout;
    setLayout(layout);

    line_edit = new MyLineEdit(parent);
    abstract_item_view = item_view;
    abstract_item_view->setParent(this);
    abstract_item_view->setModel(proxy_model);
    abstract_item_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    line_edit->setFont(get_ui_font_face_name());

    if (TOUCH_MODE) {
        QScroller::grabGesture(abstract_item_view->viewport(), QScroller::TouchGesture);
        abstract_item_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        QObject::connect(abstract_item_view, &QListView::pressed, [&](const QModelIndex& index) {
            pressed_row = index.row();
            pressed_pos = QCursor::pos();
            });

        QObject::connect(abstract_item_view, &QListView::clicked, [&](const QModelIndex& index) {
            QPoint current_pos = QCursor::pos();
            if (index.row() == pressed_row) {
                if ((current_pos - pressed_pos).manhattanLength() < 10) {
                    on_select(index);
                }
            }
            });
    }

    QTreeView* tree_view = dynamic_cast<QTreeView*>(abstract_item_view);

    if (tree_view) {
        int n_columns = item_model->columnCount();
        tree_view->expandAll();
        tree_view->setHeaderHidden(true);
        tree_view->resizeColumnToContents(0);
        tree_view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    }
    if (proxy_model) {
        proxy_model->setRecursiveFilteringEnabled(true);
    }

    layout->addWidget(line_edit);
    layout->addWidget(abstract_item_view);

    line_edit->installEventFilter(this);
    line_edit->setFocus();

    if (!TOUCH_MODE) {
        QObject::connect(abstract_item_view, &QAbstractItemView::activated, [&](const QModelIndex& index) {
            on_select(index);
            });
    }

    QObject::connect(line_edit, &QLineEdit::textChanged, [&](const QString& text) {
        on_text_changed(text);
        });

    if (TOUCH_MODE) {
        QScroller::grabGesture(abstract_item_view, QScroller::TouchGesture);
        abstract_item_view->setHorizontalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
        abstract_item_view->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
    }

    QString font_face = get_ui_font_face_name();
    if (font_face.size() > 0) {
        abstract_item_view->setFont(font_face);
        line_edit->setFont(font_face);
    }

}

void BaseSelectorWidget::on_text_changed(const QString& text) {
    if (!on_text_change(text)) {
        // generic text change handling when we don't explicitly handle text change events
        //proxy_model->setFilterFixedString(text);

        proxy_model->setFilterCustom(text);
        QTreeView* t_view = dynamic_cast<QTreeView*>(get_view());
        if (t_view) {
            t_view->expandAll();
        }
        else {
            get_view()->setCurrentIndex(get_view()->model()->index(0, 0));
        }
    }
}

QAbstractItemView* BaseSelectorWidget::get_view() {
    return abstract_item_view;
}
void BaseSelectorWidget::on_delete(const QModelIndex& source_index, const QModelIndex& selected_index) {}
void BaseSelectorWidget::on_edit(const QModelIndex& source_index, const QModelIndex& selected_index) {}

void BaseSelectorWidget::on_return_no_select(const QString& text) {
    bool is_numeric = false;
    text.toInt(&is_numeric);

    if (is_numeric){
        auto invalid_index = get_view()->model()->index(-1, 0);
        on_select(invalid_index);
    }
    else if (get_view()->model()->hasIndex(0, 0)) {
        on_select(get_view()->model()->index(0, 0));
    }
}

bool BaseSelectorWidget::on_text_change(const QString& text) {
    return false;
}

void BaseSelectorWidget::set_filter_column_index(int index) {
    proxy_model->setFilterKeyColumn(index);
}

std::optional<QModelIndex> BaseSelectorWidget::get_selected_index() {
    QModelIndexList selected_index_list = get_view()->selectionModel()->selectedIndexes();

    if (selected_index_list.size() > 0) {
        QModelIndex selected_index = selected_index_list.at(0);
        return selected_index;
    }
    return {};
}

std::wstring BaseSelectorWidget::get_selected_text() {
    return L"";
}

bool BaseSelectorWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == line_edit) {
#ifdef SIOYEK_QT6
        if (event->type() == QEvent::KeyRelease) {
            QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
            if (should_trigger_delete(key_event)) {
                handle_delete();
            }
            else if (key_event->key() == Qt::Key_Insert) {
                handle_edit();
            }
        }
#endif
        if (event->type() == QEvent::InputMethod) {
            if (TOUCH_MODE) {
                QInputMethodEvent* input_event = static_cast<QInputMethodEvent*>(event);
                QString text = input_event->preeditString();
                if (input_event->commitString().size() > 0) {
                    text = input_event->commitString();
                }
                if (text.size() > 0) {
                    on_text_changed(text);
                }
            }
        }
        if ((event->type() == QEvent::KeyPress)) {
            QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
            bool is_control_pressed = key_event->modifiers().testFlag(Qt::ControlModifier) || key_event->modifiers().testFlag(Qt::MetaModifier);
            bool is_alt_pressed = key_event->modifiers().testFlag(Qt::AltModifier);

            if (TOUCH_MODE) {
                if (key_event->key() == Qt::Key_Back) {
                    return false;
                }
            }
            // if (key_event->key() == Qt::Key_Left ||
                // key_event->key() == Qt::Key_Right
                // ) {
                // QKeyEvent* newEvent = key_event->clone();
                // QCoreApplication::postEvent(get_view(), newEvent);
                // return true;
            // }
            if (key_event->key() == Qt::Key_Tab) {
                QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Down, key_event->modifiers());
                QCoreApplication::postEvent(get_view(), new_key_event);
                return true;
            }
            if ((key_event->key() == Qt::Key_PageDown)) {
                QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_PageDown, key_event->modifiers());
                QCoreApplication::postEvent(get_view(), new_key_event);
                return true;
            }
            if ((key_event->key() == Qt::Key_PageUp)) {
                QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_PageUp, key_event->modifiers());
                QCoreApplication::postEvent(get_view(), new_key_event);
                return true;
            }
            if (key_event->key() == Qt::Key_Backtab) {
                QKeyEvent* new_key_event = new QKeyEvent(key_event->type(), Qt::Key_Up, key_event->modifiers());
                QCoreApplication::postEvent(get_view(), new_key_event);
                return true;
            }
            if (((key_event->key() == Qt::Key_C) && is_control_pressed)) {
                std::wstring text = get_selected_text();
                if (text.size() > 0) {
                    copy_to_clipboard(text);
                }
                return true;
            }
            if (key_event->key() == Qt::Key_Return || key_event->key() == Qt::Key_Enter) {
                std::optional<QModelIndex> selected_index = get_selected_index();
                if (selected_index) {
                    on_select(selected_index.value());
                }
                else {
                    on_return_no_select(line_edit->text());
                }
                return true;
            }

        }
    }
    return false;
}

void BaseSelectorWidget::simulate_end() {
    get_view()->scrollToBottom();
    int last_index = get_view()->model()->rowCount() - 1;
    get_view()->setCurrentIndex(get_view()->model()->index(last_index, 0));
}

void BaseSelectorWidget::simulate_home() {
    get_view()->scrollToTop();
    get_view()->setCurrentIndex(get_view()->model()->index(0, 0));

}
void BaseSelectorWidget::simulate_page_down() {
    QKeyEvent* new_key_event = new QKeyEvent(QEvent::Type::KeyPress, Qt::Key_PageDown, Qt::KeyboardModifier::NoModifier);
    QCoreApplication::postEvent(get_view(), new_key_event);
}

void BaseSelectorWidget::simulate_page_up() {
    QKeyEvent* new_key_event = new QKeyEvent(QEvent::Type::KeyPress, Qt::Key_PageUp, Qt::KeyboardModifier::NoModifier);
    QCoreApplication::postEvent(get_view(), new_key_event);
}


bool is_tree_view_index_last(const QModelIndex& index, QTreeView* view, bool include_chilren){
    bool index_has_children = view->model()->rowCount(index) > 0;
    if (include_chilren && index_has_children) return false;
    if (index.parent().isValid()){
        int parent_row_count = view->model()->rowCount(index.parent());
        if (index.row() == (parent_row_count - 1)){
            return is_tree_view_index_last(index.parent(), view, false);
        }
        else{
            return false;
        }
    }
    else{
        return index.row() == view->model()->rowCount() - 1;
    }
}

bool is_tree_view_index_first(const QModelIndex& index, QTreeView* view){

    if (index.parent().isValid()){
        return false;
    }
    return index.row() == 0;
    // if (include_parent && index.parent().isValid()) return false;

    // if (index.parent().isValid()){
        // if (index.row() == 0){
            // return is_tree_view_index_first(index.parent(), view, false);
        // }
        // else{
            // return false;
        // }
    // }
    // else{
        // return index.row() == 0;
    // }
}

void BaseSelectorWidget::simulate_move_down() {
    bool is_last = false;
    if (dynamic_cast<QTreeView*>(get_view())){
        QTreeView* tree_view = dynamic_cast<QTreeView*>(get_view());
        const QModelIndex& tree_index = tree_view->currentIndex();
        is_last = is_tree_view_index_last(tree_index, tree_view, true);
    }
    else{
        is_last = get_view()->currentIndex().row() == get_view()->model()->rowCount() - 1;
    }

    if (is_last){
        get_view()->setCurrentIndex(get_view()->model()->index(0, 0));
    }
    else{
        QKeyEvent* move_down_event = new QKeyEvent(QEvent::Type::KeyPress, Qt::Key_Down, Qt::KeyboardModifier::NoModifier);
        QCoreApplication::postEvent(get_view(), move_down_event);
    }
}

QModelIndex get_last_tree_index(QAbstractItemModel* model, QModelIndex parent_index){
    int row_count = model->rowCount(parent_index);
    if (row_count == 0){
        return parent_index;
    }
    else{
        QModelIndex last_child = model->index(row_count - 1, 0, parent_index);
        return get_last_tree_index(model, last_child);
    }
}

void BaseSelectorWidget::simulate_move_up() {
    bool is_first = false;
    if (dynamic_cast<QTreeView*>(get_view())){
        QTreeView* tree_view = dynamic_cast<QTreeView*>(get_view());
        const QModelIndex& tree_index = tree_view->currentIndex();
        is_first = is_tree_view_index_first(tree_index, tree_view);
    }
    else{
        is_first = get_view()->currentIndex().row() == 0;
    }

    if (is_first) {
        get_view()->setCurrentIndex(get_last_tree_index(get_view()->model(), QModelIndex()));
    }
    else{
        QKeyEvent* move_up_event = new QKeyEvent(QEvent::Type::KeyPress, Qt::Key_Up, Qt::KeyboardModifier::NoModifier);
        QCoreApplication::postEvent(get_view(), move_up_event);
    }
}

void BaseSelectorWidget::simulate_move_left() {
    QKeyEvent* move_left_event = new QKeyEvent(QEvent::Type::KeyPress, Qt::Key_Left, Qt::KeyboardModifier::NoModifier);
    QCoreApplication::postEvent(get_view(), move_left_event);
}

void BaseSelectorWidget::simulate_move_right() {
    QKeyEvent* move_right_event = new QKeyEvent(QEvent::Type::KeyPress, Qt::Key_Right, Qt::KeyboardModifier::NoModifier);
    QCoreApplication::postEvent(get_view(), move_right_event);
}

QString BaseSelectorWidget::get_selected_item() {
    if (get_selected_index()) {
        return get_view()->model()->data(get_selected_index().value()).toString();
    }
    return "";
}

void BaseSelectorWidget::simulate_select() {
    std::optional<QModelIndex> selected_index = get_selected_index();
    if (selected_index) {
        on_select(selected_index.value());
    }
    else {
        on_return_no_select(line_edit->text());
    }
}

void BaseSelectorWidget::handle_delete() {
    QModelIndexList selected_index_list = get_view()->selectionModel()->selectedIndexes();
    if (selected_index_list.size() > 0) {
        QModelIndex selected_index = selected_index_list.at(0);
        if (proxy_model->hasIndex(selected_index.row(), selected_index.column())) {
            QModelIndex source_index = proxy_model->mapToSource(selected_index);
            on_delete(source_index, selected_index);
        }
    }
}

void BaseSelectorWidget::handle_edit() {
    QModelIndexList selected_index_list = get_view()->selectionModel()->selectedIndexes();
    if (selected_index_list.size() > 0) {
        QModelIndex selected_index = selected_index_list.at(0);
        if (proxy_model->hasIndex(selected_index.row(), selected_index.column())) {
            QModelIndex source_index = proxy_model->mapToSource(selected_index);
            on_edit(source_index, selected_index);
        }
    }
}

#ifndef SIOYEK_QT6
    void BaseSelectorWidget::keyReleaseEvent(QKeyEvent* event) {
		if (should_trigger_delete(event)) {
            handle_delete();
        }
        QWidget::keyReleaseEvent(event);
    }
#endif

void BaseSelectorWidget::on_config_file_changed() {
    QString font_size_stylesheet = "";
    if (FONT_SIZE > 0) {
        font_size_stylesheet = QString("font-size: %1pt").arg(FONT_SIZE);
    }

    setStyleSheet(get_ui_stylesheet(true) + font_size_stylesheet);
    QAbstractItemView* view = get_view();
    QString style = get_view_stylesheet_type_name(view) + "::item::selected{" + get_selected_stylesheet() + "}" + get_scrollbar_stylesheet();
    view->setStyleSheet(style);
}

void BaseSelectorWidget::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    on_resize();
}

void BaseSelectorWidget::on_resize(){
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();
    setFixedSize(parent_width * MENU_SCREEN_WDITH_RATIO, parent_height * MENU_SCREEN_HEIGHT_RATIO);
    move(parent_width * (1 - MENU_SCREEN_WDITH_RATIO) / 2, parent_height * (1 - MENU_SCREEN_HEIGHT_RATIO) / 2);

    if (!is_initialized) {
        is_initialized = true;
        on_config_file_changed();
    }
}


MyLineEdit::MyLineEdit(MainWidget* parent) : QLineEdit(parent) {
    main_widget = parent;
}

int MyLineEdit::get_next_word_position() {
    int current_position = cursorPosition();

    int whitespace_position = text().indexOf(QRegularExpression("\\s"), current_position);
    if (whitespace_position == -1) {
        return text().size();
    }
    int next_word_position = text().indexOf(QRegularExpression("\\S"), whitespace_position);

    if (next_word_position == -1) {
        return text().size();
    }
    return next_word_position;
}

int MyLineEdit::get_prev_word_position() {
    int current_position = qMax(cursorPosition() -1, 0);

    int whitespace_position = text().lastIndexOf(QRegularExpression("\\s"), current_position);
    if (whitespace_position == -1) {
        return 0;
    }
    int prev_word_position = text().lastIndexOf(QRegularExpression("\\S"), whitespace_position);

    if (prev_word_position == -1) {
        return 0;
    }

    return qMin(prev_word_position + 1, text().size());
}

void MyLineEdit::keyPressEvent(QKeyEvent* event) {

    bool is_alt_pressed = event->modifiers() & Qt::AltModifier;
    bool is_control_pressed = is_platform_control_pressed(event);
    bool is_shift_pressed = event->modifiers() & Qt::ShiftModifier;
    bool is_meta_pressed = is_platform_meta_pressed(event);
    bool is_invisible = event->text().size() == 0;
    if (is_invisible || is_alt_pressed || is_control_pressed) {
        std::unique_ptr<Command> command = main_widget->input_handler->get_menu_command(main_widget, event, is_shift_pressed, is_control_pressed, is_meta_pressed, is_alt_pressed);

        if (command && command->is_menu_command()) {
            // this command will be handled later by our command manager so we ignore it here.
            event->ignore();
            return;
        }
    }

    return QLineEdit::keyPressEvent(event);

}


SelectHighlightTypeUI::SelectHighlightTypeUI(MainWidget* parent) :QWidget(parent), main_widget(parent) {

    QList<QColor> colors;
    const int N_COLORS = 26;
    for (int i = 0; i < N_COLORS; i++) {
        colors.push_back(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * i]));
    }

    new_widget = new QQuickWidget(this);

    new_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    new_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    new_widget->setClearColor(Qt::transparent);

    new_widget->rootContext()->setContextProperty("_colors", QVariant::fromValue(colors));
    new_widget->rootContext()->setContextProperty("_animate", QVariant::fromValue(false));
    new_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchSymbolColorSelector.qml"));

    new_widget->resize(width(), height() / 5);
    new_widget->move(0, height() / 2 - height() / 10);
    QObject::connect(new_widget->rootObject(), SIGNAL(colorClicked(int)), main_widget, SLOT(highlight_type_color_clicked(int)));
}

void SelectHighlightTypeUI::resizeEvent(QResizeEvent* resize_event) {
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();
    this->resize(parent_width, parent_height / 5);
    this->move(0, parent_height / 2 - parent_height / 10);
    new_widget->resize(size());
    new_widget->move(0, 0);
}

EnumConfigUI::EnumConfigUI(std::string name, MainWidget* parent, std::vector<std::wstring>& possible_values, int selected_index) : ConfigUI(name, parent) {

    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    QStringList items;
    for (auto& val : possible_values) {
        items.append(QString::fromStdWString(val));
    }

    QStringListModel* model = new QStringListModel(items);
    model->setParent(this);

    quick_widget->rootContext()->setContextProperty("_model", QVariant::fromValue(model));
    quick_widget->rootContext()->setContextProperty("_selected_index", QVariant::fromValue(selected_index));
    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchEnumSelector.qml"));

    QObject::connect(quick_widget->rootObject(), SIGNAL(itemSelected(QString, int)), this, SLOT(on_select(QString,int)));
}

void EnumConfigUI::on_select(QString name, int index) {
    main_widget->on_set_enum_config_value(config_name, name.toStdWString());
}

void EnumConfigUI::resizeEvent(QResizeEvent* resize_event) {
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();
    this->resize(parent_width * 3 / 4, parent_height * 2 / 3);
    this->move(parent_width / 8, parent_height / 6);
    quick_widget->resize(size());
    quick_widget->move(0, 0);
}



HighlightModel::HighlightModel(std::vector<Highlight>&& data, std::vector<QString>&& docs, std::vector<QString>&& doc_checksums, QObject* parent) : QAbstractTableModel(parent) {
    highlights = data;
    documents = docs;
    checksums = doc_checksums;
}

int HighlightModel::rowCount(const QModelIndex& parent) const {
    if (parent == QModelIndex()) {
        return highlights.size();
    }
    return 0;
}

int HighlightModel::columnCount(const QModelIndex& parent) const {
    if (documents.size() == 0) {
        return 3;
    }
    else if (checksums.size() == 0) {
        return 4;
    }
    else {
        return 5;
    }
}

QVariant HighlightModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        if (index.column() == HighlightModelColumn::text) {
            return QString::fromStdWString(highlights[index.row()].description);
        }
        if (index.column() == HighlightModelColumn::type) {
            return highlights[index.row()].type;
        }
        if (index.column() == HighlightModelColumn::description) {
            return QString::fromStdWString(highlights[index.row()].text_annot);
        }
        if (index.column() == HighlightModelColumn::file_name) {
            return documents[index.row()];
        }
        if (index.column() == HighlightModelColumn::checksum) {
            return checksums[index.row()];
        }
    }

    return QVariant();
}

QVariant HighlightModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return "Highlight";
    }
    return QVariant();
}

void HighlightSearchItemDelegate::set_pattern(QString p) {
    if (p.size() >= 2 && p[1] == ' ') {
        pattern = p.mid(2).toLower();
    }
    else {
        pattern = p.toLower();
    }
}

HighlightSearchItemDelegate::HighlightSearchItemDelegate(){
    QString font_name = get_ui_font_face_name();
    QFont highlight_font(font_name);
    QFont file_name_font;
    QFont comment_font(font_name);

    int font_size = FONT_SIZE > 0 ? FONT_SIZE : highlight_font.pointSize();

    if (font_size >= 0) {
        highlight_font.setPointSize(font_size);
        file_name_font.setPointSize(font_size * 3 / 4);
        comment_font.setPointSize(font_size * 7 / 8);
    }

    highlight_document.setDefaultFont(highlight_font);
    file_name_document.setDefaultFont(file_name_font);
    comment_document.setDefaultFont(comment_font);


    //std::vector<float> local_cached_heights;
    //for (int i = 0; i < highlight_model->rowCount(); i++) {
    //    local_cached_heights.push_back(compute_size_hint(500, highlight_model, i).height());
    //}
    //cached_heights = local_cached_heights;
    //std::thread([this, highlight_model]() {
    //    std::vector<float> local_cached_heights;
    //    for (int i = 0; i < highlight_model->rowCount(); i++) {
    //        local_cached_heights.push_back(compute_size_hint(500, highlight_model, i).height());
    //    }
    //    cached_heights = local_cached_heights;
    //    }).detach();

}

void HighlightSearchItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {

    int highlight_type = index.siblingAtColumn(1).data().toInt();


    float* hc = nullptr;

    QColor highlight_color;
    if (highlight_type >= 'a' && highlight_type <= 'z') {
        hc = &HIGHLIGHT_COLORS[(highlight_type - 'a') * 3];
        highlight_color = QColor::fromRgbF(hc[0], hc[1], hc[2], 1);
    }
    else if (highlight_type >= 'A' && highlight_type <= 'Z') {
        hc = &HIGHLIGHT_COLORS[(highlight_type - 'A') * 3];
        highlight_color = QColor::fromRgbF(hc[0], hc[1], hc[2], 1);
    }
    else {
        hc = &HIGHLIGHT_COLORS[0];
    }

    QColor text_color_hsl = highlight_color.toHsl();
    text_color_hsl.setHslF(text_color_hsl.hueF(), text_color_hsl.saturationF(), 0.9);
    //QColor text_color = text_color_hsl.toRgb();
    QColor text_color = QColor::fromRgbF(1, 1, 1, 1);

    QColor selected_text_color = QColor::fromRgbF(0, 0, 0, 1);
    //QColor background_color = QColor::fromRgbF(0, 0, 0, 1);
    QColor background_color = option.palette.color(QPalette::Base);
    QColor comment_text_color = QColor::fromRgbF(1, 1, 1, 0.7);
    QColor selected_comment_text_color = QColor::fromRgbF(0, 0, 0, 0.7);

    QColor selected_background_color_hsl = highlight_color.toHsl();
    selected_background_color_hsl.setHslF(selected_background_color_hsl.hueF(), selected_background_color_hsl.saturationF(), 0.9);
    QColor selected_background_color = selected_background_color_hsl.toRgb();

    //qDebug() << (char)highlight_type << " " << highlight_color.name();
    bool is_color_light = (hc[0] + hc[1] + hc[2]) > 1.5;
    QString label_text_color = is_color_light ? "black" : "white";
    QString highlight_text = index.siblingAtColumn(HighlightModel::text).data().toString();
    //QString text = get_index_text(index, label_text_color, highlight_color.name());
    QString comment_text = index.siblingAtColumn(HighlightModel::description).data().toString();



    const QSortFilterProxyModel* proxy_model = dynamic_cast<const QSortFilterProxyModel*>(index.model());

    int match_index = -1;
    if (proxy_model && (pattern.size() > 0)) {
        int text_highlight_begin = -1, text_highlight_end = -1;
        int comment_highlight_begin = -1, comment_highlight_end = -1;

        int text_similarity = similarity_score_cached(highlight_text.toLower(), pattern, &text_highlight_begin, &text_highlight_end);
        int comment_similarity = similarity_score_cached(comment_text.toLower(), pattern, &comment_highlight_begin, &comment_highlight_end);

        //auto [text_highlight_begin, text_highlight_end] = find_smallest_containing_substring(text.toLower().toStdWString(), pattern.toStdWString());
        //auto [comment_highlight_begin, comment_highlight_end] = find_smallest_containing_substring(comment_text.toLower().toStdWString(), pattern.toStdWString());

        //text = text.replace(pattern, "<span style=\"background-color: yellow; color: black;\">" + pattern + "</span>");
        //comment_text = comment_text.replace(pattern, "<span style=\"background-color: yellow; color: black;\">" + pattern + "</span>");
        const int similarity_threshold = 70;
        QString span_begin = "<span style=\"" + QString::fromStdWString(MENU_MATCHED_SEARCH_HIGHLIGHT_STYLE) + "\">";
        if (text_similarity > similarity_threshold) {
            highlight_text = highlight_text.left(text_highlight_begin) + span_begin + highlight_text.mid(text_highlight_begin, text_highlight_end - text_highlight_begin) + "</span>" + highlight_text.mid(text_highlight_end);
        }
        if (comment_similarity > similarity_threshold) {
            comment_text = comment_text.left(comment_highlight_begin) + span_begin + comment_text.mid(comment_highlight_begin, comment_highlight_end - comment_highlight_begin) + "</span>" + comment_text.mid(comment_highlight_end);
        }
    }

    QString highlight_html = get_display_text(highlight_text, highlight_type, label_text_color, highlight_color.name());

    painter->save();
    //float* highlight_type_color = &HIGHLIGHT_COLORS[highlight_type - 'a'];
    //QColor hl_color = QColor::fromRgbF(highlight_type_color[0], highlight_type_color[1], highlight_type_color[2]);
    //QColor hl_color_highlight = hl_color;
    //hl_color_highlight.setAlphaF(0.5);

    QAbstractTextDocumentLayout::PaintContext ctx;

    bool selected = option.state & QStyle::State_Selected;
    bool is_global = index.model()->columnCount() == HighlightModel::max_columns;
    //bool is_color_dark = (hc[0] + highlight_type_color[1] + highlight_type_color[2]) < 1.5;
    bool has_comment = index.siblingAtColumn(HighlightModel::description).data().toString().size() > 0;

    if (selected) {
        painter->fillRect(option.rect, selected_background_color);
        ctx.palette.setColor(QPalette::Text, selected_text_color);
    }
    else {
        painter->fillRect(option.rect, background_color);
        ctx.palette.setColor(QPalette::Text, text_color);
    }

    highlight_document.setHtml(highlight_html);
    highlight_document.setTextWidth(option.rect.width());
    painter->translate(option.rect.topLeft());
    painter->setClipRect(0, 0, option.rect.width(), option.rect.height());
    highlight_document.documentLayout()->draw(painter, ctx);
    float translate_amount = highlight_document.size().toSize().height();

    if (has_comment) {
        // draw vertical separator
        QColor separator_color;
        if (selected) {
            separator_color = QColor::fromRgbF(0, 0, 0, 0.2);
        }
        else {
            separator_color = QColor::fromRgbF(1, 1, 1, 0.2);
        }
        painter->setPen(separator_color);

        float offset = option.rect.width() / 10;
        painter->drawLine(offset, translate_amount, option.rect.width() - offset, translate_amount);



        //QColor current_text_color = ctx.palette.color(QPalette::Text);
        //current_text_color.setAlphaF(0.7);
        if (selected) {
            ctx.palette.setColor(QPalette::Text, selected_comment_text_color);
        }
        else {
            ctx.palette.setColor(QPalette::Text, comment_text_color);
        }
        comment_document.setTextWidth(option.rect.width());
        QSize highlight_size = highlight_document.size().toSize();
        painter->translate(0, translate_amount);

        comment_document.setHtml(comment_text);
        comment_document.documentLayout()->draw(painter, ctx);
        translate_amount = comment_document.size().toSize().height();

    }
    if (index.model()->columnCount() > 3) {
        file_name_document.setTextWidth(option.rect.width());

        //QSize comment_size =  comment_document.size().toSize();
        painter->translate(0, translate_amount);

        if (!selected) {
            ctx.palette.setColor(QPalette::Text, QColor::fromRgbF(1, 1, 1, 0.5));
        }
        else {
            ctx.palette.setColor(QPalette::Text, QColor::fromRgbF(0, 0, 0, 0.5));
        }

        file_name_document.setHtml("<div align=\"right\">[" + index.siblingAtColumn(HighlightModel::file_name).data().toString() + "]</div>");
        file_name_document.documentLayout()->draw(painter, ctx);
    }
    painter->restore();
}

QString HighlightSearchItemDelegate::get_display_text(const QString& highlight_text, int highlight_type, QString type_text_color, QString type_label_bg) const {
    char type_string[2] = { 0 };
    type_string[0] = highlight_type;
    std::string type_std_string = std::string(type_string);

    return "<code style=\"background-color: " + type_label_bg + "; color: " + type_text_color + ";\">&nbsp;" + QString::fromStdString(type_std_string) + "&nbsp;</code> " + highlight_text;
}

QSize HighlightSearchItemDelegate::sizeHint(const QStyleOptionViewItem& option,
    const QModelIndex& index) const {
    const QSortFilterProxyModel *proxy_model = dynamic_cast<const QSortFilterProxyModel*>(index.model());
    auto source_index = proxy_model->mapToSource(index);
    if (cached_sizes.find(source_index.row()) != cached_sizes.end()) {
        return QSize(option.rect.width(), cached_sizes[source_index.row()]);
    }

    bool is_global = index.model()->columnCount() == HighlightModel::max_columns;
    bool has_comment = index.siblingAtColumn(HighlightModel::description).data().toString().size() > 0;

    QString text = get_display_text(index.data().toString(), index.siblingAtColumn(HighlightModel::type).data().toInt());

    highlight_document.setHtml(text);
    highlight_document.setTextWidth(option.rect.width());
    comment_document.setTextWidth(option.rect.width());
    file_name_document.setTextWidth(option.rect.width());
    QSizeF res = highlight_document.size();


    if (index.model()->columnCount() > 3) {
        QString doc_text = index.siblingAtColumn(HighlightModel::file_name).data().toString();
        file_name_document.setPlainText(doc_text);
        QSizeF footer_size = file_name_document.size();
        res.setHeight(res.height() + footer_size.height());
    }

    if (has_comment) {
        QString comment_text = index.siblingAtColumn(HighlightModel::description).data().toString();
        comment_document.setHtml(comment_text);
        QSizeF comment_size = comment_document.size();
        res.setHeight(res.height() + comment_size.height());
    }
    cached_sizes[source_index.row()] = res.height();
    return res.toSize();

}

HighlightSelectorWidget::HighlightSelectorWidget(QAbstractItemView* view, QAbstractItemModel* model, MainWidget* parent) 
    : BaseCustomSelectorWidget(view, model, parent) {

    highlight_model = dynamic_cast<HighlightModel*>(model);

    if (lv) {
        lv->setItemDelegate(new HighlightSearchItemDelegate());
    }
    proxy_model->set_is_highlight(true);
    //    emit list_view->model()->dataChanged(list_view->model()->index(0, 0), list_view->model()->index(list_view->model()->rowCount() - 1, 0));

}

void BaseCustomSelectorWidget::update_render() {
    emit lv->model()->dataChanged(lv->model()->index(0, 0), lv->model()->index(lv->model()->rowCount() - 1, 0));
}


//bool HighlightSelectorWidget::on_text_change(const QString& text) {
//
//    auto highlight_item_delegate = dynamic_cast<HighlightSearchItemDelegate*>(lv->itemDelegate());
//    highlight_item_delegate->set_pattern(text);
//    return false;
//}


QString get_view_stylesheet_type_name(QAbstractItemView* view) {
    if (dynamic_cast<QTableView*>(view)) {
        return "QTableView";
    }
    if (dynamic_cast<QListView*>(view)) {
        return "QListView";
    }
    return "";
}


HighlightSelectorWidget* HighlightSelectorWidget::from_highlights(std::vector<Highlight>&& highlights, MainWidget* parent, std::vector<QString>&& doc_names, std::vector<QString>&& doc_checksums) {

    HighlightModel* highlight_model = new HighlightModel(std::move(highlights), std::move(doc_names), std::move(doc_checksums));

    QListView* list_view = get_ui_new_listview();

    HighlightSelectorWidget* highlight_selector_widget = new HighlightSelectorWidget(list_view, highlight_model, parent);

    highlight_model->setParent(highlight_selector_widget);
    list_view->setParent(highlight_selector_widget);

    //setFixedSize(parent_width * MENU_SCREEN_WDITH_RATIO, parent_height);
    //highlight_selector_widget->resize(parent->width() * MENU_SCREEN_WDITH_RATIO, parent->height());
    highlight_selector_widget->on_resize();
    highlight_selector_widget->set_filter_column_index(-1);

    highlight_selector_widget->update_render();
    return highlight_selector_widget;
}

bool HighlightModel::removeRows(int row, int count, const QModelIndex& parent) {
    beginRemoveRows(parent, row, row + count - 1);
    highlights.erase(highlights.begin() + row, highlights.begin() + row + count);

    if (documents.size() > 0) {
        documents.erase(documents.begin() + row, documents.begin() + row + count);
    }

    if (checksums.size() > 0) {
        checksums.erase(checksums.begin() + row, checksums.begin() + row + count);
    }

    endRemoveRows();
    return true;
}

bool ItemWithDescriptionModel::removeRows(int row, int count, const QModelIndex& parent) {
    beginRemoveRows(parent, row, row + count - 1);
    items.erase(items.begin() + row, items.begin() + row + count);

    if (descriptions.size() > 0) {
        descriptions.erase(descriptions.begin() + row, descriptions.begin() + row + count);
    }

    if (metadatas.size() > 0) {
        metadatas.erase(metadatas.begin() + row, metadatas.begin() + row + count);
    }

    endRemoveRows();
    return true;
}

void BaseCustomSelectorWidget::set_select_fn(std::function<void(int)>&& fn) {
    select_fn = fn;
}

void BaseCustomSelectorWidget::set_delete_fn(std::function<void(int)>&& fn) {
    delete_fn = fn;
}

void BaseCustomSelectorWidget::set_edit_fn(std::function<void(int)>&& fn) {
    edit_fn = fn;
}

BaseCustomSelectorWidget::BaseCustomSelectorWidget(
    QAbstractItemView* view,
    QAbstractItemModel* model,
    MainWidget* parent
) : BaseSelectorWidget(view, true, model, parent){

    lv = dynamic_cast<decltype(lv)>(get_view());


}

void BaseCustomSelectorWidget::resizeEvent(QResizeEvent* resize_event) {
    dynamic_cast<BaseCustomDelegate*>(lv->itemDelegate())->clear_cache();
    //clear_ca

    BaseSelectorWidget::resizeEvent(resize_event);
    //QWidget::resizeEvent(resize_event);
    update_render();
}

void BaseCustomSelectorWidget::on_select(QModelIndex value) {
    if (value.isValid()){
        QModelIndex source_index = dynamic_cast<const QSortFilterProxyModel*>(value.model())->mapToSource(value);
        int source_row = source_index.row();

        if (select_fn.has_value()) {

            select_fn.value()(source_row);
        }
    }
    else{
        select_fn.value()(-1);
    }
}

void BaseCustomSelectorWidget::on_delete(const QModelIndex& source_index, const QModelIndex& selected_index) {
    int source_row = source_index.row();

    if (delete_fn.has_value()) {
        delete_fn.value()(source_row);
    }

    bool result = proxy_model->removeRow(selected_index.row());
    update_render();
}

void BaseCustomSelectorWidget::on_edit(const QModelIndex& source_index, const QModelIndex& selected_index) {
    int source_row = source_index.row();

    if (edit_fn.has_value()) {
        edit_fn.value()(source_row);
    }

}

void BaseCustomSelectorWidget::set_selected_index(int index) {

    auto model = proxy_model->sourceModel();
    if (index != -1) {
        lv->selectionModel()->setCurrentIndex(model->index(index, 0), QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
        lv->scrollTo(this->proxy_model->mapFromSource(lv->currentIndex()), QAbstractItemView::EnsureVisible);
    }
    else{
        lv->selectionModel()->setCurrentIndex(model->index(0, 0), QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
    }
}

CommandModel::CommandModel(
    std::vector<QString> commands,
    std::vector<QStringList> keybinds,
    std::vector<bool> requires_pro)
    : commands(commands), keybinds(keybinds), requires_pro(requires_pro){

}

int CommandModel::rowCount(const QModelIndex& parent) const {
    if (parent == QModelIndex()) {
        return commands.size();
    }
    return 0;
}

int CommandModel::columnCount(const QModelIndex& parent) const {
    return CommandModel::max_columns;
}

QVariant CommandModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        if (index.column() == CommandModel::command_name) {
            return commands[index.row()];
        }
        else if (index.column() == CommandModel::keybind) {
            if (index.row() < keybinds.size()) {
                return keybinds[index.row()];
            }
        }
        else if (index.column() == CommandModel::is_pro) {
            if (index.row() < requires_pro.size()) {
                return requires_pro[index.row()];
            }
            return false;
        }
    }
    return QVariant();
}

QVariant CommandModel::headerData(int section, Qt::Orientation orientation, int role) const {

    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return "Command";
    }
    return QVariant();
}


CommandItemDelegate::CommandItemDelegate() {

    QFont command_font(get_ui_font_face_name());
    QFont keybind_font;

    int font_size = FONT_SIZE > 0 ? FONT_SIZE : command_font.pointSize();

    if (font_size >= 0) {
        command_font.setPointSize(font_size);
        keybind_font.setPointSize(font_size * 3 / 4);
    }

    command_name_document.setDefaultFont(command_font);
    keybind_document.setDefaultFont(keybind_font);
}

void CommandItemDelegate::set_ignore_prefix(QString p){
    ignore_prefix = p;
}

void CommandItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();

    QString command_name = index.data().toString();
    QStringList command_keybind = index.siblingAtColumn(CommandModel::keybind).data().toStringList();
    bool command_requires_pro = index.siblingAtColumn(CommandModel::is_pro).data().toBool();

    QAbstractTextDocumentLayout::PaintContext ctx;
    QColor text_color = QColor::fromRgbF(UI_TEXT_COLOR[0], UI_TEXT_COLOR[1], UI_TEXT_COLOR[2]);
    QColor background_color = QColor::fromRgbF(UI_BACKGROUND_COLOR[0], UI_BACKGROUND_COLOR[1], UI_BACKGROUND_COLOR[2]);

    QColor selected_text_color = QColor::fromRgbF(UI_SELECTED_TEXT_COLOR[0], UI_SELECTED_TEXT_COLOR[1], UI_SELECTED_TEXT_COLOR[2]);
    QColor selected_background_color = QColor::fromRgbF(UI_SELECTED_BACKGROUND_COLOR[0], UI_SELECTED_BACKGROUND_COLOR[1], UI_SELECTED_BACKGROUND_COLOR[2]);

    int text_highlight_begin = 0;
    int text_highlight_end = 0;
    //qDebug() << command_name << " " << text_similarity;

    const int similarity_threshold = 70;
    QString highlight_span = "<span style=\"" + QString::fromStdWString(MENU_MATCHED_SEARCH_HIGHLIGHT_STYLE) + "\">";
    if (ignore_prefix.size() == 0){
        int text_similarity = similarity_score_cached(command_name.toLower(), pattern, &text_highlight_begin, &text_highlight_end, 0.8f);
        if (text_similarity > similarity_threshold) {
            command_name = command_name.left(text_highlight_begin) + highlight_span + command_name.mid(text_highlight_begin, text_highlight_end - text_highlight_begin) + "</span>" + command_name.mid(text_highlight_end);
        }
    }
    else{
        int text_similarity = similarity_score_cached(command_name.mid(ignore_prefix.size()).toLower(), pattern.mid(ignore_prefix.size()), &text_highlight_begin, &text_highlight_end, 0.8f);

        if (text_highlight_begin >= 0) text_highlight_begin += ignore_prefix.size();
        if (text_highlight_end >= 0) text_highlight_end += ignore_prefix.size();

        if (text_similarity > similarity_threshold) {
            command_name = highlight_span + ignore_prefix + "</span>" + command_name.mid(ignore_prefix.size(), text_highlight_begin - ignore_prefix.size()) + highlight_span + command_name.mid(text_highlight_begin, text_highlight_end - text_highlight_begin) + "</span>" + command_name.mid(text_highlight_end);
        }
    }

    if (HIGHLIGHT_PRO_ONLY_COMMANDS && command_requires_pro) {
        command_name ="<code><span style=\"" + QString::fromStdWString(MENU_PRO_HIGHLIGHT_STYLE) + "\">&nbsp;+&nbsp;</span></code> " + command_name;
    }

    if (option.state & QStyle::State_Selected) {
        ctx.palette.setColor(QPalette::Text, selected_text_color);
        painter->fillRect(option.rect, selected_background_color);
    }
    else {
        ctx.palette.setColor(QPalette::Text, text_color);
        painter->fillRect(option.rect, background_color);
    }

    command_name_document.setHtml(command_name);
    //command_name_document.setPlainText(command_name);
    //command_name_document.setTextWidth(option.rect.width());
    painter->translate(option.rect.topLeft());
    painter->setClipRect(0, 0, option.rect.width(), option.rect.height());
    command_name_document.documentLayout()->draw(painter, ctx);

    QString keybind_html = "";
    QString keybind_tag_color;
    if (option.state & QStyle::State_Selected) {
        keybind_tag_color = selected_background_color.darker(120).name();
    }
    else {
        keybind_tag_color = background_color.lighter().name();
        if (background_color == Qt::black) {
            keybind_tag_color = "#222";
        }
    }
    //QString keybind_tag_color = option.state & QStyle::State_Selected ? "#ddd" : "#222";

    for (int i = 0; i < command_keybind.size(); i++) {
        keybind_html += "<kbd style=\"background-color: " + keybind_tag_color + ";\">&nbsp;" + command_keybind.at(i) + "&nbsp;</kbd>";
        if (i < command_keybind.size() - 1) {
            keybind_html += "&nbsp;";
        }
    }
    keybind_document.setHtml(keybind_html);

    //qDebug() << command_name_document.get
    //if (command_keybind.size() > 0) {
    //    //keybind_document.setHtml("<code style=\"background: gray; color: black;\">&nbsp;" + command_keybind + "&nbsp;</code>");
    //    keybind_document.setHtml("<span style=\"background: gray; color: black; border-style: solid; border-color: red;\">&nbsp;" + command_keybind.at(0) + "&nbsp;</span>");
    //}
    //else {
    //    keybind_document.setHtml("");
    //}
    keybind_document.setTextWidth(option.rect.width());
    //painter->translate(command_name_document.size().width(), 0);
    painter->translate((option.rect.width() - keybind_document.idealWidth()), (option.rect.height() - keybind_document.size().height()) / 2);
    keybind_document.documentLayout()->draw(painter, ctx);

    painter->restore();
}

QSize CommandItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (cached_size.has_value()) {
        return QSize(option.rect.width(), cached_size.value());
    }

    // text height
    //QFont some_font;
    //cached_size = QFontMetrics(some_font).height();
    command_name_document.setHtml("test");
    cached_size = command_name_document.size().height();
    //command_name_document.documentLayout()->documentSize
    return QSize(option.rect.width(), cached_size.value());
}

int BaseCustomDelegate::similarity_score_cached(QString haystack, QString needle, int* match_begin, int* match_end, float ratio) const{
    std::tuple<QString, QString, float> key = std::make_tuple(haystack, needle, ratio);
    auto cached_result = cached_similarity_scores.find(key);
    if (cached_result != cached_similarity_scores.end()) {
        *match_begin = cached_result->second.highlight_begin;
        *match_end = cached_result->second.highlight_end;
        return cached_result->second.score;
    }

    int score = similarity_score(haystack.toStdWString(), needle.toStdWString(), match_begin, match_end, ratio);
    SimilarityScoreResult result;
    result.score = score;
    result.highlight_begin = *match_begin;
    result.highlight_end = *match_end;
    cached_similarity_scores[key] = result;

    return score;

}

void BaseCustomDelegate::set_pattern(QString p) {
    pattern = p.toLower();
}

QString BaseCustomDelegate::highlight_pattern(QString txt) const{
    int text_highlight_begin = 0;
    int text_highlight_end = 0;
    int text_similarity = similarity_score_cached(txt.toLower(), pattern, &text_highlight_begin, &text_highlight_end, 0.8f);
    const int similarity_threshold = 70;
    if (text_similarity > similarity_threshold) {
        txt = txt.left(text_highlight_begin) + "<span style=\"" + QString::fromStdWString(MENU_MATCHED_SEARCH_HIGHLIGHT_STYLE) + "\">" + txt.mid(text_highlight_begin, text_highlight_end - text_highlight_begin) + "</span>" + txt.mid(text_highlight_end);
    }
    return txt;
}

CommandSelectorWidget* CommandSelectorWidget::from_commands(
    std::vector<QString> commands,
    std::vector<QString> configs,
    std::vector<QString> color_configs,
    std::vector<QString> bool_configs,
    std::vector<QStringList> keybinds,
    std::vector<bool> requires_pro,
    MainWidget* parent
) {


    std::unordered_map<QString, QStringList> command_to_keybind_map;
    std::unordered_map<QString, bool> command_to_requires_pro_map;
    std::unordered_map<QString, std::vector<QString>> prefix_command_names;
    std::unordered_map<QString, std::vector<QStringList>> prefix_keybinds;
    std::unordered_map<QString, std::vector<bool>> prefix_requires_pro;
    std::unordered_map<QString, QAbstractItemModel*> prefix_models;

    CommandModel* configs_model = new CommandModel(configs, {}, {});
    CommandModel* bool_configs_model = new CommandModel(bool_configs, {}, {});
    CommandModel* color_configs_model = new CommandModel(color_configs, {}, {});


    for (int i = 0; i < commands.size(); i++) {
        command_to_keybind_map[commands[i]] = keybinds[i];
        command_to_requires_pro_map[commands[i]] = requires_pro[i];
    }

    auto& required_prefixes = parent->command_manager->command_required_prefixes;
    for (auto command_name : commands) {
        QString required_prefix = "";

        if (required_prefixes.find(command_name) != required_prefixes.end()) {
            required_prefix = required_prefixes[command_name];
        }
        else if (command_name.startsWith("_")) {
            required_prefix = "_";
        }

        if (prefix_command_names.find(required_prefix) == prefix_command_names.end()) {
            prefix_command_names[required_prefix] = {};
            prefix_keybinds[required_prefix] = {};
            prefix_requires_pro[required_prefix] = {};
        }

        prefix_command_names[required_prefix].push_back(command_name);
        prefix_keybinds[required_prefix].push_back(QStringList());
        prefix_requires_pro[required_prefix].push_back({});

        if (command_to_keybind_map.find(command_name) != command_to_keybind_map.end()) {
            prefix_keybinds[required_prefix].back() = command_to_keybind_map[command_name];
            prefix_requires_pro[required_prefix].back() = command_to_requires_pro_map[command_name];

        }

    }

    for (auto [prefix, commands] : prefix_command_names) {
        prefix_models[prefix] = new CommandModel(prefix_command_names[prefix], prefix_keybinds[prefix], prefix_requires_pro[prefix]);
    }

    QListView* list_view = get_ui_new_listview();
    list_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    CommandSelectorWidget* command_selector_widget = new CommandSelectorWidget(
        list_view,
        prefix_models,
        configs_model,
        color_configs_model,
        bool_configs_model,
        parent
    );

    for (auto [_, model] : prefix_models) {
        model->setParent(command_selector_widget);
    }
    configs_model->setParent(command_selector_widget);
    bool_configs_model->setParent(command_selector_widget);
    color_configs_model->setParent(command_selector_widget);

    list_view->setParent(command_selector_widget);

    command_selector_widget->on_resize();
    command_selector_widget->set_filter_column_index(-1);

    command_selector_widget->update_render();
    return command_selector_widget;
}


bool BaseCustomSelectorWidget::on_text_change(const QString& text) {
    auto command_item_delegate = dynamic_cast<BaseCustomDelegate*>(lv->itemDelegate());
    command_item_delegate->set_pattern(text);
    return false;
}

//HighlightSelectorWidget::HighlightSelectorWidget(QAbstractItemView* view, QAbstractItemModel* model, MainWidget* parent) 
//    : BaseCustomSelectorWidget(view, model, parent) {
//
//    highlight_model = dynamic_cast<HighlightModel*>(model);
//
//    if (lv) {
//        lv->setItemDelegate(new HighlightSearchItemDelegate(model));
//    }
//    proxy_model->set_is_highlight(true);
//    //    emit list_view->model()->dataChanged(list_view->model()->index(0, 0), list_view->model()->index(list_view->model()->rowCount() - 1, 0));
//
//}

//CommandSelectorWidget::CommandSelectorWidget(
//    QAbstractItemView* view,
//    QAbstractItemModel* model,
//    MainWidget* parent
//) : BaseCustomSelectorWidget(view, model, parent){
//    command_model = dynamic_cast<CommandModel*>(model);
//
//    if (lv) {
//        lv->setItemDelegate(new CommandItemDelegate(model));
//    }
//
//}

CommandSelectorWidget::CommandSelectorWidget(
    QAbstractItemView* view,
    std::unordered_map<QString, QAbstractItemModel*> prefix_model,
    QAbstractItemModel* configs_model_,
    QAbstractItemModel* color_configs_model_,
    QAbstractItemModel* bool_configs_model_,
    MainWidget* parent) : BaseCustomSelectorWidget(view, prefix_model[""], parent) {
    w = parent;

    prefix_command_model = prefix_model;
    configs_model = configs_model_;
    color_configs_model = color_configs_model_;
    bool_configs_model = bool_configs_model_;

    for (auto [prefix, _] : prefix_model) {
        special_prefixes.push_back(prefix);
    }
    special_prefixes.push_back("setconfig_");
    special_prefixes.push_back("toggleconfig_");
    special_prefixes.push_back("saveconfig_");
    special_prefixes.push_back("setsaveconfig_");

    if (lv) {
        lv->setItemDelegate(new CommandItemDelegate());
        lv->setCurrentIndex(lv->model()->index(0, 0));
    }

}

//BaseCustomDelegate::BaseCustomDelegate(QAbstractItemModel* model){
//
//}

void HighlightSearchItemDelegate::clear_cache() {
    cached_sizes.clear();
}

void CommandItemDelegate::clear_cache() {
    cached_size = {};
}

bool CommandSelectorWidget::on_text_change(const QString& txt) {
    QString text = translate_command_search_string(txt);

    QString prefix = "";
    QStringList config_required_prefixes = { "setconfig_", "toggleconfig_", "saveconfig_", "setsaveconfig_", "deleteconfig_" };

    for (auto p : special_prefixes) {
        if (text.startsWith(p)) {
            if (p.size() > prefix.size()) {
                prefix = p;
            }
        }
    }
    bool is_config = false;
    bool is_boolean_config = false;
    bool is_color_config = false;
    config_prefix = "";

    if (config_required_prefixes.indexOf(prefix) != -1) {
        is_config = true;
        text = text.mid(prefix.size());
    }
    is_config_mode_ = is_config;
    if (prefix == "toggleconfig_") {
        is_boolean_config = true;
    }
    if (text.startsWith("DARK_") || text.startsWith("CUSTOM_")) {
        is_color_config = true;
        if (text.startsWith("DARK_")) {
            text = text.mid(5);
            prefix += "DARK_";
            config_prefix = "DARK_";
        }
        else {
            text = text.mid(7);
            prefix += "CUSTOM_";
            config_prefix = "CUSTOM_";
        }
    }

    if (prefix != last_prefix) {

        delete proxy_model;
        proxy_model = new MySortFilterProxyModel(true, false);
        if (is_config) {
            if (is_boolean_config) {
                proxy_model->setSourceModel(bool_configs_model);
            }
            else if (is_color_config) {
                proxy_model->setSourceModel(color_configs_model);
            }
            else {
                proxy_model->setSourceModel(configs_model);
            }
        }
        else {
            proxy_model->setSourceModel(prefix_command_model[prefix]);
        }
        proxy_model->setParent(this);
        proxy_model->set_ignore_prefix(prefix);
        get_view()->setModel(proxy_model);
        auto delegate = dynamic_cast<CommandItemDelegate*>(get_view()->itemDelegate());
        if (!is_config) {

            delegate->set_ignore_prefix(prefix);
        }
        // auto command_item_delegate = dynamic_cast<BaseCustomDelegate*>(lv->itemDelegate());

        //proxy_model->setSourceModel(prefix_command_model[prefix]);
        //proxy_model->update_scores();

        last_prefix = prefix;
    }

    return BaseCustomSelectorWidget::on_text_change(text);
}

bool CommandSelectorWidget::is_config_mode() {
    return is_config_mode_;
}

QString CommandSelectorWidget::get_config_prefix() {
    return config_prefix;
}

void BaseSelectorWidget::paintEvent(QPaintEvent* paint_event){

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    // QColor ui_background_color = convert_float3_to_qcolor();
    QColor ui_background_color = QColor::fromRgbF(UI_BACKGROUND_COLOR[0], UI_BACKGROUND_COLOR[1], UI_BACKGROUND_COLOR[2]);
    painter.setBrush(QBrush(ui_background_color));
    painter.setPen(Qt::transparent);
    
    QRect rect = this->rect();
    rect.setWidth(rect.width()-1);
    rect.setHeight(rect.height()-1);
    if (MENU_SCREEN_HEIGHT_RATIO == 1.0f){
        // no need for rounded corners
        painter.drawRect(rect);
    }
    else{
        painter.drawRoundedRect (rect, 4, 4);
    }

    QWidget::paintEvent(paint_event);
}

void BaseSelectorWidget::wheelEvent(QWheelEvent* wheel_event) {
    QWidget::wheelEvent(wheel_event);
    wheel_event->accept();
}

void CommandSelectorWidget::on_text_changed(const QString& text) {
    if (!on_text_change(text)) {
        QString pattern = dynamic_cast<CommandItemDelegate*>(lv->itemDelegate())->pattern;
        proxy_model->setFilterCustom(pattern);
        get_view()->setCurrentIndex(get_view()->model()->index(0, 0));
    }
}

QString CommandSelectorWidget::get_command_with_index(int index) {
    return proxy_model->sourceModel()->data(proxy_model->sourceModel()->index(index, CommandModel::command_name)).toString();
}

BookmarkModel::BookmarkModel(std::vector<BookMark>&& data, std::vector<QString>&& docs, std::vector<QString>&& doc_checksums, QObject* parent) : QAbstractTableModel(parent) {
    bookmarks = data;
    documents = docs;
    checksums = doc_checksums;
}

int BookmarkModel::rowCount(const QModelIndex& parent) const {
    if (parent == QModelIndex()) {
        return bookmarks.size();
    }
    return 0;
}

int BookmarkModel::columnCount(const QModelIndex& parent) const {
    if (documents.size() == 0) {
        return 2;
    }
    else if (checksums.size() == 0) {
        return 3;
    }
    else {
        return 4;
    }
}

QVariant BookmarkModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        if (index.column() == BookmarkModelColumn::description) {
            return QString::fromStdWString(bookmarks[index.row()].description);
        }
        if (index.column() == BookmarkModelColumn::bookmark) {
            return QVariant::fromValue(bookmarks[index.row()]);
        }
        if (index.column() == BookmarkModelColumn::file_name) {
            return documents[index.row()];
        }
        if (index.column() == BookmarkModelColumn::checksum) {
            return checksums[index.row()];
        }
    }
    return QVariant();
}

QVariant BookmarkModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return "Bookmark";
    }
    return QVariant();
}

bool BookmarkModel::removeRows(int row, int count, const QModelIndex& parent) {
    beginRemoveRows(parent, row, row + count - 1);
    bookmarks.erase(bookmarks.begin() + row, bookmarks.begin() + row + count);

    if (documents.size() > 0) {
        documents.erase(documents.begin() + row, documents.begin() + row + count);
    }

    if (checksums.size() > 0) {
        checksums.erase(checksums.begin() + row, checksums.begin() + row + count);
    }

    endRemoveRows();
    return true;
}

BookmarkSearchItemDelegate::BookmarkSearchItemDelegate(){
    QFont bookmark_font(get_ui_font_face_name());
    QFont file_name_font;

    int font_size = FONT_SIZE > 0 ? FONT_SIZE : bookmark_font.pointSize();

    if (font_size >= 0) {
        bookmark_font.setPointSize(font_size);
        file_name_font.setPointSize(font_size * 3 / 4);
    }

    bookmark_document.setDefaultFont(bookmark_font);
    file_name_document.setDefaultFont(file_name_font);
}

QSize BookmarkSearchItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    const QSortFilterProxyModel *proxy_model = dynamic_cast<const QSortFilterProxyModel*>(index.model());
    auto source_index = proxy_model->mapToSource(index);
    if (cached_sizes.find(source_index.row()) != cached_sizes.end()) {
        return QSize(option.rect.width(), cached_sizes[source_index.row()]);
    }

    BookMark bookmark = index.siblingAtColumn(BookmarkModel::bookmark).data().value<BookMark>();

    QString bookmark_text = QString::fromStdWString(bookmark.description);
    bool is_markdown = BookMark::should_be_displayed_as_markdown(bookmark_text);
    bookmark_text = BookMark::get_display_markdown_or_text(bookmark_text);

    bookmark_document.setTextWidth(option.rect.width());
    if (is_markdown) {
        bookmark_document.setMarkdown(bookmark_text);
    }
    else {
        bookmark_document.setHtml(bookmark_text);
    }

    QSize res = bookmark_document.size().toSize();
    int col_count = index.model()->columnCount();
    bool is_global = index.model()->columnCount() == BookmarkModel::max_columns;

    file_name_document.setTextWidth(option.rect.width());
    file_name_document.setHtml("<div align=\"right\">" + index.siblingAtColumn(BookmarkModel::file_name).data().toString() + "</div>");
    res = QSize(res.width(), res.height() + file_name_document.size().toSize().height());

    cached_sizes[source_index.row()] = res.height();
    return res;
}

void BookmarkSearchItemDelegate::clear_cache() {
    cached_sizes.clear();
}

void BookmarkSearchItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {

    painter->save();
    bool is_selected = option.state & QStyle::State_Selected;
    bool is_global = index.model()->columnCount() == BookmarkModel::max_columns;
    //BookMark bookmark = index.data().value<BookMark>();
    BookMark bookmark = index.siblingAtColumn(BookmarkModel::bookmark).data().value<BookMark>();

    bookmark_document.setTextWidth(option.rect.width());


    QString bookmark_text = index.data().toString();
    bool is_markdown = BookMark::should_be_displayed_as_markdown(bookmark_text);
    bookmark_text = BookMark::get_display_markdown_or_text(bookmark_text);

    int text_highlight_begin = -1, text_highlight_end=-1;
    int text_similarity = similarity_score_cached(bookmark_text.toLower(), pattern, &text_highlight_begin, &text_highlight_end);

    if (bookmark_text.size() > 0 && text_similarity > 0 && text_highlight_begin >= 0) {
        bookmark_text = bookmark_text.left(text_highlight_begin) + "<span style=\""+ QString::fromStdWString(MENU_MATCHED_SEARCH_HIGHLIGHT_STYLE) +"\">" + bookmark_text.mid(text_highlight_begin, text_highlight_end - text_highlight_begin) + "</span>" + bookmark_text.mid(text_highlight_end);
    }

    if (is_markdown) {
        bookmark_document.setMarkdown(bookmark_text);
    }
    else{
        bookmark_document.setHtml(bookmark_text);
    }

    QAbstractTextDocumentLayout::PaintContext ctx;


    if (is_selected) {
        ctx.palette.setColor(QPalette::Text, convert_float3_to_qcolor(UI_SELECTED_TEXT_COLOR));
    }
    else {
        ctx.palette.setColor(QPalette::Text, convert_float3_to_qcolor(UI_TEXT_COLOR));
    }
    painter->fillRect(option.rect, is_selected ? convert_float3_to_qcolor(UI_SELECTED_BACKGROUND_COLOR) : convert_float3_to_qcolor(UI_BACKGROUND_COLOR));
    //painter->fillRect(option.rect, is_selected ? Qt::red : Qt::blue);

    if (bookmark_text.size() == 0) {
        if (bookmark.description.size() > 1 && bookmark.description[0] == '#') {
            if (bookmark.description[1] >= 'a' && bookmark.description[1] <= 'z') {
                int symbol_index = bookmark.description[1] - 'a';
                float* color = &HIGHLIGHT_COLORS[3 * symbol_index];
                QColor qcolor = QColor::fromRgbF(color[0], color[1], color[2]);
                painter->save();
                painter->setPen(qcolor);
                painter->drawRect(option.rect);
                painter->restore();
            }
            if (bookmark.description[1] >= 'A' && bookmark.description[1] <= 'Z') {
                int symbol_index = bookmark.description[1] - 'A';
                float* color = &HIGHLIGHT_COLORS[3 * symbol_index];
                QColor qcolor = QColor::fromRgbF(color[0], color[1], color[2], 0.5f);
                painter->fillRect(option.rect, QBrush(qcolor));
            }
        }
    }
    painter->translate(option.rect.topLeft());
    painter->setClipRect(0, 0, option.rect.width(), option.rect.height());
    //qDebug() << "size in paint is :" << bookmark_document.toPlainText().size() << " " << index;

    bookmark_document.documentLayout()->draw(painter, ctx);

    //if (is_global) {
    painter->translate(0, bookmark_document.size().height());

    if (!is_selected) {
        ctx.palette.setColor(QPalette::Text, QColor::fromRgbF(1, 1, 1, 0.5));
    }
    else {
        ctx.palette.setColor(QPalette::Text, QColor::fromRgbF(0, 0, 0, 0.5));
    }

    file_name_document.setHtml("<div align=\"right\">" + index.siblingAtColumn(BookmarkModel::file_name).data().toString() + "</div>");
    file_name_document.documentLayout()->draw(painter, ctx);
    //}

    painter->restore();
}

BookmarkSelectorWidget::BookmarkSelectorWidget(QAbstractItemView* view, QAbstractItemModel* model, MainWidget* parent) 
    : BaseCustomSelectorWidget(view, model, parent) {

    bookmark_model = dynamic_cast<BookmarkModel*>(model);

    if (lv) {
        lv->setItemDelegate(new BookmarkSearchItemDelegate());
    }
}

BookmarkSelectorWidget* BookmarkSelectorWidget::from_bookmarks(std::vector<BookMark>&& bookmarks, MainWidget* parent, std::vector<QString>&& doc_names, std::vector<QString>&& doc_checksums) {

    BookmarkModel* bookmark_model = new BookmarkModel(std::move(bookmarks), std::move(doc_names), std::move(doc_checksums));

    QListView* list_view = get_ui_new_listview();

    BookmarkSelectorWidget* bookmark_selector_widget = new BookmarkSelectorWidget(list_view, bookmark_model, parent);

    bookmark_model->setParent(bookmark_selector_widget);
    list_view->setParent(bookmark_selector_widget);

    //setFixedSize(parent_width * MENU_SCREEN_WDITH_RATIO, parent_height);
    //bookmark_selector_widget->resize(parent->width() * MENU_SCREEN_WDITH_RATIO, parent->height());
    bookmark_selector_widget->on_resize();
    bookmark_selector_widget->set_filter_column_index(-1);

    bookmark_selector_widget->update_render();
    return bookmark_selector_widget;
}

DocumentNameModel::DocumentNameModel(
    std::vector<OpenedBookInfo>&& books, QObject* parent)
    : QAbstractTableModel(parent) {
    opened_documents = books;
}


int DocumentNameModel::rowCount(const QModelIndex& parent) const {
    if (parent == QModelIndex()) {
        return opened_documents.size();
    }
    return 0;
}

int DocumentNameModel::columnCount(const QModelIndex& parent) const {
    return DocumentNameColumn::max_columns;
}

QVariant DocumentNameModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        if (index.column() == DocumentNameColumn::file_path) {
            //return file_paths[index.row()];
            return opened_documents[index.row()].file_name;
        }
        if (index.column() == DocumentNameColumn::document_title) {
            //return document_titles[index.row()];
            return opened_documents[index.row()].document_title;
        }
        if (index.column() == DocumentNameColumn::last_access_time) {
            return opened_documents[index.row()].last_access_time;
        }
        if (index.column() == DocumentNameColumn::is_server_only) {
            return opened_documents[index.row()].is_server_only;
        }
    }

    return QVariant();
}

QVariant DocumentNameModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return "Document";
    }
    return QVariant();
}


bool DocumentNameModel::removeRows(int row, int count, const QModelIndex& parent) {
    beginRemoveRows(parent, row, row + count - 1);

    opened_documents.erase(opened_documents.begin() + row, opened_documents.begin() + row + count);

    endRemoveRows();
    return true;
}

DocumentItemDelegate::DocumentItemDelegate() {

    QFont path_font;
    QFont title_font(get_ui_font_face_name());
    QFont last_access_time_font;

    int font_size = FONT_SIZE > 0 ? FONT_SIZE : path_font.pointSize();

    if (font_size >= 0) {
        title_font.setPointSize(font_size);
        path_font.setPointSize(font_size * 3 / 4);
        last_access_time_font.setPointSize(font_size * 7 / 8);
    }

    path_document.setDefaultFont(path_font);
    title_document.setDefaultFont(title_font);
    last_access_time_document.setDefaultFont(last_access_time_font);
}

void DocumentItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {
    painter->save();
    bool is_selected = option.state & QStyle::State_Selected;

    QAbstractTextDocumentLayout::PaintContext ctx;
    if (is_selected) {
        ctx.palette.setColor(QPalette::Text, convert_float3_to_qcolor(UI_SELECTED_TEXT_COLOR));
        painter->fillRect(option.rect, convert_float3_to_qcolor(UI_SELECTED_BACKGROUND_COLOR));
    }
    else {
        ctx.palette.setColor(QPalette::Text, convert_float3_to_qcolor(UI_TEXT_COLOR));
        painter->fillRect(option.rect, convert_float3_to_qcolor(UI_BACKGROUND_COLOR));
    }
    // draw separator
    //painter->        setPen(QPen(convert_float3_to_qcolor(UI_SEPARATOR_COLOR), 1, Qt::SolidLine));
    QPoint separator_left = option.rect.bottomLeft();
    QPoint separator_right = option.rect.bottomRight();
    separator_left.setX(separator_left.x() + option.rect.width() / 8);
    separator_right.setX(separator_right.x() - option.rect.width() / 8);

    QColor separator_color = QColor::fromRgbF(
        UI_SELECTED_BACKGROUND_COLOR[0], UI_SELECTED_BACKGROUND_COLOR[1], UI_SELECTED_BACKGROUND_COLOR[2], 0.2f);
    painter->setPen(separator_color);
    painter->drawLine(separator_left, separator_right);

    bool is_server_only = index.siblingAtColumn(DocumentNameModel::is_server_only).data().toBool();
    QString title_string = highlight_pattern(index.siblingAtColumn(DocumentNameModel::document_title).data().toString().toHtmlEscaped());
    if (is_server_only){
        title_string = "<span style=\"color: #888;\">[ DOWNLOAD ] </span>" + title_string;
    }
    float offset = option.rect.topLeft().y();

    painter->setClipRect(option.rect);

    if (title_string.size() > 0) {
        painter->translate(0, offset);
        title_document.setTextWidth(option.rect.width());
        title_document.setHtml(title_string);
        title_document.documentLayout()->draw(painter, ctx);
        offset = title_document.size().height();
    }

    painter->translate(0, offset);
    path_document.setTextWidth(option.rect.width());
    path_document.setHtml("<code>" + highlight_pattern(index.siblingAtColumn(DocumentNameModel::file_path).data().toString().toHtmlEscaped()) + "</code>");
    path_document.documentLayout()->draw(painter, ctx);
    offset = path_document.size().height();

    painter->translate(0, offset);
    last_access_time_document.setTextWidth(option.rect.width());
    last_access_time_document.setHtml("<div align=\"right\">" + get_time_string(index.siblingAtColumn(DocumentNameModel::last_access_time).data().toDateTime()) + "</div>");
    last_access_time_document.documentLayout()->draw(painter, ctx);
    offset = last_access_time_document.size().height();

    painter->restore();
}

QSize DocumentItemDelegate::sizeHint(const QStyleOptionViewItem& option,
    const QModelIndex& index) const {
    const QSortFilterProxyModel *proxy_model = dynamic_cast<const QSortFilterProxyModel*>(index.model());
    auto source_index = proxy_model->mapToSource(index);
    if (cached_sizes.find(source_index.row()) != cached_sizes.end()) {
        return QSize(option.rect.width(), cached_sizes[source_index.row()]);
    }

    float height = 0;

    path_document.setTextWidth(option.rect.width());
    path_document.setHtml("<code>" + index.siblingAtColumn(DocumentNameModel::file_path).data().toString().toHtmlEscaped() + "</code>");
    height += path_document.size().height();

    QString title_string = index.siblingAtColumn(DocumentNameModel::document_title).data().toString().toHtmlEscaped();
    //if (title_string.size() > 0) {
        title_document.setTextWidth(option.rect.width());
        title_document.setHtml(title_string);
        height += title_document.size().height();
    //}

    QString time_string = index.siblingAtColumn(DocumentNameModel::last_access_time).data().toString();
    last_access_time_document.setTextWidth(option.rect.width());
    last_access_time_document.setHtml(time_string.toHtmlEscaped());
    height += last_access_time_document.size().height();

    cached_sizes[source_index.row()] = height;
    return QSize(option.rect.width(), height);
    //return QSize(option.rect.width(), 20);
}

QString DocumentItemDelegate::get_time_string(QDateTime time) const{
    int days = time.daysTo(QDateTime::currentDateTime());
    if (days == 1) {
        return "1 day ago";
    }
    return QString::number(days) + " days ago";
}

void DocumentItemDelegate::clear_cache() {
    cached_sizes.clear();
}

DocumentSelectorWidget::DocumentSelectorWidget(QAbstractItemView* view, QAbstractItemModel* model, MainWidget* parent) 
    : BaseCustomSelectorWidget(view, model, parent) {

    document_model = dynamic_cast<DocumentNameModel*>(model);

    if (lv) {
        lv->setItemDelegate(new DocumentItemDelegate());
    }
}

DocumentSelectorWidget* DocumentSelectorWidget::from_documents(
    std::vector<OpenedBookInfo>&& docs,
    MainWidget* parent
) {

    DocumentNameModel* document_model = new DocumentNameModel(std::move(docs));

    QListView* list_view = get_ui_new_listview();

    DocumentSelectorWidget* document_selector_widget = new DocumentSelectorWidget(list_view, document_model, parent);

    document_model->setParent(document_selector_widget);
    list_view->setParent(document_selector_widget);
    list_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    list_view->setUniformItemSizes(true);

    //setFixedSize(parent_width * MENU_SCREEN_WDITH_RATIO, parent_height);
    // document_selector_widget->resize(parent->width() * MENU_SCREEN_WDITH_RATIO, parent->height());
    document_selector_widget->on_resize();
    document_selector_widget->set_filter_column_index(-1);

    document_selector_widget->update_render();
    return document_selector_widget;
}

FulltextSearchWidget::FulltextSearchWidget(DatabaseManager* manager, QAbstractItemView* view, QAbstractItemModel* model, MainWidget* parent, std::wstring checksum, std::wstring tag) 
    : BaseCustomSelectorWidget(view, model, parent), db_manager(manager) {

    result_model = dynamic_cast<FulltextResultModel*>(model);
    main_widget = parent;
    maybe_file_checksum = checksum;
    maybe_tag = tag;


    if (lv) {
        lv->setItemDelegate(new SearchItemDelegate());
    }
}

FulltextSearchWidget* FulltextSearchWidget::create(MainWidget* parent, std::wstring checksum, std::wstring tag) {

    //DocumentNameModel* document_model = new DocumentNameModel(std::move(docs));
    QStringListModel* res_model = new QStringListModel();

    QListView* list_view = get_ui_new_listview();

    FulltextSearchWidget* fulltext_search_widget = new FulltextSearchWidget(parent->db_manager, list_view, res_model, parent, checksum, tag);

    res_model->setParent(fulltext_search_widget);
    list_view->setParent(fulltext_search_widget);
    list_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //list_view->setUniformItemSizes(true);

    //setFixedSize(parent_width * MENU_SCREEN_WDITH_RATIO, parent_height);
    // fulltext_search_widget->resize(parent->width() * MENU_SCREEN_WDITH_RATIO, parent->height());
    fulltext_search_widget->on_resize();
    fulltext_search_widget->set_filter_column_index(-1);

    fulltext_search_widget->update_render();
    return fulltext_search_widget;
}

SearchItemDelegate::SearchItemDelegate() {

    QFont snippet_font(get_ui_font_face_name());
    QFont location_font;

    int font_size = FONT_SIZE >= 0 ? FONT_SIZE : snippet_font.pointSize();

    if (font_size >= 0) {
        snippet_font.setPointSize(font_size);
        location_font.setPointSize(font_size * 3 / 4);
    }

    snippet_document.setDefaultFont(snippet_font);
    location_document.setDefaultFont(location_font);
    //title_document.setDefaultFont(title_font);
    //last_access_time_document.setDefaultFont(last_access_time_font);
}

void SearchItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {
    painter->save();
    bool is_selected = option.state & QStyle::State_Selected;

    QAbstractTextDocumentLayout::PaintContext ctx;
    if (is_selected) {
        ctx.palette.setColor(QPalette::Text, convert_float3_to_qcolor(UI_SELECTED_TEXT_COLOR));
        painter->fillRect(option.rect, convert_float3_to_qcolor(UI_SELECTED_BACKGROUND_COLOR));
    }
    else {
        ctx.palette.setColor(QPalette::Text, convert_float3_to_qcolor(UI_TEXT_COLOR));
        painter->fillRect(option.rect, convert_float3_to_qcolor(UI_BACKGROUND_COLOR));
    }

    QPoint separator_left = option.rect.bottomLeft();
    QPoint separator_right = option.rect.bottomRight();
    separator_left.setX(separator_left.x() + option.rect.width() / 8);
    separator_right.setX(separator_right.x() - option.rect.width() / 8);

    QColor separator_color = QColor::fromRgbF(
        UI_SELECTED_BACKGROUND_COLOR[0], UI_SELECTED_BACKGROUND_COLOR[1], UI_SELECTED_BACKGROUND_COLOR[2], 0.2f);
    painter->setPen(separator_color);
    painter->drawLine(separator_left, separator_right);

    QString location_string = get_location_string(index);

    QString snippet_string = highlight_match(index.data().toString());
    //QString location_string = 
    float offset = option.rect.topLeft().y();

    painter->setClipRect(option.rect);

    painter->translate(0, offset);
    snippet_document.setTextWidth(option.rect.width());
    snippet_document.setHtml(snippet_string);
    snippet_document.documentLayout()->draw(painter, ctx);
    offset = snippet_document.size().height();

    painter->translate(0, offset);
    location_document.setTextWidth(option.rect.width());
    location_document.setHtml(location_string);
    location_document.documentLayout()->draw(painter, ctx);
    offset = location_document.size().height();

    painter->restore();
}

QString SearchItemDelegate::highlight_match(QString match) const{
    return match.replace("SIOYEK_MATCH_BEGIN", "<span style=\""+ QString::fromStdWString(MENU_MATCHED_SEARCH_HIGHLIGHT_STYLE) +"\"; >").replace("SIOYEK_MATCH_END", "</span>");
}

QString SearchItemDelegate::get_location_string(const QModelIndex& index) const{
    QString document_name = index.siblingAtColumn(FulltextResultModel::document_title).data().toString();
    int page_number = index.siblingAtColumn(FulltextResultModel::page).data().toInt();
    QString location_string = "<div align=\"right\"><code>" + document_name + " P. " + QString::number(page_number) + "</code></div>";

    if (page_number < 0) {
        location_string = "<div align=\"right\"><code>" + document_name + "</code></div>";
    }

    return location_string;
}


QSize SearchItemDelegate::sizeHint(const QStyleOptionViewItem& option,
    const QModelIndex& index) const {
    //const QSortFilterProxyModel *proxy_model = dynamic_cast<const QSortFilterProxyModel*>(index.model());
    auto source_index = index;

    float height = 0;

    snippet_document.setTextWidth(option.rect.width());
    snippet_document.setHtml(highlight_match(index.data().toString()));
    height += snippet_document.size().height();

    QString location_string = get_location_string(index);
    location_document.setHtml(location_string);
    height += location_document.size().height();

    return QSize(option.rect.width(), height);
}

void SearchItemDelegate::clear_cache() {
    //cached_sizes.clear();
}

FulltextSearchWidget::~FulltextSearchWidget() {
    if (result_model) {
        delete result_model;
    }
}

void FulltextSearchWidget::on_text_changed(const QString& text) {

    if (result_model) {
        delete result_model;
        result_model = nullptr;

    }
    if (text.size() == 0) {
        result_model = new FulltextResultModel({}, this);
        get_view()->setModel(result_model);
    }
    else {
        QString query = text;
        query = query.replace("\"", "\\\""); // escape the "s
        query = "\"" + query + "\"" + "*";

        std::vector<FulltextSearchResult> results = db_manager->perform_fulltext_search(query.toStdWString(), maybe_file_checksum, maybe_tag);
        for (int i = 0; i < results.size(); i++) {
            results[i].document_title = main_widget->document_manager->get_path_from_hash(results[i].document_checksum).value_or(L"");
            if (results[i].document_title.size() > 0) {
                results[i].document_title = Path(results[i].document_title).filename().value_or(L"");
            }
        }

        //QStringList snippets;

        //for (auto& result : results) {
        //    snippets.append(QString::fromStdWString(result.snippet));
        //}

        //result_model->setStringList(snippets);
        result_model = new FulltextResultModel(std::move(results), this);
        get_view()->setModel(result_model);

    }
    //qDebug() << "text changed to " << text;
}

//DocumentNameModel::DocumentNameModel(
//    std::vector<OpenedBookInfo>&& books, QObject* parent)
//    : QAbstractTableModel(parent) {
//    opened_documents = books;
//}

FulltextResultModel::FulltextResultModel(std::vector<FulltextSearchResult>&& results, QObject* parent)
    : QAbstractTableModel(parent) {
    search_results = results;
}

int FulltextResultModel::rowCount(const QModelIndex& parent) const {
    if (parent == QModelIndex()) {
        return search_results.size();
    }
    return 0;
}

int FulltextResultModel::columnCount(const QModelIndex& parent) const {
    return SearchResultColumn::max_columns;
}

QVariant FulltextResultModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        if (index.column() == SearchResultColumn::snippet) {
            return QString::fromStdWString(search_results[index.row()].snippet);
        }
        if (index.column() == SearchResultColumn::document_title) {
            return QString::fromStdWString(search_results[index.row()].document_title);
        }
        if (index.column() == SearchResultColumn::page) {
            return search_results[index.row()].page;
        }
    }

    return QVariant();
}

QVariant FulltextResultModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return "Search Result";
    }
    return QVariant();
}

void FulltextSearchWidget::on_select(QModelIndex value) {
    if (select_fn.has_value()) {
        QString input_text = line_edit->text();
        select_fn.value()(value.row());
    }
    //(value.row());
}

void FulltextSearchWidget::on_delete(const QModelIndex& source_index, const QModelIndex& selected_index) {

}

QHash<int, QByteArray> HighlightModel::roleNames() const {

    QHash<int, QByteArray> roles;
    roles[HighlightModelColumn::checksum] = "checksum";
    roles[HighlightModelColumn::description] = "description";
    roles[HighlightModelColumn::file_name] = "fileName";
    roles[HighlightModelColumn::type] = "highlightType";
    roles[HighlightModelColumn::text] = "highlightText";
    return roles;

}

TouchDelegateListView::TouchDelegateListView(QAbstractTableModel* model, bool deletable, QString delegate_name, std::vector<std::pair<QString, QVariant>> props, QWidget* parent) : QWidget(parent) {
    //std::vector<Highlight> highlights = doc()->get_highlights();
    //QAbstractTableModel* highlights_model = new HighlightModel(std::move(highlights), {}, {}, this);
    model->setParent(this);

    list_view = new TouchListView(
        true,
        model,
        -1,
        this,
        deletable,
        true,
        delegate_name,
        props);

    QObject::connect(list_view, &TouchListView::itemSelected, [&](QString item, int index) {
        if (on_select.has_value()) {
            on_select.value()(index);
        }
        });

    QObject::connect(list_view, &TouchListView::itemDeleted, [&](QString item, int index) {
        if (on_delete.has_value()) {
            on_delete.value()(index);
        }
        });
}

void TouchDelegateListView::resizeEvent(QResizeEvent* resize_event) {
    QWidget::resizeEvent(resize_event);
    int parent_width = parentWidget()->size().width();
    int parent_height = parentWidget()->size().height();

    list_view->resize(parent_width * 0.9f, parent_height);
    move(parent_width * 0.05f, 0);
    resize(parent_width * 0.9f, parent_height);
}

void TouchDelegateListView::set_select_fn(std::function<void(int)>&& fn) {
    on_select = fn;
}

void TouchDelegateListView::set_delete_fn(std::function<void(int)>&& fn) {
    on_delete = fn;
}

DocumentationSearchWidget::DocumentationSearchWidget(
    DatabaseManager* manager,
    QAbstractItemView* view,
    QAbstractItemModel* model,
    MainWidget* parent
) : FulltextSearchWidget(manager, view, model, parent) {

}

void DocumentationSearchWidget::on_text_changed(const QString& text) {

    if (result_model) {
        delete result_model;
        result_model = nullptr;

    }
    if (text.size() == 0) {
        result_model = new FulltextResultModel({}, this);
        get_view()->setModel(result_model);
    }
    else {
        QString query = text;
        query = query.replace("\"", "\\\""); // escape the "s
        query = "\"" + query + "\"" + "*";

        std::vector<DocumentationSearchResult> results = db_manager->perform_documentation_search(query.toStdWString());
        std::vector<FulltextSearchResult> fulltext_results;
        for (auto docres : results) {
            FulltextSearchResult fts_result;
            fts_result.document_title = docres.item_type + L"/" + docres.item_title;
            fts_result.document_checksum = "";
            fts_result.page = -1;
            fts_result.snippet = docres.snippet;
            fulltext_results.push_back(fts_result);
        }
        //for (int i = 0; i < results.size(); i++) {
        //    results[i].document_title = main_widget->document_manager->get_path_from_hash(results[i].document_checksum).value_or(L"");
        //    if (results[i].document_title.size() > 0) {
        //        results[i].document_title = Path(results[i].document_title).filename().value_or(L"");
        //    }
        //}

        //QStringList snippets;

        //for (auto& result : results) {
        //    snippets.append(QString::fromStdWString(result.snippet));
        //}

        //result_model->setStringList(snippets);
        result_model = new FulltextResultModel(std::move(fulltext_results), this);
        get_view()->setModel(result_model);

    }
    //qDebug() << "text changed to " << text;
}

DocumentationSearchWidget* DocumentationSearchWidget::create(MainWidget* parent) {

    //DocumentNameModel* document_model = new DocumentNameModel(std::move(docs));
    QStringListModel* res_model = new QStringListModel();

    QListView* list_view = get_ui_new_listview();

    DocumentationSearchWidget* fulltext_search_widget = new DocumentationSearchWidget(parent->db_manager, list_view, res_model, parent);

    res_model->setParent(fulltext_search_widget);
    list_view->setParent(fulltext_search_widget);
    list_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    fulltext_search_widget->on_resize();
    fulltext_search_widget->set_filter_column_index(-1);

    fulltext_search_widget->update_render();
    return fulltext_search_widget;
}

SelectionIndicator::SelectionIndicator(
    QWidget* parent,
    bool begin,
    MainWidget* w,
    AbsoluteDocumentPos pos)
        : QWidget(parent), is_begin(begin), main_widget(w) {
    docpos = pos.to_document(w->doc());

    //begin_icon = QIcon(":/icons/arrow-begin.svg");
    //end_icon = QIcon(":/icons/arrow-end.svg");
    begin_icon = QIcon(":/icons/selection-begin.svg");
    end_icon = QIcon(":/icons/selection-end.svg");
}

void SelectionIndicator::update_pos() {
    WindowPos wp = docpos.to_window(main_widget->main_document_view);
    if (is_begin) {
        move(wp.x - width(), wp.y - height());
    }
    else {
        move(wp.x, wp.y);
    }

}

void SelectionIndicator::mousePressEvent(QMouseEvent* mevent) {
    is_dragging = true;
    last_press_window_pos = mapToParent(mevent->pos());
    last_press_widget_pos = pos();
    docpos_needs_recompute = true;
}

void SelectionIndicator::mouseMoveEvent(QMouseEvent* mouse_event) {
    if (is_dragging) {
        QPoint mouse_pos = mapToParent(mouse_event->pos());
        QPoint diff = mouse_pos - last_press_window_pos;
        QPoint new_widget_pos = last_press_widget_pos + diff;
        move(new_widget_pos);
        docpos_needs_recompute = true;
        main_widget->update_mobile_selection();
    }
}

void SelectionIndicator::mouseReleaseEvent(QMouseEvent* mevent) {
    is_dragging = false;
}

DocumentPos SelectionIndicator::get_docpos() {
    if (!docpos_needs_recompute) return docpos;

    if (is_begin) {
        docpos = WindowPos{ x() + width(), y() + height() }.to_document(main_widget->main_document_view);
    }
    else {
        docpos = WindowPos{x(), y()}.to_document(main_widget->main_document_view);
    }

    return docpos;

}

void SelectionIndicator::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    (is_begin ? &begin_icon : &end_icon)->paint(&painter, rect());
}

SioyekDocumentationTextBrowser::SioyekDocumentationTextBrowser(MainWidget* parent) : QTextBrowser(parent) {
    main_widget = parent;
}

void SioyekDocumentationTextBrowser::mousePressEvent(QMouseEvent* mevent) {
    // prevent the click events to be propagated to the parent widget
    QTextBrowser::mousePressEvent(mevent);
    mevent->accept();
}

void SioyekDocumentationTextBrowser::mouseReleaseEvent(QMouseEvent* mevent) {
    QTextBrowser::mouseReleaseEvent(mevent);
    mevent->accept();
}

void SioyekDocumentationTextBrowser::wheelEvent(QWheelEvent* wevent) {
    QTextBrowser::wheelEvent(wevent);
    wevent->accept();
}

void SioyekDocumentationTextBrowser::backward() {
    QTextBrowser::backward();
    QUrl current_url = historyUrl(0);
    doSetSource(current_url, QTextDocument::ResourceType::MarkdownResource);
}

void SioyekDocumentationTextBrowser::forward() {
    QTextBrowser::forward();
    QUrl current_url = historyUrl(0);
    doSetSource(current_url, QTextDocument::ResourceType::MarkdownResource);
}

void SioyekDocumentationTextBrowser::doSetSource(const QUrl& url, QTextDocument::ResourceType type) {
    // push the previous state to history
    QTextBrowser::doSetSource(url, type);

    QString url_string = url.toString();

    if (url_string.startsWith("sioyeklink#")) {
        url_string = url_string.mid(11).trimmed();
        main_widget->pop_current_widget();
        main_widget->dv()->perform_fuzzy_search(url_string.toStdWString());
        main_widget->goto_search_result(0);
        main_widget->invalidate_render();

    }
    else if (url_string.startsWith("commands.md")) {
        QString documentation_title = url_string.mid(12).trimmed();
        QString documentation = main_widget->get_command_documentation_with_title(documentation_title);
        setMarkdown(documentation);
    }
    else if (url_string.startsWith("configs.md")) {
        QString documentation_title = url_string.mid(11).trimmed();
        QString documentation = main_widget->get_config_documentation_with_title("", documentation_title).trimmed();
        setMarkdown(documentation);
    }
    else if (url_string.startsWith("setconfig")) {
        QString config_name = url_string.split('-').at(1);
        main_widget->pop_current_widget();
        main_widget->execute_macro_if_enabled(L"set_config('" + config_name.toStdWString() + L"')");
    }
    else if (url_string.startsWith("setsaveconfig")) {
        QString config_name = url_string.split('-').at(1);
        main_widget->pop_current_widget();
        main_widget->execute_macro_if_enabled(L"set_config('" + config_name.toStdWString() + L"');save_config('" + config_name.toStdWString() + L"')");
    }
    else if (url_string.startsWith("changeconfig")){
        QString config_name = url_string.split('-').at(1);
        Config* config_obj = main_widget->config_manager->get_mut_config_with_name(config_name.toStdWString());
        if (config_obj) {
            if (EXTERNAL_TEXT_EDITOR_COMMAND.size() == 0) {
                show_error_message(L"external_text_editor_command config must be set for this to work");
            }
            main_widget->pop_current_widget();

            if (config_obj->definition_file.size() > 0) {
                // if the config exists in a config file, open the location of the config
                std::wstring command = QString::fromStdWString(EXTERNAL_TEXT_EDITOR_COMMAND)
                    .replace("%{file}", QString::fromStdWString(config_obj->definition_file))
                    .replace("%{line}", QString::number(config_obj->definition_line))
                    .toStdWString();
                main_widget->execute_command(command);
            }
            else {
                // if the config is not present in any config files, open the prefs_user
                // file so that the user can add it 
                main_widget->execute_macro_if_enabled(L"prefs_user");
            }
        }
    }
    else {
        //qDebug() << "clicled on url:";
        //qDebug() << url;
        //QTextBrowser::doSetSource(url, type);
    }
}

ItemWithDescriptionDelegate::ItemWithDescriptionDelegate(){
    QFont item_font(get_ui_font_face_name());
    QFont description_font;

    int font_size = FONT_SIZE > 0 ? FONT_SIZE : item_font.pointSize();

    if (font_size >= 0) {
        item_font.setPointSize(font_size);
        description_font.setPointSize(font_size * 3 / 4);
    }

    item_document.setDefaultFont(item_font);
    description_document.setDefaultFont(description_font);
}

QSize ItemWithDescriptionDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    const QSortFilterProxyModel *proxy_model = dynamic_cast<const QSortFilterProxyModel*>(index.model());
    auto source_index = proxy_model->mapToSource(index);
    if (cached_sizes.find(source_index.row()) != cached_sizes.end()) {
        return QSize(option.rect.width(), cached_sizes[source_index.row()]);
    }

    QString item_text = index.siblingAtColumn(ItemWithDescriptionModel::item_text).data().toString();
    QString description = index.siblingAtColumn(ItemWithDescriptionModel::description).data().toString();

    item_document.setTextWidth(option.rect.width());
    item_document.setHtml(item_text);

    QSize res = item_document.size().toSize();
    int col_count = index.model()->columnCount();
    bool is_global = index.model()->columnCount() == ItemWithDescriptionModel::max_columns;

    description_document.setTextWidth(option.rect.width());
    description_document.setHtml("<div align=\"right\">" + index.siblingAtColumn(ItemWithDescriptionModel::description).data().toString() + "</div>");
    res = QSize(res.width(), res.height() + description_document.size().toSize().height());

    cached_sizes[source_index.row()] = res.height();
    return res;
}

void ItemWithDescriptionDelegate::clear_cache() {
    cached_sizes.clear();
}

void ItemWithDescriptionDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {

    painter->save();
    bool is_selected = option.state & QStyle::State_Selected;

    item_document.setTextWidth(option.rect.width());

    QString item_text = index.data().toString();

    int text_highlight_begin = -1, text_highlight_end=-1;
    int text_similarity = similarity_score_cached(item_text.toLower(), pattern, &text_highlight_begin, &text_highlight_end);

    if (text_similarity > 0 && text_highlight_begin >= 0) {
        item_text = item_text.left(text_highlight_begin) + "<span style=\""+ QString::fromStdWString(MENU_MATCHED_SEARCH_HIGHLIGHT_STYLE) +"\">" + item_text.mid(text_highlight_begin, text_highlight_end - text_highlight_begin) + "</span>" + item_text.mid(text_highlight_end);
    }

    item_document.setHtml(item_text);

    QAbstractTextDocumentLayout::PaintContext ctx;


    if (is_selected) {
        ctx.palette.setColor(QPalette::Text, convert_float3_to_qcolor(UI_SELECTED_TEXT_COLOR));
    }
    else {
        ctx.palette.setColor(QPalette::Text, convert_float3_to_qcolor(UI_TEXT_COLOR));
    }
    painter->fillRect(option.rect, is_selected ? convert_float3_to_qcolor(UI_SELECTED_BACKGROUND_COLOR) : convert_float3_to_qcolor(UI_BACKGROUND_COLOR));
    //painter->fillRect(option.rect, is_selected ? Qt::red : Qt::blue);

    painter->translate(option.rect.topLeft());
    painter->setClipRect(0, 0, option.rect.width(), option.rect.height());

    item_document.documentLayout()->draw(painter, ctx);

    //if (is_global) {
    painter->translate(0, item_document.size().height());

    if (!is_selected) {
        ctx.palette.setColor(QPalette::Text, QColor::fromRgbF(1, 1, 1, 0.5));
    }
    else {
        ctx.palette.setColor(QPalette::Text, QColor::fromRgbF(0, 0, 0, 0.5));
    }

    description_document.setHtml("<div align=\"right\">" + index.siblingAtColumn(ItemWithDescriptionModel::description).data().toString() + "</div>");
    description_document.documentLayout()->draw(painter, ctx);
    //}

    painter->restore();
}

ItemWithDescriptionModel::ItemWithDescriptionModel(std::vector<QString> && items_, std::vector<QString> && descriptions_, std::vector<QString> && metadata_, QObject * parent): QAbstractTableModel(parent) {
    items = items_;
    descriptions = descriptions_;
    metadatas = metadata_;
}

int ItemWithDescriptionModel::rowCount(const QModelIndex& parent) const {
    if (parent == QModelIndex()) {
        return items.size();
    }
    return 0;
}

int ItemWithDescriptionModel::columnCount(const QModelIndex& parent) const {
    if (metadatas.size() == 0) {
        return 2;
    }
    return 3;
}

QVariant ItemWithDescriptionModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::DisplayRole) {
        if (index.column() == ItemWithDescriptionColumn::item_text) {
            return items[index.row()];
        }
        if (index.column() == ItemWithDescriptionColumn::description) {
            return descriptions[index.row()];
        }
        if (index.column() == ItemWithDescriptionColumn::metadata) {
            return metadatas[index.row()];
        }
    }
    return QVariant();
}

QVariant ItemWithDescriptionModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        return "Item";
    }
    return QVariant();
}

ItemWithDescriptionSelectorWidget::ItemWithDescriptionSelectorWidget(QAbstractItemView* view, QAbstractItemModel* model, MainWidget* parent) 
    : BaseCustomSelectorWidget(view, model, parent) {

    item_model = dynamic_cast<ItemWithDescriptionModel*>(model);

    if (lv) {
        lv->setItemDelegate(new ItemWithDescriptionDelegate());
    }
}

ItemWithDescriptionSelectorWidget* ItemWithDescriptionSelectorWidget::from_items(std::vector<QString>&& items, std::vector<QString>&& descriptions, std::vector<QString>&& metadata, MainWidget* parent) {

    ItemWithDescriptionModel* item_model = new ItemWithDescriptionModel(std::move(items), std::move(descriptions), std::move(metadata));

    QListView* list_view = get_ui_new_listview();

    ItemWithDescriptionSelectorWidget* item_selector_widget = new ItemWithDescriptionSelectorWidget(list_view, item_model, parent);

    item_model->setParent(item_selector_widget);
    list_view->setParent(item_selector_widget);

    item_selector_widget->on_resize();
    item_selector_widget->set_filter_column_index(-1);

    item_selector_widget->update_render();
    return item_selector_widget;
}

SioyekBookmarkTextBrowser::SioyekBookmarkTextBrowser(MainWidget* parent, QString uuid, QString content, bool chat) : QWidget(parent){
    bookmark_uuid = uuid;
    main_widget = parent;

    //text_browser = new SioyekDocumentationTextBrowser(main_widget);
    //text_browser->setParent(this);
    //text_browser->setStyleSheet("QTextBrowser{" + get_status_stylesheet(false, DOCUMENTATION_FONT_SIZE) + "border-radius: 4px; padding: 10px;}\n" + get_scrollbar_stylesheet());
    //text_browser->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    //text_browser->setReadOnly(true);
    //text_browser->setSource(QUrl(), QTextDocument::ResourceType::MarkdownResource);
    //text_browser->setMarkdown(content);

    text_browser = new SioyekChatTextBrowser(this, content);
    QObject::connect(text_browser, &SioyekChatTextBrowser::anchorClicked, [this](QString url_string) {
        // percent decode the url string
        url_string = QUrl::fromPercentEncoding(url_string.toUtf8());
        if (url_string.startsWith("sioyeklink#")) {
            url_string = url_string.mid(11).trimmed();
            main_widget->pop_current_widget();
            main_widget->push_state();
            main_widget->dv()->perform_fuzzy_search(url_string.toStdWString());
            main_widget->goto_search_result(0);
            main_widget->invalidate_render();
        }
        else if (url_string.startsWith("http")) {
            open_web_url(url_string.toStdWString());
        }
        });



    if (chat) {
        line_edit = new MyLineEdit(main_widget);
        line_edit->setPlaceholderText("Chat with the document.");
        line_edit->setParent(this);

        QColor background_color = convert_float3_to_qcolor(CHAT_WINDOW_USER_MESSAGE_BACKGROUND_COLOR);
        QColor text_color = convert_float3_to_qcolor(CHAT_WINDOW_USER_TEXT_COLOR);

        line_edit->setStyleSheet("QLineEdit{background-color: " + background_color.name() + "; color: " + text_color.name() + "; border-radius: 0px; padding: 10px;}");
        text_browser->setFocusPolicy(Qt::NoFocus);
    }

    layout = new QVBoxLayout(this);

    layout->addWidget(text_browser);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    if (chat) {
        layout->addWidget(line_edit);
    }

    layout->setStretch(0, 1);

    if (chat) {
        line_edit->setFocus();
    }

    setLayout(layout);
}


void SioyekBookmarkTextBrowser::handle_resize() {
    int parent_width = parentWidget()->width();
    int parent_height = parentWidget()->height();
    setFixedSize(parent_width * MENU_SCREEN_WDITH_RATIO, parent_height * MENU_SCREEN_HEIGHT_RATIO);
    move(parent_width * (1 - MENU_SCREEN_WDITH_RATIO) / 2, parent_height * (1 - MENU_SCREEN_HEIGHT_RATIO) / 2);
}
void SioyekBookmarkTextBrowser::resizeEvent(QResizeEvent* resize_event) {

    QWidget::resizeEvent(resize_event);

    handle_resize();

}

void SioyekBookmarkTextBrowser::update_text(QString new_text) {

    int current_scroll_amount = text_browser->verticalScrollBar()->value();
    if (current_scroll_amount != last_set_scroll_amount) {
        follow_output = false;
    }

    //text_browser->setMarkdown(new_text);
    text_browser->update_text(new_text);


    if (follow_output) {
        int scroll_amount = text_browser->verticalScrollBar()->maximum();
        text_browser->verticalScrollBar()->setValue(scroll_amount);
        last_set_scroll_amount = scroll_amount;

    }
    else {
        text_browser->verticalScrollBar()->setValue(current_scroll_amount);
    }
}

void SioyekBookmarkTextBrowser::set_follow_output(bool val) {
    last_set_scroll_amount = text_browser->verticalScrollBar()->value();
    follow_output = val;
}

std::pair<int, int> SioyekChatTextBrowser::get_cursor_message_and_char_index(QPoint cursor_pos, int forced_message_index) {

    int y = margin - verticalScrollBar()->value();
    int msg_index = 0;

    for (auto [message_type, msg] : messages) {

        int box_height, box_width, box_x, inner_margin;
        QTextDocument* doc = get_doc_and_size_values_for_index(msg_index, &box_width, &box_height, &box_x, &inner_margin);

        if ((forced_message_index == -1) || (msg_index == forced_message_index)) {
            QRect message_rect(box_x, y, box_width, box_height);
            if (message_rect.contains(cursor_pos) || (forced_message_index != -1)) {
                QPointF localPos = cursor_pos - QPoint(box_x + inner_margin, y + inner_margin);
                int char_index = doc->documentLayout()->hitTest(localPos, Qt::FuzzyHit);
                return { msg_index, char_index };
            }
        }
        y += box_height + spacing;
        msg_index++;
    }
    return { -1, -1 };
}

void SioyekChatTextBrowser::mousePressEvent(QMouseEvent* mevent) {
    QPoint click_pos = mevent->pos();
    auto [message_index, char_index] = get_cursor_message_and_char_index(click_pos);
    if (message_index >= 0) {
        selection_message_index = message_index;
        selection_start_pos = char_index;
        selection_end_pos = char_index;
        is_selecting = true;
        viewport()->update();
    }
    QAbstractScrollArea::mousePressEvent(mevent);
    mevent->accept();
}

QTextDocument* SioyekChatTextBrowser::get_document_for_index(int index) {
    auto it = cached_documents.find(index);
    if (it != cached_documents.end()) {
        return it->second.get();
    }
    else {
        auto [message_type, message_text] = messages[index];
        std::unique_ptr<QTextDocument> doc = std::make_unique<QTextDocument>();
        doc->setDefaultFont(chat_font);
        doc->setMarkdown(message_text);
        if (message_type == ChatMessageType::ResponseMessage) {
            doc->setTextWidth(response_content_width);
        }
        else {
            float ideal_width = doc->idealWidth();
            float text_width = (ideal_width < user_content_width) ? ideal_width : user_content_width;
            doc->setTextWidth(text_width);
        }
        cached_documents[index] = std::move(doc);
        return cached_documents[index].get();
    }
    return nullptr;
}

QTextDocument* SioyekChatTextBrowser::get_doc_and_size_values_for_index(int index, int* out_box_width, int* out_box_height, int* out_box_x, int* out_inner_margin) {
    QTextDocument* doc = get_document_for_index(index);
    auto message_type = messages[index].message_type;
    if (message_type == ChatMessageType::UserMessage) {
        int required_width = static_cast<int>(doc->idealWidth()) + 2 * user_inner_margin;
        int box_width = std::min(user_box_width, required_width);

        *out_box_width = box_width;
        *out_inner_margin = user_inner_margin;
        *out_box_height = static_cast<int>(doc->size().height()) + 2 * user_inner_margin;
        *out_box_x = full_width - margin - box_width;
    }
    else {
        *out_box_width = response_content_width;
        *out_inner_margin = 0;
        *out_box_height = static_cast<int>(doc->size().height());
        *out_box_x = margin;
    }
    return doc;
}

QString SioyekChatTextBrowser::get_selected_text(){
    if (selection_message_index == -1) {
        return "";
    }
    else {
        QTextDocument* doc = get_document_for_index(selection_message_index);
        QTextCursor cursor(doc);
        cursor.setPosition(selection_start_pos);
        cursor.setPosition(selection_end_pos, QTextCursor::KeepAnchor);
        return cursor.selectedText();
    }
}

void SioyekChatTextBrowser::mouseMoveEvent(QMouseEvent* mevent) {
    QPoint mouse_pos = mevent->pos();

    int last_selection_end = selection_end_pos;
    if (is_selecting && selection_message_index != -1) {

        auto [message_index, char_index] = get_cursor_message_and_char_index(mouse_pos, selection_message_index);
        if (message_index >= 0 && message_index == selection_message_index) {
            selection_end_pos = char_index;
            viewport()->update();
        }
    }
    QAbstractScrollArea::mouseMoveEvent(mevent);
    mevent->accept();
}

void SioyekChatTextBrowser::mouseReleaseEvent(QMouseEvent* mevent) {
    // Check if a markdown link was clicked.
    QPoint click_pos = mevent->pos();

    int y = margin - verticalScrollBar()->value();

    int index = 0;
    for (auto [message_type, msg] : messages) {

        int box_height, box_width, box_x, inner_margin;
        QTextDocument* doc = get_doc_and_size_values_for_index(index, &box_width, &box_height, &box_x, &inner_margin);

        QSizeF docSize = doc->size();
        QRect message_rect(box_x, y, box_width, box_height);
        if (message_rect.contains(click_pos)) {
            QPointF localPos = click_pos - QPoint(box_x + inner_margin, y + inner_margin);
            QString anchor = doc->documentLayout()->anchorAt(localPos);
            if (!anchor.isEmpty()) {
                emit anchorClicked(anchor);
                break;
            }
        }
        y += box_height + spacing;
        index++;
    }

    mevent->accept();
}

void SioyekChatTextBrowser::mouseDoubleClickEvent(QMouseEvent* mevent) {
    QAbstractScrollArea::mouseDoubleClickEvent(mevent);
    mevent->accept();
}

SioyekChatTextBrowser::SioyekChatTextBrowser(QWidget* parent, QString text): QAbstractScrollArea(parent) {
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    update_text(text);
    chat_font = QFont(get_chat_font_face_name());
    chat_font.setPointSize(get_chat_font_size());
}

void SioyekChatTextBrowser::update_text(QString new_text) {
    messages.clear();
    cached_documents.clear();
    QStringList lines = new_text.split("\n", Qt::KeepEmptyParts);

    QString current_message;

    for (auto line : lines) {
        if (line.startsWith("? ")) {
            if (current_message.size() > 0) {
                messages.push_back({ ChatMessageType::ResponseMessage, current_message });
                current_message = "";
            }
            messages.push_back({ ChatMessageType::UserMessage, line.mid(2) });
        }
        else {
            current_message += line + "\n";
        }
    }
    if (current_message.size() > 0) {
        messages.push_back({ ChatMessageType::ResponseMessage, current_message });
    }


}

void SioyekChatTextBrowser::paintEvent(QPaintEvent* event) {
    QColor user_background_color = qconvert_color3(CHAT_WINDOW_USER_MESSAGE_BACKGROUND_COLOR, ColorPalette::Normal);
    QColor background_color = qconvert_color3(CHAT_WINDOW_BACKGROUND_COLOR, ColorPalette::Normal);
    QColor text_color = qconvert_color3(CHAT_WINDOW_TEXT_COLOR, ColorPalette::Normal);
    QColor user_text_color = qconvert_color3(CHAT_WINDOW_USER_TEXT_COLOR, ColorPalette::Normal);

    QPainter painter(viewport());
    painter.fillRect(rect(), background_color);
    painter.translate(0, -verticalScrollBar()->value());

    int y = margin;
    int msg_index = 0;

    QColor text_highlight_color = qconvert_color3(DEFAULT_TEXT_HIGHLIGHT_COLOR, ColorPalette::Normal);

    for (auto [message_type, msg] : messages) {

        int box_height, box_width, box_x, inner_margin;
        QTextDocument* doc = get_doc_and_size_values_for_index(msg_index, &box_width, &box_height, &box_x, &inner_margin);
            
        QRect messageRect(box_x, y, box_width, box_height);

        // draw background for user messages.
        if (message_type == ChatMessageType::UserMessage) {
            painter.save();
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setBrush(user_background_color);
            painter.setPen(Qt::NoPen);
            int radius = painter.font().pointSize();
            painter.drawRoundedRect(messageRect, radius, radius);
            painter.restore();
        }

        painter.save();
        painter.translate(box_x + inner_margin, y + inner_margin);

        QAbstractTextDocumentLayout::PaintContext ctx;
        if (message_type == ChatMessageType::UserMessage) {
            ctx.palette.setColor(QPalette::Text, user_text_color);
            ctx.palette.setColor(QPalette::Base, user_background_color);
        }
        else {
            ctx.palette.setColor(QPalette::Text, text_color);
            ctx.palette.setColor(QPalette::Base, background_color);
        }
        // if this is the message being selected and a range exists, add the selection.
        if (msg_index == selection_message_index && selection_start_pos != selection_end_pos) {
            QTextLayout::FormatRange selection;
            selection.start = qMin(selection_start_pos, selection_end_pos);
            selection.length = qAbs(selection_end_pos - selection_start_pos);
            QTextCharFormat fmt;
            fmt.setBackground(text_highlight_color);
            fmt.setForeground(Qt::black);
            selection.format = fmt;
            QAbstractTextDocumentLayout::Selection selectionRange;
            selectionRange.cursor = QTextCursor(doc);
            selectionRange.cursor.setPosition(selection.start);
            selectionRange.cursor.setPosition(selection.start + selection.length, QTextCursor::KeepAnchor);
            selectionRange.format = fmt;
            ctx.selections.append(selectionRange);
        }

        doc->documentLayout()->draw(&painter, ctx);
        painter.restore();

        y += box_height + spacing;
        ++msg_index;
    }
    update_scrollbar(y);
}

void SioyekChatTextBrowser::resizeEvent(QResizeEvent* event) {
    cached_documents.clear();
    QAbstractScrollArea::resizeEvent(event);

    full_width = viewport()->width();
    response_content_width = full_width - 2 * margin;
    user_box_width = static_cast<int>(0.7 * full_width);
    user_content_width = user_box_width - 2 * user_inner_margin;
}

void SioyekChatTextBrowser::update_scrollbar(int content_height) {
    verticalScrollBar()->setPageStep(viewport()->height());
    verticalScrollBar()->setRange(0, content_height - viewport()->height());
}

void SioyekChatTextBrowser::wheelEvent(QWheelEvent* event) {
    int delta = event->angleDelta().y();
    
    int step = delta / 8 * verticalScrollBar()->singleStep() * 2;
    
    verticalScrollBar()->setValue(verticalScrollBar()->value() - step);
    event->accept();
}

void SioyekBookmarkTextBrowser::scroll_amount(int amount) {

    int current_scroll_amount = text_browser->verticalScrollBar()->value();
    int step = text_browser->verticalScrollBar()->singleStep() * amount * 60;
    int new_scroll_amount = current_scroll_amount + step;
    text_browser->verticalScrollBar()->setValue(new_scroll_amount);
    last_set_scroll_amount = new_scroll_amount;
}