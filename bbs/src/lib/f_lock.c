#include <unistd.h>
#include <fcntl.h>

static struct flock fl = 
{
  .l_whence = SEEK_SET,
  .l_start = 0,
  .l_len = 0,
};


int
f_exlock(fd)
  int fd;
{
#if 0
  return flock(fd, LOCK_EX); 
#endif
  /* Thor.981205: 用 fcntl 取代 flock，POSIX 標準用法 */
  fl.l_type = F_WRLCK;
  /* Thor.990309: with blocking */
  return fcntl(fd, F_SETLKW /* F_SETLK */, &fl);
}


int
f_unlock(fd)
  int fd;
{
#if 0
  return flock(fd, LOCK_UN); 
#endif
  /* Thor.981205: 用 fcntl 取代 flock，POSIX 標準用法 */
  fl.l_type = F_UNLCK;
  return fcntl(fd, F_SETLKW /* F_SETLK */, &fl);
}
