#include <QWheelEvent>
#include <QAbstractTextDocumentLayout>
#include <QPainter>

#include "ui/documentation_ui.h"
#include "ui/ui_models.h"
#include "config.h"
#include "main_widget.h"
#include "document_view.h"
#include "utils.h"
#include "utils/window_utils.h"
#include "utils/color_utils.h"
#include "database.h"

extern std::wstring EXTERNAL_TEXT_EDITOR_COMMAND;
extern int FONT_SIZE;
extern float UI_TEXT_COLOR[3];
extern float UI_BACKGROUND_COLOR[3];
extern float UI_SELECTED_TEXT_COLOR[3];
extern float UI_SELECTED_BACKGROUND_COLOR[3];
extern std::wstring MENU_MATCHED_SEARCH_HIGHLIGHT_STYLE;

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


void FulltextSearchWidget::on_select(QModelIndex value) {
    if (select_fn.has_value()) {
        QString input_text = line_edit->text();
        select_fn.value()(value.row());
    }
    //(value.row());
}

void FulltextSearchWidget::on_delete(const QModelIndex& source_index, const QModelIndex& selected_index) {

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

