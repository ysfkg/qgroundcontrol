// VideoSettings.cc

#include "VideoSettings.h"
#include "VideoManager.h"

#include <QtQml/QQmlEngine>
#include <QtCore/QVariantList>

#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
#endif

#ifndef QGC_DISABLE_UVC
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QCameraDevice>
#endif

DECLARE_SETTINGGROUP(Video, "Video")
{
    qmlRegisterUncreatableType<VideoSettings>("QGroundControl.SettingsManager", 1, 0, "VideoSettings", "Reference only");

            // Setup enum values for videoSource settings into meta data
    QVariantList videoSourceList;
#if defined(QGC_GST_STREAMING) || defined(QGC_QT_STREAMING)
    videoSourceList.append(videoSourceRTSP);
    videoSourceList.append(videoSourceUDPH264);
    videoSourceList.append(videoSourceUDPH265);
    videoSourceList.append(videoSourceTCP);
    videoSourceList.append(videoSourceMPEGTS);
    videoSourceList.append(videoSource3DRSolo);
    videoSourceList.append(videoSourceParrotDiscovery);
    videoSourceList.append(videoSourceYuneecMantisG);

#ifdef QGC_HERELINK_AIRUNIT_VIDEO
    videoSourceList.append(videoSourceHerelinkAirUnit);
#else
    videoSourceList.append(videoSourceHerelinkHotspot);
#endif
#endif
#ifndef QGC_DISABLE_UVC
    QList<QCameraDevice> videoInputs = QMediaDevices::videoInputs();
    for (const auto& cameraDevice: videoInputs) {
        videoSourceList.append(cameraDevice.description());
    }
#endif
    if (videoSourceList.count() == 0) {
        _noVideo = true;
        videoSourceList.append(videoSourceNoVideo);
        setVisible(false);
    } else {
        videoSourceList.insert(0, videoDisabled);
    }

            // make translated strings
    QStringList videoSourceCookedList;
    for (const QVariant& videoSource: videoSourceList) {
        videoSourceCookedList.append( VideoSettings::tr(videoSource.toString().toStdString().c_str()) );
    }

    _nameToMetaDataMap[videoSourceName]->setEnumInfo(videoSourceCookedList, videoSourceList);

            // Same setup for videoSource1
    QVariantList videoSourceList1 = videoSourceList;


    QStringList videoSourceCookedList1;
    for (const QVariant& videoSource1: videoSourceList1) {
        videoSourceCookedList1.append( VideoSettings::tr(videoSource1.toString().toStdString().c_str()) );
    }

    _nameToMetaDataMap[videoSource1Name]->setEnumInfo(videoSourceCookedList1, videoSourceList1);

    _setForceVideoDecodeList();

            // Set default value for videoSource
    _setDefaults();
    _setDefaults1();
}

void VideoSettings::_setDefaults()
{
    if (_noVideo) {
        _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoSourceNoVideo);
    } else {
        _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoDisabled);
    }
}

void VideoSettings::_setDefaults1()
{
    if (_noVideo) {
        _nameToMetaDataMap[videoSource1Name]->setRawDefaultValue(videoSourceNoVideo);
    } else {
        _nameToMetaDataMap[videoSource1Name]->setRawDefaultValue(videoDisabled);
    }
}

DECLARE_SETTINGSFACT(VideoSettings, aspectRatio)
DECLARE_SETTINGSFACT(VideoSettings, videoFit)
DECLARE_SETTINGSFACT(VideoSettings, gridLines)
DECLARE_SETTINGSFACT(VideoSettings, showRecControl)
DECLARE_SETTINGSFACT(VideoSettings, recordingFormat)
DECLARE_SETTINGSFACT(VideoSettings, maxVideoSize)
DECLARE_SETTINGSFACT(VideoSettings, enableStorageLimit)
DECLARE_SETTINGSFACT(VideoSettings, streamEnabled)
DECLARE_SETTINGSFACT(VideoSettings, disableWhenDisarmed)

// Secondary settings with "1" suffix
DECLARE_SETTINGSFACT(VideoSettings, aspectRatio1)
DECLARE_SETTINGSFACT(VideoSettings, videoFit1)
DECLARE_SETTINGSFACT(VideoSettings, gridLines1)
DECLARE_SETTINGSFACT(VideoSettings, showRecControl1)
DECLARE_SETTINGSFACT(VideoSettings, recordingFormat1)
DECLARE_SETTINGSFACT(VideoSettings, maxVideoSize1)
DECLARE_SETTINGSFACT(VideoSettings, enableStorageLimit1)
DECLARE_SETTINGSFACT(VideoSettings, streamEnabled1)
DECLARE_SETTINGSFACT(VideoSettings, disableWhenDisarmed1)

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, videoSource)
{
    if (!_videoSourceFact) {
        _videoSourceFact = _createSettingsFact(videoSourceName);
        //-- Check for sources no longer available
        if(!_videoSourceFact->enumValues().contains(_videoSourceFact->rawValue().toString())) {
            if (_noVideo) {
                _videoSourceFact->setRawValue(videoSourceNoVideo);
            } else {
                _videoSourceFact->setRawValue(videoDisabled);
            }
        }
        connect(_videoSourceFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _videoSourceFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, videoSource1)
{
    if (!_videoSource1Fact) {
        _videoSource1Fact = _createSettingsFact(videoSource1Name);
        //-- Check for sources no longer available
        if(!_videoSource1Fact->enumValues().contains(_videoSource1Fact->rawValue().toString())) {
            if (_noVideo1) {
                _videoSource1Fact->setRawValue(videoSourceNoVideo);
            } else {
                _videoSource1Fact->setRawValue(videoDisabled);
            }
        }
        connect(_videoSource1Fact, &Fact::valueChanged, this, &VideoSettings::_configChanged1);
    }
    return _videoSource1Fact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, forceVideoDecoder)
{
    if (!_forceVideoDecoderFact) {
        _forceVideoDecoderFact = _createSettingsFact(forceVideoDecoderName);

        _forceVideoDecoderFact->setVisible(
#ifdef QGC_GST_STREAMING
            true
#else
            false
#endif
            );

        connect(_forceVideoDecoderFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _forceVideoDecoderFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, forceVideoDecoder1)
{
    if (!_forceVideoDecoder1Fact) {
        _forceVideoDecoder1Fact = _createSettingsFact(forceVideoDecoder1Name);

        _forceVideoDecoder1Fact->setVisible(
#ifdef QGC_GST_STREAMING
            true
#else
            false
#endif
            );

        connect(_forceVideoDecoder1Fact, &Fact::valueChanged, this, &VideoSettings::_configChanged1);
    }
    return _forceVideoDecoder1Fact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, lowLatencyMode)
{
    if (!_lowLatencyModeFact) {
        _lowLatencyModeFact = _createSettingsFact(lowLatencyModeName);

        _lowLatencyModeFact->setVisible(
#ifdef QGC_GST_STREAMING
            true
#else
            false
#endif
            );

        connect(_lowLatencyModeFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _lowLatencyModeFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, lowLatencyMode1)
{
    if (!_lowLatencyMode1Fact) {
        _lowLatencyMode1Fact = _createSettingsFact(lowLatencyMode1Name);

        _lowLatencyMode1Fact->setVisible(
#ifdef QGC_GST_STREAMING
            true
#else
            false
#endif
            );

        connect(_lowLatencyMode1Fact, &Fact::valueChanged, this, &VideoSettings::_configChanged1);
    }
    return _lowLatencyMode1Fact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtspTimeout)
{
    if (!_rtspTimeoutFact) {
        _rtspTimeoutFact = _createSettingsFact(rtspTimeoutName);

        _rtspTimeoutFact->setVisible(
#ifdef QGC_GST_STREAMING
            true
#else
            false
#endif
            );

        connect(_rtspTimeoutFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _rtspTimeoutFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtspTimeout1)
{
    if (!_rtspTimeout1Fact) {
        _rtspTimeout1Fact = _createSettingsFact(rtspTimeout1Name);

        _rtspTimeout1Fact->setVisible(
#ifdef QGC_GST_STREAMING
            true
#else
            false
#endif
            );

        connect(_rtspTimeout1Fact, &Fact::valueChanged, this, &VideoSettings::_configChanged1);
    }
    return _rtspTimeout1Fact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, udpPort)
{
    if (!_udpPortFact) {
        _udpPortFact = _createSettingsFact(udpPortName);
        connect(_udpPortFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _udpPortFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, udpPort1)
{
    if (!_udpPort1Fact) {
        _udpPort1Fact = _createSettingsFact(udpPort1Name);
        connect(_udpPort1Fact, &Fact::valueChanged, this, &VideoSettings::_configChanged1);
    }
    return _udpPort1Fact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtspUrl)
{
    if (!_rtspUrlFact) {
        _rtspUrlFact = _createSettingsFact(rtspUrlName);
        connect(_rtspUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _rtspUrlFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtspUrl1)
{
    if (!_rtspUrl1Fact) {
        _rtspUrl1Fact = _createSettingsFact(rtspUrl1Name);
        connect(_rtspUrl1Fact, &Fact::valueChanged, this, &VideoSettings::_configChanged1);
    }
    return _rtspUrl1Fact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, tcpUrl)
{
    if (!_tcpUrlFact) {
        _tcpUrlFact = _createSettingsFact(tcpUrlName);
        connect(_tcpUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _tcpUrlFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, tcpUrl1)
{
    if (!_tcpUrl1Fact) {
        _tcpUrl1Fact = _createSettingsFact(tcpUrl1Name);
        connect(_tcpUrl1Fact, &Fact::valueChanged, this, &VideoSettings::_configChanged1);
    }
    return _tcpUrl1Fact;
}

bool VideoSettings::streamConfigured(void)
{
    //-- First, check if it's autoconfigured
    if(VideoManager::instance()->autoStreamConfigured()) {
        qCDebug(VideoManagerLog) << "Stream auto configured";
        return true;
    }
    //-- Check if it's disabled
    QString vSource = videoSource()->rawValue().toString();
    if(vSource == videoSourceNoVideo || vSource == videoDisabled) {
        return false;
    }
    //-- If UDP, check if port is set
    if(vSource == videoSourceUDPH264 || vSource == videoSourceUDPH265) {
        qCDebug(VideoManagerLog) << "Testing configuration for UDP Stream:" << udpPort()->rawValue().toInt();
        return udpPort()->rawValue().toInt() != 0;
    }
    //-- If RTSP, check for URL
    if(vSource == videoSourceRTSP) {
        qCDebug(VideoManagerLog) << "Testing configuration for RTSP Stream:" << rtspUrl()->rawValue().toString();
        return !rtspUrl()->rawValue().toString().isEmpty();
    }
    //-- If TCP, check for URL
    if(vSource == videoSourceTCP) {
        qCDebug(VideoManagerLog) << "Testing configuration for TCP Stream:" << tcpUrl()->rawValue().toString();
        return !tcpUrl()->rawValue().toString().isEmpty();
    }
    //-- If MPEG-TS, check if port is set
    if(vSource == videoSourceMPEGTS) {
        qCDebug(VideoManagerLog) << "Testing configuration for MPEG-TS Stream:" << udpPort()->rawValue().toInt();
        return udpPort()->rawValue().toInt() != 0;
    }
    //-- If Herelink Air unit, good to go
    if(vSource == videoSourceHerelinkAirUnit) {
        qCDebug(VideoManagerLog) << "Stream configured for Herelink Air Unit";
        return true;
    }
    //-- If Herelink Hotspot, good to go
    if(vSource == videoSourceHerelinkHotspot) {
        qCDebug(VideoManagerLog) << "Stream configured for Herelink Hotspot";
        return true;
    }
#ifndef QGC_DISABLE_UVC
    const QList<QCameraDevice> videoInputs = QMediaDevices::videoInputs();
    for (const auto& cameraDevice: videoInputs) {
        if(vSource == cameraDevice.description()) {
            qCDebug(VideoManagerLog) << "Stream configured for UVC";
            return true;
        }
    }
#endif
    return false;
}

bool VideoSettings::streamConfigured1(void)
{
    //-- First, check if it's autoconfigured (using the same VideoManager instance)
    if(VideoManager::instance()->autoStreamConfigured()) {
        qCDebug(VideoManagerLog) << "Stream1 auto configured";
        return true;
    }
    //-- Check if it's disabled
    QString vSource = videoSource1()->rawValue().toString();
    if(vSource == videoSourceNoVideo || vSource == videoDisabled) {
        return false;
    }
    //-- If UDP, check if port is set
    if(vSource == videoSourceUDPH264 || vSource == videoSourceUDPH265) {
        qCDebug(VideoManagerLog) << "Testing configuration for UDP Stream1:" << udpPort1()->rawValue().toInt();
        return udpPort1()->rawValue().toInt() != 0;
    }
    //-- If RTSP, check for URL
    if(vSource == videoSourceRTSP) {
        qCDebug(VideoManagerLog) << "Testing configuration for RTSP Stream1:" << rtspUrl1()->rawValue().toString();
        return !rtspUrl1()->rawValue().toString().isEmpty();
    }
    //-- If TCP, check for URL
    if(vSource == videoSourceTCP) {
        qCDebug(VideoManagerLog) << "Testing configuration for TCP Stream1:" << tcpUrl1()->rawValue().toString();
        return !tcpUrl1()->rawValue().toString().isEmpty();
    }
    //-- If MPEG-TS, check if port is set
    if(vSource == videoSourceMPEGTS) {
        qCDebug(VideoManagerLog) << "Testing configuration for MPEG-TS Stream1:" << udpPort1()->rawValue().toInt();
        return udpPort1()->rawValue().toInt() != 0;
    }
    //-- If Herelink Air unit, good to go
    if(vSource == videoSourceHerelinkAirUnit) {
        qCDebug(VideoManagerLog) << "Stream1 configured for Herelink Air Unit";
        return true;
    }
    //-- If Herelink Hotspot, good to go
    if(vSource == videoSourceHerelinkHotspot) {
        qCDebug(VideoManagerLog) << "Stream1 configured for Herelink Hotspot";
        return true;
    }
#ifndef QGC_DISABLE_UVC
    const QList<QCameraDevice> videoInputs = QMediaDevices::videoInputs();
    for (const auto& cameraDevice: videoInputs) {
        if(vSource == cameraDevice.description()) {
            qCDebug(VideoManagerLog) << "Stream1 configured for UVC";
            return true;
        }
    }
#endif
    return false;
}

void VideoSettings::_configChanged(QVariant)
{
    emit streamConfiguredChanged(streamConfigured());
}

void VideoSettings::_configChanged1(QVariant)
{
    emit streamConfigured1Changed(streamConfigured1());
}

void VideoSettings::_setForceVideoDecodeList()
{
#ifdef QGC_GST_STREAMING
    const QVariantList removeForceVideoDecodeList{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
        GStreamer::VideoDecoderOptions::ForceVideoDecoderDirectX3D,
        GStreamer::VideoDecoderOptions::ForceVideoDecoderVideoToolbox,
#elif defined(Q_OS_WIN)
        GStreamer::VideoDecoderOptions::ForceVideoDecoderVAAPI,
        GStreamer::VideoDecoderOptions::ForceVideoDecoderVideoToolbox,
#elif defined(Q_OS_MACOS)
        GStreamer::VideoDecoderOptions::ForceVideoDecoderDirectX3D,
        GStreamer::VideoDecoderOptions::ForceVideoDecoderVAAPI,
#elif defined(Q_OS_ANDROID)
        GStreamer::VideoDecoderOptions::ForceVideoDecoderDirectX3D,
        GStreamer::VideoDecoderOptions::ForceVideoDecoderVideoToolbox,
        GStreamer::VideoDecoderOptions::ForceVideoDecoderVAAPI,
        GStreamer::VideoDecoderOptions::ForceVideoDecoderNVIDIA,
#endif
    };

    for (const auto &value : removeForceVideoDecodeList) {
        _nameToMetaDataMap[forceVideoDecoderName]->removeEnumInfo(value);
        _nameToMetaDataMap[forceVideoDecoder1Name]->removeEnumInfo(value);
    }
#endif
}

