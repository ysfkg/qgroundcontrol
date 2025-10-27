
<p align="center">
  <img src="https://raw.githubusercontent.com/Dronecode/UX-Design/35d8148a8a0559cd4bcf50bfa2c94614983cce91/QGC/Branding/Deliverables/QGC_RGB_Logo_Horizontal_Positive_PREFERRED/QGC_RGB_Logo_Horizontal_Positive_PREFERRED.svg" alt="QGroundControl Logo" width="500">
</p>

<p align="center">
  <a href="https://github.com/mavlink/QGroundControl/releases">
    <img src="https://img.shields.io/github/release/mavlink/QGroundControl.svg" alt="Latest Release">
  </a>
</p>
************************** v1.0 ************************ 

+Gimbal hareket komutları eklendi  
+Kamera görüntü gecikemsi azaltılmaya çalışıldı  
+Kamera görüntüsü yakınlaştırma eklendi 

************************** v1.0.1 **********************  Üsttekiler ile birleştirildi  
+Kamera fotoğraf çekme komutları eklendi 

************************** v1.1 ***********************   
+Video Kaynağı 2 olacak şekilde güncellendi

*************************** v1.2 *********************** Üsttekiler ile birleştirildi   
+Kalan pil yüzdesi için algoritma eklendi  
+Arayüzde düzeltmeler yapıldı 
 
*************************** v1.2.1 *********************** Üsttekiler ile birleştirildi  
+Gimbal hareketleri düz pot ile çalışacak şekilde güncellendi, vehicle.cc dosyası  

*************************** v1.2.2 *********************** Üsttekiler ile birleştirildi  
+C12 kamera zaman senkronizasyonu eklendi
+Video kayıt sorunu %99 çözüldü - PTS/DTS timestamp problemi düzeltildi
+C12 kameraya video kayıt başlatma/durdurma komutları UDP ile gönderiliyor

*************************** v1.2.1.1 *********************** Üsttekiler ile birleştirildi  
+Video akışı kesinti ve paket kaybına karşı dayanıklı hale getirildi
+RTSP video kaynağı için kontrollü restart mekanizması eklendi
+Video kaydı beklenmedik durduğunda kamera senkronizasyonu eklendi
+10 saniye error recovery mekanizması (kayıt sırasında hemen dur, görüntüleme sırasında bekle)
+Exponential backoff restart stratejisi (2s → 4s → 8s → 30s → 5dk)
+RTSP TEARDOWN mesajı temiz gönderimi sağlandı

**Değişen Dosyalar ve Detaylar:**

**src/Camera/SimulatedCameraControl.cc:**
  - QDateTime, QTimeZone, QByteArray, QString header'ları eklendi
  - toHex2Upper(): uint8_t değeri hexadecimal stringe çeviren yardımcı fonksiyon
  - addCrc(): Kamera komutuna CRC checksum ekleyen fonksiyon
  - buildTimCommandForNow(): Europe/Istanbul saat diliminde sistem saatini alıp C12 kamera formatına çeviren fonksiyon
    * Format: #TPUDFwTIMhhmmss.ssDDMMYYCC
    * hh: Saat (24 saat formatı, 00-23)
    * mm: Dakika
    * ss: Saniye
    * ss: Centisaniye (milisaniyenin 1/10'u)
    * DD: Gün
    * MM: Ay
    * YY: Yıl (son 2 hane)
    * CC: CRC checksum
  - sendTimeToCamera(): UDP ile kameraya zaman bilgisi gönderen fonksiyon
  - SimulatedCameraControl constructor'ına otomatik zaman senkronizasyonu eklendi
    * Uygulama başlatıldığında otomatik olarak C12 kameraya (192.168.144.108:5000) zaman bilgisi gönderilir
    * Cihazın sistem saati Europe/Istanbul saat dilimine çevrilerek kullanılır
    * UDP protokolü ile zaman komutu iletilir
  - takePhoto(): C12 kameraya fotoğraf çekme komutu gönderimi (#TPUD2wCAP013E)
  - startVideoRecording(): C12 kameraya video kayıt başlatma komutu (#TPUD2wREC0144)
  - stopVideoRecording(): C12 kameraya video kayıt durdurma komutu (#TPUD2wREC0043)

**src/VideoManager/VideoReceiver/GStreamer/GstVideoReceiver.cc:**
  - Video kayıt PTS/DTS timestamp problemi çözüldü (wallclock bazlı hesaplama)
  - _recordingProbe(): Yeni probe fonksiyonu eklendi - gerçek zamanlı PTS/DTS/Duration hesaplama
  - Recording queue buffer ayarları optimize edildi:
    * leaky=0 (buffer kaybı yok)
    * max-size-buffers=100 (5 saniye buffer)
    * max-size-time=5 saniye
  - Keyframe kontrolü geliştirildi - kayıt mutlaka keyframe ile başlıyor
  - Matroska muxer ayarları eklendi:
    * streamable=FALSE (dosya sonunda index yazılır, seeking için)
    * min-index-interval=0 (her keyframe'de index)
    * max-cluster-duration=2 saniye
  - Wallclock bazlı timestamp tracking:
    * _recordingStartTime: Kayıt başlangıç zamanı (monotonic clock)
    * _recordingFrameCount: Frame sayacı
    * _lastFrameTimestamp: Son frame PTS (duration hesabı için)
  - Frame-by-frame PTS hesaplama (g_get_monotonic_time kullanarak)
  - Her frame için doğru duration hesaplaması
  - Detaylı debug logging (ilk 10 frame + her 100 frame)
  - FPS ve gerçek süre istatistikleri

**src/VideoManager/VideoReceiver/GStreamer/GstVideoReceiver.h:**
  - _recordingStartTime: GstClockTime türünde yeni member
  - _recordingFrameCount: guint64 türünde frame sayacı
  - _lastFrameTimestamp: GstClockTime türünde son frame PTS
  - _recordingProbe(): Static probe fonksiyon tanımı

**v1.2.1.1 Değişiklikleri:**

**src/Camera/SimulatedCameraControl.cc:**
  - VideoManager::recordingChanged signal'ine bağlantı eklendi
  - Video kayıt beklenmedik durduğunda:
    * Timer otomatik durdurulur
    * Kameraya stop komutu (#TPUD2wREC0043) gönderilir
  - Zaman senkronizasyonu çift gönderim (2s + 7s sonra)
  - Detaylı hata loglama eklendi

**src/Camera/CameraUdpManager.cc:**
  - Komut kuyruğu loglama iyileştirildi
  - Socket durumu ve hedef adres loglama eklendi
  - Priority queue detaylı takibi

**src/VideoManager/VideoManager.h:**
  - VideoReceiverData yapısına eklenen alanlar:
    * restartCount: Restart deneme sayısı
    * lastRestartTime: Son restart zamanı (msecs)
    * currentRestartDelay: Mevcut restart gecikmesi (ms)
  - VideoReceiverData1 yapısına aynı alanlar eklendi

**src/VideoManager/VideoManager.cc:**
  - onStopComplete signal handler'ı kontrollü restart ile güncellendi:
    * Exponential backoff: 2s → 4s → 8s → 16s → 30s (max)
    * 10 başarısız denemeden sonra 5 dakika bekleme
    * QTimer::singleShot ile geciktirilmiş restart
  - onStartComplete signal handler'ı güncellendi:
    * Başarılı bağlantıda restart sayacını sıfırla
    * currentRestartDelay'i 2000ms'ye resetle
  - Detaylı restart loglama (deneme sayısı, gecikme süresi)

**src/VideoManager/VideoReceiver/GStreamer/GstVideoReceiver.h:**
  - _errorDetected: bool - Error tespit flag'i eklendi

**src/VideoManager/VideoReceiver/GStreamer/GstVideoReceiver.cc:**
  - RTSP source ayarları optimize edildi:
    * latency: 200ms → 1000ms (güçlü buffer)
    * protocols: TCP only → UDP+TCP+UDP-Multicast (0x00000007)
    * retry: 100 → 0 (GStreamer retry kapalı, VideoManager yönetir)
    * tcp-timeout: 20s → 10s
    * teardown-timeout: 5s eklendi
    * short-header: TRUE (uyumluluk için)
  - UDP source ayarları optimize edildi:
    * buffer-size: 2MB eklendi
    * timeout: 0 (sürekli dinle)
    * retrieve-sender-address: FALSE
  - TCP source timeout: 20s eklendi
  - RTP jitterbuffer ayarları optimize edildi:
    * latency: 200ms → 1000ms
    * do-retransmission: TRUE
    * rtx-max-retries: 20
    * rtx-delay: 40ms
    * do-lost: FALSE
    * mode: 1 (buffer mode)
  - Decoder queue ayarları:
    * leaky: 2 (downstream - eski frame'leri at)
    * max-size-buffers: 50 (~2.5s @ 20fps)
    * max-size-bytes: 10MB
    * max-size-time: 2s
  - H.265 parser config-interval: 1 (her 1 saniyede config gönder)
  - Video decoder ayarları:
    * output-corrupt: TRUE (hatalı frame'leri de göster)
    * skip-frame: 0 (hiçbir frame'i atlama)
  - MPEG-TS demuxer: ignore-pcr: TRUE
  - Smoothing queue optimize edildi (3 buffers, 100ms)
  - _watchdog() fonksiyonu güncellendi:
    * Kayıt sırasında timeout → hemen stop
    * _errorDetected flag set edildiğinde → 10s bekle sonra restart
    * Normal görüntüleme timeout → devam et
  - _noteTeeFrame() fonksiyonu güncellendi:
    * Frame geldiğinde _errorDetected flag'i clear et
    * "Stream recovered" logu
  - _handleEOS() fonksiyonu güncellendi:
    * Kayıt sırasında EOS → hemen stop
    * Görüntüleme sırasında EOS → _errorDetected flag set et
  - _onBusMessage() GST_MESSAGE_ERROR handler güncellendi:
    * Kayıt sırasında error → hemen stop
    * Görüntüleme sırasında error → _errorDetected flag set et
  - stop() fonksiyonu güncellendi:
    * _errorDetected flag reset
    * PAUSED → NULL state geçişi (temiz RTSP TEARDOWN)
    * 2 saniye PAUSED state bekleme
  - start() fonksiyonu güncellendi:
    * _errorDetected flag reset
  - Tüm error/timeout durumlarında detaylı loglama

**Teknik Detaylar:**

**Error Recovery Stratejisi:**
1. Error/EOS tespit edildiğinde:
   - Kayıt yapılıyorsa → hemen stop() (bozuk dosya engelle)
   - Görüntüleme yapılıyorsa → _errorDetected = true, 10s bekle
2. Watchdog her saniye kontrol eder:
   - _errorDetected = true ve 10s geçti → restart
   - 10s içinde frame geldi → _errorDetected = false, devam et
3. VideoManager restart stratejisi:
   - Deneme 1: 2s bekle
   - Deneme 2: 4s bekle
   - Deneme 3: 8s bekle
   - Deneme 4: 16s bekle
   - Deneme 5+: 30s bekle (max)
   - Deneme 11, 21, 31...: 5 dakika bekle
   - Başarılı bağlantı: Sayacı sıfırla

**RTSP Bağlantı Yönetimi:**
- UDP/TCP otomatik protokol seçimi
- GStreamer internal retry devre dışı
- VideoManager kontrollü restart
- Temiz TEARDOWN mesajı gönderimi
- 5 saniye TEARDOWN timeout

**Buffer ve Paket Kaybı Koruması:**
- 1 saniye video buffer
- 20 kez kayıp paket retransmission
- 2MB UDP buffer
- Hatalı frame'leri göster (stream kesintisiz)
- Eski frame'leri at, yeni frame'leri göster

