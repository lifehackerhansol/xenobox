/*
 * file.c
 *
 *  Created on: 2009/12/26
 *      Author: takka
 */

#include <string.h>
#include <stdlib.h>

#ifndef EMU
//#include <pspkernel.h>
//#include <pspmscm.h>
#endif

#include "file.h"
#include "main.h"
#include "error.h"
#include "ciso.h"
#include "iso.h"

#define FIO_CST_SIZE    0x0004

int compare_dir_int(const void* c1, const void* c2);
int compare_dir_str(const void* c1, const void* c2);
int compare_dir_dir(const void* c1, const void* c2);

int wild_strcasecmp(const char* str, const char* pattern);

#if 0
/*---------------------------------------------------------------------------
  ファイルサイズ変更 // 動作しません
  const char *path : パス
  int length       : サイズ

  return int       : 変更後のファイルサイズ, エラーの場合はERR_CHG_STATを返す
---------------------------------------------------------------------------*/
int file_truncate(const char *path, int length)
{
    SceIoStat psp_stat;
    int ret;

    psp_stat.st_size = length;
    ret = sceIoChstat(path, &psp_stat, FIO_CST_SIZE);
    if(ret < 0)
      ret = ERR_CHG_STAT;

    return ret;
}
#endif

// ソート時の優先順位
const char dir_type_sort[] = {
    'c', // TYPE_ISO
    'c', // TYPE_CSO
    'b', // TYPE_DIR
    'a', // TYPE_UMD
    'c', // TYPE_SYS
    'c', // TYPE_PBT
    'c', // TYPE_ETC
    'c', // TYPE_JSO
    'c', // TYPE_DAX
};

read_dir_list default_list[] = {
    { FIO_S_IFREG, "*.iso",     TYPE_ISO },
    { FIO_S_IFREG, "*.cso",     TYPE_CSO },
    { FIO_S_IFREG, "PBOOT.PBP", TYPE_PBT },
//    { FIO_S_IFREG, "*", TYPE_ETC },
    { FIO_S_IFDIR, "*",         TYPE_DIR },
    { 0,           "",          TYPE_ETC },

};

int compare_dir_str(const void* c1, const void* c2)
{
  return strcasecmp(&(((dir_t *)c1)->sort_type), &(((dir_t *)c2)->sort_type));
}

int wild_strcasecmp(const char* str, const char* pattern)
{
  int ret = -1;

  if(pattern[0] == '*')
  {
    if(pattern[1] == '\0')
      ret = 0;
    else
      ret = strcasecmp(&str[strlen(str) - strlen(&pattern[1])], &pattern[1]);
  }
  else
    ret = strcasecmp(str, pattern);

  return ret;
}
#if 0
/*---------------------------------------------------------------------------
  ディレクトリ読取り
  dir_t dir[]      : dir_t配列のポインタ
  const char *path : パス

  return int       : ファイル数, dir[0].numにも保存される
---------------------------------------------------------------------------*/
int read_dir(dir_t dir[], const char *path, int use_umd, read_dir_list read_list[])
{
  SceUID dp;
  SceIoDirent entry;
  int num;
  int file_num = 0;
  int ret;
  int loop;
  read_dir_list *list;

  ret = check_ms();

  if(use_umd == 1)
  {
    strcpy(dir[0].name, "[UMD DRIVE]");
    dir[0].type = TYPE_UMD;
    dir[file_num].sort_type = dir_type_sort[TYPE_UMD];
    file_num++;
  }

  if(read_list == NULL)
    list = default_list;
  else
    list = read_list;

  dp = sceIoDopen(path);
  if(dp >= 0)
  {
    memset(&entry, 0, sizeof(entry));

    while((sceIoDread(dp, &entry) > 0))
    {
      num = strlen(entry.d_name);

      strcpy(dir[file_num].name, entry.d_name);

      loop = 0;
      while(list[loop].mode != 0)
      {
        if((entry.d_stat.st_mode & FIO_S_IFMT) == list[loop].mode)
        {
          if((strcmp(&entry.d_name[0], ".") != 0) && (strcmp(&entry.d_name[0], "..") != 0))
          {
            if(wild_strcasecmp(entry.d_name, list[loop].name) == 0)
            {
              dir[file_num].type = list[loop].type;
              dir[file_num].sort_type = dir_type_sort[list[loop].type];
              file_num++;
            }
          }
        }
        loop++;
      }
    }
    sceIoDclose(dp);
  }

  qsort(dir, file_num, sizeof(dir_t), compare_dir_str);

  dir[0].num = file_num;

  return file_num;
}

/*---------------------------------------------------------------------------
  ディレクトリ読取り
  dir_t dir[]      : dir_t配列のポインタ
  const char *path : パス

  return int       : ファイル数, dir[0].numにも保存される
---------------------------------------------------------------------------*/
int read_dir_2(dir_t dir[], const char *path, int read_dir_flag)
{
  SceUID dp;
  SceIoDirent entry;
  int num;
  int file_num = 0;
  int ret;

  ret = check_ms();

  dp = sceIoDopen(path);
  if(dp >= 0)
  {
    memset(&entry, 0, sizeof(entry));

    while((sceIoDread(dp, &entry) > 0))
    {
      num = strlen(entry.d_name);

      strcpy(dir[file_num].name, entry.d_name);
      switch(entry.d_stat.st_mode & FIO_S_IFMT)
      {
        case FIO_S_IFREG:
          dir[file_num].type = TYPE_ETC;
          dir[file_num].sort_type = dir_type_sort[TYPE_ETC];
          file_num++;
          break;

        case FIO_S_IFDIR:
          if(read_dir_flag == 1)
          {
            if((strcmp(&entry.d_name[0], ".") != 0) && (strcmp(&entry.d_name[0], "..") != 0))
            {
              dir[file_num].type = TYPE_DIR;
              dir[file_num].sort_type = dir_type_sort[TYPE_DIR];
              file_num++;
            }
          }
          break;
      }
    }
    sceIoDclose(dp);
  }

  dir[0].num = file_num;

  // ファイル名でソート
//  qsort(&dir[0], file_num - 1, sizeof(dir_t), compare_dir_str);

  return file_num;
}
#endif
/*---------------------------------------------------------------------------
  MSのリード
  void* buf        : 読取りバッファ
  const char* path : パス
  int pos          : 読込み開始場所
  int size         : 読込みサイズ, 0を指定すると全てを読込む

  return int       : 読込みサイズ, エラーの場合は ERR_OPEN/ERR_READ を返す
---------------------------------------------------------------------------*/
int ms_read(void* buf, const char* path, int pos, int size)
{
  SceUID fp;
  SceIoStat _stat;
  int ret = ERR_OPEN;

  if(size == 0)
  {
    pos = 0;
    ret = sceIoGetstat(path, &_stat);
    if(ret < 0)
      return ERR_OPEN;
    else
    size = _stat.st_size;
  }

  fp = sceIoOpen(path, PSP_O_RDONLY, 0777);
  if(fp > 0)
  {
    if(pos != 0)
      sceIoLseek32(fp, pos, PSP_SEEK_SET);
    ret = sceIoRead(fp, buf, size);
    sceIoClose(fp);
    if(ret < 0)
      ret = ERR_READ;
  }
  return ret;
}

/*---------------------------------------------------------------------------
  MSへのライト
  const void* buf  : 書込みバッファ
  const char* path : パス
  int pos          : 書込み開始場所
  int size         : 書込みサイズ

  return int       : 書込んだサイズ, エラーの場合は ERR_OPEN/ERR_WRITE を返す
---------------------------------------------------------------------------*/
int ms_write(const void* buf, const char* path, int pos, int size, int mode)
{
  SceUID fp;
  int ret = ERR_OPEN;

  fp = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | mode, 0777);
  if(fp > 0)
  {
    sceIoLseek32(fp, pos, PSP_SEEK_SET);
    ret = sceIoWrite(fp, buf, size);
    sceIoClose(fp);
    if(ret < 0)
      ret = ERR_WRITE;
  }
  return ret;
}

/*---------------------------------------------------------------------------
  ファイルリード
---------------------------------------------------------------------------*/
int dax_read(char *buf, const char *path, int pos, int size);
int jso_read(char *buf, const char *path, int pos, int size);

int file_read(void* buf, const char* path, file_type type, int pos, int size)
{
  int ret = ERR_OPEN;

  switch(type)
  {
    case TYPE_ISO:
      if(size != 0)
        ret = ms_read(buf, path, pos, size);
      else
        ret = 0;
      break;

    case TYPE_SYS:
    case TYPE_ETC:
      ret = ms_read(buf, path, pos, size);
      break;

    case TYPE_CSO:
      if(size != 0)
        ret = cso_read(buf, path, pos, size);
      else
        ret = 0;
      break;

    case TYPE_JSO:
      if(size != 0)
        ret = jso_read(buf, path, pos, size);
      else
        ret = 0;
      break;
 
    case TYPE_DAX:
      if(size != 0)
        ret = dax_read(buf, path, pos, size);
      else
        ret = 0;
      break;

    //case TYPE_UMD:
    //  ret = umd_read(buf, path, pos, size);
    //  break;

    default:
      break;
  }
  return ret;
}

/*---------------------------------------------------------------------------
  ファイルライト
---------------------------------------------------------------------------*/
int file_write(const void* buf, const char* path, file_type type, int pos, int size, int mode)
{
  u32 ret = ERR_OPEN;

  switch(type)
  {
    case TYPE_ISO:
    case TYPE_ETC:
      ret = ms_write(buf, path, pos, size, mode);
      break;

    case TYPE_CSO:
      ret = cso_write(buf, path, pos, size, 9);
      break;

    case TYPE_UMD:
      break;

    default:
      break;
  }
  return ret;
}
#if 0
// FIO_S_IWUSR | FIO_S_IWGRP | FIO_S_IWOTH
int set_file_mode(const char* path, int bits)
{
  SceIoStat stat;
  int ret;

  ret = sceIoGetstat(path, &stat);

  if(ret >= 0)
  {
    stat.st_mode |= (bits);
    ret = sceIoChstat(path, &stat, (FIO_S_IRWXU | FIO_S_IRWXG | FIO_S_IRWXO));
  }
  if(ret < 0)
    ret = ERR_CHG_STAT;

  return ret;
}
#endif
/*---------------------------------------------------------------------------
---------------------------------------------------------------------------*/
int up_dir(char *path)
{
  int loop;
  int ret = ERR_OPEN;

  loop = strlen(path) - 2;

  while(path[loop--] != '/')
    ;

  if(path[loop - 1] != ':')
  {
    path[loop + 2] = '\0';
    ret = 0;
  }

  return ret;
}

int read_line(char* str,  SceUID fp, int num)
{
  char buf;
  int len = 0;
  int ret;

  do{
    ret = sceIoRead(fp, &buf, 1);
    if(ret == 1)
    {
      if(buf == '\n')
      {
        str[len] = '\0';
        len++;
        break;
      }
      else
        if(buf != '\r')
        {
          str[len] = buf;
          len++;
        }
    }
  }while((ret > 0) && (len < num));

  return len;
}

int get_umd_sector(const char* path, file_type type)
{
  int size = 0;
  int ret;

  ret = file_read(&size, path, type, 0x8050, 4); // 0x50から4byteがセクタ数
  if(ret < 0)
    size = ret;

  return size;
}

int get_umd_id(char* id, const char* path, file_type type)
{
  int ret;
  // 0x8373から10byteがUMD ID
  ret = file_read(id, path, type, 0x8373, 10);
  if(ret == 10)
    id[10] = '\0';
  else
    strcpy(id, "**********");

  return ret;
}

#if 0
int get_umd_name(char* name, char* e_name, const char* id, const char* path, file_type type, int mode)
{
  static char buf[1024*256]; // 256KB
  static int init = 0;
  char *ptr;
  int ptr2 = 0;
  int ret = 0;

  char param[4096];

  typedef struct {
    short label_off;
    char unk;
    char data_type;
    int datafield_used;
    int datafield_size;
    int data_off;
  } psf_t;

  int label_ptr;
  int data_ptr;
  psf_t *psf_ptr;
  int loop;

  if((init == 0)||(mode == 1))
  {
    memset(buf, 0, sizeof(buf));
    ms_read(buf, "UMD_ID.csv", 0, 0);
    init = 1;
    if(mode == 1)
      return 0;
  }

  ptr = strstr((const char *)buf, id);

  if(ptr != NULL)
  {
    ptr += 11;

    while(*ptr == '\\')
      ptr++;

    while(*ptr != '\\')
    {
      name[ptr2] = *ptr;
      ptr++;
      ptr2++;
    }
    name[ptr2] = '\0';

    while(*ptr == '\\')
      ptr++;

    ptr2 = 0;
    while((*ptr != '\r') && (*ptr != '\n'))
    {
      e_name[ptr2] = *ptr;
      ptr++;
      ptr2++;
    }
    e_name[ptr2] = '\0';

  }
  else
  {
    ret = -1;

    // PARAMからタイトルを取得する
    if(type == TYPE_UMD)
      ret = ms_read(param, "disc:/PSP_GAME/PARAM.SFO", 0, 0);
    else
      ret = iso_read(param, 4096, path, type, "PSP_GAME/PARAM.SFO");

    if(ret <= 0)
    {
      strcpy(name, id);
      strcpy(e_name, id);
      return ret;
    }

    loop = 0;
    label_ptr = read32(param+0x08);//*(int *)&param[0x08];
    data_ptr = read32(param+0x0c);//*(int *)&param[0x0c];
    psf_ptr = (psf_t *)&param[0x14];
    name[0] = '\0';
    while(loop < read32(param+0x10))//*(int *)&param[0x10])
    {
      if(strcmp((char *)(psf_ptr[loop].label_off + label_ptr + param), "TITLE") == 0)
      {
        strcpy(name, (char *)(psf_ptr[loop].data_off + data_ptr + param));
        break;
      }
      loop++;
    }

    if(name[0] == '\0')
      strcpy(name, id);
    strcpy(e_name, id);
  }

  return ret;
}
#endif

#if 0
int get_ms_free()
{
    unsigned int buf[5];
    unsigned int *pbuf = buf;
    int free = 0;
    int ret;

    //    buf[0] = 合計クラスタ数
    //    buf[1] = フリーなクラスタ数(ギリギリまで使いたいならこっち)
    //    buf[2] = フリーなクラスタ数(buf[3]やbuf[4]と掛けて1MB単位になるようになってる)
    //    buf[3] = セクタ当たりバイト数
    //    buf[4] = クラスタ当たりセクタ数
    ret = sceIoDevctl("ms0:", 0x02425818, &pbuf, sizeof(pbuf), 0, 0);

    if(ret >= 0)
      free = buf[1] * ((buf[3] * buf[4]) / 1024);// 空き容量取得(kb)

    return free;
}
#endif
int check_ms()
{
//  SceUID ms;
  int ret = DONE;

//  ms = MScmIsMediumInserted();
//  if(ms <= 0)
//  {
//    msg_win("", 0, MSG_CLEAR, 0);
//    msg_win("Memory Stickを入れて下さい", 1, MSG_WAIT, 0);
//
//    ms = -1;
//    while(ms <= 0)
//    {
//      sceKernelDelayThread(1000);
//      ms = MScmIsMediumInserted();
//    }
//    msg_win("", 0, MSG_CLEAR, 0);
//    msg_win("マウント中です", 1, MSG_WAIT, 0);
//    ret = CANCEL;
//  }
  return ret;
}

int check_file(const char* path)
{
  SceIoStat _stat;

  return sceIoGetstat(path, &_stat);
}

int file_update_copy(const char* s_path, const char* d_path)
{
  int ret;
  SceIoStat _stat;
  char new_path[256];
  char *ptr;

  strcpy(new_path, d_path);
  if(new_path[strlen(new_path) - 1] == '/')
  {
    ptr = strrchr(s_path, '/');
    if(ptr != NULL)
      strcat(new_path, &ptr[1]);
    else
      strcat(new_path, s_path);
  }

  ret = sceIoGetstat(new_path, &_stat); // TODO
  if(ret < 0)
    file_copy(s_path, new_path);

  return 0;
}

int file_copy(const char* s_path, const char* d_path)
{
  char *buf = WORK;
  int ret;
  SceIoStat _stat;
  int pos;
  int read_size;
  int write_pos;
  char new_path[256];
  char *ptr;

  strcpy(new_path, d_path);
  if(new_path[strlen(new_path) - 1] == '/')
  {
    ptr = strrchr(s_path, '/');
    if(ptr != NULL)
      strcat(new_path, &ptr[1]);
    else
      strcat(new_path, s_path);
  }

  ret = sceIoGetstat(s_path, &_stat); // TODO
  if(ret < 0)
    return -1;

  //if(strncasecmp(new_path, "flash", 5) == 0)
  //  flash_mode(new_path[5] - '0', 1);

  if(_stat.st_size <= MAX_SECTOR_NUM * SECTOR_SIZE)
  {
    ret = ms_read(buf, s_path, 0, _stat.st_size);
    if(ret < 0)
      return -1;

    ret = ms_write(buf, new_path, 0, _stat.st_size, PSP_O_TRUNC);
    if(ret < 0)
      return -1;
  }
  else
  {
    ms_write(buf, new_path, 0, 0, PSP_O_TRUNC);
    pos = 0;
    write_pos = 0;
    read_size = MAX_SECTOR_NUM * SECTOR_SIZE;
    while(_stat.st_size > 0)
    {
      ret = ms_read(buf, s_path, pos, read_size);
      if(ret < 0)
        return -1;

      ret = ms_write(buf, new_path, write_pos, ret, 0);
      if(ret < 0)
        return -1;

      pos += MAX_SECTOR_NUM * SECTOR_SIZE;
      write_pos += MAX_SECTOR_NUM * SECTOR_SIZE;
      _stat.st_size -= MAX_SECTOR_NUM * SECTOR_SIZE;
      if(_stat.st_size < MAX_SECTOR_NUM * SECTOR_SIZE)
        read_size = _stat.st_size;
    }
  }

  //if(strncasecmp(new_path, "flash", 5) == 0)
  //  flash_mode(new_path[5] - '0', 0);

  return 0;
}


int file_copy_part(const char* s_path, const char* d_path,int start, int size)
{
  char *buf = WORK;
  int ret;
  SceIoStat _stat;
  int pos;
  int read_size;
  int write_pos;
  char new_path[256];
  char *ptr;

  strcpy(new_path, d_path);
  if(new_path[strlen(new_path) - 1] == '/')
  {
    ptr = strrchr(s_path, '/');
    if(ptr != NULL)
      strcat(new_path, &ptr[1]);
    else
      strcat(new_path, s_path);
  }

  ret = sceIoGetstat(s_path, &_stat); // TODO
  if(ret < 0)
    return -1;

  //if(strncasecmp(new_path, "flash", 5) == 0)
  //  flash_mode(new_path[5] - '0', 1);

  _stat.st_size = size;

  if(_stat.st_size <= MAX_SECTOR_NUM * SECTOR_SIZE)
  {
    ret = ms_read(buf, s_path, start, _stat.st_size);
    if(ret < 0)
      return -1;

    ret = ms_write(buf, new_path, 0, _stat.st_size, PSP_O_TRUNC);
    if(ret < 0)
      return -1;
  }
  else
  {
    ms_write(buf, new_path, 0, 0, PSP_O_TRUNC);
    pos = start;
    write_pos = 0;
    read_size = MAX_SECTOR_NUM * SECTOR_SIZE;
    while(_stat.st_size > 0)
    {
      ret = ms_read(buf, s_path, pos, read_size);
      if(ret < 0)
        return -1;

      ret = ms_write(buf, new_path, write_pos, ret, 0);
      if(ret < 0)
        return -1;

      pos += MAX_SECTOR_NUM * SECTOR_SIZE;
      write_pos += MAX_SECTOR_NUM * SECTOR_SIZE;
      _stat.st_size -= MAX_SECTOR_NUM * SECTOR_SIZE;
      if(_stat.st_size < MAX_SECTOR_NUM * SECTOR_SIZE)
        read_size = _stat.st_size;
    }
  }

  //if(strncasecmp(new_path, "flash", 5) == 0)
  //  flash_mode(new_path[5] - '0', 0);

  return 0;
}

int file_rm(const char* path)
{
  int ret;

  //if(strncasecmp(path, "flash", 5) == 0)
  //  flash_mode(path[5] - '0', 1);

  ret =   sceIoRemove(path);

  //if(strncasecmp(path, "flash", 5) == 0)
  //  flash_mode(path[5] - '0', 0);

  return ret;
}

int file_mkdir(const char* path)
{
  int ret;

  ret = sceIoMkdir(path, 0777);
  return ret;
}

int file_rmdir(const char* path)
{
  int ret;

  ret = sceIoRmdir(path);
  return ret;
}

// GAME = 0, VSH = 1, POPS = 2
int set_plugin(int type, char *path, int mode)
{
  u8 *old_ptr = (u8 *)WORK;
  u8 *txt_ptr =  (u8 *)&WORK[1024 * 16];
  int len;
  SceUID fp;
  int flag = 0;
  char temp[256];
  //int ret;
  int num;

  char *txt_file[] = {
      "ms0:/seplugins/game.txt",
      "ms0:/seplugins/vsh.txt",
      "ms0:/seplugins/pops.txt",
  };

  // pathを作成
  num = strlen(path);

  // *.txtをオープン
  fp = sceIoOpen(txt_file[type], PSP_O_RDONLY, 0777);

  if(fp >=0 )
  {
    txt_ptr[0] = '\0';
    old_ptr[0] = '\0';
    while(len = read_line(temp, fp, sizeof(temp)), len > 0)
    {
      if(strncasecmp(temp, path, num) == 0)
      {
        sprintf(temp, "%s %d", path, mode);
        flag = 1;
      }
      strcat((char *)old_ptr, temp);
      strcat((char *)old_ptr, "\r\n");
    }
    sceIoClose(fp);
  }

  // mode = 1で、"ms0:/seplugins/peagasus.prx"が無かった場合は、先頭に"ms0:/seplugins/peagasus.prx 1"を追加
  if((mode == 1) && (flag == 0))
    sprintf((char *)txt_ptr, "%s 1\r\n", path);

  strcat((char *)txt_ptr, (char *)old_ptr);

  // game.txtを書出し
  /*ret =*/ ms_write(txt_ptr, txt_file[type], 0, strlen((char *)txt_ptr), PSP_O_TRUNC);

  return 0;
}

int file_patch_mem(char *buf, char *path)
{
  char temp[64];
  int addr;
  char old_data;
  char new_data;
  SceUID fp;
  char line[256];
  int ptr;
  int tmp_ptr;
  int flag;

  // OPEN
  fp = sceIoOpen(path, PSP_O_RDONLY, 0777);
  if(fp < 0)
    return -1;

  while(read_line(line, fp, sizeof(line)) != 0)
  {
    if((line[0] != '#') && (line[0] != ';') && (line[0] != '\0'))
    {
      ptr = 0;
      tmp_ptr = 0;
      flag = 0;

      // SPACE/TAB除去
      while((line[ptr] == ' ') || (line[ptr] == '\t'))
        ptr++;

      // addr
      while((line[ptr] != ' ') && (line[ptr] != '\t') && (line[ptr] != ':'))
        temp[tmp_ptr++] = line[ptr++];
      temp[tmp_ptr] = '\0';
      addr = strtoul(temp, NULL, 16);

      // SPACE/TAB/:除去
      while((line[ptr] == ' ') || (line[ptr] == '\t') || (line[ptr] == ':'))
        ptr++;

      // old_data
      temp[0] = line[ptr++];
      temp[1] = line[ptr++];
      temp[2] = '\0';
      if((temp[0] = '*') && (temp[1] = '*'))
        flag = 1;
      old_data = strtoul(temp, NULL, 16);

      // SPACE/TAB/:除去
      while((line[ptr] == ' ') || (line[ptr] == '\t') || (line[ptr] == ':'))
        ptr++;

      // new_data
      temp[0] = line[ptr++];
      temp[1] = line[ptr++];
      temp[2] = '\0';
      new_data = strtoul(temp, NULL, 16);

      if((buf[addr] == old_data) || (flag == 1))
        buf[addr] = new_data;
      else
        return addr;
    }
  }

  sceIoClose(fp);
  return 0;
}
