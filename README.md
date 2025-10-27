
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

