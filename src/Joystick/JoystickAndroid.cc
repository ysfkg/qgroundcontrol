/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "JoystickAndroid.h"
#include "JoystickManager.h"
#include "AndroidInterface.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QElapsedTimer>
#include <QtNetwork/QUdpSocket>
#include <QHostAddress>
#include <QMap>
#include <cmath>

QGC_LOGGING_CATEGORY(JoystickAndroidLog, "qgc.joystick.joystickandroid")

QList<int> JoystickAndroid::_androidBtnList(_androidBtnListCount);
int JoystickAndroid::ACTION_DOWN = 0;
int JoystickAndroid::ACTION_UP = 0;
int JoystickAndroid::AXIS_HAT_X = 0;
int JoystickAndroid::AXIS_HAT_Y = 0;
QMutex JoystickAndroid::_mutex;

namespace {
QString calculateCRC(const QString &cmd)
{
    quint32 sum = 0;
    QByteArray bytes = cmd.toUtf8();
    for (int i = 0; i < bytes.size(); i++) {
        sum += static_cast<unsigned char>(bytes.at(i));
    }
    quint8 crc = sum & 0xFF;
    return QString("%1").arg(crc, 2, 16, QLatin1Char('0')).toUpper();
}

QString buildAngleCommand(const QString &axis, double angle, double speed)
{
    QMap<QString, QString> axisMap{
        {"yaw",   "GAY"},
        {"pitch", "GAP"},
        {"roll",  "GAR"}
    };

    const QString axisLower = axis.toLower();
    if (!axisMap.contains(axisLower)) {
        qWarning() << "Geçersiz eksen:" << axis;
        return QString();
    }
    const QString idBit = axisMap.value(axisLower);

    int angleVal = static_cast<int>(angle * 100);
    if (angleVal < 0) {
        angleVal = (1 << 16) + angleVal;
    }
    const QString angleHex = QString("%1").arg(angleVal, 4, 16, QLatin1Char('0')).toUpper();

    int speedVal = static_cast<int>(speed * 10.0);
    if (speedVal < 0) speedVal = 0;
    if (speedVal > 255) speedVal = 255;
    const QString speedHex = QString("%1").arg(speedVal, 2, 16, QLatin1Char('0')).toUpper();

    const QString payload = "#TPUG6w" + idBit + angleHex + speedHex;
    const QString crc = calculateCRC(payload);
    return payload + crc + "\r\n";
}

static QUdpSocket *s_udpSocket = nullptr;
static QElapsedTimer s_udpTimer;
static int s_lastChannel14Value = 0;
}

JoystickAndroid::JoystickAndroid(const QString &name, int axisCount, int buttonCount, int id, QObject *parent)
    : Joystick(name, axisCount, buttonCount, 0, parent)
    , deviceId(id)
{
    btnCode.resize(_buttonCount);
    axisCode.resize(_axisCount);
    btnValue.resize(_buttonCount);
    axisValue.resize(_axisCount);

    QJniEnvironment env;
    const jintArray btnArr = env->NewIntArray(_androidBtnListCount);
    env->SetIntArrayRegion(btnArr, 0, _androidBtnListCount, _androidBtnList.constData());
    const QJniObject inputDevice = QJniObject::callStaticObjectMethod("android/view/InputDevice", "getDevice", "(I)Landroid/view/InputDevice;", id);
    const QJniObject btns = inputDevice.callObjectMethod("hasKeys", "([I)[Z", btnArr);
    const jbooleanArray jSupportedButtons = btns.object<jbooleanArray>();
    jboolean *const supportedButtons = env->GetBooleanArrayElements(jSupportedButtons, nullptr);
    int c = 0;
    for (int i = 0; i < _androidBtnListCount; i++) {
        if (supportedButtons[i]) {
            btnValue[c] = false;
            btnCode[c] = _androidBtnList[i];
            c++;
        }
    }

    env->ReleaseBooleanArrayElements(jSupportedButtons, supportedButtons, 0);

    const QJniObject rangeListNative = inputDevice.callObjectMethod("getMotionRanges", "()Ljava/util/List;");
    for (int i = 0; i < _axisCount; i++) {
        const QJniObject range = rangeListNative.callObjectMethod("get", "(I)Ljava/lang/Object;", i);
        axisCode[i] = range.callMethod<jint>("getAxis");
        for (int j = 0; j < i; j++) {
            if (axisCode[i] == axisCode[j]) {
                axisCode[i] = -1;
                break;
            }
        }
        axisValue[i] = 0;
    }
    qCDebug(JoystickAndroidLog) << "axis:" << _axisCount << "buttons:" << _buttonCount;

    QtAndroidPrivate::registerGenericMotionEventListener(this);
    QtAndroidPrivate::registerKeyEventListener(this);
}

JoystickAndroid::~JoystickAndroid()
{
    QtAndroidPrivate::unregisterGenericMotionEventListener(this);
    QtAndroidPrivate::unregisterKeyEventListener(this);
}

QMap<QString, Joystick*> JoystickAndroid::discover()
{
    static QMap<QString, Joystick*> ret;

    QMutexLocker lock(&_mutex);
    
    qDebug() << "=== JoystickAndroid::discover() çağrıldı ===";

    const QJniObject object = QJniObject::callStaticObjectMethod<jintArray>("android/view/InputDevice", "getDeviceIds");
    jintArray jarr = object.object<jintArray>();

    QJniEnvironment env;
    const int len = env->GetArrayLength(jarr);
    qDebug() << "=== Bulunan cihaz sayısı:" << len << "===";
    jint *const buff = env->GetIntArrayElements(jarr, nullptr);

    const int SOURCE_GAMEPAD = QJniObject::getStaticField<jint>("android/view/InputDevice", "SOURCE_GAMEPAD");
    const int SOURCE_JOYSTICK = QJniObject::getStaticField<jint>("android/view/InputDevice", "SOURCE_JOYSTICK");

    QList<QString> names;
    for (int i = 0; i < len; ++i) {
        const QJniObject inputDevice = QJniObject::callStaticObjectMethod("android/view/InputDevice", "getDevice", "(I)Landroid/view/InputDevice;", buff[i]);
        const int sources = inputDevice.callMethod<jint>("getSources", "()I");
        const QString deviceName = inputDevice.callObjectMethod("getName", "()Ljava/lang/String;").toString();
        qDebug() << "=== Cihaz" << i << ":" << deviceName << "sources:" << sources << "===";
        if (((sources & SOURCE_GAMEPAD) != SOURCE_GAMEPAD) && ((sources & SOURCE_JOYSTICK) != SOURCE_JOYSTICK)) {
            qDebug() << "=== Cihaz joystick/gamepad değil, atlanıyor ===";
            continue;
        }

        const QString id = inputDevice.callObjectMethod("getDescriptor", "()Ljava/lang/String;").toString();
        const QString name = inputDevice.callObjectMethod("getName", "()Ljava/lang/String;").toString();

        names.push_back(name);

        if (ret.contains(name)) {
            continue;
        }

        const QJniObject rangeListNative = inputDevice.callObjectMethod("getMotionRanges", "()Ljava/util/List;");
        const int axisCount = rangeListNative.callMethod<jint>("size");

        jintArray arr = env->NewIntArray(_androidBtnListCount);
        env->SetIntArrayRegion(arr, 0, _androidBtnListCount, _androidBtnList.constData());
        const QJniObject btns = inputDevice.callObjectMethod("hasKeys", "([I)[Z", arr);
        const jbooleanArray jSupportedButtons = btns.object<jbooleanArray>();
        jboolean *const supportedButtons = env->GetBooleanArrayElements(jSupportedButtons, nullptr);
        int buttonCount = 0;
        for (int j = 0; j < _androidBtnListCount; j++) {
            if (supportedButtons[j]) {
                buttonCount++;
            }
        }
        env->ReleaseBooleanArrayElements(jSupportedButtons, supportedButtons, 0);

        qCDebug(JoystickAndroidLog) << name << "id:" << buff[i] << "axes:" << axisCount << "buttons:" << buttonCount;
        qDebug() << "=== Joystick oluşturuluyor:" << name << "axes:" << axisCount << "buttons:" << buttonCount << "===";

        ret[name] = new JoystickAndroid(name, axisCount, buttonCount, buff[i]);
    }

    for (auto i = ret.begin(); i != ret.end();) {
        if (!names.contains(i.key())) {
            i = ret.erase(i);
        } else {
            i++;
        }
    }

    env->ReleaseIntArrayElements(jarr, buff, 0);

    qDebug() << "=== JoystickAndroid::discover() tamamlandı, bulunan joystick sayısı:" << ret.size() << "===";
    return ret;
}

bool JoystickAndroid::handleKeyEvent(jobject event)
{
    QMutexLocker lock(&_mutex);

    QJniObject ev(event);
    const int _deviceId = ev.callMethod<jint>("getDeviceId", "()I");
    if (_deviceId != deviceId) {
        return false;
    }

    const int action = ev.callMethod<jint>("getAction", "()I");
    const int keyCode = ev.callMethod<jint>("getKeyCode", "()I");

    for (int i = 0; i < _buttonCount; i++) {
        if (btnCode[i] != keyCode) {
            continue;
        }

        if (action == ACTION_DOWN) {
            btnValue[i] = true;
        } else if (action == ACTION_UP) {
            btnValue[i] = false;
        }

        return true;
    }

    return false;
}

bool JoystickAndroid::handleGenericMotionEvent(jobject event)
{
    QMutexLocker lock(&_mutex);

    QJniObject ev(event);
    const int _deviceId = ev.callMethod<jint>("getDeviceId", "()I");
    qDebug() << "=== handleGenericMotionEvent çağrıldı, deviceId:" << _deviceId << "beklenen:" << deviceId << "===";
    if (_deviceId != deviceId) {
        qDebug() << "=== Device ID uyuşmuyor, event reddediliyor ===";
        return false;
    }

    for (int i = 0; i < _axisCount; i++) {
        const float v = ev.callMethod<jfloat>("getAxisValue", "(I)F", axisCode[i]);
        axisValue[i] = static_cast<int>(v * 32767.f);
    }

    qCritical() << "===******************************************************************** Android Joystick Axis 14 Debug ===********************************************************************";
    // 15. kanal (index 14) değerini -90..90 dereceye çevirip UDP ile gönder
    if (_axisCount > 14) {
        const int raw = axisValue[14]; // -32767..32767

        // Timer ilk kullanımda başlat
        if (!s_udpTimer.isValid()) {
            s_udpTimer.start();
        }
        
        // Sadece kanal değeri değiştiğinde ve timer süresi dolduğunda gönder
        const bool channelChanged = (std::fabs(raw - s_lastChannel14Value) >= 10);
        const bool timerReady = (s_udpTimer.elapsed() >= 40); // 25 Hz için 40ms
        
        if (channelChanged && timerReady) {
            s_lastChannel14Value = raw;
            s_udpTimer.start();
            
            double adjusted = static_cast<double>(raw) / 32767.0; // -1..1
            // Deadzone
            if (std::fabs(adjusted) < 0.01) adjusted = 0.0;
            double angleDeg = adjusted * 90.0; // -90..90
            if (angleDeg < -90.0) angleDeg = -90.0;
            if (angleDeg > 90.0)  angleDeg =  90.0;

            // Hız: 1..10 arası örnek bir ölçekleme
            const double speed = 1.0 + (std::fabs(adjusted) * 9.0);

            const QString command = buildAngleCommand("pitch", angleDeg, speed);
            if (!command.isEmpty()) {
                if (!s_udpSocket) s_udpSocket = new QUdpSocket();
                const QHostAddress targetAddress(QStringLiteral("192.168.144.108"));
                const quint16 targetPort = 5000;
                (void) s_udpSocket->writeDatagram(command.toUtf8(), targetAddress, targetPort);
                
                qDebug() << "UDP Komut Gönderildi - Kanal 15:" << raw << "Açı:" << angleDeg << "Hız:" << speed;
            }
        }
    }

    return true;
}



int  JoystickAndroid::_getAndroidHatAxis(int axisHatCode) const
{
    for (int i = 0; i < _axisCount; i++) {
        if (axisCode[i] == axisHatCode) {
            return _getAxis(i);
        }
    }

    return 0;
}

bool JoystickAndroid::_getHat(int hat, int i) const
{
    // Android supports only one hat button
    if (hat != 0) {
        return false;
    }

    switch (i) {
    case 0:
        return (_getAndroidHatAxis(AXIS_HAT_Y) < 0);
    case 1:
        return (_getAndroidHatAxis(AXIS_HAT_Y) > 0);
    case 2:
        return (_getAndroidHatAxis(AXIS_HAT_X) < 0);
    case 3:
        return (_getAndroidHatAxis(AXIS_HAT_X) > 0);
    default:
        return false;
    }
}

bool JoystickAndroid::init()
{
    QList<int> ret(_androidBtnListCount);

    (void) AndroidInterface::cleanJavaException();

    int i;
    for (i = 1; i <= 16; i++) {
        const QString name = QStringLiteral("KEYCODE_BUTTON_") + QString::number(i);
        ret[i - 1] = QJniObject::getStaticField<jint>("android/view/KeyEvent", name.toStdString().c_str());
    }
    i--;

    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_A");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_B");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_C");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_L1");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_L2");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_R1");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_R2");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_MODE");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_SELECT");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_START");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_THUMBL");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_THUMBR");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_X");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_Y");
    ret[i++] = QJniObject::getStaticField<jint>("android/view/KeyEvent", "KEYCODE_BUTTON_Z");

    ACTION_DOWN = QJniObject::getStaticField<jint>("android/view/KeyEvent", "ACTION_DOWN");
    ACTION_UP = QJniObject::getStaticField<jint>("android/view/KeyEvent", "ACTION_UP");
    AXIS_HAT_X = QJniObject::getStaticField<jint>("android/view/MotionEvent", "AXIS_HAT_X");
    AXIS_HAT_Y = QJniObject::getStaticField<jint>("android/view/MotionEvent", "AXIS_HAT_Y");

    _androidBtnList = ret;

    return true;
}

static void jniUpdateAvailableJoysticks(JNIEnv *envA, jobject thizA)
{
    Q_UNUSED(envA); Q_UNUSED(thizA);

    qCDebug(JoystickAndroidLog) << "jniUpdateAvailableJoysticks triggered";

    emit JoystickManager::instance()->updateAvailableJoysticksSignal();
}

void JoystickAndroid::setNativeMethods()
{
    qCDebug(JoystickAndroidLog) << "Registering Native Functions";

    static const JNINativeMethod javaMethods[] {
        {"nativeUpdateAvailableJoysticks", "()V", reinterpret_cast<void*>(jniUpdateAvailableJoysticks)}
    };

    static constexpr const char *kJniClassName = "org/mavlink/qgroundcontrol/QGCUsbSerialManager";

    (void) AndroidInterface::cleanJavaException();

    QJniEnvironment jniEnv;
    jclass objectClass = jniEnv->FindClass(kJniClassName);
    if (!objectClass) {
        (void) AndroidInterface::cleanJavaException();
        qCWarning(JoystickAndroidLog) << "Couldn't find class:" << kJniClassName;
        return;
    }

    const jint val = jniEnv->RegisterNatives(objectClass, javaMethods, std::size(javaMethods));
    if (val < 0) {
        qCWarning(JoystickAndroidLog) << "Error registering methods:" << val;
    } else {
        qCDebug(JoystickAndroidLog) << "Native Functions Registered";
    }

    (void) AndroidInterface::cleanJavaException();
}
