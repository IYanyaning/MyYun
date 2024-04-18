import QtQuick 2.15
import QtQuick.Controls 2.15
import Qt.labs.folderlistmodel 2.15

ApplicationWindow {
    id:app
    visible: true
    width: 1100
    height: 730
    title: "Cloud Drive"
    minimumWidth: 1100 // 设置窗口的最小宽度
    minimumHeight: 730 // 设置窗口的最小高度
    maximumWidth: Screen.width // 设置窗口的最大宽度为屏幕宽度
    maximumHeight: Screen.height // 设置窗口的最大高度为屏幕高度

    // 在窗口大小变化时进行处理，确保不会小于最小尺寸
    onWidthChanged: {
        if (width < minimumWidth) {
            width = minimumWidth;
        }
    }

    onHeightChanged: {
        if (height < minimumHeight) {
            height = minimumHeight;
        }
    }
    header: YunHead{
        id:yunHeadRec
        width: parent.width
        height: 60
        enabled: false
        opacity: 0.5
    }
    function setMainPase(){
        yunHeadRec.opacity = 1
        yunHeadRec.enabled = true

        mainMenuRec.opacity = 1;
        mainMenuRec.enabled = true

        firstPage.opacity = 1;
        firstPage.enabled = true
    }
    MainMenu{
        id:mainMenuRec
        width: 70
        height: parent.height
        enabled: false
        opacity: 0.5
    }
    FirstPage{
        id:firstPage
        width: parent.width - mainMenuRec.width
        height: parent.height
        anchors.left: mainMenuRec.right
        visible: true
        enabled: false
        opacity: 0.5
    }
    SecondPage{
        id:secondPage
        width: parent.width - mainMenuRec.width
        height: parent.height
        anchors.left: mainMenuRec.right
        visible: false
    }
    ThirdPage{
        id:thirdPage
        width: parent.width - mainMenuRec.width
        height: parent.height
        anchors.left: mainMenuRec.right
        visible: false
    }

    Login{
        id:login
        width: 400
        height: 300
        anchors.horizontalCenter: parent.horizontalCenter
        onChangePageToReg:{
            login.visible = false
            regUserPage.visible = true
        }


    }
    RegUser{
        id:regUserPage
        width: 400
        height: 300
        visible: false
        anchors.horizontalCenter: parent.horizontalCenter
        onChangePageToLogin:{
            regUserPage.visible = false
            login.visible = true
        }
    }
    Connections{
        target: mylogin
        onInvalid:{
            invalidDialog.setInvalidText(msg)
            invalidDialog.open()
        }
    }
    Dialog{
        id:invalidDialog
        width: 350
        height: 50
        property var msg: ""
        anchors.centerIn: parent
        function setInvalidText(msg){
            invalidText.text = msg
        }
        Text {
            id:invalidText
            text: qsTr(msg)
            font{
                family: "Microsoft YaHei"
                pixelSize: 15
            }
            anchors.centerIn: parent
        }
    }

}
