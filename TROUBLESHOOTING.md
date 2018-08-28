A. If you get an a9lh error. 
- First, make sure you really don't have a9lh. If you use b9sTool with a9lh, you will brick.<br>
If you're sure this is a false positive: rename fastboot3DS/key.bin (this is your console unique NAND header) to danger_skip_a9lh.bin<br>
Then rerun b9sTool and the error should be skipped and you get to the menu. Unlike other override files, this one will stay persistant and not be deleted.<br>

B. You select "Install fastboot3DS", reset the system, but the system just boots up normal.<br>
- Go to System Settings > Other Settings > Page 4 > System Update. Make sure to actually attempt to update with wifi on.<br>
It should say "The system is up to date". Press ok and let the system fully reboot to home menu on its own. <br>
Boot b9sTool again.<br>
Choose "Install boot9strap", and hopefully you will see luma3ds config on next reboot.<br>