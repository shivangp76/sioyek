#include "book.h"
#include "utils.h"
#include "document.h"
#include "document_view.h"

extern float BOOKMARK_RECT_SIZE;

extern float FREETEXT_BOOKMARK_FONT_SIZE;
extern QString computer_modern_font_family;
extern std::wstring BOOKMARK_FONT_FACE;
extern int BACKGROUND_BOOKMARKS_PIXEL_BUDGET;
extern float HIGHLIGHT_COLORS[26 * 3];

bool operator==(const DocumentViewState& lhs, const DocumentViewState& rhs)
{
    return (lhs.book_state.offset_x == rhs.book_state.offset_x) &&
        (lhs.book_state.offset_y == rhs.book_state.offset_y) &&
        (lhs.book_state.zoom_level == rhs.book_state.zoom_level) &&
        (lhs.document_path == rhs.document_path);
}

bool operator==(const CachedPageData& lhs, const CachedPageData& rhs) {
    if (lhs.doc != rhs.doc) return false;
    if (lhs.page != rhs.page) return false;
    if (lhs.zoom_level != rhs.zoom_level) return false;
    return true;
}

Portal Portal::with_src_offset(float src_offset)
{
    Portal res = Portal();
    res.src_offset_y = src_offset;
    return res;
}

QJsonObject Mark::to_json(std::string doc_checksum) const
{
    QJsonObject res;
    res[Mark::Y_OFFSET_COLUMN_NAME] = y_offset;
    if (x_offset) {
        res[Mark::X_OFFSET_COLUMN_NAME] = x_offset.value();
    }
    res[Mark::SYMBOL_COLUMN_NAME] = symbol;

    add_metadata_to_json(res);
    return res;
}

void Annotation::add_metadata_to_json(QJsonObject& obj) const {
    obj[Annotation::CREATION_TIME_COLUMN_NAME] = QString::fromStdString(creation_time);
    obj[Annotation::MODIFICATION_TIME_COLUMN_NAME] = QString::fromStdString(modification_time);
    obj[Annotation::UUID_COLUMN_NAME] = QString::fromStdString(uuid);
}

std::string normalize_date_string(QString date_string) {
    QDateTime datetime = QDateTime::fromString(date_string, Qt::ISODate);
    datetime.setTimeSpec(Qt::UTC);
    return datetime.toString("yyyy-MM-dd hh:mm:ss").toStdString();
}

void Annotation::load_metadata_from_json(const QJsonObject& obj) {
    if (obj.contains(Annotation::CREATION_TIME_COLUMN_NAME)) {
        creation_time = normalize_date_string(obj[Annotation::CREATION_TIME_COLUMN_NAME].toString());
    }
    if (obj.contains(Annotation::MODIFICATION_TIME_COLUMN_NAME)) {
        modification_time = normalize_date_string(obj[Annotation::MODIFICATION_TIME_COLUMN_NAME].toString());
    }
    if (obj.contains(Annotation::UUID_COLUMN_NAME)) {
        uuid = obj[Annotation::UUID_COLUMN_NAME].toString().toStdString();
    }
    else {
        uuid = utf8_encode(new_uuid());
    }
}

std::vector<std::pair<QString, QVariant>> Annotation::to_tuples() {
    std::vector<std::pair<QString, QVariant>> res;

    add_to_tuples(res);

    res.push_back({Annotation::UUID_COLUMN_NAME, QString::fromStdString(uuid) });
    res.push_back({Annotation::CREATION_TIME_COLUMN_NAME, QString::fromStdString(creation_time) });
    res.push_back({Annotation::MODIFICATION_TIME_COLUMN_NAME, QString::fromStdString(modification_time) });

    return res;
}

QDateTime Annotation::get_creation_datetime() const {
    //return QDateTime::fromString(QString::fromStdString(creation_time), "yyyy-MM-dd HH:mm:ss");
    QDateTime res = QDateTime::fromString(QString::fromStdString(creation_time), Qt::ISODate);
    res.setTimeSpec(Qt::UTC);
    return res;
}

QDateTime Annotation::get_modification_datetime() const {
    //return QDateTime::fromString(QString::fromStdString(modification_time), "yyyy-MM-dd HH:mm:ss");
    QDateTime res = QDateTime::fromString(QString::fromStdString(modification_time), Qt::ISODate);
    res.setTimeSpec(Qt::UTC);
    return res;
}

void Annotation::update_creation_time() {
    //creation_time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();
    creation_time = QDateTime::currentDateTime().toUTC().toString(Qt::ISODate).toStdString();
    update_modification_time();
}

void Annotation::update_modification_time() {
    //modification_time = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss").toStdString();
    modification_time = QDateTime::currentDateTime().toUTC().toString(Qt::ISODate).toStdString();
}

Mark Mark::from_json(const QJsonObject& json_object)
{

    Mark res;
    if (json_object.contains("y_offset")) { // old style from previous sioyek versions 
        res.y_offset = json_object["y_offset"].toDouble();
        res.symbol = static_cast<char>(json_object["symbol"].toInt());
    }
    else {
        res.y_offset = json_object[Mark::Y_OFFSET_COLUMN_NAME].toDouble();
        if (json_object.contains(Mark::X_OFFSET_COLUMN_NAME)) {
            res.x_offset = json_object[Mark::X_OFFSET_COLUMN_NAME].toDouble();
        }
        res.symbol = static_cast<char>(json_object[Mark::SYMBOL_COLUMN_NAME].toInt());
    }

    res.load_metadata_from_json(json_object);
    return res;

}

void Mark::add_to_tuples(std::vector<std::pair<QString, QVariant>>& tuples) {
    tuples.push_back({Mark::Y_OFFSET_COLUMN_NAME, y_offset });
    if (x_offset.has_value()) {
        tuples.push_back({ Mark::X_OFFSET_COLUMN_NAME, x_offset.value()});
    }

    tuples.push_back({Mark::SYMBOL_COLUMN_NAME, QChar(symbol) });

}

QJsonObject BookMark::to_json(std::string doc_checksum) const
{
    QJsonObject res;

    res[BookMark::Y_OFFSET_COLUMN_NAME] = y_offset_;
    res[BookMark::DESCRIPTION_COLUMN_NAME] = QString::fromStdWString(description);
    res[BookMark::BEGIN_X_COLUMN_NAME] = begin_x;
    res[BookMark::BEGIN_Y_COLUMN_NAME] = begin_y;
    res[BookMark::END_X_COLUMN_NAME] = end_x;
    res[BookMark::END_Y_COLUMN_NAME] = end_y;

    res[BookMark::COLOR_R_COLUMN_NAME] = color[0];
    res[BookMark::COLOR_G_COLUMN_NAME] = color[1];
    res[BookMark::COLOR_B_COLUMN_NAME] = color[2];
    res[BookMark::FONT_SIZE_COLUMN_NAME] = font_size;
    res[BookMark::FONT_FACE_COLUMN_NAME] = QString::fromStdWString(font_face);

    add_metadata_to_json(res);

    return res;

}

void BookMark::add_to_tuples(std::vector<std::pair<QString, QVariant>>& tuples) {
    tuples.push_back({BookMark::Y_OFFSET_COLUMN_NAME, y_offset_ });
    tuples.push_back({BookMark::DESCRIPTION_COLUMN_NAME,  QString::fromStdWString(description) });
    tuples.push_back({BookMark::BEGIN_X_COLUMN_NAME,  begin_x });
    tuples.push_back({BookMark::BEGIN_Y_COLUMN_NAME,  begin_y });
    tuples.push_back({BookMark::END_X_COLUMN_NAME,  end_x });
    tuples.push_back({BookMark::END_Y_COLUMN_NAME,  end_y });
    tuples.push_back({BookMark::COLOR_R_COLUMN_NAME, color[0] });
    tuples.push_back({BookMark::COLOR_G_COLUMN_NAME, color[1] });
    tuples.push_back({BookMark::COLOR_B_COLUMN_NAME, color[2] });
    tuples.push_back({BookMark::FONT_SIZE_COLUMN_NAME, font_size });
    tuples.push_back({BookMark::FONT_FACE_COLUMN_NAME, QString::fromStdWString(font_face) });
}

BookMark BookMark::from_json(const QJsonObject& json_object)
{
    BookMark res;

    if (json_object.contains("description")) { // old style exported from previous sioyek versions

        res.y_offset_ = json_object["y_offset"].toDouble();
        res.description = json_object["description"].toString().toStdWString();
        res.begin_x = json_object["begin_x"].toDouble();
        res.begin_y = json_object["begin_y"].toDouble();
        res.end_x = json_object["end_x"].toDouble();
        res.end_y = json_object["end_y"].toDouble();

        if (json_object.contains("color_red")) {
            res.color[0] = json_object["color_red"].toDouble();
            res.color[1] = json_object["color_green"].toDouble();
            res.color[2] = json_object["color_blue"].toDouble();
            res.font_size = json_object["font_size"].toDouble();
            res.font_face = json_object["font_face"].toString().toStdWString();
        }
    }
    else {
        res.y_offset_ = json_object[BookMark::Y_OFFSET_COLUMN_NAME].toDouble();
        res.description = json_object[BookMark::DESCRIPTION_COLUMN_NAME].toString().toStdWString();
        res.begin_x = json_object[BookMark::BEGIN_X_COLUMN_NAME].toDouble();
        res.begin_y = json_object[BookMark::BEGIN_Y_COLUMN_NAME].toDouble();
        res.end_x = json_object[BookMark::END_X_COLUMN_NAME].toDouble();
        res.end_y = json_object[BookMark::END_Y_COLUMN_NAME].toDouble();
        res.color[0] = json_object[BookMark::COLOR_R_COLUMN_NAME].toDouble();
        res.color[1] = json_object[BookMark::COLOR_G_COLUMN_NAME].toDouble();
        res.color[2] = json_object[BookMark::COLOR_B_COLUMN_NAME].toDouble();
        res.font_size = json_object[BookMark::FONT_SIZE_COLUMN_NAME].toDouble();
        res.font_face = json_object[BookMark::FONT_FACE_COLUMN_NAME].toString().toStdWString();
    }

    res.load_metadata_from_json(json_object);
    return res;
}

bool BookMark::is_freetext() const {
    return (begin_y != -1) && (end_y != -1);
}

QString BookMark::get_render_text() const {
    if (is_box()) {
        if (description.size() > 0) {
            int space_index = description.find(L" ");
            int newline_index = description.find(L"\n");

            if ((newline_index >= 0) && (newline_index < space_index)) {
                space_index = newline_index;
            }

            if (space_index != std::wstring::npos) {
                return QString::fromStdWString(description.substr(space_index + 1));
            }
            return "";
        }
    }

    return QString::fromStdWString(description);
}

std::wstring  BookMark::get_style_text() const {
    if (is_box()) {
        if (description.size() > 0) {
            int space_index = description.find(L" ");
            int newline_index = description.find(L"\n");

            if ((newline_index >= 0) && (newline_index < space_index)) {
                space_index = newline_index;
            }

            if (space_index != std::wstring::npos) {
                return description.substr(1, space_index - 1);
            }
            else {
                return description.substr(1);
            }
        }
    }

    return {};
}

std::optional<char> BookMark::get_type() const {
    if (is_box()) {
        QString style_text = QString::fromStdWString(get_style_text());
        if (style_text.startsWith("markdown")){
            style_text = style_text.mid(8);
        }
        if (style_text.startsWith("latex")){
            style_text = style_text.mid(5);
        }

        if (style_text.size() > 0) {
            return style_text[0].unicode();
        }
    }
    return {};
}

std::optional<char> BookMark::get_background_type() const {
    if (is_box()){
        QString style_text = QString::fromStdWString(get_style_text());
        if (style_text.startsWith("markdown")){
            style_text = style_text.mid(8);
        }
        if (style_text.startsWith("latex")){
            style_text = style_text.mid(5);
        }
        if (style_text.size() > 1 && style_text.size() <= 3) {
            return style_text[1].unicode();
        }

    }
    return {};
}

std::optional<char> BookMark::get_text_type() const {
    if (is_box()){
        QString style_text = QString::fromStdWString(get_style_text());
        if (style_text.startsWith("markdown")){
            style_text = style_text.mid(8);
        }
        if (style_text.startsWith("latex")){
            style_text = style_text.mid(5);
        }
        if (style_text.size() > 2 && style_text.size() <= 3) {
            return style_text[2].unicode();
        }

    }
    return {};
}

bool BookMark::is_box() const {
    if (description.size() > 0) {
        return description[0] == '#';
    }
    return false;
}


bool BookMark::is_marked() const {
    return (begin_y > -1) && (end_y == -1);
}

bool BookMark::is_question() const {
    return description.size() >= 2 && description.substr(0, 2) == L"? ";
}

bool BookMark::is_summary() const {
    return QString::fromStdWString(description).startsWith("#summarize");
}

QString BookMark::get_question_or_summary_markdown() const {
    return BookMark::get_display_markdown_or_text(QString::fromStdWString(description));
}

QString BookMark::get_display_markdown_or_text(QString res){
    //QString res = QString::fromStdWString(description);
    if (res.startsWith("#summarize")) { // summary
        return res.mid(10);
    }
    else if (res.startsWith("#markdown")) {
        return res.mid(9);
    }
    else if (res.startsWith("? ")) { // question
        QStringList lines = res.split('\n');
        QString result;

        for (auto line : lines) {
            if (line.startsWith("? ")) {
                result += "\n---\n";
                result += "\n<b>" + line.mid(2) + "</b>\n";
                result += "\n---\n";
            }
            else {
                result += line + "\n";
            }
        }
        return result;
    }
    else if (res.startsWith("#")) {
        int first_space_index = res.indexOf(" ");
        if (first_space_index >= 0) {
            return res.mid(first_space_index + 1);
        }
        else {
            return "";
        }
    }
    else {
        return res;
    }
}

QJsonObject Highlight::to_json(std::string doc_checksum) const
{
    QJsonObject res;
    res[Highlight::SELECTION_BEGIN_X_COLUMN_NAME] = selection_begin.x;
    res[Highlight::SELECTION_BEGIN_Y_COLUMN_NAME] = selection_begin.y;
    res[Highlight::SELECTION_END_X_COLUMN_NAME] = selection_end.x;
    res[Highlight::SELECTION_END_Y_COLUMN_NAME] = selection_end.y;
    res[Highlight::DESCRIPTION_COLUMN_NAME] = QString::fromStdWString(description);
    res[Highlight::TEXT_ANNOT_COLUMN_NAME] = QString::fromStdWString(text_annot);
    res[Highlight::TYPE_COLUMN_NAME] = type;

    add_metadata_to_json(res);
    return res;
}

std::optional<AbsoluteRect> Highlight::get_rectangle() const{
    auto top_left = selection_begin;
    auto bottom_right = selection_end;
    if (top_left.y > bottom_right.y) {
        std::swap(top_left, bottom_right);
    }
    return AbsoluteRect(top_left, bottom_right);
}

void Highlight::add_to_tuples(std::vector<std::pair<QString, QVariant>>& tuples) {
    tuples.push_back({ Highlight::SELECTION_BEGIN_X_COLUMN_NAME, selection_begin.x });
    tuples.push_back({ Highlight::SELECTION_BEGIN_Y_COLUMN_NAME, selection_begin.y });
    tuples.push_back({ Highlight::SELECTION_END_X_COLUMN_NAME, selection_end.x });
    tuples.push_back({ Highlight::SELECTION_END_Y_COLUMN_NAME, selection_end.y });
    tuples.push_back({ Highlight::DESCRIPTION_COLUMN_NAME, QString::fromStdWString(description) });
    tuples.push_back({ Highlight::TEXT_ANNOT_COLUMN_NAME, QString::fromStdWString(text_annot) });
    tuples.push_back({ Highlight::TYPE_COLUMN_NAME, QChar(type) });
}

Highlight Highlight::from_json(const QJsonObject& json_object)
{
    Highlight res;
    if (json_object.contains("selection_begin_x")) { // old style exported from previous sioyek versions

        res.selection_begin.x = json_object["selection_begin_x"].toDouble();
        res.selection_begin.y = json_object["selection_begin_y"].toDouble();
        res.selection_end.x = json_object["selection_end_x"].toDouble();
        res.selection_end.y = json_object["selection_end_y"].toDouble();
        res.description = json_object["description"].toString().toStdWString();
        if (json_object["type"].isDouble()) {
            res.type = static_cast<char>(json_object["type"].toInt());
        }
        else {
            res.type = static_cast<char>(json_object["type"].toString().toInt());
        }
    }
    else {
        res.selection_begin.x = json_object[Highlight::SELECTION_BEGIN_X_COLUMN_NAME].toDouble();
        res.selection_begin.y = json_object[Highlight::SELECTION_BEGIN_Y_COLUMN_NAME].toDouble();
        res.selection_end.x = json_object[Highlight::SELECTION_END_X_COLUMN_NAME].toDouble();
        res.selection_end.y = json_object[Highlight::SELECTION_END_Y_COLUMN_NAME].toDouble();
        res.description = json_object[Highlight::DESCRIPTION_COLUMN_NAME].toString().toStdWString();
        if (json_object[Highlight::TYPE_COLUMN_NAME].isDouble()) {
            res.type = static_cast<char>(json_object[Highlight::TYPE_COLUMN_NAME].toInt());
        }
        else {
            res.type = static_cast<char>(json_object[Highlight::TYPE_COLUMN_NAME].toString().toInt());
        }
    }

    res.load_metadata_from_json(json_object);
    return res;
}

QJsonObject Portal::to_json(std::string doc_checksum) const
{
    QJsonObject res;
    res[Portal::SRC_OFFSET_Y_COLUMN_NAME] = src_offset_y;
    res[Portal::DST_DOCUMENT_COLUMN_NAME] = QString::fromStdString(dst.document_checksum);
    res[Portal::DST_OFFSET_X_COLUMN_NAME] = dst.book_state.offset_x;
    res[Portal::DST_OFFSET_Y_COLUMN_NAME] = dst.book_state.offset_y;
    res[Portal::DST_ZOOM_LEVEL_COLUMN_NAME] = dst.book_state.zoom_level;

    if (src_offset_x) {
        res[Portal::SRC_OFFSET_X_COLUMN_NAME] = src_offset_x.value();
    }

    if (src_offset_end_x) {
        res[Portal::SRC_OFFSET_END_X_COLUMN_NAME] = src_offset_end_x.value();
    }

    if (src_offset_end_y) {
        res[Portal::SRC_OFFSET_END_Y_COLUMN_NAME] = src_offset_end_x.value();
    }

    res["same"] = (doc_checksum == dst.document_checksum);
    add_metadata_to_json(res);
    return res;
}

Portal Portal::from_json(const QJsonObject& json_object)
{
    Portal res;
    if (json_object.contains("dst_checksum")) { // old style from previous sioyek versions
        res.src_offset_y = json_object["src_offset_y"].toDouble();
        res.dst.document_checksum = json_object["dst_checksum"].toString().toStdString();
        res.dst.book_state.offset_x = json_object["dst_offset_x"].toDouble();
        res.dst.book_state.offset_y = json_object["dst_offset_y"].toDouble();
        res.dst.book_state.zoom_level = json_object["dst_zoom_level"].toDouble();

        if (json_object.contains("src_offset_x")) {

            res.src_offset_x = json_object["src_offset_x"].toDouble();
        }
    }
    else{
        res.src_offset_y = json_object[Portal::SRC_OFFSET_Y_COLUMN_NAME].toDouble();
        res.dst.document_checksum = json_object[Portal::DST_DOCUMENT_COLUMN_NAME].toString().toStdString();
        res.dst.book_state.offset_x = json_object[Portal::DST_OFFSET_X_COLUMN_NAME].toDouble();
        res.dst.book_state.offset_y = json_object[Portal::DST_OFFSET_Y_COLUMN_NAME].toDouble();
        res.dst.book_state.zoom_level = json_object[Portal::DST_ZOOM_LEVEL_COLUMN_NAME].toDouble();

        if (json_object.contains(Portal::SRC_OFFSET_X_COLUMN_NAME)) {

            res.src_offset_x = json_object[Portal::SRC_OFFSET_X_COLUMN_NAME].toDouble();
        }

        if (json_object.contains(Portal::SRC_OFFSET_END_X_COLUMN_NAME)) {
            res.src_offset_end_x = json_object[Portal::SRC_OFFSET_END_X_COLUMN_NAME].toDouble();
        }

        if (json_object.contains(Portal::SRC_OFFSET_END_Y_COLUMN_NAME)) {
            res.src_offset_end_y = json_object[Portal::SRC_OFFSET_END_Y_COLUMN_NAME].toDouble();
        }
    }

    res.load_metadata_from_json(json_object);
    return res;
}

void Portal::add_to_tuples(std::vector<std::pair<QString, QVariant>>& tuples) {
    tuples.push_back({ Portal::SRC_OFFSET_Y_COLUMN_NAME, src_offset_y });
    tuples.push_back({ Portal::DST_DOCUMENT_COLUMN_NAME, QString::fromStdString(dst.document_checksum) });
    tuples.push_back({ Portal::DST_OFFSET_X_COLUMN_NAME, dst.book_state.offset_x });
    tuples.push_back({ Portal::DST_OFFSET_Y_COLUMN_NAME, dst.book_state.offset_y });
    tuples.push_back({ Portal::DST_ZOOM_LEVEL_COLUMN_NAME, dst.book_state.zoom_level });

    if (src_offset_x) {
        tuples.push_back({ Portal::SRC_OFFSET_X_COLUMN_NAME, src_offset_x.value() });
    }

    if (src_offset_end_x) {
        tuples.push_back({ Portal::SRC_OFFSET_END_X_COLUMN_NAME, src_offset_end_x.value() });
    }

    if (src_offset_end_y) {
        tuples.push_back({ Portal::SRC_OFFSET_END_Y_COLUMN_NAME, src_offset_end_y.value() });
    }

}

bool operator==(const Mark& lhs, const Mark& rhs)
{
    return (lhs.symbol == rhs.symbol) && (lhs.y_offset == rhs.y_offset);
}

bool operator==(const BookMark& lhs, const BookMark& rhs)
{
    //return lhs.uuid == rhs.uuid;
    return  (lhs.y_offset_ == rhs.y_offset_) && (lhs.description == rhs.description);
}

bool operator==(const fz_point& lhs, const fz_point& rhs) {
    return (lhs.y == rhs.y) && (lhs.x == rhs.x);
}

bool operator==(const Highlight& lhs, const Highlight& rhs)
{
    //return lhs.uuid == rhs.uuid;
    return  (lhs.selection_begin.x == rhs.selection_begin.x) && (lhs.selection_end.x == rhs.selection_end.x) &&
        (lhs.selection_begin.y == rhs.selection_begin.y) && (lhs.selection_end.y == rhs.selection_end.y);
}

bool operator==(const Portal& lhs, const Portal& rhs)
{
    return  (lhs.src_offset_y == rhs.src_offset_y) && (lhs.dst.document_checksum == rhs.dst.document_checksum);
}

bool are_same(const BookMark& lhs, const BookMark& rhs) {
    return are_same(lhs.begin_x, rhs.begin_x) && are_same(lhs.begin_y, rhs.begin_y) && are_same(lhs.end_x, rhs.end_x) && are_same(lhs.end_y, rhs.end_y);
}

bool has_changed(const Portal& lhs, const Portal& rhs) {
    if (
        lhs.src_offset_x != rhs.src_offset_x ||
        lhs.src_offset_y != rhs.src_offset_y ||
        lhs.dst.book_state.offset_x != rhs.dst.book_state.offset_x ||
        lhs.dst.book_state.offset_y != rhs.dst.book_state.offset_y ||
        lhs.dst.book_state.zoom_level != rhs.dst.book_state.zoom_level
        ) {
        return true;
    }
    return false;
}

bool has_changed(const Highlight& lhs, const Highlight& rhs){
    if (lhs.modification_time == rhs.modification_time) {
        return false;
    }
    return (lhs.text_annot != rhs.text_annot) || (lhs.type != rhs.type);
}

bool has_changed(const BookMark& lhs, const BookMark& rhs) {
    if (lhs.modification_time == rhs.modification_time) {
        return false;
    }

    return (lhs.description != rhs.description) ||
        (lhs.begin_x != rhs.begin_x) ||
        (lhs.begin_y != rhs.begin_y) ||
        (lhs.end_x != rhs.end_x) ||
        (lhs.end_y != rhs.end_y) ||
        (lhs.font_size != rhs.font_size) ||
        (lhs.font_face != rhs.font_face) ||
        (lhs.color[0] != rhs.color[0]) ||
        (lhs.color[1] != rhs.color[1]) ||
        (lhs.color[2] != rhs.color[2]);
}

bool are_same(const Highlight& lhs, const Highlight& rhs) {
    return are_same(lhs.selection_begin, rhs.selection_begin) && are_same(lhs.selection_end, rhs.selection_end);
}

bool Portal::is_visible() const {
    return src_offset_x.has_value();
}

bool Portal::is_pinned() const {
    return src_offset_end_x.has_value() && src_offset_end_y.has_value();
}

bool Portal::is_icon() const {
    return is_visible() && !is_pinned();
}


void BookMark::set_side_to_pos(OverviewSide side, AbsoluteDocumentPos pos) {
    if (is_freetext()) {
        if (side == OverviewSide::left) {
            begin_x = pos.x;
        }
        if (side == OverviewSide::right) {
            end_x = pos.x;
        }
        if (side == OverviewSide::top) {
            begin_y = pos.y;
        }
        if (side == OverviewSide::bottom) {
            end_y = pos.y;
        }
    }
}

std::optional<AbsoluteRect> BookMark::get_selection_rectangle() const {

    std::optional<AbsoluteRect> rect_ = get_rectangle();
    if (rect_.has_value()) {
        AbsoluteRect rect = rect_.value();

        if (is_freetext()) {
            rect.x0 -= 5;
            rect.x1 += 5;
            rect.y0 -= 5;
            rect.y1 += 5;
        }
        return rect;
    }
    return {};
}

std::optional<AbsoluteRect> Portal::get_selection_rectangle() const {

    std::optional<AbsoluteRect> rect_ = get_rectangle();
    if (rect_.has_value()) {
        AbsoluteRect rect = rect_.value();

        if (is_pinned()) {
            rect.x0 -= 5;
            rect.x1 += 5;
            rect.y0 -= 5;
            rect.y1 += 5;
        }
        return rect;
    }
    return {};
}

std::optional<AbsoluteRect> BookMark::get_rectangle() const{
    if (end_y > -1) {

        return AbsoluteRect(
            AbsoluteDocumentPos{ begin_x, begin_y },
            AbsoluteDocumentPos{ end_x, end_y }
        );
    }
    else {
        return AbsoluteRect(
            AbsoluteDocumentPos{ begin_x - BOOKMARK_RECT_SIZE, begin_y - BOOKMARK_RECT_SIZE },
            AbsoluteDocumentPos{ begin_x + BOOKMARK_RECT_SIZE, begin_y + BOOKMARK_RECT_SIZE }
        );
    }
}

AbsoluteRect Portal::get_actual_rectangle() const{

    if (is_pinned()) {
        return AbsoluteRect(
            AbsoluteDocumentPos{ src_offset_x.value(), src_offset_end_y.value()},
            AbsoluteDocumentPos{ src_offset_end_x.value(), src_offset_y}
        );
    }
    else {
        return AbsoluteRect(
            AbsoluteDocumentPos{ src_offset_x.value() - BOOKMARK_RECT_SIZE, src_offset_y - BOOKMARK_RECT_SIZE },
            AbsoluteDocumentPos{ src_offset_x.value() + BOOKMARK_RECT_SIZE, src_offset_y + BOOKMARK_RECT_SIZE }
        );
    }
}

std::optional<AbsoluteRect> Portal::get_rectangle() const{

    if (!src_offset_x.has_value()) return {};
    if (merged_rect && is_icon()) return merged_rect.value();
    return get_actual_rectangle();

}

float BookMark::get_y_offset() const{
    if (begin_y != -1) return begin_y;
    return y_offset_;
}

AbsoluteDocumentPos BookMark::begin_pos() const {
    return AbsoluteDocumentPos{ begin_x, begin_y };
}

AbsoluteDocumentPos BookMark::end_pos() const {
    return AbsoluteDocumentPos{ end_x, end_y };
}

AbsoluteRect BookMark::rect() const {
    return AbsoluteRect(begin_pos(), end_pos());
}

AbsoluteRect FreehandDrawing::bbox(){
    AbsoluteRect res;
    if (points.size() > 0) {
        res.x0 = points[0].pos.x;
        res.x1 = points[0].pos.x;
        res.y0 = points[0].pos.y;
        res.y1 = points[0].pos.y;
        for (int i = 1; i < points.size(); i++) {
            res.x0 = std::min(points[i].pos.x, res.x0);
            res.x1 = std::max(points[i].pos.x, res.x1);
            res.y0 = std::min(points[i].pos.y, res.y0);
            res.y1 = std::max(points[i].pos.y, res.y1);
        }
    }
    return res;
}

void SearchResult::fill(Document* doc) {
    if (rects.size() == 0) {
        doc->fill_search_result(this);
    }
}

std::string reference_type_string(ReferenceType rt) {
    if (rt == ReferenceType::NoReference) return "none";
    if (rt == ReferenceType::Equation) return "equation";
    if (rt == ReferenceType::Reference) return "reference";
    if (rt == ReferenceType::Abbreviation) return "abbreviation";
    if (rt == ReferenceType::Generic) return "generic";
    if (rt == ReferenceType::Link) return "link";
    if (rt == ReferenceType::RefLink) return "reflink";
    return "";
}

void Portal::update_merged_rect(Document* doc) const{

    if (!is_visible()) return;
    if (is_pinned()) return;

    if (merged_rect) {
        if (!merged_rect->intersects(get_actual_rectangle())) {
            merged_rect = {};
        }
    }
    if (!merged_rect.has_value()) {
        int source_page = doc->absolute_to_page_pos(AbsoluteDocumentPos{ 0, src_offset_y }).page;
        float max_intersection_area = 0;
        for (const auto& link : doc->get_page_merged_pdf_links(source_page)) {
            if (link.rects.size() > 0) {
                AbsoluteRect link_rect = DocumentRect{ link.rects[0], source_page }.to_absolute(doc);
                float interseciton_area = link_rect.intersect_rect(get_actual_rectangle()).area();
                if (interseciton_area > max_intersection_area) {
                    max_intersection_area = interseciton_area;
                    merged_rect = link_rect;
                }
            }
        }
    }
}

void SmartViewCandidate::set_highlight_rects(std::vector<DocumentRect> rects){
    highlight_rects_ = rects;
    are_highlights_computed = true;
}

const std::vector<DocumentRect> SmartViewCandidate::get_highlight_rects() {
    if (are_highlights_computed) {
        return highlight_rects_;
    }
    else {
        if (highlight_rects_func.has_value()) {
            highlight_rects_ = highlight_rects_func.value()();
            are_highlights_computed = true;
            return highlight_rects_;
        }
    }
    return {};
}

bool BookMark::is_latex() const {
    return QString::fromStdWString(description).startsWith("#latex");
}

bool BookMark::is_markdown() const {
    return QString::fromStdWString(description).startsWith("#markdown");
}

bool BookMark::can_have_links() const {
    return is_summary() || is_question();
}

std::pair<QStringList, QStringList> BookMark::get_links() const {
    QRegularExpression markdown_link_regex = QRegularExpression("\\[(.*)\\]\\((.*)\\)");

    QStringList link_targets;
    QStringList link_names;
    QString text = QString::fromStdWString(description);
    QRegularExpressionMatchIterator i = markdown_link_regex.globalMatch(text);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        link_names.push_back(match.captured(1));
        link_targets.append(match.captured(2));
    }
    return std::make_pair(link_names, link_targets);
}

bool BookMark::should_be_displayed_as_markdown(QString bookmark_text) {
    return bookmark_text.startsWith("#markdown") || bookmark_text.startsWith("#summarize") || bookmark_text.startsWith("? ");
}


QFont BookMark::get_font(float zoom_level) const {
    QFont font = font_face.size() > 0 ?
        QFont(QString::fromStdWString(font_face)) :
        (BOOKMARK_FONT_FACE.size() > 0 ? QFont(QString::fromStdWString(BOOKMARK_FONT_FACE)) : QFont(computer_modern_font_family));
    float size = font_size == -1 ? FREETEXT_BOOKMARK_FONT_SIZE : font_size;
    font.setPointSizeF(size * zoom_level * 0.75);
    return font;
}

float OverviewState::get_zoom_level(DocumentView* dv) {
    if (original_zoom_level.has_value()) {
        return zoom_level * dv->get_zoom_level() / original_zoom_level.value();
    }
    else {
        return zoom_level;
    }

}

std::optional<AbsoluteRect> Annotation::get_rectangle() const {
    return {};
}

std::optional<OverviewSide> Annotation::get_resize_side_containing_point(AbsoluteDocumentPos point) const {
    //if (is_freetext()) {

    std::optional<AbsoluteRect> r_ = get_rectangle();
    if (r_.has_value()) {
        AbsoluteRect r = r_.value();
        AbsoluteDocumentPos tl = r.top_left();
        AbsoluteDocumentPos br = r.bottom_right();
        AbsoluteDocumentPos tr = tl;
        tr.x = br.x;
        AbsoluteDocumentPos bl = br;
        bl.x = tl.x;

        AbsoluteRect top_resize_rect(tl.y_shift(-5), tr.y_shift(5));
        AbsoluteRect left_resize_rect(tl.x_shift(-5), bl.x_shift(5));
        AbsoluteRect right_resize_rect(tr.x_shift(-5), br.x_shift(5));
        AbsoluteRect bottom_resize_rect(bl.y_shift(-5), br.y_shift(5));
        if (top_resize_rect.contains(point)) return OverviewSide::top;
        if (bottom_resize_rect.contains(point)) return OverviewSide::bottom;
        if (left_resize_rect.contains(point)) return OverviewSide::left;
        if (right_resize_rect.contains(point)) return OverviewSide::right;
        return {};
    }
    //}

    return {};
}

void Portal::set_side_to_pos(OverviewSide side, AbsoluteDocumentPos pos) {
    if (is_pinned()) {
        if (side == OverviewSide::left) {
            src_offset_x = pos.x;
        }
        if (side == OverviewSide::right) {
            src_offset_end_x = pos.x;
        }
        if (side == OverviewSide::top) {
            src_offset_end_y = pos.y;
        }
        if (side == OverviewSide::bottom) {
            src_offset_y = pos.y;
        }
    }
}

AbsoluteDocumentPos OverviewState::get_absolute_pos() {
    return AbsoluteDocumentPos{ absolute_offset_x, absolute_offset_y };
}

std::optional<QColor> get_color_from_type(std::optional<char> t) {
    if (t.has_value()) {
        if (t.value() >= 'a' && t.value() <= 'z') {
            int index = t.value() - 'a';
            return QColor::fromRgbF(HIGHLIGHT_COLORS[3 * index], HIGHLIGHT_COLORS[3 * index + 1], HIGHLIGHT_COLORS[3 * index + 2]);
        }
    }
    return {};
}

std::optional<QColor> BookMark::get_background_color() const {
    std::optional<char> background_type = get_background_type();
    if (background_type.has_value()) {
        return get_color_from_type(background_type);

    }
    return {};
}

std::optional<QColor> BookMark::get_border_color() const {
    std::optional<char> border_type = get_type();
    if (border_type.has_value()) {
        return get_color_from_type(border_type);
    }
    return {};
}

std::optional<QColor> BookMark::get_text_color() const {
    std::optional<char> text_type = get_text_type();
    if (text_type.has_value()) {
        return get_color_from_type(text_type);
    }
    return QColor::fromRgbF(color[0], color[1], color[2]);
}
