#include "dao.h"
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>


static int
is_bottompost(hdr)
  HDR *hdr;
{
  return (hdr->xmode & POST_BOTTOM);
}


int
rec_bot(fpath, data, size)	/* amaki.040715: 碠Α糶郎 */
  char *fpath;
  void *data;
  int size;
{
  int fd, fsize, count;
  void *pool, *set;
  char set_pool[REC_SIZ];
  struct stat st;

  if ((fd = open(fpath, O_RDWR | O_CREAT, 0600)) < 0)
    return -1;

  /* flock(fd, LOCK_EX); */
  /* Thor.981205: ノ fcntl flock, POSIX夹非ノ猭 */
  f_exlock(fd);

  fstat(fd, &st);

  count = 0;
  set = (void *) set_pool;

  if (fsize = st.st_size)
  {
    while ((fsize -= size) >= 0)
    {
      lseek(fd, fsize, SEEK_SET);
      read(fd, set, size);

      if (!is_bottompost(set))
      {
	if (count)
	{
	  pool = (void *) malloc(count * size);

	  read(fd, pool, count * size);
	  lseek(fd, -size * count, SEEK_CUR);
	}
	break;
      }
      else if (fsize <= 0)	/* amaki.040715: 场常琌竚┏狥﹁ */
      {
	count++;
	pool = (void *) malloc(count * size);

	lseek(fd, -size, SEEK_CUR);
	read(fd, pool, count * size);
	lseek(fd, -size * count, SEEK_CUR);
	break;
      }
      else
	count++;
    }
  }

  write(fd, data, size);

  if (count)
  {
    write(fd, pool, count * size);
    free(pool);
  }

  /* flock(fd, LOCK_EX); */
  /* Thor.981205: ノ fcntl flock, POSIX夹非ノ猭 */
  f_unlock(fd);

  close(fd);

  return 0;
}
