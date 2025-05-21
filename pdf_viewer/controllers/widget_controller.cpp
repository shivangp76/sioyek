#include "controllers/widget_controller.h"
#include "main_widget.h"

WidgetController::WidgetController(MainWidget* parent) : BaseController(parent) {
}

void WidgetController::push_widget(QWidget* w){
    mw->push_current_widget(w);
}

void WidgetController::pop_widget(){
    mw->pop_current_widget();
}

void WidgetController::set_current_widget(QWidget* w){
    mw->set_current_widget(w);
}

QWidget* WidgetController::current_widget(){

    if (mw->current_widget_stack.size() == 0){
        return nullptr;
    }

    return mw->current_widget_stack.back();
}

QWidget* WidgetController::parent(){
    return mw;
}
