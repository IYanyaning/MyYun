import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtMultimedia
import QtCore 6.4
import QtQuick.LocalStorage 2.0

Rectangle {
    id:mainContentRec
    border.color: "#D2C3D5"
    //color: "yellow"
    property var fileNames: [] // 用于存储文件名的数组
    property var yes: "√"
    property var no: "✕"
    property var musicIndex: -1
    property var viewFileName: ""
    property var clientname: ""
    property var ipPortAddress: "http://172.16.1.68:10000/onlineviewfile?username=" + clientname +"&filename="
    property var specialIpPortAddress: "https/http://172.16.1.68:10000/onlineviewfile?username=chen&filename="
    //property var ipPortAddress: "http://192.168.43.15:10000/onlineviewfile?username="+clientname + "&filename="
    ShareDialog{
        id:shareDialog
        width: 450
        height: 300
        anchors.centerIn: parent
        color: "#B6CAD7"
        visible: false
    }
    MyTextArea{
        id:myTextArea
        width: parent.width
        height: parent.height
        Component.onCompleted:{
            myTextArea.myType = "first"
        }
        visible: false
    }
    MyImage{
        id:myImage
        width: parent.width
        height: parent.height
        Component.onCompleted:{
            myImage.myType = "first"
        }
        visible: false
    }
    MyVideo{
        id:myVideo
        width: parent.width
        height: parent.height
        visible: false
    }
    Component.onCompleted:{
        //client.renameResult.connect(function(flag){
        myNetworkManager.renameResult.connect(function(flag){
            if(flag){
                noticeDialog.setNoticeText("Rename success!")
            }else{
                noticeDialog.setNoticeText("Rename failed")
            }
            noticeDialog.open()
        })


        //client.sharedFilesListChanged.connect(function(list, uuid){
        myNetworkManager.sharedFilesListChanged.connect(function(list, uuid){
            if(!sharingDialog.visible){
                showSharedFilesModel.clear()
                for(var i = 0; i < list.length; ++i){
                    console.log(list[i])
                    showSharedFilesModel.append({fileName: list[i]})
                }
                showSharedFiles.uuidStr = uuid
                sharingDialog.visible = true
                //sharingDialog.open()
            }

        })

        //client.canntPreview.connect(function(){
        .canntPreview.connect(function(){
            noticeDialog.setNoticeText("该文件暂不支持预览");
            noticeDialog.open()
        });
    }
    Connections{
        target:myImage
        onImageLoaded:{
            console.log("image loaded")
            controlMainColume()
            myImage.visible = true
        }
    }
    Connections{
        target: yunHeadRec
        onFindNextMusic:{
            var fileSource = findNextMusic()
            if(fileSource[1] !== ""){
                yunHeadRec.setMusicPlayerSource(fileSource[0], fileSource[1])
            }else{

            }
        }
        onFindPreviousMusic:{
            var fileSource = findPreviosMusic()
            if(fileSource[1] !== ""){
                yunHeadRec.setMusicPlayerSource(fileSource[0], fileSource[1])
            }else{

            }
        }
    }
    Connections{
        //target: client
        target: myNetworkManager
        onUsernameReceived:{
            clientname = username
        }
        onDownloadForQml:{
            if(!myTextArea.visible){
                myTextArea.setTextAndFileName(data, viewFileName)
                myTextArea.visible = true
                mainColumn.opacity = 0
                mainColumn.enabled = false
            }
        }
        onImageReady:{
//            if(!myImage.visible){
//                myImage.setImageSourceAndFileName("data:image/jpg;base64," + imageData, viewFileName)
//                myImage.visible = true
//                mainColumn.opacity = 0
//                mainColumn.enabled = false
//            }

        }
        onMusicReady:{
//            yunHeadRec.setMusicPlayerSource("data:audio/mpeg;base64," + musicData, viewFileName)
        }
    }
    function controlMainColume(){
        mainColumn.opacity =  mainColumn.opacity === 0 ? 1 : 0
        mainColumn.enabled = mainColumn.enabled === false ? true : false
    }
    function getFileExtension(filename) {
        // 使用正则表达式匹配文件扩展名
        const extensionMatch = filename.match(/\.([0-9a-z]+)(?:[\?#]|$)/i);
        if (extensionMatch) {
            return extensionMatch[1].toLowerCase();
        } else {
            return null; // 没有找到扩展名
        }
    }

    function checkFileType(filename) {
        const extension = getFileExtension(filename);
//        if (extension === null) {
//            console.log("无法确定文件类型，因为文件没有扩展名。");
//            return;
//        }

        switch(extension) {
            case 'jpg':
            case 'jpeg':
            case 'png':
            case 'gif':
                console.log("这是一个图片文件。");
                myImage.setImageSourceAndFileName(ipPortAddress + filename, filename)
//                if(myImage.imagePreviewStatus === Image.Ready){

//                }
                break;
            case 'mp4':
            case 'avi':
            case 'mkv':
                console.log("这是一个视频文件。");
                myVideo.setVideoPlayerSource(ipPortAddress + filename, filename)
                //if(myVideo.videoStatus === MediaPlayer.LoadedMedia){
                    controlMainColume()
                    myVideo.visible = true
                //}
                //Qt.openUrlExternally(ipPortAddress + filename)
                break;
            case 'mp3':
            case 'wav':
            case 'aac':
                console.log("这是一个音频文件。");
                musicIndex = showFiles.currentIndex
                yunHeadRec.setMusicPlayerSource(ipPortAddress + filename, filename)
                break;
            case 'pdf':
                Qt.openUrlExternally(ipPortAddress + filename)
                console.log("这是一个PDF文件。");
                break;
            case 'doc':
            case 'docx':
                console.log("这是一个Word文档。");
                break;
            case 'txt':
            case 'md':
            case 'c':
            case 'cpp':
            case 'h':
            case 'md':
                console.log("文本文件")
                client.downLoadSingleFile(filename)
                controlMainColume()
                break;
            case 'html':
                console.log("html文件")
                Qt.openUrlExternally(ipPortAddress + filename)
                break;
            // 添加更多的文件类型判断
            default:
                console.log("未知的文件类型。");
                noticeDialog.setNoticeText("暂时无法预览此类型文件")
                noticeDialog.open()
        }
    }

    function findPreviosMusic(){
        var index = musicIndex - 1;
        var length = showFiles.contentItem.children.length;
        for(var i = index; i >= 0; --i){
            var item = showFiles.contentItem.children[i]
            var extension = item.getExtension()
            if(extension === "mp3" || extension === "mpeg" || extension === "mpga"){
                musicIndex = i
                var arr = [2]
                arr[0] = ipPortAddress + item.returnSingleFileName()
                arr[1] = item.returnSingleFileName()
                return arr
            }
        }
        for(var j = length - 2; j >= 0; --j){
            var newitem = showFiles.contentItem.children[j]
            var newextension = newitem.getExtension()
            if(newextension === "mp3" || newextension === "mpeg" || newextension === "mpga"){
                musicIndex = i
                var newarr = [2]
                newarr[0] = ipPortAddress + newitem.returnSingleFileName()
                newarr[1] = newitem.returnSingleFileName()
                return newarr
            }
        }

        return ""
    }

    function findNextMusic(){
        var index = musicIndex + 1;
        var length = showFiles.contentItem.children.length;
        for(var i = index; i < length - 1; ++i){
            var item = showFiles.contentItem.children[i]
            var extension = item.getExtension()
            if(extension === "mp3" || extension === "mpeg" || extension === "mpga"){
                musicIndex = i
                var arr = [2]
                arr[0] = ipPortAddress + item.returnSingleFileName()
                arr[1] = item.returnSingleFileName()
                return arr
            }
        }
        for(var j = 0; j < length - 1; ++j){
            var newitem = showFiles.contentItem.children[j]
            var newextension = newitem.getExtension()
            if(newextension === "mp3" || newextension === "mpeg" || newextension === "mpga"){
                musicIndex = i
                var newarr = [2]
                newarr[0] = ipPortAddress + newitem.returnSingleFileName()
                newarr[1] = newitem.returnSingleFileName()
                return newarr
            }
        }

        return ""
    }

    Column{
        id:mainColumn

        Rectangle{
            id:topRec
            width: mainContentRec.width
            height: 60

            Rectangle{
                id:uploadFileRec
                width: 500
                height: 60
                border.color: "transparent"
                anchors.top: parent.top
                anchors.left: parent.left
                Rectangle{
                    id:uploadButtonRec
                    width: 80
                    height: 30
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    color: "#87cefa"
                    radius: 45
                    Text{
                        id:upload
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("上  传")
                        font{
                            family: "Microsoft YaHei"
                            pixelSize: 20
                        }
                    }
                    TapHandler{
                        onTapped: {
                            fileDialog.open()
                        }
                    }
                }

                Rectangle{
                    id:mkdirRec
                    width: 80
                    height: 30
                    anchors.left: uploadButtonRec.right
                    anchors.leftMargin: 30
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 45
                    color: "#87cefa"

                    Text{
                        id:mkdir
                        text: qsTr("下  载")
                        font{
                            family: "Microsoft YaHei"
                            pixelSize: 20
                        }
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    TapHandler{
                        onTapped: {
                            console.log("下载")
                            if(fileNames.length <= 0){
                                noticeDialog.setNoticeText("请选择要下载的文件")
                                noticeDialog.open()
                            }else{
                                downloadDialog.open()
                            }
                        }
                    }
                }

                Rectangle{
                    id:deleteRec
                    width: 80
                    height: 30
                    anchors.left: mkdirRec.right
                    anchors.leftMargin: 30
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 45
                    color: "#87cefa"

                    Text{
                        id:deleteText
                        text: qsTr("删  除")
                        font{
                            family: "Microsoft YaHei"
                            pixelSize: 20
                        }
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    TapHandler{
                        onTapped: {
                            console.log("删除")
                            client.deleteFiles(fileNames)
                        }
                    }
                }

                Rectangle{
                    id:reNameRec
                    width: 80
                    height: 30
                    anchors.left: deleteRec.right
                    anchors.leftMargin: 30
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 45
                    color: "#87cefa"
                    visible: false

                    Text{
                        id:reName
                        text: qsTr("重命名")
                        font{
                            family: "Microsoft YaHei"
                            pixelSize: 20
                        }
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    TapHandler{
                        onTapped: {
                            renameFileDialog.open()
                        }
                    }
                }

                Rectangle{
                    id:shareRec
                    width: 80
                    height: 30
                    anchors.left: reNameRec.right
                    anchors.leftMargin: 30
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 45
                    color: "#87cefa"

                    Text{
                        id:share
                        text: qsTr("分  享")
                        font{
                            family: "Microsoft YaHei"
                            pixelSize: 20
                        }
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    TapHandler{
                        onTapped: {
                            if(fileNames.length <= 0){
                                noticeDialog.setNoticeText("请先选择文件再分享")
                                noticeDialog.open()
                            }else{
                                shareDialog.z = mainColumn.z + 1
                                shareDialog.setSharedFileName()
                                shareDialog.visible = true
                                mainColumn.enabled = false
                            }

                        }
                    }
                }

            }

            Rectangle{
                id:searcherRec
                width: 300
                height: 40
                radius: 45
                anchors.right: parent.right
                anchors.rightMargin: 30
                color: "#f8f8ff"
                //border.color: "yellow"
                anchors.verticalCenter: parent.verticalCenter
                TextInput{
                    id: textInput
                    anchors.fill: parent
                    verticalAlignment: TextInput.AlignVCenter
                    anchors.leftMargin: 10
                    focus: true
                    font{
                        family: "Microsoft YaHei"
                        pixelSize: 20
                    }
                    onAccepted: {
                        client.keyWordsFilterFiles(textInput.text)
                    }

                }
                Rectangle{
                    id:clearRec
                    width: 20
                    height: 20
                    anchors.right: searchBut.left
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter

                    Text{
                        anchors.fill: parent
                        text: no
                        font{
                            family: "Microsoft YaHei"
                            pixelSize: 20
                        }
                        verticalAlignment: Text.AlignVCenter
                    }

                    color: "transparent"
                    TapHandler{
                        onTapped: {
                            textInput.text = ""
                        }
                    }
                }
                Rectangle{
                    id:searchBut
                    width: 45
                    height: 30
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter

                    Text{
                        anchors.fill: parent
                        text: qsTr("搜索")
                        font{
                            family: "Microsoft YaHei"
                            pixelSize: 20
                        }
                        verticalAlignment: Text.AlignVCenter
                    }

                    color: "transparent"
                    TapHandler{
                        onTapped: {
                            client.keyWordsFilterFiles(textInput.text)
                        }
                    }
                }

            }

        }

        Rectangle{
            id:fileBar
            width: mainContentRec.width
            height: 50
            color: "pink"


            Button{
                id:selectAll
                width: 20
                height: 20
                anchors.left: fileBar.left
                anchors.leftMargin: 20
                anchors.verticalCenter: fileBar.verticalCenter
                onClicked: {
                    console.log(fileNames.length)
                    selectAll.text === "" ? selectAll.text = yes : selectAll.text = ""
                    for( var i = 0 ; i < showFiles.contentItem.children.length; ++i){
                        var item = showFiles.contentItem.children[i]
                        if(selectAll.text === yes){
                            if(item.checkSelectSingleButton() !== 0){
                                item.doSelectSingleFile()
                            }
                        }else{
                            if(item.checkSelectSingleButton() === 0){
                                item.doSelectSingleFile()
                            }
                        }

                    }
                }
                Component.onCompleted:{
                    if( fileNames.length !== showFiles.count){
                        selectAll.text = ""
                    }

                }
            }
            Text{
                id:fileName
                text:qsTr("文件名")
                font{
                    family: "Microsoft YaHei"
                    pixelSize: 20
                }
                anchors.verticalCenter: fileBar.verticalCenter
                anchors.left: selectAll.right
                anchors.leftMargin: 10
            }
            Text{
                id:fileSize
                text:qsTr("大小")
                font{
                    family: "Microsoft YaHei"
                    pixelSize: 20
                }
                //anchors.horizontalCenter: fileBar.horizontalCenter
                anchors.verticalCenter: fileBar.verticalCenter
                anchors.right: fileBar.right
                anchors.rightMargin: 200
            }
            Text{
                id:fileType
                text:qsTr("类型")
                font{
                    family: "Microsoft YaHei"
                    pixelSize: 20
                }
                anchors.verticalCenter: fileBar.verticalCenter
                anchors.right: fileBar.right
                anchors.rightMargin: 80
            }


        }

        ScrollView{
            width: mainContentRec.width
            height: mainContentRec.height - topRec.height - fileBar.height
            DropArea{
                id:dropArea
                anchors.fill: parent
                onDropped: (drop) =>{
                           //console.log("drop hasUrls: ",drop.hasUrls)
                               if(drop.hasUrls){
                                   var filesPath = []
                                   var urlsTemp = drop.urls
                                    for(var i = 0; i < urlsTemp.length; ++i){
                                        filesPath.push("/" + (urlsTemp[i].toString().replace("file:///", "file://").replace("file://", "")))
                                   }
                                   client.uploadFilesList(filesPath)
                               }
                           }
            }
            Rectangle{
                id:bottRec
                width: mainContentRec.width
                height: mainContentRec.height - topRec.height - fileBar.height
                border.color: "transparent"
                Row{
                    spacing: 10
                    ListView {
                        id:showFiles
                        width: bottRec.width - spacing
                        //height: 300
                        height:bottRec.height
                        model: [] // qmlFilesList 是从 C++ 代码传递过来的 QVariantList
                        clip: true
                        focus: true
                        spacing: 5
                        currentIndex: -1
                        property var lastCurrentIndex: -1

                        property int hoveredIndex: -1
//                        highlight: Rectangle {
//                            width: showFiles.width
//                            color: "lightgray"
//                        }
                        delegate: Rectangle {
                            width: bottRec.width
                            height: 50
                            border.color: "lightblue"
                            color: showFiles.currentIndex == index ? "#464879" : "lightgray"
                            function doSelectSingleFile(){
                                selectSingleFile.clicked()
                            }
                            function checkSelectSingleButton(){
                                if(selectSingleFile.text === yes){
                                    return 0;
                                }else{
                                    return -1;
                                }
                            }
                            function returnSingleFileName(){
                                return singleFileName.text
                            }

                            function removeStringFromFileNames( str){
                                var index = fileNames.indexOf(str)
                                if( index !== -1){
                                    fileNames.splice(index, 1)
                                }
                            }
                            function changeColor(){
                                singleFile.color = "blue"
                            }
                            function getFileExtension(){
                                const extensionMatch = modelData.file_name.match(/\.([0-9a-z]+)(?:[\?#]|$)/i);
                                if (extensionMatch) {
                                    return extensionMatch[1].toLowerCase();
                                } else {
                                    return "null"; // 没有找到扩展名
                                }
                            }
                            function getExtension(){
                                return singleFileType.text
                            }
                            Rectangle {
                                id:singleFile
                                //color: index === showFiles.currentIndex ? "lightblue" : (index === showFiles.hoveredIndex ? "lightgray" : "white")
                                color: "white"
                                width: parent.width
                                height: 50
                                border.color: "lightblue"
                                Component.onCompleted: {
                                    singleFileType.text = getFileExtension()
                                }
                                Button{
                                    id:selectSingleFile
                                    width: 20
                                    height: 20
                                    anchors.left: singleFile.left
                                    anchors.leftMargin: 20
                                    anchors.verticalCenter: singleFile.verticalCenter
                                    visible: true
                                    onClicked: {
                                        selectSingleFile.text === "" ? selectSingleFile.text = "√" : selectSingleFile.text = ""
                                        //singleFile.color === "#ffffff" ? singleFile.color = "red" : singleFile.color = "#ffffff"
//                                        if (parent.color === "white") {
//                                            parent.color = "lightblue";
//                                        } else {
//                                            parent.color = "white";
//                                        }

                                        if(selectSingleFile.text === yes){
                                            fileNames.push(singleFileName.text)
                                        }else{
                                            removeStringFromFileNames(singleFileName.text)
                                            selectAll.text = ""
                                        }
                                        if(fileNames.length === showFiles.count){
                                            selectAll.text = yes
                                        }
                                    }
                                }
                                Text {
                                    id:singleFileName
                                    anchors.left: selectSingleFile.right
                                    anchors.leftMargin: 20
                                    anchors.verticalCenter: singleFile.verticalCenter
                                    text:  modelData.file_name
                                    font{
                                        family: "Microsoft YaHei"
                                        pixelSize: 20
                                    }
                                }
                                Text {
                                    anchors.right: singleFile.right
                                    anchors.rightMargin: 190
                                    width: 50
                                    anchors.verticalCenter: singleFile.verticalCenter
                                    //text: (modelData.size / 1024).toFixed(1) + qsTr("Kib")
                                    text: modelData.size > 1024 ? (modelData.size / 1024).toFixed(2) > 1024 ? ((modelData.size / 1024 / 1024).toFixed(2) + qsTr("M")) : ((modelData.size / 1024).toFixed(2) + qsTr("Kib")): (modelData.size + qsTr("B"))
                                    font{
                                        family: "Microsoft YaHei"
                                        pixelSize: 20
                                    }
                                }
                                Text {
                                    id:singleFileType
                                    anchors.right: singleFile.right
                                    anchors.rightMargin: 80
                                    width: 50
                                    anchors.verticalCenter: singleFile.verticalCenter
                                    font{
                                        family: "Microsoft YaHei"
                                        pixelSize: 20
                                    }
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


//                                TapHandler{
//                                    onDoubleTapped: {
//                                        console.log("download")
//                                        viewFileName = singleFileName.text
//                                        checkFileType(viewFileName)

//                                    }
//                                    onTapped: {
//                                        selectSingleFile.clicked()
//                                        fileNames.length === 1 ? reNameRec.visible = true : reNameRec.visible  = false

//                                        if(showFiles.lastCurrentIndex != -1){
//                                            if(showFiles.itemAtIndex(showFiles.lastCurrentIndex)){
//                                                showFiles.itemAtIndex(showFiles.lastCurrentIndex).color = "white"
//                                            }
//                                        }
//                                        showFiles.currentIndex = index
//                                        singleFile.color = "#464879"
//                                        showFiles.lastCurrentIndex = index

//                                    }

//                                }

                            }

                            MouseArea{
                                anchors.fill: parent
                                hoverEnabled: true
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
//                                onEntered: {
//                                    console.log("enter"+index)
//                                                console.log(showFiles.contentItem.children[index].returnSingleFileName())

//                                    if(showFiles.contentItem.children[index].checkSelectSingleButton() === 0){
//                                        return
//                                    }
//                                    singleFile.color = "lightgray"
//                                }
//                                onExited: {
//                                    console.log("exit"+index)
//                                    console.log(showFiles.contentItem.children[index].returnSingleFileName())

//                                    if(showFiles.contentItem.children[index].checkSelectSingleButton() === 0){
//                                        return
//                                    }
//                                    singleFile.color = "white"
//                                }
                                onClicked: {
                                    if(mouse.button === Qt.LeftButton){
                                        selectSingleFile.clicked()
                                        fileNames.length === 1 ? reNameRec.visible = true : reNameRec.visible  = false

//                                        if(showFiles.lastCurrentIndex != -1 || showFiles.contentItem.children[index].checkSelectSingleButton() != 0){
//                                            if(showFiles.itemAtIndex(showFiles.lastCurrentIndex)){
//                                                showFiles.itemAtIndex(showFiles.lastCurrentIndex).color = "white"
//                                            }
//                                        }
                                        showFiles.currentIndex = index
                                        singleFile.color = "lightblue"
                                        //showFiles.lastCurrentIndex = index
                                    }else if(mouse.button === Qt.RightButton){
                                        console.log("right click")
                                        menu.x = mouseX
                                        menu.y = mouseY
                                        menu.visible = true
                                    }

                                }
                                onDoubleClicked: {
                                    if(mouse.button === Qt.LeftButton){
                                        viewFileName = singleFileName.text
                                        checkFileType(viewFileName)
                                    }
                                }


                            }
                        }
                        Connections{
                            //target:client
                            target: myNetworkManager
                            onFilesListChanged:{
                                showFiles.model = client.getFilesList()
                            }
//                            onFilterFilesListChanged:{
//                                showFiles.model = client.getFilterFilesList()
//                            }

                        }
                        Connections{
                            //target:client
                            target: myFileManager
                            onFilterFilesListChanged:{
                                showFiles.model = client.getFilterFilesList()
                            }

                        }
                    }


                }


            }
        }


    }

    function convertToNativePath(url) {
        // 移除多余的斜杠并添加文件协议
        var nativePath = url.replace("file:///", "file:///").replace("file://", "")
        return Qt.platform.os === "linux" ? "/" + nativePath : nativePath
    }
    FileDialog{
        id:fileDialog
        title: qsTr("选择文件")
        modality: Qt.ApplicationModal
        currentFolder: StandardPaths.standardLocations(StandardPaths.DocumentsLocation)
        fileMode: FileDialog.OpenFiles & FileDialog.MultiFile

        onAccepted: {
            var selectedUrl = convertToNativePath(fileDialog.selectedFile.toString())
            client.uploadFileChunks("/" + (currentFile.toString().replace("file:///", "file://").replace("file://", "")))
        }
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
    Dialog {
        id: renameFileDialog
        title: "Rename File"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: 500
        height: 150
        anchors.centerIn: parent
        Column {
            spacing: 10
            Label {
                text: "Enter new file name:"
                color: "gray"
            }

            Rectangle {
                width: renameFileDialog.width - 10
                height: 40
                color: "lightblue"
                radius: 5

                TextInput {
                    id:newFileName
                    anchors.fill: parent
                    anchors.verticalCenter: singleFile.verticalCenter
                    color: "black"
                    selectByMouse: true
                    maximumLength: 35
                    wrapMode:Text.WordWrap
                    font{
                        family: "Microsoft YaHei"
                        pixelSize: 20
                    }
                }
            }
        }

        onAccepted: {
            client.renameFileName(fileNames[0], newFileName.text)
            newFileName.text = ""
        }
        onClosed: {
            newFileName.text = ""
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

    Dialog{
        id:sharingDialog
        anchors.centerIn: parent
        width: 500
        height: 500
        title: "以下文件分享成功，并生成对应提取码"
        modal: true
        standardButtons: Dialog.Ok
        visible: false
        ListModel{
            id:showSharedFilesModel
        }
        ListView{
            id:showSharedFiles
            anchors.fill: parent
            clip: true
            property var uuidStr: ""
            model:showSharedFilesModel

            footer: Rectangle{
                width: showSharedFiles.width
                height: 60
                color:"white"
                anchors.verticalCenter: showSharedFiles.verticalCenter
                Label{
                    id:uuidLabel
                    text: qsTr("提取码: ")
                    font{
                        family: "Microsoft YaHei"
                        pixelSize: 20
                    }
                }
                TextArea {
                    id: uuidText
                    text: showSharedFiles.uuidStr
                    readOnly: true
                    font{
                        family: "Microsoft YaHei"
                        pixelSize: 20
                    }
                    anchors.left: uuidLabel.right
                    anchors.leftMargin: 10

                }
            }
            delegate: Rectangle{
                width: showSharedFiles.width
                height: 50
                border.color: "transparent"
                color:"white"
                Text {
                    id:singleSharedFileName
                    anchors.verticalCenter: showSharedFiles.verticalCenter
                    text:  fileName
                    font{
                        family: "Microsoft YaHei"
                        pixelSize: 20
                    }
                }
            }
        }
        onAccepted: {
            sharingDialog.close()

        }
    }
}
