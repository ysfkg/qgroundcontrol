import QtQuick 2.15
import QtQuick.Controls 2.15
import QtMultimedia 6.6

ApplicationWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("RTSP Görüntü")

    MediaPlayer {
        id: mediaPlayer
        source: "rtsp://localhost:8554/mystream"
        videoOutput: videoOutput
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
    }

    Component.onCompleted: {
        mediaPlayer.play()
    }

    Text {
        text: {
            switch (mediaPlayer.mediaStatus) {
                case MediaPlayer.NoMedia: return "⛔ No media";
                case MediaPlayer.LoadingMedia: return "⏳ Loading...";
                case MediaPlayer.LoadedMedia: return "✅ Loaded";
                case MediaPlayer.StalledMedia: return "⚠️ Stalled";
                case MediaPlayer.InvalidMedia: return "❌ Invalid media";
                default: return ""
            }
        }
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        color: "white"
        font.pixelSize: 16
    }
}
