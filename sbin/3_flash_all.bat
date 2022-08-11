
esptool.py -p COM4 -b 460800 --before default_reset --after hard_reset --chip esp32s3  write_flash --flash_mode dio --flash_size detect --flash_freq 80m 0x0 ../build/bootloader/bootloader.bin 0x8000 ../build/partition_table/partition-table.bin 0x10000 ../build/monofon_ESP_6_4.bin  0x100000 ../build/storage.bin

