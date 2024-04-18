import QtQuick
import QtQuick.Controls

Rectangle {
    id:login

    width: 400
    height: 300
    property bool loginFlag: false
    border.color: "lightblue"
    Component.onCompleted:{
        //client.checkLogin.connect(function(flag){
        myNetworkManager.checkLogin.connect(function(flag){
            if(!flag){
                var dialog = Qt.createQmlObject('import QtQuick.Controls 2.15; Dialog { title: "登录错误"; width: 200; height: 150; standardButtons: Dialog.Ok | Dialog.Cancel; Label { text: "请输入正确的账号和密码!"; anchors.verticalCenter: parent.verticalCenter; anchors.horizontalCenter:parent.horizontalCenter;} }', userInfoList);
                dialog.open();
            }else{
                loginFlag = true
                login.visible = false
                client.saveUsernameAndPassword(rememberCheckbox.checked)
                setMainPase()
                }

        })
        client.credentialsLoaded.connect(function(username, password){
            rememberCheckbox.checked = true
            userName.text = username
            userPass.text = password
        })

    }

    signal changePageToReg()
    Rectangle{
        id:signLogo
        width: parent.width
        height: 100
        anchors.topMargin: 10
        Label{
            id:hello1
            text: "Hello!"
            font{
                family: "Microsoft YaHei"
                pixelSize: 30
            }
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            anchors.horizontalCenter:parent.horizontalCenter
        }
        Label{
            id:hello2
            text: "Sign in  to your account"
            font{
                family: "Microsoft YaHei"
                pixelSize: 14
            }
            anchors.top: hello1.bottom
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            anchors.horizontalCenter:parent.horizontalCenter
        }
    }
    Grid{
        id: userInfoList
        //anchors.top: signLogo.bottom
        columns: 2
        rows : 3
        spacing: 10
        anchors.horizontalCenter:login.horizontalCenter
        anchors.top: signLogo.bottom
        topPadding: 0
        Text{
            text: "用户名"
        }
        TextField {
            id:userName
            width: 200
            maximumLength: 20
        }
        Text{
            text: "密   码"
        }
        TextField {
            id:userPass
            width: 200
            echoMode: TextField.Password
            maximumLength: 20

            onAccepted: {
                if(userName.text === "" || userPass.text === ""){
                    var dialog = Qt.createQmlObject('import QtQuick.Controls 2.15; Dialog { title: "登录错误"; width: 200; height: 150; standardButtons: Dialog.Ok | Dialog.Cancel; Label { text: "请输入正确的账号和密码!"; anchors.verticalCenter: parent.verticalCenter; anchors.horizontalCenter:parent.horizontalCenter;} }', parent);
                    dialog.open();


                }else{
                    client.login(userName.text, userPass.text)
                }
            }

        }

    }
    CheckBox {
        id: rememberCheckbox
        anchors.top: userInfoList.bottom
        text: "Remember Password"
        checked: false // 初始状态未选中
        anchors.horizontalCenter: parent.horizontalCenter
    }
    Rectangle{
        id:signin
        width: parent.width

        height: 30
        anchors.top: rememberCheckbox.bottom
        anchors.topMargin: 5
        color: "#8B864E"

        Label{
            text: "Sign in"
            anchors.fill: parent
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            font{
                family: "Microsoft YaHei"
                pixelSize: 20
            }
        }
        TapHandler{
            id:loginTap
            onTapped: {
                if(userName.text === "" || userPass.text === ""){
                    var dialog = Qt.createQmlObject('import QtQuick.Controls 2.15; Dialog { title: "登录错误"; width: 200; height: 150; standardButtons: Dialog.Ok | Dialog.Cancel; Label { text: "请输入正确的账号和密码!"; anchors.verticalCenter: parent.verticalCenter; anchors.horizontalCenter:parent.horizontalCenter;} }', parent);
                    dialog.open();


                }else{
                    client.login(userName.text, userPass.text)
                }
            }

        }
    }
    Rectangle{
        id:signUp
        width: parent.width - 30
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: signin.bottom
        anchors.topMargin: 30
        height: 30

        Label{
            text: "没有账号？那注册一个吧！"
            anchors.fill: parent
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            font{
                family: "Microsoft YaHei"
                pixelSize: 14
            }
        }
        TapHandler{
            onTapped: {changePageToReg()}
        }
    }

}
