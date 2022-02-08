/*
 * ciso.c
 *
 *  Created on: 2009/10/12
 *      Author: takka
 */

#include "../../lib/zlib/zlib.h"
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "ciso.h"
#include "file.h"

/*---------------------------------------------------------------------------
  deflateの解凍を行う
    char* o_buff 解凍先
    int o_size   解凍先バッファサイズ
    char* i_buff 入力
    int i_size   入力サイズ

    返値 解凍後のサイズ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int inflate_cso(char* o_buff, int o_size, const char* i_buff, int i_size)
{
  z_stream z;
  int size;

  // 初期化
  z.zalloc = Z_NULL;
  z.zfree = Z_NULL;
  z.opaque = Z_NULL;
  z.next_in = Z_NULL;
  z.avail_in = 0;
  if(inflateInit2(&z, -15) != Z_OK)
    return ERR_INFLATE;

  z.next_in = (unsigned char*)i_buff;
  z.avail_in = i_size;
  z.next_out = (unsigned char*)o_buff;
  z.avail_out = o_size;

  inflate(&z, Z_FINISH);

  // 出力サイズ
  size = o_size - z.avail_out;

  if(inflateEnd(&z) != Z_OK)
    return ERR_INFLATE;

  return size;
}

/*---------------------------------------------------------------------------
  deflateの圧縮を行う
    char* o_buff 圧縮先
    int o_size   圧縮先バッファサイズ
    char* i_buff 入力
    int i_size   入力サイズ
    int level    圧縮レベル(0-9)

    返値 圧縮後のサイズ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int deflate_cso(char* o_buff, int o_size, const char* i_buff, int i_size, int level)
{

  z_stream z;
  int size;

  // 初期化
  z.zalloc = Z_NULL;
  z.zfree = Z_NULL;
  z.opaque = Z_NULL;
  z.next_in = Z_NULL;
  z.avail_in = 0;
  if(deflateInit2(&z, level , Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY) != Z_OK)
    return ERR_DEFLATE;

  z.next_in = (unsigned char*)i_buff;
  z.avail_in = i_size;
  z.next_out = (unsigned char*)o_buff;
  z.avail_out = o_size;

  deflate(&z, Z_FINISH);

  // 出力サイズ
  size = o_size - z.avail_out;

  if (deflateEnd(&z) != Z_OK)
    return ERR_DEFLATE;

  return size;
}

/*---------------------------------------------------------------------------
  deflateの自動圧縮を行う(最大9倍の時間がかかるので注意)
    char* o_buff 圧縮先
    int o_size   圧縮先バッファサイズ
    char* i_buff 入力
    int i_size   入力サイズ
    int aim_size 目標サイズ

    返値 圧縮後のサイズ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int auto_deflate_cso(char* o_buff, int o_size, const char* i_buff, int i_size, int aim_size)
{
  z_stream z;
  int size = 0x7fffffff;
  int level = 1;

  while(size > aim_size)
  {
    // 初期化
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.opaque = Z_NULL;
    z.next_in = Z_NULL;
    z.avail_in = 0;

    if(deflateInit2(&z, level , Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY) != Z_OK)
      return ERR_DEFLATE;

    z.next_in = (unsigned char*)i_buff;
    z.avail_in = i_size;
    z.next_out = (unsigned char*)o_buff;
    z.avail_out = o_size;

    deflate(&z, Z_FINISH);

    // 出力サイズ
    size = o_size - z.avail_out;

    if (deflateEnd(&z) != Z_OK)
      return -1;

    level++;
    if(level > 9)
      return ERR_DEFLATE_SIZE;
  }

  return size;
}

/*---------------------------------------------------------------------------
  CSOから読込む
    char *buf        読込みバッファ
    const char *path パス
    int pos          読込み位置
    int size         読込みサイズ

    返値 実際に読み込んだ長さ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int cso_read(char *buf, const char *path, int pos, int size)
{
  SceUID fp;
  int ret;
  //int err;

  fp = sceIoOpen(path, PSP_O_RDONLY, 0777);

  if(fp < 0)
    return ERR_OPEN;

  ret = cso_read_fp(buf, fp, pos, size);

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
int cso_read_fp(char *buf, SceUID fp, int pos, int size)
{
  static SceUID old_fp = 0;
  static CISO_H header;
  int start_sec;
  int max_sector;
  int end_sec;
  int sector_num;
  unsigned long long int now_pos = 0;
  unsigned long long int next_pos = 0;
  int read_size;
  unsigned int zip_flag;
  //char __decompbuf[65536];   // 展開済みデータバッファ
  //char __compbuf[65536 * 2]; // 圧縮データ読み込みバッファ
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

    err = sceIoRead(fp, &header, CISO_HEADER_SIZE);
    if(err < 0)
      return ERR_READ;

    old_fp = fp;
  }

  // 読込セクタ数を計算
  if((pos + size) > header.total_bytes)
    size = header.total_bytes - pos;

  max_sector = header.total_bytes / header.block_size - 1;
  start_sec = pos / header.block_size;
  end_sec = (pos + size - 1) / header.block_size;
  sector_num = start_sec;

  if(sector_num > max_sector)
    return ERR_SEEK;

  if(end_sec > max_sector)
    end_sec = max_sector;

  ret = 0;
  while(sector_num <= end_sec)
  {
    // セクタ番号からファイル位置と長さを取得
    err = sceIoLseek32(fp, CISO_HEADER_SIZE + (sector_num * 4), PSP_SEEK_SET);
    if(err < 0)
      return ERR_SEEK;

    err = sceIoRead(fp, &now_pos, 4);
    if(err < 0)
      return ERR_READ;

    zip_flag = now_pos & 0x80000000;
    now_pos = (now_pos & 0x7fffffff) << header.align;

    err = sceIoRead(fp, &next_pos, 4);
    if(err < 0)
      return ERR_READ;

    read_size = ((next_pos & 0x7fffffff) << header.align) - now_pos;

    // １セクタを読込
    err = sceIoLseek32(fp, now_pos, PSP_SEEK_SET);
    if(err < 0)
      return ERR_SEEK;

    if(zip_flag != 0)
    {
      // 未圧縮
      err = sceIoRead(fp, __decompbuf, header.block_size);
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
      err = inflate_cso(__decompbuf, header.block_size, __compbuf, read_size);
      if(err < 0)
        return ERR_INFLATE;
    }

    // 指定バッファに転送
    if((sector_num > start_sec) && (sector_num < end_sec))
    {
      // 全転送
      memcpy(buf, __decompbuf, header.block_size);
      read_size = header.block_size;
    }
    else if((sector_num == start_sec) || (sector_num == end_sec))
    {
      // 部分転送
      start_pos = 0;
      end_pos = header.block_size;
      if(sector_num == start_sec)
        start_pos = pos - (start_sec * header.block_size);
      if(sector_num == end_sec)
        end_pos = (pos + size) - (end_sec * header.block_size);
      read_size = end_pos - start_pos;
      memcpy(buf, &__decompbuf[start_pos], read_size);
    }

    buf += read_size;
    ret += read_size;
    sector_num++;
  }

  return ret;
}

/*---------------------------------------------------------------------------
  CSOに書込む
    char *buf        書込みバッファ
    const char *path パス
    int pos          書込み位置
    int size         書込みサイズ
    int level        圧縮レベル(0-9)

    返値 実際に書込んだ長さ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int cso_write(const char *buf, const char *path, int pos, int size, int level)
{
  SceUID fp;
  int ret;
  //int err;

  fp = sceIoOpen(path, PSP_O_RDWR_PSP_O_CREAT, 0777);
  if(fp < 0)
    return ERR_OPEN;

  ret = cso_write_fp(buf, fp, pos, size, level);

  /*err =*/ sceIoClose(fp);
  if(fp < 0)
    return ERR_CLOSE;

  return ret;
}

/*---------------------------------------------------------------------------
  CSOに連続で書込む
    char *buf 書込みバッファ
    SceUID fp ファイルポインタ
    int pos   書込み位置
    int size  書込みサイズ
    int level 圧縮レベル(0-9)

    返値 実際に書込んだ長さ / エラーの場合は負を返す

    事前にsceIoOpen(PSP_O_WRONLYで) / 終了後にsceIoCloseが必要
---------------------------------------------------------------------------*/
int cso_write_fp(const char *buf, SceUID fp, int pos, int size, int level)
{
  static SceUID old_fp = 0;
  static CISO_H header;
  int start_sec;
  int end_sec;
  int sector_num;
  unsigned long long int now_pos = 0;
  unsigned long long int next_pos = 0;
  int write_size;
  int data_size;
  int read_size = 0;
  unsigned int zip_flag;
  //char tmp_buf[SECTOR_SIZE * 2];   // 部分書込合成用バッファ
  //char tmp_buf_2[SECTOR_SIZE * 2]; // 圧縮済みバッファ
  int ret;
  int start_pos;
  int end_pos;
  int err;

  // ヘッダー読込
  if(old_fp != fp)
  {
    err = sceIoLseek32(fp, 0, PSP_SEEK_SET);
    if(err < 0)
      return ERR_SEEK;

    err = sceIoRead(fp, &header, CISO_HEADER_SIZE);
    if(err < 0)
      return ERR_READ;

    old_fp = fp;
  }

  // 書込セクタ数を計算
  start_sec = pos / SECTOR_SIZE;
  end_sec = (pos + size - 1) / SECTOR_SIZE;
  sector_num = start_sec;

  ret = 0;
  while(sector_num <= end_sec)
  {
    // セクタ番号からファイル位置と長さを取得
    err = sceIoLseek32(fp, CISO_HEADER_SIZE + (sector_num * 4), PSP_SEEK_SET);
    if(err < 0)
      return ERR_SEEK;

    err = sceIoRead(fp, &now_pos, 4);
    if(err < 0)
      return ERR_READ;

    zip_flag = now_pos & 0x80000000;
    now_pos = (now_pos & 0x7fffffff) << header.align;

    err = sceIoRead(fp, &next_pos, 4);
    if(err < 0)
      return ERR_READ;

    write_size = ((next_pos & 0x7fffffff) << header.align) - now_pos;


    // 指定バッファから書き込みバッファに転送
    if((sector_num > start_sec) && (sector_num < end_sec))
    {
      // 全転送
      memcpy(__decompbuf, buf, header.block_size);
      read_size = header.block_size;
    }
    else if((sector_num == start_sec) || (sector_num == end_sec))
    {
      cso_read_fp(__decompbuf, fp, sector_num * SECTOR_SIZE, header.block_size);
      // 部分転送
      start_pos = 0;
      end_pos = header.block_size;
      if(sector_num == start_sec)
        start_pos = pos - (start_sec * header.block_size);
      if(sector_num == end_sec)
        end_pos = (pos + size) - (end_sec * header.block_size);
      read_size = end_pos - start_pos;
      memcpy(&__decompbuf[start_pos], buf, read_size);
    }

    if(zip_flag != 0)
    {
      data_size = header.block_size;
      memcpy(__compbuf, __decompbuf, data_size);
    }
    else
    {
      if(level > 0)
        data_size = deflate_cso(__compbuf, header.block_size, __decompbuf, header.block_size, level);
      else
        data_size = auto_deflate_cso(__compbuf, header.block_size, __decompbuf, header.block_size, write_size);

      if(data_size < 0)
        return data_size;

      if(data_size <= write_size)
        memset(&__compbuf[data_size], 0, write_size - data_size);
      else
        return ERR_DEFLATE_SIZE;
    }

    err = sceIoLseek32(fp, now_pos, PSP_SEEK_SET);
    if(err < 0)
      return ERR_SEEK;

    err = sceIoWrite(fp, __compbuf, write_size);
    if(err < 0)
      return ERR_READ;

    buf += read_size;
    ret += read_size;
    sector_num++;
  }

  return ret;
}

