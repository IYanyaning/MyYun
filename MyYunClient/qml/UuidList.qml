import QtQuick
import QtQuick.Controls

Rectangle {
    id:uuidRec
    width: 450
    height: parent.height
    //border.color:  "#5D7DB3"
    border.color: "transparent"
    ListModel{
        id:uuidModel
    }
    ListView{
        id:showUuid
        width: parent.width
        height: parent.height
        model: uuidModel
        focus: false
        clip: true
        spacing: 5
        delegate: Rectangle{
            width: parent.width
            height: 50
            radius: 15
            //border.color: "lightgray"
            border.color: "transparent"
            property bool hovered: false // 添加用于跟踪鼠标进入和退出状态的属性
            color: (showUuid.currentIndex === index || hovered) ? "lightblue" : "transparent"
            Text {
                id:uuidText
                anchors.centerIn: parent
                text:  uuid

                focus: false
                font{
                    family: "Microsoft YaHei"
                    pixelSize: 20
                }
            }
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
                    showUuid.currentIndex = index
                    client.getSharedFilesListFromUuid(uuid)
                }
            }

//            TapHandler{
//                onTapped: {
//                    client.getSharedFilesListFromUuid(uuid)
//                }
//                onDoubleTapped: {
//                    //Qt.application.clipboard.clear()
//                    console.log(uuidText.text)

//                }
//            }

        }
    }
    Connections{
        //target: client
        target: myNetworkManager
        onGetSharedUUidList:{
            uuidModel.clear()
            console.log(uuidList.length)
            for(var i = 0; i < uuidList.length; ++i){
                uuidModel.append({"uuid" : uuidList[i]})
            }
        }
    }
}
