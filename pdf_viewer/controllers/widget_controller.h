#pragma once

#include "controllers/base_controller.h"

class QWidget;

class WidgetController : public BaseController{
public:
    WidgetController(MainWidget* parent);
    void push_widget(QWidget* w);
    void set_current_widget(QWidget* w);
    void pop_widget();
    QWidget* current_widget();
    QWidget* parent();
};
