@rem Run script 

@echo off

set backupPath=%path%
set NSPath=%CD%
cd /d c:\Program Files\qemu
set path=%path%;%NSPath%

qemu-system-i386.exe -no-reboot -no-shutdown    ^
-device sb16                                    ^
-d cpu_reset                                    ^
-m 256M                                         ^
-boot d                                         ^
-cdrom %nspath%\build\image.iso                 ^
-debugcon stdio                                 ^
-display sdl                                    ^
-accel tcg                                      ^
-monitor telnet:127.0.0.1:55555,server,nowait

: -s -S                                         -- for debugging with GDB
: -serial COM7                                  -- to output the serial port to somewhere real
: -kernel %nspath%/kernel.bin 
: -debugcon stdio
: -monitor telnet:127.0.0.1:55555,server,nowait -- to use the QEMU console
:
:qemu-system-i386 -m 16M -drive file=\\.\PHYSICALDRIVE1,format=raw
rem -s -S 

:-drive id=disk,file=%nspath%\vdisk.vdi,if=none  ^
:-device ahci,id=ahci                            ^
:-device ide-hd,drive=disk,bus=ahci.0            ^

rem go back
cd /d %NSPath%

set path=%backupPath%
