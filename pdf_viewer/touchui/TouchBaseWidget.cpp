#include "TouchBaseWidget.h"

TouchShowKeyboardWidget::TouchShowKeyboardWidget(QWidget* parent) : QWidget(parent) {
}

void TouchShowKeyboardWidget::connect_show_signal(){
    QObject::connect(
        dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(showKeyboard()),
        this,
        SLOT(handleShowKeyboard()));
}

void TouchShowKeyboardWidget::handleShowKeyboard(){
    qDebug() << "show keyboard called";
    quick_widget->setFocus();
}
