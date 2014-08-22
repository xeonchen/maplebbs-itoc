#include <fcntl.h>


int
rec_add(fpath, data, size)
  char *fpath;
  void *data;
  int size;
{
  int fd;

  if ((fd = open(fpath, O_WRONLY | O_CREAT | O_APPEND, 0600)) < 0)
    return -1;

  /* flock(fd, LOCK_EX); */
  write(fd, data, size);
  /* flock(fd, LOCK_UN); */
  close(fd);

  return 0;
}
