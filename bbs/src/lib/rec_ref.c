#include "dao.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>


int
rec_ref(fpath, data, size, pos, fchk, fref)
  char *fpath;
  void *data;
  int size, pos;
  int (*fchk)();
  void (*fref)();
{
  int fd;
  off_t off, len;
  char pool[REC_SIZ];
  struct stat st;

  if ((fd = open(fpath, O_RDWR, 0600)) < 0)
    return -1;

  /* flock(fd, LOCK_EX); */
  /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
  f_exlock(fd);

  fstat(fd, &st);
  len = st.st_size;

  fpath = pool;
  off = size * pos;

  /* 驗證 pos 位置資料的正確性 */

  if (len > off)
  {
    lseek(fd, off, SEEK_SET);
    read(fd, fpath, size);
    pos = fchk ? (*fchk) (fpath) : 1;
  }
  else
  {
    if (len)
    {
      pos = 0;	/* 從頭找起 */
    }
    else
    {
      /* 若原本是空檔案，那麼放棄 */
      f_unlock(fd);
      close(fd);
      return -1;
    }
  }

  /* 不對的話，重頭找起 */

  if (!pos)
  {
    off = 0;
    lseek(fd, off, SEEK_SET);
    while (read(fd, fpath, size) == size)
    {
      if (pos = (*fchk) (fpath))
	break;

      off += size;
    }
  }

  /* 找到之後，更新資料 */

  if (pos)
  {
    (*fref) (fpath, data);
    lseek(fd, off, SEEK_SET);
    write(fd, fpath, size);
  }

  /* flock(fd, LOCK_UN); */
  /* Thor.981205: 用 fcntl 取代flock, POSIX標準用法 */
  f_unlock(fd);

  close(fd);

  return 0;
}
