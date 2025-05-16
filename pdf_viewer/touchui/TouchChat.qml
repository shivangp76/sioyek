import QtQuick 2.2
import QtQuick.Controls 2.15

Rectangle {
    id: rootitem
    property int anchorMargin: 15
    color: _chat_background

    signal onMessageSend(message: string);
    signal anchorClicked(link: string);
    signal showKeyboard();

    ListView {
        id: message_list
        anchors.top: parent.top
        anchors.bottom: message_input.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: rootitem.anchorMargin

        model: _messages

        delegate: Item {
            width: parent.width
            height: background.height
            Rectangle {
                property bool is_user: (_messages[index].message_type == "user")
                width: is_user ? Math.min(parent.width * 3 / 4, message_text.implicitWidth + 2 * rootitem.anchorMargin) : (parent.width - rootitem.anchorMargin)
                height: message_text.implicitHeight + 2 * rootitem.anchorMargin
                color: is_user ? _user_background : rootitem.color
                // the radius should be the same as the font size
                radius: message_text.font.pixelSize * 2
                anchors.right: parent.right
                anchors.rightMargin: rootitem.anchorMargin
                // anchors.margins: rootitem.anchorMargin
                id: background

                Text {
                    id: message_text
                    text: _messages[index].message
                    color: background.is_user ? _user_text_color : _text_color
                    textFormat: Text.MarkdownText
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: rootitem.anchorMargin
                    wrapMode: Text.Wrap
                    font: _font_name

                    onLinkActivated: {
                        /* emit */ anchorClicked(link)
                    }
                }
            }
        }
    }
    Rectangle{
        id: message_input
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 50
        color: input_text.focus ? "#333" : "#222"

        function handle_submit(){
            if (input_text.text.length > 0) {
                /* emit */ onMessageSend(input_text.text);
                input_text.text = "";
            }
        }

        TextInput{
            color: "white"
            anchors.left: parent.left
            anchors.right: submit_button.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: rootitem.anchorMargin
            inputMethodHints: Qt.ImhPreferLowercase | Qt.ImhNoPredictiveText
            // anchors.verticalCenter: parent.verticalCenter
            verticalAlignment: "AlignVCenter"
            font: _font_name
            enabled: !_pending
            id: input_text

            onActiveFocusChanged: {
                if (activeFocus){
                    /* emit */ showKeyboard();
                }
            }

            onAccepted: {
                message_input.handle_submit();
            }

        }


        Text{
            id: placeholder_text
            anchors.left: input_text.anchors.left
            anchors.right: input_text.anchors.right
            anchors.top: input_text.anchors.top
            anchors.bottom: input_text.anchors.bottom
            anchors.leftMargin: rootitem.anchorMargin
            verticalAlignment: "AlignVCenter"
            color: "#555"
            visible: (!input_text.focus && input_text.text.length == 0) || _pending
            text: _pending ? "Waiting for response ..." : "Ask"

        }

        Button{
            id: submit_button
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.top: parent.top
            anchors.margins: 5
            text: "Send"
            visible: !_pending
            onClicked: {
                message_input.handle_submit();
            }
        }
    }
}
