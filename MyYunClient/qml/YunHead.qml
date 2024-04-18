import QtQuick
import QtQuick.Controls

Rectangle {
    id:yunHeadRec
    //width: parent.width
    //height: 60
    color: "#E1E4EA"
    property var musicStatus: myMusic.musicStatus
    function setMusicPlayerSource(data, name){
        myMusic.setMusicPlayerSource(data, name)
    }
    signal findNextMusic()
    signal findPreviousMusic()
    Connections{
        target: myMusic
        onFindNextMusic:{
            findNextMusic()
        }
        onFindPreviousMusic:{
            findPreviousMusic()
        }
    }

    Rectangle{
        id:leftHeadRec
        width: parent.width - rightHeadRec.width
        height: parent.height
        color:"#E1E4EA"
        //border.color: "blue"
        MyMusic{
            id:myMusic
            width: parent.width
            height: parent.height
            focus: true
        }
    }


    Rectangle{
        id:rightHeadRec
        width: 330
        height: yunHeadRec.height //60
        anchors.right: yunHeadRec.right
        color: "#E1E4EA"
        Connections{
            //target: client
            target: myNetworkManager
            onUsernameReceived:{
                userNameText.text = username
            }
        }

        Rectangle{
            id:userNameRec
            width: 70
            height: 40 //60
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            color: "#E1E4EA"
            Text {
                id: userNameText
                text: qsTr("")
                anchors.fill: parent
                font{
                    family: "Microsoft YaHei"
                    pixelSize: 20
                }
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter

            }
            TapHandler{
                onTapped: {
                    console.log("click username!")
                }
            }
        }

        Rectangle{
            id:settingRec
            width: 70
            height: 40
            color: "#f8f8ff"
            anchors.verticalCenter: parent.verticalCenter
            radius: 45
            border.color: "transparent"
            anchors.right: parent.right
            anchors.rightMargin: 30
            Text {
                id: settingText
                anchors.fill: parent
                text: qsTr("设  置")
                font{
                    family: "Microsoft YaHei"
                    pixelSize: 20
                }
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }
            TapHandler{
                onTapped: {
                    console.log("设置")
                }
            }

        }






    }
}
