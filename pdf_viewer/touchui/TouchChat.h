#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QStringListModel>
#include "TouchBaseWidget.h"
#include "book.h"

class TouchChat : public TouchShowKeyboardWidget {
    Q_OBJECT
public:
    TouchChat(QWidget* parent);
    void resizeEvent(QResizeEvent* resize_event) override;
    void set_messages(QList<ChatMessage> messages);
    void set_pending(bool is_pending);
    // void update_context_properties();

public slots:
    // void handleSelectText();
    void handleOnMessageSend(QString);
    void handleAnchorClicked(QString);
    // void handleShowKeyboard();

signals:
    void onMessageSend(QString);
    void anchorClicked(QString);

};
