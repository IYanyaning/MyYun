import QtQuick
import QtQuick.Controls
import QtMultimedia
Rectangle {
    id:myVideo
    property var videoStatus: videoPlayer.mediaStatus
    signal videoLoaded()
    function setVideoPlayerSource(data, name){
        //videoPlayer.pause()
        videoPlayer.setSource(data)
        console.log((data))
        videoPlayer.play()
    }
    MediaPlayer{
        id:videoPlayer
        audioOutput: AudioOutput{}
        videoOutput: videoOutPut
    }

    VideoOutput{
        id:videoOutPut
        width: parent.width
        height: parent.height
        MouseArea{
            anchors.fill: parent
            onClicked: {mainContentRec.controlMainColume();myVideo.visible = false}
        }
    }
}
