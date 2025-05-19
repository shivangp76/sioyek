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
    void on_bookmark_edited(BookMark bm, bool was_manual_edit, bool was_move_or_resize);
    void delete_highlight_with_uuid(const std::string& uuid);
    std::optional<Highlight> delete_current_document_highlight_with_uuid(const std::string& uuid);
};
