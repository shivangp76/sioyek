
import QtQuick 2.2
import QtQuick.Controls 2.15

import "qrc:/pdf_viewer/touchui"

Rectangle {

    function highlightQuery(txt){

        let queryString = query.text.toLowerCase();
        if (queryString.length > 2  && queryString[1] == ' '){
            queryString = queryString.slice(2);
        }

        let selectionRange = lview.model.get_highlight_positions(txt.toLowerCase(), queryString);
        if ((selectionRange[0] === -1) || (selectionRange[1] - selectionRange[0] > queryString.length * 1.5)){
            return txt;
        }
        return txt.slice(0, selectionRange[0]) + "<span style='background-color: yellow; color: black;'>" + txt.slice(selectionRange[0], selectionRange[1]) + "</span>" + txt.slice(selectionRange[1]);
    }

    id: rootitem
    color: "black"
    property bool deletable: _deletable || false
    property var root_model: _model
    property Component delegateComponent: GenericListViewDelegate{}

    property alias listModel: lview.model
    property alias listView: lview


    signal itemSelected(item: string, index: int)
    signal itemPressAndHold(item: string, index: int)
    signal itemDeleted(item: string, index: int)

    TextInput{
        id: query
        color: "white"
        inputMethodHints: Qt.ImhSensitiveData | Qt.ImhPreferLowercase
        focus: _focus
        activeFocusOnTab: true

        anchors {
            top: rootitem.top
            left: rootitem.left
            right: rootitem.right
            leftMargin: 10
            topMargin: 10
        }

        font.pixelSize: 15

        Text { // palceholder text
            text: "Search"
            color: "#888"
            visible: !query.text
            font.pixelSize: 15
        }


        onTextChanged: {
            //lview.model.setFilterRegularExpression(text);
            lview.model.setFilterCustom(text);
            //rootitem.root_model.setFilterRegularExpression(text);
        }

        onAccepted: {
            let item = lview.model.data(lview.model.index(0, 0));
            //console.log(item);
            rootitem.itemSelected(item, 0);
        }

    }


    ListView{
//        model: MyModel {}
        Component.onCompleted: {
            positionViewAtIndex(_selected_index, ListView.Beginning);
        }
        anchors {
            top: query.bottom
            topMargin: 10
            left: rootitem.left
            right: rootitem.right
            bottom: rootitem.bottom
        }
        //model: _model
        model: rootitem.root_model
        id: lview
        clip: true
        //anchors.fill: parent


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

        delegate: delegateComponent
    }
}