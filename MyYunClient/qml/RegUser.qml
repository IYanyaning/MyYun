import QtQuick
import QtQuick.Controls

Rectangle {
    id: regUserPage
    visible: false
    width: 400
    height: 300
    border.color: "lightblue"
    Connections{
        //target: client
        target: myNetworkManager
        onCheckRegist:{
            if(res){
                signupMessageBoxText.text = "注册成功"
                signupMessageBox.visible = true
            }else{
                signupMessageBoxText.text = "注册失败"
                signupMessageBox.visible = true
            }
        }
    }
    signal changePageToLogin()
    Popup {
        id: signupMessageBox
        visible: false
        anchors.centerIn: parent
        width: parent.width
        height: 40
        modal: true
        Text{
            id: signupMessageBoxText
            anchors.centerIn: parent
            text:""
            width: parent.width
        }
    }

    Rectangle{
        id:signLogo
        width: parent.width
        height: 100
        Label{
            id:hello1
            text: "Sign up"
            font{
                family: "Microsoft YaHei"
                pixelSize: 30
            }
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            anchors.horizontalCenter:parent.horizontalCenter
        }
    }
    Grid{
        id: regUserInfoList
        anchors.top: signLogo.bottom
        columns: 2
        rows : 4
        spacing: 10
        anchors.horizontalCenter:regUserPage.horizontalCenter
        Text{
            text: "用户名"
        }
        TextField {
            id: userName
            width: 200
            maximumLength: 20
        }
        Text{
            text: "密   码"
        }
        TextField {
            id: userPass
            width: 200
            maximumLength: 20
            echoMode: TextField.Password
            onAccepted: {
                client.regist(userName.text, userPass.text)
            }
        }
    }
    Rectangle{
        id:signup
        width: parent.width
        height: 30
        anchors.top: regUserInfoList.bottom
        anchors.topMargin: 20
        color: "#8B864E"
        Label{
            text: "Sign up"
            anchors.fill: parent
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            font{
                family: "Microsoft YaHei"
                pixelSize: 20
            }
        }
        TapHandler{
            onTapped: {
                client.regist(userName.text, userPass.text)
            }
        }
    }


    Rectangle{
        id:signIn
        width: parent.width - 30
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: signup.bottom
        anchors.topMargin: 30
        height: 30

        Label{
            text: "已经有账号了？那就登陆吧！"
            anchors.fill: parent
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            font{
                family: "Microsoft YaHei"
                pixelSize: 14
            }
        }
        TapHandler{
            onTapped: {changePageToLogin()}
        }
    }
}
