import QtQuick
import QtQuick.Controls
import QtMultimedia
Rectangle {
    id:myMusic
    width: parent.width //700
    height: parent.height //60
    color:"#E1E4EA"
    property var reccolor: "transparent"
    property real volumeValue: 0.5 // 初始音量值
    property var musicStatus: musicPlayer.mediaStatus
    signal findPreviousMusic()
    signal findNextMusic()
    function formatTime(time) {
           return (time < 10 ? "0" : "") + time.toString();
       }
    function setMusicPlayerSource(data, name){
        //console.log(data)
        //musicPlayer.pause()
        musicPlayer.setSource(data)
        //musicPlayer.source = data
        musicNameLab.text = name
        flickable.contentX = 0 // 回到起始位置
        musicPlayer.play()
    }
    MediaPlayer{
        id:musicPlayer
        audioOutput: AudioOutput {
            volume: volumeValue
        }
    }
    Row{
        spacing: 20
        Rectangle{
            id:musicNameRec
            width: 140
            height: myMusic.height
            color:"#E1E4EA"
            //ScrollView {
                //id: scrollView
               // anchors.fill: parent
            Rectangle{
                width: parent.width
                height: musicNameLab.height
                anchors.centerIn: parent
                color:"#E1E4EA"
                Flickable {
                    id: flickable
                    contentWidth: musicNameLab.width
                    contentHeight: musicNameLab.height
                    flickableDirection: Flickable.HorizontalFlick

                    Label{
                        id:musicNameLab
                        text: ""
                        //width: 140
                        font: {
                        }
                        leftPadding: 20
                    }

                    Timer {
                        id: scrollTimer
                        interval: 500 // 滚动间隔，单位毫秒
                        repeat: true
                        running: true
                        onTriggered: {
                            if(flickable.contentWidth >= musicNameRec.width){
                                flickable.contentX += 3 // 每次滚动的距离
                                if (flickable.contentX >= (musicNameLab.width - flickable.width)) {
                                    flickable.contentX = 0 // 回到起始位置
                                }
                            }
                        }
                    }
                }
            }


            //}

            //border.color: reccolor
        }

        Rectangle{
            id:musicControl
            width: 130
            height: myMusic.height
            color:"#E1E4EA"
            Row{
                spacing: 10
                Rectangle{
                    width: 30
                    height:  musicControl.height
                    color:"#E1E4EA"
                    Rectangle{
                        id:previousMusic
                        width: 30
                        height:  30

                        Text {

                            text: qsTr("上")
                            anchors.centerIn: parent
                        }
                        radius: 45
                        anchors.verticalCenter: parent.verticalCenter

                        TapHandler{
                            onTapped: {
                                console.log("priviousone")
                                findPreviousMusic()
                            }
                        }
                        border.color: reccolor
                    }
                }
                Rectangle{
                    width: 35
                    height:  musicControl.height
                    color:"#E1E4EA"
                    RoundButton{
                        id: playButton
                        radius: 50.0
                        text: musicPlayer.playbackState === MediaPlayer.PlayingState ? "\u2016" : "\u25B6"
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: {
                            musicPlayer.playbackState === MediaPlayer.PlayingState ? musicPlayer.pause() : musicPlayer.play()
                        }
                    }
                }
                Rectangle{
                    width: 30
                    height:  musicControl.height
                    color:"#E1E4EA"
                    Rectangle{
                        id:nextMusic
                        width: 30
                        height:  30

                        Text {

                            text: qsTr("下")
                            anchors.centerIn: parent
                        }
                        radius: 45
                        anchors.verticalCenter: parent.verticalCenter

                        TapHandler{
                            onTapped: {
                                console.log("nextsone")
                                findNextMusic()
                            }
                        }
                        border.color: reccolor
                    }
                }

            }
        }
        Rectangle{
            id:progressBar
            width: 330
            height: myMusic.height
            color:"#E1E4EA"
            Row{
                spacing: 20
                Rectangle{
                    width: 20
                    height: progressBar.height
                    color:"#E1E4EA"
                    Label{
                        id:startTime
                        text: "00:00"//00:00 /
                        width: 20
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Timer {
                         interval: 1000 // 每秒更新一次
                         //running: mediaPlayer.playbackState === MediaPlayer.Playing // 只有在音乐播放时才更新时间
                         running: musicPlayer.playbackRate === MediaPlayer.PlayingState
                         repeat: true
                         function formatTime(time) {
                             return (time < 10 ? "0" : "") + time.toString();
                         }
                         onTriggered: {
                             var totalTime = musicPlayer.duration;
                             var currentTime = musicPlayer.position;
                             var totalMinutes = Math.floor(totalTime / 60000);
                             var totalSeconds = Math.floor((totalTime % 60000) / 1000);
                             var currentMinutes = Math.floor(currentTime / 60000);
                             var currentSeconds = Math.floor((currentTime % 60000) / 1000);
                             startTime.text =  formatTime(currentMinutes) + ":" + formatTime(currentSeconds) + " / " +formatTime(totalMinutes) + ":" + formatTime(totalSeconds);
                             //formatTime(currentMinutes) + ":" + formatTime(currentSeconds) + " / " +
                         }
                     }
                }
                Rectangle{
                    width: 220
                    height: progressBar.height
                    color:"#E1E4EA"
                    Slider{
                        id:progressSlider
                        width: 220
                        from:0
                        to:musicPlayer.duration
                        value: musicPlayer.position
//                        onValueChanged: {
//                            musicPlayer.position = value
//                        }
                        anchors.verticalCenter: parent.verticalCenter
                        background: Rectangle{
                            x: progressSlider.leftPadding
                            y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2

                            implicitWidth: 200
                                     implicitHeight: 4
                                     //width: control.availableWidth
                                     height: implicitHeight
                                     width: parent.width
                                     //height: parent.height
                                     radius: 2
                            color: "lightgray"
                            Rectangle{
                                width: progressSlider.visualPosition * parent.width
                                             height: parent.height
                                             color: "red"
                                             radius: 2
                            }

                        }

                        handle: Rectangle{
                            x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth - width)
                                     y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                                     implicitWidth: 26 / 2
                                     implicitHeight: 26 / 2
                                     radius: 13
                                     color: progressSlider.pressed ? "#f0f0f0" : "#f6f6f6"
                                     border.color: "#bdbebf"

                        }
                    }
                }
                Rectangle{
                    width: 20
                    height: progressBar.height
                    color:"#E1E4EA"
                    Label{
                        id:endTime
                        text:musicPlayer.playbackRate === MediaPlayer.PlayingState ? setEndTime() : "00:00"
                        width: 20
                        anchors.verticalCenter: parent.verticalCenter
                        function formatTime(time) {
                            return (time < 10 ? "0" : "") + time.toString();
                        }
                        function setEndTime(){
                            var totalTime = musicPlayer.duration;
                            var totalMinutes = Math.floor(totalTime / 60000);
                            var totalSeconds = Math.floor((totalTime % 60000) / 1000);
                            return formatTime(totalMinutes) + ":" + formatTime(totalSeconds);
                        }
                    }
                }




            }
            border.color: reccolor
        }
        Rectangle{
            id:musicSetRec
            width: 100
            height: musicControl.height
            color:"#E1E4EA"
            Slider {
                id: volumeSlider
                anchors.centerIn: parent
                width: parent.width * 0.8
                height: parent.height

                //minimumValue: 0 // 最小值
                //maximumValue: 1 // 最大值
                stepSize: 0.01 // 步长
                value: volumeValue // 绑定音量值

                onValueChanged: {
                    volumeValue = value; // 更新音量值
                    // 在这里可以添加设置音量的逻辑，例如：audioPlayer.volume = volumeValue;
                }
                background: Rectangle{
                    x: volumeSlider.leftPadding
                    y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2

                    implicitWidth: 200
                             implicitHeight: 4
                             //width: control.availableWidth
                             height: implicitHeight
                             width: parent.width
                             //height: parent.height
                             radius: 2
                    color: "lightgray"
                    Rectangle{
                        width: volumeSlider.visualPosition * parent.width
                                     height: parent.height
                                     color: "red"
                                     radius: 2
                    }

                }
                handle: Rectangle{
                    x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                             y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                             implicitWidth: 26 / 2
                             implicitHeight: 26 / 2
                             radius: 13
                             color: progressSlider.pressed ? "#f0f0f0" : "#f6f6f6"
                             border.color: "#bdbebf"

                }
            }
        }
    }
}
