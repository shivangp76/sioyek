#pragma once

#include <string>

class MainWidget;
class Document;
class DocumentView;

class AnnotationController{
private:
    MainWidget* mw;
public:
    AnnotationController(MainWidget* parent);

    void scroll_selected_bookmark(int amount);
    void handle_bookmark_ask_query(std::wstring query, std::wstring bookmark_uuid);
    Document* doc();
    DocumentView* dv();
    DocumentView* mdv();

};
