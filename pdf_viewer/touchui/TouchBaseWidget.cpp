#include "TouchBaseWidget.h"

TouchBaseWidget::TouchBaseWidget(QWidget* parent) : QWidget(parent) {
}

void TouchBaseWidget::initialize_base(){
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

void TouchBaseWidget::connect_show_signal(){
    QObject::connect(
        dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(showKeyboard()),
        this,
        SLOT(handleShowKeyboard()));
}

void TouchBaseWidget::handleShowKeyboard(){
    quick_widget->setFocus();
}

void TouchBaseWidget::resizeEvent(QResizeEvent* resize_event) {
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}
