import QtQuick
import QtQuick.Controls

Rectangle {
    id:mainMenuRec

    //border.color: "#464879"

    ListModel{
        id:typesModel
        ListElement { name: qsTr("首 页") }
        ListElement { name: qsTr("下 载") }
        ListElement { name: qsTr("分 享") }
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
            radius: 15
            //border.color: "lightgray"
            border.color: "transparent"
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
//                    if( name === "首 页"){
//                        firstPage.visible = true
//                        secondPage.visible = false
//                        thirdPage.visible = false
//                    }else if( name === "下 载"){
//                        firstPage.visible = false
//                        secondPage.visible = true
//                        thirdPage.visible = false
//                    }else if( name === "分 享"){
//                        firstPage.visible = false
//                        secondPage.visible = false
//                        thirdPage.visible = true
//                    }
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
                    if( name === "首 页"){
                        firstPage.visible = true
                        secondPage.visible = false
                        thirdPage.visible = false
                    }else if( name === "下 载"){
                        firstPage.visible = false
                        secondPage.visible = true
                        thirdPage.visible = false
                    }else if( name === "分 享"){
                        firstPage.visible = false
                        secondPage.visible = false
                        thirdPage.visible = true
                    }
                }
            }
        }
    }
}
