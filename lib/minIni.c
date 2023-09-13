/*  minIni - Multi-Platform INI file parser, suitable for embedded systems
 *
 *  These routines are in part based on the article "Multiplatform .INI Files"
 *  by Joseph J. Graf in the March 1994 issue of Dr. Dobb's Journal.
 *
 *  Copyright (c) ITB CompuPhase, 2008-2009
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy
 *  of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations
 *  under the License.
 *
 *  Version: $Id: minIni.c 24 2009-05-06 08:01:53Z thiadmer.riemersma $
 */

#include "minIni.h"
#define NDEBUG

#if (defined _UNICODE || defined __UNICODE__ || defined UNICODE) && !defined MININI_ANSI
#if !defined UNICODE /* for Windows */
#define UNICODE
#endif
#if !defined _UNICODE /* for C library */
#define _UNICODE
#endif
#endif

#if defined NDEBUG
#define assert(e)
#else
#include <assert.h>
#endif

#if !defined __T
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
/* definition of mTCHAR already in minIni.h */
#define __T(s)    s
#define _tcscat   strcat
#define _tcschr   strchr
#define _tcscmp   strcmp
#define _tcscpy   strcpy
#define _tcsicmp  stricmp
#define _tcslen   strlen
#define _tcsncmp  strncmp
#define _tcsnicmp strnicmp
#define _tcsrchr  strrchr
#define _tcstol   strtol
#define _tcstod   strtod
#define _totupper toupper
#define _stprintf sprintf
#define _tfgets   fgets
#define _tfputs   fputs
#define _tfopen   fopen
#define _tremove  remove
#define _trename  rename
#endif

#if defined __linux || defined __linux__
#define __LINUX__
#elif defined FREEBSD && !defined __FreeBSD__
#define __FreeBSD__
#elif defined(_MSC_VER)
#pragma warning(disable : 4996) /* for Microsoft Visual C/C++ */
#endif
#if !defined strnicmp && !defined PORTABLE_STRNICMP
#if defined __LINUX__ || defined __FreeBSD__ || defined __OpenBSD__ || defined __APPLE__
#define strnicmp strncasecmp
#endif
#endif

// #if !defined INI_LINETERM
//   #define INI_LINETERM    __T("\n")
// #endif
#if !defined INI_FILETYPE
#define INI_FILETYPE FILE*
#endif

mTCHAR* INI_LINETERM;
#define LF   __T("\n")
#define CRLF __T("\r\n")

#if !defined sizearray
#define sizearray(a) (sizeof(a) / sizeof((a)[0]))
#endif

enum quote_option
{
    QUOTE_NONE,
    QUOTE_ENQUOTE,
    QUOTE_DEQUOTE,
};

#if defined PORTABLE_STRNICMP
int strnicmp(const mTCHAR* s1, const mTCHAR* s2, size_t n)
{
    register unsigned mTCHAR c1, c2;

    while (n-- != 0 && (*s1 || *s2))
    {
        c1 = *(const unsigned mTCHAR*)s1++;
        if ('a' <= c1 && c1 <= 'z')
            c1 += ('A' - 'a');
        c2 = *(const unsigned mTCHAR*)s2++;
        if ('a' <= c2 && c2 <= 'z')
            c2 += ('A' - 'a');
        if (c1 != c2)
            return c1 - c2;
    } /* while */
    return 0;
}
#endif /* PORTABLE_STRNICMP */

static mTCHAR* skipleading(const mTCHAR* str)
{
    assert(str != NULL);
    while (*str != '\0' && *str <= ' ')
        str++;
    return (mTCHAR*)str;
}

static mTCHAR* skiptrailing(const mTCHAR* str, const mTCHAR* base)
{
    assert(str != NULL);
    assert(base != NULL);
    while (str > base && *(str - 1) <= ' ')
        str--;
    return (mTCHAR*)str;
}

static mTCHAR* striptrailing(mTCHAR* str)
{
    mTCHAR* ptr = skiptrailing(_tcschr(str, '\0'), str);
    assert(ptr != NULL);
    *ptr = '\0';
    return str;
}

static mTCHAR* save_strncpy(mTCHAR* dest, const mTCHAR* source, size_t maxlen, enum quote_option option)
{
    size_t d, s;

    assert(maxlen > 0);
    if (option == QUOTE_ENQUOTE && maxlen < 3)
        option = QUOTE_NONE; /* cannot store two quotes and a terminating zero in less than 3 characters */

    switch (option)
    {
        case QUOTE_NONE:
            for (d = 0; d < maxlen - 1 && source[d] != '\0'; d++)
                dest[d] = source[d];
            assert(d < maxlen);
            dest[d] = '\0';
            break;
        case QUOTE_ENQUOTE:
            d = 0;
            dest[d++] = '"';
            for (s = 0; source[s] != '\0' && d < maxlen - 2; s++, d++)
            {
                if (source[s] == '"')
                {
                    if (d >= maxlen - 3)
                        break; /* no space to store the escape character plus the one that follows it */
                    dest[d++] = '\\';
                } /* if */
                dest[d] = source[s];
            } /* for */
            dest[d++] = '"';
            dest[d] = '\0';
            break;
        case QUOTE_DEQUOTE:
            for (d = s = 0; source[s] != '\0' && d < maxlen - 1; s++, d++)
            {
                if ((source[s] == '"' || source[s] == '\\') && source[s + 1] == '"')
                    s++;
                dest[d] = source[s];
            } /* for */
            dest[d] = '\0';
            break;
        default:
            assert(0);
    } /* switch */

    return dest;
}

static int getkeystring(INI_FILETYPE* fp, const mTCHAR* Section, const mTCHAR* Key, int idxSection, int idxKey,
                        mTCHAR* Buffer, int BufferSize)
{
    mTCHAR *sp, *ep;
    int len, idx, isstring = 0;
    enum quote_option quotes;
    mTCHAR LocalBuffer[INI_BUFFERSIZE];

    assert(fp != NULL);
    /* Move through file 1 line at a time until a section is matched or EOF. If
     * parameter Section is NULL, only look at keys above the first section. If
     * idxSection is postive, copy the relevant section name.
     */
    len = (Section != NULL) ? _tcslen(Section) : 0;
    if (len > 0 || idxSection >= 0)
    {
        idx = -1;
        do
        {
            if (!ini_read(LocalBuffer, INI_BUFFERSIZE, fp))
                return 0;
            sp = skipleading(LocalBuffer);
            ep = _tcschr(sp, ']');
        } while (*sp != '[' || ep == NULL ||
                 (((int)(ep - sp - 1) != len || _tcsnicmp(sp + 1, Section, len) != 0) && ++idx != idxSection));
        if (idxSection >= 0)
        {
            if (idx == idxSection)
            {
                assert(ep != NULL);
                assert(*ep == ']');
                *ep = '\0';
                save_strncpy(Buffer, sp + 1, BufferSize, QUOTE_NONE);
                return 1;
            }         /* if */
            return 0; /* no more section found */
        }             /* if */
    }                 /* if */

    /* Now that the section has been found, find the entry.
     * Stop searching upon leaving the section's area.
     */
    assert(Key != NULL || idxKey >= 0);
    len = (Key != NULL) ? (int)_tcslen(Key) : 0;
    idx = -1;
    do
    {
        if (!ini_read(LocalBuffer, INI_BUFFERSIZE, fp) || *(sp = skipleading(LocalBuffer)) == '[')
            return 0;
        sp = skipleading(LocalBuffer);
        ep = _tcschr(sp, '='); /* Parse out the equal sign */
        if (ep == NULL)
            ep = _tcschr(sp, ':');
    } while (*sp == ';' || *sp == '#' || ep == NULL ||
             (((int)(skiptrailing(ep, sp) - sp) != len || _tcsnicmp(sp, Key, len) != 0) && ++idx != idxKey));
    if (idxKey >= 0)
    {
        if (idx == idxKey)
        {
            assert(ep != NULL);
            assert(*ep == '=' || *ep == ':');
            *ep = '\0';
            striptrailing(sp);
            save_strncpy(Buffer, sp, BufferSize, QUOTE_NONE);
            return 1;
        }         /* if */
        return 0; /* no more key found (in this section) */
    }             /* if */

    /* Copy up to BufferSize chars to buffer */
    assert(ep != NULL);
    assert(*ep == '=' || *ep == ':');
    sp = skipleading(ep + 1);
    /* Remove a trailing comment */
    isstring = 0;
    for (ep = sp; *ep != '\0' && ((*ep != ';' && *ep != '#') || isstring); ep++)
    {
        if (*ep == '"')
        {
            if (*(ep + 1) == '"')
                ep++; /* skip "" (both quotes) */
            else
                isstring = !isstring; /* single quote, toggle isstring */
        }
        else if (*ep == '\\' && *(ep + 1) == '"')
        {
            ep++; /* skip \" (both quotes */
        }         /* if */
    }             /* for */
    assert(ep != NULL && (*ep == '\0' || *ep == ';' || *ep == '#'));
    *ep = '\0'; /* terminate at a comment */
    striptrailing(sp);
    /* Remove double quotes surrounding a value */
    quotes = QUOTE_NONE;
    if (*sp == '"' && (ep = _tcschr(sp, '\0')) != NULL && *(ep - 1) == '"')
    {
        sp++;
        *--ep = '\0';
        quotes = QUOTE_DEQUOTE; /* this is a string, so remove escaped characters */
    }                           /* if */
    save_strncpy(Buffer, sp, BufferSize, quotes);
    return 1;
}

/** ini_gets()
 * \param Section     the name of the section to search for
 * \param Key         the name of the entry to find the value of
 * \param DefValue    default string in the event of a failed read
 * \param Buffer      a pointer to the buffer to copy into
 * \param BufferSize  the maximum number of characters to copy
 * \param Filename    the name and full path of the .ini file to read from
 *
 * \return            the number of characters copied into the supplied buffer
 */
int ini_gets(const mTCHAR* Section, const mTCHAR* Key, const mTCHAR* DefValue, mTCHAR* Buffer, int BufferSize,
             const mTCHAR* Filename)
{
    INI_FILETYPE fp;
    int ok = 0;

    if (Buffer == NULL || BufferSize <= 0 || Key == NULL)
        return 0;
    if (ini_openread(Filename, &fp))
    {
        ok = getkeystring(&fp, Section, Key, -1, -1, Buffer, BufferSize);
        ini_close(&fp);
    } /* if */
    if (!ok)
        save_strncpy(Buffer, DefValue, BufferSize, QUOTE_NONE);
    return _tcslen(Buffer);
}

/** ini_getl()
 * \param Section     the name of the section to search for
 * \param Key         the name of the entry to find the value of
 * \param DefValue    the default value in the event of a failed read
 * \param Filename    the name of the .ini file to read from
 *
 * \return            the value located at Key
 */
long ini_getl(const mTCHAR* Section, const mTCHAR* Key, long DefValue, const mTCHAR* Filename)
{
    mTCHAR buff[64];
    int len = ini_gets(Section, Key, __T(""), buff, sizearray(buff), Filename);
    return (len == 0) ? DefValue
                      : ((len >= 2 && _totupper(buff[1]) == 'X') ? _tcstol(buff, NULL, 16) : _tcstol(buff, NULL, 10));
}

#if 0
/** ini_getf()
 * \param Section     the name of the section to search for
 * \param Key         the name of the entry to find the value of
 * \param DefValue    the default value in the event of a failed read
 * \param Filename    the name of the .ini file to read from
 *
 * \return            the value located at Key
 */
double ini_getf(const mTCHAR *Section, const mTCHAR *Key, double DefValue, const mTCHAR *Filename)
{
  mTCHAR buff[64];
  int len = ini_gets(Section, Key, __T(""), buff, sizearray(buff), Filename);
  return (len == 0) ? DefValue : _tcstod(buff,NULL);
}
#endif

/** ini_getbool()
 * \param Section     the name of the section to search for
 * \param Key         the name of the entry to find the value of
 * \param DefValue    default value in the event of a failed read; it should
 *                    zero (0) or one (1).
 * \param Buffer      a pointer to the buffer to copy into
 * \param BufferSize  the maximum number of characters to copy
 * \param Filename    the name and full path of the .ini file to read from
 *
 A true boolean is found if one of the following is matched:

  - A string starting with 'y'
  - A string starting with 'Y'
  - A string starting with 't'
  - A string starting with 'T'
  - A string starting with '1'

  A false boolean is found if one of the following is matched:

  - A string starting with 'n'
  - A string starting with 'N'
  - A string starting with 'f'
  - A string starting with 'F'
  - A string starting with '0'
 *
 * \return            the true/false flag as interpreted at Key
 */
int ini_getbool(const mTCHAR* Section, const mTCHAR* Key, int DefValue, const mTCHAR* Filename)
{
    mTCHAR buff[2];
    // int ret;

    ini_gets(Section, Key, __T(""), buff, sizearray(buff), Filename);
    /*
      buff[0] = toupper(buff[0]);
      if (buff[0]=='Y' || buff[0]=='1' || buff[0]=='T')
        ret = 1;
      else if (buff[0]=='N' || buff[0]=='0' || buff[0]=='F')
        ret = 0;
      else
        ret = DefValue;

      return(ret);
    */
    switch (buff[0])
    {
        case 'y':
        case 'Y':
        case 't':
        case 'T':
        case '1':
            return 1;
        case 'n':
        case 'N':
        case 'f':
        case 'F':
        case '0':
            return 0;
    }
    return !!DefValue;
}

/** ini_getsection()
 * \param idx         the zero-based sequence number of the section to return
 * \param Buffer      a pointer to the buffer to copy into
 * \param BufferSize  the maximum number of characters to copy
 * \param Filename    the name and full path of the .ini file to read from
 *
 * \return            the number of characters copied into the supplied buffer
 */
int ini_getsection(int idx, mTCHAR* Buffer, int BufferSize, const mTCHAR* Filename)
{
    INI_FILETYPE fp;
    int ok = 0;

    if (Buffer == NULL || BufferSize <= 0 || idx < 0)
        return 0;
    if (ini_openread(Filename, &fp))
    {
        ok = getkeystring(&fp, NULL, NULL, idx, -1, Buffer, BufferSize);
        ini_close(&fp);
    } /* if */
    if (!ok)
        *Buffer = '\0';
    return _tcslen(Buffer);
}

/** ini_getkey()
 * \param Section     the name of the section to browse through, or NULL to
 *                    browse through the keys outside any section
 * \param idx         the zero-based sequence number of the key to return
 * \param Buffer      a pointer to the buffer to copy into
 * \param BufferSize  the maximum number of characters to copy
 * \param Filename    the name and full path of the .ini file to read from
 *
 * \return            the number of characters copied into the supplied buffer
 */
int ini_getkey(const mTCHAR* Section, int idx, mTCHAR* Buffer, int BufferSize, const mTCHAR* Filename)
{
    INI_FILETYPE fp;
    int ok = 0;

    if (Buffer == NULL || BufferSize <= 0 || idx < 0)
        return 0;
    if (ini_openread(Filename, &fp))
    {
        ok = getkeystring(&fp, Section, NULL, -1, idx, Buffer, BufferSize);
        ini_close(&fp);
    } /* if */
    if (!ok)
        *Buffer = '\0';
    return _tcslen(Buffer);
}

#if !defined INI_READONLY
static void ini_tempname(mTCHAR* dest, const mTCHAR* source, int maxlength)
{
    mTCHAR* p;

    save_strncpy(dest, source, maxlength, QUOTE_NONE);
    p = _tcsrchr(dest, '\0');
    assert(p != NULL);
    *(p - 1) = '~';
}

static enum quote_option check_enquote(const mTCHAR* Value)
{
    const mTCHAR* p;

    /* run through the value, if it has trailing spaces, or '"', ';' or '#'
     * characters, enquote it
     */
    assert(Value != NULL);
    for (p = Value; *p != '\0' && *p != '"' && *p != ';' && *p != '#'; p++)
        /* nothing */;
    return (*p != '\0' || (p > Value && *(p - 1) == ' ')) ? QUOTE_ENQUOTE : QUOTE_NONE;
}

static void writesection(mTCHAR* LocalBuffer, const mTCHAR* Section, INI_FILETYPE* fp)
{
    mTCHAR* p;

    if (Section != NULL && _tcslen(Section) > 0)
    {
        LocalBuffer[0] = '[';
        save_strncpy(LocalBuffer + 1, Section, INI_BUFFERSIZE - 4,
                     QUOTE_NONE); /* -1 for '[', -1 for ']', -2 for '\r\n' */
        p = _tcsrchr(LocalBuffer, '\0');
        assert(p != NULL);
        *p++ = ']';
        _tcscpy(p, INI_LINETERM); /* copy line terminator (typically "\n") */
        ini_write(LocalBuffer, fp);
    } /* if */
}

static void writekey(mTCHAR* LocalBuffer, const mTCHAR* Key, const mTCHAR* Value, INI_FILETYPE* fp)
{
    mTCHAR* p;
    enum quote_option option = check_enquote(Value);
    save_strncpy(LocalBuffer, Key, INI_BUFFERSIZE - 3, QUOTE_NONE); /* -1 for '=', -2 for '\r\n' */
    p = _tcsrchr(LocalBuffer, '\0');
    assert(p != NULL);
    *p++ = '=';
    save_strncpy(p, Value, INI_BUFFERSIZE - (p - LocalBuffer) - 2, option); /* -2 for '\r\n' */
    p = _tcsrchr(LocalBuffer, '\0');
    assert(p != NULL);
    _tcscpy(p, INI_LINETERM); /* copy line terminator (typically "\n") */
    ini_write(LocalBuffer, fp);
}

static void write_quoted(const mTCHAR* Value, INI_FILETYPE* fp)
{
    mTCHAR s[3];
    int idx;
    if (check_enquote(Value) == QUOTE_NONE)
    {
        ini_write(Value, fp);
    }
    else
    {
        ini_write("\"", fp);
        for (idx = 0; Value[idx] != '\0'; idx++)
        {
            if (Value[idx] == '"')
            {
                s[0] = '\\';
                s[1] = Value[idx];
                s[2] = '\0';
            }
            else
            {
                s[0] = Value[idx];
                s[1] = '\0';
            } /* if */
            ini_write(s, fp);
        } /* for */
        ini_write("\"", fp);
    } /* if */
}

/** ini_puts()
 * \param Section     the name of the section to write the string in
 * \param Key         the name of the entry to write, or NULL to erase all keys in the section
 * \param Value       a pointer to the buffer the string, or NULL to erase the key
 * \param Filename    the name and full path of the .ini file to write to
 *
 * \return            1 if successful, otherwise 0
 */
int ini_puts(const mTCHAR* Section, const mTCHAR* Key, const mTCHAR* Value, const mTCHAR* Filename)
{
    INI_FILETYPE rfp;
    INI_FILETYPE wfp;
    mTCHAR *sp, *ep;
    mTCHAR LocalBuffer[INI_BUFFERSIZE];
    int len, match, count;

    assert(Filename != NULL);
    INI_LINETERM = LF;
    if (!ini_openread(Filename, &rfp))
    {
        /* If the .ini file doesn't exist, make a new file */
        if (Key != NULL && Value != NULL)
        {
            if (!ini_openwrite(Filename, &wfp))
                return 0;
            writesection(LocalBuffer, Section, &wfp);
            writekey(LocalBuffer, Key, Value, &wfp);
            ini_close(&wfp);
        } /* if */
        return 1;
    } /* if */

    memset(LocalBuffer, 0, sizeof(LocalBuffer));
    ini_read(LocalBuffer, INI_BUFFERSIZE, &rfp);
    if (_tcslen(LocalBuffer) > 1 && LocalBuffer[_tcslen(LocalBuffer) - 2] == '\r')
        INI_LINETERM = CRLF;
    ini_rewind(&rfp);
    /* If parameters Key and Value are valid (so this is not an "erase" request)
     * and the setting already exists and it already has the correct value, do
     * nothing. This early bail-out avoids rewriting the INI file for no reason.
     */
    if (Key != NULL && Value != NULL)
    {
        match = getkeystring(&rfp, Section, Key, -1, -1, LocalBuffer, sizearray(LocalBuffer));
        if (match && _tcscmp(LocalBuffer, Value) == 0)
        {
            ini_close(&rfp);
            return 1;
        } /* if */
        /* key not found, or different value -> proceed (but rewind the input file first) */
        ini_rewind(&rfp);
    } /* if */

    /* Get a temporary file name to copy to. Use the existing name, but with
     * the last character set to a '~'.
     */
    ini_tempname(LocalBuffer, Filename, INI_BUFFERSIZE);
    if (!ini_openwrite(LocalBuffer, &wfp))
    {
        ini_close(&rfp);
        return 0;
    } /* if */

    /* Move through the file one line at a time until a section is
     * matched or until EOF. Copy to temp file as it is read.
     */
    count = 0;
    len = (Section != NULL) ? _tcslen(Section) : 0;
    if (len > 0)
    {
        do
        {
            if (!ini_read(LocalBuffer, INI_BUFFERSIZE, &rfp))
            {
                /* Failed to find section, so add one to the end */
                if (Key != NULL && Value != NULL)
                {
                    ini_write(
                        INI_LINETERM,
                        &wfp); /* force a new line (there may not have been one) behind the last line of the INI file */
                    writesection(LocalBuffer, Section, &wfp);
                    writekey(LocalBuffer, Key, Value, &wfp);
                } /* if */
                /* Clean up and rename */
                ini_close(&rfp);
                ini_close(&wfp);
                ini_remove(Filename);
                ini_tempname(LocalBuffer, Filename, INI_BUFFERSIZE);
                ini_rename(LocalBuffer, Filename);
                return 1;
            } /* if */
            /* Copy the line from source to dest, but not if this is the section that
             * we are looking for and this section must be removed
             */
            sp = skipleading(LocalBuffer);
            ep = _tcschr(sp, ']');
            match = (*sp == '[' && ep != NULL && (int)(ep - sp - 1) == len && _tcsnicmp(sp + 1, Section, len) == 0);
            if (!match || Key != NULL)
            {
                /* Remove blank lines, but insert a blank line (possibly one that was
                 * removed on the previous iteration) before a new section. This creates
                 * "neat" INI files.
                 */
                if (_tcslen(sp) > 0)
                {
                    if (*sp == '[' && count > 0)
                        ini_write(INI_LINETERM, &wfp);
                    ini_write(sp, &wfp);
                    count++;
                } /* if */
            }     /* if */
        } while (!match);
    } /* if */

    /* Now that the section has been found, find the entry. Stop searching
     * upon leaving the section's area. Copy the file as it is read
     * and create an entry if one is not found.
     */
    len = (Key != NULL) ? _tcslen(Key) : 0;
    for (;;)
    {
        if (!ini_read(LocalBuffer, INI_BUFFERSIZE, &rfp))
        {
            /* EOF without an entry so make one */
            if (Key != NULL && Value != NULL)
            {
                ini_write(
                    INI_LINETERM,
                    &wfp); /* force a new line (there may not have been one) behind the last line of the INI file */
                writekey(LocalBuffer, Key, Value, &wfp);
            } /* if */
            /* Clean up and rename */
            ini_close(&rfp);
            ini_close(&wfp);
            ini_remove(Filename);
            ini_tempname(LocalBuffer, Filename, INI_BUFFERSIZE);
            ini_rename(LocalBuffer, Filename);
            return 1;
        } /* if */
        sp = skipleading(LocalBuffer);
        ep = _tcschr(sp, '='); /* Parse out the equal sign */
        if (ep == NULL)
            ep = _tcschr(sp, ':');
        match = (ep != NULL && (int)(skiptrailing(ep, sp) - sp) == len && _tcsnicmp(sp, Key, len) == 0);
        if ((Key != NULL && match) || *sp == '[')
            break; /* found the key, or found a new section */
        /* in the section that we re-write, do not copy empty lines */
        if (Key != NULL && _tcslen(sp) > 0)
            ini_write(sp, &wfp);
    } /* for */
    if (*sp == '[')
    {
        /* found start of new section, the key was not in the specified
         * section, so we add it just before the new section
         */
        if (Key != NULL && Value != NULL)
        {
            /* We cannot use "writekey()" here, because we need to preserve the
             * contents of LocalBuffer.
             */
            ini_write(Key, &wfp);
            ini_write("=", &wfp);
            write_quoted(Value, &wfp);
            ini_write(INI_LINETERM, &wfp); /* put a blank line between the current and the next section */
            ini_write(INI_LINETERM, &wfp);
        } /* if */
        /* write the new section header that we read previously */
        ini_write(sp, &wfp);
    }
    else
    {
        /* We found the key; ignore the line just read (with the key and
         * the current value) and write the key with the new value.
         */
        if (Key != NULL && Value != NULL)
            writekey(LocalBuffer, Key, Value, &wfp);
    } /* if */
    /* Copy the rest of the INI file (removing empty lines, except before a section) */
    while (ini_read(LocalBuffer, INI_BUFFERSIZE, &rfp))
    {
        sp = skipleading(LocalBuffer);
        if (_tcslen(sp) > 0)
        {
            if (*sp == '[')
                ini_write(INI_LINETERM, &wfp);
            ini_write(sp, &wfp);
        } /* if */
    }     /* while */
    /* Clean up and rename */
    ini_close(&rfp);
    ini_close(&wfp);
    ini_remove(Filename);
    ini_tempname(LocalBuffer, Filename, INI_BUFFERSIZE);
    ini_rename(LocalBuffer, Filename);
    return 1;
}

/* Ansi C "itoa" based on Kernighan & Ritchie's "Ansi C" book. */
#define ABS(v) ((v) < 0 ? -(v) : (v))

static void strreverse(mTCHAR* str)
{
    mTCHAR t;
    int i, j;

    for (i = 0, j = _tcslen(str) - 1; i < j; i++, j--)
    {
        t = str[i];
        str[i] = str[j];
        str[j] = t;
    } /* for */
}

static void long2str(long value, mTCHAR* str)
{
    int i = 0;
    long sign = value;
    int n;

    /* generate digits in reverse order */
    do
    {
        n = (int)(value % 10);             /* get next lowest digit */
        str[i++] = (mTCHAR)(ABS(n) + '0'); /* handle case of negative digit */
    } while (value /= 10);                 /* delete the lowest digit */
    if (sign < 0)
        str[i++] = '-';
    str[i] = '\0';

    strreverse(str);
}

/** ini_putl()
 * \param Section     the name of the section to write the value in
 * \param Key         the name of the entry to write
 * \param Value       the value to write
 * \param Filename    the name and full path of the .ini file to write to
 *
 * \return            1 if successful, otherwise 0
 */
int ini_putl(const mTCHAR* Section, const mTCHAR* Key, long Value, const mTCHAR* Filename)
{
    mTCHAR buff[32];
    long2str(Value, buff);
    return ini_puts(Section, Key, buff, Filename);
}

/** ini_putf()
 * \param Section     the name of the section to write the value in
 * \param Key         the name of the entry to write
 * \param Value       the value to write
 * \param Filename    the name and full path of the .ini file to write to
 *
 * \return            1 if successful, otherwise 0
 */
int ini_putf(const mTCHAR* Section, const mTCHAR* Key, double Value, const mTCHAR* Filename)
{
    mTCHAR buff[64];
    _stprintf(buff, __T("%lf"), Value);
    return ini_puts(Section, Key, buff, Filename);
}
#endif /* !INI_READONLY */
