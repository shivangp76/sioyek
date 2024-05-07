
import QtQuick 2.2
import QtQuick.Controls 2.15


Rectangle {

    id: rootitem
    color: "black"
    property var root_model: _model

    signal itemSelected(item: string, index: int)

    ListView{
        Component.onCompleted: {
            positionViewAtIndex(_selected_index, ListView.Beginning);
        }
        anchors {
            top: parent.top
            topMargin: 10
            left: rootitem.left
            right: rootitem.right
            bottom: rootitem.bottom
        }
        model: rootitem.root_model
        id: lview
        clip: true


        displaced: Transition{
            PropertyAction {
                properties: "opacity, scale"
                value: 1
            }

            NumberAnimation{
                properties: "x, y"
                duration: 50
            }
        }


        delegate: Rectangle {
            anchors {
                left: parent ? parent.left : undefined
                right: parent ? parent.right : undefined
            }
            height: inner.height * 2.5
            color: lview.model.index(index, 0).row == _selected_index ? "#444": (index % 2 == 0 ? "black" : "#111")
            id: bg

            Item{
                anchors.left: parent.left
                anchors.right: parent.right

                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                Item{
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: parent.width
                    id: inner_container

                    Text {
                        id: inner
                        text: model.display
                        color: "white"
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        font.pixelSize: 15
                        wrapMode: Text.Wrap
                    }
                }

            }

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    lview.currentIndex = index;
                    /* emit */ itemSelected(model.display, index);

                }

            }
        }


    }
}
