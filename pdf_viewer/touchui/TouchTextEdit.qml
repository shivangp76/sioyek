
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"

Rectangle {

    color: "black"

    signal confirmed(text: string);
    signal cancelled();
    id: root


    Label{
        text: _name
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: parent.height / 8
        color: "white"
        id: label
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

    }

    Item{
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: label.bottom
        id: edit
        height: 3 * parent.height / 4
        property alias loaderItem:textLoader.item
        Component {
            id: normalInput
            TextInput{
                anchors.fill: parent
                color: "white"
                text: _initialValue
                focus: true
                id: textView
                wrapMode: TextInput.WrapAnywhere
                onAccepted:{
                    root.confirmed(edit.loaderItem.text);
                }
            }
        }

        Component {
            id: passwordInput
            TextInput{
                anchors.fill: parent
                color: "white"
                text: _initialValue
                focus: true
                id: textView
                echoMode: TextInput.Password
                wrapMode: TextInput.WrapAnywhere
                onAccepted:{
                    root.confirmed(edit.loaderItem.text);
                }
            }
        }

        Loader{
            anchors.fill: parent
            id: textLoader
            sourceComponent: _isPassword ? passwordInput : normalInput
        }
    }

    TouchButtonGroup{
        anchors.top: edit.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        buttons: ["Cancel", "Confirm"]
        onButtonClicked: function(index, name){
            if (index == 1){
                root.confirmed(edit.loaderItem.text);
            }
            if (index == 0){
                root.cancelled();
            }
        }
    }

}

