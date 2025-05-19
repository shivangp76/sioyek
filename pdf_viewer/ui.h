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
#include <qabstractitemmodel.h>
#include <qtextdocument.h>
#include <qhash.h>
#include <QQuickWidget>

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




class TouchDelegateListView : public QWidget {
    Q_OBJECT
private:

    std::optional<std::function<void(int)>> on_select = {};
    std::optional<std::function<void(int)>> on_delete = {};

public:
    TouchListView* list_view = nullptr;
    QAbstractTableModel* model = nullptr;

    TouchDelegateListView(QAbstractTableModel* model, bool deletable, QString delegate_name, std::vector<std::pair<QString, QVariant>> props, QWidget* parent);

    // void resizeEvent(QResizeEvent* resize_event) override;
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);

    void set_select_fn(std::function<void(int)>&& fn);
    void set_delete_fn(std::function<void(int)>&& fn);
};




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

    QString get_view_stylesheet_type_name() {
        return "QListView";
    }

    FileSelector(bool is_fuzzy, std::function<void(std::wstring)> on_done, MainWidget* parent, QString last_path) :
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

    virtual bool on_text_change(const QString& text) {
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

    QStringList get_dir_contents(QString root, QString prefix) {

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

    void on_select(QModelIndex index) {
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

class HighlightModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum HighlightModelColumn {
        text = 0,
        type = 1,
        description = 2,
        file_name = 3,
        checksum = 4,
        max_columns = 5,
    };

    std::vector<Highlight> highlights;
    std::vector<QString> documents;
    std::vector<QString> checksums;


    HighlightModel(std::vector<Highlight>&& data, std::vector<QString>&& documents = {}, std::vector<QString>&& checksums = {}, QObject * parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    QHash<int, QByteArray> roleNames() const override;


};

class CommandModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum CommandModelColumn{
        command_name = 0,
        keybind = 1,
        is_pro = 2,
        max_columns = 3
    };

    std::vector<QString> commands;
    std::vector<QStringList> keybinds;
    std::vector<bool> requires_pro;


    CommandModel(std::vector<QString> commands, std::vector<QStringList> keybinds, std::vector<bool> requires_pro);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
};

struct SimilarityScoreResult {
    int score;
    int highlight_begin;
    int highlight_end;
};

class BaseCustomDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    QString pattern;

    mutable std::map<std::tuple<QString, QString, float>, SimilarityScoreResult> cached_similarity_scores;


    virtual void set_pattern(QString p);
    virtual void clear_cache() = 0;
    QString highlight_pattern(QString txt) const;

    int similarity_score_cached(QString haystack, QString needle, int* match_begin, int* match_end, float ratio=0.5f) const;
};

class HighlightSearchItemDelegate : public BaseCustomDelegate {
    Q_OBJECT
public:
    //QString pattern;

    mutable QTextDocument highlight_document;
    mutable QTextDocument file_name_document;
    mutable QTextDocument comment_document;
    mutable std::unordered_map<int, float> cached_sizes;

    HighlightSearchItemDelegate();
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QString get_display_text(const QString& highlight_text, int highlight_type, QString type_text_color="#000000", QString type_label_bg = "#ffffff") const;
    void clear_cache();
    void set_pattern(QString p) override;
};

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


class BaseCustomSelectorWidget : public BaseSelectorWidget {

private:

    mutable std::unordered_map<int, float> cached_sizes;
public:
    std::optional<std::function<void(int)>> select_fn = {};
    std::optional<std::function<void(int)>> delete_fn = {};
    std::optional<std::function<void(int)>> edit_fn = {};
    QListView* lv = nullptr;
    BaseCustomSelectorWidget(
        QAbstractItemView* view,
        QAbstractItemModel* model,
        MainWidget* parent
    );

    void set_select_fn(std::function<void(int)>&& fn);
    void set_delete_fn(std::function<void(int)>&& fn);
    void set_edit_fn(std::function<void(int)>&& fn);
    void resizeEvent(QResizeEvent* resize_event) override;

    void on_select(QModelIndex value) override;
    void on_delete(const QModelIndex& source_index, const QModelIndex& selected_index) override;
    void on_edit(const QModelIndex& source_index, const QModelIndex& selected_index) override;

    void set_selected_index(int index);
    void update_render();
    virtual bool on_text_change(const QString& text) override;

    //virtual void update_render() = 0;
};


class HighlightSelectorWidget : public BaseCustomSelectorWidget{
private:
    HighlightSelectorWidget(
        QAbstractItemView* view,
        QAbstractItemModel* model,
        MainWidget* parent
    );
public:

    static HighlightSelectorWidget* from_highlights(std::vector<Highlight>&& highlights, MainWidget* parent, std::vector<QString>&& doc_names = {}, std::vector<QString>&& doc_checksums = {});

    HighlightModel* highlight_model = nullptr;

    //bool on_text_change(const QString& text) override;
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

class BookmarkModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum BookmarkModelColumn {
        description = 0,
        bookmark = 1,
        file_name = 2,
        checksum = 3,
        max_columns = 4
    };

    std::vector<BookMark> bookmarks;
    std::vector<QString> documents;
    std::vector<QString> checksums;


    BookmarkModel(std::vector<BookMark>&& data, std::vector<QString>&& documents = {}, std::vector<QString>&& checksums = {}, QObject * parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
};

class ItemWithDescriptionModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum ItemWithDescriptionColumn {
        item_text = 0,
        description = 1,
        metadata = 2,
        max_columns = 4
    };

    std::vector<QString> items;
    std::vector<QString> descriptions;
    std::vector<QString> metadatas;


    ItemWithDescriptionModel(std::vector<QString>&& items, std::vector<QString>&& descriptions, std::vector<QString>&& metadata, QObject * parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

};

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

class BookmarkSearchItemDelegate : public BaseCustomDelegate {
    Q_OBJECT
public:
    //QString pattern;

    mutable QTextDocument bookmark_document;
    mutable QTextDocument file_name_document;
    mutable std::unordered_map<int, float> cached_sizes;

    BookmarkSearchItemDelegate();
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    //QString get_display_text(const QString& highlight_text, int highlight_type, QString type_text_color="#000000", QString type_label_bg = "#ffffff") const;
    void clear_cache();
};

class BookmarkSelectorWidget : public BaseCustomSelectorWidget{
private:
    BookmarkSelectorWidget(
        QAbstractItemView* view,
        QAbstractItemModel* model,
        MainWidget* parent
    );
public:

    static BookmarkSelectorWidget* from_bookmarks(std::vector<BookMark>&& bookmarks, MainWidget* parent, std::vector<QString>&& doc_names = {}, std::vector<QString>&& doc_checksums = {});

    BookmarkModel* bookmark_model = nullptr;
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

class DocumentNameModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum DocumentNameColumn {
        file_path = 0,
        document_title = 1,
        last_access_time = 2,
        is_server_only = 3,
        max_columns = 4,
    };

    std::vector<OpenedBookInfo> opened_documents;
    //std::vector<QString> document_titles;
    //std::vector<QDateTime> last_access_times;


    DocumentNameModel(std::vector<OpenedBookInfo>&& books, QObject * parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

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

class FulltextResultModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum SearchResultColumn {
        snippet = 0,
        document_title = 1,
        page = 2,
        max_columns = 3,
    };

    std::vector<FulltextSearchResult> search_results;


    FulltextResultModel(std::vector<FulltextSearchResult>&& results, QObject * parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

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

