XIAO ESP32-C6 firmware binaries
===============================

These are verified ESP32-C6 build artifacts (Arduino core 3.3.11, NimBLE 2.5.1),
used by the drop-in flasher:

  python flash.py --c6

Files:
  bootloader.bin             ESP32-C6 second-stage bootloader   (0x0000)
  partitions.bin             4 MB partition table                (0x8000)
  boot_app0.bin              OTA data                            (0xe000)
  oui-spy-unified-blue.bin   application firmware                (0x10000)

Flash offsets are the same as the S3; --c6 targets chip esp32c6 / 4MB.

To regenerate after changing the firmware:

  pio run -e seeed_xiao_esp32c6
  cp .pio/build/seeed_xiao_esp32c6/bootloader.bin firmware-c6/
  cp .pio/build/seeed_xiao_esp32c6/partitions.bin firmware-c6/
  cp .pio/build/seeed_xiao_esp32c6/firmware.bin    firmware-c6/oui-spy-unified-blue.bin
  # boot_app0.bin comes from the installed core and rarely changes:
  find ~/.platformio/packages -path '*tools/partitions/boot_app0.bin' | head -1 | xargs -I{} cp {} firmware-c6/

Or skip flash.py and let PlatformIO flash directly:

  pio run -e seeed_xiao_esp32c6 -t upload
