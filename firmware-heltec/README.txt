Heltec WiFi LoRa 32 V4 firmware binaries
========================================

Verified ESP32-S3 build artifacts for the Heltec WiFi LoRa 32 V4 (Arduino
core 2.x / espressif32, NimBLE 1.4.x, on-board OLED). Used by flash.py:

  python flash.py

Files:
  bootloader.bin             ESP32-S3 bootloader                 (0x0000)
  partitions.bin             16 MB partition table                (0x8000)
  boot_app0.bin              OTA data                             (0xe000)
  oui-spy-unified-blue.bin   application firmware                 (0x10000)

Board notes: on-board white LED on GPIO35 (reads inverted); no on-board buzzer
or NeoPixel, so the audible/RGB alerts are silent unless you wire external parts
to GPIO7 (buzzer) / GPIO4 (NeoPixel). The on-board OLED (SDA 17 / SCL 18 / RST 21,
powered via Vext GPIO36) shows a live status/stats screen. Full pin map: see the
[env:heltec_wifi_lora_32_v4] block in platformio.ini.

To regenerate after changing the firmware:

  pio run
  cp .pio/build/heltec_wifi_lora_32_v4/bootloader.bin firmware-heltec/
  cp .pio/build/heltec_wifi_lora_32_v4/partitions.bin firmware-heltec/
  cp .pio/build/heltec_wifi_lora_32_v4/firmware.bin    firmware-heltec/oui-spy-unified-blue.bin
  # boot_app0.bin comes from the installed core:
  find ~/.platformio/packages -path '*framework-arduinoespressif32*tools/partitions/boot_app0.bin' | head -1 | xargs -I{} cp {} firmware-heltec/

Or just let PlatformIO flash directly:

  pio run -t upload
