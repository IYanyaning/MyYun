import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
Rectangle {
    id:secondPage
    property var viewFileName: ""
    property var clientname: ""
    property var ipPortAddress: "http://172.16.1.68:10000/onlineviewfile?username=" + clientname +"&filename="
    //property var ipPortAddress: "http://192.168.43.15:10000/onlineviewfile?username=" + clientname + "&filename="
    property var fileNames: [] // 用于存储文件名的数组
    property var username: ""
    function getFileExtension(filename) {
        // 使用正则表达式匹配文件扩展名
        const extensionMatch = filename.match(/\.([0-9a-z]+)(?:[\?#]|$)/i);
        if (extensionMatch) {
            return extensionMatch[1].toLowerCase();
        } else {
            return null; // 没有找到扩展名
        }
    }

    function controlMainColume(){
        fileScrollView.opacity =  fileScrollView.opacity === 0 ? 1 : 0
        fileScrollView.enabled = fileScrollView.enabled === false ? true : false
    }
    function checkFileType(filename) {
        const extension = getFileExtension(filename);
        if (extension === null) {
            console.log("无法确定文件类型，因为文件没有扩展名。");
            return;
        }

        switch(extension) {
            case 'jpg':
            case 'jpeg':
            case 'png':
            case 'gif':
                console.log("这是一个图片文件。");
                myImage.setImageSourceAndFileName(ipPortAddress + filename, filename)
                console.log(ipPortAddress + filename)
//                if(myImage.imagePreviewStatus === Image.Ready){

//                }
                break;
            case 'mp4':
            case 'avi':
            case 'mkv':
                console.log("这是一个视频文件。");
//                myVideo.setVideoPlayerSource(ipPortAddress + filename, filename)
//                //if(myVideo.videoStatus === MediaPlayer.LoadedMedia){
//                    controlMainColume()
//                    myVideo.visible = true
//                //}
                Qt.openUrlExternally(ipPortAddress + filename)
                break;
            case 'mp3':
            case 'wav':
            case 'aac':
                console.log("这是一个音频文件。");
                yunHeadRec.setMusicPlayerSource(ipPortAddress + filename, filename)
                break;
            case 'pdf':
                console.log("这是一个PDF文件。");
                break;
            case 'doc':
            case 'docx':
                console.log("这是一个Word文档。");
                break;
            case 'txt':
            case 'md':
                console.log("文本文件")
                client.downLoadSingleFile(filename, username, 1)

                break;
            case 'html':
                console.log("html文件")
                Qt.openUrlExternally(ipPortAddress + filename)
            // 添加更多的文件类型判断
            default:
                console.log("未知的文件类型。");
                noticeDialog.setNoticeText("暂时无法预览此类型文件")
        }
    }
    Connections{
        //target: client
        target: myNetworkManager
        onDownloadForQmlOther:{
            if(!myTextArea.visible){
                myTextArea.setTextAndFileName(data, viewFileName)
                myTextArea.visible = true
                controlMainColume()
            }
        }
        onUsernameReceived:{
            clientname = username
        }
    }
    Connections{
        target:myImage
        onImageLoaded:{
            console.log("image loaded")
            controlMainColume()
            myImage.visible = true
        }
    }
    MyImage{
        id:myImage
        width: parent.width
        height: parent.height - topRec.height
        anchors.top: topRec.bottom
        Component.onCompleted:{
            myImage.myType = "second"
        }
        visible: false
    }
    MyTextArea{
        id:myTextArea
        width: parent.width
        height: parent.height - topRec.height
        anchors.top: topRec.bottom
        visible: false
        Component.onCompleted:{
            setForReadOnly()
            myTextArea.myType = "second"
        }
    }
    MyVideo{
        id:myVideo
        width: parent.width
        height: parent.height - topRec.height
        anchors.top: topRec.bottom
        visible: false
    }
    Rectangle{
        id:topRec
        width: parent.width
        height: 50
        Rectangle{
            width:80
            height: 30
            anchors.left: parent.left
            radius: 45
            color: "#87cefa"
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 30
            Text {

                text: qsTr("提取码")
                font{
                    family: "Microsoft YaHei"
                    pixelSize: 20
                }
                anchors.centerIn: parent
            }

            TapHandler{
                onTapped: {
                    uuidInputRec.visible = true
                    fileScrollView.enabled = false
                    fileScrollView.opacity = 0.8
                }
            }
        }
        Text {
            id: topTit
            font{
                family: "Microsoft YaHei"
                pixelSize: 20
            }
            anchors.centerIn: parent
        }
    }


    ScrollView{
        id:fileScrollView
        width: parent.width
        height: parent.height - topRec.height
        anchors.top: topRec.bottom
        Rectangle{
            id:bottRec
            width: secondPage.width
            height: secondPage.height - topRec.height
            border.color: "lightgray"
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
                    spacing:5
                    currentIndex: -1
                    delegate: Rectangle{
                        width: showFiles.width
                        height: 50
                        border.color: "lightgray"
                        property bool hovered: false // 添加用于跟踪鼠标进入和退出状态的属性
                        Rectangle{
                            id:singleFile
                            //color: index === showFiles.currentIndex ? "#B6CAD7" : "white"
                            color: (showFiles.currentIndex === index || hovered) ? "lightblue" : "transparent"
                            width: parent.width
                            height: 50
                            border.color: "lightgray"
                            Text {
                                id: singleFileName
                                text:  fileName
                                anchors.left: parent.left
                                font{
                                    family: "Microsoft YaHei"
                                    pixelSize: 20
                                }
                                anchors.centerIn: parent
                            }
                            Menu{
                                id:menu
                                width: 50
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
//                        TapHandler{
//                            onDoubleTapped: {
//                                viewFileName = singleFileName.text
//                                checkFileType(viewFileName)
//                            }

//                        }
                        MouseArea{
                            id:mouseArea
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            hoverEnabled: true
                            onEntered: {
                                hovered = true // 更新 hovered 属性来跟踪鼠标的进入和退出状态。
                            }
                            onExited: {
                               hovered = false // 更新 hovered 属性来跟踪鼠标的进入和退出状态。
                            }
                            onDoubleClicked: {
                                if(mouse.button === Qt.LeftButton){
                                    viewFileName = singleFileName.text
                                    checkFileType(viewFileName)
                                }

                            }
                            onClicked: {
                                if(mouse.button === Qt.RightButton){
                                    menu.x = mouseX
                                    menu.y = mouseY
                                    menu.visible = true
                                }
                                if(mouse.button === Qt.LeftButton){
                                    showFiles.currentIndex = index

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
        onGetUuidFilesListAndUserName:{
            showFilesModel.clear()
            for(var i = 0; i < filesList.length; ++i){
                //console.log(filesList[i])
                showFilesModel.append({fileName:filesList[i]})
            }
            username = userName
            topTit.text = "来自" + userName + "--" + uuidInput.text + "分享的文件"
            uuidInputRec.visible = false
        }
        onInvalidUuid:{
            noticeDialog.setNoticeText("无效提取码")
            noticeDialog.open()
        }
    }
    function convertToNativePath(url) {
        // 移除多余的斜杠并添加文件协议
        var nativePath = url.replace("file:///", "file:///").replace("file://", "")
        return Qt.platform.os === "linux" ? "/" + nativePath : nativePath
    }
    FolderDialog{
        id:downloadDialog
        title: qsTr("选择下载路径")
        property var downPath: ""
        property string filename: ""
        onAccepted: {

            var selectedFolder = convertToNativePath(currentFolder.toString())
            console.log("Selected folder: " + selectedFolder);
            downPath = selectedFolder
            if(downloadDialog.filename != ""){
                client.downLoadFiles(downloadDialog.filename, selectedFolder)
                downloadDialog.filename = ""
            }

        }
        onRejected: {
            console.log("Canceled");
            // 用户取消选择时的处理逻辑
        }
    }

    Rectangle{
        id:uuidInputRec
        width: 600
        height: 300
        border.color: "blue"
        anchors.centerIn: parent

        radius: 5
        Label{
            id:tit
            width: 200
            height:40
            text: "输入提取码(uuid)"
            leftPadding: 180
            topPadding: 30
            font{
                family: "Microsoft YaHei"
                pixelSize: 30
            }
            anchors.topMargin: 10

        }
        Rectangle{
            id:uuidRec
            width: 550
            height: 40
            anchors.top: tit.bottom
            anchors.topMargin: 100
            anchors.horizontalCenter: parent.horizontalCenter
            color: "lightblue"
            radius: 5
            TextInput{
                id:uuidInput
                anchors.centerIn: parent
                anchors.fill: parent
                color: "black"
                leftPadding: 20
                topPadding: 10
                selectByMouse: true
                maximumLength: 36
                wrapMode:Text.WordWrap
                font{
                    family: "Microsoft YaHei"
                    pixelSize: 20
                }
                focus: true

            }
        }
        Rectangle{
            width: 250
            height: 50
            anchors.top: uuidRec.bottom
            anchors.topMargin: 20
            anchors.right: parent.right
            anchors.rightMargin: 10
            Row{
                spacing: 50

                Rectangle{
                    width: 80
                    height: 30
                    radius: 45
                    color: "#87cefa"
                    Text {
                        text: qsTr("取消")
                        font{
                            family: "Microsoft YaHei"
                            pixelSize: 16
                        }
                        anchors.centerIn: parent
                    }
                    TapHandler{
                        onTapped: {
                            uuidInputRec.visible = false
                            fileScrollView.enabled = true
                            fileScrollView.opacity = 1.0
                        }
                    }
                }
                Rectangle{
                    width: 80
                    height: 30
                    radius: 45
                    color: "#87cefa"
                    Text {
                        text: qsTr("查看")
                        font{
                            family: "Microsoft YaHei"
                            pixelSize: 16
                        }
                        anchors.centerIn: parent
                    }
                    TapHandler{
                        onTapped: {
                            client.downLoadFromSharing(uuidInput.text)
                            fileScrollView.enabled = true
                            fileScrollView.opacity = 1.0
                        }
                    }
                }
            }
        }

    }
    Dialog{
        id:noticeDialog
        anchors.centerIn: parent
        width: 200
        height: 100
        function setNoticeText(str){
            noticeText.text =  str
        }
        Text {
            id: noticeText
            text: qsTr("")
            font{
                family: "Microsoft YaHei"
                pixelSize: 20
            }
            anchors.centerIn: parent
        }
    }
}
