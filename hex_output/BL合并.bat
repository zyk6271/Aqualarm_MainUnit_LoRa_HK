srec_cat.exe Bootloader.bin -Binary -offset 0x8000000 -o boot.hex -Intel
srec_cat.exe ..\HongKong\rtthread.bin -Binary -offset 0x800C000 -o app.hex -Intel
copy /b .\boot.hex + .\app.hex Aqualarm_CW.hex
del boot.hex
del app.hex