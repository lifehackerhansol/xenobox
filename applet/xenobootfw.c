#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
#include "../libprism.h"
#else
#include "../xenobox.h"
#endif

// 0.79: this decoder is based on DeSmuME, but not copy any longer as I replaced functions with secure area encryption's.
void enc_decrypt(u32 *magic, u32 *arg1, u32 *arg2);
void enc_init1(u32 cardheader_gamecode, int level, u32 modulo);

#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
#define PRINTF _consolePrintf2
#else
static char *output;
#include <stdarg.h>
static int PRINTF(const char* format, ...){
    va_list args;
    va_start( args, format );
    int ret = vfprintf( stderr, format, args );
    va_end(args);
    return ret;
}
#endif

static u8		*tmp_data9;
static u8		*tmp_data7;
static u32		size9, size7;

extern u32 card_hash[0x412];
//static u32		keyCode[3];
static u32		ARM9bootAddr;
static u32		ARM7bootAddr;
static bool		patched;

typedef struct{
	u16	part3_rom_gui9_addr;		// 000h
	u16	part4_rom_wifi7_addr;		// 002h
	u16	part34_gui_wifi_crc16;		// 004h
	u16	part12_boot_crc16;			// 006h
	u8	fw_identifier[4];			// 008h
	u16	part1_rom_boot9_addr;		// 00Ch
	u16	part1_ram_boot9_addr;		// 00Eh
	u16	part2_rom_boot7_addr;		// 010h
	u16	part2_ram_boot7_addr;		// 012h
	u16	shift_amounts;				// 014h
	u16	part5_data_gfx_addr;		// 016h

	u8	fw_timestamp[5];			// 018h
	u8	console_type;				// 01Dh
	u16	unused1;					// 01Eh
	u16	user_settings_offset;		// 020h
	u16	unknown1;					// 022h
	u16	unknown2;					// 024h
	u16	part5_crc16;				// 026h
	u16	unused2;					// 028h	- FFh filled 
} HEADER;
static HEADER header;

//static void crypt64BitUp(u32 *ptr){enc_encrypt(card_hash, ptr+1, ptr);}

static void crypt64BitDown(u32 *ptr){enc_decrypt(card_hash, ptr+1, ptr);}
static void noop(u32 *ptr){}

static bool initKeycode(u32 idCode, int level){
	enc_init1(idCode,level,12);
	return true;
}

static u16 getBootCodeCRC16(){
	u16 crc=0xffff;
	crc=swiCRC16(crc,tmp_data9,size9);
	crc=swiCRC16(crc,tmp_data7,size7);
	return crc;
}

#define FILLBUF if(!(xIn&7))memcpy(curBlock,in+xIn,8),operation(curBlock);
static u32 process(const u8 *in, u8** _out, type_u32p operation)
{
	u32 curBlock[2] = { 0 };
	u32 blockSize = 0;
	u32 xLen = 0;

	u32 i = 0, j = 0;
	u32 xIn = 0, xOut = 0;
	u32 len = 0;
	u32 offset = 0;
	u32 windowOffset = 0;
	u8 d = 0;
	u16 data = 0;

	FILLBUF
	xIn=4;
	blockSize = curBlock[0] >> 8;

	if(!blockSize)return 0;

	*_out = (u8*)malloc(blockSize);
	if(!*_out)return 0;
	u8 *out=*_out;
	memset(out, 0xFF, blockSize);

	xLen = blockSize;
	while(xLen > 0)
	{
		d = ((u8*)(curBlock))[xIn&7];
		xIn++;
		FILLBUF

		for(i = 0; i < 8; i++)
		{
			if(d & 0x80)
			{
				data = ((u8*)(curBlock))[xIn&7] << 8;
				xIn++;
				FILLBUF
				data |= ((u8*)(curBlock))[xIn&7];
				xIn++;
				FILLBUF

				len = (data >> 12) + 3;
				offset = (data & 0xFFF);
				windowOffset = (xOut - offset - 1);

				for(j = 0; j < len; j++){
					out[xOut]=out[windowOffset];
					xOut++;
					windowOffset++;

					xLen--;
					if(!xLen)return blockSize;
				}
			}
			else
			{
				out[xOut]=((u8*)(curBlock))[xIn&7];
				xOut++;
				xIn++;
				FILLBUF

				xLen--;
				if(!xLen)return blockSize;
			}

			d = ((d << 1) & 0xFF);
		}
	}
	
	return blockSize;
}
//================================================================================
static int load(u8 *data){
	//u32 size = 0;
	u16 shift1 = 0, shift2 = 0, shift3 = 0, shift4 = 0;
	u32 part1addr = 0, part2addr = 0, part3addr = 0, part4addr = 0, part5addr = 0;
	u32 part1ram = 0, part2ram = 0;
	
	//u32	src = 0;

	memcpy(&header, data, sizeof(header));
	if ((header.fw_identifier[0] != 'M') ||
			(header.fw_identifier[1] != 'A') ||
				(header.fw_identifier[2] != 'C')){
					free(data);
					return 4;
				}

	// only 12bits are used
	shift1 = ((header.shift_amounts >> 0) & 0x07);
	shift2 = ((header.shift_amounts >> 3) & 0x07);
	shift3 = ((header.shift_amounts >> 6) & 0x07);
	shift4 = ((header.shift_amounts >> 9) & 0x07);

	part1addr = (header.part1_rom_boot9_addr << (2 + shift1));
	part1ram = (0x02800000 - (header.part1_ram_boot9_addr << (2+shift2)));
	part2addr = (header.part2_rom_boot7_addr << (2+shift3));
	part2ram = (((header.shift_amounts&0x1000)?0x02800000:0x03810000) - (header.part2_ram_boot7_addr << (2+shift4)));
	part3addr = (header.part3_rom_gui9_addr << 3);
	part4addr = (header.part4_rom_wifi7_addr << 3);
	part5addr = (header.part5_data_gfx_addr << 3);

	ARM9bootAddr = part1ram;
	ARM7bootAddr = part2ram;

	initKeycode(read32(data+8), 1);
	crypt64BitDown((u32*)&data[0x18]);

	initKeycode(read32(data+8), 2);

	size9 = process(data + part1addr, &tmp_data9, crypt64BitDown);
	if (!tmp_data9){
		free(data);
		return 3;
	}

	size7 = process(data + part2addr, &tmp_data7, crypt64BitDown);
	if (!tmp_data7){
		free(tmp_data9);
		free(data);
		return 3;
	}

	u16 crc16_mine = getBootCodeCRC16();

	if (crc16_mine != header.part12_boot_crc16){
		PRINTF("Firmware: ERROR: the boot code CRC16 (0x%04X) doesn't match the value in the firmware header (0x%04X)", crc16_mine, header.part12_boot_crc16);
		free(tmp_data7);
		free(tmp_data9);
		free(data);
		return 4;
	}

	PRINTF("Firmware:\n");
	//PRINTF("- path: %s\n", CommonSettings.Firmware);
	//PRINTF("- size: %i bytes (%i Mbit)\n", size, size/1024/8);
	PRINTF("- CRC : 0x%04X\n", header.part12_boot_crc16);
	PRINTF("- header: \n");
	PRINTF("   * size firmware %i\n", ((header.shift_amounts >> 12) & 0xF) * 128 * 1024);
	PRINTF("   * ARM9 boot code address:     0x%08X\n", part1addr);
	PRINTF("   * ARM9 boot code RAM address: 0x%08X\n", ARM9bootAddr);
	PRINTF("   * ARM9 unpacked size:         0x%08X (%i) bytes\n", size9, size9);
	PRINTF("   * ARM9 GUI code address:      0x%08X\n", part3addr);
	PRINTF("\n");
	PRINTF("   * ARM7 boot code address:     0x%08X\n", part2addr);
	PRINTF("   * ARM7 boot code RAM address: 0x%08X\n", ARM7bootAddr);
	PRINTF("   * ARM7 WiFi code address:     0x%08X\n", part4addr);
	PRINTF("   * ARM7 unpacked size:         0x%08X (%i) bytes\n", size7, size7);
	PRINTF("\n");
	PRINTF("   * Data/GFX address:           0x%08X\n", part5addr);

	patched = false;
	if(data[0x17C] != 0xFF)patched = true;

	if(patched){
		free(tmp_data7);
		free(tmp_data9);

		u32 patch_offset = 0x3FC80;
		if (data[0x17C] > 1)
			patch_offset = 0x3F680;

		memcpy(&header, data + patch_offset, sizeof(header));

		shift1 = ((header.shift_amounts >> 0) & 0x07);
		shift2 = ((header.shift_amounts >> 3) & 0x07);
		shift3 = ((header.shift_amounts >> 6) & 0x07);
		shift4 = ((header.shift_amounts >> 9) & 0x07);

		part1addr = (header.part1_rom_boot9_addr << (2 + shift1));
		part1ram = (0x02800000 - (header.part1_ram_boot9_addr << (2+shift2)));
		part2addr = (header.part2_rom_boot7_addr << (2+shift3));
		part2ram = (((header.shift_amounts&0x1000)?0x02800000:0x03810000) - (header.part2_ram_boot7_addr << (2+shift4)));

		ARM9bootAddr = part1ram;
		ARM7bootAddr = part2ram;

		size9 = process(data + part1addr, &tmp_data9, noop);
		if (!tmp_data9){
			free(data);
			return 3;
		}

		size7 = process(data + part2addr, &tmp_data7, noop);
		if (!tmp_data7){
			free(tmp_data9);
			free(data);
			return 3;
		};

		PRINTF("\nFlashme:\n");
		PRINTF("- header: \n");
		PRINTF("   * ARM9 boot code address:     0x%08X\n", part1addr);
		PRINTF("   * ARM9 boot code RAM address: 0x%08X\n", ARM9bootAddr);
		PRINTF("   * ARM9 unpacked size:         0x%08X (%i) bytes\n", size9, size9);
		PRINTF("\n");
		PRINTF("   * ARM7 boot code address:     0x%08X\n", part2addr);
		PRINTF("   * ARM7 boot code RAM address: 0x%08X\n", ARM7bootAddr);
		PRINTF("   * ARM7 unpacked size:         0x%08X (%i) bytes\n", size7, size7);
	}

	free(data);
	u32 pad9=0x100-(size9&0xff);
	u32 pad7=0x100-(size7&0xff);

#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
	//Build NDS image from tmp_dataX and sizeX
	extern u8 ndshead[];

	u8 *pFileBuf=(u8*)malloc(0x200+size9+size7);
#else
unsigned char ndshead[512]={
  0x2e,0x00,0x00,0xea,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x23,0x23,0x23,0x23,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,
  0x00,0x02,0x00,0x00,0x00,0x00,0x38,0x02,0x00,0x00,0x38,0x02,0x00,0xa0,0x00,0x00,
  0x00,0xa2,0x00,0x00,0x00,0xd8,0x3a,0x02,0x00,0xd8,0x3a,0x02,0x00,0x19,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0xff,0x7f,0x7f,0x00,0xff,0x1f,0x3f,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x1e,0x05,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0xbe,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x53,0x52,0x41,0x4d,0x5f,0x56,0x31,0x31,0x30,0x00,0x00,0x00,0x50,0x41,0x53,0x53,
  0x30,0x31,0x96,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0xc8,0x60,0x4f,0xe2,0x01,0x70,0x8f,0xe2,0x17,0xff,0x2f,0xe1,0x12,0x4f,0x11,0x48,
  0x12,0x4c,0x20,0x60,0x64,0x60,0x7c,0x62,0x30,0x1c,0x39,0x1c,0x10,0x4a,0x00,0xf0,
  0x14,0xf8,0x30,0x6a,0x80,0x19,0xb1,0x6a,0xf2,0x6a,0x00,0xf0,0x0b,0xf8,0x30,0x6b,
  0x80,0x19,0xb1,0x6b,0xf2,0x6b,0x00,0xf0,0x08,0xf8,0x70,0x6a,0x77,0x6b,0x07,0x4c,
  0x60,0x60,0x38,0x47,0x07,0x4b,0xd2,0x18,0x9a,0x43,0x07,0x4b,0x92,0x08,0xd2,0x18,
  0x0c,0xdf,0xf7,0x46,0x04,0xf0,0x1f,0xe5,0x00,0xfe,0x7f,0x02,0xf0,0xff,0x7f,0x02,
  0xf0,0x01,0x00,0x00,0xff,0x01,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1a,0x9e,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

	u8 *pFileBuf=(u8*)malloc(0x200+size9+pad9+size7+pad7);
#endif
	memcpy(pFileBuf,ndshead,512);
	write32(pFileBuf+0x24,ARM9bootAddr);
	write32(pFileBuf+0x28,ARM9bootAddr);
	write32(pFileBuf+0x2c,size9/*+pad9*/);
	write32(pFileBuf+0x30,size9+pad9+0x200);
	write32(pFileBuf+0x34,ARM7bootAddr);
	write32(pFileBuf+0x38,ARM7bootAddr);
	write32(pFileBuf+0x3c,size7/*+pad7*/);
	write32(pFileBuf+0x80,0x200+size9+pad9+size7+pad7);
	write16(pFileBuf+0x15e,swiCRC16(0xffff,pFileBuf,0x15e));
	memcpy(pFileBuf+0x200,tmp_data9,size9);
#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
	memcpy(pFileBuf+0x200+size9,tmp_data7,size7);
#else
	memcpy(pFileBuf+0x200+size9+pad9,tmp_data7,size7);
#endif

#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
	_consolePrint("Rebooting...\n");
	//free(tmp_data7);
	//free(tmp_data9);
	//installargv(pFileBuf,(char*)0x023ff400,pFilename);
	*(vu32*)0x02fFFDF4=(u32)pFileBuf;
	disc_unmount();
	DC_FlushAll();
	//IPCZ->cmd=ResetRudolph;
	NotifyARM7(ResetRudolph);
	ret_menu9_GENs();
	//bootMoonlight((u32)pFileBuf+0xc0);
#else
	FILE *f=fopen(output,"wb");
	fwrite(pFileBuf,1,0x200+size9+pad9+size7+pad7,f);
	fclose(f);
#endif

	return 0;
}

#if !defined(FEOS) && (defined(ARM9) || defined(ARM7))
int returnDSMenu(){
	if(IPCZ->NDSType>=NDSi){disc_unmount();NotifyARM7(ReturnDSiMenu);while(1);}
	u8 *p=(u8*)malloc(IPCZ->fwsize);
	if(!p)return 3;
	_consolePrint("Using GPL-free decoder.\n");
	_consolePrint("Getting Firmware...\n");
	IPCZ->firmware_addr=p;
	IPCZ->firmware_bufsize=IPCZ->fwsize;
	DC_FlushAll();
	CallARM7(GetFirmware);
	DC_InvalidateAll();
	_consolePrint("Decoding Firmware...\n");
	return load(p);
}
#else
#include <sys/stat.h>
int xenobootfw(const int argc, const char **argv){
	struct stat st;
	if(argc<3){
		fprintf(stderr,"XenoBootFW fw.bin out.nds\n");
		return 1;
	}
	output=(char*)argv[2];
	int i=1;
	for(;i<2;i++){
		FILE *f=fopen(argv[i],"rb");
		if(!f)continue;
		fstat(fileno(f),&st);
		u8 *p=(u8*)malloc(st.st_size);
		if(!p){fclose(f);continue;}
		fread(p,1,st.st_size,f);
		fclose(f);
		//fprintf(stderr,"Processing %s... ",argv[i]);
		load(p);
	}
	return 0;
}
#endif

