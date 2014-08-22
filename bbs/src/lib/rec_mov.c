#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>


int
rec_mov(fpath, size, from, to)
  char *fpath;
  int size;
  int from;
  int to;
{
  int fd, backward;
  off_t off, len;
  char *pool;
  struct stat st;

  if ((fd = open(fpath, O_RDWR)) < 0)
    return -1;

  /* flock(fd, LOCK_EX); */
  /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
  f_exlock(fd);

  fstat(fd, &st);
  len = st.st_size / size - 1;

  if (from > to)
  {
    backward = from;
    from = to;
    to = backward;
    backward = 1;
  }
  else
  {
    backward = 0;
  }

  if (to >= len)
    to = len;

  off = size * from;
  lseek(fd, off, SEEK_SET);

  len = (to - from + 1) * size;
  pool = fpath = (char *) malloc(len + size);

  if (backward)
    fpath += size;
  read(fd, fpath, len);

  fpath = pool + len;
  if (backward)
    memcpy(pool, fpath, size);
  else
    memcpy(fpath, pool, size);

  fpath = pool;
  if (!backward)
    fpath += size;

  lseek(fd, off, SEEK_SET);
  write(fd, fpath, len);

  /* flock(fd, LOCK_UN); */
  /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
  f_unlock(fd);

  close(fd);
  free(pool);

  return 0;
}
