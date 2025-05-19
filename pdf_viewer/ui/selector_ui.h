#pragma once

#include <QWidget>
#include <QVBoxLayout>

#include "mysortfilterproxymodel.h"
#include "touchui/TouchListView.h"

class SioyekResizableQWidget : public QWidget{
    Q_OBJECT
public:
    SioyekResizableQWidget(QWidget* parent);
    Q_INVOKABLE QRect get_prefered_rect(QRect parent_rect);
};

template <typename T>
class TouchFilteredSelectWidget : public SioyekResizableQWidget {
private:
    //    QStringListModel string_list_model;
    //    MySortFilterProxyModel proxy_model;
    TouchListView* list_view = nullptr;
    QWidget* parent_widget;
    std::vector<T> values;
    std::function<void(T*)> on_done;
    std::function<void(T*)> on_delete_function = nullptr;

public:
    //	void set_selected_index(int index) {
    //		list_view->set_selected_index(index);
    //	}
    void initialize() {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->addWidget(list_view);
        setLayout(layout);

        QObject::connect(list_view, &TouchListView::itemSelected, [&](QString name, int index) {
            on_done(&values[index]);
            deleteLater();
            });

        QObject::connect(list_view, &TouchListView::itemDeleted, [&](QString name, int index) {
            on_delete_function(&values[index]);
            //deleteLater();
            });
    }
    TouchFilteredSelectWidget(bool is_fuzzy, std::vector<std::wstring> std_string_list,
        std::vector<T> values_,
        int selected_index,
        std::function<void(T*)> on_done_,
        std::function<void(T*)> on_delete,
        QWidget* parent) :
        SioyekResizableQWidget(parent),
        values(values_),
        on_done(on_done_),
        on_delete_function(on_delete) {

        parent_widget = parent;
        QStringList string_list;
        for (auto s : std_string_list) {
            string_list.append(QString::fromStdWString(s));
        }
        //        string_list_model.setStringList(string_list);
        //        proxy_model.setSourceModel(string_list_model);

        list_view = new TouchListView(is_fuzzy, string_list, selected_index, this, true);
        initialize();
    }

    TouchFilteredSelectWidget(bool is_fuzzy, QAbstractItemModel* model,
        int selected_index,
        std::function<void(T*)> on_done_,
        QWidget* parent) :
        SioyekResizableQWidget(parent),
        on_done(on_done_) {
        parent_widget = parent;
        list_view = new TouchListView(is_fuzzy, model, selected_index, this, false, false, "TouchTreeView");

        QObject::connect(list_view, &TouchListView::itemSelected, [&](QString name, int index) {
            on_done(&values[index]);
            deleteLater();
            });
    }

    TouchFilteredSelectWidget(bool is_fuzzy, QAbstractItemModel* model,
        std::vector<T> values_,
        int selected_index,
        std::function<void(T*)> on_done_,
        std::function<void(T*)> on_delete,
        QWidget* parent) :
        SioyekResizableQWidget(parent),
        values(values_),
        on_done(on_done_),
        on_delete_function(on_delete) {

        parent_widget = parent;
        //        string_list_model.setStringList(string_list);
        //        proxy_model.setSourceModel(string_list_model);

        list_view = new TouchListView(is_fuzzy, model, selected_index, this, true);

        initialize();
    }


    void set_filter_column_index(int index) {
        list_view->proxy_model->setFilterKeyColumn(index);
    }

    void set_value_second_item(T value, QString str) {

        auto source_model = list_view->proxy_model->sourceModel();

        if (source_model->columnCount() < 2) return;

        for (int i = 0; i < values.size(); i++) {
            if (values[i] == value) {
                source_model->setData(source_model->index(i, 1), str);
                list_view->update_model();
                return;
            }
        }

    }
};
