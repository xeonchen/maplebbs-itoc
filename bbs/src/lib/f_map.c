#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>


#ifdef MAP_FILE         /* 44BSD defines this & requires it to mmap files */
#  define DAO_MAP       (MAP_SHARED | MAP_FILE)
#else
#  define DAO_MAP       (MAP_SHARED)
#endif


char *
f_map(fpath, fsize)
  char *fpath;
  int *fsize;
{
  int fd, size;
  struct stat st;

  if ((fd = open(fpath, O_RDONLY)) < 0)
    return (char *) -1;

  if (fstat(fd, &st) || !S_ISREG(st.st_mode) || (size = st.st_size) <= 0)
  {
    close(fd);
    return (char *) -1;
  }

  fpath = (char *) mmap(NULL, size, PROT_READ, DAO_MAP, fd, 0);
  close(fd);
  *fsize = size;
  return fpath;
}
