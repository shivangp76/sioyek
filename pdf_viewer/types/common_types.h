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

enum DocumentLocationMismatchStrategy {
    Local = 0,
    Server = 1,
    Ask = 2,
    ShowButton = 3
};

enum class ServerStatus {
    NotLoggedIn,
    InvalidCredentials,
    ServerOffline,
    LoggingIn,
    LoggedIn
};

enum class PaperDownloadFinishedAction {
    DoNothing,
    OpenInSameWindow,
    OpenInNewWindow,
    Portal
};

struct AdditionalKeymapData {
    std::wstring file_name;
    int line_number;
    std::wstring keymap_string;
};

enum RenderBackend {
    SioyekNoRendererBackend = 0,
    SioyekOpenGLRendererBackend = 1,
    SioyekQPainterRendererBackend = 2,
    SioyekRhiBackend = 3
};

struct MenuNode {
    QString name;
    QString doc;
    std::vector<MenuNode*> children;
};

enum OverviewSide {
    bottom = 0,
    top = 1,
    left = 2,
    right = 3
};

bool are_same(float f1, float f2);

std::wstring new_uuid();
std::string new_uuid_utf8();

enum class SmoothMoveDirection{
    Up,
    Down,
    Left,
    Right
};
