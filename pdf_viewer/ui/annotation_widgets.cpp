#include <QPainter>
#include <QAbstractTextDocumentLayout>

#include "ui/annotation_widgets.h"
#include "ui/ui_models.h"

extern int FONT_SIZE;
extern std::wstring MENU_MATCHED_SEARCH_HIGHLIGHT_STYLE;
extern float UI_TEXT_COLOR[3];
extern float UI_BACKGROUND_COLOR[3];
extern float UI_SELECTED_TEXT_COLOR[3];
extern float UI_SELECTED_BACKGROUND_COLOR[3];

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
    QColor bookmark_background_fill_color = is_selected ? convert_float3_to_qcolor(UI_SELECTED_BACKGROUND_COLOR) : convert_float3_to_qcolor(UI_BACKGROUND_COLOR);
    painter->fillRect(option.rect, bookmark_background_fill_color);
    //painter->fillRect(option.rect, is_selected ? Qt::red : Qt::blue);

    std::optional<QColor> bookmark_background_color = optional_from_qvariant<QColor>(bookmark.get_background_color());
    std::optional<QColor> bookmark_border_color = optional_from_qvariant<QColor>(bookmark.get_border_color());
    std::optional<QColor> bookmark_text_color = optional_from_qvariant<QColor>(bookmark.get_text_color());

    if (bookmark_background_color.has_value()){
        bookmark_background_color = bookmark_background_color;
        if (is_selected){
            painter->fillRect(option.rect, QBrush(bookmark_background_color.value().lighter()));
        }
        else{
            painter->fillRect(option.rect, QBrush(bookmark_background_color.value()));
        }
    }

    if (bookmark_border_color.has_value()){
        painter->save();
        painter->setPen(bookmark_border_color.value());
        painter->drawRect(option.rect);
        painter->restore();
    }

    if (bookmark_text_color.has_value()) {
        if (!bookmark_background_color.has_value() && (bookmark_text_color->lightness() < 50)){
            if (bookmark_text_color.value() == Qt::black){
                if (is_selected){
                    ctx.palette.setColor(QPalette::Text, QColor(0, 0, 0));
                }
                else{
                    ctx.palette.setColor(QPalette::Text, QColor(255, 255, 255));
                }
            }
            else{
                ctx.palette.setColor(QPalette::Text, bookmark_text_color.value().lighter());
            }
        }
        else{
            ctx.palette.setColor(QPalette::Text, bookmark_text_color.value());
        }
    }

    // if (bookmark_text.size() == 0) {
    //     if (bookmark.description.size() > 1 && bookmark.description[0] == '#') {
    //         if (bookmark.description[1] >= 'a' && bookmark.description[1] <= 'z') {
    //             int symbol_index = bookmark.description[1] - 'a';
    //             float* color = &HIGHLIGHT_COLORS[3 * symbol_index];
    //             QColor qcolor = QColor::fromRgbF(color[0], color[1], color[2]);
    //             painter->save();
    //             painter->setPen(qcolor);
    //             painter->drawRect(option.rect);
    //             painter->restore();
    //         }
    //         if (bookmark.description[1] >= 'A' && bookmark.description[1] <= 'Z') {
    //             int symbol_index = bookmark.description[1] - 'A';
    //             float* color = &HIGHLIGHT_COLORS[3 * symbol_index];
    //             QColor qcolor = QColor::fromRgbF(color[0], color[1], color[2], 0.5f);
    //             painter->fillRect(option.rect, QBrush(qcolor));
    //         }
    //     }
    // }
    painter->translate(option.rect.topLeft());
    painter->setClipRect(0, 0, option.rect.width(), option.rect.height());
    //qDebug() << "size in paint is :" << bookmark_document.toPlainText().size() << " " << index;

    bookmark_document.documentLayout()->draw(painter, ctx);

    //if (is_global) {
    painter->translate(0, bookmark_document.size().height());

    if (!bookmark_text_color.has_value()){
        if (!is_selected) {
            ctx.palette.setColor(QPalette::Text, QColor::fromRgbF(1, 1, 1, 0.5));
        }
        else {
            ctx.palette.setColor(QPalette::Text, QColor::fromRgbF(0, 0, 0, 0.5));
        }
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
