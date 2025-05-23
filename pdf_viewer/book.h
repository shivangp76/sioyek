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

#include "coordinates.h"

class DocumentView;
class Document;
class MainWidget;


enum class DrawingMode {
    NotDrawing=0,
    Drawing=1,
    PenDrawing=2
};


enum class ColorPalette {
    Normal,
    Dark,
    Custom,
    NoPalette
};

enum class SelectedObjectType {
    Drawing,
    Pixmap
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
    bool is_server_only = false;
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

struct DocumentViewState {
    std::wstring document_path;
    OpenedBookState book_state;
};




bool operator==(const DocumentViewState& lhs, const DocumentViewState& rhs);

struct SearchResult {
    std::vector<PagelessDocumentRect> rects;
    int page;
    int begin_index_in_page;
    int end_index_in_page;
    QString message;

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
    NoReference
};

struct PaperNameWithRects{
    QString paper_name;
    std::vector<PagelessDocumentRect> character_rects;
};

std::string reference_type_string(ReferenceType rt);

struct SmartViewCandidate {
    Document* doc = nullptr;
    AbsoluteRect source_rect;
    std::wstring source_text;
    std::variant<DocumentPos, AbsoluteDocumentPos> target_pos;
    ReferenceType reference_type = ReferenceType::NoReference;

    // this function lazily computes highlight_rects_ when they are needed
    std::optional<std::function<std::vector<DocumentRect>()>> highlight_rects_func = {};
    std::optional<QString> target_reference_text = {};
    bool are_highlights_computed = false;

    const std::vector<DocumentRect> get_highlight_rects();
    void set_highlight_rects(std::vector<DocumentRect> rects);
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


struct PdfLinkTextInfo {
    std::wstring link_text = L"";
    fz_stext_char* chr = nullptr;
    fz_stext_line* line = nullptr;
    fz_stext_block* block = nullptr;
    int position_in_block = -1;
};
struct TextUnderPointerInfo{
    std::vector<SmartViewCandidate> candidates;
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

