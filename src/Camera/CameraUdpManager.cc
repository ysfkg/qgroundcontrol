/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CameraUdpManager.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(CameraUdpManagerLog, "qgc.camera.udpmanager")

CameraUdpManager* CameraUdpManager::_instance = nullptr;

CameraUdpManager* CameraUdpManager::instance()
{
    if (!_instance) {
        _instance = new CameraUdpManager();
    }
    return _instance;
}

CameraUdpManager::CameraUdpManager(QObject* parent)
    : QObject(parent)
    , _socket(new QUdpSocket(this))
    , _cameraAddress("192.168.144.108")
    , _cameraPort(5000)
    , _sendTimer(new QTimer(this))
    , _isSending(false)
{
    // Timer'ı ayarla - 20ms'de bir kontrol et (50Hz max)
    _sendTimer->setInterval(SEND_INTERVAL_MS);
    connect(_sendTimer, &QTimer::timeout, this, &CameraUdpManager::_processQueue);
    _sendTimer->start();

    qCDebug(CameraUdpManagerLog) << "===== CameraUdpManager initialized =====";
    qCDebug(CameraUdpManagerLog) << "  Camera:" << _cameraAddress.toString() << ":" << _cameraPort;
    qCDebug(CameraUdpManagerLog) << "  Send interval:" << SEND_INTERVAL_MS << "ms";
    qCDebug(CameraUdpManagerLog) << "  Socket state:" << _socket->state();
}

CameraUdpManager::~CameraUdpManager()
{
    _sendTimer->stop();
    _commandQueue.clear();
    _priorityQueue.clear();
}

void CameraUdpManager::setCameraAddress(const QHostAddress& address, quint16 port)
{
    _cameraAddress = address;
    _cameraPort = port;
    qCDebug(CameraUdpManagerLog) << "Camera address updated:" << address.toString() << ":" << port;
}

void CameraUdpManager::sendCommand(const QByteArray& command, bool priority)
{
    if (command.isEmpty()) {
        qCWarning(CameraUdpManagerLog) << "Empty command received!";
        return;
    }

    QMutexLocker locker(&_queueMutex);
    
    if (priority) {
        _priorityQueue.enqueue(command);
        qCDebug(CameraUdpManagerLog) << "Priority command queued:" << command.left(50) << "Queue size:" << _priorityQueue.size();
    } else {
        // Normal kuyruk çok büyürse eski komutları at (gimbal komutları için)
        if (_commandQueue.size() > 5) {
            _commandQueue.dequeue();  // En eskiyi at
        }
        _commandQueue.enqueue(command);
        qCDebug(CameraUdpManagerLog) << "Normal command queued:" << command.left(50) << "Queue size:" << _commandQueue.size();
    }
}

void CameraUdpManager::_processQueue()
{
    if (_isSending) {
        return;  // Hala önceki komut gönderiliyor
    }

    QMutexLocker locker(&_queueMutex);

    // Önce priority kuyruğuna bak
    if (!_priorityQueue.isEmpty()) {
        QByteArray command = _priorityQueue.dequeue();
        locker.unlock();
        _sendNextCommand();
        _isSending = true;
        
        qint64 written = _socket->writeDatagram(command, _cameraAddress, _cameraPort);
        if (written == -1) {
            qCWarning(CameraUdpManagerLog) << "Failed to send priority command:" << _socket->errorString();
        } else {
            qCDebug(CameraUdpManagerLog) << "Priority command sent:" << written << "bytes to" << _cameraAddress.toString() << ":" << _cameraPort;
            qCDebug(CameraUdpManagerLog) << "  Command:" << command.left(60);
        }
        
        _isSending = false;
        return;
    }

    // Normal kuyruktan gönder
    if (!_commandQueue.isEmpty()) {
        QByteArray command = _commandQueue.dequeue();
        locker.unlock();
        _sendNextCommand();
        _isSending = true;
        
        qint64 written = _socket->writeDatagram(command, _cameraAddress, _cameraPort);
        if (written == -1) {
            qCWarning(CameraUdpManagerLog) << "Failed to send command:" << _socket->errorString();
        }
        // Normal gimbal komutları için log çok fazla olmasın
        
        _isSending = false;
    }
}

void CameraUdpManager::_sendNextCommand()
{
    // Bu fonksiyon gelecekte daha karmaşık gönderim mantığı için kullanılabilir
    // Şimdilik boş
}

