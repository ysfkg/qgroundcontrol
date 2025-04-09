/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QRunnable>
#include <QtCore/QSize>
#include <QtQmlIntegration/QtQmlIntegration>

Q_DECLARE_LOGGING_CATEGORY(VideoManagerLog)

#define MAX_VIDEO_RECEIVERS 2

class FinishVideoInitialization;
class SubtitleWriter;
class Vehicle;
class VideoReceiver;
class VideoSettings;

class VideoManager : public QObject
{
    Q_OBJECT
    // QML_ELEMENT
    // QML_UNCREATABLE("")
    Q_MOC_INCLUDE("Vehicle.h")
    Q_PROPERTY(bool     gstreamerEnabled        READ gstreamerEnabled                           CONSTANT)
    Q_PROPERTY(bool     qtmultimediaEnabled     READ qtmultimediaEnabled                        CONSTANT)
    Q_PROPERTY(bool     uvcEnabled              READ uvcEnabled                                 CONSTANT)
    Q_PROPERTY(bool     uvcEnabled1              READ uvcEnabled1                                 CONSTANT)
    Q_PROPERTY(bool     autoStreamConfigured    READ autoStreamConfigured                       NOTIFY autoStreamConfiguredChanged)
    Q_PROPERTY(bool     decoding                READ decoding                                   NOTIFY decodingChanged)
    Q_PROPERTY(bool     decoding1                READ decoding1                                   NOTIFY decodingChanged1)
    Q_PROPERTY(bool     fullScreen              READ fullScreen             WRITE setfullScreen NOTIFY fullScreenChanged)
    Q_PROPERTY(bool     fullScreen1              READ fullScreen1             WRITE setfullScreen1 NOTIFY fullScreenChanged1)
    Q_PROPERTY(bool     hasThermal              READ hasThermal                                 NOTIFY decodingChanged)
    Q_PROPERTY(bool     hasThermal1              READ hasThermal1                                 NOTIFY decodingChanged1)
    Q_PROPERTY(bool     hasVideo                READ hasVideo                                   NOTIFY hasVideoChanged)
    Q_PROPERTY(bool     hasVideo1                READ hasVideo1                                   NOTIFY hasVideoChanged1)
    Q_PROPERTY(bool     isStreamSource          READ isStreamSource                             NOTIFY isStreamSourceChanged)
    Q_PROPERTY(bool     isStreamSource1          READ isStreamSource1                             NOTIFY isStreamSourceChanged1)
    Q_PROPERTY(bool     isUvc                   READ isUvc                                      NOTIFY isUvcChanged)
    Q_PROPERTY(bool     isUvc1                   READ isUvc1                                      NOTIFY isUvcChanged1)
    Q_PROPERTY(bool     recording               READ recording                                  NOTIFY recordingChanged)
    Q_PROPERTY(bool     recording1               READ recording1                                  NOTIFY recordingChanged1)
    Q_PROPERTY(bool     streaming               READ streaming                                  NOTIFY streamingChanged)
    Q_PROPERTY(bool     streaming1               READ streaming1                                  NOTIFY streamingChanged1)
    Q_PROPERTY(double   aspectRatio             READ aspectRatio                                NOTIFY aspectRatioChanged)
    Q_PROPERTY(double   hfov                    READ hfov                                       NOTIFY aspectRatioChanged)
    Q_PROPERTY(double   thermalAspectRatio      READ thermalAspectRatio                         NOTIFY aspectRatioChanged)
    Q_PROPERTY(double   thermalHfov             READ thermalHfov                                NOTIFY aspectRatioChanged)
    Q_PROPERTY(double   aspectRatio1             READ aspectRatio1                                NOTIFY aspectRatioChanged1)
    Q_PROPERTY(double   hfov1                    READ hfov1                                       NOTIFY aspectRatioChanged1)
    Q_PROPERTY(double   thermalAspectRatio1      READ thermalAspectRatio1                         NOTIFY aspectRatioChanged1)
    Q_PROPERTY(double   thermalHfov1             READ thermalHfov1                                NOTIFY aspectRatioChanged1)
    Q_PROPERTY(QSize    videoSize               READ videoSize                                  NOTIFY videoSizeChanged)
    Q_PROPERTY(QSize    videoSize1               READ videoSize1                                  NOTIFY videoSizeChanged1)
    Q_PROPERTY(QString  imageFile               READ imageFile                                  NOTIFY imageFileChanged)
    Q_PROPERTY(QString  imageFile1               READ imageFile1                                  NOTIFY imageFileChanged1)
    Q_PROPERTY(QString  uvcVideoSourceID        READ uvcVideoSourceID                           NOTIFY uvcVideoSourceIDChanged)
    Q_PROPERTY(QString  uvcVideoSourceID1        READ uvcVideoSourceID1                           NOTIFY uvcVideoSourceIDChanged1)

    friend class FinishVideoInitialization;

public:
    explicit VideoManager(QObject *parent = nullptr);
    ~VideoManager();

    /// Gets the singleton instance of VideoManager.
    ///     @return The singleton instance.
    static VideoManager *instance();
    static void registerQmlTypes();

    Q_INVOKABLE void grabImage(const QString &imageFile = QString());
    Q_INVOKABLE void grabImage1(const QString &imageFile = QString());
    Q_INVOKABLE void startRecording(const QString &videoFile = QString());
    Q_INVOKABLE void startRecording1(const QString &videoFile = QString());
    Q_INVOKABLE void startVideo();
    Q_INVOKABLE void startVideo1();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void stopRecording1();
    Q_INVOKABLE void stopVideo();
    Q_INVOKABLE void stopVideo1();

    void init();
    void init1();
    void cleanup();
    void cleanup1();
    bool autoStreamConfigured() const;
    bool decoding() const { return _decoding; }
    bool decoding1() const { return _decoding1; }
    bool fullScreen() const { return _fullScreen; }
    bool fullScreen1() const { return _fullScreen1; }
    bool gstreamerEnabled() const;
    bool hasThermal() const;
    bool hasThermal1() const;
    bool hasVideo() const;
    bool hasVideo1() const;
    bool isStreamSource() const;
    bool isStreamSource1() const;
    bool isUvc() const;
    bool isUvc1() const;
    bool qtmultimediaEnabled() const;
    bool recording() const { return _recording; }
    bool recording1() const { return _recording1; }
    bool streaming() const { return _streaming; }
    bool streaming1() const { return _streaming1; }
    bool uvcEnabled() const;
    bool uvcEnabled1() const;
    double aspectRatio() const;
    double aspectRatio1() const;
    double hfov() const;
    double hfov1() const;
    double thermalAspectRatio() const;
    double thermalHfov() const;
    double thermalAspectRatio1() const;
    double thermalHfov1() const;
    QSize videoSize() const { return QSize((_videoSize >> 16) & 0xFFFF, _videoSize & 0xFFFF); }
    QSize videoSize1() const { return QSize((_videoSize1 >> 16) & 0xFFFF, _videoSize1 & 0xFFFF); }
    QString imageFile() const { return _imageFile; }
    QString imageFile1() const { return _imageFile1; }
    QString uvcVideoSourceID() const { return _uvcVideoSourceID; }
    QString uvcVideoSourceID1() const { return _uvcVideoSourceID1; }
    void setfullScreen(bool on);
    void setfullScreen1(bool on);

signals:
    void aspectRatioChanged();
    void aspectRatioChanged1();
    void autoStreamConfiguredChanged();
    void decodingChanged();
    void decodingChanged1();
    void fullScreenChanged();
    void fullScreenChanged1();
    void hasVideoChanged();
    void hasVideoChanged1();
    void imageFileChanged();
    void imageFileChanged1();
    void isAutoStreamChanged();
    void isAutoStreamChanged1();
    void isStreamSourceChanged();
    void isStreamSourceChanged1();
    void isUvcChanged();
    void isUvcChanged1();
    void recordingChanged();
    void recordingChanged1();
    void recordingStarted();
    void streamingChanged();
    void streamingChanged1();
    void uvcVideoSourceIDChanged();
    void uvcVideoSourceIDChanged1();
    void videoSizeChanged();
    void videoSizeChanged1();

private slots:
    bool _updateUVC();
    bool _updateUVC1();
    void _communicationLostChanged(bool communicationLost);
    void _communicationLostChanged1(bool communicationLost);
    void _lowLatencyModeChanged() { _restartAllVideos(); }
    void _lowLatencyModeChanged1() { _restartAllVideos1(); }
    void _setActiveVehicle(Vehicle *vehicle);
    void _videoSourceChanged();
    void _videoSourceChanged1();

private:
    bool _updateAutoStream(unsigned id);
    bool _updateSettings(unsigned id);
    bool _updateSettings1(unsigned id);
    bool _updateVideoUri(unsigned id, const QString &uri);
    bool _updateVideoUri1(unsigned id, const QString &uri);
    void _initVideo();
    void _initVideo1();
    void _restartAllVideos();
    void _restartAllVideos1();
    void _restartVideo(unsigned id);
    void _restartVideo1(unsigned id);
    void _startReceiver(unsigned id);
    void _startReceiver1(unsigned id);
    void _stopReceiver(unsigned id);
    void _stopReceiver1(unsigned id);
    static void _cleanupOldVideos();
    static void _cleanupOldVideos1();

    struct VideoReceiverData {
        VideoReceiver *receiver = nullptr;
        void *sink = nullptr;
        QString uri;
        bool started = false;
        bool lowLatencyStreaming = false;
        size_t index = 0;
        QString name;
    };
    struct VideoReceiverData1 {
        VideoReceiver *receiver = nullptr;
        void *sink = nullptr;
        QString uri;
        bool started = false;
        bool lowLatencyStreaming = false;
        size_t index = 0;
        QString name;
    };
    QList<VideoReceiverData> _videoReceiverData = QList<VideoReceiverData>(MAX_VIDEO_RECEIVERS);
    QList<VideoReceiverData1> _videoReceiverData1 = QList<VideoReceiverData1>(MAX_VIDEO_RECEIVERS);


    SubtitleWriter *_subtitleWriter = nullptr;

    bool _initialized = false;
    bool _initialized1 = false;
    bool _fullScreen = false;
    bool _fullScreen1 = false;

    QAtomicInteger<bool> _decoding = false;
    QAtomicInteger<bool> _decoding1 = false;
    QAtomicInteger<bool> _recording = false;
    QAtomicInteger<bool> _recording1 = false;
    QAtomicInteger<bool> _streaming = false;
    QAtomicInteger<bool> _streaming1 = false;
    QAtomicInteger<quint32> _videoSize = 0;
    QAtomicInteger<quint32> _videoSize1 = 0;
    QString _imageFile;
    QString _imageFile1;
    QString _uvcVideoSourceID;
    QString _uvcVideoSourceID1;
    QString _videoFile;
    Vehicle *_activeVehicle = nullptr;
    VideoSettings *_videoSettings = nullptr;
};

/*===========================================================================*/

class FinishVideoInitialization : public QRunnable
{
public:
    explicit FinishVideoInitialization();
    ~FinishVideoInitialization();

    void run() final;
};
