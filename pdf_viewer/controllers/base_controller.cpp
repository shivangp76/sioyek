#include "controllers/base_controller.h"
#include "main_widget.h"


BaseController::BaseController(MainWidget* parent) : mw(parent){
}

Document* BaseController::doc(){
    return mw->doc();
}
DocumentView* BaseController::dv(){
    return mw->dv();
}

DocumentView* BaseController::mdv(){
    return mw->mdv();
}
