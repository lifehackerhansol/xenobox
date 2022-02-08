/*
 * file.h
 *
 *  Created on: 2009/12/26
 *      Author: takka
 */

#ifndef FILE_H_

#define FILE_H_

#include "../../xenobox.h"
#include "../pspiofilemgr_stdio.h"

//#include <psptypes.h>
//#include <pspiofilemgr.h>

#define PSP_O_RDWR_PSP_O_CREAT PSP_O_RDWR|PSP_O_CREAT
#define MAX_PATH_LEN (256)

typedef enum {
  TYPE_ISO = 0,
  TYPE_CSO,
  TYPE_DIR,
  TYPE_UMD,
  TYPE_SYS,
  TYPE_PBT,
  TYPE_ETC,
  TYPE_JSO,
  TYPE_DAX,
} file_type;

typedef struct {
  char padding[3];
  char sort_type;
  char name[MAX_PATH_LEN];
  file_type type;
  int num;
} dir_t;

typedef struct {
  int mode;
  char name[256];
  file_type type;
} read_dir_list;

typedef struct {
  char name[MAX_PATH_LEN];
  file_type type;
  int iso_flag;
  char iso_name[MAX_PATH_LEN];
  int size;
  int sector;
  void* next_file;
  void* before_file;
  char umd_id[11];
  char e_name[128];
  int umd_size;
  int file_size;
  int num;
} dir_t_2;

#define SECTOR_SIZE (0x800)

/*---------------------------------------------------------------------------
  指定したディレクトリ情報を読取る
---------------------------------------------------------------------------*/
int read_dir(dir_t dir[], const char *path, int use_umd, read_dir_list list[]);
int read_dir_2(dir_t dir[], const char *path, int read_dir_flag);

int ms_read(void* buf, const char* path, int pos, int size);

int ms_write(const void* buf, const char* path, int pos, int size, int mode);

/*---------------------------------------------------------------------------
  ファイルリード
---------------------------------------------------------------------------*/
//int ms_sector_read(void* buf, const char* path, int sec, int num);
int file_read(void* buf, const char* path, file_type type, int pos, int size);

/*---------------------------------------------------------------------------
  ファイルライト
---------------------------------------------------------------------------*/
//int ms_sector_write(const void* buf, const char* path, int sec, int num);
int file_write(const void* buf, const char* path, file_type type, int pos, int size, int mode);

/*---------------------------------------------------------------------------
---------------------------------------------------------------------------*/
int set_file_mode(const char* path, int bits);
int get_file_mode(const char* path);

int up_dir(char *path);

int get_umd_sector(const char* path, file_type type);

int get_umd_id(char* id, const char* path, file_type type);

int get_umd_name(char* name, char* e_name, const char* id, const char* path, file_type type, int mode);

int read_line(char* str,  SceUID fp, int num);

int get_ms_free();
int check_ms();
int check_file(const char* path);

int file_copy(const char* s_path, const char* d_path);
int file_copy_part(const char* s_path, const char* d_path,int start, int size);
int file_update_copy(const char* s_path, const char* d_path);
int file_rm(const char* path);
int file_mkdir(const char* path);
int file_rmdir(const char* path);

int set_plugin(int type, char *path, int mode);

int file_patch_mem(char *buf, char *path);

#endif /* FILE_H_ */
