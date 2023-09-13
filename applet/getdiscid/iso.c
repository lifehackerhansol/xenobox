/*
 * iso.c
 *
 *  Created on: 2009/12/29
 *      Author: takka
 */

#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "file.h"
#include "iso.h"
#include "error.h"

/*---------------------------------------------------------------------------
  ISO/CSOからファイルを読込む
  void *buf             : 読込みバッファ
  int max_buf           : 最大バッファサイズ
  const char* iso_path  : ISOのパス
  file_type type        : ファイルタイプ(TYPE_ISO/TYPE_CSOのみ)
  const char* file_path : 読込むファイルのパス

  return int       : 読込みしたサイズ
---------------------------------------------------------------------------*/
int iso_read(void* buf, int max_buf, const char* iso_path, file_type type, const char* file_path)
{

    iso_file_stat stat;
    int ret;

    ret = iso_get_file_info(&stat, iso_path, type, file_path);
    if (ret < 0)
        return ret;
    else if (stat.size > max_buf)
        return ERR_BUF_SIZE_OVER;

    ret = file_read(buf, iso_path, type, stat.pos, stat.size);

    return ret;
}

int iso_write(void* buf, int size, const char* path, file_type type, const char* file)
{
    iso_file_stat stat;
    int ret;
    unsigned char* endian;
    char prx_data[8];

    ret = iso_get_file_info(&stat, path, type, file);
    if (ret < 0)
        return ret;

    endian = (unsigned char*)&size;
    prx_data[0] = endian[0];
    prx_data[1] = endian[1];
    prx_data[2] = endian[2];
    prx_data[3] = endian[3];
    prx_data[4] = endian[3];
    prx_data[5] = endian[2];
    prx_data[6] = endian[1];
    prx_data[7] = endian[0];
    ret = file_write(prx_data, path, type, stat.size_pos, 8, 0);
    if (ret < 0)
        return ret;

    ret = file_write(buf, path, type, stat.pos, size, 0);

    return ret;
}

int iso_file_add(char* iso_dir, char* iso_name, char* dir_path, char* file_dir, char* file_name)
{
    char iso_path[MAX_PATH_LEN];
    char file_path[MAX_PATH_LEN];
    char write_path[MAX_PATH_LEN];
    SceIoStat file_stat;
    SceIoStat iso_stat;
    iso_file_stat boot_st;
    iso_file_stat tbl_st;
    iso_file_stat file_st;
    //  int boot_pos, boot_size, boot_size_pos;
    //  int tbl_pos, tbl_size, tbl_size_pos;
    //  int file_pos, file_size, file_size_pos
    int file_sector;

    char* read_ptr = &WORK[CRYPT_DATA * SECTOR_SIZE];

    int ret;
    int use_boot_bin_flag = 1;
    unsigned char* endian;
    int len;

    /* tbl_data = tbl_data_1 + file name + tbl_data_2 */
    char tbl_data[MAX_PATH_LEN];

    char tbl_data_1[0x21] = {
        0x2F, /* total number */
        0x00, 0xFF, 0xEE, 0x00, 0x00, 0x00, 0x00, 0xEE, 0xFF, 0xB6, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x0D, 0xB6,
        0x6D, 0x0C, 0x0E, 0x17, 0x29, 0x00, 0x24, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, /* file name number */
    };

    char tbl_data_2[0x0e] = {
        0x00, 0x00, 0x00, 0x00, 0x0D, 0x55, 0x58, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    char boot_data[8];
    int ptr;

    strcpy(iso_path, iso_dir);
    strcat(iso_path, iso_name);

    strcpy(file_path, file_dir);
    strcat(file_path, file_name);

    strcpy(write_path, dir_path);
    if (dir_path[strlen(dir_path)] != '/')
        strcat(write_path, "/");
    strcat(write_path, file_name);

    // ISO/書込みファイルのデータ取得
    ret = sceIoGetstat(iso_path, &iso_stat);
    ret = sceIoGetstat(file_path, &file_stat);
    file_st.size = file_stat.st_size;

    // BOOT.BINのデータ取得
    ret = iso_get_file_info(&boot_st, iso_path, TYPE_ISO, "PSP_GAME/SYSDIR/BOOT.BIN");

    if ((file_st.size >> 11) > (boot_st.size >> 11))
        use_boot_bin_flag = 0;

    // 書込みセクタ数
    file_sector = (file_st.size + (SECTOR_SIZE - 1)) / SECTOR_SIZE;

    if (use_boot_bin_flag == 1)
    {
        file_st.pos = boot_st.pos / SECTOR_SIZE;
        // BOOT.BINのサイズ/位置を調整
        boot_st.size = boot_st.size - file_sector * SECTOR_SIZE;
        boot_st.pos = boot_st.pos / SECTOR_SIZE + file_sector;

        // BOOT.BINのパステーブル変更
        endian = (unsigned char*)&boot_st.pos;
        boot_data[0] = boot_data[7] = endian[0];
        boot_data[1] = boot_data[6] = endian[1];
        boot_data[2] = boot_data[5] = endian[2];
        boot_data[3] = boot_data[4] = endian[3];
        ret = file_write(boot_data, iso_path, TYPE_ISO, boot_st.size_pos - 8, 8, 0);

        endian = (unsigned char*)&boot_st.size;
        boot_data[0] = boot_data[7] = endian[0];
        boot_data[1] = boot_data[6] = endian[1];
        boot_data[2] = boot_data[5] = endian[2];
        boot_data[3] = boot_data[4] = endian[3];
        ret = file_write(boot_data, iso_path, TYPE_ISO, boot_st.size_pos, 8, 0);
    }
    else
    {
        // PVDのISOサイズの変更
        ret = file_read(&file_st.pos, iso_path, TYPE_ISO, 0x8050, 4);

        boot_st.size = file_st.pos + file_sector;
        endian = (unsigned char*)&boot_st.size;
        boot_data[0] = boot_data[7] = endian[0];
        boot_data[1] = boot_data[6] = endian[1];
        boot_data[2] = boot_data[5] = endian[2];
        boot_data[3] = boot_data[4] = endian[3];
        ret = file_write(boot_data, iso_path, TYPE_ISO, 0x8050, 8, 0);
    }

    // パステーブルの処理
    ret = iso_get_file_info(&tbl_st, iso_path, TYPE_ISO, write_path);

    if (ret < 0)
    {
        // 新規ファイルの場合
        // パステーブルデータの作成
        len = strlen(file_name);
        tbl_data_1[0x20] = (u8)len;
        memcpy(tbl_data, tbl_data_1, sizeof(tbl_data_1));
        memcpy(tbl_data + sizeof(tbl_data_1), file_name, len);

        if ((len & 1) == 0)
        {
            memset(tbl_data + sizeof(tbl_data_1) + len, 0, 1);
            len++;
        }
        memcpy(tbl_data + sizeof(tbl_data_1) + len, tbl_data_2, sizeof(tbl_data_2));

        tbl_data[0x0] = 0x2f + (u8)len;

        endian = (unsigned char*)&file_st.pos;
        tbl_data[0x02] = tbl_data[0x09] = endian[0];
        tbl_data[0x03] = tbl_data[0x08] = endian[1];
        tbl_data[0x04] = tbl_data[0x07] = endian[2];
        tbl_data[0x05] = tbl_data[0x06] = endian[3];

        endian = (unsigned char*)&file_st.size;
        tbl_data[0x0a] = tbl_data[0x11] = endian[0];
        tbl_data[0x0b] = tbl_data[0x10] = endian[1];
        tbl_data[0x0c] = tbl_data[0x0f] = endian[2];
        tbl_data[0x0d] = tbl_data[0x0e] = endian[3];

        ret = iso_get_file_info(&tbl_st, iso_path, TYPE_ISO, dir_path);

        ret = file_read(read_ptr, iso_path, TYPE_ISO, tbl_st.pos, tbl_st.size);
        // TODO:セクタをオーバーした場合の処理

        // テーブルの最後まで移動
        ptr = 0;
        while (read_ptr[ptr] != 0)
            ptr += read_ptr[ptr];

        memcpy(&read_ptr[ptr], tbl_data, tbl_data[0x0]);
        ret = file_write(read_ptr, iso_path, TYPE_ISO, tbl_st.pos, tbl_st.size, 0);
    }
    else
    {
        // 同じファイルがあった場合は場所/サイズのみ書換え
        endian = (unsigned char*)&file_st.pos;
        boot_data[0] = boot_data[7] = endian[0];
        boot_data[1] = boot_data[6] = endian[1];
        boot_data[2] = boot_data[5] = endian[2];
        boot_data[3] = boot_data[4] = endian[3];
        ret = file_write(boot_data, iso_path, TYPE_ISO, tbl_st.size_pos - 8, 8, 0);

        endian = (unsigned char*)&file_st.size;
        boot_data[0] = boot_data[7] = endian[0];
        boot_data[1] = boot_data[6] = endian[1];
        boot_data[2] = boot_data[5] = endian[2];
        boot_data[3] = boot_data[4] = endian[3];
        ret = file_write(boot_data, iso_path, TYPE_ISO, tbl_st.size_pos, 8, 0);
    }

    // ファイル本体の書込み
    memset(read_ptr, 0, file_sector * SECTOR_SIZE);
    ret = ms_read(read_ptr, file_path, 0, 0);
    ret = file_write(read_ptr, iso_path, TYPE_ISO, file_st.pos * SECTOR_SIZE, file_sector * SECTOR_SIZE, 0);

    return 0;
}

int iso_file_del(char* iso_dir, char* iso_name, char* dir_path, char* file_dir, char* file_name)
{

    return 0;
}

int iso_read_dir(dir_t dir[], const char* iso_path, const char* dir_path, int read_dir_flag)
{
    iso_file_stat tbl_stat;

    char* buf = WORK;

    int ret;
    int ptr;
    int num;

    dir[0].num = -1;

    // パステーブルの処理
    num = 0;
    ret = iso_get_file_info(&tbl_stat, iso_path, TYPE_ISO, dir_path);
    if (ret < 0)
        return -1;

    ret = file_read(buf, iso_path, TYPE_ISO, tbl_stat.pos, tbl_stat.size);

    if (ret != tbl_stat.size)
        return -1;

    ptr = 0;
    while (ptr < tbl_stat.size)
    {
        if (buf[ptr] == 0)
            ptr++;
        else
        {
            if (buf[ptr + 0x21] > 0x1f)
            {
                memcpy(dir[num].name, &buf[ptr + 0x21], buf[ptr + 0x20]);
                dir[num].name[(int)buf[ptr + 0x20]] = '\0';
                dir[num].type = TYPE_ETC;
                num++;
            }
            ptr += buf[ptr];
        }
    }

    dir[0].num = num;
    return num;
}

int iso_get_file_info(iso_file_stat* stat, const char* path, file_type type, const char* name)
{
    char* ptr;
    char work[MAX_PATH_LEN];
    char s_path[MAX_PATH_LEN];
    char s_file[MAX_PATH_LEN];
    int path_table_addr;
    int path_table_size;
    int dir_recode_addr;
    char* table_buf; // パステーブルバッファ
    char* dir_buf;   // ディレクトリテーブルバッファ
    short int befor_dir_num = 0x0001;
    int now_dir_num = 1;
    int tbl_ptr;
    int dir_ptr;
    unsigned char len_di;
    unsigned char len_dr;
    int ret;

    memset(stat, 0, sizeof(iso_file_stat));

    dir_recode_addr = 0;

    if (*name == '/')
        name++;

    strcpy(work, name);

    // nameのパスとファイル名を分離
    ptr = strrchr(work, '/');
    if (ptr != NULL)
    {
        *ptr++ = '\0';
        strcpy(s_path, work);
    }
    else
    {
        s_path[0] = '\0';
        ptr = (char*)work;
    }

    // ファイル名をコピー
    strcpy(s_file, ptr);

    // パステーブルの場所/サイズを読込
    file_read(&path_table_size, path, type, 0x8084, 4);
    file_read(&path_table_addr, path, type, 0x808c, 4);
    path_table_addr *= SECTOR_SIZE;
    table_buf = malloc(path_table_size);
    // if(table_buf < 0)
    if (!table_buf)
        return ERR_NO_MEMORY;

    // パステーブルをワークエリアに読込
    ret = file_read(table_buf, path, type, path_table_addr, path_table_size);

    //  オフセット 型             内容
    //  0          uint8          ディレクトリ識別子の長さ(LEN_DI) ルートは01
    //  1          uint8          拡張属性レコードの長さ
    //  2～5       uint32le/be(*) エクステントの位置
    //  6～7       uint16le/be(*) 親ディレクトリの番号 ルートは01(自分自身)
    //  8～        fileid(LEN_DI) ディレクトリ識別子
    //  8+LEN_DI   uint8          padding / LEN_DIが奇数の場合のみ

    if (s_path[0] == '\0')
    {
        // 探すファイルがルートにある場合
        bin2int(&dir_recode_addr, &table_buf[2]);
    }
    else
    {
        // パステーブルからディレクトリレコードの場所を調べる
        befor_dir_num = 0x0001;

        tbl_ptr = 0;
        now_dir_num = 0x0001;
        ptr = s_path;

        while (tbl_ptr < path_table_size)
        {
            len_di = (unsigned char)table_buf[tbl_ptr];
            if (len_di == 0)
                break;

            // 親ディレクトリの番号を比較
            tbl_ptr += 6;

            if (befor_dir_num == *(short int*)&table_buf[tbl_ptr])
            {
                tbl_ptr += 2;

                // ディレクトリ名が一致するか確認
                if (strncasecmp(&table_buf[tbl_ptr], ptr, len_di) == 0)
                {
                    // befor_dir_numを更新
                    befor_dir_num = now_dir_num;
                    ptr = strchr(ptr, '/');
                    if (ptr != NULL)
                        ptr++;
                    else
                    {
                        bin2int(&dir_recode_addr, &table_buf[tbl_ptr - 6]);
                        break;
                    }
                }
            }
            else
            {
                tbl_ptr += 2;
            }

            tbl_ptr += (len_di + 1) & ~1; // padding
            now_dir_num++;
        }
    }

    free(table_buf);

    if (dir_recode_addr == 0)
        return ERR_NOT_FOUND;

    // エクステントの位置等を調べる

    //  オフセット 型             内容
    //  0          uint8          ディレクトリレコードの長さ(LEN_DR)
    //  1          uint8          拡張属性レコードの長さ
    //  2～9       uint32both     エクステントの位置
    //  10～17     uint32both     データ長
    //  18～24     datetime_s     記録日付及び時刻
    //  25         8ビット        ファイルフラグ
    //  26         uint8          ファイルユニットの大きさ
    //  27         uint8          インタリーブ間隙の大きさ
    //  28～31     uint16both     ボリューム順序番号
    //  32         uint8          ファイル識別子の長さ(LEN_FI)
    //  33～       fileid(LEN_FI) ファイル/ディレクトリ識別子
    //  33+LEN_FI  uint8          padding / LEN_FIが偶数の場合のみ

    dir_recode_addr *= SECTOR_SIZE;

    int dir_record_size;

    ret = file_read(&dir_record_size, path, type, dir_recode_addr + 10, 4);

    dir_buf = malloc(dir_record_size);
    // if(dir_buf < 0)
    if (!dir_buf)
        return ERR_NO_MEMORY;

    dir_ptr = 0;
    ret = ERR_NOT_FOUND;

    file_read(dir_buf, path, type, dir_recode_addr, dir_record_size);

    while (dir_ptr < dir_record_size)
    {
        len_dr = (unsigned char)dir_buf[dir_ptr];
        if (len_dr == 0)
            dir_ptr++;
        else
        {
            // ファイル名が一致するか確認
            if (strncasecmp(&dir_buf[dir_ptr + 33], s_file, dir_buf[dir_ptr + 32]) == 0)
            {
                bin2int(&stat->pos, &dir_buf[dir_ptr + 2]);
                stat->pos *= SECTOR_SIZE;

                bin2int(&stat->size, &dir_buf[dir_ptr + 10]);

                stat->size_pos = dir_recode_addr + dir_ptr + 10;

                ret = 0;
                break;
            }
            dir_ptr += len_dr;
        }
    }

    free(dir_buf);

    return ret;
}
