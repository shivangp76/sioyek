#pragma once

#include <vector>
#include <string>
#include <variant>
#include <mupdf/fitz.h>
//#include <gl/glew.h>
#include <optional>
#include <qopengl.h>
#include <variant>
#include <qjsonobject.h>
#include <qdatetime.h>
#include <qpixmap.h>

#include "coordinates.h"

class DocumentView;
class Document;
class MainWidget;

enum class VisibleObjectType {
    Portal,
    PinnedPortal,
    PendingPortal,
    Bookmark,
    Highlight
};

struct VisibleObjectIndex {
    VisibleObjectType object_type;
    int index;

    void handle_move_begin(MainWidget* widget, AbsoluteDocumentPos mouse_pos);
};

enum OverviewSide {
    bottom = 0,
    top = 1,
    left = 2,
    right = 3
};

enum class ColorPalette {
    Normal,
    Dark,
    Custom,
    None
};

enum class SelectedObjectType {
    Drawing,
    Pixmap
};

enum class PaperDownloadFinishedAction {
    None,
    OpenInSameWindow,
    OpenInNewWindow,
    Portal
};

enum class ServerStatus {
    NotLoggedIn,
    InvalidCredentials,
    ServerOffline,
    LoggingIn,
    LoggedIn
};

struct SelectedObjectIndex {
    int index;
    SelectedObjectType type;
};

struct OpenedBookInfo {
    std::string checksum;
    QString document_title;
    QString file_name;
    QDateTime last_access_time;
    float offset_y;
};

struct BookState {
    std::wstring document_path;
    float offset_y;
};

struct RulerLineIndexInfo {
    int merged_index;
    std::vector<int> unmerged_indices;
};

struct OpenedBookState {
    float zoom_level;
    float offset_x;
    float offset_y;
    bool ruler_mode = false;
    std::optional<int> presentation_page = {};
    std::optional<AbsoluteRect> ruler_rect = {};
    float ruler_pos = 0;
    std::optional<RulerLineIndexInfo> ruler_info = {};
};

struct Annotation {

    static inline const QString CREATION_TIME_COLUMN_NAME = "creation_time";
    std::string creation_time;

    static inline const QString MODIFICATION_TIME_COLUMN_NAME = "modification_time";
    std::string modification_time;

    static inline const QString UUID_COLUMN_NAME = "uuid";
    std::string uuid;

    static inline const QString IS_SYNCED_COLUMN_NAME = "is_synced";
    bool is_synced = false;

    virtual QJsonObject to_json(std::string doc_checksum) const = 0;
    virtual void  add_to_tuples(std::vector<std::pair<QString, QVariant>>& tuples) = 0;
    virtual std::vector<std::pair<QString, QVariant>> to_tuples();

    void add_metadata_to_json(QJsonObject& obj) const;
    void load_metadata_from_json(const QJsonObject& obj);

    QDateTime get_creation_datetime() const;
    QDateTime get_modification_datetime() const;

    void update_creation_time();
    void update_modification_time();

    virtual std::optional<AbsoluteRect> get_rectangle() const;
    std::optional<OverviewSide> get_resize_side_containing_point(AbsoluteDocumentPos point) const;

};

/*
    A mark is a location in the document labeled with a symbol (which is a single character [a-z]). For example
    we can mark a location with symbol 'a' and later return to that location by going to the mark named 'a'.
    Lower case marks are local to the document and upper case marks are global.
*/
struct Mark : Annotation {
    static Mark from_json(const QJsonObject& json_object);

    static inline const std::string TABLE_NAME = "marks";

    static inline const QString Y_OFFSET_COLUMN_NAME = "offset_y";
    float y_offset;

    static inline const QString SYMBOL_COLUMN_NAME = "symbol";
    char symbol;

    static inline const QString X_OFFSET_COLUMN_NAME = "offset_x";
    std::optional<float> x_offset = {};

    static inline const QString ZOOM_LEVEL_COLUMN_NAME = "zoom_level";
    std::optional<float> zoom_level = {};

    QJsonObject to_json(std::string doc_checksum) const;
    void add_to_tuples(std::vector<std::pair<QString, QVariant>>& tuples) override;
};

/*
    A bookmark is similar to mark but instead of being indexed by a symbol, it has a description.
*/
struct BookMark : Annotation {
    static BookMark from_json(const QJsonObject& json_object);

    static inline const std::string TABLE_NAME = "bookmarks";

    static inline const QString Y_OFFSET_COLUMN_NAME = "offset_y";
    float y_offset_ = -1;

    static inline const QString DESCRIPTION_COLUMN_NAME = "desc";
    std::wstring description;

    static inline const QString BEGIN_X_COLUMN_NAME = "begin_x";
    float begin_x = -1;

    static inline const QString BEGIN_Y_COLUMN_NAME = "begin_y";
    float begin_y = -1;

    static inline const QString END_X_COLUMN_NAME = "end_x";
    float end_x = -1;

    static inline const QString END_Y_COLUMN_NAME = "end_y";
    float end_y = -1;

    
    static inline const QString COLOR_R_COLUMN_NAME = "color_red";
    static inline const QString COLOR_G_COLUMN_NAME = "color_green";
    static inline const QString COLOR_B_COLUMN_NAME = "color_blue";
    float color[3] = { 0 };

    static inline const QString FONT_SIZE_COLUMN_NAME = "font_size";
    float font_size = -1;

    static inline const QString FONT_FACE_COLUMN_NAME = "font_face";
    std::wstring font_face;

    QString get_question_or_summary_markdown() const;
    static QString get_display_markdown_or_text(QString bookmark_desc);
    AbsoluteDocumentPos begin_pos();
    AbsoluteDocumentPos end_pos();
    AbsoluteRect rect();
    QJsonObject to_json(std::string doc_checksum) const;
    void add_to_tuples(std::vector<std::pair<QString, QVariant>>& tuples) override;
    float get_y_offset() const;

    QFont get_font(float zoom_level) const;
    bool is_freetext() const;
    bool is_box() const;
    bool is_marked() const;
    bool is_question() const;
    bool is_summary() const;
    bool is_latex() const;
    bool is_markdown() const;
    static bool should_be_displayed_as_markdown(QString bookmark_text);
    std::optional<char> get_type() const;

    std::optional<AbsoluteRect> get_rectangle() const override;
    std::optional<AbsoluteRect> get_selection_rectangle() const;
    //std::optional<OverviewSide> get_resize_side_containing_point(AbsoluteDocumentPos point) const;
    void set_side_to_pos(OverviewSide side, AbsoluteDocumentPos pos);
};

struct Highlight : Annotation {
    static Highlight from_json(const QJsonObject& json_object);

    static inline const std::string TABLE_NAME = "highlights";

    static inline const QString SELECTION_BEGIN_X_COLUMN_NAME = "begin_x";
    static inline const QString SELECTION_BEGIN_Y_COLUMN_NAME = "begin_y";
    AbsoluteDocumentPos selection_begin;

    static inline const QString SELECTION_END_X_COLUMN_NAME = "end_x";
    static inline const QString SELECTION_END_Y_COLUMN_NAME = "end_y";
    AbsoluteDocumentPos selection_end;

    static inline const QString DESCRIPTION_COLUMN_NAME = "desc";
    std::wstring description;

    static inline const QString TEXT_ANNOT_COLUMN_NAME = "text_annot";
    std::wstring text_annot;

    static inline const QString TYPE_COLUMN_NAME = "type";
    char type;

    std::vector<AbsoluteRect> highlight_rects;

    QJsonObject to_json(std::string doc_checksum) const;
    void add_to_tuples(std::vector<std::pair<QString, QVariant>>& tuples) override;

};


struct PdfLink {
    //fz_rect rect;
    std::vector<PagelessDocumentRect> rects;
    int source_page;
    std::string uri;
};


struct ParsedUri {
    int page;
    float x;
    float y;
};

struct FreehandDrawingPoint {
    AbsoluteDocumentPos pos;
    float thickness;
};

enum class SearchCaseSensitivity {
    CaseSensitive,
    CaseInsensitive,
    SmartCase
};

struct DocumentCharacter {
    int c;
    PagelessDocumentRect rect;
    bool is_final = false;
    fz_stext_block* stext_block;
    fz_stext_line* stext_line;
    fz_stext_char* stext_char;
};

struct FreehandDrawing {
    std::vector<FreehandDrawingPoint> points;
    char type;
    float alpha = 1;
    QDateTime creattion_time;
    AbsoluteRect bbox();
};

struct PixmapDrawing {
    QPixmap pixmap;
    AbsoluteRect rect;
};

struct CharacterAddress {
    int page;
    fz_stext_block* block;
    fz_stext_line* line;
    fz_stext_char* character;
    Document* doc;

    CharacterAddress* previous_character = nullptr;

    bool advance(char c);
    bool backspace();
    bool next_char();
    bool next_line();
    bool next_block();
    bool next_page();

    float focus_offset();

};

struct DocumentViewState {
    std::wstring document_path;
    OpenedBookState book_state;
};

struct PortalViewState {
    std::string document_checksum;
    OpenedBookState book_state;
};

struct OverviewResizeData {
    fz_rect original_rect;
    NormalizedWindowPos original_normal_mouse_pos;
    OverviewSide side_index;
};

struct VisibleObjectResizeData {
    VisibleObjectType type;
    int object_index;
    AbsoluteRect original_rect;
    AbsoluteDocumentPos original_mouse_pos;
    OverviewSide side_index;
};

struct VisibleObjectScrollData {
    VisibleObjectType type;
    int object_index;
    float original_scroll_amount;
    std::optional<float> original_scroll_amount_x = {};
    AbsoluteDocumentPos original_mouse_pos;
};


struct OverviewMoveData {
    fvec2 original_offsets;
    NormalizedWindowPos original_normal_mouse_pos;
};

struct OverviewTouchMoveData {
    AbsoluteDocumentPos overview_original_pos_absolute;
    NormalizedWindowPos original_mouse_normalized_pos;
};

/*
    A link is a connection between two document locations. For example when reading a paragraph that is referencing a figure,
    we may want to link that paragraphs's location to the figure. We can then easily switch between the paragraph and the figure.
    Also if helper window is opened, it automatically displays the closest link to the current location.
    Note that this is different from PdfLink which is the built-in link functionality in PDF file format.
*/
struct Portal : Annotation {
    static Portal from_json(const QJsonObject& json_object);

    static inline const std::string TABLE_NAME = "links";

    static Portal with_src_offset(float src_offset);

    static inline const QString DST_DOCUMENT_COLUMN_NAME = "dst_document";
    static inline const QString DST_OFFSET_X_COLUMN_NAME = "dst_offset_x";
    static inline const QString DST_OFFSET_Y_COLUMN_NAME = "dst_offset_y";
    static inline const QString DST_ZOOM_LEVEL_COLUMN_NAME = "dst_zoom_level";
    PortalViewState dst;

    static inline const QString SRC_OFFSET_Y_COLUMN_NAME = "src_offset_y";
    float src_offset_y;

    static inline const QString SRC_OFFSET_X_COLUMN_NAME = "src_offset_x";
    std::optional<float> src_offset_x = {};

    static inline const QString SRC_OFFSET_END_X_COLUMN_NAME = "src_offset_end_x";
    std::optional<float> src_offset_end_x = {};

    static inline const QString SRC_OFFSET_END_Y_COLUMN_NAME = "src_offset_end_y";
    std::optional<float> src_offset_end_y = {};

    mutable bool is_merged_rect_valid = false;
    mutable std::optional<AbsoluteRect> merged_rect = {};

    bool is_visible() const;
    bool is_icon() const;
    bool is_pinned() const;

    QJsonObject to_json(std::string doc_checksum) const;
    void add_to_tuples(std::vector<std::pair<QString, QVariant>>& tuples) override;

    std::optional<AbsoluteRect> get_rectangle() const override;
    AbsoluteRect get_actual_rectangle() const;
    void update_merged_rect(Document* doc) const;
    void set_side_to_pos(OverviewSide side, AbsoluteDocumentPos pos);
};

struct OverviewState {
    float absolute_offset_y;
    float absolute_offset_x = 0;
    float zoom_level = -1;
    Document* doc = nullptr;
    std::optional<std::string> overview_type;
    std::vector<DocumentRect> highlight_rects;
    std::optional<Portal> source_portal = {};
    std::optional<AbsoluteRect> source_rect = {};
    std::optional<float> original_zoom_level;

    float get_zoom_level(DocumentView* dv);
};

bool operator==(const DocumentViewState& lhs, const DocumentViewState& rhs);

struct SearchResult {
    std::vector<fz_rect> rects;
    int page;
    int begin_index_in_page;
    int end_index_in_page;

    void fill(Document* doc);
};


struct TocNode {
    std::vector<TocNode*> children;
    std::wstring title;
    int page;

    float y;
    float x;
};


class Document;

struct CachedPageData {
    Document* doc = nullptr;
    int page;
    float zoom_level;
};

enum class ReferenceType {
    Generic,
    Equation,
    Reference,
    Abbreviation,
    Link,
    RefLink,
    None
};

std::string reference_type_string(ReferenceType rt);

struct SmartViewCandidate {
    Document* doc = nullptr;
    AbsoluteRect source_rect;
    std::wstring source_text;
    std::variant<DocumentPos, AbsoluteDocumentPos> target_pos;
    ReferenceType reference_type = ReferenceType::None;

    // this function lazily computes highlight_rects_ when they are needed
    std::optional<std::function<std::vector<DocumentRect>()>> highlight_rects_func = {};
    bool are_highlights_computed = false;

    const std::vector<DocumentRect> get_highlight_rects();
    Document* get_document(DocumentView* view);
    DocumentPos get_docpos(DocumentView* view);
    AbsoluteDocumentPos get_abspos(DocumentView* view);

private:
    std::vector<DocumentRect> highlight_rects_;
};

/*
    A cached page consists of cached_page_data which is the header that describes the rendered location
    and the actual rendered page. We have two different rendered formats: the pixmap we got from mupdf and
    the cached_page_texture which is an OpenGL texture. The reason we need both formats in the structure is because
    we render the pixmaps in a background mupdf thread, but in order to be able to use a texture in the main thread,
    the texture has to be created in the main thread. Therefore, we fill this structure's cached_page_pixmap value in the
    background thread and then send it to the main thread where we create the texture (which is a relatively fast operation
    so it doesn't matter that it is in the main thread). When cached_page_texture is created, we can safely delete the
    cached_page_pixmap, but the pixmap can only be deleted in the thread that it was created in, so we have to once again,
    send the cached_page_texture back to the background thread to be deleted.
*/
struct CachedPage {
    CachedPageData cached_page_data;
    fz_pixmap* cached_page_pixmap = nullptr;

    // last_access_time is used to garbage collect old pages
    unsigned int last_access_time;
    GLuint cached_page_texture;
};
bool operator==(const CachedPageData& lhs, const CachedPageData& rhs);


/*
    When a document does not have built-in links to the figures, we use a heuristic to find the figures
    and index them in FigureData structure. Using this, we can quickly find the figures when user clicks on the
    text descripbing the figure (for example 'Fig. 2.13')
*/
struct IndexedData {
    int page;
    float y_offset;
    std::wstring text;
};

bool operator==(const Mark& lhs, const Mark& rhs);
bool operator==(const BookMark& lhs, const BookMark& rhs);
bool operator==(const Highlight& lhs, const Highlight& rhs);
bool operator==(const Portal& lhs, const Portal& rhs);

bool are_same(const BookMark& lhs, const BookMark& rhs);

bool are_same(const Highlight& lhs, const Highlight& rhs);

bool has_changed(const BookMark& lhs, const BookMark& rhs);
bool has_changed(const Highlight& lhs, const Highlight& rhs);
bool has_changed(const Portal& lhs, const Portal& rhs);

struct PdfLinkTextInfo {
    std::wstring link_text = L"";
    fz_stext_char* chr = nullptr;
    fz_stext_line* line = nullptr;
    fz_stext_block* block = nullptr;
    int position_in_block = -1;
};
struct TextUnderPointerInfo{
    ReferenceType reference_type;
    std::vector<DocumentPos> targets;
    AbsoluteRect source_rect;
    std::wstring source_text;
    std::vector<DocumentRect> overview_highlight_rects;
};

struct FulltextSearchResult {
    std::string document_checksum;
    int page;
    std::wstring snippet;
    std::wstring document_title = L"";
};

struct DocumentationSearchResult {
    std::wstring snippet;
    std::wstring item_title = L"";
    std::wstring item_type = L"";
};
