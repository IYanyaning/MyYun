import QtQuick
import QtQuick.Controls

Rectangle {
    id:processStyleRec
    width: 100
    height: parent.height
    border.color: "#5D7DB3"
    ListModel{
        id:typesModel
        ListElement { name: qsTr("正在上传") }
        ListElement { name: qsTr("正在下载") }
        ListElement { name: qsTr("传输完成") }
    }

    ListView{
        id:showProcess
        width: parent.width
        height: 600
        model: typesModel
        focus: false
        delegate: Rectangle{
            width: parent.width
            height: 50
            border.color: "blue"
            Text {
                id:processStyleText
                anchors.centerIn: parent
                text:  name
                font{
                    family: "Microsoft YaHei"
                    pixelSize: 14
                }
            }
            TapHandler{
                onTapped: {
                    console.log("showProcess")
                }
            }
        }
    }
}
