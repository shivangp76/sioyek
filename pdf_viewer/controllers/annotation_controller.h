#pragma once

#include <string>
#include <QString>
#include "book.h"

class MainWidget;
class Document;
class DocumentView;

class AnnotationController{
private:
    MainWidget* mw;
public:
    AnnotationController(MainWidget* parent);

    Document* doc();
    DocumentView* dv();
    DocumentView* mdv();

    void scroll_selected_bookmark(int amount);
    void handle_bookmark_ask_query(std::wstring query, std::wstring bookmark_uuid);
    void pin_current_overview_as_portal();
    void scroll_selected_bookmark_to_end();
    void handle_ask();
    void open_selected_bookmark_in_widget(std::string bookmark_uuid="", bool force_chat=false, bool is_bookmark_pending=false);
    void accept_new_bookmark_message_with_text(QString message);
    void accept_new_bookmark_message();
    void update_current_bookmark_widget_text(BookMark* bm);
    void handle_scroll_selected_bookmark_to_ends(bool goto_start);
    void handle_bookmark_summarize_query(std::wstring bookmark_uuid);
    BookMark* add_chunk_to_bookmark(Document* document, std::string bookmark_uuid, QString chunk);
    void sync_newly_added_annot(const std::string& annot_type, const std::string& uuid);
    void sync_edited_annot(const std::string& annot_type, const std::string& uuid);

    void on_bookmark_edited(BookMark bm, bool was_manual_edit, bool was_move_or_resize);
    void on_highlight_deleted(const Highlight& highlight, const std::string& document_checksum);
    void on_bookmark_deleted(const BookMark& bookmark, const std::string& document_checksum);
    void on_new_bookmark_added(const std::string& uuid);
    void on_new_portal_added(const std::string& uuid);
    void on_portal_deleted(const Portal& uuid, const std::string& document_checksum);
    void on_portal_edited(const std::string& uuid);
    void on_mark_added(const std::string& uuid, char type);
    void on_new_highlight_added(const std::string& uuid);
    void on_highlight_annotation_edited(const std::string& uuid);
    void on_highlight_type_edited(const std::string& uuid);

    void delete_highlight_with_uuid(const std::string& uuid);
    std::optional<Highlight> delete_current_document_highlight_with_uuid(const std::string& uuid);
    void delete_current_document_highlight(Highlight* hl);

    std::string add_highlight_to_current_document(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type);
    std::wstring handle_add_highlight(char symbol);
    void change_selected_highlight_type(char new_type);
    char get_current_selected_highlight_type();

    void handle_goto_portal_list();
    void handle_goto_bookmark(bool manual_only=false, bool chat=false);
    void handle_goto_bookmark_global(bool manual_only=false);
    void handle_goto_highlight();
    void handle_goto_highlight_global();
    void handle_delete_highlight_under_cursor();
    std::optional<Highlight> handle_delete_selected_highlight();
    std::optional<BookMark> handle_delete_selected_bookmark();
    std::optional<Portal> handle_delete_selected_portal();
    void handle_overview_to_portal();
    void handle_special_bookmarks(std::wstring bookmark_text, std::wstring bookmark_uuid);
    std::wstring handle_freetext_bookmark_perform(const std::wstring& text, const std::string& pending_uuid);

    void add_portal(std::wstring source_path, Portal new_link);
    void change_bookmark_text(std::string uuid, const std::wstring& new_text);
    void change_highlight_text_annot(std::string uuid, const std::wstring& new_text);
    std::optional<BookMark> delete_current_document_bookmark(const std::string& uuid);
};
