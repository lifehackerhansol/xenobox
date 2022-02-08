#include "lzma.h"

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
#include "memstream.h"

#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
	#include <windows.h>
	#include <basetyps.h>
	//broken...
	const IID IID_IUnknown=
	{0x00000000,0x0000,0x0000,
	{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
#else
	#ifndef NODLOPEN //dynamic load
	#ifdef FEOS
		#include <feos.h>
		#undef ARM9
		#define LoadLibraryA(filename) FeOS_LoadModule(filename) 
		#define GetProcAddress FeOS_FindSymbol
		#define FreeLibrary FeOS_FreeModule
	#else
		#include <dlfcn.h>
		#define LoadLibraryA(filename) dlopen(filename,RTLD_NOW)
		#define GetProcAddress dlsym
		#define FreeLibrary dlclose
	#endif
	#endif
	#include "windows.h"
	#include "basetyps.h"
#endif

//GUID
#include "lzma/CPP/Common/MyWindows.h"
#include "lzma/CPP/Common/MyInitGuid.h"
#include "lzma/CPP/Common/MyCom.h"

//Interfaces
#include "lzma/CPP/7zip/Archive/IArchive.h"
#include "lzma/CPP/7zip/ICoder.h"
#include "lzma/CPP/7zip/IStream.h"
#include "lzma/CPP/7zip/IPassword.h"
#include "lzma/CPP/7zip/IProgress.h"

#include "lzma/CPP/Windows/NtCheck.h"

class CSequentialInStream: public ISequentialInStream, public CMyUnknownImp{
protected:
	void *handle;
	tRead pRead;
	tClose pClose;
public:
	MY_UNKNOWN_IMP
	CSequentialInStream(void *_handle, tRead _pRead, tClose _pClose=NULL){
		handle=_handle;
		pRead=_pRead;
		pClose=_pClose;
	}
	~CSequentialInStream(){if(handle&&pClose)pClose(handle);}
	STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize){
		if(!handle||!pRead)return E_FAIL;
		int readlen=pRead(handle,(char*)data,size);
		if(processedSize)*processedSize=readlen;
		return S_OK;
	}
};
#if 0
class CInStream: public IInStream, public CSequentialInStream{
protected:
	tSeek pSeek;
public:
	MY_UNKNOWN_IMP
	CInStream(void *_handle, tRead _pRead, tSeek _pSeek, tClose _pClose=NULL):
		CSequentialInStream(_handle,_pRead,_pClose){
		pSeek=_pSeek;
	}
	STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition){
		if(!handle||!pSeek)return E_FAIL;
		fpos_t pos=pSeek(handle,offset,seekOrigin);
		if(newPosition)*newPosition=pos;
		return S_OK;
	}
	STDMETHOD(SetSize)(UInt64 newSize){return S_OK;}
};
#endif
class CSequentialOutStream: public ISequentialOutStream, public CMyUnknownImp{
protected:
	void *handle;
	tWrite pWrite;
	tClose pClose;
public:
	MY_UNKNOWN_IMP
	CSequentialOutStream(void *_handle, tWrite _pWrite, tClose _pClose=NULL){
		handle=_handle;
		pWrite=_pWrite;
		pClose=_pClose;
	}
	~CSequentialOutStream(){if(handle&&pClose)pClose(handle);}
	STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize){
		if(!handle||!pWrite)return E_FAIL;
		int writelen=pWrite(handle,(char*)data,size);
		if(processedSize)*processedSize=writelen;
		return S_OK;
	}
};
	

typedef HRESULT (WINAPI*funcCreateObject)(const GUID*,const GUID*,void**);
static funcCreateObject pCreateArchiver,pCreateCoder;

#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
	static HMODULE h7z=NULL;
#else
	static void *h7z=NULL;
#endif

int lzmaOpen7z(){
	h7z=NULL;
#ifndef NODLOPEN
#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
	h7z=LoadLibraryA("7z.dll");
#else
	h7z=LoadLibraryA("/usr/lib/p7zip/7z.so"); //Generic
	if(!h7z)h7z=LoadLibraryA("/usr/local/lib/p7zip/7z.so");
	if(!h7z)h7z=LoadLibraryA("/opt/local/lib/p7zip/7z.so"); //MacPorts
	if(!h7z)h7z=LoadLibraryA("/sw/lib/p7zip/7z.so"); //Fink
	if(!h7z)h7z=LoadLibraryA("7z.so");
	if(!h7z)h7z=LoadLibraryA("7z");
#endif
#endif
	if(!h7z)return 1;

#ifndef NODLOPEN
#if defined(WIN32) || (!defined(__GNUC__) && !defined(__clang__))
	pCreateArchiver=(funcCreateObject)GetProcAddress(h7z,"CreateObject");
	pCreateCoder=(funcCreateObject)GetProcAddress(h7z,"CreateObject");
#else
	pCreateArchiver=(funcCreateObject)GetProcAddress(h7z,"CreateArchiver");
	pCreateCoder=(funcCreateObject)GetProcAddress(h7z,"CreateCoder");
#endif
#endif
	if(!pCreateArchiver||!pCreateArchiver){
		FreeLibrary(h7z);h7z=NULL;
		return 2;
	}
	return 0; //now you can call 7z.so.
}
bool lzma7zAlive(){
	return h7z&&pCreateArchiver&&pCreateCoder;
}
int lzmaClose7z(){
	if(!h7z)return 1;
	FreeLibrary(h7z);h7z=NULL;
	return 0;
}

#if 0
//call with CreateArchiver
GUID CLSID_CArchiveHandler=
	{0x23170F69,0x40C1,0x278A,
	{0x10,0x00,0x00,0x01,0x10,/*archive type*/0x07,0x00,0x00}};

//call with CreateCodec
GUID CLSID_CCodec=
	{0x23170F69,0x40C1,/*decoder:0x2790 encoder:0x2791*/0x2791,
	{0x08,0x01,0x04,0x00,0x00,0x00,0x00,0x00}}; //Codec ID in little endian
	//{0x01,0x01,0x03,0x00,0x00,0x00,0x00,0x00}}; //LZMA
	//{0x02,0x02,0x04,0x00,0x00,0x00,0x00,0x00}}; //BZip2
#endif

int lzmaGUIDSetArchiver(void *g,unsigned char arctype){
	if(!g)return 1;
	GUID *guid=(GUID*)g;
	guid->Data1=0x23170F69;
	guid->Data2=0x40C1;
	guid->Data3=0x278A;
	guid->Data4[0]=0x10;
	guid->Data4[1]=0x00;
	guid->Data4[2]=0x00;
	guid->Data4[3]=0x01;
	guid->Data4[4]=0x10;
	guid->Data4[5]=arctype;
	guid->Data4[6]=0x00;
	guid->Data4[7]=0x00;
	return 0;
}

int lzmaGUIDSetCoder(void *g,unsigned long long int codecid,int encode){
	if(!g||(encode!=0&&encode!=1))return 1;
	GUID *guid=(GUID*)g;
	guid->Data1=0x23170F69;
	guid->Data2=0x40C1;
	guid->Data3=0x2790|encode;
	int i=0;
	for(;i<8;i++,codecid>>=8)guid->Data4[i]=codecid&0xff;
	return 0;
}

int lzmaCreateArchiver(void **archiver,unsigned char arctype,int encode,int level){
	if(!h7z||!archiver)return -1;
	GUID CLSID_CArchiveHandler;
	lzmaGUIDSetArchiver(&CLSID_CArchiveHandler,arctype);
	if(pCreateArchiver(&CLSID_CArchiveHandler, encode?&IID_IOutArchive:&IID_IInArchive, archiver)!=S_OK){*archiver=NULL;return 1;}
	return 0;
}
int lzmaDestroyArchiver(void **archiver,int encode){
	if(!h7z||!archiver)return -1;
	if(encode)(*(IOutArchive**)archiver)->Release();
	else (*(IInArchive**)archiver)->Release();
	*archiver=NULL;
	return 0;
}
#if 0
int lzmaMakeReader(void **_reader,void *h,tRead pRead,tSeek pSeek,tClose pClose){
	if(!_reader)return 1;
	*_reader=new CInStream(h,pRead,pSeek,pClose);
	return 0;
}
int lzmaDestroyReader(void **reader){
	if(!h7z||!reader)return -1;
	delete (*(CInStream**)reader);
	*reader=NULL;
	return 0;
}
int lzmaOpenArchive(void *archiver,void *reader){
	return ((IInArchive*)archiver)->Open((CInStream*)reader,NULL,NULL);
}
#endif
int lzmaCreateCoder(void **coder,unsigned long long int id,int encode,int level){
	if(!h7z||!coder)return -1;
	GUID CLSID_CCodec;
	lzmaGUIDSetCoder(&CLSID_CCodec,id,encode);
	if(pCreateCoder(&CLSID_CCodec, &IID_ICompressCoder, coder)!=S_OK){*coder=NULL;return 1;}
	if(!encode)return 0;

	ICompressSetCoderProperties *coderprop;
	(*(ICompressCoder**)coder)->QueryInterface(IID_ICompressSetCoderProperties, (void**)&coderprop);
	if(id==0x040108||id==0x040109){
		//todo other than Deflate
		PROPID ID[]={NCoderPropID::kNumPasses,NCoderPropID::kNumFastBytes};
		PROPVARIANT VAR[]={{VT_UI4,0,0,0,{0}},{VT_UI4,0,0,0,{0}}};
		switch(level){
			case 9: VAR[0].ulVal=10,VAR[1].ulVal=255;break;
			case 8: VAR[0].ulVal=8,VAR[1].ulVal=255;break;
			case 7: VAR[0].ulVal=6,VAR[1].ulVal=128;break;
			case 6: VAR[0].ulVal=5,VAR[1].ulVal=128;break;
			case 5: VAR[0].ulVal=4,VAR[1].ulVal=128;break;
			case 4: VAR[0].ulVal=3,VAR[1].ulVal=64;break;
			case 3: VAR[0].ulVal=2,VAR[1].ulVal=64;break;
			case 2: VAR[0].ulVal=2,VAR[1].ulVal=32;break;
			case 1: VAR[0].ulVal=1,VAR[1].ulVal=32;break;
		}
		coderprop->SetCoderProperties(ID,VAR,2);
	}
	return 0;
}
int lzmaDestroyCoder(void **coder){
	if(!h7z||!coder)return -1;
	(*(ICompressCoder**)coder)->Release();
	*coder=NULL;
	return 0;
}

static int mread2(void *h, char *p, int n){return mread(p,n,(memstream*)h);}
static int mwrite2(void *h, const char *p, int n){return mwrite(p,n,(memstream*)h);}
static int mclose2(void *h){return 0;}
int lzmaCodeOneshot(void *coder, unsigned char *in, size_t isize, unsigned char *out, size_t *osize){
		if(!h7z||!in||!out||!osize||!*osize)return -1;
		//unsigned long long int dummy=0;
		memstream min;
		mopen(in,isize,&min);
		CMyComPtr<CSequentialInStream> sin=new CSequentialInStream(&min,mread2);
		memstream mout;
		mopen(out,*osize,&mout);
		CMyComPtr<CSequentialOutStream> sout=new CSequentialOutStream(&mout,mwrite2,mclose2);
		HRESULT r=((ICompressCoder*)coder)->Code(sin, sout, (UINT64*)&isize, (UINT64*)osize, NULL);
		if(r!=S_OK)return r;
		*osize=mtell(&mout);
		return 0;
}
int lzmaCodeCallback(void *coder, void *hin, tRead pRead_in, tClose pClose_in, void *hout, tWrite pWrite_out, tClose pClose_out){
//decoder isn't working here...
		if(!h7z||!hin||!hout)return -1;
		unsigned long long int isize=0xffffffffUL,osize=0xffffffffUL; //lol
		CMyComPtr<CSequentialInStream> sin=new CSequentialInStream(hin,pRead_in,pClose_in);
		CMyComPtr<CSequentialOutStream> sout=new CSequentialOutStream(hout,pWrite_out,pClose_out);
		HRESULT r=((ICompressCoder*)coder)->Code(sin, sout, (UINT64*)&isize, (UINT64*)osize, NULL);
		if(r!=S_OK)return r;
		return 0;
}
