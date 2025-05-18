#pragma once

#include <QObject>
#include <QString>

enum class ChatMessageType {
    UserMessage,
    ResponseMessage
};

struct ChatMessage {
    Q_GADGET
    Q_PROPERTY(QString message MEMBER messgae)
    Q_PROPERTY(QString message_type READ get_message_type_string)
public:
    ChatMessageType message_type;
    QString messgae;

    QString get_message_type_string();
};
