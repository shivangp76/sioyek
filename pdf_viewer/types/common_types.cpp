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
