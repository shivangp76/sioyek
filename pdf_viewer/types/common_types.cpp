#include <quuid.h>

#include "types/common_types.h"

QString ChatMessage::get_message_type_string(){
    if (message_type == ChatMessageType::ResponseMessage){
        return "response";
    }
    if (message_type == ChatMessageType::UserMessage){
        return "user";
    }
    return "";
}


bool are_same(float f1, float f2) {
    return std::abs(f1 - f2) < 0.01;
}

std::wstring new_uuid() {
    return QUuid::createUuid().toString().toStdWString();
}

std::string new_uuid_utf8() {
    return QUuid::createUuid().toString().toStdString();
}
