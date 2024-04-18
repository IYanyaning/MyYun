import QtQuick
import QtQuick.Controls

Rectangle {
    id:sharedFilesRec
    Rectangle{
        id:topRec
        width: sharedFilesRec.width
        height: 50
        Text {
            id: tit
            text:  qsTr("文件名")
            font{
                family: "Microsoft YaHei"
                pixelSize: 20
            }
            anchors.centerIn: parent
        }
    }
    ScrollView{
        id:shareFilesScroll
        width: parent.width
        height: parent.height - topRec.height
        anchors.top: topRec.bottom
        Rectangle{
            id:bottRec
            width: sharedFilesRec.width
            height: sharedFilesRec.height - topRec.height
            //border.color: "lightgray"
            border.color: "transparent"
            //anchors.top: topRec.bottom
            Row{
                spacing:10
                ListView{
                    id:showFiles
                    width: bottRec.width - spacing
                    height: bottRec.height
                    model: showFilesModel
                    clip: true
                    focus: true

                    delegate: Rectangle{
                        width: showFiles.width
                        height: 50
                        radius: 15
                        //border.color: "lightgray"
                        border.color: "transparent"
                        property bool hovered: false // 添加用于跟踪鼠标进入和退出状态的属性
                        color: (showFiles.currentIndex === index || hovered) ? "lightblue" : "transparent"
                        Rectangle{
                            id:singleFile
                            color: "white"
                            width: parent.width
                            height: 50
                            border.color: "lightgray"
                            //border.color: "transparent"
                            Text {
                                id: singleFileName
                                text:  fileName
                                font{
                                    family: "Microsoft YaHei"
                                    pixelSize: 20
                                }
                                anchors.centerIn: parent
                            }
                            Menu{
                                id:menu
                                width: 50
                                //height: 50
                                visible: false
                                MenuItem{
                                    text: qsTr("下载")
                                    onTriggered: {
                                        downloadDialog.filename = singleFileName.text
                                        downloadDialog.open()
                                    }
                                }
                                MenuItem{
                                    text: qsTr("预览")
                                    onTriggered: {
                                        viewFileName = singleFileName.text
                                        checkFileType(viewFileName)
                                    }
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
                                    if(mouse.button === Qt.LeftButton){
                                        showFiles.currentIndex = index
                                    }else if(mouse.button === Qt.RightButton){
                                        menu.x = mouseX
                                        menu.y = mouseY
                                        menu.visible = true
                                    }
                                }
                                onDoubleClicked: {
                                    if(mouse.button === Qt.LeftButton){
                                        thirdPage.viewFileName = singleFileName.text
                                        checkFileType(viewFileName)
                                    }
                                }
                            }
                        }
                    }
                }
                ListModel{
                    id:showFilesModel
                }
            }

        }
    }

    Connections{
        //target: client
        target: myNetworkManager
        onGetSharedFilesList:{
            showFilesModel.clear()
            for(var i = 0; i < filesList.length; ++i){
                //console.log(filesList[i])
                showFilesModel.append({fileName:filesList[i]})
            }
        }
    }
}
