#pragma once

class MainWidget;
class Document;
class DocumentView;

class BaseController{
protected:
    MainWidget* mw;
public:
    BaseController(MainWidget* parent);

    Document* doc();
    DocumentView* dv();
    DocumentView* mdv();
};
