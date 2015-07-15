/*****************************************************************************

				WWIV Version 4
                    Copyright (C) 1988-1995 by Wayne Bell

Distribution of the source code for WWIV, in any form, modified or unmodified,
without PRIOR, WRITTEN APPROVAL by the author, is expressly prohibited.
Distribution of compiled versions of WWIV is limited to copies compiled BY
THE AUTHOR.  Distribution of any copies of WWIV not compiled by the author
is expressly prohibited.


*****************************************************************************/

#include <stdio.h>
#include <io.h>
#include <conio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <process.h>
#include <sys\stat.h>
#include <dos.h>
#include <dir.h>
#include <alloc.h>
#include <share.h>
#include <errno.h>

#include "share.h"


#ifdef __OS2__
#define INCL_DOSPROCESS
#include <os2.h>
#define delay(ms) DosSleep((ULONG) ms)
#endif /* __OS2__ */



#define SHARE_LEVEL 10
#define WAIT_TIME 10
#define TRIES 100


/* debug levels:

   0 turns all debug operations off

   1 or greater shows file information if the file must be waited upon.

   2 or greater shows file information when an attempt is made to open a file.

   3 or greater shows file information BEFORE any attempt is made to open
     a file.

   4 or greater waits for key from console with each file open.

*/

int sh_open(char *path, int file_access, unsigned mode)
{
  int handle, count, share;
  char drive[MAXDRIVE], dir[MAXDIR], file[MAXFILE], ext[MAXEXT];

  if ((file_access & O_RDWR) || (file_access & O_WRONLY)  || (mode & S_IWRITE)) {
    share=SH_DENYRW;
  } else {
    share=SH_DENYWR;
  }
  handle=open(path,file_access|share,mode);
  if (handle < 0) {
    count=1;
    fnsplit(path,drive,dir,file,ext);
    if (access(path,0)!=-1) {
      delay(WAIT_TIME);
      handle=open(path,file_access|share,mode);
      while (((handle < 0) && (errno == EACCES)) && (count < TRIES)) {
        if (count%2)
          delay(WAIT_TIME);
        count++;
        handle=open(path,file_access|share,mode);
      }
    }
  }
  return(handle);
}


int sh_open1(char *path, int access)
{
  unsigned mode;

  mode=0;
  if ((access & O_RDWR) || (access & O_WRONLY))
    mode |=S_IWRITE;
  if ((access & O_RDWR) || (access & O_RDONLY))
    mode |=S_IREAD;
  return(sh_open(path, access, mode));
}


FILE *fsh_open(char *path, char *mode)
{
  FILE *f;
  int count,share, md, fd;
  char drive[MAXDRIVE], dir[MAXDIR], file[MAXFILE], ext[MAXEXT];

  share=SH_DENYWR;
  md=0;
  if (strchr(mode,'w')!=NULL) {
    share=SH_DENYRD;
    md= O_RDWR|O_CREAT|O_TRUNC;
  } else if (strchr(mode,'a')!=NULL) {
    share=SH_DENYRD;
    md= O_RDWR|O_CREAT;
  } else {
    md= O_RDONLY;
  }
  if (strchr(mode,'b')!=NULL) {
    md |= O_BINARY;
  }
  if (strchr(mode,'+')!=NULL) {
    md&= ~O_RDONLY;
    md|= O_RDWR;
    share=SH_DENYRD;
  }
  fd=open(path, md|share, S_IREAD|S_IWRITE);
  if (fd<0) {
    count=1;
    fnsplit(path,drive,dir,file,ext);
    if (access(path,0)!=-1) {
      delay(WAIT_TIME);
      fd=open(path, md|share, S_IREAD|S_IWRITE);
      while (((fd < 0) && (errno == EACCES)) && (count < TRIES)) {
        delay(WAIT_TIME);
	  count++;
        fd=open(path, md|share, S_IREAD|S_IWRITE);
      }
    }
  }
  if (fd>0) {
    if (strchr(mode,'a'))
      lseek(fd, 0L, SEEK_END);

    f=fdopen(fd,mode);
    if (!f) {
      close(fd);
    }
  } else
    f=0;
  return(f);
}


int sh_close(int f)
{
  if (f!=-1)
    close(f);
  return(-1);
}


void fsh_close(FILE *f)
{
  fclose(f);
}


void share_installed(void)
/* detects if SHARE is installed - only in DOS versions 4 or later */
/* returns FALSE if DOS version less than 4 or not installed */
{
#ifndef __OS2__
  if (_osmajor >= 3) {
    _AX=0x1000;
    geninterrupt(0x2f);
  } else {
    printf("\r\nIncorrect DOS version.\r\n");
    exit(SHARE_LEVEL);
  }
  if (_AL == 0xFF) {
    return;
  }
  if (_AL == 0x01) {
    printf("\r\nShare can not be loaded.\r\n");
    exit(SHARE_LEVEL);
  }
  if (_AL == 0x00) {
    printf("\r\nShare should be INSTALLED in your config.sys.\r\n");
    exit(SHARE_LEVEL);
  }
  printf("\r\nUnexpected result from SHARE TEST %d.\r\n",_AL);
  exit(SHARE_LEVEL);
#endif
}

long sh_lseek(int handle, long offset, int fromwhere)
{

  if (handle == -1) {
    return(-1L);
  }
  return(lseek(handle, offset, fromwhere));
}


int sh_read(int handle, void *buf, unsigned len)
{

  if (handle == -1) {
    return(-1);
  }
  return(read(handle, buf, len));
}


int sh_write(int handle, void *buf, unsigned len)
{

  if (handle == -1) {
    return(-1);
  }
  return(write(handle, buf, len));
}


size_t fsh_read(void *ptr, size_t size, size_t n, FILE *stream)
{

  if (stream == NULL) {
    return(0);
  }
  return(fread(ptr, size, n, stream));
}


size_t fsh_write(void *ptr, size_t size, size_t n, FILE *stream)
{

  if (stream == NULL) {
    return(0);
  }
  return(fwrite(ptr, size, n, stream));
}

