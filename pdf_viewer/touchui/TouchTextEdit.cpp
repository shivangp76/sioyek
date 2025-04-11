#include "touchui/TouchTextEdit.h"
#include "utils.h"
#include <QVBoxLayout>


TouchTextEdit::TouchTextEdit(QString name, QString initial_value, bool is_password, QWidget* parent) : QWidget(parent) {


    setAttribute(Qt::WA_NoMousePropagation);

    quick_widget = new QQuickWidget(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(quick_widget);
    setLayout(layout);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    quick_widget->rootContext()->setContextProperty("_initialValue", initial_value);
    quick_widget->rootContext()->setContextProperty("_name", name);
    quick_widget->rootContext()->setContextProperty("_isPassword", is_password);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchTextEdit.qml"));

    QObject::connect(
        dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(confirmed(QString)),
        this,
        SLOT(handleConfirm(QString)));
    QObject::connect(
        dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(cancelled()),
        this,
        SLOT(handleCancel()));
    quick_widget->setFocus();

}

void TouchTextEdit::set_text(const std::wstring& txt) {
    quick_widget->rootContext()->setContextProperty("_initialValue", QString::fromStdWString(txt));
}


void TouchTextEdit::handleConfirm(QString text) {
    emit confirmed(text);
}

void TouchTextEdit::handleCancel() {
    emit cancelled();
}

QRect TouchTextEdit::get_prefered_rect(QRect parent_rect){
    // QWidget::resizeEvent(resize_event);

    int parent_width = parent_rect.width();
    int parent_height = parent_rect.height();

    float twenty_cm = 20 * logicalDpiX() / 2.54f;

    int w = static_cast<int>(std::min(parent_width * 0.8f, twenty_cm));
    int h = static_cast<int>(std::min(parent_height * 0.5f, twenty_cm));
    return QRect((parent_width - w) / 2, (parent_height - h) / 2, w, h);
}

void TouchTextEdit::keyPressEvent(QKeyEvent* kevent) {
    if (kevent->key() == Qt::Key_Return) {
        kevent->accept();
        return;
    }
    QWidget::keyPressEvent(kevent);
    //return true;
}
