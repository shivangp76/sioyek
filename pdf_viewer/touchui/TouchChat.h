#pragma once

#include <QWidget>
#include <QQuickWidget>
#include <QStringListModel>
#include "TouchBaseWidget.h"
#include "types/common_types.h"

class TouchChat : public TouchBaseWidget {
    Q_OBJECT
public:
    TouchChat(QWidget* parent);
    void set_messages(QList<ChatMessage> messages);
    void set_pending(bool is_pending);
    void initialize_widget() override;
    // void update_context_properties();

public slots:
    void handleOnMessageSend(QString);
    void handleAnchorClicked(QString);

signals:
    void onMessageSend(QString);
    void anchorClicked(QString);

};
