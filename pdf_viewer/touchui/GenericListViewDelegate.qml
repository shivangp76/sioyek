
import QtQuick 2.2
import QtQuick.Controls 2.15



Rectangle {
    //property list<int> selection_range: model.get_highlight_positions(model.data(model.display.slice(0, 0), query.text))
    anchors {
        left: parent ? parent.left : undefined
        right: parent ? parent.right : undefined
    }
    height: Math.max(inner.height * 2.5, inner2.height * 1.5)
//            color: index % 2 == 0 ? "black" : "#080808"
    color: lview.model.mapToSource(lview.model.index(index, 0)).row == _selected_index ? "#444": (index % 2 == 0 ? "black" : "#111")
    //color: (index % 2 == 0 ? "black" : "#111")
    id: bg

    Item{
        anchors.left: parent.left
        anchors.right: right_label.left
        //anchors.right: parent.right
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        Item{
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: rootitem.root_model.columnCount() == 3 ? parent.width / 2 : parent.width
            id: inner_container



            Text {
                function getDisplayText(){
                    let selectionRange = lview.model.get_highlight_positions(model.display.toLowerCase(), query.text.toLowerCase());
                    if (selectionRange[0] == -1){
                        return model.display;
                    }
                    return model.display.slice(0, selectionRange[0]) + "<span style='background-color: yellow; color: black;'>" + model.display.slice(selectionRange[0], selectionRange[1]) + "</span>" + model.display.slice(selectionRange[1]);
                }

                id: inner
                text:  getDisplayText()
                color: "white"
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.right: parent.right
                font.pixelSize: 15
                wrapMode: Text.Wrap
                textFormat: Text.RichText
            }
        }
        Item{
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: inner_container.right
            width: rootitem.root_model.columnCount() == 3 ? parent.width / 2 : parent.width
            visible: rootitem.root_model.columnCount() == 3

            Text {
                id: inner2
                text: model.display ? model.display.slice(0, 0) + lview.model.data(lview.model.index(index, 1)) : "";
                color: "white"
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.right: parent.right
                font.pixelSize: 15
                wrapMode: Text.Wrap
            }
        }

    }
    Item{
        anchors.right: parent.right
        width: Math.min(pagetext.contentWidth, parent.width / 2)
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        id: right_label
        Text {
            id: pagetext

            // model.display.slice(0, 0) is a hack to get qml to redraw this widget
            // when model changes there has to be a better way to do this
            text: model.display ? model.display.slice(0, 0) + lview.model.data(lview.model.index(index, rootitem.root_model.columnCount()-1)) : "";
            color: "white"
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 15
        }
        visible: rootitem.root_model.columnCount() >= 2
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
            /* emit */ itemDeleted(model.display, index);
            lview.model.removeRows(index, 1);
        }

    }

    MouseArea {
        anchors.fill: parent

        onPressAndHold: function(event) {
            if (rootitem.deletable){
                deletebutton.visible = true;
            }
            else{
                /* emit */ itemPressAndHold(model.display, index);
            }
        }
        onClicked: {
            lview.currentIndex = index;
            /* emit */ itemSelected(model.display, index);

        }

    }
}