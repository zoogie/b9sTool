#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include "payload.h"
#include "firm_old.h"
#include "firm_new.h"

#define VERSION "3.0.0"
#define RWCHUNK	(2048*512) //2048 sectors (1 MB)
#define RWMINI	(RWCHUNK/16) //64 KB

//Function index________________________
int main();
int checkNCSD();
void dump3dsNand(int mode);
void restore3dsNand(int mode);
void xorbuff(u8 *in1, u8 *in2, u8 *out);
void installB9S();
u32 handleUI();
u32 waitNandWriteDecision();
u32 getMBremaining();
void error(int code);
//______________________________________

const char *green="\x1b[32;1m";
const char *yellow="\x1b[33;1m";
const char *blue="\x1b[34;1m";
const char *white="\x1b[37;1m";

int menu_index=0;
u32 sysid=0;  //NCSD magic
u32 ninfo=0;  //nand size in sectors
u32 sizeMB=0; //nand/firm size in MB
u32 remainMB=0;
u32 System=0; //will be one of the below 2 variables
int b9s_install_count=0; //don't let user do this more than once!
u32 N3DS=2;
u32 O3DS=1;
char workdir[]= "boot9strap";
char nand_type[80]={0};   //sd filename buffer for nand/firm dump/restore.
u8 *workbuffer; //raw dump and restore in first two options
u8 *fbuff; //sd firm files
u8 *xbuff; //xorpad
u8 *nbuff; //nand
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
	
    iprintf("initializing fat...\n");
	if (!fatInitDefault()) error(0);

	workbuffer = (u8*)malloc(RWCHUNK);  //setup and allocate our buffers. 
	fbuff = (u8*)malloc(RWMINI);
	nbuff = (u8*)malloc(RWMINI);
	xbuff = (u8*)malloc(RWMINI);
	
	remainMB=getMBremaining();
	
	mkdir(workdir, 0777);   //boot9strap folder creation
	chdir(workdir);
	checkNCSD();     //read ncsd header for needed info
	
	//iprintf("\rwaiting...%c%c%c",firm_old[0], firm_new[0], payload[0]);
	//while(1)swiWaitForVBlank();
	while(handleUI());  //game loop

	systemShutDown();
	return 0;
}

int checkNCSD() {
	nand_ReadSectors(0 , 1 , workbuffer);   //get NCSD (nand) header of the 3ds
	memcpy(&sysid, workbuffer + 0x100, 4);  //NCSD magic
	memcpy(&ninfo, workbuffer + 0x104, 4);  //nand size in sectors (943 or 1240 MB). used to determine old/new 3ds.
	if     (ninfo==0x00200000){ System=O3DS;}    //old3ds
	else if(ninfo==0x00280000){ System=N3DS;}    //new3ds
	if(!System || sysid != 0x4453434E)error(2);  //this error triggers if NCSD magic not present or nand size unexpected value.

	return 0;
}

void dump3dsNand(int mode) {
	consoleClear();
	remainMB=getMBremaining();
	u32 foffset;
	if(mode){                                   //full nand dump/restore (mode=1).
		foffset=0x0;
		if(System==O3DS){
			sizeMB=943;
			strcpy(nand_type,"NAND_OLD3DS.BIN");
		}
		else{  //System==N3DS
			sizeMB=1240;
			strcpy(nand_type,"NAND_NEW3DS.BIN");
		}
	}
	else{                                      //firm0 and firm1 dump/restore (mode=0)
		foffset=0x0B130000/0x200;
		if(System==O3DS){
			sizeMB=8;
			strcpy(nand_type,"F0F1_OLD3DS.BIN");
		}
		else {
			sizeMB=8;
			strcpy(nand_type,"F0F1_NEW3DS.BIN");
		}
	}
	
	if(remainMB < (sizeMB + 20) ) {
		iprintf("Error: not enough disk space!\n");
		iprintf("You need %ldMB free\n", sizeMB + 20);  //the extra 20 MBs is a "safety buffer"
		iprintf("You have %ldMB free\n", remainMB);
		iprintf("You need to free up %ldMB\n", sizeMB + 20 - remainMB);
		int wait=60; while(wait--)swiWaitForVBlank();
		return;
	}
	
	iprintf("open boot9strap/%s\n",nand_type); //open boot9strap/F0F1_OLD3DS.BIN (31 chars - ok)
	iprintf("hold B to cancel\n");

	FILE *f = fopen(nand_type, "wb");
	if(!f)error(1);

	u32 rwTotal=sizeMB*1024*1024;
	iprintf("progress 0/%dMB    \r", (int)rwTotal / 0x100000);
	for(int i=0;i<rwTotal;i+=RWCHUNK){           //read from nand, dump to sd
		if(nand_ReadSectors(i / 0x200 + foffset, RWCHUNK / 0x200, workbuffer) == false){
			iprintf("nand read error\noffset: %08X\naborting...", (int)i);
			fclose(f); unlink(nand_type);
			break;
		}
		if(fwrite(workbuffer, 1, RWCHUNK, f) < RWCHUNK){
			iprintf("sdmc write error\noffset: %08X\naborting...", (int)i);
			fclose(f); unlink(nand_type);
			break;
		}
		iprintf("progress %d/%dMB    \r",i / 0x100000 + 1, (int)rwTotal / 0x100000);
		scanKeys();
		int keys = keysHeld();
		if(keys & KEY_B){
			iprintf("\ncanceling...");
			fclose(f); unlink(nand_type);
			break;
		}
	}

	fclose(f);
	iprintf("\ndone.\r");
	remainMB=getMBremaining(); //for the title status
}

void restore3dsNand(int mode) {
	consoleClear();
	u32 foffset;
	if(mode){
		foffset=0x0;
		if(System==O3DS){
			sizeMB=943;
			strcpy(nand_type,"NAND_OLD3DS.BIN");
		}
		else{
			sizeMB=1240;
			strcpy(nand_type,"NAND_NEW3DS.BIN");
		}
	}
	else{
		foffset=0x0B130000/0x200;
		if(System==O3DS){
			sizeMB=8;
			strcpy(nand_type,"F0F1_OLD3DS.BIN");
		}
		else {
			sizeMB=8;
			strcpy(nand_type,"F0F1_NEW3DS.BIN");
		}
	}

	int rerror=0;
	int werror=0;

	if(waitNandWriteDecision())return;
	consoleClear();

	iprintf("open boot9strap/%s\n",nand_type);
	iprintf("do NOT poweroff!!\n");

	FILE *f = fopen(nand_type, "rb");
	if(!f)error(1);

	u32 rwTotal=sizeMB*1024*1024;
	iprintf("progress 0/%dMB    \r", (int)rwTotal / 0x100000);
	for(int i=0;i<rwTotal;i+=RWCHUNK){
		if(fread(workbuffer, 1, RWCHUNK, f) < RWCHUNK){
			rerror++;
			continue;  //better not to write anything if it's bad
		}
		if(nand_WriteSectors(i / 0x200 + foffset, RWCHUNK / 0x200, workbuffer) == false){
			werror++;
		}
		iprintf("progress %d/%dMB    \r",i / 0x100000 + 1, (int)rwTotal / 0x100000);
	}

	fclose(f);
	iprintf("\nreadErrors=%d\nwriteErrors=%d\n",rerror, werror);
	iprintf("done.\n");
	if(rerror || werror) iprintf("if there's any errors, it's\nbest to run this again now.");
}

void xorbuff(u8 *in1, u8 *in2, u8 *out){

	for(int i=0; i < RWMINI; i++){
		out[i] = in1[i] ^ in2[i];
	}

}

void installB9S() {
	consoleClear();
	iprintf("%sWILL BRICK%s if A9LH is installed!", yellow, white); 
	iprintf("%sBACKUP%s your NAND&F0F1 first!\n", yellow, white);
	iprintf("%sMAKE SURE%s your firmware matches\n", yellow, white);
	iprintf("the %sBLUE%s above!\n\n", blue, white);
	if(waitNandWriteDecision())return;
	consoleClear();
	if(b9s_install_count){
		iprintf("Don't do this again!\n");
		int wait=60; while(wait--)swiWaitForVBlank();
		return;
	}

	if     (System==O3DS){
		memcpy(fbuff, firm_old, payload_len);
	}
	else {
		memcpy(fbuff, firm_new, payload_len);
	}

	u32 foffset=0x0B130000 / 0x200; //firm0 nand offset

	nand_ReadSectors(foffset, payload_len / 0x200, nbuff);  //get raw enc firm 11.x on nand
	xorbuff(fbuff,nbuff,xbuff);                             //xor the enc firm 11.x with plaintext firm 11.x to create xorpad buff
	memcpy(fbuff, payload, payload_len);                    //get payload
	xorbuff(fbuff,xbuff,nbuff);                             //xor payload and xorpad to create final encrypted image to write to nand
	nand_WriteSectors(foffset, payload_len / 0x200, nbuff); //write it to nand

	iprintf("%d KBs written to FIRM0\r", payload_len / 1024);
	iprintf("\ndone.");
	b9s_install_count++;
}

u32 handleUI(){
	consoleSelect(&topScreen);
	consoleClear();

	const int menu_size=6;
	const char menu[][64]={
		{"Exit\n"},
		{"Install boot9strap\n\n   \x1b[32;1m--NAND--\x1b[37;1m"},
		
		//--NAND--
		{"Dump to sdmc"},
		{"Restore from sdmc\n\n   \x1b[32;1m--F0F1--\x1b[37;1m"},
		
		//--F0F1--
		{"Dump to sdmc"},
		{"Restore from sdmc"},
	};

	iprintf("b9sTool %s | %ldMB free\n", VERSION, remainMB); 
	if    (System==O3DS)iprintf("%sOLD 3DS%s\n", yellow, white);
	else                iprintf("%sNEW 3DS%s\n", yellow, white);
	iprintf("%s%s%s\n\n", blue, firmwareNames, white);

	for(int i=0;i<menu_size;i++){
		iprintf("%s%s\n", i==menu_index ? " > " : "   ", menu[i]);
	}

	swiWaitForVBlank();
	scanKeys();
	int keypressed=keysDown();

	if     (keypressed & KEY_DOWN)menu_index++;
	else if(keypressed & KEY_UP)  menu_index--;
	else if(keypressed & KEY_A){
		consoleSelect(&bottomScreen);
		if     (menu_index==0)return 0;          //break game loop and exit app
		else if(menu_index==1)installB9S();
		else if(menu_index==2)dump3dsNand(1);    // dump/restore full nand
		else if(menu_index==3)restore3dsNand(1);
		else if(menu_index==4)dump3dsNand(0);    // dump/restore firm0 and firm1
		else if(menu_index==5)restore3dsNand(0);
	}
	if(menu_index >= menu_size)menu_index=0;     //menu index wrap around check
	else if(menu_index < 0)    menu_index=menu_size-1;

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

void error(int code){
	switch(code){
		case 0: iprintf("Fat could not be initialized!\n"); break;
		case 1: iprintf("Could not open file handle!\n"); break;
		case 2: iprintf("Not a 3ds (or nand read error)!\n"); break;
		case 3: iprintf("Could not open sdmc files!\n"); break;
		default: break;
	}

	iprintf("Press home to exit\n");
	//free(workbuffer); free(fbuff); free(nbuff); free(xbuff);
	while(1)swiWaitForVBlank();
}
