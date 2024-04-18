import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
Rectangle{
    id:thirdPage
    width: parent.width
    height: parent.height
    property var  clientname: ""
    property var viewFileName: ""
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
        uuidRec.opacity =  uuidRec.opacity === 0 ? 1 : 0
        uuidRec.enabled = uuidRec.enabled === false ? true : false
        sharedFilesRec.opacity =  sharedFilesRec.opacity === 0 ? 1 : 0
        sharedFilesRec.enabled = sharedFilesRec.enabled === false ? true : false

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
                Qt.openUrlExternally(ipPortAddress + filename)
                break;
            case 'doc':
            case 'docx':
                console.log("这是一个Word文档。");
                break;
            case 'txt':
            case 'md':
                console.log("文本文件")
                client.downLoadSingleFile(filename, username, 3)

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
        onUsernameReceived:{
            clientname = username
        }
        onDownloadForQmlOther3:{
            if(!myTextArea.visible){
                myTextArea.setTextAndFileName(data, viewFileName)
                myTextArea.visible = true
                controlMainColume()
            }
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
    MyTextArea{
        id:myTextArea
        width: parent.width
        height: parent.height
        Component.onCompleted:{
            myTextArea.myType = "third"
        }
        visible: false
    }
    MyImage{
        id:myImage

        width: parent.width
        height: parent.height
        Component.onCompleted:{
            myImage.myType = "third"
        }
        visible: false
    }
    UuidList{
        id:uuidRec
        height: parent.height
        width: 450
    }
    SharedFiles{
        id:sharedFilesRec
        width: parent.width - uuidRec.width
        height: parent.height
        anchors.left: uuidRec.right
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
        property var filename: ""
        onAccepted: {

            var selectedFolder = convertToNativePath(currentFolder.toString())
            console.log("Selected folder: " + selectedFolder);
            downPath = selectedFolder
            if(downloadDialog.filename != ""){
                client.downLoadFiles(downloadDialog.filename, selectedFolder)
                downloadDialog.filename = ""
            }else{
                client.downLoadFiles(fileNames, selectedFolder)
            }

        }
        onRejected: {
            console.log("Canceled");
            // 用户取消选择时的处理逻辑
        }
    }
    Dialog{
        id:noticeDialog
        anchors.centerIn: parent
        width: noticeText.text
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
