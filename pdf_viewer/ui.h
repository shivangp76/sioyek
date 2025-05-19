#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <optional>
#include <unordered_map>
#include <qtextbrowser.h>

#include <QListWidget>
#include <QScroller>
#include <qsizepolicy.h>
#include <qapplication.h>
#include <qqmlengine.h>
#include <qpushbutton.h>
#include <qopenglwidget.h>
#include <qopenglextrafunctions.h>
#include <qopenglfunctions.h>
#include <qopengl.h>
#include <qwindow.h>
#include <QKeyEvent>
#include <qinputmethod.h>
#include <qlineedit.h>
#include <qtreeview.h>
#include <qsortfilterproxymodel.h>
#include <qabstractitemmodel.h>
#include <qopenglshaderprogram.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qstackedwidget.h>
#include <qboxlayout.h>
#include <qlistview.h>
#include <qtableview.h>
#include <qstringlistmodel.h>
#include <qpalette.h>
#include <qstandarditemmodel.h>
#include <qfilesystemmodel.h>
#include <qheaderview.h>
#include <qcolordialog.h>
#include <qslider.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qstyleditemdelegate.h>
#include <qtextdocument.h>
#include <qhash.h>
#include <QQuickWidget>

#include "ui/base_delegate.h"
#include "ui/ui_models.h"
#include "ui/common_ui.h"
#include "touchui/ConfigUI.h"
#include "touchui/TouchSlider.h"
#include "touchui/TouchCheckbox.h"
#include "touchui/TouchListView.h"
#include "touchui/TouchCopyOptions.h"
#include "touchui/TouchRectangleSelectUI.h"
#include "touchui/TouchRangeSelectUI.h"
#include "touchui/TouchPageSelector.h"
#include "touchui/TouchMainMenu.h"
#include "touchui/TouchTextEdit.h"
#include "touchui/TouchSearchButtons.h"
#include "touchui/TouchDeleteButton.h"
#include "touchui/TouchHighlightButtons.h"
#include "touchui/TouchAudioButtons.h"
#include "touchui/TouchDrawControls.h"
#include "touchui/TouchMacroEditor.h"
#include "touchui/TouchGenericButtons.h"
#include "touchui/TouchChat.h"

#include "mysortfilterproxymodel.h"
#include "rapidfuzz_amalgamated.hpp"

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "fts_fuzzy_match.h"
#undef FTS_FUZZY_MATCH_IMPLEMENTATION


#include "utils.h"
#include "config.h"

class DatabaseManager;
class MainWidget;
extern bool SMALL_TOC;
extern bool TOUCH_MODE;

class HierarchialSortFilterProxyModel : public QSortFilterProxyModel {
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
    //public:
    //	mutable int count = 0;
};

class ConfigFileChangeListener {
    static std::vector<ConfigFileChangeListener*> registered_listeners;


public:
    ConfigFileChangeListener();
    ~ConfigFileChangeListener();
    virtual void on_config_file_changed(ConfigManager* new_config_manager) = 0;
    static void notify_config_file_changed(ConfigManager* new_config_manager);
};


class MyLineEdit: public QLineEdit {
    Q_OBJECT

public:
    MainWidget* main_widget;
    MyLineEdit(MainWidget* parent);

    void keyPressEvent(QKeyEvent* event) override;
    int get_next_word_position();
    int get_prev_word_position();
    bool is_autocomplete_active = false;
    void set_autocomplete_strings(QStringList strings);

private:
    QListWidget* autocomplete_popup = nullptr;
    QString current_autocomplete_prefix;
    QStringList autocomplete_strings;
    int autocomplete_start_pos = -1;

    void show_autocomplete_popup();
    void hide_autocomplete_popup();
    void update_autocomplete_popup();
    void insert_autocomplete_suggestion(const QString& suggestion);
    QStringList get_autocomplete_suggestions(const QString& prefix);
};

QString get_view_stylesheet_type_name(QAbstractItemView* view);





class TouchCommandSelector : public QWidget {
    Q_OBJECT
public:
    TouchCommandSelector(bool is_fuzzy, const QStringList& commands, MainWidget* mw);
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
    // void resizeEvent(QResizeEvent* resize_event) override;
    //    void keyReleaseEvent(QKeyEvent* key_event) override;

private:
    MainWidget* main_widget;
    TouchListView* list_view;

};

//class FileSelector : public BaseSelectorWidget<std::wstring, QListView, QSortFilterProxyModel> {
class FileSelector : public BaseSelectorWidget {
private:

    QStringListModel* list_model = nullptr;
    std::function<void(std::wstring)> on_done = nullptr;
    QString last_root = "";

protected:

public:

    QString get_view_stylesheet_type_name();
    FileSelector(bool is_fuzzy, std::function<void(std::wstring)> on_done, MainWidget* parent, QString last_path);
    virtual bool on_text_change(const QString& text);
    QStringList get_dir_contents(QString root, QString prefix);
    void on_select(QModelIndex index);
};

class AndroidSelector : public QWidget {
    Q_OBJECT
public:

    AndroidSelector(QWidget* parent);
    // void resizeEvent(QResizeEvent* resize_event) override;

    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
    void update_context_properties();
    //    bool event(QEvent *event) override;
private:
    //    QVBoxLayout* layout;
    //    QPushButton* fullscreen_button;
    //    QPushButton* select_text_button;
    //    QPushButton* open_document_button;
    //    QPushButton* open_prev_document_button;
    //    QPushButton* command_button;
    //    QPushButton* visual_mode_button;
    //    QPushButton* search_button;
    //    QPushButton* set_background_color;
    //    QPushButton* set_dark_mode_contrast;
    //    QPushButton* set_ruler_mode;
    //    QPushButton* restore_default_config_button;
    //    QPushButton* toggle_dark_mode_button;
    //    QPushButton* ruler_mode_bounds_config_button;
    //    QPushButton* goto_page_button;
    //    QPushButton* set_rect_config_button;
    //    QPushButton* test_rectangle_select_ui;
    TouchMainMenu* main_menu;


    MainWidget* main_widget;

};

//class TextSelectionButtons : public QWidget{
//public:
//    TextSelectionButtons(MainWidget* parent);
//    void resizeEvent(QResizeEvent* resize_event) override;
//private:

//    MainWidget* main_widget;
//    QHBoxLayout* layout;
//    QPushButton* copy_button;
//    QPushButton* search_in_scholar_button;
//    QPushButton* search_in_google_button;
//    QPushButton* highlight_button;

//};


class DrawControlsUI : public QWidget {
    Q_OBJECT
public:
    DrawControlsUI(MainWidget* parent);
    // void resizeEvent(QResizeEvent* resize_event) override;
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
    TouchDrawControls* controls_ui;
private:
    MainWidget* main_widget;

};

class TouchTextSelectionButtons : public QWidget {
    Q_OBJECT
public:
    TouchTextSelectionButtons(MainWidget* parent);
    // void resizeEvent(QResizeEvent* resize_event) override;
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
private:
    MainWidget* main_widget;

    TouchCopyOptions* buttons_ui;
};

class HighlightButtons : public QWidget {
    Q_OBJECT

public:
    HighlightButtons(MainWidget* parent);
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
    TouchHighlightButtons* highlight_buttons;
private:

    MainWidget* main_widget;
    //TouchDeleteButton* delete_button;
    //QQuickWidget* buttons_widget;
    //QHBoxLayout* layout;
    //QPushButton* delete_highlight_button;
};

class SearchButtons : public QWidget {

    Q_OBJECT
public:
    SearchButtons(MainWidget* parent);
    // void resizeEvent(QResizeEvent* resize_event) override;
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
private:

    MainWidget* main_widget;
    TouchSearchButtons* buttons_widget;
    //QHBoxLayout* layout;
    //QPushButton* prev_match_button;
    //QPushButton* next_match_button;
    //QPushButton* goto_initial_location_button;
};

class Color3ConfigUI : public ConfigUI {
    Q_OBJECT
public:
    Color3ConfigUI(std::string name, MainWidget* parent, float* config_location_);
    // void resizeEvent(QResizeEvent* resize_event) override;
    // Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);

private:
    float* color_location;
    QColorDialog* color_picker;
    //    QQuickWidget* color_picker;

};

class Color4ConfigUI : public ConfigUI {
public:
    Color4ConfigUI(std::string name, MainWidget* parent, float* config_location_);

private:
    float* color_location;
    QColorDialog* color_picker;

};

class BoolConfigUI : public ConfigUI {
    Q_OBJECT
public:
    BoolConfigUI(std::string name, MainWidget* parent, bool* config_location, QString name_);
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
    // void resizeEvent(QResizeEvent* resize_event) override;
private:

    bool* bool_location;
    TouchCheckbox* checkbox;
    //   	TouchCh
    //    QHBoxLayout* layout;
    //    QCheckBox* checkbox;
    //    QLabel* label;

};


//class TextConfigUI : public ConfigUI{
//public:
//	TextConfigUI(MainWidget* parent, std::wstring* config_location);
//    void resizeEvent(QResizeEvent* resize_event) override;
//private:
//    std::wstring* float_location;
//    TouchTextEdit* text_edit = nullptr;
//};

class FloatConfigUI : public ConfigUI {
    Q_OBJECT
public:
    FloatConfigUI(std::string name, MainWidget* parent, float* config_location, float min_value, float max_value);
    // void resizeEvent(QResizeEvent* resize_event) override;
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
private:
    float* float_location;
    float min_value;
    float max_value;
    TouchSlider* slider = nullptr;
};

class SelectHighlightTypeUI;

class SymbolConfigUI : public ConfigUI {
    Q_OBJECT
public:
    SymbolConfigUI(std::string name, MainWidget* parent, wchar_t* config_location);
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
    // void resizeEvent(QResizeEvent* resize_event) override;
private:
    wchar_t* symbol_location;
    SelectHighlightTypeUI* selector = nullptr;
};

class EnumConfigUI : public ConfigUI {
    Q_OBJECT
public:
    EnumConfigUI(std::string name, MainWidget* parent, std::vector<std::wstring>& possible_values, int selected_index);
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
    // void resizeEvent(QResizeEvent* resize_event) override;
private:
    QQuickWidget* quick_widget = nullptr;

public slots:
    void on_select(QString name, int index);
};

class MacroConfigUI : public ConfigUI {
    Q_OBJECT
public:
    MacroConfigUI(std::string name, MainWidget* parent, std::wstring* config_location, std::wstring initial_macro);
    // void resizeEvent(QResizeEvent* resize_event) override;
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
private:
    TouchMacroEditor* macro_editor = nullptr;
};

class IntConfigUI : public ConfigUI {
    Q_OBJECT
public:
    IntConfigUI(std::string name, MainWidget* parent, int* config_location, int min_value, int max_value);
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
    // void resizeEvent(QResizeEvent* resize_event) override;
private:
    int* int_location;
    int min_value;
    int max_value;
    TouchSlider* slider = nullptr;
};

class PageSelectorUI : public ConfigUI {
    Q_OBJECT
public:
    PageSelectorUI(MainWidget* parent, int current, int num_pages);
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
    // void resizeEvent(QResizeEvent* resize_event) override;
private:
    TouchPageSelector* page_selector = nullptr;
};


class RectangleConfigUI : public ConfigUI {
    Q_OBJECT
public:
    RectangleConfigUI(std::string name, MainWidget* parent, UIRect* config_location);
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);

    // void resizeEvent(QResizeEvent* resize_event) override;
private:
    UIRect* rect_location;

    TouchRectangleSelectUI* rectangle_select_ui = nullptr;
};

class RangeConfigUI : public ConfigUI {
public:
    RangeConfigUI(std::string name, MainWidget* parent, float* top_location, float* bottom_location);

    // void resizeEvent(QResizeEvent* resize_event) override;
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
private:
    float* top_location;
    float* bottom_location;

    TouchRangeSelectUI* range_select_ui = nullptr;
};

class SelectHighlightTypeUI : public QWidget {
    Q_OBJECT
public:
    SelectHighlightTypeUI(QWidget* parent);
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);

    // void resizeEvent(QResizeEvent* resize_event) override;

signals:
    void symbolClicked(int);

private:
    QWidget* parent_widget;
    QQuickWidget* new_widget;

};

std::wstring select_document_file_name(std::optional<QString> root_dir={});
std::wstring select_json_file_name(std::optional<QString> root_dir={});
std::wstring select_any_file_name(std::optional<QString> root_dir={});
std::wstring select_command_file_name(std::string command_name, std::optional<QString> root_dir={});
std::wstring select_new_json_file_name(std::optional<QString> root_dir={});
std::wstring select_new_pdf_file_name(std::optional<QString> root_dir={});
std::wstring select_command_folder_name(std::optional<QString> root_dir={});
std::wstring select_any_existing_file_name(std::optional<QString> root_dir={});

//QWidget* color3_configurator_ui(MainWidget* main_widget, void* location);
//QWidget* color4_configurator_ui(MainWidget* main_widget, void* location);

//template<float min_value, float max_value>
//QWidget* float_configurator_ui(MainWidget* main_widget, void* location){
//    return new FloatConfigUI(main_widget, (float*)location, min_value, max_value);
//}

//template<QString name>
//QWidget* bool_configurator_ui(MainWidget* main_widget, void* location){
//    return new BoolConfigUI(main_widget, (float*)location, name);
//}





class CommandItemDelegate : public BaseCustomDelegate {
    Q_OBJECT
public:
    QString ignore_prefix;

    mutable QTextDocument command_name_document;
    mutable QTextDocument keybind_document;
    mutable std::optional<float> cached_size = {};

    CommandItemDelegate();
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void clear_cache();
    void set_ignore_prefix(QString p);
};





class CommandSelectorWidget : public BaseCustomSelectorWidget{
private:
    //CommandSelectorWidget(
    //    QAbstractItemView* view,
    //    QAbstractItemModel* model,
    //    MainWidget* parent
    //);

    MainWidget* w;
    CommandSelectorWidget(
        QAbstractItemView* view,
        std::unordered_map<QString, QAbstractItemModel*> prefix_model,
        QAbstractItemModel* configs_model,
        QAbstractItemModel* color_configs_model,
        QAbstractItemModel* bool_configs_model,
        MainWidget* parent
    );
    std::vector<QString> special_prefixes;
    bool is_config_mode_ = false;
    QString config_prefix = "";

public:

    std::unordered_map<QString, QAbstractItemModel*> prefix_command_model;
    QAbstractItemModel* configs_model;
    QAbstractItemModel* color_configs_model;
    QAbstractItemModel* bool_configs_model;
    QString last_prefix = "";

    QString get_command_with_index(int index);
    static CommandSelectorWidget* from_commands(
        std::vector<QString> commands,
        std::vector<QString> configs,
        std::vector<QString> color_configs,
        std::vector<QString> bool_configs,
        std::vector<QStringList> keybinds,
        std::vector<bool> requires_pro,
        MainWidget* parent
    );
    bool on_text_change(const QString& text) override;
    void on_text_changed(const QString& text) override;
    bool is_config_mode();
    QString get_config_prefix();


    //void update_render() override;
    //bool on_text_change(const QString& text) override;
};

QString translate_command_search_string(QString raw_search_string);



class ItemWithDescriptionDelegate : public BaseCustomDelegate {
    Q_OBJECT
public:
    //QString pattern;

    mutable QTextDocument item_document;
    mutable QTextDocument description_document;
    mutable std::unordered_map<int, float> cached_sizes;

    ItemWithDescriptionDelegate();
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void clear_cache();
};


class ItemWithDescriptionSelectorWidget : public BaseCustomSelectorWidget{
private:
    ItemWithDescriptionSelectorWidget(
        QAbstractItemView* view,
        QAbstractItemModel* model,
        MainWidget* parent
    );
public:

    static ItemWithDescriptionSelectorWidget* from_items(std::vector<QString>&& items, std::vector<QString>&& descriptions, std::vector<QString>&& metadata, MainWidget* parent);

    ItemWithDescriptionModel* item_model = nullptr;
};


class DocumentItemDelegate : public BaseCustomDelegate {
    Q_OBJECT
public:
    //QString pattern;

    mutable QTextDocument path_document;
    mutable QTextDocument title_document;
    mutable QTextDocument last_access_time_document;
    mutable std::unordered_map<int, float> cached_sizes;

    DocumentItemDelegate();
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void clear_cache();
    QString get_time_string(QDateTime time) const;
};

class SearchItemDelegate : public BaseCustomDelegate {
    Q_OBJECT
public:
    //QString pattern;

    mutable QTextDocument snippet_document;
    mutable QTextDocument location_document;
    //mutable std::unordered_map<int, float> cached_sizes;

    SearchItemDelegate();
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void clear_cache();
    QString highlight_match(QString match) const;
    QString get_location_string(const QModelIndex& index) const;
};

class DocumentSelectorWidget : public BaseCustomSelectorWidget{
private:
    DocumentSelectorWidget(
        QAbstractItemView* view,
        QAbstractItemModel* model,
        MainWidget* parent
    );
public:

    static DocumentSelectorWidget* from_documents(std::vector<OpenedBookInfo>&& opened_documents, MainWidget* parent);

    DocumentNameModel* document_model = nullptr;
};


class FulltextSearchWidget : public BaseCustomSelectorWidget{
public:
    DatabaseManager* db_manager = nullptr;
    MainWidget* main_widget = nullptr;
    std::wstring maybe_file_checksum = L"";
    std::wstring maybe_tag = L"";
    FulltextSearchWidget(
        DatabaseManager* manager,
        QAbstractItemView* view,
        QAbstractItemModel* model,
        MainWidget* parent,
        std::wstring checksum=L"",
        std::wstring tag=L""
    );
    ~FulltextSearchWidget();

    static FulltextSearchWidget* create(MainWidget* parent, std::wstring checksum=L"", std::wstring tag=L"");

    virtual void on_text_changed(const QString& text) override;
    virtual void on_select(QModelIndex value) override;
    virtual void on_delete(const QModelIndex& source_index, const QModelIndex& selected_index) override;

    //QStringListModel* result_model = nullptr;
    FulltextResultModel* result_model = nullptr;
};

class DocumentationSearchWidget : public FulltextSearchWidget {
public:
    DocumentationSearchWidget(
        DatabaseManager* manager,
        QAbstractItemView* view,
        QAbstractItemModel* model,
        MainWidget* parent
    );

    static DocumentationSearchWidget* create(MainWidget* parent);
    virtual void on_text_changed(const QString& text) override;
};


class SelectionIndicator : public QWidget {
    // touch mode text selection indicator widgets
private:
    bool is_dragging = false;
    bool is_begin;
    MainWidget* main_widget;

    QIcon begin_icon;
    QIcon end_icon;
    QPoint last_press_window_pos;
    QPoint last_press_widget_pos;
    DocumentPos docpos;
    bool docpos_needs_recompute = false;
public:

    SelectionIndicator(QWidget* parent, bool begin, MainWidget* w, AbsoluteDocumentPos pos);

    void update_pos();
    void mousePressEvent(QMouseEvent* mevent);
    void mouseMoveEvent(QMouseEvent* mouse_event);
    void mouseReleaseEvent(QMouseEvent* mevent);
    DocumentPos get_docpos();
    void paintEvent(QPaintEvent* event);
};

class SioyekDocumentationTextBrowser : public QTextBrowser {
private:
    MainWidget* main_widget = nullptr;
protected:
    void wheelEvent(QWheelEvent* wevent) override;
public:
    SioyekDocumentationTextBrowser(MainWidget* parent);

    void mousePressEvent(QMouseEvent* mevent) override;
    void mouseReleaseEvent(QMouseEvent* mevent) override;
    void backward() override;
    void forward() override;
    void doSetSource(const QUrl& url, QTextDocument::ResourceType type = QTextDocument::UnknownResource) override;
};




// class sioyektouchbookmarktextbrowser : public qwidget {
//     Q_OBJECT
// private:
//     MainWidget* main_widget = nullptr;

// public:
//     QString bookmark_uuid;
//     QVBoxLayout* layout = nullptr;
//     TouchChat* text_browser = nullptr;

//     SioyekTouchBookmarkTextBrowser(MainWidget* parent, QString bookmark_uuid, QString content, bool chat);

//     Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
//     void update_text(QString new_text);

//     // void set_follow_output(bool val);

//     // void scroll_amount(int amount);
//     // void scroll_to_start();
//     // void scroll_to_end();
//     // void set_pending(bool pending);
//     // ~SioyekBookmarkTextBrowser();

// };

