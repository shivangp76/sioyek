#include "ui/common_ui.h"
#include "ui/base_delegate.h"
#include "commands/base_commands.h"
#include "main_widget.h"

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

void MyLineEdit::show_autocomplete_popup() {
    if (autocomplete_popup->count() == 0) return;

    is_autocomplete_active = true;

    // Calculate position for popup (below current cursor)
    QRect cursor_rect = this->cursorRect();
    QPoint global_pos = this->mapToGlobal(cursor_rect.bottomLeft());

    // Set appropriate size
    //autocompletePopup->setMinimumWidth(width() / 2);
    //autocompletePopup->adjustSize();

    // Ensure popup stays within screen bounds
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screen_geometry = screen->availableGeometry();
        QSize popup_size = autocomplete_popup->size();

        if (global_pos.x() + popup_size.width() > screen_geometry.right()) {
            global_pos.setX(screen_geometry.right() - popup_size.width());
        }

        if (global_pos.y() + popup_size.height() > screen_geometry.bottom()) {
            // Move popup above text edit if it would go off bottom of screen
            global_pos.setY(mapToGlobal(cursor_rect.topLeft()).y() - popup_size.height());
        }
    }

    autocomplete_popup->move(global_pos);
    autocomplete_popup->show();

    // Select first item
    if (autocomplete_popup->count() > 0) {
        autocomplete_popup->setCurrentRow(0);
    }
}

void MyLineEdit::hide_autocomplete_popup() {
    is_autocomplete_active = false;
    autocomplete_popup->hide();
    autocomplete_popup->clear();
    autocomplete_start_pos = -1;
    setFocus();
}

void MyLineEdit::update_autocomplete_popup() {
    if (!hasFocus()) {
        hide_autocomplete_popup();
        return;
    }

    int cursor_pos = cursorPosition();
    QString text = this->text();

    // Find the start position of current word
    int start = cursor_pos;
    while (start > 0 && !text[start-1].isSpace()) {
        start--;
    }

    if (cursor_pos > start) {
        QString prefix = text.mid(start, cursor_pos - start);

        // Only update if prefix has changed or position changed
        if (prefix != current_autocomplete_prefix || start != autocomplete_start_pos) {
            autocomplete_start_pos = start;
            current_autocomplete_prefix = prefix;

            QStringList suggestions = get_autocomplete_suggestions(prefix);

            autocomplete_popup->clear();
            if (suggestions.isEmpty()) {
                hide_autocomplete_popup();
                return;
            }

            autocomplete_popup->addItems(suggestions);
            show_autocomplete_popup();
        }
    } else {
        hide_autocomplete_popup();
    }
    setFocus();
}

void MyLineEdit::insert_autocomplete_suggestion(const QString& suggestion) {
    if (autocomplete_start_pos < 0) return;

    QString text = this->text();
    int cursor_pos = cursorPosition();

    // Replace current word with suggestion
    QString newText = text.left(autocomplete_start_pos) + suggestion;
    if (cursor_pos < text.length()) {
        // Preserve text after cursor
        newText += text.mid(cursor_pos);
    }

    setText(newText);
    setCursorPosition(autocomplete_start_pos + suggestion.length());
    hide_autocomplete_popup();
}

QStringList MyLineEdit::get_autocomplete_suggestions(const QString& prefix) {
    QStringList res;
    for (auto comp : autocomplete_strings) {
        if (comp.startsWith(prefix)) {
            res.append(comp);
        }
    }
    return res;
}

void MyLineEdit::set_autocomplete_strings(QStringList strings) {
    autocomplete_strings = strings;
}

MyLineEdit::MyLineEdit(QWidget* parent) : QLineEdit(parent) {
    if (dynamic_cast<MainWidget*>(parent)){
        main_widget = dynamic_cast<MainWidget*>(parent);
    }

    // Initialize autocomplete popup
    autocomplete_popup = new QListWidget(this);
    autocomplete_popup->setWindowFlags(Qt::ToolTip);
    autocomplete_popup->setFocusPolicy(Qt::NoFocus);
    autocomplete_popup->setFocusProxy(this);
    autocomplete_popup->setMouseTracking(true);
    autocomplete_popup->setSelectionBehavior(QAbstractItemView::SelectRows);
    autocomplete_popup->setFrameStyle(QFrame::Box | QFrame::Plain);
    autocomplete_popup->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    autocomplete_popup->hide();

    connect(autocomplete_popup, &QListWidget::itemClicked, [this](QListWidgetItem* item) {
        insert_autocomplete_suggestion(item->text());
        hide_autocomplete_popup();
    });
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
    // Handle special keys for autocomplete popup
    if (is_autocomplete_active) {
        switch (event->key()) {
            case Qt::Key_Up:
                {
                    int row = autocomplete_popup->currentRow() - 1;
                    if (row < 0) row = autocomplete_popup->count() - 1;
                    autocomplete_popup->setCurrentRow(row);
                    return;
                }
            case Qt::Key_Down:
                {
                    int row = (autocomplete_popup->currentRow() + 1) % autocomplete_popup->count();
                    autocomplete_popup->setCurrentRow(row);
                    return;
                }
            case Qt::Key_Tab:
            case Qt::Key_Return:
            case Qt::Key_Enter:
                if (autocomplete_popup->currentItem()) {
                    insert_autocomplete_suggestion(autocomplete_popup->currentItem()->text());
                    event->accept();
                    return;
                }
                break;
            case Qt::Key_Escape:
                hide_autocomplete_popup();
                event->accept();
                return;
        }
    }

    bool is_alt_pressed = event->modifiers() & Qt::AltModifier;
    bool is_control_pressed = is_platform_control_pressed(event->modifiers());
    bool is_shift_pressed = event->modifiers() & Qt::ShiftModifier;
    bool is_meta_pressed = is_platform_meta_pressed(event->modifiers());
    bool is_invisible = event->text().size() == 0;
    if ((main_widget != nullptr) && (is_invisible || is_alt_pressed || is_control_pressed)) {
        std::unique_ptr<Command> command = main_widget->input_handler->get_menu_command(main_widget, event, is_shift_pressed, is_control_pressed, is_meta_pressed, is_alt_pressed);

        if (command && command->is_menu_command()) {
            // this command will be handled later by our command manager so we ignore it here.
            event->ignore();
            return;
        }
        if (command && (command->get_name() == "copy") && (selectionLength() == 0)) {
            // In the bookmark chat mode, the line edit is always focused
            // and the line edit consumes the C-c input so we can not copy the selected text
            // here if there is no selection on the line edit itself, we don't handle the event
            // so that it propagates to the main widget wihch can handle the chat window selection copy
            event->ignore();
            return;
        }
    }

    QLineEdit::keyPressEvent(event);

    // Update autocomplete after key press for printable characters
    if (!event->text().isEmpty() &&
        event->key() != Qt::Key_Escape &&
        event->key() != Qt::Key_Return &&
        event->key() != Qt::Key_Enter) {
        update_autocomplete_popup();
    }

}
