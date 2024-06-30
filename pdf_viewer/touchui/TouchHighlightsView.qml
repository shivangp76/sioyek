
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"

TouchListView{
    id: lview

    function isColorDark(color){
        return color.r * 0.299 + color.g * 0.587 + color.b * 0.114 < 0.65;
    }

    delegateComponent: Rectangle{
        anchors {
            left: parent ? parent.left : undefined
            right: parent ? parent.right : undefined
        }
        anchors.leftMargin: 20
        anchors.rightMargin: 20



        // height: Math.max(inner.height * 2.5, inner2.height * 1.5)
        // height: 20
        property color hlColor: _colorMap["" + listModel.data(listModel.index(index, 1))]
        property string displayText: listModel.data(listModel.index(index, 0))
        property string fileNameOrPageNumber: listModel.data(listModel.index(index, 3))
        property string annotText: listModel.data(listModel.index(index, 2))
        property string typeString: String.fromCharCode(listModel.data(listModel.index(index, 1)))
        property color hlTextColor: isColorDark(hlColor) ? "white" : "black"

        height: textContainer.height + 30

        // color: mosuearea.pressed ? "#111" : "black"
        color: mosuearea.pressed ? "#222" : (listModel.mapToSource(listModel.index(index, 0)).row == _selected_index ? "#444": "black")

        Item{
            id: textContainer
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: highlight_text.height + highlight_annot.height + location_text.height

            Text {

                id: highlight_text
                text: "<span style=\"background-color: " + hlColor + "; color: " + hlTextColor + ";\"><code>&nbsp;"+ typeString + "&nbsp;</code></span> &nbsp;" + highlightQuery(displayText)
                color: "white"

                // anchors.verticalCenter: parent.verticalCenter
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                font.pixelSize: 15
                wrapMode: Text.Wrap
                textFormat: Text.RichText
            }

            Rectangle {
                width: parent.width
                height: 1
                color: "#444"
                anchors.top: highlight_text.bottom
                visible: annotText.length > 0
            }
            Text {
                id: highlight_annot
                text: highlightQuery(annotText)
                // text: fileNameOrPageNumber
                color: "white"

                anchors.top: highlight_text.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                font.pixelSize: 15
                wrapMode: Text.Wrap
                textFormat: Text.RichText
            }
            Text{
                id: location_text
                anchors.top: highlight_annot.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                color: "#ddd"

                horizontalAlignment: Text.AlignRight
                anchors.rightMargin: 20

                text: "<code>[" + fileNameOrPageNumber + "]</code>"
                textFormat: Text.RichText
            }

        }

        // draw a separator
        // Rectangle {
        //     width: parent.width
        //     height: 1
        //     color: "#222"
        //     anchors.bottom: parent.bottom
        // }

        
        Button{
            anchors.right: parent.right
            anchors.top:  parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 10
            z: 10

            id: deletebutton
            text: "Delete"
            visible: false

            onClicked: {
                /* emit */ itemDeleted(displayText, index);
                listModel.removeRows(index, 1);
            }

        }
        MouseArea {
            id: mosuearea
            anchors.fill: parent

            onPressAndHold: function(event) {
                let deletable = _deletable || false;

                if (deletable){
                    console.log('showing delete button');
                    deletebutton.visible = true;
                }
                else{
                    /* emit */ itemPressAndHold(displayText, index);
                }
            }

            onClicked: {
                listView.currentIndex = index;
                /* emit */ itemSelected(displayText, index);

            }

        }

    }
}