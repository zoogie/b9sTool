A. If you get an a9lh error.<br>
- First, make sure you really don't have a9lh. If you use b9sTool with a9lh, you will brick.
If you're sure this is a false positive: rename boot9strap/key.bin (this is your console unique NAND header) to danger_skip_a9lh.bin
Then rerun b9sTool and the error should be skipped and you get to the menu. Unlike other override files, this one will stay persistant and not be deleted.

B. The first time you run b9sTool, you get BACKUP.BIN verify errors of some sort.
- Make sure BACKUP.BIN is in the boot9strap folder. Don't move it around or rename it. Moving/renaming it will not cause the file to regenerate. Just don't ever touch it.
Rename boot9strap/key.bin to danger_reset_backup.bin. Then rerun b9sTool and hopefully the menu shows up.

C. You select "Install boot9strap", reset the system, but the system just boots up normal.
- Boot b9sTool and select "Restore Stock Firmware". Turn off system then restart.
Go to System Settings > Other Settings > Page 4 > System Update. Make sure to actually attempt to update with wifi on.
It should say "your system is up to date". Press ok and let the system fully reboot to home menu on its own. Then turn it off.
Boot b9sTool again and hold X + LeftDpad for 5 seconds. A red 3rd menu option should appear for you to reset the backup. Select it. This will only work if FIRM STATUS is STOCK.
After the backup.bin resets, choose "Install boot9strap", and hopefully you will see luma3ds config on next reboot.

D. After installing boot9strap, you get FIRM STATUS: UNKNOWN<br>
Go to System Settings > Other Settings > Page 4 > System Update. Make sure to actually attempt to update with wifi on. It should say "your system is up to date".<br>
Press ok and let the system fully reboot to home menu on its own. Then turn it off.<br>
Go and delete sdmc:/boot9strap/BACKUP.BIN manually from the sd card<br>
Rename key.bin to danger_reset_backup.bin<br>
Then boot b9sTool and the FIRM STATUS should be STOCK.<br>
choose "Install boot9strap", and hopefully you will see luma3ds config on next reboot.