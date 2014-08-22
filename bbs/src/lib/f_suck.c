#include "dao.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>


void
f_suck(fp, fpath)
  FILE *fp;
  char *fpath;
{
  int fd;

  if ((fd = open(fpath, O_RDONLY)) >= 0)
  {
    char pool[BLK_SIZ];
    int size;

    fpath = pool;
    while ((size = read(fd, fpath, BLK_SIZ)) > 0)
    {
      fwrite(fpath, size, 1, fp);
    }
    close(fd);
  }
}
