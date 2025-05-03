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


    // Platformları algılama
    property bool isMobile: Qt.platform.os === "android" || Qt.platform.os === "ios"

    // Tam ekran geçişi için yardımcı fonksiyon
    function toggleFullScreen() {
        console.log("Tam ekran durumu değiştiriliyor")
        QGroundControl.videoManager.fullScreen = !QGroundControl.videoManager.fullScreen
    }

    // Limitli pan değerleri
    function limitPan() {
        if (zoomLevel <= 1.0) {
            // Zoom seviyesi 1 veya daha az ise, ortalanmış durumda kalır
            panX = 0
            panY = 0
            return
        }

        // Görüntünün gerçek genişlik ve yüksekliği
        var viewWidth = videoWrapper.width
        var viewHeight = videoWrapper.height

        // Zoom nedeniyle genişlemiş boyutlar
        var scaledWidth = viewWidth * zoomLevel
        var scaledHeight = viewHeight * zoomLevel

        // İzin verilen hareket limitleri (görüntünün dışına çıkmamasını sağlar)
        var maxPanX = (scaledWidth - viewWidth) / 2
        var maxPanY = (scaledHeight - viewHeight) / 2

        // Pan değerlerini sınırla
        panX = Math.max(-maxPanX, Math.min(panX, maxPanX))
        panY = Math.max(-maxPanY, Math.min(panY, maxPanY))
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

        // Burada scale ve transform özelliklerini ayrı tutuyoruz
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

        FlightDisplayViewVideo {
            id: videoStreaming
            anchors.fill: parent
            useSmallFont: _root.pipState.state !== _root.pipState.fullState
            visible: QGroundControl.videoManager.isStreamSource
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

        onPressed:(mouse) => {
            startPos = Qt.point(mouse.x, mouse.y)
            lastMousePos = startPos
            hasMovedSignificantly = false
            console.log("Mouse/Touch pressed at: " + mouse.x + "," + mouse.y)
        }

        onPositionChanged:(mouse) => {
            if (pressed && zoomLevel > 1.0) {
                var dx = mouse.x - lastMousePos.x
                var dy = mouse.y - lastMousePos.y

                // Önemli bir hareket var mı kontrol et
                if (Math.abs(mouse.x - startPos.x) > 5 || Math.abs(mouse.y - startPos.y) > 5) {
                    hasMovedSignificantly = true
                }

                // Tek parmak kaydırma - PAN
                panX += dx * 1.5  // Duyarlılık çarpanı
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
                    console.log("🖱🖱 Çift tıklama algılandı")
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
                        console.log("🖱 Tek tıklama algılandı")
                        // Buraya tek tık işlemini yaz
                    }
                }
            }

        // Fare tekerleği kaydırma işlevi
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
                gesturePolicy: TapHandler.DragThreshold  // veya TapHandler.WithinBounds
                onDoubleTapped:{
                    toggleFullScreen()
                }
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
                        console.log("Sürükleme başladı: " + lastPosition.x + "," + lastPosition.y)
                    } else {
                        isDragging = false
                        console.log("Sürükleme bitti")
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
                        console.log("Sürükleniyor: " + dx + "," + dy)
                    }
                }
            }

            onPinchStarted: {
                startScale = zoomLevel
                prevScale = startScale
                // Pinch merkezi olarak ekranın merkezini kullan
                pinchCenter = Qt.point(width / 2, height / 2)
                console.log("Pinch başladı, başlangıç zoom: " + startScale + ", merkez: " + pinchCenter.x + "," + pinchCenter.y)
            }

            onPinchUpdated: (pinch) => {
                // Yeni zoom seviyesini hesapla
                var newScale = startScale * pinch.scale
                var oldZoomLevel = zoomLevel
                zoomLevel = Math.max(1.0, Math.min(newScale, 4.0))

                // Ekranın merkezinden yakınlaştırma/uzaklaştırma yapmak için
                if (oldZoomLevel !== zoomLevel) {
                    // Merkez noktası olarak ekranın ortasını kullan
                    var centerX = width / 2
                    var centerY = height / 2

                    // Zoom uygulandığında pan değerlerini merkez etrafında ayarla
                    var scaleFactor = zoomLevel / oldZoomLevel

                    // Merkezi korumak için pan değerlerini güncelle
                    // Bu formül, yakınlaştırma/uzaklaştırma sırasında merkez noktasının sabit kalmasını sağlar
                    if (zoomLevel > 1.0) {
                        // Ekranın merkezini koruyacak şekilde pan değerlerini ayarla
                        // Bu değerleri 0'a yaklaştırmak merkeze doğru yakınlaştırır
                        panX = panX * scaleFactor
                        panY = panY * scaleFactor
                    } else {
                        // Zoom seviyesi 1'e eşit veya daha küçükse, merkezi sıfırla
                        panX = 0
                        panY = 0
                    }

                    prevScale = zoomLevel
                }

                limitPan()
            }

            onPinchFinished: {
                console.log("Pinch bitti, son zoom: " + zoomLevel)
                limitPan()
            }
        }
    }





    // Görsel zoom göstergesi
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
