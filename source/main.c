#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include "payload.h"
#include "firm_old.h"
#include "firm_new.h"
#include "stage2_payload.h"
#include "stage3_payload.h"
#include "a9lh_bin_installer.h"
#include "secret_sector.h"
#include "hash_stash.h"

#define VERSION "5.0.2"
#define WKDIR "boot9strap"
#define RWMINI	(payload_len)

u32 foffset=0x0B130000 / 0x200; //FIRM0
//  foffset=0x0B530000 / 0x200;   FIRM1 (for experts or yolo'ers only!)
int menu_size=2;

//Function index________________________
int main();
int checkNCSD();
void xorbuff(u8 *in1, u8 *in2, u8 *out);
void installB9S();
void handleA9LH();
u32 handleUI();
u32 waitNandWriteDecision();
u32 getMBremaining();
int getFirmBuf(u32 len);
int file2buf(char *path, u8 *buf, u32 limit);
int buf2file(char *path, const u8 *buf, u32 size);
int checkA9LH();
int verifyUnlockKey(char *path, bool delete);
void error(int code);
//______________________________________

const char *green="\x1b[32;1m";
const char *yellow="\x1b[33;1m";
const char *blue="\x1b[34;1m";
const char *dblue="\x1b[34;0m";
const char *white="\x1b[37;1m";
const char *red="\x1b[31;1m";

int menu_index=0;
u32 MB=0x100000;
u32 KB=0x400;
u32 sysid=0;  //NCSD magic
u32 ninfo=0;  //nand size in sectors
u32 sizeMB=0; //nand/firm size in MB
u32 remainMB=0;
u32 System=0; //will be one of the below 2 variables
u32 N3DS=2;
u32 O3DS=1;
u32 firmStatus=0;
u64 frame=0;

char workdir[]= WKDIR;
u8 *workbuffer; //raw dump and restore in first two options
u8 *fbuff; //sd firm files
u8 *xbuff; //xorpad
u8 *nbuff; //nand
char a9lhkey[14];
bool clear_stage2 = false;
PrintConsole topScreen;
PrintConsole bottomScreen;

int main() {
	
	videoSetModeSub(MODE_0_2D); //first 7 lines are all dual screen printing init taken from libnds's nds-examples
	videoSetMode(MODE_0_2D);   
	vramSetBankC(VRAM_C_SUB_BG);
	vramSetBankA(VRAM_A_MAIN_BG);
	consoleInit(&bottomScreen, 3,BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
	consoleInit(&topScreen, 3,BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
	consoleSelect(&topScreen); 
	
	iprintf("Initializing FAT... ");
	if (!fatInitDefault()) error(0);
	printf("good\n");
	iprintf("Initializing NAND... ");
	if(!nand_Startup()) error(5);
	printf("good\n");
	
	workbuffer = (u8*)malloc(RWMINI);  //setup and allocate our buffers. 
	fbuff = (u8*)malloc(RWMINI);
	nbuff = (u8*)malloc(RWMINI);
	xbuff = (u8*)malloc(RWMINI);
	if(!workbuffer || !fbuff || !nbuff || !xbuff) error(4);
	
	remainMB=getMBremaining();
	mkdir(workdir, 0777);   //app folder creation
	chdir(workdir);
	checkNCSD();            //read ncsd header for needed info
	dumpUnlockKey();

	int a9lh = checkA9LH();
	if(a9lh) {
		const char* impl = (a9lh == 12) ? "ShadowNAND" : "arm9loaderhax";
		iprintf("\n\n\n%s detected!\n\nHandling for this is\nexperimental; please try to\nuninstall it some other way\nbefore running b9stool.\n\n", impl);
		iprintf("If you have exhausted\nother options for\nuninstalling %s,\npress A to continue.\nPress B to exit.\n\nAssitance: https://discord.gg/C29hYvh", impl);
		while (1) {
			scanKeys();
			int keys = keysHeld();
			if(keys & KEY_A) break;
			if(keys & KEY_B) {
				consoleClear();
				systemShutDown();
				return 0;
			}
			swiWaitForVBlank();
		}
		consoleSelect(&bottomScreen);
		handleA9LH();
	}
	
	int wait=120;
	while(wait--)swiWaitForVBlank();
	while(handleUI());  //game loop

	systemShutDown();
	return 0;
}

int checkNCSD() {
	u8 hash[20];
	nand_ReadSectors(0 , 1 , workbuffer);   //get NCSD (nand) header of the 3ds
	swiSHA1Calc(hash, workbuffer, 0x200);
	sprintf(a9lhkey, "a9lh_%04X.bin", *((u16*) hash));
	memcpy(&sysid, workbuffer + 0x100, 4);  //NCSD magic
	memcpy(&ninfo, workbuffer + 0x104, 4);  //nand size in sectors (943 or 1240 MB). used to determine old/new 3ds.
	if     (ninfo==0x00200000){ System=O3DS;}    //old3ds
	else if(ninfo==0x00280000){ System=N3DS;}    //new3ds
	if(!System || sysid != 0x4453434E)error(2);  //this error triggers if NCSD magic not present or nand size unexpected value.

	return 0;
}

void xorbuff(u8 *in1, u8 *in2, u8 *out){

	for(int i=0; i < RWMINI; i++){
		out[i] = in1[i] ^ in2[i];
	}

}

bool verified_nand_ReadSectors(sec_t sector, sec_t numSectors, void* buffer) {
	u8 hash1[20];
	u8 hash2[20];

	if(!nand_ReadSectors(sector, numSectors, buffer))
		return false;

	swiSHA1Calc(hash1, buffer, numSectors * 0x200);

	if(!nand_ReadSectors(sector, numSectors, buffer))
		return false;

	swiSHA1Calc(hash2, buffer, numSectors * 0x200);

	return (memcmp(hash1, hash2, 20) == 0);

}

bool verified_nand_WriteSectors(sec_t sector, sec_t numSectors, const void* buffer) {
	u8* buf = malloc(numSectors * 0x200);

	if(!buf) return false;

	if(!nand_WriteSectors(sector, numSectors, buffer) ||
	!nand_ReadSectors(sector, numSectors, buf) ||
	(memcmp(buffer, buf, numSectors * 0x200) != 0)) {
		free(buf);
		return false;
	}

	free(buf);
	return true;
}

void installB9S() {
	int res;
	u8 hash[20];
	char xorname[64]={0};
	consoleClear();
	//iprintf("%sMAY BRICK%s if A9LH is installed!", yellow, white); 
	iprintf("%sREMEMBER:%s\n", yellow, white);
	iprintf("%s%s%s!\n\n", blue, firmwareNames, white);
	if(waitNandWriteDecision())return;
	consoleClear();
	
	memset(workbuffer, 0, RWMINI);
	iprintf("Loading clean firm ...\n");
	
	if(!verified_nand_ReadSectors(foffset, payload_len / 0x200, workbuffer)) 
		error(7);

	memcpy(nbuff, workbuffer, payload_len);
	
	res = getFirmBuf(payload_len);  
	if(!res) error(6);

	if(res==O3DS){
		memcpy(fbuff, firm_old, payload_len);
	}
	else if(res==N3DS){
		memcpy(fbuff, firm_new, payload_len);
	}
	
	iprintf("Preparing payload firm...\n");
	
	xorbuff(fbuff,nbuff,xbuff);                             //xor the enc firm with plaintext firm to create xorpad buff
	memcpy(fbuff, payload, payload_len);                    //get payload
	xorbuff(fbuff,xbuff,nbuff);                             //xor payload and xorpad to create final encrypted image to write to destination
	memcpy(workbuffer, nbuff, payload_len);	                //write it to destination
	
	res = verified_nand_WriteSectors(foffset, payload_len/0x200, workbuffer) ? 0 : 1;
	if(res) iprintf("%sNAND WRITE FAIL !!!!!!!!!!!!!!\n",red);
	iprintf("Result: %08X %s\n", res, res ? "HASH FAIL":"HASH GOOD!");
	iprintf("%d KBs written to FIRM\n", payload_len / 1024);
	unlink("/luma/config.bin");
	
	if(!res && clear_stage2) {
		nand_ReadSectors(0x5BFFF, 1, workbuffer);
		if((workbuffer[0] != 0xFF) && (workbuffer[0] != 0x00))
			workbuffer[0] = 0x00;
		memset(workbuffer, workbuffer[0], 0x200);
		for(int i=0;i<0x44E;i++)
			nand_WriteSectors(0x5BFFF+i, 1, workbuffer);
	}

	swiSHA1Calc(hash, xbuff, payload_len);
	sprintf(xorname, "xorpad_%02X%02X%02X%02X.bin", hash[0],hash[1],hash[2],hash[3]);
	buf2file(xorname, xbuff, payload_len); 					//might be useful someday, but for now, we'll just put it here for safekeeping.
	iprintf("%s created\n", xorname);
	
	iprintf("\nDone!\n");
	error(99); //not really an error, we just don't want multiple nand writes in one session.
}

void handleA9LH() {
	iprintf("After b9stool runs, the\nsystem will reboot.\n\nIf you don't see a\nluma configuration menu\nafter the reboot,\nrun b9stool again ONCE.\n\n");

	if(waitNandWriteDecision()) error(99);
	
	u8 hash[20];

	swiSHA1Calc(hash, payload, payload_len);
	if(memcmp(hash, SHA1B9S, 20)) error(6);
	swiSHA1Calc(hash, stage2_payload, stage2_payload_len);
	if(memcmp(hash, SHA1S2P, 20)) error(6);
	swiSHA1Calc(hash, stage3_payload, stage3_payload_len);
	if(memcmp(hash, SHA1S3P, 20)) error(6);
	swiSHA1Calc(hash, a9lh_bin_installer, a9lh_bin_installer_len);
	if(memcmp(hash, SHA1ABI, 20)) error(6);
	if(System == N3DS) {
		swiSHA1Calc(hash, secret_sector, secret_sector_len);
		if(memcmp(hash, SHA1SEC, 20)) {
			if(file2buf("secret_sector.bin", secret_sector, secret_sector_len))
				error(6);
			swiSHA1Calc(hash, secret_sector, secret_sector_len);
			if(memcmp(hash, SHA1SEC, 20))
				error(6);
		} else if(buf2file("secret_sector.bin", secret_sector, secret_sector_len))
			error(6);
	}

	if (
		buf2file("boot9strap.firm", payload, payload_len) ||
		buf2file("boot9strap.firm.sha", SHA2B9S, 32) ||
		buf2file("arm9loaderhax_si.bin", a9lh_bin_installer, a9lh_bin_installer_len)
	) error(6);

	rename("key.bin", a9lhkey);
	unlink("/luma/config.bin");

	// LAZINESS WARNING: assumes payload lengths divide nicely into a number of sectors
	if (!verified_nand_WriteSectors(0x5C000, stage2_payload_len / 0x200, stage2_payload))
		iprintf("%sSTAGE2 WRITE FAIL !!!!!!!!!!!!!!\n",red);
	if (!verified_nand_WriteSectors(0x5C008, stage3_payload_len / 0x200, stage3_payload))
		iprintf("%sSTAGE3 WRITE FAIL !!!!!!!!!!!!!!\n",red);

	iprintf("\nDone!\n");
	error(99);
}

u32 handleUI(){
	consoleSelect(&topScreen);
	consoleClear();
	
	char action[64];
	sprintf(action,"Install %s\n", WKDIR);

	char menu[2][64];
	strcpy(menu[0],"Exit\n");
	strcpy(menu[1],action);

	iprintf("b9sTool %s | %ldMB free\n", VERSION, remainMB); 
	if    (System==O3DS)iprintf("%sOLD 3DS%s\n\n", blue, white);
	else                iprintf("%sNEW 3DS%s\n\n", blue, white);

	for(int i=0;i<menu_size;i++){
		iprintf("%s%s\n", i==menu_index ? " > " : "   ", menu[i]);
	}
	
	iprintf("\n%sWARNING:%s DONT install payload\n",yellow,white);
	iprintf("multiple times!!\n");
	iprintf("%sWARNING:%s Only use b9sTool with\n",yellow,white);
	iprintf("https://3ds.hacks.guide\n");
	iprintf("%sWARNING:%s\n",yellow,white);

	if(frame % 30 < 15) iprintf("%s%s%s\n", blue,  firmwareNames, white);
		else 			iprintf("%s%s%s\n", white, firmwareNames, white);

	swiWaitForVBlank();
	scanKeys();
	int keypressed=keysDown();

	if     (keypressed & KEY_DOWN)menu_index++;
	else if(keypressed & KEY_UP)  menu_index--;
	else if(keypressed & KEY_A){
		consoleSelect(&bottomScreen);
		if     (menu_index==0)return 0;          //break game loop and exit app
		else if(menu_index==1)installB9S();
	}
	if(menu_index >= menu_size)menu_index=0;     //menu index wrap around check
	else if(menu_index < 0)    menu_index=menu_size-1;

	frame++;
	return 1;  //continue game loop
}

u32 waitNandWriteDecision(){
	iprintf("NAND writing is DANGEROUS!\n");
	iprintf("START+SELECT = %sGO!%s\n", green, white);
	iprintf("B to decline.\n");
	while(1){
		scanKeys();
		int keys = keysHeld();
		if((keys & KEY_START) && (keys & KEY_SELECT))break;
		if(keys & KEY_B){
			consoleClear();
			return 1;
		}
		swiWaitForVBlank();
	}
	return 0;
}

u32 getMBremaining(){
	struct statvfs stats;
	statvfs("/", &stats);
	u64 total_remaining = ((u64)stats.f_bsize * (u64)stats.f_bavail) / (u64)0x100000;
	return (u32)total_remaining;
}

int getFirmBuf(u32 len){
	u8 digest[20];
	int res=0;
	
	iprintf("Loading %s from RAM...\n", WKDIR);
	swiSHA1Calc(digest, payload, len);
	res = memcmp(SHA1B9S, digest, 20);
	if(res) return 0;
	
	iprintf("Loading Native Firm from RAM...\n");
	if(System==O3DS){
		swiSHA1Calc(digest, firm_old, len);
		res = memcmp(SHA1OLD, digest, 20);
		if(!res) return O3DS;
		printf("O3DS firm not in RAM trying SD\n");
		file2buf(FIRM_OLD, firm_old, payload_len);
		swiSHA1Calc(digest, firm_old, len);
		res = memcmp(SHA1OLD, digest, 20);
		if(!res) return O3DS;
	}
	else if(System==N3DS){
		swiSHA1Calc(digest, firm_new, len);
		res = memcmp(SHA1NEW, digest, 20);
		if(!res) return N3DS;
		printf("N3DS firm not in RAM trying SD\n");
		file2buf(FIRM_NEW, firm_new, payload_len);
		swiSHA1Calc(digest, firm_new, len);
		res = memcmp(SHA1NEW, digest, 20);
		if(!res) return N3DS;
	}
	
	iprintf("Native Firm not found, abort...\n");
	
	return 0;  //fail
}

int file2buf(char *path, u8 *buf, u32 limit){
	u32 size=0;
	FILE *f=fopen(path, "rb");
	if(!f) return 1;
	
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if(size > limit) size = limit;
	
	fread(buf, 1, size, f);
	fclose(f);
	
	return 0;
}

int buf2file(char *path, const u8 *buf, u32 size){
	u32 bytes_written=0;
	FILE *f=fopen(path, "wb");
	if(!f) return 1;
	
	bytes_written = fwrite(buf, 1, size, f);
	fclose(f);
	
	if(bytes_written < size) return 2;
	
	return 0;
}

int checkA9LH(){
	int res=0;
	iprintf("Checking for A9LH...\n");
	
	res = verifyUnlockKey(a9lhkey, true);  //override check if file found
	if(!res){
		iprintf("A9LH file found, overriding\nA9LH check.\n");
		clear_stage2 = true;
		return 0;
	}
	
	if (!verified_nand_ReadSectors(0x5C000, 80*KB/0x200, workbuffer))
		error(7); 
	if(memmem(workbuffer, 80*KB, "arm9loaderhax.bin", 17) != 0) return 9; //standard a9lh and 3dsafe
	if(memmem(workbuffer, 80*KB, "boot.bin", 8) != 0) return 12;          //shadowNAND
	//nand offset 0xB800000 or FIRM1+0x2D0000
    //this is where plaintext stage2 a9lh payload is written
    //and the string "arm9loaderhax.bin" or "boot.bin" should be within 80KB of that offset
	
	return 0;
}

int verifyUnlockKey(char *path, bool delete){
	FILE *f=fopen(path,"rb");
	if(!f){
		//iprintf("Unlock file read error\n");
		return 1;
	}
	fread(workbuffer, 1, 0x200, f);
	fclose(f);
	nand_ReadSectors(0, 1, workbuffer+0x200);
	
	if(memcmp(workbuffer, workbuffer+0x200, 0x200)) return 1;
	if(delete==true) unlink(path);
	
	return 0;
}

int dumpUnlockKey(){
	FILE *f=fopen("key.bin","wb");
	if(!f){
		error(10);
	}
	nand_ReadSectors(0, 1, workbuffer);
	fwrite(workbuffer, 1, 0x200, f);
	fclose(f);
	return 0;
}

void error(int code){
	switch(code){ //0 4 5 6 9 2 99 9 12 10
		case 0:  iprintf("Fat could not be initialized!\n"); break;
		case 2:  iprintf("Not a 3ds (or nand read error)!\n"); break;
		case 4:  iprintf("Workbuffer(s) failed init!\n"); break;
		case 5:  iprintf("Nand failed init!\n"); break;
		case 6:  iprintf("File load failed"); break;
		case 7:  iprintf("Nand read error"); break;
		//case 9:  iprintf("A9LH dectected! Brick avoided!!\nhttps://discord.gg/C29hYvh (assistance)\n"); break;
		case 10: iprintf("Unlock file read error\n"); break;
		//case 12: iprintf("shadowNAND! Brick avoided!!\nhttps://discord.gg/C29hYvh (assistance)\n"); break;
		case 99:;
		default: break;
	}

	iprintf("\nPress home to exit\n");
	while(1)swiWaitForVBlank();
}
