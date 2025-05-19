#include "ui/common_ui.h"
#include "ui/base_delegate.h"

QRect TouchDelegateListView::get_prefered_rect(QRect parent_rect){
    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();

    return QRect(parent_width * 0.05f, 0, parent_width * 0.9f, parent_height);

}

void TouchDelegateListView::set_select_fn(std::function<void(int)>&& fn) {
    on_select = fn;
}

void TouchDelegateListView::set_delete_fn(std::function<void(int)>&& fn) {
    on_delete = fn;
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

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(list_view);
    setLayout(layout);

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

void BaseCustomSelectorWidget::update_render() {
    emit lv->model()->dataChanged(lv->model()->index(0, 0), lv->model()->index(lv->model()->rowCount() - 1, 0));
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
