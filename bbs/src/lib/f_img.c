#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>


char *
f_img(fpath, fsize)
  char *fpath;
  int *fsize;
{
  int fd, size;
  struct stat st;

  if ((fd = open(fpath, O_RDONLY)) < 0)
    return NULL;

  fpath = NULL;

  if (!fstat(fd, &st) && S_ISREG(st.st_mode) && (size = st.st_size) > 0 &&
    (fpath = (char *) malloc(size)))
  {
    *fsize = size;
    if (read(fd, fpath, size) != size)
    {
      free(fpath);
      fpath = NULL;
    }
  }

  close(fd);
  return fpath;
}
