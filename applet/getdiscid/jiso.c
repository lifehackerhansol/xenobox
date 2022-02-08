typedef struct
{
	unsigned char magic[4];			// +0x00 : 'J','I','S','O'
	unsigned short version;			// +0x04 : 0001
	unsigned short compression_block_size;	// +0x06 : uncompressed block size in byte
	unsigned short compression_block_header;// +0x08 : block header size in byte (not used, keeped for legacy support)
	unsigned short compression_algorithm;	// +0x0A : compression algorithm (lzo / zlib)
	unsigned int  uncompressed_size;	// +0x0C : original iso file length in byte
	unsigned char md5_digest[16];		// +0x10 : md5 16-byte long digest
	unsigned int  index_offset;		// +0x20 : file offset to index block
	unsigned short nNCareas;		// +0x24 : number of non-compressed areas
	unsigned short reserved_short;		// +0x26 : reserved for futur use
	unsigned int  NCareas_offset;		// +0x28 : file offset to NCArea block
	unsigned int  reserved_long;		// +0x2C : reserved for futur use
} JISO_FH;					// +0x30 : end of header
#define JISO_HEADER_SIZE (0x30)

int jso_read_fp(char *buf, SceUID fp, int pos, int size);
int lzo1x_decompress_oneshot(char* o_buff, int o_size, const char* i_buff, int i_size);

/*---------------------------------------------------------------------------
  CSOから読込む
    char *buf        読込みバッファ
    const char *path パス
    int pos          読込み位置
    int size         読込みサイズ

    返値 実際に読み込んだ長さ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int jso_read(char *buf, const char *path, int pos, int size)
{
  SceUID fp;
  int ret;
  //int err;

  fp = sceIoOpen(path, PSP_O_RDONLY, 0777);

  if(fp < 0)
    return ERR_OPEN;

  ret = jso_read_fp(buf, fp, pos, size);

  /*err =*/ sceIoClose(fp);

  if(fp < 0)
    return ERR_CLOSE;

  return ret;
}

/*---------------------------------------------------------------------------
  CSOから連続で読込む
    char *buf 読込みバッファ
    SceUID fp ファイルポインタ
    int pos   読込み位置
    int size  読込みサイズ

    返値 実際に読み込んだ長さ / エラーの場合は負を返す

    事前にsceIoOpen / 終了後にsceIoCloseが必要
---------------------------------------------------------------------------*/
int jso_read_fp(char *buf, SceUID fp, int pos, int size)
{
  static SceUID old_fp = 0;
  static JISO_FH header;
  int start_sec;
  int max_sector;
  int end_sec;
  int sector_num;
  unsigned long long int now_pos = 0;
  unsigned long long int next_pos = 0;
  int read_size;
  unsigned int zip_flag;
  //char tmp_buf[65536];   // 展開済みデータバッファ
  //char tmp_buf_2[65536 * 2]; // 圧縮データ読み込みバッファ
  int ret;
  int err;
  int start_pos;
  int end_pos;

  // ヘッダー読込
  if(old_fp != fp)
  {
    err = sceIoLseek32(fp, 0, PSP_SEEK_SET);
    if(err < 0)
      return ERR_SEEK;

    err = sceIoRead(fp, &header, JISO_HEADER_SIZE);
    if(err < 0)
      return ERR_READ;

	if(header.nNCareas)return ERR_READ; //NC isn't supported.
	if(header.compression_block_header)return ERR_READ; //compression_block_header isn't supported. anyway it is legacy.

    old_fp = fp;
  }

  // 読込セクタ数を計算
  if((pos + size) > header.uncompressed_size)
    size = header.uncompressed_size - pos;

  max_sector = header.uncompressed_size / header.compression_block_size - 1;
  start_sec = pos / header.compression_block_size;
  end_sec = (pos + size - 1) / header.compression_block_size;
  sector_num = start_sec;

  if(sector_num > max_sector)
    return ERR_SEEK;

  if(end_sec > max_sector)
    end_sec = max_sector;

  ret = 0;
  for(;sector_num <= end_sec;sector_num++)
  {
    // セクタ番号からファイル位置と長さを取得
    err = sceIoLseek32(fp, JISO_HEADER_SIZE + (sector_num * 4), PSP_SEEK_SET);
    if(err < 0)
      return ERR_SEEK;

    err = sceIoRead(fp, &now_pos, 4);
    if(err < 0)
      return ERR_READ;

    zip_flag = 0;//now_pos & 0x80000000;
    now_pos = (now_pos & 0x7fffffff) << 0;//header.align;

    err = sceIoRead(fp, &next_pos, 4);
    if(err < 0)
      return ERR_READ;

    read_size = ((next_pos & 0x7fffffff) << 0/*header.align*/) - now_pos;

    // １セクタを読込
    err = sceIoLseek32(fp, now_pos, PSP_SEEK_SET);
    if(err < 0)
      return ERR_SEEK;

    if(zip_flag != 0)
    {
      // 未圧縮
      err = sceIoRead(fp, __decompbuf, header.compression_block_size);
      if(err < 0)
        return ERR_READ;
    }
    else
    {
      // 圧縮済
      err = sceIoRead(fp, __compbuf, read_size);
      if(err < 0)
        return ERR_READ;
      // バッファに展開
		if(header.compression_algorithm)
			err = inflate_cso(__decompbuf, header.compression_block_size, __compbuf, read_size);
		else
			err = lzo1x_decompress_oneshot(__decompbuf, header.compression_block_size, __compbuf, read_size);
      if(err < 0)
        return ERR_INFLATE;
    }

    // 指定バッファに転送
    if((sector_num > start_sec) && (sector_num < end_sec))
    {
      // 全転送
      memcpy(buf, __decompbuf, header.compression_block_size);
      read_size = header.compression_block_size;
    }
    else if((sector_num == start_sec) || (sector_num == end_sec))
    {
      // 部分転送
      start_pos = 0;
      end_pos = header.compression_block_size;
      if(sector_num == start_sec)
        start_pos = pos - (start_sec * header.compression_block_size);
      if(sector_num == end_sec)
        end_pos = (pos + size) - (end_sec * header.compression_block_size);
      read_size = end_pos - start_pos;
      memcpy(buf, &__decompbuf[start_pos], read_size);
    }

    buf += read_size;
    ret += read_size;
  }

  return ret;
}
