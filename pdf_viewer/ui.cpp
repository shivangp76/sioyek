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
#include <qpainterpath.h>

#include "database.h"
#include "document.h"
#include "document_view.h"
#include "touchui/TouchSlider.h"
#include "touchui/TouchConfigMenu.h"
#include "touchui/TouchSettings.h"

#include "utils/text_utils.h"
#include "utils/color_utils.h"
#include "utils/window_utils.h"
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


extern float UI_TEXT_COLOR[3];
extern float UI_BACKGROUND_COLOR[3];
extern float UI_SELECTED_TEXT_COLOR[3];
extern float UI_SELECTED_BACKGROUND_COLOR[3];
extern wchar_t FREEHAND_TYPE;
extern float FREEHAND_SIZE;

#ifdef SIOYEK_IOS
extern "C" QString promptUserToSelectPdfFile(QString);
extern "C" float ios_brightness_get();
#endif
extern Path standard_data_path;

std::wstring select_command_file_name(std::string command_name, std::optional<QString> root_dir) {
    if (command_name == "open_document") {
        return select_document_file_name(root_dir);
    }
    else if (command_name == "source_config") {
        return select_any_file_name(root_dir);
    }
    else {
        return select_any_file_name(root_dir);
    }
}

std::wstring select_command_folder_name(std::optional<QString> root_dir) {
    QString dir_name = QFileDialog::getExistingDirectory(nullptr, "Select Folder", root_dir.value_or(QString()));
    return dir_name.toStdWString();
}

std::wstring select_document_file_name(std::optional<QString> root_dir) {
    if (DEFAULT_OPEN_FILE_PATH.size() == 0) {

#ifdef SIOYEK_IOS
        
        QString file_url = promptUserToSelectPdfFile(QString::fromStdWString(standard_data_path.get_path()));
        return file_url.toStdWString();
        // QUrl file_url = QFileDialog::getOpenFileUrl(nullptr, "Select Document", QUrl(), "");
        
        // return file_url.toLocalFile().toStdWString();
#else
        QString file_name = QFileDialog::getOpenFileName(nullptr, "Select Document", root_dir.value_or(""), "Documents (*.pdf *.epub *.cbz)");
        return file_name.toStdWString();
#endif
    }
    else {

        QFileDialog fd = QFileDialog(nullptr, "Select Document", root_dir.value_or(""), "Documents (*.pdf *.epub *.cbz)");
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

std::wstring select_json_file_name(std::optional<QString> root_dir) {
    QString file_name = QFileDialog::getSaveFileName(nullptr, "Select Document", root_dir.value_or(""), "Documents (*.json )");
    return file_name.toStdWString();
}

std::wstring select_any_file_name(std::optional<QString> root_dir) {
    QString file_name = QFileDialog::getSaveFileName(nullptr, "Select File", root_dir.value_or(""), "Any (*)");
    return file_name.toStdWString();
}

std::wstring select_any_existing_file_name(std::optional<QString> root_dir) {
    QString file_name = QFileDialog::getOpenFileName(nullptr, "Select File", root_dir.value_or(""), "Any (*)");
    return file_name.toStdWString();
}

std::wstring select_new_json_file_name(std::optional<QString> root_dir) {
    QString file_name = QFileDialog::getSaveFileName(nullptr, "Select Document", root_dir.value_or(""), "Documents (*.json )");
    return file_name.toStdWString();
}

std::wstring select_new_pdf_file_name(std::optional<QString> root_dir) {
    QString file_name = QFileDialog::getSaveFileName(nullptr, "Select Document", root_dir.value_or(""), "Documents (*.pdf )");
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
void AndroidSelector::update_context_properties(){
    main_menu->update_context_properties();
}

AndroidSelector::AndroidSelector(QWidget* parent) : QWidget(parent) {
    //     layout = new QVBoxLayout();

    main_widget = dynamic_cast<MainWidget*>(parent);
    int current_colorscheme_index = main_widget->get_current_colorscheme_index();
    bool horizontal_locked = main_widget->horizontal_scroll_locked;
    bool fullscreen = main_widget->isFullScreen();
    bool ruler = main_widget->main_document_view->is_ruler_mode();
    bool speaking = main_widget->is_lq_ttsing();
    bool portaling = main_widget->main_document_view->is_pending_link_source_filled();
    bool fit_mode = main_widget->main_document_view->last_smart_fit_page.has_value();
    bool is_logged_in = main_widget->is_logged_in();
    QJsonObject current_user = main_widget->get_current_user();
    bool is_current_document_synced = main_widget->is_current_document_available_on_server();
    DrawingMode drawing_mode = main_widget->freehand_drawing_mode;
    #ifdef SIOYEK_ANDROID
    float current_brightness = android_brightness_get();
    #elif defined(SIOYEK_IOS) 
    float current_brightness = ios_brightness_get();
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
        drawing_mode,
        current_user,
        this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(main_menu);
    setLayout(layout);

    // main_menu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);


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
        // main_widget->pop_current_widget();

        // main_widget->invalidate_render();

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
        if (main_widget->mdv()->is_ruler_mode()){
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
       // main_widget->run_command_with_name("start_reading", true);
        main_widget->run_command_with_name("start_reading_high_quality", true);
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

    QObject::connect(main_menu, &TouchMainMenu::drawingModeSelected, [&](QString mode) {
        if (mode == "none") {
            main_widget->set_hand_drawing_mode(DrawingMode::NotDrawing);
        }
        else if (mode == "finger"){
            main_widget->set_hand_drawing_mode(DrawingMode::Drawing);
        }
        else if (mode == "pen"){
            main_widget->set_hand_drawing_mode(DrawingMode::PenDrawing);
        }
        // qDebug() << "drawing mode = " << mode;
        });

    QObject::connect(main_menu, &TouchMainMenu::drawColorClicked, [&]() {
        main_widget->execute_macro_if_enabled(L"show_touch_ui_for_config(freehand_drawing_type)");
        // qDebug() << "draw color clicked";
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


TouchTextSelectionButtons::TouchTextSelectionButtons(MainWidget* parent) : QWidget(parent) {
    main_widget = parent;
    buttons_ui = new TouchCopyOptions(this);
    this->setAttribute(Qt::WA_NoMousePropagation);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(buttons_ui);
    setLayout(layout);

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
    controls_ui = new TouchDrawControls(FREEHAND_SIZE, FREEHAND_TYPE, this);
    this->setAttribute(Qt::WA_NoMousePropagation);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(controls_ui);
    setLayout(layout);

    QObject::connect(controls_ui, &TouchDrawControls::exitDrawModePressed, [&]() {
        main_widget->exit_freehand_drawing_mode();
        });

    QObject::connect(controls_ui, &TouchDrawControls::changeColorPressed, [&](int color_index) {
        FREEHAND_TYPE = 'a' + color_index;
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
        FREEHAND_SIZE = val;
        });


}
QRect DrawControlsUI::get_prefered_rect(QRect parent_rect){
    int pwidth = parent_rect.width();
    int width = parent_rect.width() * 3 / 4;
    int height = std::max(parent_rect.height() / 16, 50);
    return QRect((pwidth - width) / 2, height, width, height);
}

QRect AndroidSelector::get_prefered_rect(QRect parent_rect){

    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();

    int ten_cm = static_cast<int>(12 * logicalDpiX() / 2.54f);

    int w = static_cast<int>(parent_width * 0.9f);
    int h = parent_height;

    w = std::min(w, ten_cm);
    h = std::min(h, static_cast<int>(ten_cm * 1.3f));

    int offset_x = (parent_width - w) / 2;
    int offset_y = (parent_height - h) / 2;
    return QRect(offset_x, offset_y, w, h);
}

QRect TouchTextSelectionButtons::get_prefered_rect(QRect parent_rect){
    int pwidth = parent_rect.width();
    int width = parent_rect.width() * 3 / 4;
    int height = parent_rect.height() / 16;
    return QRect((pwidth - width) / 2, height, width, height);
}


HighlightButtons::HighlightButtons(MainWidget* parent) : QWidget(parent) {
    main_widget = parent;
    //layout = new QHBoxLayout();

    //delete_highlight_button = new QPushButton("Delete");
    //buttons_widget = new 
    highlight_buttons = new TouchHighlightButtons(main_widget->get_current_selected_highlight_type(), this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(highlight_buttons);
    setLayout(layout);

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

QRect HighlightButtons::get_prefered_rect(QRect parent_rect){

    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();

    int dpi = physicalDpiY();
    // float parent_height_in_centimeters = static_cast<float>(parent_height) / dpi * 2.54f;

    //int w = static_cast<int>(parent_width / 5);
    int w = parent_width;
    int h = static_cast<int>(static_cast<float>(dpi) * 2 / 2.54f);
    w = std::min(std::max(w, h * 6), parent_width);

    return QRect((parent_width - w) / 2, parent_height / 5, w, h);
}


SearchButtons::SearchButtons(MainWidget* parent) : QWidget(parent) {
    main_widget = parent;
    buttons_widget = new TouchSearchButtons(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(buttons_widget);
    setLayout(layout);

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
}


QRect SearchButtons::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();

    int width = 2 * parent_rect.width() / 3;
    int height = parent_rect.height() / 12;


    return QRect((parent_width - width) / 2, parent_height - 3 * height / 2, width, height);
}

//ConfigUI::ConfigUI(MainWidget* parent) : QQuickWidget(parent){

Color3ConfigUI::Color3ConfigUI(std::string name, MainWidget* parent, float* config_location_) : ConfigUI(name, parent) {
    color_location = config_location_;
    QColor initial_color = convert_float3_to_qcolor(color_location);

    color_picker = new QColorDialog(initial_color, this);
    color_picker->show();

    // QVBoxLayout* layout = new QVBoxLayout(this);
    // layout->addWidget(color_picker);
    // setLayout(layout)

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

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(macro_editor);
    setLayout(layout);

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

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(slider);
    setLayout(layout);

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

SymbolConfigUI::SymbolConfigUI(std::string name, MainWidget* parent, wchar_t* config_location) : ConfigUI(name, parent) {

    symbol_location = config_location;

    //int current_value = static_cast<int>((*config_location - min_value) / (max_value - min_value) * 100);
    wchar_t current_value = *config_location;

    selector = new SelectHighlightTypeUI(this);
    // set the mouse focus on selector
    setFocusPolicy(Qt::StrongFocus);
    setFocus();

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(selector);
    setLayout(layout);

    QObject::connect(selector, &SelectHighlightTypeUI::symbolClicked, [&](int val) {
        //float value = min_value + (static_cast<float>(val) / 100.0f) * (max_value - min_value);
        *symbol_location = (wchar_t)(val + 'a');
        on_change();
        main_widget->invalidate_render();
        main_widget->pop_current_widget();
        });

}


IntConfigUI::IntConfigUI(std::string name, MainWidget* parent, int* config_location, int min_value_, int max_value_) : ConfigUI(name, parent) {

    min_value = min_value_;
    max_value = max_value_;
    int_location = config_location;

    int current_value = static_cast<int>((*config_location - min_value) / (max_value - min_value) * 100);
    slider = new TouchSlider(min_value, max_value, current_value, this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(slider);
    setLayout(layout);

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

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(page_selector);
    setLayout(layout);

    QObject::connect(page_selector, &TouchPageSelector::pageSelected, [&](int val) {
        main_widget->goto_page_with_page_number(val);
        main_widget->invalidate_render();
        });

}

QRect PageSelectorUI::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();

    int w = 2 * parent_width / 3;
    int h = parent_height / 6;
    return QRect((parent_width - w) / 2, parent_height - 2 * h, w, h);
}


BoolConfigUI::BoolConfigUI(std::string name_, MainWidget* parent, bool* config_location, QString qname) : ConfigUI(name_, parent) {
    bool_location = config_location;

    checkbox = new TouchCheckbox(qname, *config_location, this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(checkbox);
    setLayout(layout);

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

QRect BoolConfigUI::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();

    int five_cm = static_cast<int>(12 * logicalDpiX() / 2.54f);

    int w = 2 * parent_width / 3;
    int h = parent_height / 2;

    w = std::min(w, five_cm);
    h = std::min(h, five_cm);

    return QRect((parent_width - w) / 2, (parent_height - h) / 2, w, h);
}

QRect MacroConfigUI::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();

    int w = 2 * parent_width / 3;
    int h = parent_height / 2;
    return QRect(parent_width / 6, parent_height / 4, w, h);
}

QRect FloatConfigUI::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();

    int w = 2 * parent_width / 3;
    int h = parent_height / 2;
    return QRect(parent_width / 6, parent_height / 4, w, h);
}

QRect SymbolConfigUI::get_prefered_rect(QRect parent_rect){
    int w = parent_rect.width();
    int h = parent_rect.height() / 5;

    return QRect(0, (parent_rect.height() - h) / 2, w, h);
}

QRect IntConfigUI::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();

    int w = 2 * parent_width / 3;
    int h = parent_height / 2;
    return QRect(parent_width / 6, parent_height / 4, w, h);
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

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(list_view);
    setLayout(layout);

    QObject::connect(list_view, &TouchListView::itemSelected, [&](QString val, int index) {
        //main_widget->current_widget = nullptr;
        main_widget->pop_current_widget();
        main_widget->on_command_done(val.toStdString(), val.toStdString(), false);
        //deleteLater();
        });

    QObject::connect(list_view, &TouchListView::itemPressAndHold, [&](QString val, int index) {
        main_widget->pop_current_widget();
        main_widget->open_documentation_file_for_name("command", val);
        });
}

QRect TouchCommandSelector::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();
    int w = parent_width * 0.8f;
    int h = parent_height * 0.8f;
    return QRect(parent_width * 0.1f, parent_height * 0.1f, w, h);
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

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(rectangle_select_ui);
    setLayout(layout);


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

QRect RectangleConfigUI::get_prefered_rect(QRect parent_rect){

    return parent_rect;
}

RangeConfigUI::RangeConfigUI(std::string name, MainWidget* parent, float* top_config_location, float* bottom_config_location) : ConfigUI(name, parent) {

    //    range_location = config_location;
    top_location = top_config_location;
    bottom_location = bottom_config_location;

    float current_top = -(*top_location);
    float current_bottom = -(*bottom_location) + 1;


    range_select_ui = new TouchRangeSelectUI(current_top, current_bottom, this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(range_select_ui);
    setLayout(layout);

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

QRect RangeConfigUI::get_prefered_rect(QRect parent_rect){
    return parent_rect;
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


BaseSelectorWidget::BaseSelectorWidget(QAbstractItemView* item_view, bool fuzzy, QAbstractItemModel* item_model, QWidget* parent, MySortFilterProxyModel* custom_proxy_model) : QWidget(parent) {

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
    auto delegate = get_view()->itemDelegate();
    if (delegate) {
        BaseCustomDelegate* custom_delegate = dynamic_cast<BaseCustomDelegate*>(delegate);
        if (custom_delegate) {
            custom_delegate->clear_cache();
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

QRect BaseSelectorWidget::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();

    int offset_x = parent_width * (1 - MENU_SCREEN_WDITH_RATIO) / 2;
    int offset_y = parent_height * (1 - MENU_SCREEN_HEIGHT_RATIO) / 2;
    int width = parent_width * MENU_SCREEN_WDITH_RATIO;
    int height = parent_height * MENU_SCREEN_HEIGHT_RATIO;

    QRect result(offset_x, offset_y, width, height);

    if (!is_initialized) {
        is_initialized = true;
        on_config_file_changed();
    }

    return result;
}

// QRect BaseSelectorWidget::on_parent_resize(QRect parent_rect){
//     QRect pr = get_prefered_rect(parent_rect);
//     move(pr.x(), pr.y());
//     resize(pr.width(), pr.height());

//     if (!is_initialized) {
//         is_initialized = true;
//         on_config_file_changed();
//     }
// }

void BaseSelectorWidget::on_resize(){
    // int parent_width = parentWidget()->width();
    // int parent_height = parentWidget()->height();
    // setFixedSize(parent_width * MENU_SCREEN_WDITH_RATIO, parent_height * MENU_SCREEN_HEIGHT_RATIO);
    // move(parent_width * (1 - MENU_SCREEN_WDITH_RATIO) / 2, parent_height * (1 - MENU_SCREEN_HEIGHT_RATIO) / 2);

    // if (!is_initialized) {
    //     is_initialized = true;
    //     on_config_file_changed();
    // }
}

SelectHighlightTypeUI::SelectHighlightTypeUI(QWidget* parent) :QWidget(parent), parent_widget(parent) {

    QList<QColor> colors;
    const int N_COLORS = 26;
    for (int i = 0; i < N_COLORS; i++) {
        colors.push_back(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * i]));
    }

    new_widget = new QQuickWidget(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(new_widget);
    setLayout(layout);


    new_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    new_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    new_widget->setClearColor(Qt::transparent);

    new_widget->rootContext()->setContextProperty("_colors", QVariant::fromValue(colors));
    new_widget->rootContext()->setContextProperty("_animate", QVariant::fromValue(false));
    new_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchSymbolColorSelector.qml"));

    new_widget->resize(width(), height() / 5);
    new_widget->move(0, height() / 2 - height() / 10);
    MainWidget* main_widget = dynamic_cast<MainWidget*>(parent_widget);

    if (main_widget){
        QObject::connect(new_widget->rootObject(), SIGNAL(colorClicked(int)), main_widget, SLOT(highlight_type_color_clicked(int)));
    }

    QObject::connect(new_widget->rootObject(), SIGNAL(colorClicked(int)), this, SIGNAL(symbolClicked(int)));
}

QRect SelectHighlightTypeUI::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();
    return QRect(0, parent_height / 2 - parent_height / 10, parent_width, parent_height / 5);
}

EnumConfigUI::EnumConfigUI(std::string name, MainWidget* parent, std::vector<std::wstring>& possible_values, int selected_index) : ConfigUI(name, parent) {

    quick_widget = new QQuickWidget(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(quick_widget);
    setLayout(layout);

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

QRect EnumConfigUI::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();
    return QRect(parent_width / 8, parent_height / 6, parent_width * 3 / 4, parent_height * 2 / 3);
}



QString get_view_stylesheet_type_name(QAbstractItemView* view) {
    if (dynamic_cast<QTableView*>(view)) {
        return "QTableView";
    }
    if (dynamic_cast<QListView*>(view)) {
        return "QListView";
    }
    return "";
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



SioyekResizableQWidget::SioyekResizableQWidget(QWidget* parent) : QWidget(parent) {
}

QRect SioyekResizableQWidget::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();
    return QRect(parent_width * 0.05f, 0, parent_width * 0.9f, parent_height);
}

QString FileSelector::get_view_stylesheet_type_name() {
    return "QListView";
}

FileSelector::FileSelector(bool is_fuzzy, std::function<void(std::wstring)> on_done, MainWidget* parent, QString last_path) :
    BaseSelectorWidget(new QListView(), is_fuzzy, nullptr, parent),
    on_done(on_done)
{


    QString root_path;
    QString file_name;

    if (last_path.size() > 0) {
        split_root_file(last_path, root_path, file_name);
        root_path += QDir::separator();
    }

    QStringList dir_contents = get_dir_contents(root_path, "");
    int current_index = -1;
    for (int i = 0; i < dir_contents.size(); i++) {
        if (dir_contents.at(i) == file_name) {
            current_index = i;
            break;
        }
    }

    list_model = new QStringListModel(dir_contents);
    last_root = root_path;
    line_edit->setText(last_root);


    dynamic_cast<QListView*>(get_view())->setModel(list_model);

    if (current_index != -1) {
        dynamic_cast<QListView*>(get_view())->setCurrentIndex(list_model->index(current_index));
    }
    else {
        dynamic_cast<QListView*>(get_view())->setCurrentIndex(list_model->index(0, 0));
    }
    //dynamic_cast<QListView*>(get_view())->setCurrentIndex();
}

bool FileSelector::on_text_change(const QString& text) {
    QString root_path;
    QString partial_name;
    split_root_file(text, root_path, partial_name);

    last_root = root_path;
    if (last_root.size() > 0) {
        if (last_root.at(last_root.size() - 1) == QDir::separator()) {
            last_root.chop(1);
        }
    }

    QStringList match_list = get_dir_contents(root_path, partial_name);
    QStringListModel* new_list_model = new QStringListModel(match_list);
    dynamic_cast<QListView*>(get_view())->setModel(new_list_model);
    delete list_model;
    list_model = new_list_model;
    dynamic_cast<QListView*>(get_view())->setCurrentIndex(list_model->index(0, 0));
    return true;
}

QStringList FileSelector::get_dir_contents(QString root, QString prefix) {

    root = expand_home_dir(root);
    QDir directory(root);
    QStringList res = directory.entryList({ prefix + "*" });
    if (res.size() == 0) {

        bool should_consider_case = false;
        if (!prefix.isLower()) {
            should_consider_case = true;
        }

        std::wstring encoded_prefix = prefix.toStdWString();
        QStringList all_directory_files = directory.entryList();
        std::vector<std::pair<std::wstring, float>> file_scores;

        for (auto file : all_directory_files) {
            std::wstring encoded_file = file.toStdWString();
            std::wstring encoded_file_case_corrected;
            if (should_consider_case) {
                encoded_file_case_corrected = encoded_file;
            }
            else {
                encoded_file_case_corrected = file.toLower().toStdWString();
            }

            float score = similarity_score(encoded_file_case_corrected, encoded_prefix);
            file_scores.push_back(std::make_pair(encoded_file, score));
        }
        std::sort(file_scores.begin(), file_scores.end(), [](std::pair<std::wstring, float> lhs, std::pair<std::wstring, float> rhs) {
            return lhs.second > rhs.second;
        });
        for (auto [file, score] : file_scores) {
            if (score > 50) {
                res.push_back(QString::fromStdWString(file));
            }
        }
    }
    return res;
}

void FileSelector::on_select(QModelIndex index) {
    QString name = list_model->data(index).toString();
    QChar sep = QDir::separator();
    QString full_path = expand_home_dir((last_root.size() > 0) ? (last_root + sep + name) : name);

    if (QFileInfo(full_path).isFile()) {
        on_done(full_path.toStdWString());
        hide();
        parentWidget()->setFocus();
    }
    else {
        line_edit->setText(full_path + sep);
    }
}
