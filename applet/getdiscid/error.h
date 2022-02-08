/*
 * error.h
 *
 *  Created on: 2009/10/17
 *      Author: takka
 */

#ifndef ERROR_H_
#define ERROR_H_

typedef enum {
  ERR_OPEN          = -1,
  ERR_READ          = -2,
  ERR_WRITE         = -3,
  ERR_SEEK          = -4,
  ERR_CLOSE         = -5,

  ERR_DECRYPT       = -6,
  ERR_NOT_CRYPT     = -7,

  ERR_DEFLATE       = -8,
  ERR_DEFLATE_SIZE  = -9,
  ERR_INFLATE       = -10,

  ERR_INIT          = -11,

  ERR_PRX           = -12,

  ERR_NOT_FOUND     = -13,
  ERR_BUF_SIZE_OVER = -14,

  ERR_CHG_STAT      = -15,

  ERR_NO_UMD        = -16,

  ERR_RENAME        = -17,

  ERR_NO_MEMORY     = -18,

  ERR_ISO_SIZE_OVER = -19,

} err_msg_num;

#define YES                 (1)
#define NO                  (0)

#define DONE                (0)
#define CANCEL              (-1)

int err_msg(err_msg_num num);

#endif
