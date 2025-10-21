// VideoSettings.h

#pragma once

#include "SettingsGroup.h"

class VideoSettings : public SettingsGroup
{
    Q_OBJECT

   public:
    VideoSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

            // Original video settings
    DEFINE_SETTINGFACT(videoSource)
    DEFINE_SETTINGFACT(udpPort)
    DEFINE_SETTINGFACT(tcpUrl)
    DEFINE_SETTINGFACT(rtspUrl)
    DEFINE_SETTINGFACT(aspectRatio)
    DEFINE_SETTINGFACT(videoFit)
    DEFINE_SETTINGFACT(gridLines)
    DEFINE_SETTINGFACT(showRecControl)
    DEFINE_SETTINGFACT(recordingFormat)
    DEFINE_SETTINGFACT(maxVideoSize)
    DEFINE_SETTINGFACT(enableStorageLimit)
    DEFINE_SETTINGFACT(rtspTimeout)
    DEFINE_SETTINGFACT(streamEnabled)
    DEFINE_SETTINGFACT(disableWhenDisarmed)
    DEFINE_SETTINGFACT(lowLatencyMode)
    DEFINE_SETTINGFACT(forceVideoDecoder)

            // Secondary video settings with "1" suffix
    DEFINE_SETTINGFACT(videoSource1)
    DEFINE_SETTINGFACT(udpPort1)
    DEFINE_SETTINGFACT(tcpUrl1)
    DEFINE_SETTINGFACT(rtspUrl1)
    DEFINE_SETTINGFACT(aspectRatio1)
    DEFINE_SETTINGFACT(videoFit1)
    DEFINE_SETTINGFACT(gridLines1)
    DEFINE_SETTINGFACT(showRecControl1)
    DEFINE_SETTINGFACT(recordingFormat1)
    DEFINE_SETTINGFACT(maxVideoSize1)
    DEFINE_SETTINGFACT(enableStorageLimit1)
    DEFINE_SETTINGFACT(rtspTimeout1)
    DEFINE_SETTINGFACT(streamEnabled1)
    DEFINE_SETTINGFACT(disableWhenDisarmed1)
    DEFINE_SETTINGFACT(lowLatencyMode1)
    DEFINE_SETTINGFACT(forceVideoDecoder1)

            // Original properties
    Q_PROPERTY(bool     streamConfigured        READ streamConfigured       NOTIFY streamConfiguredChanged)
    Q_PROPERTY(QString  rtspVideoSource         READ rtspVideoSource        CONSTANT)
    Q_PROPERTY(QString  udp264VideoSource       READ udp264VideoSource      CONSTANT)
    Q_PROPERTY(QString  udp265VideoSource       READ udp265VideoSource      CONSTANT)
    Q_PROPERTY(QString  tcpVideoSource          READ tcpVideoSource         CONSTANT)
    Q_PROPERTY(QString  mpegtsVideoSource       READ mpegtsVideoSource      CONSTANT)
    Q_PROPERTY(QString  disabledVideoSource     READ disabledVideoSource    CONSTANT)

            // Secondary properties with "1" suffix
    Q_PROPERTY(bool     streamConfigured1       READ streamConfigured1      NOTIFY streamConfigured1Changed)
    Q_PROPERTY(QString  rtspVideoSource1        READ rtspVideoSource1       CONSTANT)
    Q_PROPERTY(QString  udp264VideoSource1      READ udp264VideoSource1     CONSTANT)
    Q_PROPERTY(QString  udp265VideoSource1      READ udp265VideoSource1     CONSTANT)
    Q_PROPERTY(QString  tcpVideoSource1         READ tcpVideoSource1        CONSTANT)
    Q_PROPERTY(QString  mpegtsVideoSource1      READ mpegtsVideoSource1     CONSTANT)
    Q_PROPERTY(QString  disabledVideoSource1    READ disabledVideoSource1   CONSTANT)

            // Original methods
    bool     streamConfigured       ();
    QString  rtspVideoSource        () { return videoSourceRTSP; }
    QString  udp264VideoSource      () { return videoSourceUDPH264; }
    QString  udp265VideoSource      () { return videoSourceUDPH265; }
    QString  tcpVideoSource         () { return videoSourceTCP; }
    QString  mpegtsVideoSource      () { return videoSourceMPEGTS; }
    QString  disabledVideoSource    () { return videoDisabled; }

            // Secondary methods with "1" suffix
    bool     streamConfigured1      ();
    QString  rtspVideoSource1       () { return videoSourceRTSP; }
    QString  udp264VideoSource1     () { return videoSourceUDPH264; }
    QString  udp265VideoSource1     () { return videoSourceUDPH265; }
    QString  tcpVideoSource1        () { return videoSourceTCP; }
    QString  mpegtsVideoSource1     () { return videoSourceMPEGTS; }
    QString  disabledVideoSource1   () { return videoDisabled; }

            // Video source constants (shared between both streams)
    static constexpr const char* videoSourceNoVideo           = QT_TRANSLATE_NOOP("VideoSettings", "No Video Available");
    static constexpr const char* videoDisabled                = QT_TRANSLATE_NOOP("VideoSettings", "Video Stream Disabled");
    static constexpr const char* videoSourceRTSP              = QT_TRANSLATE_NOOP("VideoSettings", "RTSP Video Stream");
    static constexpr const char* videoSourceUDPH264           = QT_TRANSLATE_NOOP("VideoSettings", "UDP h.264 Video Stream");
    static constexpr const char* videoSourceUDPH265           = QT_TRANSLATE_NOOP("VideoSettings", "UDP h.265 Video Stream");
    static constexpr const char* videoSourceTCP               = QT_TRANSLATE_NOOP("VideoSettings", "TCP-MPEG2 Video Stream");
    static constexpr const char* videoSourceMPEGTS            = QT_TRANSLATE_NOOP("VideoSettings", "MPEG-TS (h.264) Video Stream");
    static constexpr const char* videoSource3DRSolo           = QT_TRANSLATE_NOOP("VideoSettings", "3DR Solo (requires restart)");
    static constexpr const char* videoSourceParrotDiscovery   = QT_TRANSLATE_NOOP("VideoSettings", "Parrot Discovery");
    static constexpr const char* videoSourceYuneecMantisG     = QT_TRANSLATE_NOOP("VideoSettings", "Yuneec Mantis G");
    static constexpr const char* videoSourceHerelinkAirUnit   = QT_TRANSLATE_NOOP("VideoSettings", "Herelink AirUnit");
    static constexpr const char* videoSourceHerelinkHotspot   = QT_TRANSLATE_NOOP("VideoSettings", "Herelink Hotspot");

   signals:
    // Original signals
    void streamConfiguredChanged    (bool configured);

    // Secondary signals with "1" suffix
    void streamConfigured1Changed   (bool configured);

   private slots:
    // Original slots
    void _configChanged             (QVariant value);

    // Secondary slots with "1" suffix
    void _configChanged1            (QVariant value);

   private:
    // Original methods
    void _setDefaults               ();
    void _setForceVideoDecodeList   ();

    // Secondary methods with "1" suffix
    void _setDefaults1              ();

   private:
    // Original members
    bool _noVideo = false;

    // Secondary members with "1" suffix
    bool _noVideo1 = false;
};
