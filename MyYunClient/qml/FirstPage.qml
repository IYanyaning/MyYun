import QtQuick
import QtQuick.Controls

Rectangle {
    id:firstPage
    width: parent.width
    height: parent.height

    FileStyle{
        id:fileStyleRec
        width: 170
        height: parent.height
    }

    MainContent{
        id:mainContentRec
        width: parent.width  - fileStyleRec.width
        height: parent.height
        anchors.left: fileStyleRec.right
    }
}
