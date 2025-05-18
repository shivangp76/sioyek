#pragma once

#include <string>
#include <QString>

class MainWidget;
class Document;
class DocumentView;
class BookMark;

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

};
