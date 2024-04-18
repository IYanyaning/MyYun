import QtQuick
import QtQuick.Controls

Rectangle {
    id:shareDialog
    width: 450
    height: 300
    anchors.centerIn: parent
    function setSharedFileName(){
        if(fileNames.length > 1){
            filename.text = fileNames[0] + "等" + fileNames.length + "个文件"
        }else{
            filename.text = fileNames[0]
        }

    }
    Rectangle{
        id:sonRec
        anchors.leftMargin: 20
        anchors.left: parent.left
        width: 500
        height: parent.height
        color: "#B6CAD7"
        Column{
            spacing: 10
            Rectangle{
                id:head
                width: sonRec.width
                height: 60

                Rectangle{
                    width: parent.width
                    height: parent.height
                    color: "#B6CAD7"
                    anchors.centerIn: parent

                    Label{
                        id:shareLable
                        text: qsTr("分享：")
                        font.pixelSize: 16
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text{
                        id:filename
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: shareLable.right
                        font.pixelSize: 16
                        text: ""
                    }
                }
            }
            Rectangle{
                id:time
                width: sonRec.width
                height: 60
                color: "#B6CAD7"
                Label{
                    id:timeLabel
                    text: qsTr("有效期：")
                    font.pixelSize: 16
                    anchors.verticalCenter: time.verticalCenter
                }
                RadioButton{
                    id:oneDay
                    anchors.left: timeLabel.right
                    anchors.leftMargin: 30
                    text: "1天"
                    font.pixelSize: 16
                    anchors.verticalCenter: time.verticalCenter
                }
                RadioButton{
                    id:sevenDay
                    anchors.left: oneDay.right
                    anchors.leftMargin: 20
                    text: "7天"
                    font.pixelSize: 16
                    anchors.verticalCenter: time.verticalCenter
                }
                RadioButton{
                    id:thirtyDay
                    anchors.left: sevenDay.right
                    anchors.leftMargin: 20
                    text: "30天"
                    font.pixelSize: 16
                    anchors.verticalCenter: time.verticalCenter
                }
            }
            Rectangle{
                id:fetch
                width: sonRec.width
                height: 60
                color: "#B6CAD7"
                Label{
                    id:fetchLabel
                    text: qsTr("提取码：")
                    font.pixelSize: 16
                    anchors.verticalCenter: fetch.verticalCenter
                }
                RadioButton{
                    text:qsTr("系统随机生成提取码")
                    font.pixelSize: 16
                    anchors.verticalCenter: fetch.verticalCenter
                    anchors.left: fetchLabel.right
                    anchors.leftMargin: 20
                    checked: true
                    checkable: false
                }
            }
            Rectangle{
                width: sonRec.width
                height: 60

                Rectangle{
                    width: parent.width
                    height: parent.height
                    color: "#B6CAD7"
                    Rectangle{
                        id:cancel
                        width: 80
                        height: 30
                        radius: 45
                        color: "#87cefa"
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: submmit.right
                        anchors.rightMargin: 100
                        Text{
                            text: "取消"
                            font{
                                family: "Microsoft YaHei"
                                pixelSize: 20
                            }
                            anchors.centerIn: parent
                        }
                        TapHandler{
                            onTapped: {
                                var day = oneDay.checked ? 1 : (sevenDay.checked ? 7 : 30)
                                client.shareFiles(fileNames, day)
                                shareDialog.visible = false
                                mainColumn.enabled = true
                                shareDialog.z -= 1
                            }
                        }
                    }
                    Rectangle{
                        id:submmit
                        width: 80
                        height: 30
                        radius: 45
                        color: "#87cefa"
                        anchors.verticalCenter: parent.verticalCenter
                        Text{
                            text: "提交"
                            font{
                                family: "Microsoft YaHei"
                                pixelSize: 20
                            }
                            anchors.centerIn: parent
                        }
                        TapHandler{
                            onTapped: {
                                var day = oneDay.checked ? 1 : (sevenDay.checked ? 7 : 30)
                                console.log(day)
                                client.shareFiles(fileNames, day)
                                shareDialog.visible = false
                                mainColumn.enabled = true
                                shareDialog.z -= 1
                            }
                        }
                        anchors.right: parent.right
                        anchors.rightMargin: 30
                    }
                }

            }
        }
    }

}
