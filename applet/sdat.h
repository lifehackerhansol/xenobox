typedef struct tagSDATHeader{
	struct tagNdsStdFile {
		u8  type[4];   // 'SDAT'
		u32 magic;	// 0x0100feff
		u32 nFileSize;
		u16 nSize;
		u16 nBlock;    // usually 4, but some have 3 only ( Symbol Block omitted )
	} file;
	u32 nSymbOffset;  	// offset of Symbol Block = 0x40
	u32 nSymbSize;    	// size of Symbol Block
	u32 nInfoOffset; 	// offset of Info Block
	u32 nInfoSize;    	// size of Info Block
	u32 nFatOffset;   	// offset of FAT
	u32 nFatSize;     	// size of FAT
	u32 nFileOffset; 	// offset of File Block
	u32 nFileSize;   	// size of File Block
	u8  reserved[16]; 	// unused, 0s
} SDATHEADER;

typedef struct tagSDATSymbol{
	char type[4];		// 'SYMB'
	u32 nSize;		// size of this Symbol Block
	u32 nRecOffset[8];	// offset of Records (note below)
	u8  reserved[24];	// unused, 0s
} SDATSYMB;

typedef struct tagSDATSymbolRec{
	u32 nCount;		// No of entries in this record
	u32 nEntryOffset[1];	// Array of offsets of each entry
} SDATSYMBREC;

typedef struct tagSDATInfo{
	char type[4];           // 'INFO'
	u32 nSize;             // size of this Info Block
	u32 nRecOffset[8];     // offset of a Record
	u8  reserved[24];       // unused, 0s
} SDATINFO;

typedef struct tagSDATInfoRec{
	u32 nCount;            // No of entries in this record
	u32 nEntryOffset[1];   // array of offsets of each entry
} SDATINFOREC;

typedef struct tagSDATInfoSseq{
	u16 fileID;	// for accessing this file
	u16 unknown;
	u16 bnk;	// Associated BANK
	u8  vol;	// Volume
	u8  cpr;
	u8  ppr;
	u8  ply;
	u8  unknown2[2];
} SDATINFOSSEQ;

typedef struct tagSDATInfoSsar{
	u16 fileID;
	u16 unknown;
} SDATINFOSSAR;

typedef struct tagSDATInfoBank{
	u16 fileID;
	u16 unknown;
	u16 wa[4];      // Associated WAVEARC. 0xffff if not in use
} SDATINFOSBNK;

typedef struct tagSDATInfoSwar{
	u16 fileID;
	u16 unknown;
} SDATINFOSWAR;

typedef struct tagSDATInfoPlayer{
	u8  unknown;
	u8  padding[3];
	u32 unknown2;
} SDATINFOPLAYER;

typedef struct tagSDATInfoGroup{
	u32 nCount;		// number of sub-records
        struct {		// array of Group
		u32 type;
		u32 nEntry;
	} Group[1];
} SDATINFOGROUP;

typedef struct SDATInfoPlayer2{
	u8  nCount;
	u8  v[16];		// 0xff if not in use
	u8  reserved[7];	// padding, 0s
} SDATINFOPLAYER2;

typedef struct SDATInfoStrm{
	u16 fileID;		// for accessing the file
	u16 unknown;
	u8  vol;		// volume
	u8  pri;
	u8  ply;
	u8  reserved[5];
} SDATINFOSTRM;

typedef struct tagSDATFATREC{
	u32 nOffset;		// offset of the sound file
	u32 nSize;		// size of the Sound file
	u32 reserved[2];	// always 0s, for storing data in runtime.
} SDATFATREC;

typedef struct tagSDATFAT{
	char type[4];		// 'FAT '
	u32 nSize;		// size of the FAT
	u32 nCount;		// Number of FAT records
	SDATFATREC Rec[1];	// Arrays of FAT records
} SDATFAT;

typedef struct tagSDATFILE{
	char type[4];  // 'FILE'
	u32 nSize;    // size of this block
	u32 nCount;   // Mumber of sound files
	u32 reserved; // always 0
} SDATFILE;
