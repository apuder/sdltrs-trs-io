
#ifndef USE_SOCKET_IO

#include "fileio.h"
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#define AM_DIR 0x10

void f_log(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

FRESULT f_open (
  FIL* fp,           /* [OUT] Pointer to the file object structure */
  const TCHAR* path, /* [IN] File name */
  BYTE mode          /* [IN] Mode flags */
                ) {
  char* m = "";
  switch(mode) {
  case FA_READ:
    m = "r";
    break;
  case FA_READ | FA_WRITE:
    m = "r+";
    break;
  case FA_CREATE_ALWAYS | FA_WRITE:
    m = "w";
    break;
  case FA_CREATE_ALWAYS | FA_WRITE | FA_READ:
    m = "w+";
    break;
  case FA_OPEN_APPEND | FA_WRITE:
    m = "a";
    break;
  case FA_OPEN_APPEND | FA_WRITE | FA_READ:
    m = "a+";
    break;
  case  FA_CREATE_NEW | FA_WRITE:
    m = "wx";
    break;
  case FA_CREATE_NEW | FA_WRITE | FA_READ:
    m = "w+x";
    break;
  default:
    assert(0);
  }
  fp->f = fopen(path, m);
  return (fp->f == NULL) ? FR_NO_FILE : FR_OK;
}

FRESULT f_opendir (
  DIR_* dp,           /* [OUT] Pointer to the directory object structure */
  const TCHAR* path  /* [IN] Directory name */
                   ) {
  path = "."; // XXX
  dp->dir = opendir(path);
  return (dp->dir != NULL) ? FR_OK : FR_DISK_ERR;
}

FRESULT f_write (
  FIL* fp,          /* [IN] Pointer to the file object structure */
  const void* buff, /* [IN] Pointer to the data to be written */
  UINT btw,         /* [IN] Number of bytes to write */
  UINT* bw          /* [OUT] Pointer to the variable to return number of bytes written */
                 ) {
  *bw = fwrite(buff, 1, btw, fp->f);
  return FR_OK;
}

FRESULT f_read (
  FIL* fp,     /* [IN] File object */
  void* buff,  /* [OUT] Buffer to store read data */
  UINT btr,    /* [IN] Number of bytes to read */
  UINT* br     /* [OUT] Number of bytes read */
                ) {
  *br = fread(buff, 1, btr, fp->f);

  return FR_OK;
}

FRESULT f_readdir (
  DIR_* dp,      /* [IN] Directory object */
  FILINFO* fno  /* [OUT] File information structure */
                   ) {
  FILE* f;

  while (1) {
    struct dirent* entry = readdir(dp->dir);
    if (entry == NULL) {
      closedir(dp->dir);
      fno->fname[0] = '\0';
      break;
    }
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    if (strlen(entry->d_name) > 12) {
      continue;
    }
    strcpy(fno->fname, entry->d_name);
    f = fopen(fno->fname, "rb");
    fseek(f, 0L, SEEK_END);
    fno->fsize = ftell(f);
    fclose(f);
    fno->fattrib = 1;
    break;
  }
  return FR_OK;
}

FSIZE_t f_tell (
  FIL* fp   /* [IN] File object */
                ) {
  assert(0);
}

FRESULT f_sync (
  FIL* fp     /* [IN] File object */
                ) {
  assert(0);
}

FRESULT f_lseek (
  FIL*    fp,  /* [IN] File object */
  FSIZE_t ofs  /* [IN] File read/write pointer */
                 ) {
  fseek(fp->f, ofs, SEEK_SET);
  return FR_OK;
}

FRESULT f_close (
  FIL* fp     /* [IN] Pointer to the file object */
                 ) {
  fclose(fp->f);
  return FR_OK;
}

FRESULT f_unlink (
        const TCHAR* path  /* [IN] Object name */
) {
    return (remove(path) == 0) ? FR_OK : FR_NO_FILE;
}

FRESULT f_stat (
        const TCHAR* path,  /* [IN] Object name */
        FILINFO* fno        /* [OUT] FILINFO structure */
) {
  FILE* f = fopen(path, "r");
  if (f == NULL) {
    return FR_NO_FILE;
  }
  strcpy(fno->fname, path);
  fseek(f, 0L, SEEK_END);
  fno->fsize = ftell(f);
  fclose(f);
  struct stat path_stat;
  stat(path, &path_stat);
  fno->fattrib = 0;
  if (S_ISDIR(path_stat.st_mode)) {
    fno->fattrib |= AM_DIR;
  }
  return FR_OK;
}

void init_socket_io() {
  // Do nothing
}

#endif

