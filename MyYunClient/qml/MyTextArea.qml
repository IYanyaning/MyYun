import QtQuick
import QtQuick.Controls
import Qt.labs.platform
Rectangle {
    id:myTextArea
    property var myType: ""
    function setTextAndFileName(data,fileName){
        textArea.text = data
        filename.text = fileName
    }
    function setForReadOnly(){
        changedLabel.visible = false
        changedLabel.enabled = false
        cancel.visible = false
        cancel.enabled = false
        save.visible = false
        save.enabled = false
        textArea.readOnly = true
        textMessageDialog.visible = false

    }
    Rectangle{
        id:head
        width: parent.width
        height: 60
        color: "lightgray"
        Text {
            id: filename
            text: qsTr("")
            font.pixelSize: 20
            anchors.verticalCenter: head.verticalCenter
            anchors.left: head.left
            anchors.leftMargin: 30
        }
        Label{
            id:changedLabel
            width: 50
            text: qsTr("已修改")
            font.pixelSize: 20
            anchors.verticalCenter: head.verticalCenter
            anchors.left: filename.right
            anchors.leftMargin: 10
            visible: false
        }
        Rectangle{
            id:cancel
            width: 80
            height: 30
            anchors.right: save.left
            anchors.rightMargin: 30
            radius: 45
            color: "#87cefa"
            anchors.verticalCenter: parent.verticalCenter
            Text{

                text: "撤销"
                font{
                    family: "Microsoft YaHei"
                    pixelSize: 20
                }
                anchors.centerIn: parent
            }
            TapHandler{
                onTapped: {

                    if(textArea.changedFlag >= 2){
                        textArea.undo()
                        textArea.changedFlag--
                    }else{
                        changedLabel.visible = false
                    }
                }
            }
        }
        Rectangle{
            id:save
            width: 80
            height: 30
            anchors.right: exit.left
            anchors.rightMargin: 30
            radius: 45
            color: "#87cefa"
            anchors.verticalCenter: parent.verticalCenter
            Text{

                text: "保存"
                font{
                    family: "Microsoft YaHei"
                    pixelSize: 20
                }
                anchors.centerIn: parent
            }
            TapHandler{
                onTapped: {
                    if(textArea.changedFlag >= 2){
                        client.saveChangedText(filename.text, textArea.text)
                        textArea.changedFlag = 1
                        changedLabel.visible = false
                    }
                }
            }
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

                    if(myTextArea.myType === "second"){
                        //textArea.clear()
                        myTextArea.visible = false
                        secondPage.controlMainColume()
                    }else{
                        if(textArea.changedFlag >= 2){
                            console.log("open")
                            textMessageDialog.open()
                        }else{
                            myTextArea.visible = false
                            //textArea.clear()
                            textArea.changedFlag = 0
                            if(myType === "first"){
                                mainColumn.opacity = 1
                                mainColumn.enabled = true
                            }else if(myType === "third"){
                                thirdPage.controlMainColume()
                            }
                            changedLabel.visible = false
                        }
                    }
                }
            }
        }


    }
    ScrollView{
        width: parent.width
        height: parent.height - head.height
        anchors.top: head.bottom

        TextArea{
            id:textArea
            width: parent.width
            wrapMode: TextEdit.Wrap
            //textDocument: TextEdit.AutoText
            focus: true
            text: ""
            font.pixelSize: 16
            property var changedFlag: 0

            onTextChanged: {
                if(changedFlag >= 2 && myType != "second"){
                    changedLabel.visible = true
                }
                changedFlag++
            }
        }
    }
    MessageDialog {
        id:textMessageDialog
        text: "The document has been modified."
        informativeText: "Do you want to save your changes?"


        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onAccepted: {
            client.saveChangedText(filename.text, textArea.text)
            myTextArea.visible = false

            textArea.changedFlag = 0
            changedLabel.visible = false
            if(myType === "first"){
                mainColumn.opacity = 1
                mainColumn.enabled = true
            }else if(myType === "third"){
                thirdPage.controlMainColume()
            }

        }
        onCancelClicked: {
            myTextArea.visible = false

            textArea.changedFlag = 0
            changedLabel.visible = false
            if(myType === "first"){
                mainColumn.opacity = 1
                mainColumn.enabled = true
            }else if(myType === "third"){
                thirdPage.controlMainColume()
            }

        }
    }

}
