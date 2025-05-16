#include "touchui/TouchChat.h"
#include <QQuickItem>
#include <QQmlContext>
#include <QStringListModel>
#include "utils.h"

extern float CHAT_WINDOW_BACKGROUND_COLOR[3];
extern float CHAT_WINDOW_USER_MESSAGE_BACKGROUND_COLOR[3];
extern float CHAT_WINDOW_TEXT_COLOR[3];
extern float CHAT_WINDOW_USER_TEXT_COLOR[3];

TouchChat::TouchChat(QWidget* parent) : QWidget(parent) {

    setAttribute(Qt::WA_NoMousePropagation);

    quick_widget = new QQuickWidget(this);

    quick_widget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
    quick_widget->setAttribute(Qt::WA_AlwaysStackOnTop);
    quick_widget->setClearColor(Qt::transparent);

    QList<ChatMessage> messages;

    QColor user_background_color = qconvert_color3(CHAT_WINDOW_USER_MESSAGE_BACKGROUND_COLOR, ColorPalette::Normal);
    QColor background_color = qconvert_color3(CHAT_WINDOW_BACKGROUND_COLOR, ColorPalette::Normal);
    QColor text_color = qconvert_color3(CHAT_WINDOW_TEXT_COLOR, ColorPalette::Normal);
    QColor user_text_color = qconvert_color3(CHAT_WINDOW_USER_TEXT_COLOR, ColorPalette::Normal);
    QString font_name = get_chat_font_face_name();

    quick_widget->rootContext()->setContextProperty("_messages", QVariant::fromValue(messages));
    quick_widget->rootContext()->setContextProperty("_user_background", user_background_color);
    quick_widget->rootContext()->setContextProperty("_chat_background", background_color);
    quick_widget->rootContext()->setContextProperty("_text_color", text_color);
    quick_widget->rootContext()->setContextProperty("_user_text_color", user_text_color);
    quick_widget->rootContext()->setContextProperty("_font_name", font_name);
    quick_widget->rootContext()->setContextProperty("_pending", false);

    quick_widget->setSource(QUrl("qrc:/pdf_viewer/touchui/TouchChat.qml"));


    QObject::connect(
        dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(onMessageSend(QString)),
        this,
        SLOT(handleOnMessageSend(QString)));

    QObject::connect(
        dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(anchorClicked(QString)),
        this,
        SLOT(handleAnchorClicked(QString)));

    QObject::connect(
        dynamic_cast<QObject*>(quick_widget->rootObject()),
        SIGNAL(showKeyboard()),
        this,
        SLOT(handleShowKeyboard()));

    setFocusProxy(quick_widget);
    quick_widget->setFocus();

}


void TouchChat::handleOnMessageSend(QString message) {
    emit onMessageSend(message);
}

void TouchChat::handleAnchorClicked(QString message) {
    emit anchorClicked(message);
}

void TouchChat::handleShowKeyboard() {
    emit showKeyboard();
}

void TouchChat::resizeEvent(QResizeEvent* resize_event) {
    quick_widget->resize(resize_event->size().width(), resize_event->size().height());
    QWidget::resizeEvent(resize_event);

}

void TouchChat::set_messages(QList<ChatMessage> messages){

    quick_widget->rootContext()->setContextProperty("_messages", QVariant::fromValue(messages));
    // model->setStringList(messages);
    // quick_widget->rootContext()->setContextProperty("_messages", model);
}

void TouchChat::set_pending(bool is_pending){
    quick_widget->rootContext()->setContextProperty("_pending", is_pending);
}
