
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

