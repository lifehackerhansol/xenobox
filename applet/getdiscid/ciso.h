/*
 * ciso.h
 *
 *  Created on: 2009/10/12
 *      Author: takka
 */

#ifndef __CISO_H__
#define __CISO_H__

#ifndef EMU
//#include <pspiofilemgr.h>
#else
#endif

#include "file.h"

typedef struct ciso_header
{
    unsigned char magic[4];         /* +00 : 'C','I','S','O'                 */
    unsigned int  header_size;      /* +04 : header size (==0x18)            */
    unsigned long long int total_bytes; /* +08 : number of original data size    */
    unsigned int  block_size;       /* +10 : number of compressed block size */
    unsigned char ver;              /* +14 : version 01                      */
    unsigned char align;            /* +15 : align of index value            */
    unsigned char rsv_06[2];        /* +16 : reserved                        */
}CISO_H;

#define CISO_HEADER_SIZE (0x18)

/*---------------------------------------------------------------------------
  deflateの解凍を行う
    char* o_buff 解凍先
    int o_size   解凍先バッファサイズ
    char* i_buff 入力
    int i_size   入力サイズ

    返値 解凍後のサイズ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int inflate_cso(char* o_buff, int o_size, const char* i_buff, int i_size);

/*---------------------------------------------------------------------------
  deflateの圧縮を行う
    char* o_buff 圧縮先
    int o_size   圧縮先バッファサイズ
    char* i_buff 入力
    int i_size   入力サイズ
    int level    圧縮レベル(0-9)

    返値 圧縮後のサイズ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int deflate_cso(char* o_buff, int o_size, const char* i_buff, int i_size, int level);

/*---------------------------------------------------------------------------
  deflateの自動圧縮を行う(最大9倍の時間がかかるので注意)
    char* o_buff 圧縮先
    int o_size   圧縮先バッファサイズ
    char* i_buff 入力
    int i_size   入力サイズ
    int aim_size 目標サイズ

    返値 圧縮後のサイズ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int auto_deflate_cso(char* o_buff, int o_size, const char* i_buff, int i_size, int aim_size);

/*---------------------------------------------------------------------------
  CSOから読込む
    char *buf        読込みバッファ
    const char *path パス
    int pos          読込み位置
    int size         読込みサイズ

    返値 実際に読み込んだ長さ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int cso_read(char *buf, const char *path, int pos, int size);

/*---------------------------------------------------------------------------
  CSOから連続で読込む
    char *buf 読込みバッファ
    SceUID fp ファイルポインタ
    int pos   読込み位置
    int size  読込みサイズ

    返値 実際に読み込んだ長さ / エラーの場合は負を返す

    事前にsceIoOpen / 終了後にsceIoCloseが必要
---------------------------------------------------------------------------*/
int cso_read_fp(char *buf, SceUID fp, int pos, int size);

/*---------------------------------------------------------------------------
  CSOに書込む
    char *buf        書込みバッファ
    const char *path パス
    int pos          書込み位置
    int size         書込みサイズ
    int level        圧縮レベル(0-9)

    返値 実際に書込んだ長さ / エラーの場合は負を返す
---------------------------------------------------------------------------*/
int cso_write(const char *buf, const char *path, int pos, int size, int level);

/*---------------------------------------------------------------------------
  CSOに連続で書込む
    char *buf 書込みバッファ
    SceUID fp ファイルポインタ
    int pos   書込み位置
    int size  書込みサイズ
    int level 圧縮レベル(0-9)

    返値 実際に書込んだ長さ / エラーの場合は負を返す

    事前にsceIoOpen / 終了後にsceIoCloseが必要
---------------------------------------------------------------------------*/
int cso_write_fp(const char *buf, SceUID fp, int pos, int size, int level);

#endif
