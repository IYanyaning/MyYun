import QtQuick
import QtQuick.Controls

Rectangle {
    id:fileStyleRec
    width: 100
    height: parent.height
    //border.color: "#5D7DB3"
    ListModel{
        id:typesModel
        ListElement { name: qsTr("默 认") }
        ListElement { name: qsTr("图 片") }
        ListElement { name: qsTr("文 档") }
        ListElement { name: qsTr("视 频") }
        ListElement { name: qsTr("音 频") }
        ListElement { name: qsTr("其 他") }
    }

    ListView{
        id:showFileStyles
        width: parent.width
        height: 600
        model: typesModel
        focus: false
        spacing:5
        delegate: Rectangle{
            width: parent.width
            height: 50
            //border.color: "lightgray"
            border.color: "transparent"
            radius: 15
            property bool hovered: false // 添加用于跟踪鼠标进入和退出状态的属性
            color: (showFileStyles.currentIndex === index || hovered) ? "lightblue" : "transparent"
            Text {
                id:fileStyleText
                anchors.centerIn: parent
                text:  name
                font{
                    family: "Microsoft YaHei"
                    pixelSize: 20
                }
            }
//            TapHandler{
//                onTapped: {
//                    console.log(name)
//                    client.filterFiles(name)
//                }
//            }
            MouseArea{
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                hoverEnabled: true
                onEntered: {
                    hovered = true // 更新 hovered 属性来跟踪鼠标的进入和退出状态。
                }
                onExited: {
                   hovered = false // 更新 hovered 属性来跟踪鼠标的进入和退出状态。
                }
                onClicked: {
                    showFileStyles.currentIndex = index
                    client.filterFiles(name)
                }
            }

        }
    }
}
