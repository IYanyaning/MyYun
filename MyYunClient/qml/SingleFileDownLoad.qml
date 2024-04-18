import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    width: 400
    height: 100
    color: "lightgray"

    property string fileName: ""
    property int fileSize: 0
    property int downloadedSize: 0
    property bool downloadSuccess: false
    Component.onCompleted:{
        progress()
    }

    function progress(){
        while(1){
            progressBar.value += 1
            sleep(1)
        }
    }
    Text {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        text: "File Name: " + fileName
    }

    Text {
        anchors.top: fileNameLabel.bottom
        text: "File Size: " + fileSize + " KB"
    }

    ProgressBar {
        id:progressBar
        anchors.top: fileSizeLabel.bottom
        width: parent.width - 20
        value: 0
        from: 0
        to:100
        indeterminate: false
    }

    Text {
        anchors.top: progressBar.bottom
        text: downloadSuccess ? "Downloaded successfully" : "Download failed"
        color: downloadSuccess ? "green" : "red"
    }
}
