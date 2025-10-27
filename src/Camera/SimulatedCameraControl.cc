/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "SimulatedCameraControl.h"
#include "VideoManager.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "FlyViewSettings.h"
#include "Vehicle.h"
#include "CameraUdpManager.h"

#include <QtQml/QQmlEngine>

#include <QUdpSocket>
#include <QHostAddress>
#include <QDateTime>
#include <QTimeZone>
#include <QByteArray>
#include <QString>

//-----------------------------------------------------------------------------
// Zaman senkronizasyonu için yardımcı fonksiyonlar
//-----------------------------------------------------------------------------
static QString toHex2Upper(uint8_t v) {
    return QString("%1").arg(v, 2, 16, QLatin1Char('0')).toUpper();
}

static QByteArray addCrc(const QByteArray &cmdWithoutCrc) {
    int crc = 0;
    for (auto b : cmdWithoutCrc) crc += static_cast<unsigned char>(b);
    uint8_t crcByte = static_cast<uint8_t>(crc & 0xFF);
    QString cmdWithCrc = QString::fromUtf8(cmdWithoutCrc) + toHex2Upper(crcByte);
    return cmdWithCrc.toUtf8();
}

static QByteArray buildTimCommandForNow() {
    // Zaman dilimini Europe/Istanbul olarak ayarlıyoruz
    QTimeZone tz("Europe/Istanbul");
    QDateTime now = QDateTime::currentDateTimeUtc();
    if (tz.isValid()) now = now.toTimeZone(tz);

    // hh mm ss
    int hh = now.time().hour();
    int mm = now.time().minute();
    int ss = now.time().second();
    int msec = now.time().msec();

    // PDF formatı: hhmmss.ssDDMMYY -- burada .ss kısmını centisecond (ms/10) alıyoruz
    int centisec = (msec / 10) % 100; // 0..99

    int day = now.date().day();
    int month = now.date().month();
    int year2 = now.date().year() % 100; // YY

    QString timestamp = QString("%1%2%3.%4%5%6%7")
        .arg(hh, 2, 10, QLatin1Char('0'))
        .arg(mm, 2, 10, QLatin1Char('0'))
        .arg(ss, 2, 10, QLatin1Char('0'))
        .arg(centisec, 2, 10, QLatin1Char('0'))
        .arg(day, 2, 10, QLatin1Char('0'))
        .arg(month, 2, 10, QLatin1Char('0'))
        .arg(year2, 2, 10, QLatin1Char('0'));

    QString cmd = QString("#TPUDFwTIM%1").arg(timestamp);
    return addCrc(cmd.toUtf8()); // CRC eklenmiş QByteArray döner
}

static void sendTimeToCamera() {
    QByteArray packet = buildTimCommandForNow();
    CameraUdpManager::instance()->sendCommand(packet, true);  // Öncelikli komut
    qCDebug(CameraControlLog) << "Time sync command queued:" << packet.size() << "bytes";
}

//-----------------------------------------------------------------------------
SimulatedCameraControl::SimulatedCameraControl(Vehicle* vehicle, QObject* parent)
    : MavlinkCameraControl  (parent)
      , _vehicle              (vehicle)
{
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);

    connect(VideoManager::instance(), &VideoManager::recordingChanged, this, [this]() {
        bool isRecording = VideoManager::instance()->recording();
        qCCritical(CameraControlLog) << "===== VIDEO RECORDING STATUS CHANGED =====" << isRecording;
        qCCritical(CameraControlLog) << "  Timer active:" << _videoRecordTimeUpdateTimer.isActive();
        qCCritical(CameraControlLog) << "  Elapsed time:" << _videoRecordTimeElapsedTimer.elapsed() << "ms";
        qCCritical(CameraControlLog) << "  Vehicle:" << (_vehicle ? _vehicle->id() : -1);
        qCCritical(CameraControlLog) << "  Callback stack:";
        qCCritical(CameraControlLog) << "    - This function called due to VideoManager recordingChanged signal";
        
        // Eğer kayıt durdu ve timer hala aktifse, beklenmeyen bir durum
        if (!isRecording && _videoRecordTimeUpdateTimer.isActive()) {
            qCCritical(CameraControlLog) << "  !!!!! WARNING: Recording stopped unexpectedly!";
            qCCritical(CameraControlLog) << "  !!!!! Sending stop command to camera to keep sync.";
            qCCritical(CameraControlLog) << "  !!!!! Check VideoManager/VideoReceiver logs for the root cause!";
            qCCritical(CameraControlLog) << "  !!!!! This usually happens due to:";
            qCCritical(CameraControlLog) << "  !!!!!   1. VideoReceiver timeout (>10s no frames)";
            qCCritical(CameraControlLog) << "  !!!!!   2. GStreamer error";
            qCCritical(CameraControlLog) << "  !!!!!   3. Vehicle/connection issue";
            
            // Beklenmeyen durum: VideoReceiver kayıt durdu ama bizim timer aktif
            // Kameraya dur komutu gönder
            static const QByteArray stopCommand = "#TPUD2wREC0043";
            CameraUdpManager::instance()->sendCommand(stopCommand, true);
            qCDebug(CameraControlLog) << "Unexpected stop - camera command sent:" << stopCommand;
            
            // Timer'ı durdur
            _videoRecordTimeUpdateTimer.stop();
        }
        
        emit videoCaptureStatusChanged();
    });

    auto flyViewSettings = SettingsManager::instance()->flyViewSettings();
    connect(flyViewSettings->showSimpleCameraControl(), &Fact::rawValueChanged, this, &SimulatedCameraControl::infoChanged);

    _videoRecordTimeUpdateTimer.setInterval(1000);
    connect(&_videoRecordTimeUpdateTimer, &QTimer::timeout, this, &SimulatedCameraControl::recordTimeChanged);

    // C12 kameraya zaman bilgisi gönder - ağ ve kamera hazır olması için gecikme ekle
    QTimer::singleShot(2000, this, [this]() {
        qCDebug(CameraControlLog) << "Sending initial time sync to camera...";
        sendTimeToCamera();
        qCDebug(CameraControlLog) << "Camera time synchronized (delayed initialization)";
        
        // 5 saniye sonra tekrar gönder (kameraya emin olsun diye)
        QTimer::singleShot(5000, this, [this]() {
            qCDebug(CameraControlLog) << "Sending second time sync to camera...";
            sendTimeToCamera();
        });
    });
}

SimulatedCameraControl::~SimulatedCameraControl()
{

}

QString SimulatedCameraControl::recordTimeStr()
{
    return QTime(0, 0).addMSecs(static_cast<int>(recordTime())).toString("hh:mm:ss");
}

SimulatedCameraControl::VideoCaptureStatus SimulatedCameraControl::videoCaptureStatus()
{
    return _videoCaptureStatus = VideoManager::instance()->recording() ? VIDEO_CAPTURE_STATUS_RUNNING : VIDEO_CAPTURE_STATUS_STOPPED;
}

void SimulatedCameraControl::setCameraMode(CameraMode mode)
{
    qCDebug(CameraControlLog) << "setCameraMode" << cameraModeToStr(mode);

    if (hasModes()) {
        if (mode == CAM_MODE_VIDEO) {
            _setCameraMode(CAM_MODE_VIDEO);
        } else if (mode == CAM_MODE_PHOTO) {
            _setCameraMode(CAM_MODE_PHOTO);
        } else {
            qCWarning(CameraControlLog) << "setCameraMode invalid mode" << mode;
        }
    } else {
        qCWarning(CameraControlLog) << "setCameraMode called when camera does not support modes";
    }
}

void SimulatedCameraControl::_setCameraMode(CameraMode mode)
{
    if (_cameraMode != mode) {
        _cameraMode = mode;
        emit cameraModeChanged();
    }
}

void SimulatedCameraControl::toggleCameraMode()
{
    if(cameraMode() == CAM_MODE_PHOTO || cameraMode() == CAM_MODE_SURVEY) {
        setCameraModeVideo();
    } else if(cameraMode() == CAM_MODE_VIDEO) {
        setCameraModePhoto();
    }
}

bool SimulatedCameraControl::toggleVideoRecording()
{
    if(videoCaptureStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        return stopVideoRecording();
    } else {
        return startVideoRecording();
    }
}

void SimulatedCameraControl::setCameraModeVideo()
{
    qCDebug(CameraControlLog) << "setCameraModeVideo()";

    if (!hasModes()) {
        qCWarning(CameraControlLog) << "setCameraModeVideo: Camera does not support modes";
        return;
    }

    _setCameraMode(CAM_MODE_VIDEO);
}

void SimulatedCameraControl::setCameraModePhoto()
{
    qCDebug(CameraControlLog) << "setCameraModePhoto()";

    if (!hasModes()) {
        qCWarning(CameraControlLog) << "setCameraModePhoto: Camera does not support modes";
        return;
    }

    _setCameraMode(CAM_MODE_PHOTO);
}


bool SimulatedCameraControl::takePhoto()
{

    if (!capturesPhotos()) {
        qCWarning(CameraControlLog) << "takePhoto: Camera does not handle image capture";
        return false;
    }
    if (photoCaptureStatus() != PHOTO_CAPTURE_IDLE) {
        qCWarning(CameraControlLog) << "Camera not idle";
        return false;
    }
    if (cameraMode() != CAM_MODE_PHOTO && cameraMode() != CAM_MODE_SURVEY) {
        qCWarning(CameraControlLog) << "takePhoto: Camera not in correct mode:" << cameraModeToStr(cameraMode());
        return false;
    }

    if (photoCaptureMode() == PHOTO_CAPTURE_SINGLE) {
        _vehicle->triggerSimpleCamera();
        VideoManager::instance()->grabImage();

        if (VideoManager::instance()->hasVideo1()) {
            QTimer::singleShot(50, this, []() {
                VideoManager::instance()->grabImage1();
            });

        }

                // UDP ile C12 kameraya fotoğraf çekme komutu gönder
        static const QByteArray command = "#TPUD2wCAP013E";  // Komut sabit
        CameraUdpManager::instance()->sendCommand(command, true);  // Öncelikli komut
        qCDebug(CameraControlLog) << "Photo command queued (priority):" << command;

        _photoCaptureStatus = PHOTO_CAPTURE_IN_PROGRESS;
        emit photoCaptureStatusChanged();
        QTimer::singleShot(500, [this]() { _photoCaptureStatus = PHOTO_CAPTURE_IDLE; emit photoCaptureStatusChanged(); });


    } else if (photoCaptureMode() == PHOTO_CAPTURE_TIMELAPSE) {
        qgcApp()->showAppMessage(tr("Time lapse capture not supported by this camera"));
    }

    return true;
}

bool SimulatedCameraControl::startVideoRecording()
{
    qCDebug(CameraControlLog) << "startVideoRecording()";

    if (!capturesVideo()) {
        qCWarning(CameraControlLog) << "startVideoRecording: Camera does not handle video capture";
        return false;
    }
    if (cameraMode() == CAM_MODE_PHOTO) {
        qCWarning(CameraControlLog) << "startVideoRecording: Camera does not take video in photo mode";
        return false;
    }
    if(videoCaptureStatus() == VIDEO_CAPTURE_STATUS_RUNNING) {
        qCWarning(CameraControlLog) << "startVideoRecording: Camera already recording";
        return false;
    }

    // Önce C12 kameraya kayıt başlat komutu gönder
    static const QByteArray command = "#TPUD2wREC0144";  // C12 video kayıt başlatma komutu
    CameraUdpManager::instance()->sendCommand(command, true);  // Öncelikli komut
    qCDebug(CameraControlLog) << "Video start command queued (priority):" << command;

    // Timer'ları başlat
    _videoRecordTimeUpdateTimer.start();
    _videoRecordTimeElapsedTimer.start();
    
    // Video 0 kaydını başlat
    VideoManager::instance()->startRecording();
    qCDebug(CameraControlLog) << "Video0 startRecording called";

    // Video 1 varsa onu da başlat (daha kısa gecikme)
    if (VideoManager::instance()->hasVideo1()) {
        QTimer::singleShot(100, this, [this]() {
            VideoManager::instance()->startRecording1();
            qCDebug(CameraControlLog) << "Video1 recording started";
        });
    }

    // Status değişikliğini bildir (VideoManager recordingChanged sinyali zaten emit ediyor ama yine de)
    QTimer::singleShot(200, this, [this]() {
        emit videoCaptureStatusChanged();
    });

    return true;  // Başarılı!
}

bool SimulatedCameraControl::stopVideoRecording()
{
    qCDebug(CameraControlLog) << "stopVideoRecording()";

    // VideoManager'ın recording durumunu kontrol et
    bool isRecording = VideoManager::instance()->recording();
    qCDebug(CameraControlLog) << "Current recording status:" << isRecording;

    if(!isRecording) {
        qCWarning(CameraControlLog) << "stopVideoRecording: Camera not recording";
        return false;
    }

    // Önce C12 kameraya dur komutu gönder
    static const QByteArray command = "#TPUD2wREC0043";  // C12 video kayıt durdurma komutu
    CameraUdpManager::instance()->sendCommand(command, true);  // Öncelikli komut
    qCDebug(CameraControlLog) << "Video stop command queued (priority):" << command;

    // Timer'ı durdur
    _videoRecordTimeUpdateTimer.stop();
    
    // Video 0 kaydını durdur
    VideoManager::instance()->stopRecording();
    qCDebug(CameraControlLog) << "Video0 stopRecording called";

    // Video 1 varsa onu da durdur (hemen)
    if (VideoManager::instance()->hasVideo1()) {
        VideoManager::instance()->stopRecording1();
        qCDebug(CameraControlLog) << "Video1 stopRecording called";
    }

    // Status değişikliğini bildir
    emit videoCaptureStatusChanged();

    return true;
}

quint32  SimulatedCameraControl::recordTime()
{
    if (_videoRecordTimeUpdateTimer.isActive()) {
        return _videoRecordTimeElapsedTimer.elapsed();
    } else {
        return 0;
    }
}

bool SimulatedCameraControl::capturesVideo()
{
    return VideoManager::instance()->hasVideo();
}

void SimulatedCameraControl::setPhotoLapse(double)
{
   // FIXME: NYI
}

bool SimulatedCameraControl::capturesPhotos()
{
    return SettingsManager::instance()->flyViewSettings()->showSimpleCameraControl()->rawValue().toBool();
}

bool SimulatedCameraControl::hasVideoStream()
{
    return VideoManager::instance()->hasVideo();
}

void SimulatedCameraControl::setPhotoLapseCount(int)
{
   // FIXME: NYI
}

void SimulatedCameraControl::setPhotoCaptureMode(MavlinkCameraControl::PhotoCaptureMode photoCaptureMode)
{
    if (_photoCaptureMode != photoCaptureMode) {
        _photoCaptureMode = photoCaptureMode;
        emit photoCaptureModeChanged();
    }
}

bool SimulatedCameraControl::hasModes()
{
    if (capturesPhotos() && capturesVideo()) {
        return true;
    } else {
        return false;
    }
}
