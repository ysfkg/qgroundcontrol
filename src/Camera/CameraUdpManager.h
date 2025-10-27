/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QQueue>
#include <QTimer>
#include <QMutex>

/**
 * @brief Kamera UDP komutlarını merkezi olarak yöneten singleton sınıf
 * 
 * Bu sınıf tüm UDP komutlarını kuyruklar ve sırayla gönderir.
 * Böylece komutlar çakışmaz ve her komut düzgün şekilde gönderilir.
 */
class CameraUdpManager : public QObject
{
    Q_OBJECT

public:
    static CameraUdpManager* instance();

    /**
     * @brief Kameraya UDP komutu gönder (kuyruklu sistem)
     * @param command Gönderilecek komut
     * @param priority true ise öncelikli kuyruğa eklenir
     */
    void sendCommand(const QByteArray& command, bool priority = false);

    /**
     * @brief Kamera IP ve port ayarla
     */
    void setCameraAddress(const QHostAddress& address, quint16 port);

private:
    explicit CameraUdpManager(QObject* parent = nullptr);
    ~CameraUdpManager();

    static CameraUdpManager* _instance;

    void _processQueue();
    void _sendNextCommand();

    QUdpSocket*     _socket;
    QHostAddress    _cameraAddress;
    quint16         _cameraPort;
    
    QQueue<QByteArray>  _commandQueue;          // Normal kuyruk
    QQueue<QByteArray>  _priorityQueue;         // Öncelikli kuyruk (foto/video komutları)
    QMutex              _queueMutex;
    
    QTimer*         _sendTimer;
    bool            _isSending;
    
    static constexpr int SEND_INTERVAL_MS = 20;  // 50Hz'den fazla göndermeyi engelle
};

