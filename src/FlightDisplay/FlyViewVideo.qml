import QtQuick
import QGroundControl
import QGroundControl.Controls
import QGroundControl.Controllers
import QGroundControl.ScreenTools

Item {
    id: _root

    property real zoomLevel: 1.0
    property real panX: 0
    property real panY: 0
    property point lastMousePos: Qt.point(0, 0)

    // Limitli pan değerleri
    function limitPan() {
        if (zoomLevel <= 1.0) {
            panX = 0
            panY = 0
            return
        }

        const halfWidth = videoWrapper.width * (zoomLevel - 1) / 2
        const halfHeight = videoWrapper.height * (zoomLevel - 1) / 2

        panX = Math.max(-halfWidth, Math.min(panX, halfWidth))
        panY = Math.max(-halfHeight, Math.min(panY, halfHeight))
    }

    property Item pipView
    property Item pipState: videoPipState

    PipState {
        id: videoPipState
        pipView: _root.pipView
        isDark: true

        onWindowAboutToOpen: {
            QGroundControl.videoManager.stopVideo()
            videoStartDelay.start()
        }

        onWindowAboutToClose: {
            QGroundControl.videoManager.stopVideo()
            videoStartDelay.start()
        }

        onStateChanged: {
            if (pipState.state !== pipState.fullState)
                QGroundControl.videoManager.fullScreen = false
        }
    }

    Timer {
        id: videoStartDelay
        interval: 2000
        running: false
        repeat: false
        onTriggered: QGroundControl.videoManager.startVideo()
    }

    // Zoom/pan uygulanacak video alanı
    Item {
        id: videoWrapper
        width: parent.width
        height: parent.height
        scale: zoomLevel
        x: panX
        y: panY
        transformOrigin: Item.Center

        FlightDisplayViewVideo {
            id: videoStreaming
            anchors.fill: parent
            useSmallFont: _root.pipState.state !== _root.pipState.fullState
            visible: QGroundControl.videoManager.isStreamSource
        }
    }

    // Scroll Zoom ve Pan kontrolü
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onWheel: (wheel) => {
            if (wheel.angleDelta.y > 0)
                zoomLevel = Math.min(zoomLevel + 0.1, 4.0)
            else
                zoomLevel = Math.max(zoomLevel - 0.1, 1.0)
            limitPan()
        }

        onPressed: (mouse) => {
            lastMousePos = Qt.point(mouse.x, mouse.y)
        }

        onPositionChanged: (mouse) => {
            if (mouse.buttons === Qt.LeftButton && zoomLevel > 1.0) {
                panX += mouse.x - lastMousePos.x
                panY += mouse.y - lastMousePos.y
                lastMousePos = Qt.point(mouse.x, mouse.y)
                limitPan()
            }
        }

        onDoubleClicked: {
            QGroundControl.videoManager.fullScreen = !QGroundControl.videoManager.fullScreen
        }
    }

    // Dokunmatik pinch zoom
    PinchArea {
        anchors.fill: parent
        pinch.target: videoWrapper
        pinch.minimumScale: 1.0
        pinch.maximumScale: 4.0

        onPinchUpdated: {
            zoomLevel = Math.max(1.0, Math.min(zoomLevel * pinch.scale, 4.0))
            limitPan()
        }

        onPinchFinished: {
            limitPan()
        }
    }

    // Görsel zoom göstergesi (isteğe bağlı)
    QGCLabel {
        text: "Zoom: " + zoomLevel.toFixed(1) + "x"
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        color: "white"
        visible: zoomLevel !== 1.0
        font.pointSize: ScreenTools.smallFontPointSize
    }
}
