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
    property bool isPanning: false
    property Item pipView
    property Item pipState: videoPipState
    property bool isMobile: Qt.platform.os === "android" || Qt.platform.os === "ios"

    function toggleFullScreen() {
        QGroundControl.videoManager.fullScreen1 = !QGroundControl.videoManager.fullScreen1
    }

    function limitPan() {
        if (zoomLevel <= 1.0) {
            panX = 0
            panY = 0
            return
        }

        var viewWidth = videoWrapper.width
        var viewHeight = videoWrapper.height
        var scaledWidth = viewWidth * zoomLevel
        var scaledHeight = viewHeight * zoomLevel
        var maxPanX = (scaledWidth - viewWidth) / 2
        var maxPanY = (scaledHeight - viewHeight) / 2

        panX = Math.max(-maxPanX, Math.min(panX, maxPanX))
        panY = Math.max(-maxPanY, Math.min(panY, maxPanY))
    }

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
                QGroundControl.videoManager.fullScreen1 = false
        }
    }

    Timer {
        id: videoStartDelay
        interval: 2000
        running: false
        repeat: false
        onTriggered: QGroundControl.videoManager.startVideo()
    }

    Item {
        id: videoWrapper
        width: parent.width
        height: parent.height

        transform: [
            Scale {
                origin.x: videoWrapper.width / 2
                origin.y: videoWrapper.height / 2
                xScale: zoomLevel
                yScale: zoomLevel
            },
            Translate {
                x: panX
                y: panY
            }
        ]

        FlightDisplayViewVideo1 {
            id: videoStreaming
            anchors.fill: parent
            useSmallFont: _root.pipState.state !== _root.pipState.fullState
            visible: QGroundControl.videoManager.isStreamSource1
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        visible: !isMobile
        enabled: !isMobile

        property bool hasMovedSignificantly: false
        property point startPos: Qt.point(0, 0)
        property bool waitingForDoubleClick: false
        property int clickDelay: 300

        onPressed: (mouse) => {
            startPos = Qt.point(mouse.x, mouse.y)
            lastMousePos = startPos
            hasMovedSignificantly = false
        }

        onPositionChanged: (mouse) => {
            if (pressed && zoomLevel > 1.0) {
                var dx = mouse.x - lastMousePos.x
                var dy = mouse.y - lastMousePos.y

                if (Math.abs(mouse.x - startPos.x) > 5 || Math.abs(mouse.y - startPos.y) > 5) {
                    hasMovedSignificantly = true
                }

                panX += dx * 1.5
                panY += dy * 1.5

                limitPan()
                lastMousePos = Qt.point(mouse.x, mouse.y)
            }
        }

        onClicked: {
            if (!waitingForDoubleClick) {
                waitingForDoubleClick = true
                singleClickTimer.start()
            } else {
                waitingForDoubleClick = false
                singleClickTimer.stop()
                toggleFullScreen()
            }
        }

        Timer {
            id: singleClickTimer
            interval: mouseArea.clickDelay
            running: false
            repeat: false
            onTriggered: {
                if (mouseArea.waitingForDoubleClick) {
                    mouseArea.waitingForDoubleClick = false
                }
            }
        }

        onWheel: (wheel) => {
            if (wheel.angleDelta.y > 0)
                zoomLevel = Math.min(zoomLevel + 0.1, 4.0)
            else
                zoomLevel = Math.max(zoomLevel - 0.1, 1.0)
            limitPan()
        }
    }

    Item {
        id: interactionLayer
        anchors.fill: parent
        visible: isMobile

        PinchArea {
            id: pinchArea
            anchors.fill: parent
            enabled: true

            property real startScale: 1.0
            property real prevScale: 1.0
            property point pinchCenter: Qt.point(0, 0)

            TapHandler {
                id: tapHandler
                acceptedDevices: PointerDevice.TouchScreen | PointerDevice.Mouse
                gesturePolicy: TapHandler.DragThreshold
                onDoubleTapped: toggleFullScreen()
            }

            DragHandler {
                id: dragHandler
                target: null
                acceptedDevices: PointerDevice.TouchScreen
                enabled: zoomLevel > 1.0

                property point lastPosition: Qt.point(0, 0)
                property bool isDragging: false

                onActiveChanged: {
                    if (active) {
                        isDragging = true
                        lastPosition = centroid.position
                    } else {
                        isDragging = false
                    }
                }

                onCentroidChanged: {
                    if (active && isDragging) {
                        var dx = centroid.position.x - lastPosition.x
                        var dy = centroid.position.y - lastPosition.y
                        panX += dx * 1.5
                        panY += dy * 1.5
                        limitPan()
                        lastPosition = centroid.position
                    }
                }
            }

            onPinchStarted: {
                startScale = zoomLevel
                prevScale = startScale
                pinchCenter = Qt.point(width / 2, height / 2)
            }

            onPinchUpdated: (pinch) => {
                var newScale = startScale * pinch.scale
                var oldZoomLevel = zoomLevel
                zoomLevel = Math.max(1.0, Math.min(newScale, 4.0))

                if (oldZoomLevel !== zoomLevel) {
                    var centerX = width / 2
                    var centerY = height / 2
                    var scaleFactor = zoomLevel / oldZoomLevel

                    if (zoomLevel > 1.0) {
                        panX = panX * scaleFactor
                        panY = panY * scaleFactor
                    } else {
                        panX = 0
                        panY = 0
                    }

                    prevScale = zoomLevel
                }

                limitPan()
            }

            onPinchFinished: {
                limitPan()
            }
        }
    }

    QGCLabel {
        text: "Zoom: " + zoomLevel.toFixed(1) + "x"
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        color: "white"
        visible: zoomLevel !== 1.0
        font.pointSize: ScreenTools.smallFontPointSize
    }

    ProximityRadarVideoView {
        anchors.fill: parent
        vehicle: QGroundControl.multiVehicleManager.activeVehicle
    }

    ObstacleDistanceOverlayVideo {
        id: obstacleDistance
        showText: pipState.state === pipState.fullState
    }
}
