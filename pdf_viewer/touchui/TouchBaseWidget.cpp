#include "TouchBaseWidget.h"

TouchShowKeyboardWidget::TouchShowKeyboardWidget(QWidget* parent) : QWidget(parent) {
}

void TouchShowKeyboardWidget::initialize_base(){
    setAttribute(Qt::WA_NoMousePropagation);

    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    initialize_widget();

    connect_show_signal();
    setFocusProxy(quick_widget);
    quick_widget->setFocus();
}

void TouchShowKeyboardWidget::connect_show_signal(){
    QObject::connect(
        dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(showKeyboard()),
        this,
        SLOT(handleShowKeyboard()));
}

void TouchShowKeyboardWidget::handleShowKeyboard(){
    quick_widget->setFocus();
}

void TouchShowKeyboardWidget::resizeEvent(QResizeEvent* resize_event) {
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}
