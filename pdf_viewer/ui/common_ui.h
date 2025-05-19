#pragma once

#include <QAbstractItemView>
#include <QTableView>
#include <QTreeView>
#include <QHeaderView>
#include <QListView>

#include "main_widget.h"
#include "ui/selector_ui.h"

extern bool TOUCH_MODE;
extern bool SMALL_TOC;

class BaseSelectorWidget : public QWidget {
    Q_OBJECT

protected:
    BaseSelectorWidget(QAbstractItemView* item_view, bool fuzzy, QAbstractItemModel* item_model, MainWidget* parent, MySortFilterProxyModel* custom_proxy_model=nullptr);

    virtual void on_text_changed(const QString& text);




    //QSortFilterProxyModel* proxy_model = nullptr;
    MySortFilterProxyModel* proxy_model = nullptr;
    bool is_fuzzy = false;
    int pressed_row = -1;
    QPoint pressed_pos;
    bool is_initialized = false;

    QAbstractItemView* abstract_item_view;

    virtual void on_select(QModelIndex value) = 0;
    virtual void on_delete(const QModelIndex& source_index, const QModelIndex& selected_index);
    virtual void on_edit(const QModelIndex& source_index, const QModelIndex& selected_index);

    virtual void on_return_no_select(const QString& text);

    // should return true when we want to manually handle text change events
    virtual bool on_text_change(const QString& text);


public:
    QLineEdit* line_edit = nullptr;


    virtual QAbstractItemView* get_view();
    void set_filter_column_index(int index);
    std::optional<QModelIndex> get_selected_index();
    virtual std::wstring get_selected_text();
    bool eventFilter(QObject* obj, QEvent* event) override;
    void simulate_move_down();
    void simulate_move_up();
    void simulate_move_left();
    void simulate_move_right();
    void simulate_page_down();
    void simulate_page_up();
    void simulate_end();
    void simulate_home();
    void simulate_select();
    void handle_delete();
    void handle_edit();
    virtual void on_resize();
    QString get_selected_item();
    void paintEvent(QPaintEvent* paint_event) override;
    void wheelEvent(QWheelEvent* wheel_event) override;


#ifndef SIOYEK_QT6
    void keyReleaseEvent(QKeyEvent* event) override;
#endif

    virtual void on_config_file_changed();
    // void resizeEvent(QResizeEvent* resize_event) override;
    Q_INVOKABLE virtual QRect get_prefered_rect(QRect parent_rect);
    // virtual QRect on_parent_resize(QRect parent_rect);

};

template<typename T>
class FilteredSelectTableWindowClass : public BaseSelectorWidget {
private:

    QStringListModel* string_list_model = nullptr;
    std::vector<T> values;
    std::vector<std::wstring> item_strings;
    std::function<void(T*)> on_done = nullptr;
    std::function<void(T*)> on_delete_function = nullptr;
    std::function<void(T*)> on_edit_function = nullptr;
    int n_cols = 1;

protected:

public:

    QString get_view_stylesheet_type_name() {
        return "QTableView";
    }

    FilteredSelectTableWindowClass(
        bool fuzzy,
        bool multiline,
        const std::vector<std::vector<std::wstring>>& table,
        std::vector<T> values,
        int selected_index,
        std::function<void(T*)> on_done,
        MainWidget* parent,
        std::function<void(T*)> on_delete_function = nullptr) : BaseSelectorWidget(new QTableView(), fuzzy, nullptr, parent),
        values(values),
        on_done(on_done),
        on_delete_function(on_delete_function)
    {
        item_strings = table[0];
        QVector<QString> q_string_list;
        for (const auto& s : table[0]) {
            q_string_list.push_back(QString::fromStdWString(s));

        }


        QAbstractItemModel* model = create_table_model(table);
        //model->setItem(selected_index, 0, new QStandardItem(QString::fromStdWString(std_string_list[selected_index])));
        //QStandardItemModel* model = new QStandardItemModel();

        //for (size_t i = 0; i < std_string_list.size(); i++) {
        //	QStandardItem* name_item = new QStandardItem(QString::fromStdWString(std_string_list[i]));
        //	QStandardItem* key_item = new QStandardItem(QString::fromStdWString(std_string_list_right[i]));
        //	key_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        //	model->appendRow(QList<QStandardItem*>() << name_item << key_item);
        //}

        this->proxy_model->setSourceModel(model);

        QTableView* table_view = dynamic_cast<QTableView*>(this->get_view());
        //        QScroller::grabGesture(table_view, QScroller::LeftMouseButtonGesture);
        //        table_view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        //        table_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);


        if (selected_index != -1) {
            table_view->selectionModel()->setCurrentIndex(model->index(selected_index, 0), QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
        }
        else {
            table_view->selectionModel()->setCurrentIndex(model->index(0, 0), QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
        }

        table_view->setSelectionMode(QAbstractItemView::SingleSelection);
        table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
        table_view->setEditTriggers(QAbstractItemView::NoEditTriggers);

        if (table[0].size() > 0) {
            n_cols = table.size();
            table_view->horizontalHeader()->setStretchLastSection(false);
            for (int i = 0; i < table.size() - 1; i++) {
                table_view->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
            }

            table_view->horizontalHeader()->setSectionResizeMode(table.size() - 1, QHeaderView::ResizeToContents);
        }

        table_view->horizontalHeader()->hide();
        table_view->verticalHeader()->hide();

        if (multiline) {
            table_view->setWordWrap(true);
            table_view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        }

        if (selected_index != -1) {
            table_view->scrollTo(this->proxy_model->mapFromSource(table_view->currentIndex()), QAbstractItemView::EnsureVisible);
        }
    }

    void set_stretch_column_index(int index) {
        if (item_strings.size() > 0) {
            QTableView* table_view = dynamic_cast<QTableView*>(get_view());
            for (int i = 0; i < n_cols; i++) {
                if (i == index) {
                    table_view->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
                }
                else {
                    table_view->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
                }
            }
        }
    }

    void set_horizontal_header(QStringList header_titles) {

        QTableView* table_view = dynamic_cast<QTableView*>(this->get_view());
        QStandardItemModel* model = dynamic_cast<QStandardItemModel*>(proxy_model->sourceModel());
        model->setHorizontalHeaderLabels(header_titles);

        table_view->horizontalHeader()->show();

    }

    void set_equal_columns() {
        if (item_strings.size() > 0) {
            QTableView* table_view = dynamic_cast<QTableView*>(get_view());
            for (int i = 0; i < n_cols; i++) {
                table_view->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
            }
        }
    }

    void set_on_edit_function(std::function<void(T*)> edit_func) {
        on_edit_function = edit_func;
    }

    void set_value_second_item(T value, QString str) {

        for (int i = 0; i < values.size(); i++) {
            if (values[i] == value) {
                auto source_model = this->proxy_model->sourceModel();
                source_model->setData(source_model->index(i, 1), str);
                return;
            }
        }

    }

    virtual std::wstring get_selected_text() {
        auto index = this->get_selected_index();

        if (index) {

            auto source_index = this->proxy_model->mapToSource(index.value());
            return item_strings[source_index.row()];
        }

        return L"";
    }

    virtual void on_delete(const QModelIndex& source_index, const QModelIndex& selected_index) override {
        if (on_delete_function) {
            on_delete_function(&values[source_index.row()]);
            this->proxy_model->removeRow(selected_index.row());
            values.erase(values.begin() + source_index.row());
        }
    }

    virtual void on_edit(const QModelIndex& source_index, const QModelIndex& selected_index) override {
        if (on_edit_function) {
            on_edit_function(&values[source_index.row()]);
        }
    }

    void on_select(QModelIndex index) {
        this->hide();
        this->parentWidget()->setFocus();
        auto source_index = this->proxy_model->mapToSource(index);
        on_done(&values[source_index.row()]);
    }
};

template<typename T>
class FilteredSelectWindowClass : public BaseSelectorWidget {
private:

    QStringListModel* string_list_model = nullptr;
    std::vector<T> values;
    std::function<void(T*)> on_done = nullptr;
    std::function<void(T*)> on_delete_function = nullptr;

protected:

public:

    QString get_view_stylesheet_type_name() {
        return "QListView";
    }


    FilteredSelectWindowClass(bool fuzzy, std::vector<std::wstring> std_string_list,
        std::vector<T> values,
        std::function<void(T*)> on_done,
        MainWidget* parent,
        std::function<void(T*)> on_delete_function = nullptr, int selected_index = -1) : BaseSelectorWidget(new QListView(), fuzzy, nullptr, parent),
        values(values),
        on_done(on_done),
        on_delete_function(on_delete_function)
    {
        QVector<QString> q_string_list;
        for (const auto& s : std_string_list) {
            q_string_list.push_back(QString::fromStdWString(s));

        }
        QStringList string_list = QStringList::fromVector(q_string_list);


        string_list_model = new QStringListModel(string_list);
        this->proxy_model->setSourceModel(string_list_model);

        if (selected_index != -1) {
            dynamic_cast<QListView*>(this->get_view())->selectionModel()->setCurrentIndex(string_list_model->index(selected_index), QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
        }
        else {
            dynamic_cast<QListView*>(this->get_view())->selectionModel()->setCurrentIndex(string_list_model->index(0, 0), QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
        }

    }

    virtual void on_delete(const QModelIndex& source_index, const QModelIndex& selected_index) override {
        if (on_delete_function) {
            on_delete_function(&values[source_index.row()]);
            this->proxy_model->removeRow(selected_index.row());
            values.erase(values.begin() + source_index.row());
        }
    }

    void on_select(QModelIndex index) {
        this->hide();
        this->parentWidget()->setFocus();
        auto source_index = this->proxy_model->mapToSource(index);
        on_done(&values[source_index.row()]);
    }
};

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

template<typename T>
class FilteredTreeSelect : public BaseSelectorWidget {
private:
    std::function<void(const std::vector<int>&)> on_done;

protected:

public:

    QString get_view_stylesheet_type_name() {
        return "QTreeView";
    }

    FilteredTreeSelect(bool fuzzy, QAbstractItemModel* item_model,
        std::function<void(const std::vector<int>&)> on_done,
        MainWidget* parent,
        std::vector<int> selected_index) : BaseSelectorWidget(new QTreeView(), fuzzy, item_model, parent),
        on_done(on_done)
    {
        auto index = QModelIndex();
        for (auto i : selected_index) {
            index = this->proxy_model->index(i, 0, index);
        }

        QTreeView* tree_view = dynamic_cast<QTreeView*>(this->get_view());
        if (SMALL_TOC) {
            tree_view->collapseAll();
            tree_view->expand(index);
        }

        tree_view->header()->setStretchLastSection(false);
        tree_view->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        tree_view->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        tree_view->setCurrentIndex(index);

    }

    void on_select(QModelIndex index) {
        this->hide();
        this->parentWidget()->setFocus();
        auto source_index = this->proxy_model->mapToSource(index);
        std::vector<int> indices;
        while (source_index != QModelIndex()) {
            indices.push_back(source_index.row());
            source_index = source_index.parent();
        }
        on_done(indices);
    }
};

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
