/*
 * iso.h
 *
 *  Created on: 2009/12/29
 *      Author: takka
 */

#ifndef ISO_H_
#define ISO_H_

#include "file.h"

typedef struct
{
    int pos;
    int size;
    int size_pos;
} iso_file_stat;

int iso_read(void* buf, int max_buf, const char* path, file_type type, const char* file);
int iso_write(void* buf, int max_buf, const char* path, file_type type, const char* file);
int iso_file_add(char* iso_dir, char* iso_name, char* dir_path, char* file_dir, char* file_name);
int iso_file_del(char* iso_dir, char* iso_name, char* dir_path, char* file_dir, char* file_name);
int iso_read_dir(dir_t dir[], const char* iso_path, const char* dir_path, int read_dir_flag);
int iso_get_file_info(iso_file_stat* stat, const char* path, file_type type, const char* name);

#endif /* ISO_H_ */
