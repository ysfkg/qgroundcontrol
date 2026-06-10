# QGroundControl (QGC) — Proje Bağlam Dokümantasyonu

> Bu dosya, yeni sohbetlerde veya yeni geliştiriciler için QGC kod tabanının anında kavranmasını sağlar.
> Workspace: `/home/yusuf/qgroundcontrol`

---

## 1. Yüksek Seviye Mimari Özet

### 1.1 Genel Yapı

QGC, **Qt 6 + C++ backend + QML frontend** ile yazılmış bir GCS (Ground Control Station) uygulamasıdır. Mimari olarak **MVVM benzeri** bir desen kullanır:

| Katman | Teknoloji | Sorumluluk |
|--------|-----------|------------|
| **Model** | C++ sınıfları (`Vehicle`, `Fact`, `FactGroup`, `MissionItem`) | MAVLink verisi, durum, iş mantığı |
| **ViewModel** | C++ controller'lar + `Q_PROPERTY` / `Q_INVOKABLE` | QML'e expose edilen API |
| **View** | QML (`FlyView.qml`, `PlanView.qml`, vb.) | Kullanıcı arayüzü |

Tüm uygulama kodu tek bir statik kütüphanede toplanır:

```
CMakeLists.txt (kök)
  └── qt_add_executable(QGroundControl, src/main.cc, ...)
  └── add_subdirectory(src)
        └── qt_add_library(QGC STATIC ...)   ← tüm modüller buraya linklenir
```

Giriş noktası: `src/main.cc` → `QGCApplication`

### 1.2 C++ ↔ QML Veri Akışı

```
Fiziksel Bağlantı (Serial/UDP/TCP/Bluetooth)
    │  bytesReceived sinyali
    ▼
LinkManager  (src/Comms/LinkManager.cc)
    │  connect → MAVLinkProtocol::receiveBytes
    ▼
MAVLinkProtocol  (src/Comms/MAVLinkProtocol.cc)
    │  mavlink_parse_char() → messageReceived sinyali (broadcast)
    ▼
MultiVehicleManager  (HEARTBEAT → new Vehicle)
    │
    ▼
Vehicle::_mavlinkMessageReceived()  (src/Vehicle/Vehicle.cc)
    ├── FirmwarePlugin::adjustIncomingMavlinkMessage
    ├── ParameterManager, FTPManager, TerrainProtocolHandler, ...
    ├── FactGroup::handleMessage()  →  Fact::setRawValue()
    └── Vehicle::handleMessage() + switch(msgid) özel handler'lar
            │
            ▼
    Fact::valueChanged sinyali  →  QML property binding
            │
            ▼
QML:  QGroundControl.multiVehicleManager.activeVehicle.heading.value
```

### 1.3 QML Erişim Mekanizmaları

| Mekanizma | Kullanım | Örnek |
|-----------|----------|-------|
| **QML Singleton** | Global yöneticilere erişim | `QGroundControl.linkManager` |
| **Q_PROPERTY** | C++ nesnesinin QML'e expose edilmesi | `vehicle.armed`, `vehicle.gps.lat.value` |
| **qmlRegisterType** | QML'den instantiate edilebilir controller | `ParameterEditorController` |
| **qmlRegisterUncreatableType** | Sadece referans/sinyal için | `Vehicle`, `LinkManager` |
| **QQmlContext::setContextProperty** | Engine seviyesinde global inject | `joystickManager`, `logDownloadController` |
| **Signal/Slot** | Olay tabanlı iletişim | `vehicle.coordinateChanged` |

QML type kayıtları merkezi olarak `QGCApplication::init()` içinde yapılır (`src/QGCApplication.cc`, satır ~271).

QML engine kurulumu: `QGCCorePlugin::createQmlApplicationEngine()` (`src/API/QGCCorePlugin.cc`)
- Import path: `qrc:/qml`
- Root window: `qrc:/qml/MainRootWindow.qml` → `src/UI/MainRootWindow.qml`

### 1.4 Fact Sistemi (Telemetri Omurgası)

```
QObject
├── Fact                    src/FactSystem/Fact.h
│   └── Q_PROPERTY(QVariant value READ cookedValue NOTIFY valueChanged)
└── FactGroup               src/FactSystem/FactGroup.h
    ├── VehicleFactGroup    src/Vehicle/FactGroups/VehicleFactGroup.h
    │   └── Vehicle         src/Vehicle/Vehicle.h
    └── VehicleGPSFactGroup, VehicleWindFactGroup, ... (nested)
```

- `Fact::setRawValue()` → `valueChanged` sinyali (anında veya `_updateRateMSecs` ile ertelenmiş)
- `FactGroup::handleMessage(Vehicle*, mavlink_message_t&)` — MAVLink mesajını Fact değerlerine çevirir
- Metadata: JSON dosyaları (`:/json/Vehicle/GPSFact.json` vb.) veya inline `FactMetaData`

### 1.5 Firmware / Araç Soyutlama

QGC'de **Vehicle alt sınıfları yoktur**. Firmware ve araç tipi farkları plugin sistemiyle çözülür:

```
FirmwarePluginFactory (kayıt: FirmwarePluginFactoryRegister)
    ├── PX4FirmwarePluginFactory  →  PX4FirmwarePlugin
    └── APMFirmwarePluginFactory  →  ArduCopter/Plane/Rover/Sub FirmwarePlugin

FirmwarePlugin::autopilotPlugin(Vehicle*)  →  AutoPilotPlugin
    ├── PX4AutoPilotPlugin
    ├── APMAutoPilotPlugin
    └── GenericAutoPilotPlugin
```

---

## 2. Kritik Klasör Hiyerarşisi ve Görevleri

### 2.1 Kök Dizin

| Dizin | Görev |
|-------|-------|
| `src/` | Tüm C++ ve QML kaynak kodu |
| `cmake/` | Build modülleri, `CustomOptions.cmake`, install scriptleri |
| `libs/` | Vendored bağımlılıklar: `mavlink`, `eigen`, `libevents`, `qmdnsengine`, `shapelib`, `xz-embedded` |
| `resources/` | İkonlar, fontlar, görseller |
| `deploy/` | Platform paketleme (Windows NSIS, macOS, iOS, Android) |
| `android/` | Android native kodu ve serial port kütüphanesi |
| `test/` | Unit/integration testleri (`QGC_BUILD_TESTING` ile) |
| `translations/` | i18n `.ts` dosyaları |
| `custom-example/` | Özel markalı build şablonu (`QGC_CUSTOM_BUILD`) |
| `qgroundcontrol.qrc` | Ana QML ve JSON resource bundle |
| `qgcresources.qrc` | SVG, font, kalibrasyon görselleri |
| `qgcimages.qrc` | UI ikonları, airframe görselleri |

### 2.2 `src/` Alt Dizinleri (29 modül)

| Dizin | Sorumluluk |
|-------|------------|
| `src/ADSB/` | ADS-B trafik: `ADSBVehicle`, `ADSBVehicleManager`, TCP link |
| `src/AnalyzeView/` | Analiz araçları: log indirme, geo-tag, MAVLink konsol/inspector, titreşim; C++ controller + QML sayfalar |
| `src/API/` | Eklenti API'si: `QGCCorePlugin`, `QGCOptions`, QML engine/window oluşturma |
| `src/AutoPilotPlugins/` | Araç kurulum UI plugin'leri: `APM/`, `PX4/`, `Common/`, `Generic/`; `VehicleComponent` tabanlı setup sayfaları |
| `src/Camera/` | MAVLink kamera kontrolü: `QGCCameraManager`, `VehicleCameraControl`, `CameraUdpManager` |
| `src/Comms/` | İletişim: `LinkManager`, `MAVLinkProtocol`, UDP/TCP/Serial/Bluetooth linkler; `MockLink/`, `AirLink/` |
| `src/FactSystem/` | Ayarlar ve parametreler: `Fact`, `FactGroup`, `ParameterManager`; `FactControls/` QML binding bileşenleri |
| `src/FirmwarePlugin/` | Firmware soyutlama: `FirmwarePluginManager`, `APM/`, `PX4/` plugin'leri |
| `src/FlightDisplay/` | Fly view QML: HUD, video, checklist, guided actions (~50+ dosya); QRC ile paketlenir |
| `src/FlightMap/` | Harita QML: `FlightMap.qml`, `MapItems/`, `Widgets/`; compass, attitude widget'ları |
| `src/FollowMe/` | GCS follow-me konum raporlama: `FollowMe` singleton |
| `src/Gimbal/` | Gimbal MAVLink kontrolü: `GimbalController` |
| `src/GPS/` | RTK/GPS donanım: `GPSManager`, `GPSRtk`, `Drivers/` |
| `src/Joystick/` | Joystick girişi: `JoystickManager`, SDL/Android backend'leri |
| `src/MAVLink/` | MAVLink yardımcıları: `QGCMAVLink`, FTP, signing, `LibEvents/` |
| `src/MissionManager/` | Görev planlama: controller'lar, complex item'lar, geo-fence, rally point'ler |
| `src/PlanView/` | Plan view QML editörleri ve harita görselleri (~30 dosya) |
| `src/PositionManager/` | GCS konumu: `QGCPositionManager`, `SimulatedPosition` |
| `src/QmlControls/` | Paylaşılan QML kontrolleri, paletler, `QGroundControlQmlGlobal`, controller'lar |
| `src/QtLocationPlugin/` | Özel Qt Location plugin, tile cache, `QGCMapEngine`, `QGCMapEngineManager` |
| `src/Settings/` | Uygulama ayar grupları + `SettingsManager` singleton |
| `src/Terrain/` | Arazi yükseklik sorguları: `TerrainQuery`, `TerrainTileManager` |
| `src/UI/` | Ana pencere (`MainRootWindow.qml`), toolbar, preferences; tool drawer shell |
| `src/Utilities/` | Cross-cutting: `JsonHelper`, logging, `Audio/`, `Geo/`, `Compression/`, `FileSystem/` |
| `src/UTMSP/` | UTM servis entegrasyonu: `UTMSPManager` (opsiyonel, `QGC_UTM_ADAPTER`) |
| `src/Vehicle/` | Çekirdek araç modeli: `Vehicle`, `MultiVehicleManager`, alt yöneticiler, `FactGroups/`, `VehicleSetup/`, `Actuators/` |
| `src/VideoManager/` | Video streaming: `VideoManager`, GStreamer/Qt backend'leri (`VideoReceiver/`) |
| `src/Viewer3D/` | Opsiyonel 3D harita görüntüleyici (`Viewer3DManager`, Qt Quick 3D) |

**Not:** `FlightDisplay/`, `FlightMap/`, `PlanView/`, `UI/` CMakeLists.txt'de yorum satırıdır; kaynakları QRC üzerinden paketlenir, ayrı derlenmez.

### 2.3 QML Modül Yapısı

Fiziksel kaynak `src/QmlControls/QGroundControl/` altındaki `qmldir` dosyalarıyla modül olarak expose edilir:

```
QGroundControl.Controls       → src/QmlControls/QGCButton.qml, QGCTextField.qml, ...
QGroundControl.FlightDisplay  → src/FlightDisplay/*.qml (alias ile)
QGroundControl.FlightMap      → src/FlightMap/*.qml
QGroundControl.FactControls   → src/FactSystem/FactControls/*.qml
QGroundControl.ScreenTools    → ScreenTools.qml (singleton)
QGroundControl.Palette        → QGCPalette (C++ registered)
QGroundControl.PX4            → PX4 setup QML'leri
```

Import path: `qrc:/qml` (engine başlatılırken eklenir).

---

## 3. Çekirdek Singleton ve Yönetici Sınıfları

### 3.1 Erişim Kalıpları

| Kalıp | Kullanım |
|-------|----------|
| `ClassName::instance()` | C++ tarafından global singleton erişimi |
| `QGroundControl.<property>` | QML tarafından global erişim (`QGroundControlQmlGlobal`) |
| `qgcApp()` / `qApp` | `QGCApplication` cast (`src/QGCApplication.h`) |
| `vehicle.<manager>` | Per-vehicle yöneticilere QML/C++ erişimi |

Singleton'lar `Q_APPLICATION_STATIC(Class, varName)` makrosuyla tanımlanır (Qt uygulama ömrüne bağlı).

### 3.2 Global Singleton Tablosu

| Sınıf | Header | Görev | QML Erişimi |
|-------|--------|-------|-------------|
| `QGCApplication` | `src/QGCApplication.h` | Uygulama yaşam döngüsü, QML type kayıtları, boot sırası | `qgcApp()` |
| `SettingsManager` | `src/Settings/SettingsManager.h` | Tüm ayar gruplarına merkezi erişim | `QGroundControl.settingsManager` |
| `LinkManager` | `src/Comms/LinkManager.h` | Bağlantı oluşturma/yönetimi (serial, UDP, TCP, BT) | `QGroundControl.linkManager` |
| `MAVLinkProtocol` | `src/Comms/MAVLinkProtocol.h` | MAVLink parse, log, heartbeat dispatch | (doğrudan QML'de yok) |
| `MultiVehicleManager` | `src/Vehicle/MultiVehicleManager.h` | Araç oluşturma/silme, aktif araç seçimi | `QGroundControl.multiVehicleManager` |
| `FirmwarePluginManager` | `src/FirmwarePlugin/FirmwarePluginManager.h` | MAV_AUTOPILOT/MAV_TYPE → `FirmwarePlugin` | (C++ only) |
| `QGCCorePlugin` | `src/API/QGCCorePlugin.h` | Extensibility: analyze sayfaları, toolbar, palette, MAVLink hook | `QGroundControl.corePlugin` |
| `QGCPositionManager` | `src/PositionManager/PositionManager.h` | GCS GPS/konum | `QGroundControl.qgcPositionManger` |
| `VideoManager` | `src/VideoManager/VideoManager.h` | Video streaming/kayıt | `QGroundControl.videoManager` |
| `JoystickManager` | `src/Joystick/JoystickManager.h` | Joystick keşif ve mapping | context property `joystickManager` |
| `QGCMapEngineManager` | `src/QtLocationPlugin/QMLControl/QGCMapEngineManager.h` | Offline harita tile yönetimi | `QGroundControl.mapEngineManager` |
| `MissionCommandTree` | `src/MissionManager/MissionCommandTree.h` | MAVLink mission command metadata | `QGroundControl.missionCommandTree` |
| `ADSBVehicleManager` | `src/ADSB/ADSBVehicleManager.h` | ADS-B trafik araçları | `QGroundControl.adsbVehicleManager` |
| `GPSManager` | `src/GPS/GPSManager.h` | RTK GPS donanım | (C++ only) |
| `FollowMe` | `src/FollowMe/FollowMe.h` | Follow-me GCS konumu | (C++ only) |
| `AudioOutput` | `src/Utilities/Audio/AudioOutput.h` | TTS uyarıları | (C++ only) |

**Önemli:** `VehicleManager` adında bir sınıf **yoktur**. Araç filosu `MultiVehicleManager`, tekil araç durumu `Vehicle` ile yönetilir.

### 3.3 QML Köprü Singleton: `QGroundControlQmlGlobal`

- **Dosyalar:** `src/QmlControls/QGroundControlQmlGlobal.h`, `QGroundControlQmlGlobal.cc`
- **QML modülü:** `import QGroundControl` → singleton `QGroundControl`
- Tüm global manager'lara, palete, birim dönüşümüne erişim sağlar

```qml
import QGroundControl

property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
property var _settings:      QGroundControl.settingsManager.appSettings
```

### 3.4 Per-Vehicle Yöneticiler (Vehicle'a ait, global singleton değil)

`Vehicle::_commonInit()` içinde oluşturulur; QML'de `vehicle.<property>` ile erişilir:

| Sınıf | Header | QML Property |
|-------|--------|--------------|
| `MissionManager` | `src/MissionManager/MissionManager.h` | `vehicle.missionManager` |
| `GeoFenceManager` | `src/MissionManager/GeoFenceManager.h` | (Vehicle üzerinden) |
| `RallyPointManager` | `src/MissionManager/RallyPointManager.h` | (Vehicle üzerinden) |
| `ParameterManager` | `src/FactSystem/ParameterManager.h` | `vehicle.parameterManager` |
| `VehicleLinkManager` | `src/Vehicle/VehicleLinkManager.h` | `vehicle.vehicleLinkManager` |
| `FTPManager` | `src/Vehicle/FTPManager.h` | (Vehicle üzerinden) |
| `RemoteIDManager` | `src/Vehicle/RemoteIDManager.h` | `vehicle.remoteIDManager` |
| `QGCCameraManager` | `src/Camera/QGCCameraManager.h` | (Vehicle üzerinden) |
| `ComponentInformationManager` | `src/Vehicle/ComponentInformation/` | (Vehicle üzerinden) |
| `AutoPilotPlugin` | `src/AutoPilotPlugins/AutoPilotPlugin.h` | `vehicle.autopilotPlugin` |
| `FirmwarePlugin` | (Vehicle içinde pointer) | `vehicle.firmwarePlugin` (dolaylı) |

### 3.5 Boot Sırası

`QGCApplication::_initForNormalAppBoot()` (`src/QGCApplication.cc`):

```
1. QGCCorePlugin::instance()
2. VideoManager::instance()
3. QGCCorePlugin::createQmlApplicationEngine() → createRootWindow()
4. AudioOutput::init()
5. FollowMe::init()
6. QGCPositionManager::init()
7. LinkManager::init()
8. MultiVehicleManager::init()      ← MAVLinkProtocol::vehicleHeartbeatInfo'ya bağlanır
9. MAVLinkProtocol::init()
10. VideoManager::init() / init1()
```

---

## 4. Yeni Özellik Ekleme Kılavuzu (Extensibility Playbook)

### Senaryo A: Yeni UI Bileşeni veya QML Ekranı/Sekmesi

QGC'de UI ekleme yeri, ekranın türüne göre değişir. Ana shell `src/UI/MainRootWindow.qml`'dir (`ApplicationWindow`).

#### A.1 Yeni Paylaşılan QML Kontrolü

| Adım | Dosya / Dizin | İşlem |
|------|---------------|-------|
| 1 | `src/QmlControls/MyControl.qml` | Kontrolü oluştur; `QGCPalette`, `ScreenTools` import et |
| 2 | `src/QmlControls/QGroundControl/Controls/qmldir` | `MyControl 1.0 MyControl.qml` satırı ekle |
| 3 | `qgroundcontrol.qrc` | İki alias ekle: flat (`MyControl.qml`) + modül (`QGroundControl/Controls/MyControl.qml`) |
| 4 | Kullanım | `import QGroundControl.Controls` → `MyControl { }` |

Referans desen: `src/QmlControls/QGCButton.qml`

#### A.2 Yeni Analyze Aracı Sekmesi

| Adım | Dosya / Dizin | İşlem |
|------|---------------|-------|
| 1 | `src/AnalyzeView/MyAnalyzePage.qml` | Sayfa oluştur; isteğe bağlı `AnalyzePage` base layout kullan |
| 2 | `src/AnalyzeView/MyAnalyzeController.h/.cc` | Gerekirse C++ controller yaz |
| 3 | `src/AnalyzeView/CMakeLists.txt` | `target_sources(QGC PRIVATE ...)` |
| 4 | `QGCApplication::init()` | `qmlRegisterType<MyAnalyzeController>("QGroundControl.Controllers", 1, 0, "MyAnalyzeController")` |
| 5 | `qgroundcontrol.qrc` | QML dosyasını ekle |
| 6 | `src/API/QGCCorePlugin.cc` → `analyzePages()` | `QmlComponentInfo(title, QUrl("qrc:/qml/MyAnalyzePage.qml"), icon)` ekle |

Analyze navigasyonu: `AnalyzeView.qml` → `corePlugin.analyzePages` modeli → sağda `Loader`.

#### A.3 Yeni Uygulama Ayarları Sekmesi

| Adım | Dosya / Dizin | İşlem |
|------|---------------|-------|
| 1 | `src/UI/preferences/MySettings.qml` | `SettingsGroupLayout` tabanlı sayfa |
| 2 | `qgroundcontrol.qrc` | QML ekle |
| 3 | `src/UI/SettingsPagesModel.qml` | Yeni `ListElement { name, url, iconUrl, pageVisible }` |

#### A.4 Yeni Toolbar Göstergesi (Indicator)

| Adım | Dosya / Dizin | İşlem |
|------|---------------|-------|
| 1 | `src/UI/toolbar/MyIndicator.qml` | `showIndicator` property'si olan indicator QML |
| 2 | `qgroundcontrol.qrc` | `/toolbar` prefix ve/veya `QGroundControl/Controls` alias |
| 3a | App-level | `QGCCorePlugin::toolBarIndicators()` listesine URL ekle |
| 3b | Vehicle-level | `FirmwarePlugin::toolIndicators(Vehicle*)` override et |

Yükleme: `src/UI/toolbar/FlyViewToolBarIndicators.qml` → `Repeater` modeli.

#### A.5 Yeni Araç Kurulum (Setup) Bileşeni

| Adım | Dosya / Dizin | İşlem |
|------|---------------|-------|
| 1 | `src/AutoPilotPlugins/PX4/MyComponent.qml` | `SetupPage` tabanlı QML |
| 2 | `src/AutoPilotPlugins/PX4/MyComponent.h/.cc` | `VehicleComponent` alt sınıfı |
| 3 | `PX4AutoPilotPlugin::vehicleComponents()` | Yeni component'i listeye ekle |
| 4 | `qgroundcontrol.qrc` | QML ekle |

Setup navigasyonu: `src/Vehicle/VehicleSetup/SetupView.qml` → `autopilotPlugin.vehicleComponents`.

#### A.6 Yeni Ana Görünüm (Fly/Plan seviyesinde — nadir)

| Adım | Dosya / Dizin | İşlem |
|------|---------------|-------|
| 1 | `src/FlightDisplay/MyView.qml` (veya yeni dizin) | Tam ekran view |
| 2 | `src/UI/MainRootWindow.qml` | `FlyView`/`PlanView` yanına sibling ekle; `showMyView()` fonksiyonu |
| 3 | `src/UI/toolbar/` | Tool select dialog'a giriş ekle |
| 4 | `qgroundcontrol.qrc` | QML ekle |

#### A.7 Custom Build ile UI Genişletme

`custom-example/` şablonu:
- `QGCCorePlugin` alt sınıfı → `analyzePages()`, `toolBarIndicators()`, `paletteOverride()` override
- `createQmlApplicationEngine()` → `addImportPath("qrc:/Custom/Widgets")`
- Örnek: `custom-example/src/FlyViewCustomLayer.qml`, `CustomGuidedActionsController.qml`

---

### Senaryo B: Yeni Telemetri Verisi veya MAVLink Mesajı (C++ → QML)

#### B.1 Yeni FactGroup ile Telemetri (Önerilen Yol)

Mevcut örnek: `src/Vehicle/FactGroups/VehicleGPSFactGroup.h/.cc`

| Adım | Dosya | İşlem |
|------|-------|-------|
| 1 | `src/Vehicle/FactGroups/MyFactGroup.h` | `FactGroup` alt sınıfı; `Q_OBJECT`; Fact üyeleri tanımla |
| 2 | `src/Vehicle/FactGroups/MyFactGroup.cc` | Constructor'da `_addFact()`; `handleMessage()` override → `switch(msgid)` → `setRawValue()` |
| 3 | `src/Vehicle/FactGroups/CMakeLists.txt` | `target_sources(QGC PRIVATE MyFactGroup.cc MyFactGroup.h)` |
| 4 | `:/json/Vehicle/MyFact.json` (opsiyonel) | Fact metadata; `qgroundcontrol.qrc`'ye ekle |
| 5 | `src/Vehicle/Vehicle.h` | Member: `MyFactGroup _myFactGroup`; `Q_PROPERTY(FactGroup* my READ myFactGroup CONSTANT)` |
| 6 | `src/Vehicle/Vehicle.cc` → `_commonInit()` | `_addFactGroup(&_myFactGroup, "my")` |
| 7 | QML | `vehicle.my.someFactName.value` |

`handleMessage` otomatik çağrılır: `Vehicle::_mavlinkMessageReceived()` tüm kayıtlı FactGroup'ları iterate eder.

**FactGroup update rate:** Constructor'da `FactGroup(updateRateMsecs, ...)` — `0` = anında `valueChanged`, `1000` = saniyede bir batch.

#### B.2 Mevcut FactGroup'a Yeni Alan Ekleme

| Adım | Dosya | İşlem |
|------|-------|-------|
| 1 | İlgili `Vehicle*FactGroup.h` | Yeni `Fact _myFact` üyesi + accessor |
| 2 | `.cc` constructor | `_addFact(&_myFact, _myFactName)` |
| 3 | `.cc` `handleMessage()` | İlgili MAVLink case'inde `_myFact.setRawValue(...)` |
| 4 | JSON metadata (varsa) | `:/json/Vehicle/...json` güncelle |

#### B.3 Vehicle Seviyesinde Özel MAVLink Handler

MAVLink mesajı Fact sistemine uymuyorsa (ör. durum makinesi, tek seferlik olay):

| Adım | Dosya | İşlem |
|------|-------|-------|
| 1 | `src/Vehicle/Vehicle.cc` → `_mavlinkMessageReceived()` | `switch(message.msgid)` içine yeni `case` |
| 2 | Handler metodu | `_handleMyMessage(message)` — decode, state güncelle, `emit mySignalChanged()` |
| 3 | `src/Vehicle/Vehicle.h` | `Q_PROPERTY` + `NOTIFY` sinyali ekle |
| 4 | QML | `vehicle.myProperty` binding |

#### B.4 Firmware-Spesifik Telemetri

| Adım | Dosya | İşlem |
|------|-------|-------|
| 1 | `src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc` | `factGroups()` override → yeni FactGroup döndür |
| 2 | `Vehicle::_commonInit()` | Firmware plugin'den gelen FactGroup'lar otomatik `_addFactGroup()` ile eklenir |

#### B.5 Custom Build MAVLink Hook

| Adım | Dosya | İşlem |
|------|-------|-------|
| 1 | `QGCCorePlugin` alt sınıfı | `mavlinkMessage(Vehicle*, LinkInterface*, message)` override |
| 2 | Dönüş | `false` döndürürse Vehicle mesajı işlemez |

#### B.6 Veri Akışı Özeti

```
MAVLink mesajı
  → Vehicle::_mavlinkMessageReceived()
    → [FactGroup::handleMessage]  →  Fact::setRawValue()  →  valueChanged  →  QML binding
    → [Vehicle switch handler]    →  Q_PROPERTY notify    →  QML binding
```

---

### Senaryo C: Yeni Araç Tipi veya Mission Item Davranışı

#### C.1 Yeni Firmware / Araç Tipi Desteği

QGC tek `Vehicle` sınıfı kullanır; yeni araç tipi = yeni plugin.

| Adım | Dosya / Dizin | İşlem |
|------|---------------|-------|
| 1 | `src/FirmwarePlugin/MyFW/MyFirmwarePluginFactory.h/.cc` | `FirmwarePluginFactory` alt sınıfı; `firmwarePluginForAutopilot()` |
| 2 | `src/FirmwarePlugin/MyFW/MyFirmwarePlugin.h/.cc` | `FirmwarePlugin` alt sınıfı: flight mode'lar, yetenekler, mission command override |
| 3 | `src/FirmwarePlugin/MyFW/CMakeLists.txt` | `target_sources`; üst `FirmwarePlugin/CMakeLists.txt`'ye `add_subdirectory` |
| 4 | Factory constructor | `FirmwarePluginFactoryRegister::instance()->registerPluginFactory(this)` |
| 5 | `src/AutoPilotPlugins/MyFW/MyAutoPilotPlugin.h/.cc` | `AutoPilotPlugin` alt sınıfı: `vehicleComponents()` |
| 6 | `MyFirmwarePlugin::autopilotPlugin(Vehicle*)` | AutoPilotPlugin instance döndür |
| 7 | `src/AutoPilotPlugins/MyFW/` | `VehicleComponent` alt sınıfları + Setup QML'leri |
| 8 | `qgroundcontrol.qrc` | Setup QML'leri ekle |

Referans implementasyonlar:
- PX4: `src/FirmwarePlugin/PX4/`, `src/AutoPilotPlugins/PX4/`
- APM: `src/FirmwarePlugin/APM/`, `src/AutoPilotPlugins/APM/`

**APM araç tipi seçimi:** `APMFirmwarePluginFactory` → `MAV_TYPE`'a göre `ArduCopterFirmwarePlugin`, `ArduPlaneFirmwarePlugin`, vb.

#### C.2 Mevcut Firmware'e Yeni Yetenek / Flight Mode

| Adım | Dosya | İşlem |
|------|-------|-------|
| 1 | `PX4FirmwarePlugin.cc` (veya APM karşılığı) | `flightModes()`, `setFlightMode()`, `capabilities()` güncelle |
| 2 | `src/FirmwarePlugin/FirmwarePlugin.h` | `FirmwareCapabilities` enum'una yeni bit (gerekirse) |
| 3 | `Vehicle.h` | İlgili `Q_PROPERTY(bool xxxSupported)` (varsa) |

#### C.3 Yeni Toolbar/Mode Indicator (Firmware-Spesifik)

| Adım | Dosya | İşlem |
|------|-------|-------|
| 1 | `src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc` | `toolIndicators()` / `modeIndicators()` override |
| 2 | Indicator QML | `QmlComponentInfo` ile URL döndür |

#### C.4 Yeni Basit Mission Item (Waypoint Varyantı)

| Adım | Dosya | İşlem |
|------|-------|-------|
| 1 | `src/MissionManager/MissionCommandTree` | JSON override: `src/FirmwarePlugin/PX4/PX4-MavCmdInfo*.json` |
| 2 | `src/MissionManager/MavCmdInfo*.json` | Komut metadata (label, param info, friendly edit) |
| 3 | `MissionController` | Gerekirse özel editor QML bağlantısı |

Mission command hiyerarşisi: `MissionCommandTree` → firmware/vehicle class override'ları.

#### C.5 Yeni Complex Mission Item (Survey, Corridor vb. benzeri)

| Adım | Dosya | İşlem |
|------|-------|-------|
| 1 | `src/MissionManager/MyComplexItem.h/.cc` | `ComplexMissionItem` alt sınıfı |
| 2 | `patternName()`, `complexDistance()`, `minAMSLAltitude()` vb. override | |
| 3 | `src/MissionManager/MyComplexItem.qml` | Harita üzerinde görsel editör |
| 4 | `src/MissionManager/CMakeLists.txt` | `target_sources` |
| 5 | `qgroundcontrol.qrc` | QML ekle |
| 6 | `MissionController` veya `PlanMasterController` | Yeni item tipini oluşturma/listeye ekleme mantığı |
| 7 | `PlanView` QML | Mission item palette'ine yeni buton |

Mevcut örnekler:
- `SurveyComplexItem` ← `TransectStyleComplexItem` ← `ComplexMissionItem`
- `StructureScanComplexItem`, `CorridorScanComplexItem`, `LandingComplexItem`

#### C.6 Yeni Section (Kamera/Hız vb. waypoint eki)

| Adım | Dosya | İşlem |
|------|-------|-------|
| 1 | `src/MissionManager/MySection.h/.cc` | `Section` alt sınıfı |
| 2 | `SimpleMissionItem` | Section desteği entegrasyonu |
| 3 | Plan View QML | Section editor UI |

#### C.7 Mission Wire Protocol (Upload/Download)

| Adım | Dosya | İşlem |
|------|-------|-------|
| 1 | `src/MissionManager/MissionItem.h/.cc` | Ham MAVLink `MISSION_ITEM` temsili |
| 2 | `src/MissionManager/MissionManager.h/.cc` | `PlanManager` alt sınıfı; MAVLink mission protocol |
| 3 | `VisualMissionItem::save()` / `load()` | UI modeli ↔ `MissionItem` dönüşümü |

---

## 5. Geliştirici Notları ve Kritik Kurallar

### 5.1 Asla Yapılmaması Gerekenler

| Hata | Doğru Yaklaşım |
|------|----------------|
| `Vehicle` alt sınıfı oluşturmak | `FirmwarePlugin` + `FactGroup` kullan |
| QML'den doğrudan `LinkInterface::_writeBytes()` çağırmak | `Vehicle::sendMessageOnLinkThreadSafe()` veya `Vehicle` metotları |
| Global singleton'ı `new` ile oluşturmak | `Q_APPLICATION_STATIC` + `instance()` kalıbını takip et |
| MAVLink işlemeyi QML'de yapmak | Tüm parse/decode C++'da (`Vehicle`, `FactGroup`) |
| Her telemetry için ayrı `QTimer` | `FactGroup` update rate mekanizmasını kullan |
| `qt_add_qml_module` ile modül oluşturmak (mevcut yapıda) | QRC + `qmldir` + manuel `qmlRegisterType` kalıbını takip et |
| CMake'e yeni `.cc` ekleyip `target_sources(QGC PRIVATE ...)` unutmak | Her yeni C++ dosyası ilgili modül `CMakeLists.txt`'sine eklenmeli |
| Fact olmadan ham `Q_PROPERTY(double)` ile telemetri expose etmek | `Fact`/`FactGroup` sistemi birim dönüşümü, metadata, deferred update sağlar |

### 5.2 Thread Safety Kuralları

QGC'nin ana iş parçacığı modeli:

```
Ana Thread (Qt Event Loop)
├── QML UI rendering
├── Tüm Vehicle/Manager/Settings işlemleri
├── MAVLinkProtocol::receiveBytes()  ← link sinyalleri genellikle ana thread'de
└── Fact::valueChanged → QML binding
```

**Thread-safe yazma (link'e veri gönderme):**

```cpp
// DOĞRU — herhangi bir thread'den çağrılabilir
link->writeBytesThreadSafe(bytes, len);
// İçeride: QMetaObject::invokeMethod(this, "_writeBytes", Qt::AutoConnection, data)

// YANLIŞ — doğrudan _writeBytes() çağırma
link->_writeBytes(data);  // LinkInterface.h: "Not thread safe if called directly"
```

**Thread-safe MAVLink gönderme:**

```cpp
vehicle->sendMessageOnLinkThreadSafe(link, message);
vehicle->sendJoystickDataThreadSafe(roll, pitch, yaw, thrust, buttons);
```

**GStreamer video:** `GstVideoReceiver` ayrı `Worker : QThread` kullanır (`src/VideoManager/VideoReceiver/GStreamer/GstVideoReceiver.h`). Video pipeline işlemleri worker thread'de; UI güncellemeleri signal/slot ile ana thread'e dönmeli.

**Firmware upgrade:** `PX4FirmwareUpgradeThread` ayrı thread'de çalışır; timer ve bootloader işlemleri o thread'de oluşturulur.

**Genel kural:** QObject yaşam döngüsü ve QML binding'leri **ana thread'de** kalmalı. Başka thread'den UI veya Fact güncellemesi → `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` veya sinyal/slot.

### 5.3 QML/C++ Performans İpuçları

| Konu | Detay |
|------|-------|
| **Deferred Fact updates** | `FactGroup(1000, ...)` → saniyede 1 `valueChanged`; yüksek frekanslı veri için dikkatli seç |
| **Signal compression** | `QGCApplication::addCompressedSignal()` — aynı sinyalin kuyrukta tekrarlanmasını önler |
| **QML Loader** | Tool drawer ve settings analyze'da `Loader` ile lazy load; ağır sayfaları başlangıçta yükleme |
| **Binding zinciri** | `vehicle.gps.lat.value` gibi derin binding'lerde gereksiz ara property oluşturma |
| **Repeater model** | Toolbar indicator'ları `Repeater` ile yükler; model listesini sabit tut |
| **QmlObjectListModel** | Vehicle listesi gibi dinamik listeler için C++ tarafında `QmlObjectListModel` kullan (QML `ListView` modeli) |
| **Live updates** | `FactGroup::setLiveUpdates(true)` — debug/inspector için; production'da performans maliyeti var |

### 5.4 QML Type Kayıt Checklist

Yeni C++ sınıfını QML'e açarken:

1. `ClassName::registerQmlTypes()` static metodu yaz (veya `QGCApplication::init()`'e ekle)
2. Uygun modül URI seç: `QGroundControl.Controllers`, `QGroundControl.Vehicle`, vb.
3. Creatable mı?
   - Evet → `qmlRegisterType<>` (controller'lar)
   - Hayır → `qmlRegisterUncreatableType<>` (Vehicle, Fact, LinkManager)
   - Global tekil → `qmlRegisterSingletonType<>`
4. `.h` dosyasında `Q_OBJECT`, gerekli `Q_PROPERTY`, `Q_INVOKABLE`, `Q_MOC_INCLUDE` ekle
5. CMake'de dosyayı `target_sources(QGC PRIVATE ...)` ile ekle

### 5.5 QRC Kayıt Checklist

Yeni QML dosyası eklerken `qgroundcontrol.qrc`'ye **iki alias** ekle:

```xml
<!-- Drawer Loader için flat path -->
<file alias="MyPage.qml">src/AnalyzeView/MyPage.qml</file>
<!-- Module import için -->
<file alias="QGroundControl/Controls/MyPage.qml">src/AnalyzeView/MyPage.qml</file>
```

### 5.6 Settings Ekleme

| Adım | Dosya |
|------|-------|
| 1 | `src/Settings/MySettings.h/.cc` — `FactGroup` veya `SettingsGroup` alt sınıfı |
| 2 | `src/Settings/SettingsManager.h/.cc` — yeni settings grubu property |
| 3 | `src/Settings/CMakeLists.txt` — `target_sources` |
| 4 | QML preferences sayfası — `FactTextField` / `FactComboBox` ile binding |

### 5.7 Logging

```cpp
QGC_LOGGING_CATEGORY(MyComponentLog, "qgc.mycomponent")
// Kullanım: qCDebug(MyComponentLog) << "message";
```

### 5.8 Test

Testler `test/` altında; `QGC_BUILD_TESTING=ON` ile derlenir. İlgili modül test dizinine yeni test ekle (ör. `test/Vehicle/`, `test/MissionManager/`).

---

## Hızlı Referans: En Çok Değiştirilen Dosyalar

| Amaç | Dosya |
|------|-------|
| Uygulama boot | `src/QGCApplication.cc` |
| QML engine / root window | `src/API/QGCCorePlugin.cc` |
| Ana UI shell | `src/UI/MainRootWindow.qml` |
| Fly view | `src/FlightDisplay/FlyView.qml` |
| Plan view | `src/PlanView/PlanView.qml` |
| Global QML API | `src/QmlControls/QGroundControlQmlGlobal.h` |
| Araç oluşturma | `src/Vehicle/MultiVehicleManager.cc` |
| MAVLink routing | `src/Vehicle/Vehicle.cc` → `_mavlinkMessageReceived()` |
| Telemetri modeli | `src/Vehicle/FactGroups/` |
| Bağlantı yönetimi | `src/Comms/LinkManager.cc` |
| MAVLink parse | `src/Comms/MAVLinkProtocol.cc` |
| Firmware davranışı | `src/FirmwarePlugin/PX4/`, `src/FirmwarePlugin/APM/` |
| Setup UI | `src/AutoPilotPlugins/`, `src/Vehicle/VehicleSetup/SetupView.qml` |
| Mission planlama | `src/MissionManager/MissionController.h` |
| QML resource bundle | `qgroundcontrol.qrc` |
| Build yapılandırması | `CMakeLists.txt`, `src/CMakeLists.txt`, `cmake/CustomOptions.cmake` |
| Custom build örneği | `custom-example/` |

---

## Mimari Diyagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        QML View Layer                           │
│  MainRootWindow → FlyView / PlanView / ToolDrawer(Loader)       │
│  import QGroundControl, QGroundControl.Controls, ...            │
└──────────────────────────┬──────────────────────────────────────┘
                           │ Q_PROPERTY / valueChanged
┌──────────────────────────▼──────────────────────────────────────┐
│                   QGroundControlQmlGlobal                       │
│  linkManager, multiVehicleManager, settingsManager, corePlugin  │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                     C++ Manager Layer                           │
│  LinkManager → MAVLinkProtocol → MultiVehicleManager            │
│  SettingsManager, VideoManager, FirmwarePluginManager, ...      │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                      Vehicle (per drone)                        │
│  FactGroups (telemetry) + MissionManager + ParameterManager     │
│  FirmwarePlugin → AutoPilotPlugin → VehicleComponents           │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                   Comms / MAVLink Layer                         │
│  LinkInterface (Serial/UDP/TCP/BT) → bytesReceived → parse       │
└─────────────────────────────────────────────────────────────────┘
```

---

*Son güncelleme: kod tabanı analizi baz alınarak oluşturulmuştur. Dosya yolları `/home/yusuf/qgroundcontrol` workspace'ine göredir.*
