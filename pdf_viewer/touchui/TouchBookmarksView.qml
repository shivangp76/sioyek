
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


        function getBackgroundColor(bm){
            let bgColor = bm.get_background_color();
            if (bgColor === undefined){
                bgColor = Qt.rgba(0.0, 0.0, 0.0, 1);
            }
            if (mosuearea.pressed){
                // return a lighter color
                return Qt.rgba(bgColor.r + 0.1, bgColor.g + 0.1, bgColor.b + 0.1, 1);
            }
            return bgColor;
        }

        function getTextColor(bm){
            let textColor = bm.get_text_color();
            let bgColor = getBackgroundColor(bm);

            if (textColor === undefined){
                if (isColorDark(bgColor)) {
                    return "white";
                }
                else{
                    return "black";
                }
            }
            else{
                if (textColor == bgColor){
                    // return inverted background color
                    return Qt.rgba(1 - bgColor.r, 1 - bgColor.g, 1 - bgColor.b, 1);
                }
                return textColor;
            }
        }

        property var bookmark:  listModel.data(listModel.index(index, 1))
        property string displayText: bookmark.get_render_text()
        property string fileNameOrPageNumber: listModel.data(listModel.index(index, 2))

        height: textContainer.height + 30

        color: getBackgroundColor(bookmark)

        Item{
            id: textContainer
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            height: highlight_text.height + location_text.height

            Text {

                id: highlight_text
                text: highlightQuery(displayText)
                color: getTextColor(bookmark)

                // anchors.verticalCenter: parent.verticalCenter
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                font.pixelSize: 15
                wrapMode: Text.Wrap
                textFormat: Text.MarkdownText
            }

            Text{
                id: location_text
                anchors.top: highlight_text.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                color: "#ddd"

                horizontalAlignment: Text.AlignRight
                anchors.rightMargin: 20

                text: "<code>" + fileNameOrPageNumber + "</code>"
                textFormat: Text.RichText
            }

        }

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

        // draw a faint divider
        Rectangle {
            width: parent.width
            height: 1
            color: "#222"
            anchors.bottom: parent.bottom
        }

    }
}
