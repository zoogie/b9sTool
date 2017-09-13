#### Instructions

https://3ds.guide/

#### General Info

This app only writes to FIRM0, not FIRM1, so it should be safe given your FIRM1 is not corrupt or a9lh'd.    
Never use this on anything but 3DS FIRM 11.4.X-YZ. NEVER use this if arm9loaderhax or sighax is installed (on any firmware)! The result will be a brick!

Compiling: Just supply the current decrypted FIRMs for both new/old 3ds and put them in the firm_new and firm_old 
directories respectively. Then place the newest boot9strap.firm payload in the payload directory. Then compile with
make FIRM_INFO="11.4-11.5 only" or whatever is acually the firmware range of the current native firm.
The user does not have to supply any extra files like in previous versions. Needs the latest libfat version.

#### Credits

+ zoogie
