typedef struct
{
    unsigned char magic[4];   /* +00 : 'D','A','X', 0                            */
    unsigned int total_bytes; /* +04 : Original data size                        */
    unsigned int ver;         /* +08 : Version 0x00000001                        */
    unsigned int nNCareas;    /* +12 : Number of non-compressed areas            */
    unsigned int reserved[4]; /* +16 : Reserved for future uses                  */
} DAX_H;
#define DAX_HEADER_SIZE (0x20)
#define DAX_BLOCK_SIZE  (8192)

/*---------------------------------------------------------------------------
  deflateの解凍を行う
    char* o_buff 解凍先
    int o_size   解凍先バッファサイズ
    char* i_buff 入力
    int i_size   入力サイズ

    返値 解凍後のサイズ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int inflate_dax(char* o_buff, int o_size, const char* i_buff, int i_size)
{
    z_stream z;
    int size;

    // 初期化
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.opaque = Z_NULL;
    z.next_in = Z_NULL;
    z.avail_in = 0;
    // if(inflateInit2(&z, -15) != Z_OK) //initializer not compat with cso
    if (inflateInit(&z) != Z_OK)
        return ERR_INFLATE;

    z.next_in = (unsigned char*)i_buff;
    z.avail_in = i_size;
    z.next_out = (unsigned char*)o_buff;
    z.avail_out = o_size;

    inflate(&z, Z_FINISH);

    // 出力サイズ
    size = o_size - z.avail_out;

    if (inflateEnd(&z) != Z_OK)
        return ERR_INFLATE;

    return size;
}

int dax_read_fp(char* buf, SceUID fp, int pos, int size);

/*---------------------------------------------------------------------------
  CSOから読込む
    char *buf        読込みバッファ
    const char *path パス
    int pos          読込み位置
    int size         読込みサイズ

    返値 実際に読み込んだ長さ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int dax_read(char* buf, const char* path, int pos, int size)
{
    SceUID fp;
    int ret;
    // int err;

    fp = sceIoOpen(path, PSP_O_RDONLY, 0777);

    if (fp < 0)
        return ERR_OPEN;

    ret = dax_read_fp(buf, fp, pos, size);

    /*err =*/sceIoClose(fp);

    if (fp < 0)
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
int dax_read_fp(char* buf, SceUID fp, int pos, int size)
{
    static SceUID old_fp = 0;
    static DAX_H header;
    int start_sec;
    int max_sector;
    int end_sec;
    int sector_num;
    unsigned long long int now_pos = 0;
    unsigned long long int next_pos = 0;
    int read_size;
    unsigned int zip_flag;
    // char tmp_buf[DAX_BLOCK_SIZE];   // 展開済みデータバッファ
    // char tmp_buf_2[DAX_BLOCK_SIZE * 2]; // 圧縮データ読み込みバッファ
    int ret;
    int err;
    int start_pos;
    int end_pos;

    // ヘッダー読込
    if (old_fp != fp)
    {
        err = sceIoLseek32(fp, 0, PSP_SEEK_SET);
        if (err < 0)
            return ERR_SEEK;

        err = sceIoRead(fp, &header, DAX_HEADER_SIZE);
        if (err < 0)
            return ERR_READ;

        if (header.nNCareas)
            return ERR_READ; // per file NC isn't supported bah...

        old_fp = fp;
    }

    // max_sectors = (int)(ciso.total_bytes) / ciso.block_size; // -> NCarea(?)

    // 読込セクタ数を計算
    if ((pos + size) > header.total_bytes)
        size = header.total_bytes - pos;

    max_sector = header.total_bytes / DAX_BLOCK_SIZE - 1;
    start_sec = pos / DAX_BLOCK_SIZE;
    end_sec = (pos + size - 1) / DAX_BLOCK_SIZE;
    sector_num = start_sec;

    if (sector_num > max_sector)
        return ERR_SEEK;

    if (end_sec > max_sector)
        end_sec = max_sector;

    ret = 0;
    while (sector_num <= end_sec)
    {
        // セクタ番号からファイル位置と長さを取得
        err = sceIoLseek32(fp, DAX_HEADER_SIZE + (sector_num * 4), PSP_SEEK_SET);
        if (err < 0)
            return ERR_SEEK;

        err = sceIoRead(fp, &now_pos, 4);
        if (err < 0)
            return ERR_READ;

        zip_flag = 0; // now_pos & 0x80000000;
        // now_pos = (now_pos & 0x7fffffff) << 0;//header.align;

        err = sceIoRead(fp, &next_pos, 4);
        if (err < 0)
            return ERR_READ;
        // read_size = ((next_pos & 0x7fffffff) << 0/*header.align*/) - now_pos;
        read_size = next_pos - now_pos;

#if 0
    err = sceIoLseek32(fp, DAX_HEADER_SIZE + ((max_sector+2) * 4) + (sector_num * 2), PSP_SEEK_SET);
    if(err < 0)
      return ERR_SEEK;

	read_size=0;
    err = sceIoRead(fp, &read_size, 2);
    if(err < 0)
      return ERR_READ;
#endif

        // １セクタを読込
        err = sceIoLseek32(fp, now_pos, PSP_SEEK_SET);
        if (err < 0)
            return ERR_SEEK;

        if (zip_flag != 0)
        {
            // 未圧縮
            err = sceIoRead(fp, __decompbuf, DAX_BLOCK_SIZE);
            if (err < 0)
                return ERR_READ;
        }
        else
        {
            // 圧縮済
            err = sceIoRead(fp, __compbuf, read_size);
            if (err < 0)
                return ERR_READ;
            // バッファに展開
            err = inflate_dax(__decompbuf, DAX_BLOCK_SIZE, __compbuf, read_size);
            if (err < 0)
                return ERR_INFLATE;
        }

        // 指定バッファに転送
        if ((sector_num > start_sec) && (sector_num < end_sec))
        {
            // 全転送
            memcpy(buf, __decompbuf, DAX_BLOCK_SIZE);
            read_size = DAX_BLOCK_SIZE;
        }
        else if ((sector_num == start_sec) || (sector_num == end_sec))
        {
            // 部分転送
            start_pos = 0;
            end_pos = DAX_BLOCK_SIZE;
            if (sector_num == start_sec)
                start_pos = pos - (start_sec * DAX_BLOCK_SIZE);
            if (sector_num == end_sec)
                end_pos = (pos + size) - (end_sec * DAX_BLOCK_SIZE);
            read_size = end_pos - start_pos;
            memcpy(buf, &__decompbuf[start_pos], read_size);
        }

        buf += read_size;
        ret += read_size;
        sector_num++;
    }

    return ret;
}
