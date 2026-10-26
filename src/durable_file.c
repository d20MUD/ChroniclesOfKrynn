#define _POSIX_C_SOURCE 200809L
#include "durable_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef LEGENDARY_TRANSACTION_TEST
int durable_file_fault;
#define FAIL(n) (durable_file_fault == (n))
#else
#define FAIL(n) 0
#endif
bool durable_file_replace(const char *path, const void *data, size_t size)
{
  char *temporary, *parent, *slash;
  int fd=-1, directory=-1;
  bool ok=false;
  size_t used=0;
  ssize_t written;
  struct stat old;
  if (!path || !*path || (!data && size)) return false;
  temporary=malloc(strlen(path)+16); parent=strdup(path);
  if (!temporary || !parent) goto done;
  sprintf(temporary,"%s.tmp.XXXXXX",path);
  slash=strrchr(parent,'/');
  if (!slash) strcpy(parent,".");
  else if (slash==parent) slash[1]='\0';
  else *slash='\0';
  directory=open(parent,O_RDONLY|O_DIRECTORY);
  if (directory<0 || FAIL(1)) goto done;
  fd=mkstemp(temporary);
  if (fd<0) goto done;
  if (stat(path,&old)==0 && fchmod(fd,old.st_mode & 0777)) goto done;
  while (used<size) {
    written=write(fd,(const char *)data+used,size-used);
    if (written<0 && errno==EINTR) continue;
    if (written<=0 || FAIL(2)) goto done;
    used+=(size_t)written;
  }
  if (FAIL(3) || fsync(fd)) goto done;
  if (close(fd)) {fd=-1; goto done;} fd=-1;
  if (FAIL(4) || rename(temporary,path)) goto done;
  if (FAIL(5) || fsync(directory)) goto done;
  ok=true;
done:
  if (fd>=0) close(fd);
  if (directory>=0) close(directory);
  if (temporary) {unlink(temporary); free(temporary);}
  free(parent); return ok;
}
