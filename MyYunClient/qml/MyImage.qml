import QtQuick
import QtQuick.Controls

Rectangle {
    id:myImage
    //width: parent.width
    //height: parent.height
    visible: false
    property var myType: ""
    property var imagePreviewStatus: imagePreview.status
    function setImageSourceAndFileName(url,fileName){
        imagePreview.source = url
        imageLoaded()
        filename.text = fileName
    }
    signal imageLoaded()
    Component.onCompleted:{
        if(imagePreview.status === Image.Ready) { imageLoaded()}
    }


    Rectangle{
        id:head
        width: parent.width
        height: 60
        color: "lightgray"
        Text{
            id: filename
            text: qsTr("")
            font.pixelSize: 20
            anchors.verticalCenter: head.verticalCenter
            anchors.left: head.left
            anchors.leftMargin: 30
        }
        Rectangle{
            id:exit
            width: 80
            height: 30
            anchors.right: head.right
            anchors.rightMargin: 30
            radius: 45
            color: "#87cefa"
            anchors.verticalCenter: parent.verticalCenter
            Text{
                text: "退出"
                font{
                    family: "Microsoft YaHei"
                    pixelSize: 20
                }
                anchors.centerIn: parent
            }
            TapHandler{
                onTapped: {
                    myImage.visible = false
                    if(myType === "first"){
                        mainColumn.opacity = 1
                        mainColumn.enabled = true
                    }else if(myType === "second"){
                        secondPage.controlMainColume()
                    }else if(myType === "third"){
                        thirdPage.controlMainColume()
                    }
                }
            }
        }

    }
    Rectangle{
        id:imageRec
        width: myImage.width
        height: myImage.height - head.height
        anchors.top: head.bottom
        Image{
            id:imagePreview
            source: ""
            anchors.fill: parent
            fillMode: Image.PreserveAspectFit
        }

        TapHandler{
            onTapped: {
                myImage.visible = false
                imagePreview.source = ""
                if(myType === "first"){
                    mainColumn.opacity = 1
                    mainColumn.enabled = true

                }else if(myType === "second"){
                    secondPage.controlMainColume()
                } else if(myType === "third"){
                    thirdPage.controlMainColume()
                }
            }
        }
    }

}
