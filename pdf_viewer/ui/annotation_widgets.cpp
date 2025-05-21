#include <QPainter>
#include <QAbstractTextDocumentLayout>

#include "ui/annotation_widgets.h"
#include "ui/ui_models.h"
#include "utils.h"
#include "utils/color_utils.h"
#include "utils/window_utils.h"

extern int FONT_SIZE;
extern std::wstring MENU_MATCHED_SEARCH_HIGHLIGHT_STYLE;
extern float UI_TEXT_COLOR[3];
extern float UI_BACKGROUND_COLOR[3];
extern float UI_SELECTED_TEXT_COLOR[3];
extern float UI_SELECTED_BACKGROUND_COLOR[3];
extern float HIGHLIGHT_COLORS[26 * 3];

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

void HighlightSearchItemDelegate::clear_cache() {
    cached_sizes.clear();
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
